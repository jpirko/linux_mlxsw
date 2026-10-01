// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <kunit/visibility.h>
#include <linux/attestation/ima.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/ctlv_device.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/overflow.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "internal.h"

#define ATTESTATION_MAX_DEVICES 4096

DEFINE_XARRAY(attestation_instances);
DEFINE_SRCU(attestation_instances_srcu);
EXPORT_SYMBOL_NS_GPL(attestation_instances_srcu, "ATTESTATION");

static DEFINE_MUTEX(attestation_registry_lock);
/* Never reused, so a stale index cannot name a newer instance. */
static unsigned long attestation_next_index;

/* A reply carries only what hash-algo in ctlv-attestation.yaml lists. */
static bool attestation_algorithm_valid(enum hash_algo algorithm)
{
	return algorithm >= 0 && algorithm <= HASH_ALGO_SHA3_512 && hash_digest_size[algorithm] &&
	       hash_digest_size[algorithm] <= HASH_MAX_DIGESTSIZE;
}

static int attestation_mr_props_validate(const struct attestation_provider *provider,
					 const struct attestation_mr_props *props)
{
	DECLARE_BITMAP(seen, HASH_ALGO__LAST) = {};
	size_t index;

	if (!props || !props->name || !*props->name || strlen(props->name) > 128 ||
	    !props->algorithms || !props->algorithms_count || props->algorithms_count > 23)
		return -EINVAL;
	for (index = 0; index < props->algorithms_count; index++) {
		enum hash_algo algorithm = props->algorithms[index];

		if (!attestation_algorithm_valid(algorithm) ||
		    __test_and_set_bit(algorithm, seen))
			return -EINVAL;
	}
	if (props->extendable && !provider->mr_extend)
		return -EINVAL;
	return 0;
}

/* What device-get and evidence-get carry, as ctlv-attestation.yaml bounds it. */
#define ATTESTATION_EVIDENCE_OBJECTS_MAX 16
#define ATTESTATION_EVIDENCE_FORMATS_MAX 4
#define ATTESTATION_CHALLENGE_LENGTHS_MAX 8
#define ATTESTATION_CHALLENGE_MAX 4096
/* Moves with each value appended to evidence-format. */
#define ATTESTATION_EVIDENCE_FORMAT_LAST CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EK_CERT

static bool
attestation_challenge_lengths_valid(const struct attestation_evidence_supported_object *object)
{
	size_t index;

	if (object->evidence_class != ATTESTATION_EVIDENCE_SIGNED)
		return !object->challenge_lengths_count;
	if (!object->challenge_lengths || !object->challenge_lengths_count ||
	    object->challenge_lengths_count > ATTESTATION_CHALLENGE_LENGTHS_MAX)
		return false;
	for (index = 0; index < object->challenge_lengths_count; index++) {
		const struct attestation_evidence_challenge_length *length =
			&object->challenge_lengths[index];

		if (!length->min || length->min > length->max ||
		    length->max > ATTESTATION_CHALLENGE_MAX ||
		    (index && object->challenge_lengths[index - 1].max >= length->min))
			return false;
	}
	return true;
}

static bool
attestation_formats_valid(const struct attestation_evidence_supported_object *object,
			  u32 vendor_max)
{
	size_t counts[2] = {}, index, prior;

	if (!object->formats || !object->formats_count)
		return false;
	for (index = 0; index < object->formats_count; index++) {
		const struct attestation_evidence_format *format = &object->formats[index];
		u32 max = format->vendor ? vendor_max : ATTESTATION_EVIDENCE_FORMAT_LAST;

		if (!format->value || format->value > max ||
		    ++counts[format->vendor] > ATTESTATION_EVIDENCE_FORMATS_MAX)
			return false;
		for (prior = 0; prior < index; prior++)
			if (object->formats[prior].vendor == format->vendor &&
			    object->formats[prior].value == format->value)
				return false;
	}
	return true;
}

static int attestation_evidence_caps_validate(const struct attestation_provider *provider,
					      const struct ctlv_family *family)
{
	const struct attestation_evidence_caps *caps = provider->evidence_caps;
	size_t index;
	u32 vendor_max;

	if (!caps)
		return provider->evidence_get ? -EINVAL : 0;
	if (!provider->evidence_get || !caps->objects || !caps->objects_count ||
	    caps->objects_count > ATTESTATION_EVIDENCE_OBJECTS_MAX)
		return -EINVAL;

	vendor_max = ctlv_enum_ext_max(family, CTLV_FAMILY_ATTESTATION,
				       CTLV_ATTESTATION_ENUM_EXT_EVIDENCE_VENDOR_FORMAT);
	for (index = 0; index < caps->objects_count; index++) {
		const struct attestation_evidence_supported_object *object = &caps->objects[index];

		if (object->evidence_class < ATTESTATION_EVIDENCE_SIGNED ||
		    object->evidence_class > ATTESTATION_EVIDENCE_AUXILIARY ||
		    !attestation_formats_valid(object, vendor_max) ||
		    !attestation_challenge_lengths_valid(object) ||
		    (object->has_tcb_digest &&
		     !attestation_algorithm_valid(object->tcb_digest_algorithm)) ||
		    !object->max_length || object->max_length > UINT_MAX)
			return -EINVAL;
	}
	return 0;
}

int attestation_provider_validate(const struct attestation_provider *provider,
				  const struct ctlv_family *family)
{
	size_t index;
	int ret;

	if (!provider)
		return -EINVAL;
	ret = attestation_evidence_caps_validate(provider, family);
	if (ret)
		return ret;
	if (provider->evidence_accept && !provider->evidence_caps)
		return -EINVAL;
	if (!provider->mr_caps &&
	    (provider->mr_read || provider->mr_extend || provider->mrs || provider->mrs_count))
		return -EINVAL;
	if (provider->mr_caps && !provider->mr_read)
		return -EINVAL;
	if (provider->mrs_count && !provider->mrs)
		return -EINVAL;
	for (index = 0; index < provider->mrs_count; index++) {
		ret = attestation_mr_props_validate(provider, &provider->mrs[index]);
		if (ret)
			return ret;
	}
	return 0;
}

static bool attestation_parent_exists(struct device *parent)
{
	struct attestation_dev *att;
	unsigned long index;

	xa_for_each(&attestation_instances, index, att)
		if (att->node.dev.parent == parent)
			return true;
	return false;
}

static int attestation_registry_insert(struct attestation_dev *att)
{
	unsigned long index;
	void *old;
	int ret;

	guard(mutex)(&attestation_registry_lock);
	if (attestation_next_index > UINT_MAX)
		return -ENOSPC;
	index = attestation_next_index;
	ret = xa_reserve(&attestation_instances, index, GFP_KERNEL);
	if (ret)
		return ret;

	xa_lock(&attestation_instances);
	if (attestation_parent_exists(att->node.dev.parent)) {
		__xa_erase(&attestation_instances, index);
		ret = -EEXIST;
	} else {
		old = __xa_store(&attestation_instances, index, att, GFP_NOWAIT);
		ret = xa_err(old);
	}
	if (!ret) {
		att->index = index;
		attestation_next_index++;
	}
	xa_unlock(&attestation_instances);
	return ret;
}

static void attestation_registry_remove(struct attestation_dev *att)
{
	xa_erase(&attestation_instances, att->index);
}

bool attestation_registered(struct attestation_dev *att)
{
	bool registered;

	lockdep_assert_held(&att->lock);
	xa_lock(&attestation_instances);
	registered = xa_get_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	return registered;
}

struct attestation_dev *attestation_instance_lock(unsigned int index)
{
	struct attestation_dev *att;

	xa_lock(&attestation_instances);
	if (!xa_get_mark(&attestation_instances, index, ATTESTATION_REGISTERED)) {
		xa_unlock(&attestation_instances);
		return ERR_PTR(-ENODEV);
	}
	att = xa_load(&attestation_instances, index);
	xa_unlock(&attestation_instances);
	if (!att)
		return ERR_PTR(-ENODEV);

	mutex_lock(&att->lock);
	if (!attestation_registered(att)) {
		mutex_unlock(&att->lock);
		return ERR_PTR(-ENODEV);
	}
	return att;
}

void attestation_instance_unlock(struct attestation_dev *att)
{
	mutex_unlock(&att->lock);
}

static void attestation_evidence_ranges_clear(struct attestation_dev *att, bool dependent_only)
{
	struct attestation_evidence_range *range;
	unsigned long index;

	xa_for_each(&att->evidence_ranges, index, range) {
		if (dependent_only && !range->mr_dependent)
			continue;
		xa_erase(&att->evidence_ranges, index);
		kfree(range);
	}
}

void attestation_device_changed(struct attestation_dev *att)
{
	lockdep_assert_held(&att->lock);
	attestation_evidence_ranges_clear(att, false);
	att->device_generation++;
	att->evidence_floor = att->evidence_generation;
	att->mr_read_generation++;
	att->devsec_version++;
}

void attestation_mr_changed(struct attestation_dev *att)
{
	lockdep_assert_held(&att->lock);
	att->log->generation++;
	att->mr_read_generation++;
	attestation_evidence_ranges_clear(att, true);
}

void attestation_stop(struct attestation_dev *att)
{
	lockdep_assert_held(&att->lock);
	xa_lock(&attestation_instances);
	__xa_clear_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	attestation_kexec_invalidate(att);
}

int attestation_preflight(struct attestation_dev *att, bool device, bool evidence, bool mr)
{
	const char *exhausted;

	lockdep_assert_held(&att->lock);
	if (device && att->device_generation == UINT_MAX)
		exhausted = "device";
	else if (evidence && att->evidence_generation == UINT_MAX)
		exhausted = "evidence";
	else if (mr && att->log && att->log->generation == UINT_MAX)
		exhausted = "mr";
	else
		return 0;
	/* A stopped instance can still be asked, so only the stop itself warns. */
	if (attestation_registered(att))
		dev_warn(att->node.dev.parent,
			 "attestation instance %u stopped: %s generation exhausted\n",
			 att->index, exhausted);
	attestation_stop(att);
	return -EOVERFLOW;
}

static struct attestation_mr *attestation_mr_alloc(struct attestation_dev *att,
						   const struct attestation_mr_props *props,
						   bool removable)
{
	struct attestation_mr *mr;
	int ret;

	ret = attestation_mr_props_validate(att->provider, props);
	if (ret)
		return ERR_PTR(ret);
	if (xa_load(&att->mrs, props->id))
		return ERR_PTR(-EEXIST);
	mr = kzalloc_obj(*mr, GFP_KERNEL);
	if (!mr)
		return ERR_PTR(-ENOMEM);
	mr->att = att;
	mr->props = props;
	mr->removable = removable;
	ret = xa_insert(&att->mrs, props->id, mr, GFP_KERNEL);
	if (ret) {
		kfree(mr);
		return ERR_PTR(ret);
	}
	return mr;
}

struct attestation_mr *attestation_mr_lookup_live(struct attestation_dev *att, unsigned int mr_id)
{
	struct attestation_mr *mr;

	lockdep_assert_held(&att->lock);
	mr = xa_load(&att->mrs, mr_id);
	return mr ? mr : ERR_PTR(-ENOENT);
}

static int attestation_dynamic_mr_publish(struct attestation_mr *mr)
{
	struct attestation_dev *att = mr->att;
	struct attestation_mr_log_entry_info *info;
	struct attestation_event_batch events = {};
	int ret;

	ret = attestation_preflight(att, false, false, true);
	if (ret)
		return ret;

	info = attestation_mr_info_alloc(mr);
	if (IS_ERR(info))
		return PTR_ERR(info);
	ret = attestation_events_prepare(att, &events, false, false, true);
	if (ret)
		goto err_info;

	info->entry.mr_generation = att->log->generation + 1;
	ret = xa_reserve(&att->log->entries, info->entry.mr_generation, GFP_KERNEL);
	if (ret)
		goto err_events;

	attestation_mr_changed(att);
	info->entry.device_generation = att->device_generation;
	mr->start_generation = info->entry.mr_generation;
	attestation_mr_log_append(att, &info->entry);
	attestation_mr_log_event_emit(&events, &info->entry);
	return 0;

err_events:
	attestation_events_abort(&events);
err_info:
	attestation_mr_log_entry_free(&info->entry);
	return ret;
}

/**
 * attestation_mr_create - Create a dynamic MR.
 * @att: provider instance
 * @props: immutable MR properties
 *
 * Return: MR handle, or an error pointer.
 */
struct attestation_mr *attestation_mr_create(struct attestation_dev *att,
					     const struct attestation_mr_props *props)
{
	struct attestation_mr *mr;
	int ret;

	guard(mutex)(&att->lock);
	if (!att->provider->mr_caps || !att->provider->mr_caps->supports_dynamic_topology)
		return ERR_PTR(-EOPNOTSUPP);
	if (att->log && att->log->frozen)
		return ERR_PTR(-EBUSY);
	mr = attestation_mr_alloc(att, props, true);
	if (IS_ERR(mr))
		return mr;
	if (!attestation_registered(att))
		return mr;
	ret = attestation_dynamic_mr_publish(mr);
	if (ret) {
		xa_erase(&att->mrs, props->id);
		kfree(mr);
		return ERR_PTR(ret);
	}
	return mr;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_create, "ATTESTATION");

/**
 * attestation_mr_destroy - Destroy a dynamic MR.
 * @mr: MR handle returned by attestation_mr_create()
 */
void attestation_mr_destroy(struct attestation_mr *mr)
{
	struct attestation_dev *att;

	if (WARN_ON_ONCE(!mr->removable))
		return;
	att = mr->att;
	guard(mutex)(&att->lock);
	if (WARN_ON_ONCE(xa_load(&att->mrs, mr->props->id) != mr))
		return;
	attestation_kexec_invalidate(att);
	xa_erase(&att->mrs, mr->props->id);
	if (attestation_registered(att)) {
		if (attestation_preflight(att, false, false, true))
			attestation_stop(att);
		else
			attestation_mr_changed(att);
	}
	kfree(mr);
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_destroy, "ATTESTATION");

/**
 * attestation_state_changed_locked - Report a native provider state change.
 * @att: provider instance
 *
 * Call with the instance mutex held, as from a provider callback that finds
 * the state changed.
 */
void attestation_state_changed_locked(struct attestation_dev *att)
{
	struct attestation_event_batch events = {};
	bool evidence;
	int ret;

	lockdep_assert_held(&att->lock);
	if (!attestation_registered(att))
		return;

	evidence = att->evidence_generation > att->evidence_floor;
	attestation_kexec_invalidate(att);
	ret = attestation_preflight(att, true, false, false);
	if (ret)
		return;

	ret = attestation_events_prepare(att, &events, true, evidence, false);
	attestation_device_changed(att);
	if (ret) {
		ctlv_event_lost(&att->ctlv, CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED);
		if (evidence)
			ctlv_event_lost(&att->ctlv, CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);
		return;
	}

	attestation_device_event_emit(&events, att->device_generation);
	if (evidence)
		attestation_evidence_event_emit(&events, att);
	attestation_events_abort(&events);
}
EXPORT_SYMBOL_NS_GPL(attestation_state_changed_locked, "ATTESTATION");

/**
 * attestation_state_changed - Report a native provider state change.
 * @att: provider instance
 *
 * Call without the instance mutex or conflicting native locks held.
 */
void attestation_state_changed(struct attestation_dev *att)
{
	guard(mutex)(&att->lock);
	attestation_state_changed_locked(att);
}
EXPORT_SYMBOL_NS_GPL(attestation_state_changed, "ATTESTATION");

/**
 * attestation_evidence_accept_enable - Set whether evidence-accept can run.
 * @att: device of a provider with evidence_accept
 * @enable: whether its native transition is possible now
 *
 * The operation starts enabled. Call this before attestation_register() and on
 * every native change that decides it, from evidence_accept too.
 */
void attestation_evidence_accept_enable(struct attestation_dev *att, bool enable)
{
	if (enable)
		ctlv_attestation_evidence_accept_enable(&att->ctlv);
	else
		ctlv_attestation_evidence_accept_disable(&att->ctlv);
}
EXPORT_SYMBOL_NS_GPL(attestation_evidence_accept_enable, "ATTESTATION");

/**
 * attestation_evidence_write - Write an evidence object to its blob.
 * @object: the requested object
 * @data: the object's bytes
 * @length: their length, at most the object's max_length
 *
 * A blob too short for the object fails the operation, and the caller is told
 * how long it has to be.
 *
 * Return: 0, -EMSGSIZE where the blob is too short, or another negative error.
 */
int attestation_evidence_write(const struct attestation_evidence_object_request *object,
			       const void *data, size_t length)
{
	if (length > object->supported_object->max_length)
		return -E2BIG;
	if (length > ctlv_blob_len(object->output))
		return ctlv_blob_too_short(object->output, length);
	return ctlv_blob_write(object->output, 0, data, length);
}
EXPORT_SYMBOL_NS_GPL(attestation_evidence_write, "ATTESTATION");

void attestation_provide_ops(struct attestation_dev *att)
{
	const struct attestation_provider *provider = att->provider;
	struct ctlv *ctlv = &att->ctlv;

	if (provider->evidence_caps) {
		ctlv_attestation_evidence_get_provide(ctlv);
		ctlv_attestation_evidence_get_enable(ctlv);
		if (provider->evidence_accept) {
			ctlv_attestation_evidence_accept_provide(ctlv);
			ctlv_attestation_evidence_accept_enable(ctlv);
		}
	}

	if (provider->mr_caps) {
		ctlv_attestation_mr_get_provide(ctlv);
		ctlv_attestation_mr_log_get_provide(ctlv);
		ctlv_attestation_mr_read_provide(ctlv);
		if (provider->mr_extend)
			ctlv_attestation_mr_extend_provide(ctlv);
	}

	if (provider->tdisp_state_get)
		ctlv_attestation_devsec_state_get_provide(ctlv);
}

static int attestation_mrs_create(struct attestation_dev *att)
{
	const struct attestation_provider *provider = att->provider;
	size_t index;

	for (index = 0; index < provider->mrs_count; index++) {
		struct attestation_mr *mr;

		mr = attestation_mr_alloc(att, &provider->mrs[index], false);
		if (IS_ERR(mr))
			return PTR_ERR(mr);
	}
	return 0;
}

/* Runs from the release, so it must not reach the provider's module. */
static void attestation_state_free(struct attestation_dev *att)
{
	struct attestation_evidence_range *range;
	struct attestation_mr *mr;
	unsigned long index;

	xa_for_each(&att->evidence_ranges, index, range) {
		xa_erase(&att->evidence_ranges, index);
		kfree(range);
	}
	xa_for_each(&att->mrs, index, mr) {
		xa_erase(&att->mrs, index);
		kfree(mr);
	}
	attestation_mr_log_free(att->log);
	xa_destroy(&att->mrs);
	xa_destroy(&att->evidence_ranges);
	mutex_destroy(&att->lock);
}

static int attestation_op_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
	struct attestation_dev *att = attestation_from_ctlv(ctlv);

	mutex_lock(&att->lock);
	if (!attestation_registered(att)) {
		mutex_unlock(&att->lock);
		return -ENODEV;
	}
	return 0;
}

static void attestation_op_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
	mutex_unlock(&attestation_from_ctlv(ctlv)->lock);
}

static const struct ctlv_hooks attestation_ctlv_hooks = {
	.pre = attestation_op_pre,
	.post = attestation_op_post,
};

/* Kernel callers find a device by its mark, and are drained once it is gone. */
static void attestation_withdraw(struct attestation_dev *att)
{
	xa_lock(&attestation_instances);
	__xa_clear_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	attestation_kexec_invalidate(att);
	synchronize_srcu(&attestation_instances_srcu);
}

static char *attestation_devnode(const struct device *dev, umode_t *mode)
{
	return kasprintf(GFP_KERNEL, "attestation/%s", dev_name(dev));
}

static void attestation_dev_release(struct device *dev)
{
	struct attestation_dev *att = container_of(dev, struct attestation_dev, node.dev);

	attestation_state_free(att);
	ctlv_fini(&att->ctlv);
	ctlv_device_fini(&att->node);
	kfree(att);
}

/*
 * The core registers the class, so an open holds the core module and not the
 * provider's, which may unload while readers remain.
 */
static struct ctlv_device_class attestation_class = {
	.class = {
		.name = "attestation",
		.dev_release = attestation_dev_release,
		.devnode = attestation_devnode,
	},
	.max_devices = ATTESTATION_MAX_DEVICES,
};

struct attestation_dev *_attestation_alloc(struct device *parent,
					   const struct attestation_provider *provider,
					   const struct ctlv_family *family, size_t size,
					   size_t offset)
{
	struct attestation_dev *att;
	int ret;

	if (!parent || WARN_ON_ONCE(offset || size < sizeof(*att)) ||
	    !ctlv_family_inherits(family, &ctlv_attestation_family))
		return ERR_PTR(-EINVAL);
	ret = attestation_provider_validate(provider, family);
	if (ret)
		return ERR_PTR(ret);

	att = kzalloc(size, GFP_KERNEL);
	if (!att)
		return ERR_PTR(-ENOMEM);
	att->provider = provider;
	mutex_init(&att->lock);
	xa_init(&att->evidence_ranges);
	xa_init(&att->mrs);

	/* From here on, the release frees whatever was set up. */
	ret = ctlv_device_init(&att->node, &attestation_class, parent, &att->ctlv);
	if (!ret)
		ret = ctlv_init(&att->ctlv, &att->node.dev, family, &attestation_ctlv_hooks);
	if (!ret)
		ret = attestation_mrs_create(att);
	if (ret) {
		put_device(&att->node.dev);
		return ERR_PTR(ret);
	}
	attestation_provide_ops(att);
	return att;
}
EXPORT_SYMBOL_NS_GPL(_attestation_alloc, "ATTESTATION");

/**
 * attestation_register - Make an allocated device reachable.
 * @att: device from attestation_alloc()
 *
 * Return: 0, or a negative errno with @att still to be put.
 */
int attestation_register(struct attestation_dev *att)
{
	int ret;

	if (att->provider->mr_caps && xa_empty(&att->mrs) &&
	    !att->provider->mr_caps->supports_dynamic_topology)
		return -EINVAL;

	ret = attestation_registry_insert(att);
	if (ret)
		return ret;
	ctlv_publish(&att->ctlv);

	ret = attestation_kexec_restore(att);
	if (ret < 0)
		goto err_withdraw;
	att->kexec_restored = ret > 0;
	if (!ret) {
		ret = attestation_mr_log_init(att);
		if (ret)
			goto err_withdraw;
	}

	xa_lock(&attestation_instances);
	__xa_set_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);

	ret = ctlv_device_add(&att->node);
	if (ret)
		goto err_withdraw;
	return 0;

err_withdraw:
	attestation_withdraw(att);
	ctlv_unpublish(&att->ctlv);
	attestation_registry_remove(att);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(attestation_register, "ATTESTATION");

/**
 * attestation_unregister - Cut a registered device off from every caller.
 * @att: registered device
 *
 * No provider callback runs once this returns, though open files may still
 * hold @att.
 */
void attestation_unregister(struct attestation_dev *att)
{
	/* No new opens, and no node to open. */
	ctlv_device_del(&att->node);
	attestation_withdraw(att);
	ctlv_unpublish(&att->ctlv);
	attestation_registry_remove(att);
}
EXPORT_SYMBOL_NS_GPL(attestation_unregister, "ATTESTATION");

/**
 * attestation_put - Drop the reference attestation_alloc() returned.
 * @att: device
 */
void attestation_put(struct attestation_dev *att)
{
	put_device(&att->node.dev);
}
EXPORT_SYMBOL_NS_GPL(attestation_put, "ATTESTATION");

static int __init attestation_module_init(void)
{
	int ret;

	ret = ctlv_device_class_register(&attestation_class);
	if (ret)
		return ret;
	ret = attestation_kexec_init();
	if (ret)
		ctlv_device_class_unregister(&attestation_class);
	return ret;
}

static void __exit attestation_module_exit(void)
{
	attestation_kexec_exit();
	WARN_ON_ONCE(!xa_empty(&attestation_instances));
	xa_destroy(&attestation_instances);
	ctlv_device_class_unregister(&attestation_class);
}

module_init(attestation_module_init);
module_exit(attestation_module_exit);
MODULE_DESCRIPTION("Attestation core");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("CTLV");
