// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/sha2.h>
#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/tdx-guest.h>

#include "../generated/ctlv_attestation_tdx_gen.h"

struct attestation_tdx_init_info {
	struct device *parent_dev;
	const struct tdx_guest_ops *ops;
};

struct attestation_tdx {
	struct device *parent_dev;
	const struct tdx_guest_ops *ops;
};

static const enum hash_algo attestation_tdx_algorithms[] = {
	HASH_ALGO_SHA384,
};

static const struct attestation_mr_props attestation_tdx_mrs[] = {
	{
		.id = 0,
		.name = "mrtd",
		.algorithms = attestation_tdx_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_tdx_algorithms),
		.readable = true,
	},
	{
		.id = 1,
		.name = "rtmr0",
		.algorithms = attestation_tdx_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_tdx_algorithms),
		.has_register_index = true,
		.register_index = 0,
		.readable = true,
		.extendable = true,
	},
	{
		.id = 2,
		.name = "rtmr1",
		.algorithms = attestation_tdx_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_tdx_algorithms),
		.has_register_index = true,
		.register_index = 1,
		.readable = true,
		.extendable = true,
	},
	{
		.id = 3,
		.name = "rtmr2",
		.algorithms = attestation_tdx_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_tdx_algorithms),
		.has_register_index = true,
		.register_index = 2,
		.readable = true,
		.extendable = true,
	},
	{
		.id = 4,
		.name = "rtmr3",
		.algorithms = attestation_tdx_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_tdx_algorithms),
		.has_register_index = true,
		.register_index = 3,
		.readable = true,
		.extendable = true,
	},
};

static const struct attestation_evidence_supported_object
attestation_tdx_evidence_objects[] = {
	{
		.format = "tdx-tdreport-1.0",
		.max_length = TDX_GUEST_REPORT_LEN,
		.binds_mrs = true,
	},
	{
		.format = "tdx-quote-1.0",
		.max_length = TDX_GUEST_QUOTE_MAX_LEN,
		.binds_mrs = true,
	},
};

static const struct attestation_evidence_caps attestation_tdx_evidence_caps = {
	.min_challenge_length = TDX_GUEST_REPORTDATA_LEN,
	.max_challenge_length = TDX_GUEST_REPORTDATA_LEN,
	.max_total_output_length = TDX_GUEST_QUOTE_MAX_LEN,
	.objects = attestation_tdx_evidence_objects,
	.objects_count = ARRAY_SIZE(attestation_tdx_evidence_objects),
};

static int
attestation_tdx_evidence_get(struct attestation *att,
			     const struct attestation_evidence_request *request,
			     struct attestation_evidence_result *result)
{
	const struct attestation_evidence_object_request *object;
	struct attestation_tdx *tdx = ctlv_priv_child(att);
	size_t length;
	void *buffer;
	int ret;

	if (request->objects_count != 1)
		return -EOPNOTSUPP;
	object = &request->objects[0];
	buffer = kvmalloc(object->supported_object->max_length, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	if (object->supported_object == &attestation_tdx_evidence_objects[0])
		ret = tdx->ops->report_get(tdx->parent_dev,
			request->challenge,
			request->challenge_length, buffer,
			object->supported_object->max_length, &length);
	else if (object->supported_object ==
		 &attestation_tdx_evidence_objects[1])
		ret = tdx->ops->quote_get(tdx->parent_dev,
			request->challenge,
			request->challenge_length, buffer,
			object->supported_object->max_length, &length);
	else
		ret = -EOPNOTSUPP;
	if (!ret)
		ret = ctlv_blob_write(object->output, 0, buffer, length);
	if (!ret)
		result->object_lengths[0] = length;
	kvfree(buffer);
	return ret;
}

static int attestation_tdx_mr_read(struct attestation *att, unsigned int mr_id,
				   struct attestation_digest_set *values)
{
	static const enum tdx_guest_mr native_mrs[] = {
		TDX_GUEST_MR_MRTD,
		TDX_GUEST_MR_RTMR0,
		TDX_GUEST_MR_RTMR1,
		TDX_GUEST_MR_RTMR2,
		TDX_GUEST_MR_RTMR3,
	};
	struct attestation_tdx *tdx = ctlv_priv_child(att);

	if (mr_id >= ARRAY_SIZE(native_mrs) ||
	    values->values_count != 1 ||
	    values->values[0].algorithm != HASH_ALGO_SHA384)
		return -EINVAL;
	return tdx->ops->mr_read(tdx->parent_dev, native_mrs[mr_id],
		values->values[0].data);
}

static int
attestation_tdx_mr_extend(struct attestation *att, unsigned int mr_id,
			  const struct attestation_digest_set *values)
{
	struct attestation_tdx *tdx = ctlv_priv_child(att);

	if (mr_id < 1 || mr_id > 4)
		return -EOPNOTSUPP;
	if (values->values_count != 1 ||
	    values->values[0].algorithm != HASH_ALGO_SHA384)
		return -EINVAL;
	return tdx->ops->rtmr_extend(tdx->parent_dev, mr_id - 1,
				     values->values[0].data);
}

static const struct attestation_provider attestation_tdx_provider = {
	.evidence_get = attestation_tdx_evidence_get,
	.mr_read = attestation_tdx_mr_read,
	.mr_extend = attestation_tdx_mr_extend,
	.evidence_caps = &attestation_tdx_evidence_caps,
	.mrs = attestation_tdx_mrs,
	.mrs_count = ARRAY_SIZE(attestation_tdx_mrs),
};

size_t ctlv_attestation_tdx_priv_size(void)
{
	return sizeof(struct attestation_tdx);
}

const void *ctlv_attestation_tdx_pre_init(const void *init_info)
{
	return &attestation_tdx_provider;
}

int ctlv_attestation_tdx_init(struct ctlv_device *ctlvdev,
			      const void *init_info, void *priv)
{
	const struct attestation_tdx_init_info *info = init_info;
	struct attestation_tdx *tdx = priv;

	if (!info || !info->parent_dev || !info->ops)
		return -EINVAL;
	tdx->parent_dev = info->parent_dev;
	tdx->ops = info->ops;
	return 0;
}

void ctlv_attestation_tdx_fini(struct ctlv_device *ctlvdev, void *priv)
{
}

int ctlv_attestation_tdx_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return ctlv_attestation_op_pre(ctlv_priv_parent(priv), ctx);
}

void ctlv_attestation_tdx_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_attestation_op_post(ctlv_priv_parent(priv), ctx);
}

int ctlv_attestation_tdx_device_info_fill(void *priv,
					  struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *nest)
{
	struct attestation_tdx *tdx = priv;
	const struct tdx_guest_info *info =
		tdx->ops->info_get(tdx->parent_dev);
	bool ok;

	if (!info)
		return -ENODEV;
	ok = ctlv_attestation_tdx_info_mrconfigid_put(nest, info->mrconfigid,
						      sizeof(info->mrconfigid));
	ok &= ctlv_attestation_tdx_info_mrowner_put(nest, info->mrowner,
						    sizeof(info->mrowner));
	ok &= ctlv_attestation_tdx_info_mrownerconfig_put(nest,
							   info->mrownerconfig,
							   sizeof(info->mrownerconfig));
	return ok ? 0 : -EMSGSIZE;
}

void ctlv_attestation_tdx_device_info_read(void *priv,
					   struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *nest)
{
}

static void *attestation_tdx_add(struct device *parent_dev,
				 const struct tdx_guest_ops *ops)
{
	struct attestation_tdx_init_info info = {
		.parent_dev = parent_dev,
		.ops = ops,
	};

	return ctlv_create(parent_dev, &ctlv_attestation_tdx_family,
			   &info);
}

static void attestation_tdx_remove(void *client_priv)
{
	ctlv_destroy(client_priv);
}

static const struct tdx_guest_client attestation_tdx_client = {
	.add = attestation_tdx_add,
	.remove = attestation_tdx_remove,
};

static int __init attestation_tdx_init_module(void)
{
	return tdx_guest_client_register(&attestation_tdx_client);
}

static void __exit attestation_tdx_exit_module(void)
{
	tdx_guest_client_unregister(&attestation_tdx_client);
}

module_init(attestation_tdx_init_module);
module_exit(attestation_tdx_exit_module);
MODULE_DESCRIPTION("TDX attestation provider");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("ATTESTATION");
MODULE_IMPORT_NS("CTLV");
MODULE_IMPORT_NS("TDX_GUEST");
