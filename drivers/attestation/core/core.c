// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <kunit/visibility.h>
#include <linux/attestation/ima.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/overflow.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "internal.h"

DEFINE_XARRAY(attestation_instances);
DEFINE_SRCU(attestation_instances_srcu);
EXPORT_SYMBOL_NS_GPL(attestation_instances_srcu, "ATTESTATION");

static DEFINE_MUTEX(attestation_registry_lock);
static unsigned long attestation_next_index;

static bool attestation_algorithm_valid(enum hash_algo algorithm)
{
	return algorithm >= 0 && algorithm < HASH_ALGO__LAST && hash_digest_size[algorithm] &&
	       hash_digest_size[algorithm] <= HASH_MAX_DIGESTSIZE;
}

static int attestation_mr_props_validate(const struct attestation_provider *provider,
					 const struct attestation_mr_props *props)
{
	size_t index;

	if (!props || !props->name || !*props->name || strlen(props->name) > 128 ||
	    !props->algorithms || !props->algorithms_count || props->algorithms_count > 21)
		return -EINVAL;
	for (index = 0; index < props->algorithms_count; index++) {
		enum hash_algo algorithm = props->algorithms[index];

		if (!attestation_algorithm_valid(algorithm) ||
		    (index && props->algorithms[index - 1] >= algorithm))
			return -EINVAL;
	}
	if (props->readable && !provider->mr_read)
		return -EINVAL;
	if (props->extendable && !provider->mr_extend)
		return -EINVAL;
	return 0;
}

static int attestation_evidence_caps_validate(const struct attestation_provider *provider)
{
	const struct attestation_evidence_caps *caps = provider->evidence_caps;
	size_t object_index;

	if (!caps)
		return provider->evidence_get ? -EINVAL : 0;
	if (!provider->evidence_get || !caps->objects || !caps->objects_count ||
	    caps->objects_count > 16 || caps->min_challenge_length > caps->max_challenge_length ||
	    caps->max_challenge_length > 4096 || !caps->max_total_output_length)
		return -EINVAL;
	for (object_index = 0; object_index < caps->objects_count; object_index++) {
		const struct attestation_evidence_supported_object *object =
			&caps->objects[object_index];
		bool tcb_digest = false;
		size_t index;

		if (!object->format || !*object->format || strlen(object->format) > 128 ||
		    !object->representations || !object->representations_count ||
		    object->representations_count > 2 || object->hash_algorithms_count > 21 ||
		    !object->max_length || object->max_length > UINT_MAX ||
		    object->max_length > caps->max_total_output_length)
			return -EINVAL;
		for (index = 0; index < object->representations_count; index++) {
			if (object->representations[index] !=
				    ATTESTATION_EVIDENCE_REPRESENTATION_RAW &&
			    object->representations[index] !=
				    ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST)
				return -EINVAL;
			if (index &&
			    object->representations[index - 1] >= object->representations[index])
				return -EINVAL;
			tcb_digest |= object->representations[index] ==
				      ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST;
		}
		if (tcb_digest && !object->hash_algorithms_count)
			return -EINVAL;
		for (index = 0; index < object->hash_algorithms_count; index++) {
			if (!attestation_algorithm_valid(object->hash_algorithms[index]))
				return -EINVAL;
			if (index &&
			    object->hash_algorithms[index - 1] >= object->hash_algorithms[index])
				return -EINVAL;
		}
		for (index = 0; index < object_index; index++) {
			const struct attestation_evidence_supported_object *prior =
				&caps->objects[index];

			if (prior->blob_class == object->blob_class &&
			    prior->has_blob_instance == object->has_blob_instance &&
			    (!object->has_blob_instance ||
			     prior->blob_instance == object->blob_instance) &&
			    !strcmp(prior->format, object->format))
				return -EINVAL;
		}
	}
	return 0;
}

int attestation_provider_validate(const struct attestation_provider *provider)
{
	size_t index;
	int ret;

	if (!provider)
		return -EINVAL;
	if (!!provider->evidence_accept != !!provider->evidence_accept_clear)
		return -EINVAL;
	ret = attestation_evidence_caps_validate(provider);
	if (ret)
		return ret;
	if ((provider->evidence_accept || provider->evidence_accept_clear) &&
	    !provider->evidence_caps)
		return -EINVAL;
	if (!provider->mr_caps &&
	    (provider->mr_read || provider->mr_extend || provider->mrs || provider->mrs_count))
		return -EINVAL;
	if (provider->mrs_count && !provider->mrs)
		return -EINVAL;
	for (index = 0; index < provider->mrs_count; index++) {
		ret = attestation_mr_props_validate(provider, &provider->mrs[index]);
		if (ret)
			return ret;
	}
	if (provider->devsec_run && (!provider->evidence_accept ||
				     (!provider->tdisp_state_get && !provider->device_trust_get)))
		return -EINVAL;
	return 0;
}

static bool attestation_parent_exists(struct device *parent)
{
	struct attestation *att;
	unsigned long index;

	xa_for_each(&attestation_instances, index, att)
		if (ctlv_device_parent(att->ctlvdev) == parent)
			return true;
	return false;
}

static int attestation_registry_insert(struct attestation *att)
{
	unsigned long index;
	void *old;
	int ret;

	guard(mutex)
		(&attestation_registry_lock);
	if (attestation_next_index > UINT_MAX)
		return -ENOSPC;
	index = attestation_next_index;
	ret = xa_reserve(&attestation_instances, index, GFP_KERNEL);
	if (ret)
		return ret;

	xa_lock(&attestation_instances);
	if (attestation_parent_exists(ctlv_device_parent(att->ctlvdev))) {
		__xa_erase(&attestation_instances, index);
		ret = -EEXIST;
		goto out_unlock;
	}
	old = __xa_store(&attestation_instances, index, att, GFP_NOWAIT);
	ret = xa_err(old);
	if (!ret) {
		att->index = index;
		attestation_next_index++;
	}
out_unlock:
	xa_unlock(&attestation_instances);
	return ret;
}

static void attestation_registry_remove(struct attestation *att)
{
	xa_erase(&attestation_instances, att->index);
}

bool attestation_registered(struct attestation *att)
{
	bool registered;

	lockdep_assert_held(&att->lock);
	xa_lock(&attestation_instances);
	registered = xa_get_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	return registered;
}

struct attestation *attestation_instance_lock(unsigned int index)
{
	struct attestation *att;

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

void attestation_instance_unlock(struct attestation *att)
{
	mutex_unlock(&att->lock);
}

#if IS_ENABLED(CONFIG_KUNIT)
struct attestation *
attestation_test_find_device(const struct ctlv_device *ctlvdev)
{
	struct attestation *att;
	unsigned long index;

	xa_for_each(&attestation_instances, index, att)
		if (att->ctlvdev == ctlvdev)
			return att;
	return NULL;
}
EXPORT_SYMBOL_IF_KUNIT(attestation_test_find_device);
#endif

void attestation_evidence_accept_clear_locked(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	if (!att->verdict_recorded)
		return;
	att->provider->evidence_accept_clear(att);
	att->verdict_recorded = false;
}

static void attestation_evidence_ranges_clear(struct attestation *att, bool dependent_only)
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

void attestation_device_changed(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	attestation_evidence_accept_clear_locked(att);
	attestation_evidence_ranges_clear(att, false);
	att->device_generation++;
	att->evidence_floor = att->evidence_generation;
	att->mr_read_generation++;
	att->devsec_version++;
}

void attestation_mr_changed(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	att->log->generation++;
	att->mr_read_generation++;
	attestation_evidence_ranges_clear(att, true);
}

void attestation_evidence_verdict_changed(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	att->devsec_version++;
}

void attestation_stop(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	xa_lock(&attestation_instances);
	__xa_clear_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	attestation_evidence_accept_clear_locked(att);
	attestation_kexec_invalidate(att);
}

int attestation_preflight(struct attestation *att, bool device, bool evidence, bool mr)
{
	lockdep_assert_held(&att->lock);
	if ((device && att->device_generation == UINT_MAX) ||
	    (evidence && att->evidence_generation == UINT_MAX) ||
	    (mr && att->log && att->log->generation == UINT_MAX)) {
		attestation_stop(att);
		return -EOVERFLOW;
	}
	return 0;
}

static struct attestation_mr *attestation_mr_alloc(struct attestation *att,
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

struct attestation_mr *attestation_mr_lookup_live(struct attestation *att, unsigned int mr_id)
{
	struct attestation_mr *mr;

	lockdep_assert_held(&att->lock);
	mr = xa_load(&att->mrs, mr_id);
	return mr ? mr : ERR_PTR(-ENOENT);
}

static int attestation_dynamic_mr_publish(struct attestation_mr *mr)
{
	struct attestation *att = mr->att;
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

struct attestation_mr *attestation_mr_create(struct attestation *att,
					     const struct attestation_mr_props *props)
{
	struct attestation_mr *mr;
	int ret;

	guard(mutex)
		(&att->lock);
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

void attestation_mr_destroy(struct attestation_mr *mr)
{
	struct attestation *att;

	if (WARN_ON_ONCE(!mr->removable))
		return;
	att = mr->att;
	guard(mutex)
		(&att->lock);
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

void attestation_state_changed(struct attestation *att)
{
	struct attestation_event_batch events = {};
	bool evidence;
	int ret;

	guard(mutex)
		(&att->lock);
	if (!attestation_registered(att))
		return;
	evidence = att->evidence_generation > att->evidence_floor || att->verdict_recorded;
	attestation_kexec_invalidate(att);
	ret = attestation_preflight(att, true, false, false);
	if (ret)
		return;
	ret = attestation_events_prepare(att, &events, true, evidence, false);
	attestation_device_changed(att);
	if (ret) {
		ctlv_event_lost(att->ctlvdev, CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED);
		if (evidence)
			ctlv_event_lost(att->ctlvdev, CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);
		return;
	}
	attestation_device_event_emit(&events, att->device_generation);
	if (evidence)
		attestation_evidence_event_emit(&events, att);
	attestation_events_abort(&events);
}
EXPORT_SYMBOL_NS_GPL(attestation_state_changed, "ATTESTATION");

void attestation_provide_ops(struct attestation *att)
{
	const struct attestation_provider *provider = att->provider;
	struct ctlv_device *ctlvdev = att->ctlvdev;

	if (provider->evidence_caps) {
		ctlv_attestation_evidence_get_provide(ctlvdev);
		ctlv_attestation_evidence_get_enable(ctlvdev);
		if (provider->evidence_accept) {
			ctlv_attestation_evidence_accept_provide(ctlvdev);
			ctlv_attestation_evidence_accept_clear_provide(ctlvdev);
		}
	}
	if (provider->mr_caps) {
		ctlv_attestation_mr_get_provide(ctlvdev);
		ctlv_attestation_mr_dump_provide(ctlvdev);
		ctlv_attestation_mr_log_get_provide(ctlvdev);
		if (provider->mr_read)
			ctlv_attestation_mr_read_provide(ctlvdev);
		if (provider->mr_extend)
			ctlv_attestation_mr_extend_provide(ctlvdev);
	}
	if (provider->tdisp_state_get || provider->device_trust_get || provider->evidence_accept)
		ctlv_attestation_devsec_state_get_provide(ctlvdev);
	if (provider->devsec_run) {
		ctlv_attestation_devsec_run_provide(ctlvdev);
		ctlv_attestation_devsec_run_enable(ctlvdev);
	}
}

int attestation_core_init(struct attestation *att, struct ctlv_device *ctlvdev,
			  const struct attestation_provider *provider)
{
	size_t index;
	int ret;

	ret = attestation_provider_validate(provider);
	if (ret)
		return ret;
	mutex_init(&att->lock);
	xa_init(&att->evidence_ranges);
	xa_init(&att->mrs);
	att->ctlvdev = ctlvdev;
	att->provider = provider;
	for (index = 0; index < provider->mrs_count; index++) {
		struct attestation_mr *mr;

		mr = attestation_mr_alloc(att, &provider->mrs[index], false);
		if (IS_ERR(mr)) {
			ret = PTR_ERR(mr);
			goto err_mrs;
		}
	}
	ret = attestation_registry_insert(att);
	if (ret)
		goto err_mrs;
	attestation_provide_ops(att);
	att->initialized = true;
	return 0;

err_mrs:
	attestation_core_fini(att);
	return ret;
}

void attestation_core_fini(struct attestation *att)
{
	struct attestation_evidence_range *range;
	struct attestation_mr *mr;
	unsigned long index;

	if (!att->ctlvdev)
		return;
	if (att->initialized)
		attestation_registry_remove(att);
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
	att->ctlvdev = NULL;
}

size_t ctlv_attestation_priv_size(void)
{
	return sizeof(struct attestation);
}

int ctlv_attestation_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv)
{
	return attestation_core_init(priv, ctlvdev, init_info);
}

int ctlv_attestation_post_init(struct ctlv_device *ctlvdev, void *priv)
{
	struct attestation *att = priv;
	int ret;

	if (att->provider->mr_caps && xa_empty(&att->mrs) &&
	    !att->provider->mr_caps->supports_dynamic_topology)
		return -EINVAL;
	ret = attestation_kexec_restore(att);
	if (ret < 0)
		return ret;
	att->kexec_restored = ret > 0;
	if (!ret) {
		ret = attestation_mr_log_init(att);
		if (ret)
			return ret;
	}
	xa_lock(&attestation_instances);
	__xa_set_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	return 0;
}

void ctlv_attestation_pre_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct attestation *att = priv;

	xa_lock(&attestation_instances);
	__xa_clear_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	attestation_kexec_invalidate(att);
	synchronize_srcu(&attestation_instances_srcu);
}

void ctlv_attestation_fini(struct ctlv_device *ctlvdev, void *priv)
{
	attestation_core_fini(priv);
}

int ctlv_attestation_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	struct attestation *att = priv;

	mutex_lock(&att->lock);
	if (!attestation_registered(att)) {
		mutex_unlock(&att->lock);
		return -ENODEV;
	}
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attestation_op_pre, "ATTESTATION");

void ctlv_attestation_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
	struct attestation *att = priv;

	mutex_unlock(&att->lock);
}
EXPORT_SYMBOL_NS_GPL(ctlv_attestation_op_post, "ATTESTATION");

static int __init attestation_module_init(void)
{
	return attestation_kexec_init();
}

static void __exit attestation_module_exit(void)
{
	attestation_kexec_exit();
	WARN_ON_ONCE(!xa_empty(&attestation_instances));
	xa_destroy(&attestation_instances);
}

module_init(attestation_module_init);
module_exit(attestation_module_exit);
MODULE_DESCRIPTION("Attestation core");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("CTLV");
