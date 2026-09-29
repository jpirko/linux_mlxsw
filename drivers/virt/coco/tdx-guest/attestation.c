// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/sha2.h>
#include <linux/acpi.h>
#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/unaligned.h>

#include <uapi/linux/tdx-guest.h>

#include "ctlv_attestation_tdx_gen.h"
#include "internal.h"

struct tdx_attestation {
	struct attestation_dev att;
	const struct tdx_guest_info *info;
	void __iomem *log;
	size_t log_length;
};

static struct tdx_attestation *tdx_att;

static struct tdx_attestation *to_tdx_attestation(struct attestation_dev *att)
{
	return container_of(att, struct tdx_attestation, att);
}

static const enum hash_algo tdx_attestation_algorithms[] = {
	HASH_ALGO_SHA384,
};

static const struct attestation_mr_props tdx_attestation_mrs[] = {
	{
		.id = 0,
		.name = "mrtd",
		.algorithms = tdx_attestation_algorithms,
		.algorithms_count = ARRAY_SIZE(tdx_attestation_algorithms),
	},
	{
		.id = 1,
		.name = "rtmr0",
		.algorithms = tdx_attestation_algorithms,
		.algorithms_count = ARRAY_SIZE(tdx_attestation_algorithms),
		.has_register_index = true,
		.register_index = 0,
		.extendable = true,
	},
	{
		.id = 2,
		.name = "rtmr1",
		.algorithms = tdx_attestation_algorithms,
		.algorithms_count = ARRAY_SIZE(tdx_attestation_algorithms),
		.has_register_index = true,
		.register_index = 1,
		.extendable = true,
	},
	{
		.id = 3,
		.name = "rtmr2",
		.algorithms = tdx_attestation_algorithms,
		.algorithms_count = ARRAY_SIZE(tdx_attestation_algorithms),
		.has_register_index = true,
		.register_index = 2,
		.extendable = true,
	},
	{
		.id = 4,
		.name = "rtmr3",
		.algorithms = tdx_attestation_algorithms,
		.algorithms_count = ARRAY_SIZE(tdx_attestation_algorithms),
		.has_register_index = true,
		.register_index = 3,
		.extendable = true,
	},
};

#define TDX_QUOTE_HEADER_LEN 48
#define TDX_QUOTE_TEE_TYPE_TDX 0x81

static const struct attestation_evidence_format tdx_attestation_quote_formats[] = {
	{ .vendor = true, .value = CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE },
	{ .vendor = true, .value = CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE_DICE },
};

static const struct attestation_evidence_format tdx_attestation_log_formats[] = {
	{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL },
};

static const struct attestation_evidence_challenge_length
tdx_attestation_challenge_lengths[] = {
	{ .min = TDX_REPORTDATA_LEN, .max = TDX_REPORTDATA_LEN },
};

/* The log comes last, so a guest without a CCEL table can leave it out. */
static struct attestation_evidence_supported_object
tdx_attestation_evidence_objects[] = {
	{
		.evidence_class = ATTESTATION_EVIDENCE_SIGNED,
		.formats = tdx_attestation_quote_formats,
		.formats_count = ARRAY_SIZE(tdx_attestation_quote_formats),
		.challenge_lengths = tdx_attestation_challenge_lengths,
		.challenge_lengths_count =
			ARRAY_SIZE(tdx_attestation_challenge_lengths),
		.max_length = TDX_GUEST_QUOTE_MAX_LEN,
		.binds_mrs = true,
	},
	{
		.evidence_class = ATTESTATION_EVIDENCE_EVENT_LOG,
		.formats = tdx_attestation_log_formats,
		.formats_count = ARRAY_SIZE(tdx_attestation_log_formats),
	},
};

/* The object count and the total length are set at init. */
static struct attestation_evidence_caps tdx_attestation_evidence_caps = {
	.objects = tdx_attestation_evidence_objects,
};

/*
 * A DCAP Quote's header starts with its version, 4 or 5, and TEE type. A
 * DICE-based Quote, a CBOR Web Token, starts with the CBOR magic, d9 d9 f7.
 */
static int tdx_attestation_quote_label(const u8 *quote, size_t length,
				       struct attestation_evidence_format *format)
{
	format->vendor = true;
	if (length >= TDX_QUOTE_HEADER_LEN &&
	    (get_unaligned_le16(quote) == 4 || get_unaligned_le16(quote) == 5) &&
	    get_unaligned_le32(quote + 4) == TDX_QUOTE_TEE_TYPE_TDX) {
		format->value = CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE;
		return 0;
	}
	if (length >= 3 && !memcmp(quote, "\xd9\xd9\xf7", 3)) {
		format->value = CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE_DICE;
		return 0;
	}
	return -EPROTO;
}

static void __iomem *tdx_attestation_log_map(size_t *length)
{
	struct acpi_table_header *header;
	struct acpi_table_ccel *ccel;
	void __iomem *log = NULL;

	if (ACPI_FAILURE(acpi_get_table(ACPI_SIG_CCEL, 0, &header)))
		return NULL;
	ccel = container_of(header, struct acpi_table_ccel, header);
	if (header->length >= sizeof(*ccel) && ccel->log_area_start_address &&
	    ccel->log_area_minimum_length &&
	    ccel->log_area_minimum_length <= UINT_MAX)
		log = acpi_os_map_iomem(ccel->log_area_start_address,
					ccel->log_area_minimum_length);
	if (log)
		*length = ccel->log_area_minimum_length;
	acpi_put_table(header);
	return log;
}

static int
tdx_attestation_evidence_get(struct attestation_dev *att,
			     const struct attestation_evidence_request *request,
			     struct attestation_evidence_result *result)
{
	struct tdx_attestation *tdx = to_tdx_attestation(att);
	size_t index, length;
	void *buffer;
	int ret = 0;

	for (index = 0; !ret && index < request->objects_count; index++) {
		const struct attestation_evidence_object_request *object =
			&request->objects[index];
		struct attestation_evidence_object_result *produced =
			&result->objects[index];

		buffer = kvmalloc(object->supported_object->max_length,
				  GFP_KERNEL_ACCOUNT);
		if (!buffer)
			return -ENOMEM;

		if (object->supported_object == &tdx_attestation_evidence_objects[0]) {
			ret = tdx_guest_quote_get(request->challenge, buffer,
						  &length);
			if (!ret)
				ret = tdx_attestation_quote_label(buffer, length,
								  &produced->format);
		} else {
			memcpy_fromio(buffer, tdx->log, tdx->log_length);
			length = tdx->log_length;
			produced->format = tdx_attestation_log_formats[0];
		}

		if (!ret)
			ret = attestation_evidence_write(object, buffer, length);
		if (!ret)
			produced->length = length;
		kvfree(buffer);
	}
	return ret;
}

static int tdx_attestation_mr_read(struct attestation_dev *att, unsigned int mr_id,
				   struct attestation_digest_set *values)
{
	static const enum tdx_guest_mr native_mrs[] = {
		TDX_GUEST_MR_MRTD,
		TDX_GUEST_MR_RTMR0,
		TDX_GUEST_MR_RTMR1,
		TDX_GUEST_MR_RTMR2,
		TDX_GUEST_MR_RTMR3,
	};

	if (mr_id >= ARRAY_SIZE(native_mrs) ||
	    values->values_count != 1 ||
	    values->values[0].algorithm != HASH_ALGO_SHA384)
		return -EINVAL;
	return tdx_guest_mr_read(native_mrs[mr_id], values->values[0].data);
}

static int
tdx_attestation_mr_extend(struct attestation_dev *att, unsigned int mr_id,
			  const struct attestation_digest_set *values)
{
	if (mr_id < 1 || mr_id > 4)
		return -EOPNOTSUPP;
	if (values->values_count != 1 ||
	    values->values[0].algorithm != HASH_ALGO_SHA384)
		return -EINVAL;
	return tdx_guest_rtmr_extend(mr_id - 1, values->values[0].data);
}

static const struct attestation_provider tdx_attestation_provider = {
	.evidence_get = tdx_attestation_evidence_get,
	.mr_read = tdx_attestation_mr_read,
	.mr_extend = tdx_attestation_mr_extend,
	.evidence_caps = &tdx_attestation_evidence_caps,
	.mrs = tdx_attestation_mrs,
	.mrs_count = ARRAY_SIZE(tdx_attestation_mrs),
};

int ctlv_attestation_tdx_device_info_fill(struct ctlv *ctlv,
					  struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *record)
{
	const struct tdx_guest_info *info =
		to_tdx_attestation(attestation_from_ctlv(ctlv))->info;
	bool ok;

	ok = ctlv_attestation_tdx_device_info_mrconfigid_put(record, info->mrconfigid,
							     sizeof(info->mrconfigid));
	ok &= ctlv_attestation_tdx_device_info_mrowner_put(record, info->mrowner,
							   sizeof(info->mrowner));
	ok &= ctlv_attestation_tdx_device_info_mrownerconfig_put(record,
								 info->mrownerconfig,
								 sizeof(info->mrownerconfig));
	return ok ? 0 : -EMSGSIZE;
}

int tdx_attestation_register(struct device *parent,
			     const struct tdx_guest_info *info)
{
	struct attestation_evidence_caps *caps = &tdx_attestation_evidence_caps;
	struct tdx_attestation *tdx;
	size_t log_length = 0;
	void __iomem *log;
	int ret;

	log = tdx_attestation_log_map(&log_length);
	tdx_attestation_evidence_objects[1].max_length = log_length;
	caps->objects_count = ARRAY_SIZE(tdx_attestation_evidence_objects);
	if (!log)
		caps->objects_count--;

	tdx = attestation_alloc(parent, &tdx_attestation_provider,
				&ctlv_attestation_tdx_family, struct tdx_attestation, att);
	if (IS_ERR(tdx)) {
		ret = PTR_ERR(tdx);
		goto err_unmap;
	}
	tdx->info = info;
	tdx->log = log;
	tdx->log_length = log_length;
	ret = attestation_register(&tdx->att);
	if (ret) {
		attestation_put(&tdx->att);
		goto err_unmap;
	}
	tdx_att = tdx;
	return 0;

err_unmap:
	if (log)
		acpi_os_unmap_iomem(log, log_length);
	return ret;
}

void tdx_attestation_unregister(void)
{
	if (!tdx_att)
		return;
	attestation_unregister(&tdx_att->att);
	if (tdx_att->log)
		acpi_os_unmap_iomem(tdx_att->log, tdx_att->log_length);
	attestation_put(&tdx_att->att);
	tdx_att = NULL;
}

MODULE_IMPORT_NS("ATTESTATION");
MODULE_IMPORT_NS("CTLV");
