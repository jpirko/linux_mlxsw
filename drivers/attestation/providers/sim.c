// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash.h>
#include <crypto/hash_info.h>
#include <linux/attestation.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_sim_gen.h"

#define ATTESTATION_SIM_NAME_MAX 63
#define ATTESTATION_SIM_COMMAND_MAX 256
#define ATTESTATION_SIM_EVIDENCE_MAX 4096
#define ATTESTATION_SIM_DYNAMIC_MRS_MAX 16

enum attestation_sim_callback {
	ATTESTATION_SIM_CB_NONE = 0,
	ATTESTATION_SIM_CB_EVIDENCE_GET = 1,
	ATTESTATION_SIM_CB_EVIDENCE_ACCEPT = 2,
	ATTESTATION_SIM_CB_MR_READ = 3,
	ATTESTATION_SIM_CB_MR_EXTEND = 4,
	ATTESTATION_SIM_CB_TDISP_STATE_GET = 5,
	ATTESTATION_SIM_CB_COUNT = 6,
};

struct attestation_sim_fixture;

struct attestation_sim_mr {
	struct attestation_mr_props props;
	enum hash_algo *algorithms;
	struct attestation_digest *values;
	struct attestation_mr *handle;
};

struct attestation_sim {
	struct attestation_dev att;
	struct attestation_sim_fixture *fixture;
	/* Protects simulated hardware and dynamic register state. */
	struct mutex hardware_lock;
	struct xarray dynamic_mrs;
	struct attestation_sim_mr fixed[2];
	enum attestation_tdisp_state tdisp_state;
	bool notification_pending;
};

struct attestation_sim_fixture {
	struct list_head node;
	/* Serializes fixture controls and destruction. */
	struct mutex control_lock;
	/* Protects failure and pause injection state. */
	spinlock_t callback_lock;
	wait_queue_head_t callback_wait;
	atomic_t callback_count[ATTESTATION_SIM_CB_COUNT];
	enum attestation_sim_callback fail_next;
	int fail_error;
	enum attestation_sim_callback pause_next;
	enum attestation_sim_callback paused;
	bool pause_release;
	bool stopping;
	bool dead;
	char name[ATTESTATION_SIM_NAME_MAX + 1];
	struct device *parent;
	struct attestation_sim *sim;
	struct dentry *debugfs_dir;
};

static DEFINE_MUTEX(attestation_sim_fixtures_lock);
static LIST_HEAD(attestation_sim_fixtures);
static struct dentry *attestation_sim_debugfs;
static char *early_fixture;
module_param(early_fixture, charp, 0400);
MODULE_PARM_DESC(early_fixture,
		 "create one built-in fixture before late initcalls");

static const enum hash_algo attestation_sim_mr0_algorithms[] = {
	HASH_ALGO_SHA384,
};

static const enum hash_algo attestation_sim_mr1_algorithms[] = {
	HASH_ALGO_SHA256,
	HASH_ALGO_SHA384,
};

static const struct attestation_mr_props attestation_sim_mrs[] = {
	{
		.id = 0,
		.name = "sim-read-only",
		.algorithms = attestation_sim_mr0_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_sim_mr0_algorithms),
		.has_register_index = true,
		.register_index = 0,
	},
	{
		.id = 1,
		.name = "sim-extendable",
		.algorithms = attestation_sim_mr1_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_sim_mr1_algorithms),
		.has_register_index = true,
		.register_index = 1,
		.extendable = true,
	},
};

/* The objects' IDs, their places in attestation_sim_evidence_objects. */
enum attestation_sim_object {
	ATTESTATION_SIM_OBJECT_REPORT,
	ATTESTATION_SIM_OBJECT_TOKEN,
	ATTESTATION_SIM_OBJECT_LOG,
	ATTESTATION_SIM_OBJECT_MANIFEST,
};

static const struct attestation_evidence_format attestation_sim_report_formats[] = {
	{ .vendor = true, .value = CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_REPORT },
};

static const struct attestation_evidence_format attestation_sim_token_formats[] = {
	{ .vendor = true, .value = CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_TOKEN },
};

static const struct attestation_evidence_format attestation_sim_log_formats[] = {
	{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL },
};

static const struct attestation_evidence_format attestation_sim_manifest_formats[] = {
	{ .vendor = true, .value = CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_MANIFEST },
};

/* A range and an exact length, so that userspace sees both forms. */
static const struct attestation_evidence_challenge_length
attestation_sim_report_challenge_lengths[] = {
	{ .min = 1, .max = 64 },
	{ .min = 128, .max = 128 },
};

/* Within the report's, so that a challenge may suit both or only the report. */
static const struct attestation_evidence_challenge_length
attestation_sim_token_challenge_lengths[] = {
	{ .min = 32, .max = 32 },
};

/* A TCG crypto-agile event log of its Spec ID event alone, for SHA-384. */
static const u8 attestation_sim_log[] = {
	/* TCG_PCClientPCREvent: PCR 0, EV_NO_ACTION, a zero SHA-1 digest. */
	0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	33, 0x00, 0x00, 0x00,
	/* TCG_EfiSpecIdEvent: 2.0, UINT64, TPM_ALG_SHA384 of 48 bytes. */
	'S', 'p', 'e', 'c', ' ', 'I', 'D', ' ',
	'E', 'v', 'e', 'n', 't', '0', '3', 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x02,
	0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 48, 0x00,
	0x00,
};

static const struct attestation_evidence_supported_object
attestation_sim_evidence_objects[] = {
	[ATTESTATION_SIM_OBJECT_REPORT] = {
		.evidence_class = ATTESTATION_EVIDENCE_SIGNED,
		.formats = attestation_sim_report_formats,
		.formats_count = ARRAY_SIZE(attestation_sim_report_formats),
		.challenge_lengths = attestation_sim_report_challenge_lengths,
		.challenge_lengths_count =
			ARRAY_SIZE(attestation_sim_report_challenge_lengths),
		.has_tcb_digest = true,
		.tcb_digest_algorithm = HASH_ALGO_SHA384,
		.max_length = ATTESTATION_SIM_EVIDENCE_MAX,
		.binds_mrs = true,
	},
	[ATTESTATION_SIM_OBJECT_TOKEN] = {
		.evidence_class = ATTESTATION_EVIDENCE_SIGNED,
		.formats = attestation_sim_token_formats,
		.formats_count = ARRAY_SIZE(attestation_sim_token_formats),
		.challenge_lengths = attestation_sim_token_challenge_lengths,
		.challenge_lengths_count =
			ARRAY_SIZE(attestation_sim_token_challenge_lengths),
		.max_length = 1024,
	},
	[ATTESTATION_SIM_OBJECT_LOG] = {
		.evidence_class = ATTESTATION_EVIDENCE_EVENT_LOG,
		.formats = attestation_sim_log_formats,
		.formats_count = ARRAY_SIZE(attestation_sim_log_formats),
		.max_length = sizeof(attestation_sim_log),
	},
	[ATTESTATION_SIM_OBJECT_MANIFEST] = {
		.evidence_class = ATTESTATION_EVIDENCE_AUXILIARY,
		.formats = attestation_sim_manifest_formats,
		.formats_count = ARRAY_SIZE(attestation_sim_manifest_formats),
		.has_instance = true,
		.instance = 1,
		.max_length = 1024,
	},
};

static const struct attestation_evidence_caps attestation_sim_evidence_caps = {
	.supports_mr_selection = true,
	.supports_privlevel = true,
	.supports_service_attestation = true,
	.objects = attestation_sim_evidence_objects,
	.objects_count = ARRAY_SIZE(attestation_sim_evidence_objects),
};

static const struct attestation_mr_caps attestation_sim_mr_caps = {
	.supports_dynamic_topology = true,
};

static const char * const attestation_sim_callback_names[] = {
	[ATTESTATION_SIM_CB_EVIDENCE_GET] = "evidence_get",
	[ATTESTATION_SIM_CB_EVIDENCE_ACCEPT] = "evidence_accept",
	[ATTESTATION_SIM_CB_MR_READ] = "mr_read",
	[ATTESTATION_SIM_CB_MR_EXTEND] = "mr_extend",
	[ATTESTATION_SIM_CB_TDISP_STATE_GET] = "tdisp_state_get",
};

static enum attestation_sim_callback
attestation_sim_callback_parse(const char *name)
{
	enum attestation_sim_callback callback;

	for (callback = 1; callback < ATTESTATION_SIM_CB_COUNT; callback++)
		if (!strcmp(name, attestation_sim_callback_names[callback]))
			return callback;
	return ATTESTATION_SIM_CB_NONE;
}

static int attestation_sim_callback_enter(struct attestation_sim *sim,
					  enum attestation_sim_callback callback)
{
	struct attestation_sim_fixture *fixture = sim->fixture;
	bool pause = false;
	int error = 0;

	atomic_inc(&fixture->callback_count[callback]);
	scoped_guard(spinlock, &fixture->callback_lock) {
		if (fixture->fail_next == callback) {
			error = fixture->fail_error;
			fixture->fail_next = ATTESTATION_SIM_CB_NONE;
		}
		if (!error && fixture->pause_next == callback) {
			fixture->pause_next = ATTESTATION_SIM_CB_NONE;
			fixture->paused = callback;
			fixture->pause_release = false;
			pause = true;
		}
	}
	if (error)
		return error;
	if (pause) {
		wait_event(fixture->callback_wait,
			   READ_ONCE(fixture->pause_release) ||
			   READ_ONCE(fixture->stopping));
		scoped_guard(spinlock, &fixture->callback_lock)
			fixture->paused = ATTESTATION_SIM_CB_NONE;
	}
	return READ_ONCE(fixture->stopping) ? -ENODEV : 0;
}

static struct attestation_sim *attestation_sim_from_att(struct attestation_dev *att)
{
	return container_of(att, struct attestation_sim, att);
}

static struct attestation_sim_mr *
attestation_sim_mr_lookup(struct attestation_sim *sim, unsigned int id)
{
	struct attestation_sim_mr *mr;

	if (id < ARRAY_SIZE(sim->fixed))
		return &sim->fixed[id];
	mr = xa_load(&sim->dynamic_mrs, id);
	return mr && mr->handle ? mr : NULL;
}

static int attestation_sim_mr_init(struct attestation_sim_mr *mr,
				   const struct attestation_mr_props *props)
{
	size_t index;

	mr->props = *props;
	mr->values = kcalloc(props->algorithms_count,
			     sizeof(*mr->values), GFP_KERNEL);
	if (!mr->values)
		return -ENOMEM;
	for (index = 0; index < props->algorithms_count; index++)
		mr->values[index].algorithm = props->algorithms[index];
	return 0;
}

static void attestation_sim_mr_fini(struct attestation_sim_mr *mr)
{
	kfree(mr->values);
	kfree(mr->algorithms);
	kfree(mr->props.name);
	kfree(mr);
}

static int attestation_sim_digest(enum hash_algo algorithm,
				  const u8 *first, size_t first_length,
				  const u8 *second, size_t second_length,
				  u8 *digest)
{
	struct crypto_shash *tfm;
	struct shash_desc *desc;
	int ret;

	tfm = crypto_alloc_shash(hash_algo_name[algorithm], 0, 0);
	if (IS_ERR(tfm))
		return PTR_ERR(tfm);
	desc = kzalloc(sizeof(*desc) + crypto_shash_descsize(tfm), GFP_KERNEL);
	if (!desc) {
		crypto_free_shash(tfm);
		return -ENOMEM;
	}
	desc->tfm = tfm;
	ret = crypto_shash_init(desc);
	if (!ret)
		ret = crypto_shash_update(desc, first, first_length);
	if (!ret)
		ret = crypto_shash_finup(desc, second, second_length, digest);
	kfree(desc);
	crypto_free_shash(tfm);
	return ret;
}

static int attestation_sim_mr_read(struct attestation_dev *att, unsigned int mr_id,
				   const struct ctlv_cursor *mr_options,
				   struct attestation_digest_set *values)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	struct attestation_sim_mr *mr;
	size_t index;
	int ret;

	ret = attestation_sim_callback_enter(sim, ATTESTATION_SIM_CB_MR_READ);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	mr = attestation_sim_mr_lookup(sim, mr_id);
	if (!mr)
		return -ENOENT;
	if (values->values_count != mr->props.algorithms_count)
		return -EINVAL;
	for (index = 0; index < values->values_count; index++) {
		if (values->values[index].algorithm !=
		    mr->values[index].algorithm)
			return -EINVAL;
		memcpy(values->values[index].data, mr->values[index].data,
		       hash_digest_size[mr->values[index].algorithm]);
	}
	return 0;
}

static int attestation_sim_mr_extend(struct attestation_dev *att,
				     unsigned int mr_id,
				     const struct ctlv_cursor *mr_options,
				     const struct attestation_digest_set *values)
{
	struct attestation_digest changed[HASH_ALGO__LAST];
	struct attestation_sim *sim = attestation_sim_from_att(att);
	DECLARE_BITMAP(extended, HASH_ALGO__LAST) = {};
	struct attestation_sim_mr *mr;
	unsigned long bank;
	size_t index;
	int ret;

	ret = attestation_sim_callback_enter(sim, ATTESTATION_SIM_CB_MR_EXTEND);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	mr = attestation_sim_mr_lookup(sim, mr_id);
	if (!mr)
		return -ENOENT;
	for (index = 0; index < values->values_count; index++) {
		const struct attestation_digest *value = &values->values[index];
		size_t length = hash_digest_size[value->algorithm];

		for (bank = 0; bank < mr->props.algorithms_count; bank++)
			if (mr->values[bank].algorithm == value->algorithm)
				break;
		if (bank == mr->props.algorithms_count ||
		    __test_and_set_bit(bank, extended))
			return -EINVAL;
		changed[bank].algorithm = value->algorithm;
		ret = attestation_sim_digest(value->algorithm,
					     mr->values[bank].data, length,
					     value->data, length,
					     changed[bank].data);
		if (ret)
			return ret;
	}
	for_each_set_bit(bank, extended, HASH_ALGO__LAST)
		mr->values[bank] = changed[bank];
	return 0;
}

struct attestation_sim_buffer {
	u8 data[ATTESTATION_SIM_EVIDENCE_MAX];
	size_t length;
};

static int attestation_sim_buffer_put(struct attestation_sim_buffer *buffer,
				      const void *data, size_t length)
{
	if (length > sizeof(buffer->data) - buffer->length)
		return -E2BIG;
	if (!length)
		return 0;
	memcpy(buffer->data + buffer->length, data, length);
	buffer->length += length;
	return 0;
}

static int attestation_sim_buffer_u32(struct attestation_sim_buffer *buffer,
				      u32 value)
{
	__le32 encoded = cpu_to_le32(value);

	return attestation_sim_buffer_put(buffer, &encoded, sizeof(encoded));
}

static int
attestation_sim_report(struct attestation_sim *sim,
		       const struct attestation_evidence_request *request,
		       struct attestation_sim_buffer *buffer)
{
	struct attestation_sim_mr *mr;
	unsigned long mr_id;
	size_t selection;
	int ret;

	ret = attestation_sim_buffer_put(buffer, "SIM-REPORT-v1", 13);
	ret |= attestation_sim_buffer_u32(buffer, request->challenge_length);
	ret |= attestation_sim_buffer_put(
		buffer, request->challenge, request->challenge_length);
	ret |= attestation_sim_buffer_u32(buffer, request->has_privlevel);
	ret |= attestation_sim_buffer_u32(buffer, request->privlevel);
	if (request->service_provider) {
		ret |= attestation_sim_buffer_u32(
			buffer, strlen(request->service_provider));
		ret |= attestation_sim_buffer_put(
			buffer, request->service_provider,
			strlen(request->service_provider));
		ret |= attestation_sim_buffer_put(
			buffer, &request->service_guid,
			sizeof(request->service_guid));
		ret |= attestation_sim_buffer_u32(
			buffer, request->service_manifest_version);
	} else {
		ret |= attestation_sim_buffer_u32(buffer, 0);
	}
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	if (request->selection_mrs_count) {
		ret = attestation_sim_buffer_u32(
			buffer, request->selection_mrs_count);
		for (selection = 0;
		     !ret && selection < request->selection_mrs_count;
		     selection++) {
			const struct attestation_evidence_selection_mr *selected =
				&request->selection_mrs[selection];
			size_t index;

			mr = attestation_sim_mr_lookup(sim, selected->mr_id);
			if (!mr)
				return -ENOENT;
			for (index = 0; index < mr->props.algorithms_count;
			     index++)
				if (mr->values[index].algorithm ==
				    selected->hash_algorithm)
					break;
			if (index == mr->props.algorithms_count)
				return -EINVAL;
			ret = attestation_sim_buffer_u32(
				buffer, selected->mr_id);
			ret |= attestation_sim_buffer_u32(
				buffer, selected->hash_algorithm);
			ret |= attestation_sim_buffer_put(
				buffer, mr->values[index].data,
				hash_digest_size[selected->hash_algorithm]);
		}
		return ret;
	}

	selection = 0;
	for (mr_id = 0; mr_id < ARRAY_SIZE(sim->fixed); mr_id++)
		selection += sim->fixed[mr_id].props.algorithms_count;
	xa_for_each(&sim->dynamic_mrs, mr_id, mr)
		if (mr->handle)
			selection += mr->props.algorithms_count;
	ret = attestation_sim_buffer_u32(buffer, selection);
	for (mr_id = 0; !ret && mr_id < ARRAY_SIZE(sim->fixed); mr_id++) {
		mr = &sim->fixed[mr_id];
		for (selection = 0;
		     selection < mr->props.algorithms_count; selection++) {
			ret = attestation_sim_buffer_u32(buffer, mr->props.id);
			ret |= attestation_sim_buffer_u32(
				buffer, mr->values[selection].algorithm);
			ret |= attestation_sim_buffer_put(
				buffer, mr->values[selection].data,
				hash_digest_size[
					mr->values[selection].algorithm]);
		}
	}
	xa_for_each(&sim->dynamic_mrs, mr_id, mr)
		for (selection = 0; mr->handle &&
		     !ret && selection < mr->props.algorithms_count;
		     selection++) {
			ret = attestation_sim_buffer_u32(buffer, mr->props.id);
			ret |= attestation_sim_buffer_u32(
				buffer, mr->values[selection].algorithm);
			ret |= attestation_sim_buffer_put(
				buffer, mr->values[selection].data,
				hash_digest_size[
					mr->values[selection].algorithm]);
		}
	return ret;
}

static int
attestation_sim_evidence_get(struct attestation_dev *att,
			     const struct attestation_evidence_request *request,
			     struct attestation_evidence_result *result)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	struct attestation_sim_buffer *buffer;
	size_t object_index;
	int ret;

	ret = attestation_sim_callback_enter(
		sim, ATTESTATION_SIM_CB_EVIDENCE_GET);
	if (ret)
		return ret;
	buffer = kzalloc_obj(*buffer, GFP_KERNEL);
	if (!buffer)
		return -ENOMEM;
	for (object_index = 0; object_index < request->objects_count;
	     object_index++) {
		const struct attestation_evidence_object_request *object =
			&request->objects[object_index];
		const struct attestation_evidence_supported_object *supported =
			object->supported_object;
		struct attestation_evidence_object_result *produced =
			&result->objects[object_index];

		buffer->length = 0;
		switch (supported - attestation_sim_evidence_objects) {
		case ATTESTATION_SIM_OBJECT_REPORT:
			ret = attestation_sim_report(sim, request, buffer);
			break;
		case ATTESTATION_SIM_OBJECT_TOKEN:
			ret = attestation_sim_buffer_put(
				buffer, "SIM-TOKEN-v1", 12);
			ret |= attestation_sim_buffer_put(
				buffer, request->challenge,
				request->challenge_length);
			break;
		case ATTESTATION_SIM_OBJECT_LOG:
			ret = attestation_sim_buffer_put(buffer, attestation_sim_log,
							 sizeof(attestation_sim_log));
			break;
		case ATTESTATION_SIM_OBJECT_MANIFEST:
			ret = attestation_sim_buffer_put(
				buffer, "SIM-MANIFEST-v1", 15);
			ret |= attestation_sim_buffer_put(
				buffer, sim->fixture->name,
				strlen(sim->fixture->name));
			break;
		default:
			ret = -EOPNOTSUPP;
		}
		if (ret)
			goto out;
		if (supported->has_tcb_digest) {
			ret = attestation_sim_digest(
				supported->tcb_digest_algorithm, buffer->data,
				buffer->length, NULL, 0, produced->tcb_digest);
			if (ret)
				goto out;
		}
		ret = attestation_evidence_write(object, buffer->data,
						 buffer->length);
		if (ret)
			goto out;
		produced->format = supported->formats[0];
		produced->length = buffer->length;
	}
out:
	kfree(buffer);
	return ret;
}

static int attestation_sim_evidence_accept(struct attestation_dev *att)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	int ret;

	ret = attestation_sim_callback_enter(
		sim, ATTESTATION_SIM_CB_EVIDENCE_ACCEPT);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	if (sim->tdisp_state != ATTESTATION_TDISP_STATE_LOCKED)
		return -EACCES;
	sim->tdisp_state = ATTESTATION_TDISP_STATE_RUN;
	attestation_evidence_accept_enable(att, false);
	return 0;
}

static int
attestation_sim_tdisp_state_get(struct attestation_dev *att,
				enum attestation_tdisp_state *state)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	int ret;

	ret = attestation_sim_callback_enter(
		sim, ATTESTATION_SIM_CB_TDISP_STATE_GET);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	*state = sim->tdisp_state;
	return 0;
}

static const struct attestation_provider attestation_sim_provider = {
	.evidence_get = attestation_sim_evidence_get,
	.evidence_accept = attestation_sim_evidence_accept,
	.mr_read = attestation_sim_mr_read,
	.mr_extend = attestation_sim_mr_extend,
	.tdisp_state_get = attestation_sim_tdisp_state_get,
	.evidence_caps = &attestation_sim_evidence_caps,
	.mr_caps = &attestation_sim_mr_caps,
	.mrs = attestation_sim_mrs,
	.mrs_count = ARRAY_SIZE(attestation_sim_mrs),
};

static int attestation_sim_hw_init(struct attestation_sim *sim,
				   struct attestation_sim_fixture *fixture)
{
	size_t index;
	int ret;

	sim->fixture = fixture;
	mutex_init(&sim->hardware_lock);
	xa_init(&sim->dynamic_mrs);
	for (index = 0; index < ARRAY_SIZE(sim->fixed); index++) {
		ret = attestation_sim_mr_init(
			&sim->fixed[index], &attestation_sim_mrs[index]);
		if (ret)
			goto err_fixed;
	}
	sim->tdisp_state = ATTESTATION_TDISP_STATE_UNLOCKED;
	attestation_evidence_accept_enable(&sim->att, false);
	fixture->sim = sim;
	return 0;

err_fixed:
	while (index)
		kfree(sim->fixed[--index].values);
	xa_destroy(&sim->dynamic_mrs);
	mutex_destroy(&sim->hardware_lock);
	return ret;
}

/* After unregister: no provider callback runs, and the core still keeps MRs. */
static void attestation_sim_hw_fini(struct attestation_sim *sim)
{
	struct attestation_sim_mr *mr;
	unsigned long index;

	xa_for_each(&sim->dynamic_mrs, index, mr) {
		xa_erase(&sim->dynamic_mrs, index);
		attestation_mr_destroy(mr->handle);
		attestation_sim_mr_fini(mr);
	}
	xa_destroy(&sim->dynamic_mrs);
	for (index = 0; index < ARRAY_SIZE(sim->fixed); index++)
		kfree(sim->fixed[index].values);
	sim->fixture->sim = NULL;
	mutex_destroy(&sim->hardware_lock);
}

int ctlv_attestation_sim_device_info_fill(struct ctlv *ctlv,
					  struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *record)
{
	struct attestation_sim *sim = attestation_sim_from_att(attestation_from_ctlv(ctlv));

	return ctlv_attestation_sim_device_info_name_put(
		record, sim->fixture->name) ? 0 : -EMSGSIZE;
}

void ctlv_attestation_sim_device_info_read(struct ctlv *ctlv,
					   struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *record)
{
}

static void
attestation_sim_fixture_free(struct attestation_sim_fixture *fixture)
{
	mutex_destroy(&fixture->control_lock);
	kfree(fixture);
}

static int attestation_sim_copy_command(const char __user *buf, size_t len,
					char *command)
{
	if (!len || len > ATTESTATION_SIM_COMMAND_MAX)
		return -EINVAL;
	if (copy_from_user(command, buf, len))
		return -EFAULT;
	command[len] = '\0';
	strim(command);
	return *command ? 0 : -EINVAL;
}

static struct attestation_sim_fixture *
attestation_sim_fixture_find(const char *name)
{
	struct attestation_sim_fixture *fixture;

	list_for_each_entry(fixture, &attestation_sim_fixtures, node)
		if (!strcmp(fixture->name, name))
			return fixture;
	return NULL;
}

static int attestation_sim_fixture_create(const char *name);
static int attestation_sim_fixture_destroy(const char *name);

static ssize_t attestation_sim_create_write(struct file *file,
					    const char __user *buf,
					    size_t len, loff_t *ppos)
{
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (!ret)
		ret = attestation_sim_fixture_create(command);
	return ret ? ret : len;
}

static ssize_t attestation_sim_destroy_write(struct file *file,
					     const char __user *buf,
					     size_t len, loff_t *ppos)
{
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (!ret)
		ret = attestation_sim_fixture_destroy(command);
	return ret ? ret : len;
}

static const struct file_operations attestation_sim_create_fops = {
	.owner = THIS_MODULE,
	.write = attestation_sim_create_write,
	.llseek = noop_llseek,
};

static const struct file_operations attestation_sim_destroy_fops = {
	.owner = THIS_MODULE,
	.write = attestation_sim_destroy_write,
	.llseek = noop_llseek,
};

static const char *attestation_sim_tdisp_name(enum attestation_tdisp_state state)
{
	switch (state) {
	case ATTESTATION_TDISP_STATE_UNLOCKED:
		return "UNLOCKED";
	case ATTESTATION_TDISP_STATE_LOCKED:
		return "LOCKED";
	case ATTESTATION_TDISP_STATE_RUN:
		return "RUN";
	case ATTESTATION_TDISP_STATE_ERROR:
		return "ERROR";
	}
	return "UNKNOWN";
}

static int attestation_sim_tdisp_parse(const char *name,
				       enum attestation_tdisp_state *state)
{
	enum attestation_tdisp_state value;

	for (value = ATTESTATION_TDISP_STATE_UNLOCKED;
	     value <= ATTESTATION_TDISP_STATE_ERROR; value++)
		if (!strcmp(name, attestation_sim_tdisp_name(value))) {
			*state = value;
			return 0;
		}
	return -EINVAL;
}

static ssize_t attestation_sim_state_read(struct file *file, char __user *buf,
					  size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char text[64];
	int length;

	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim)
		return -ENODEV;
	scoped_guard(mutex, &fixture->sim->hardware_lock)
		length = scnprintf(
			text, sizeof(text), "%s\n",
			attestation_sim_tdisp_name(fixture->sim->tdisp_state));
	return simple_read_from_buffer(buf, len, ppos, text, length);
}

static ssize_t attestation_sim_state_write(struct file *file,
					   const char __user *buf,
					   size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	enum attestation_tdisp_state tdisp;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (!ret)
		ret = attestation_sim_tdisp_parse(command, &tdisp);
	if (ret)
		return ret;
	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim)
		return -ENODEV;
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		fixture->sim->notification_pending = true;
		fixture->sim->tdisp_state = tdisp;
	}
	attestation_state_changed(&fixture->sim->att);
	attestation_evidence_accept_enable(&fixture->sim->att,
					   tdisp == ATTESTATION_TDISP_STATE_LOCKED);
	scoped_guard(mutex, &fixture->sim->hardware_lock)
		fixture->sim->notification_pending = false;
	return len;
}

static const struct file_operations attestation_sim_state_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = attestation_sim_state_read,
	.write = attestation_sim_state_write,
	.llseek = default_llseek,
};

static int attestation_sim_algorithm_parse(const char *name,
					   enum hash_algo *algorithm)
{
	enum hash_algo value;

	for (value = 0; value < HASH_ALGO__LAST; value++)
		if (hash_algo_name[value] && !strcmp(name, hash_algo_name[value])) {
			*algorithm = value;
			return 0;
		}
	return -EINVAL;
}

static int attestation_sim_mr_add(struct attestation_sim_fixture *fixture,
				  struct attestation_sim_mr *mr, unsigned int id)
{
	struct attestation_sim_mr *walk;
	struct attestation_mr *handle;
	unsigned long walk_id;
	size_t mr_count = 0;
	int ret;

	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim)
		return -ENODEV;
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		xa_for_each(&fixture->sim->dynamic_mrs, walk_id, walk)
			mr_count++;
		if (mr_count >= ATTESTATION_SIM_DYNAMIC_MRS_MAX)
			return -ENOSPC;
		if (xa_load(&fixture->sim->dynamic_mrs, id))
			return -EEXIST;
		ret = xa_insert(&fixture->sim->dynamic_mrs, id, mr,
				GFP_KERNEL);
		if (ret)
			return ret;
		fixture->sim->notification_pending = true;
	}
	handle = attestation_mr_create(&fixture->sim->att, &mr->props);
	if (IS_ERR(handle)) {
		scoped_guard(mutex, &fixture->sim->hardware_lock) {
			xa_erase(&fixture->sim->dynamic_mrs, id);
			fixture->sim->notification_pending = false;
		}
		return PTR_ERR(handle);
	}
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		mr->handle = handle;
		fixture->sim->notification_pending = false;
	}
	return 0;
}

static ssize_t attestation_sim_mr_add_write(struct file *file,
					    const char __user *buf,
					    size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	struct attestation_sim_mr *mr;
	char *cursor, *token;
	unsigned int id;
	size_t count = 0;
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	cursor = command;
	token = strsep(&cursor, " ");
	if (!token || kstrtouint(token, 0, &id) || id < 2)
		return -EINVAL;
	mr = kzalloc_obj(*mr, GFP_KERNEL);
	if (!mr)
		return -ENOMEM;
	mr->algorithms = kcalloc(HASH_ALGO__LAST,
				 sizeof(*mr->algorithms), GFP_KERNEL);
	if (!mr->algorithms) {
		ret = -ENOMEM;
		goto err_mr;
	}
	while ((token = strsep(&cursor, " "))) {
		enum hash_algo algorithm;

		if (!*token)
			continue;
		ret = attestation_sim_algorithm_parse(token, &algorithm);
		if (ret || count == HASH_ALGO__LAST) {
			ret = -EINVAL;
			goto err_mr;
		}
		mr->algorithms[count++] = algorithm;
	}
	if (!count) {
		ret = -EINVAL;
		goto err_mr;
	}
	mr->props = (struct attestation_mr_props) {
		.id = id,
		.algorithms = mr->algorithms,
		.algorithms_count = count,
		.has_register_index = true,
		.register_index = id,
		.extendable = true,
	};
	mr->props.name = kasprintf(GFP_KERNEL, "sim-dynamic-%u", id);
	if (!mr->props.name) {
		ret = -ENOMEM;
		goto err_mr;
	}
	ret = attestation_sim_mr_init(mr, &mr->props);
	if (ret)
		goto err_mr;
	ret = attestation_sim_mr_add(fixture, mr, id);
	if (ret)
		goto err_mr;
	return len;

err_mr:
	attestation_sim_mr_fini(mr);
	return ret;
}

static ssize_t attestation_sim_mr_remove_write(struct file *file,
					       const char __user *buf,
					       size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	struct attestation_sim_mr *mr;
	struct attestation_mr *handle = NULL;
	unsigned int id;
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	if (kstrtouint(command, 0, &id))
		return -EINVAL;
	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim)
		return -ENODEV;
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		mr = xa_load(&fixture->sim->dynamic_mrs, id);
		if (mr) {
			handle = mr->handle;
			mr->handle = NULL;
			xa_erase(&fixture->sim->dynamic_mrs, id);
			fixture->sim->notification_pending = true;
		}
	}
	if (!mr)
		return -ENOENT;
	attestation_mr_destroy(handle);
	scoped_guard(mutex, &fixture->sim->hardware_lock)
		fixture->sim->notification_pending = false;
	attestation_sim_mr_fini(mr);
	return len;
}

static const struct file_operations attestation_sim_mr_add_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = attestation_sim_mr_add_write,
	.llseek = noop_llseek,
};

static const struct file_operations attestation_sim_mr_remove_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = attestation_sim_mr_remove_write,
	.llseek = noop_llseek,
};

static int attestation_sim_errno_parse(const char *name)
{
	if (!strcmp(name, "EIO"))
		return -EIO;
	if (!strcmp(name, "EAGAIN"))
		return -EAGAIN;
	if (!strcmp(name, "ENOMEM"))
		return -ENOMEM;
	if (!strcmp(name, "EBUSY"))
		return -EBUSY;
	return 0;
}

static ssize_t attestation_sim_fail_next_write(struct file *file,
					       const char __user *buf,
					       size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	char callback_name[32], error_name[32], extra;
	enum attestation_sim_callback callback;
	int error;
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	if (sscanf(command, "%31s %31s %c",
		   callback_name, error_name, &extra) != 2)
		return -EINVAL;
	callback = attestation_sim_callback_parse(callback_name);
	error = attestation_sim_errno_parse(error_name);
	if (!callback || !error)
		return -EINVAL;
	guard(mutex)(&fixture->control_lock);
	if (fixture->dead)
		return -ENODEV;
	scoped_guard(spinlock, &fixture->callback_lock) {
		if (fixture->fail_next)
			return -EBUSY;
		fixture->fail_next = callback;
		fixture->fail_error = error;
	}
	return len;
}

static ssize_t attestation_sim_pause_next_write(struct file *file,
						const char __user *buf,
						size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	enum attestation_sim_callback callback;
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	callback = attestation_sim_callback_parse(command);
	if (!callback)
		return -EINVAL;
	guard(mutex)(&fixture->control_lock);
	if (fixture->dead)
		return -ENODEV;
	scoped_guard(spinlock, &fixture->callback_lock) {
		if (fixture->pause_next || fixture->paused)
			return -EBUSY;
		fixture->pause_next = callback;
	}
	return len;
}

static ssize_t attestation_sim_resume_write(struct file *file,
					    const char __user *buf,
					    size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	if (strcmp(command, "1"))
		return -EINVAL;
	scoped_guard(spinlock, &fixture->callback_lock) {
		if (!fixture->paused)
			return -EINVAL;
		fixture->pause_release = true;
	}
	wake_up_all(&fixture->callback_wait);
	return len;
}

static ssize_t attestation_sim_status_read(struct file *file,
					   char __user *buf,
					   size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char text[512];
	enum attestation_sim_callback callback;
	const char *paused = "none";
	size_t at = 0;

	scoped_guard(spinlock, &fixture->callback_lock)
		if (fixture->paused)
			paused = attestation_sim_callback_names[fixture->paused];
	at += scnprintf(text + at, sizeof(text) - at,
			"paused %s\n", paused);
	for (callback = 1; callback < ATTESTATION_SIM_CB_COUNT; callback++)
		at += scnprintf(text + at, sizeof(text) - at, "%s %d\n",
				attestation_sim_callback_names[callback],
				atomic_read(
					&fixture->callback_count[callback]));
	return simple_read_from_buffer(buf, len, ppos, text, at);
}

static const struct file_operations attestation_sim_fail_next_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = attestation_sim_fail_next_write,
	.llseek = noop_llseek,
};

static const struct file_operations attestation_sim_pause_next_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = attestation_sim_pause_next_write,
	.llseek = noop_llseek,
};

static const struct file_operations attestation_sim_resume_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = attestation_sim_resume_write,
	.llseek = noop_llseek,
};

static const struct file_operations attestation_sim_status_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = attestation_sim_status_read,
	.llseek = default_llseek,
};

static int attestation_sim_debugfs_fixture(struct attestation_sim_fixture *fixture)
{
	struct dentry *dir;

	dir = debugfs_create_dir(fixture->name, attestation_sim_debugfs);
	if (IS_ERR(dir))
		return PTR_ERR(dir);
	fixture->debugfs_dir = dir;
	debugfs_create_file("state", 0600, dir, fixture,
			    &attestation_sim_state_fops);
	debugfs_create_file("mr-add", 0200, dir, fixture,
			    &attestation_sim_mr_add_fops);
	debugfs_create_file("mr-remove", 0200, dir, fixture,
			    &attestation_sim_mr_remove_fops);
	debugfs_create_file("fail-next", 0200, dir, fixture,
			    &attestation_sim_fail_next_fops);
	debugfs_create_file("pause-next", 0200, dir, fixture,
			    &attestation_sim_pause_next_fops);
	debugfs_create_file("resume", 0200, dir, fixture,
			    &attestation_sim_resume_fops);
	debugfs_create_file("status", 0400, dir, fixture,
			    &attestation_sim_status_fops);
	return 0;
}

static struct attestation_sim_fixture *
attestation_sim_fixture_new(const char *name)
{
	struct attestation_sim_fixture *fixture;
	struct attestation_sim *sim;
	char *parent_name;
	int ret;

	lockdep_assert_held(&attestation_sim_fixtures_lock);
	fixture = kzalloc_obj(*fixture, GFP_KERNEL);
	if (!fixture)
		return ERR_PTR(-ENOMEM);
	mutex_init(&fixture->control_lock);
	spin_lock_init(&fixture->callback_lock);
	init_waitqueue_head(&fixture->callback_wait);
	strscpy(fixture->name, name, sizeof(fixture->name));
	parent_name = kasprintf(GFP_KERNEL, "attestation-sim-%s", name);
	if (!parent_name) {
		ret = -ENOMEM;
		goto err_fixture;
	}
	fixture->parent = root_device_register(parent_name);
	kfree(parent_name);
	if (IS_ERR(fixture->parent)) {
		ret = PTR_ERR(fixture->parent);
		fixture->parent = NULL;
		goto err_fixture;
	}
	sim = attestation_alloc(fixture->parent, &attestation_sim_provider,
				&ctlv_attestation_sim_family, struct attestation_sim, att);
	if (IS_ERR(sim)) {
		ret = PTR_ERR(sim);
		goto err_parent;
	}
	ret = attestation_sim_hw_init(sim, fixture);
	if (ret)
		goto err_put;
	ret = attestation_register(&sim->att);
	if (ret)
		goto err_hw;
	ret = attestation_sim_debugfs_fixture(fixture);
	if (ret)
		goto err_unregister;
	return fixture;

err_unregister:
	attestation_unregister(&sim->att);
err_hw:
	attestation_sim_hw_fini(sim);
err_put:
	attestation_put(&sim->att);
err_parent:
	root_device_unregister(fixture->parent);
err_fixture:
	attestation_sim_fixture_free(fixture);
	return ERR_PTR(ret);
}

static int attestation_sim_fixture_create(const char *name)
{
	struct attestation_sim_fixture *fixture;

	if (!*name || strlen(name) > ATTESTATION_SIM_NAME_MAX ||
	    strchr(name, '/'))
		return -EINVAL;
	guard(mutex)(&attestation_sim_fixtures_lock);
	if (attestation_sim_fixture_find(name))
		return -EEXIST;
	fixture = attestation_sim_fixture_new(name);
	if (IS_ERR(fixture))
		return PTR_ERR(fixture);
	list_add_tail(&fixture->node, &attestation_sim_fixtures);
	return 0;
}

static void attestation_sim_fixture_stop(struct attestation_sim_fixture *fixture)
{
	WRITE_ONCE(fixture->stopping, true);
	scoped_guard(spinlock, &fixture->callback_lock)
		fixture->pause_release = true;
	wake_up_all(&fixture->callback_wait);
}

static int attestation_sim_fixture_destroy(const char *name)
{
	struct attestation_sim_fixture *fixture;
	struct attestation_sim *sim;

	mutex_lock(&attestation_sim_fixtures_lock);
	fixture = attestation_sim_fixture_find(name);
	if (!fixture) {
		mutex_unlock(&attestation_sim_fixtures_lock);
		return -ENOENT;
	}
	list_del(&fixture->node);
	mutex_lock(&fixture->control_lock);
	fixture->dead = true;
	mutex_unlock(&fixture->control_lock);
	debugfs_remove_recursive(fixture->debugfs_dir);
	attestation_sim_fixture_stop(fixture);
	mutex_unlock(&attestation_sim_fixtures_lock);

	sim = fixture->sim;
	attestation_unregister(&sim->att);
	attestation_sim_hw_fini(sim);
	attestation_put(&sim->att);
	root_device_unregister(fixture->parent);
	attestation_sim_fixture_free(fixture);
	return 0;
}

static int __init attestation_sim_init_module(void)
{
	int ret;

	attestation_sim_debugfs = debugfs_create_dir("attestation-sim", NULL);
	if (IS_ERR(attestation_sim_debugfs))
		return PTR_ERR(attestation_sim_debugfs);
	debugfs_create_file("create", 0200, attestation_sim_debugfs, NULL,
			    &attestation_sim_create_fops);
	debugfs_create_file("destroy", 0200, attestation_sim_debugfs, NULL,
			    &attestation_sim_destroy_fops);
	if (!early_fixture || !*early_fixture)
		return 0;
	ret = attestation_sim_fixture_create(early_fixture);
	if (ret)
		debugfs_remove_recursive(attestation_sim_debugfs);
	return ret;
}

static void __exit attestation_sim_exit_module(void)
{
	struct attestation_sim_fixture *fixture;
	char name[ATTESTATION_SIM_NAME_MAX + 1];

	for (;;) {
		mutex_lock(&attestation_sim_fixtures_lock);
		fixture = list_first_entry_or_null(
			&attestation_sim_fixtures,
			struct attestation_sim_fixture, node);
		if (fixture)
			strscpy(name, fixture->name, sizeof(name));
		mutex_unlock(&attestation_sim_fixtures_lock);
		if (!fixture)
			break;
		attestation_sim_fixture_destroy(name);
	}
	debugfs_remove_recursive(attestation_sim_debugfs);
}

module_init(attestation_sim_init_module);
module_exit(attestation_sim_exit_module);
MODULE_DESCRIPTION("Simulated attestation provider");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("ATTESTATION");
MODULE_IMPORT_NS("CTLV");
