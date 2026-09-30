// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
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
		size_t index;

		if (!object->format || !*object->format || strlen(object->format) > 128 ||
		    !object->max_length || object->max_length > UINT_MAX ||
		    object->max_length > caps->max_total_output_length)
			return -EINVAL;
		for (index = 0; index < object_index; index++) {
			const struct attestation_evidence_supported_object *prior =
				&caps->objects[index];

			if (!strcmp(prior->format, object->format))
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
	ret = attestation_evidence_caps_validate(provider);
	if (ret)
		return ret;
	if (!!provider->mrs != !!provider->mrs_count)
		return -EINVAL;
	if ((provider->mr_read || provider->mr_extend) && !provider->mrs_count)
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
	if (!xa_get_mark(&attestation_instances, index,
			 ATTESTATION_REGISTERED)) {
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

void attestation_mr_changed(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	att->log->generation++;
	att->mr_read_generation++;
}

void attestation_stop(struct attestation *att)
{
	lockdep_assert_held(&att->lock);
	xa_lock(&attestation_instances);
	__xa_clear_mark(&attestation_instances, att->index, ATTESTATION_REGISTERED);
	xa_unlock(&attestation_instances);
	attestation_kexec_invalidate(att);
}

static struct attestation_mr *attestation_mr_alloc(struct attestation *att,
						   const struct attestation_mr_props *props)
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
	mr->props = props;
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

void attestation_provide_ops(struct attestation *att)
{
	const struct attestation_provider *provider = att->provider;
	struct ctlv_device *ctlvdev = att->ctlvdev;

	if (provider->evidence_caps) {
		ctlv_attestation_evidence_get_provide(ctlvdev);
		ctlv_attestation_evidence_get_enable(ctlvdev);
	}
	if (provider->mrs_count) {
		ctlv_attestation_mr_get_provide(ctlvdev);
		ctlv_attestation_mr_dump_provide(ctlvdev);
		ctlv_attestation_mr_log_get_provide(ctlvdev);
		if (provider->mr_read)
			ctlv_attestation_mr_read_provide(ctlvdev);
		if (provider->mr_extend)
			ctlv_attestation_mr_extend_provide(ctlvdev);
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
	xa_init(&att->mrs);
	att->ctlvdev = ctlvdev;
	att->provider = provider;
	for (index = 0; index < provider->mrs_count; index++) {
		struct attestation_mr *mr;

		mr = attestation_mr_alloc(att, &provider->mrs[index]);
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
	struct attestation_mr *mr;
	unsigned long index;

	if (!att->ctlvdev)
		return;
	if (att->initialized)
		attestation_registry_remove(att);
	xa_for_each(&att->mrs, index, mr) {
		xa_erase(&att->mrs, index);
		kfree(mr);
	}
	attestation_mr_log_free(att->log);
	xa_destroy(&att->mrs);
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
