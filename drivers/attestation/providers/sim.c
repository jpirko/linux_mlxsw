// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash.h>
#include <crypto/hash_info.h>
#include <linux/attestation.h>
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
	ATTESTATION_SIM_CB_NONE,
	ATTESTATION_SIM_CB_EVIDENCE_GET,
	ATTESTATION_SIM_CB_EVIDENCE_ACCEPT,
	ATTESTATION_SIM_CB_MR_READ,
	ATTESTATION_SIM_CB_MR_EXTEND,
	ATTESTATION_SIM_CB_TDISP_STATE_GET,
	ATTESTATION_SIM_CB_DEVICE_TRUST_GET,
	ATTESTATION_SIM_CB_DEVSEC_RUN,
	ATTESTATION_SIM_CB_COUNT,
};

struct attestation_sim_fixture;

struct attestation_sim_mr {
	struct attestation_mr_props props;
	enum hash_algo *algorithms;
	struct attestation_digest *values;
	struct attestation_mr *handle;
};

struct attestation_sim {
	struct ctlv_device *ctlvdev;
	struct attestation_sim_fixture *fixture;
	/* Protects simulated hardware and dynamic register state. */
	struct mutex hardware_lock;
	struct xarray dynamic_mrs;
	struct attestation_sim_mr fixed[2];
	enum attestation_tdisp_state tdisp_state;
	enum attestation_device_trust_state trust_state;
	bool pending_verdict;
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
	struct ctlv_device *ctlvdev;
	struct attestation_sim *sim;
	struct dentry *debugfs_dir;
};

struct attestation_sim_init_info {
	struct attestation_sim_fixture *fixture;
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
		.readable = true,
	},
	{
		.id = 1,
		.name = "sim-extendable",
		.algorithms = attestation_sim_mr1_algorithms,
		.algorithms_count = ARRAY_SIZE(attestation_sim_mr1_algorithms),
		.has_register_index = true,
		.register_index = 1,
		.readable = true,
		.extendable = true,
	},
};

static const enum attestation_evidence_representation
attestation_sim_primary_representations[] = {
	ATTESTATION_EVIDENCE_REPRESENTATION_RAW,
	ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST,
};

static const enum attestation_evidence_representation
attestation_sim_raw_representation[] = {
	ATTESTATION_EVIDENCE_REPRESENTATION_RAW,
};

static const enum hash_algo attestation_sim_evidence_algorithms[] = {
	HASH_ALGO_SHA256,
	HASH_ALGO_SHA384,
};

static const struct attestation_evidence_supported_object
attestation_sim_evidence_objects[] = {
	{
		.blob_class = ATTESTATION_EVIDENCE_BLOB_PRIMARY,
		.format = "attestation-sim-primary-v1",
		.representations = attestation_sim_primary_representations,
		.representations_count =
			ARRAY_SIZE(attestation_sim_primary_representations),
		.hash_algorithms = attestation_sim_evidence_algorithms,
		.hash_algorithms_count =
			ARRAY_SIZE(attestation_sim_evidence_algorithms),
		.max_length = ATTESTATION_SIM_EVIDENCE_MAX,
		.binds_mrs = true,
	},
	{
		.blob_class = ATTESTATION_EVIDENCE_BLOB_AUXILIARY,
		.format = "attestation-sim-aux-v1",
		.representations = attestation_sim_raw_representation,
		.representations_count =
			ARRAY_SIZE(attestation_sim_raw_representation),
		.max_length = 1024,
	},
	{
		.blob_class = ATTESTATION_EVIDENCE_BLOB_MANIFEST,
		.has_blob_instance = true,
		.blob_instance = 1,
		.format = "attestation-sim-manifest-v1",
		.representations = attestation_sim_raw_representation,
		.representations_count =
			ARRAY_SIZE(attestation_sim_raw_representation),
		.max_length = 1024,
	},
};

static const struct attestation_evidence_caps attestation_sim_evidence_caps = {
	.min_challenge_length = 0,
	.max_challenge_length = 64,
	.max_total_output_length = 8192,
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
	[ATTESTATION_SIM_CB_DEVICE_TRUST_GET] = "device_trust_get",
	[ATTESTATION_SIM_CB_DEVSEC_RUN] = "devsec_run",
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

static struct attestation_sim *attestation_sim_from_att(struct attestation *att)
{
	return ctlv_priv_child(att);
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

static int attestation_sim_mr_read(struct attestation *att, unsigned int mr_id,
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

static int attestation_sim_mr_extend(struct attestation *att,
				     unsigned int mr_id,
				     const struct ctlv_cursor *mr_options,
				     const struct attestation_digest_set *values)
{
	struct attestation_digest changed[HASH_ALGO__LAST];
	struct attestation_sim *sim = attestation_sim_from_att(att);
	struct attestation_sim_mr *mr;
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
	if (values->values_count != mr->props.algorithms_count)
		return -EINVAL;
	for (index = 0; index < values->values_count; index++) {
		enum hash_algo algorithm = mr->values[index].algorithm;
		size_t length = hash_digest_size[algorithm];

		if (values->values[index].algorithm != algorithm)
			return -EINVAL;
		changed[index].algorithm = algorithm;
		ret = attestation_sim_digest(
			algorithm, mr->values[index].data, length,
			values->values[index].data, length,
			changed[index].data);
		if (ret)
			return ret;
	}
	memcpy(mr->values, changed,
	       values->values_count * sizeof(*changed));
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
attestation_sim_primary(struct attestation_sim *sim,
			const struct attestation_evidence_request *request,
			struct attestation_sim_buffer *buffer)
{
	struct attestation_sim_mr *mr;
	unsigned long mr_id;
	size_t selection;
	int ret;

	ret = attestation_sim_buffer_put(buffer, "SIM-PRIMARY-v1", 14);
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
attestation_sim_evidence_get(struct attestation *att,
			     const struct attestation_evidence_request *request,
			     struct attestation_evidence_result *result)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	struct attestation_sim_buffer *buffer;
	u8 digest[HASH_MAX_DIGESTSIZE];
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

		buffer->length = 0;
		switch (object->supported_object->blob_class) {
		case ATTESTATION_EVIDENCE_BLOB_PRIMARY:
			ret = attestation_sim_primary(sim, request, buffer);
			break;
		case ATTESTATION_EVIDENCE_BLOB_AUXILIARY:
			ret = attestation_sim_buffer_put(
				buffer, "SIM-AUX-v1", 10);
			ret |= attestation_sim_buffer_put(
				buffer, request->challenge,
				request->challenge_length);
			break;
		case ATTESTATION_EVIDENCE_BLOB_MANIFEST:
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
		if (object->representation ==
		    ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST) {
			ret = attestation_sim_digest(
				object->hash_algorithm, buffer->data,
				buffer->length, NULL, 0, digest);
			if (ret)
				goto out;
			buffer->length =
				hash_digest_size[object->hash_algorithm];
			memcpy(buffer->data, digest, buffer->length);
		}
		ret = ctlv_blob_write(object->output, 0,
				      buffer->data, buffer->length);
		if (ret)
			goto out;
		result->object_lengths[object_index] = buffer->length;
	}
out:
	kfree(buffer);
	return ret;
}

static int attestation_sim_evidence_accept(struct attestation *att)
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
	sim->pending_verdict = true;
	return 0;
}

static void attestation_sim_evidence_accept_clear(struct attestation *att)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);

	guard(mutex)(&sim->hardware_lock);
	sim->pending_verdict = false;
}

static int
attestation_sim_tdisp_state_get(struct attestation *att,
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

static int
attestation_sim_device_trust_get(
	struct attestation *att, enum attestation_device_trust_state *trust)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	int ret;

	ret = attestation_sim_callback_enter(
		sim, ATTESTATION_SIM_CB_DEVICE_TRUST_GET);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	*trust = sim->trust_state;
	return 0;
}

static int attestation_sim_devsec_run(struct attestation *att)
{
	struct attestation_sim *sim = attestation_sim_from_att(att);
	int ret;

	ret = attestation_sim_callback_enter(
		sim, ATTESTATION_SIM_CB_DEVSEC_RUN);
	if (ret)
		return ret;
	guard(mutex)(&sim->hardware_lock);
	if (sim->notification_pending)
		return -EAGAIN;
	if (sim->tdisp_state != ATTESTATION_TDISP_STATE_LOCKED ||
	    !sim->pending_verdict)
		return -EACCES;
	sim->tdisp_state = ATTESTATION_TDISP_STATE_RUN;
	sim->pending_verdict = false;
	return 0;
}

static const struct attestation_provider attestation_sim_provider = {
	.evidence_get = attestation_sim_evidence_get,
	.evidence_accept = attestation_sim_evidence_accept,
	.evidence_accept_clear = attestation_sim_evidence_accept_clear,
	.mr_read = attestation_sim_mr_read,
	.mr_extend = attestation_sim_mr_extend,
	.tdisp_state_get = attestation_sim_tdisp_state_get,
	.device_trust_get = attestation_sim_device_trust_get,
	.devsec_run = attestation_sim_devsec_run,
	.evidence_caps = &attestation_sim_evidence_caps,
	.mr_caps = &attestation_sim_mr_caps,
	.mrs = attestation_sim_mrs,
	.mrs_count = ARRAY_SIZE(attestation_sim_mrs),
};

size_t ctlv_attestation_sim_priv_size(void)
{
	return sizeof(struct attestation_sim);
}

const void *ctlv_attestation_sim_pre_init(const void *init_info)
{
	return &attestation_sim_provider;
}

int ctlv_attestation_sim_init(struct ctlv_device *ctlvdev,
			      const void *init_info, void *priv)
{
	const struct attestation_sim_init_info *info = init_info;
	struct attestation_sim *sim = priv;
	size_t index;
	int ret;

	if (!info || !info->fixture)
		return -EINVAL;
	sim->ctlvdev = ctlvdev;
	sim->fixture = info->fixture;
	mutex_init(&sim->hardware_lock);
	xa_init(&sim->dynamic_mrs);
	for (index = 0; index < ARRAY_SIZE(sim->fixed); index++) {
		ret = attestation_sim_mr_init(
			&sim->fixed[index], &attestation_sim_mrs[index]);
		if (ret)
			goto err_fixed;
	}
	sim->tdisp_state = ATTESTATION_TDISP_STATE_UNLOCKED;
	sim->trust_state = ATTESTATION_DEVICE_TRUST_NONE;
	info->fixture->sim = sim;
	return 0;

err_fixed:
	while (index)
		kfree(sim->fixed[--index].values);
	xa_destroy(&sim->dynamic_mrs);
	mutex_destroy(&sim->hardware_lock);
	return ret;
}

void ctlv_attestation_sim_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct attestation_sim *sim = priv;
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

int ctlv_attestation_sim_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return ctlv_attestation_op_pre(ctlv_priv_parent(priv), ctx);
}

void ctlv_attestation_sim_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_attestation_op_post(ctlv_priv_parent(priv), ctx);
}

int ctlv_attestation_sim_device_info_fill(void *priv,
					  struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *nest)
{
	struct attestation_sim *sim = priv;

	return ctlv_attestation_sim_sim_name_put(
		nest, sim->fixture->name) ? 0 : -EMSGSIZE;
}

void ctlv_attestation_sim_device_info_read(void *priv,
					   struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *nest)
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

static const char *
attestation_sim_trust_name(enum attestation_device_trust_state state)
{
	switch (state) {
	case ATTESTATION_DEVICE_TRUST_UNSET:
		return "UNSET";
	case ATTESTATION_DEVICE_TRUST_NONE:
		return "NONE";
	case ATTESTATION_DEVICE_TRUST_ADVERSARY:
		return "ADVERSARY";
	case ATTESTATION_DEVICE_TRUST_AUTO:
		return "AUTO";
	case ATTESTATION_DEVICE_TRUST_TCB:
		return "TCB";
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

static int
attestation_sim_trust_parse(const char *name,
			    enum attestation_device_trust_state *state)
{
	enum attestation_device_trust_state value;

	for (value = ATTESTATION_DEVICE_TRUST_UNSET;
	     value <= ATTESTATION_DEVICE_TRUST_TCB; value++)
		if (!strcmp(name, attestation_sim_trust_name(value))) {
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
			text, sizeof(text), "%s %s\n",
			attestation_sim_tdisp_name(fixture->sim->tdisp_state),
			attestation_sim_trust_name(fixture->sim->trust_state));
	return simple_read_from_buffer(buf, len, ppos, text, length);
}

static ssize_t attestation_sim_state_write(struct file *file,
					   const char __user *buf,
					   size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	enum attestation_device_trust_state trust;
	enum attestation_tdisp_state tdisp;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	char tdisp_name[32], trust_name[32], extra;
	struct attestation *att;
	int ret;

	ret = attestation_sim_copy_command(buf, len, command);
	if (ret)
		return ret;
	if (sscanf(command, "%31s %31s %c",
		   tdisp_name, trust_name, &extra) != 2)
		return -EINVAL;
	ret = attestation_sim_tdisp_parse(tdisp_name, &tdisp);
	if (!ret)
		ret = attestation_sim_trust_parse(trust_name, &trust);
	if (ret)
		return ret;
	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim)
		return -ENODEV;
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		fixture->sim->notification_pending = true;
		fixture->sim->tdisp_state = tdisp;
		fixture->sim->trust_state = trust;
		fixture->sim->pending_verdict = false;
	}
	att = ctlv_priv_parent(fixture->sim);
	attestation_state_changed(att);
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

static ssize_t attestation_sim_mr_add_write(struct file *file,
					    const char __user *buf,
					    size_t len, loff_t *ppos)
{
	struct attestation_sim_fixture *fixture = file->private_data;
	char command[ATTESTATION_SIM_COMMAND_MAX + 1];
	struct attestation_sim_mr *mr;
	struct attestation_sim_mr *walk;
	struct attestation_mr *handle;
	char *cursor, *token;
	unsigned long walk_id;
	unsigned int id;
	size_t count = 0, mr_count = 0;
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
		if (ret || (count && mr->algorithms[count - 1] >= algorithm)) {
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
		.readable = true,
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

	guard(mutex)(&fixture->control_lock);
	if (fixture->dead || !fixture->sim) {
		ret = -ENODEV;
		goto err_mr;
	}
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		xa_for_each(&fixture->sim->dynamic_mrs, walk_id, walk)
			mr_count++;
		if (mr_count >= ATTESTATION_SIM_DYNAMIC_MRS_MAX) {
			ret = -ENOSPC;
			goto err_mr;
		}
		if (xa_load(&fixture->sim->dynamic_mrs, id)) {
			ret = -EEXIST;
			goto err_mr;
		}
		ret = xa_insert(&fixture->sim->dynamic_mrs, id, mr,
				GFP_KERNEL);
		if (!ret)
			fixture->sim->notification_pending = true;
	}
	if (ret)
		goto err_mr;
	handle = attestation_mr_create(
		ctlv_priv_parent(fixture->sim), &mr->props);
	if (IS_ERR(handle)) {
		ret = PTR_ERR(handle);
		scoped_guard(mutex, &fixture->sim->hardware_lock) {
			xa_erase(&fixture->sim->dynamic_mrs, id);
			fixture->sim->notification_pending = false;
		}
		goto err_mr;
	}
	scoped_guard(mutex, &fixture->sim->hardware_lock) {
		mr->handle = handle;
		fixture->sim->notification_pending = false;
	}
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

static int attestation_sim_fixture_create(const char *name)
{
	struct attestation_sim_init_info info;
	struct attestation_sim_fixture *fixture;
	char *parent_name;
	int ret;

	if (!*name || strlen(name) > ATTESTATION_SIM_NAME_MAX ||
	    strchr(name, '/'))
		return -EINVAL;
	guard(mutex)(&attestation_sim_fixtures_lock);
	if (attestation_sim_fixture_find(name))
		return -EEXIST;
	fixture = kzalloc_obj(*fixture, GFP_KERNEL);
	if (!fixture)
		return -ENOMEM;
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
	info.fixture = fixture;
	fixture->ctlvdev = ctlv_create(
		fixture->parent, &ctlv_attestation_sim_family, &info);
	if (IS_ERR(fixture->ctlvdev)) {
		ret = PTR_ERR(fixture->ctlvdev);
		fixture->ctlvdev = NULL;
		goto err_parent;
	}
	ret = attestation_sim_debugfs_fixture(fixture);
	if (ret)
		goto err_ctlv;
	list_add_tail(&fixture->node, &attestation_sim_fixtures);
	return 0;

err_ctlv:
	ctlv_destroy(fixture->ctlvdev);
err_parent:
	root_device_unregister(fixture->parent);
err_fixture:
	attestation_sim_fixture_free(fixture);
	return ret;
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

	ctlv_destroy(fixture->ctlvdev);
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
