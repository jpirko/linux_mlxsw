// SPDX-License-Identifier: GPL-2.0-only
#include <linux/attestation/ima.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "internal.h"

static bool attestation_family_matches(const struct attestation *att,
				       const char *name)
{
	const struct ctlv_family *family;

	if (!name)
		return true;
	for (family = ctlv_device_family(att->ctlvdev);
	     family; family = family->parent)
		if (!strcmp(family->schema->name, name))
			return true;
	return false;
}

static bool attestation_parent_matches(const struct attestation *att,
				       const char *bus_name,
				       const char *device_name)
{
	struct device *parent;
	const char *bus;

	if (!bus_name)
		return true;
	parent = ctlv_device_parent(att->ctlvdev);
	bus = dev_bus_name(parent);
	return bus && !strcmp(bus, bus_name) &&
	       !strcmp(dev_name(parent), device_name);
}

static int attestation_selector_validate(const char *family_name,
					 const char *bus_name,
					 const char *device_name)
{
	if (!family_name && !bus_name && !device_name)
		return -EINVAL;
	if (!!bus_name != !!device_name)
		return -EINVAL;
	if ((family_name && !*family_name) ||
	    (bus_name && (!*bus_name || !*device_name)))
		return -EINVAL;
	return 0;
}

static bool attestation_target_matches(struct attestation *att,
				       const char *family_name,
				       const char *bus_name,
				       const char *device_name,
				       unsigned int mr_id,
				       struct attestation_mr **mr_out)
{
	struct attestation_mr *mr;

	if (!attestation_family_matches(att, family_name) ||
	    !attestation_parent_matches(att, bus_name, device_name))
		return false;
	mr = xa_load(&att->mrs, mr_id);
	if (!mr || !mr->props->extendable)
		return false;
	*mr_out = mr;
	return true;
}

struct attestation_mr_target *
attestation_mr_target_get(const char *family_name, const char *bus_name,
			  const char *device_name, unsigned int mr_id)
{
	struct attestation_mr_target *target;
	struct attestation_mr *mr;
	struct attestation *att;
	unsigned long index;
	unsigned int match = UINT_MAX;
	int srcu_index;
	int ret;

	ret = attestation_selector_validate(family_name, bus_name, device_name);
	if (ret)
		return ERR_PTR(ret);
	target = kzalloc_obj(*target, GFP_KERNEL);
	if (!target)
		return ERR_PTR(-ENOMEM);

	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	index = 0;
	for (;;) {
		rcu_read_lock();
		att = xa_find(&attestation_instances, &index, UINT_MAX,
			      XA_PRESENT);
		rcu_read_unlock();
		if (!att)
			break;
		att = attestation_instance_lock(index);
		if (!IS_ERR(att)) {
			if (attestation_target_matches(att, family_name, bus_name,
						       device_name, mr_id, &mr)) {
				if (match != UINT_MAX) {
					attestation_instance_unlock(att);
					ret = -ENOTUNIQ;
					goto out_srcu;
				}
				match = index;
			}
			attestation_instance_unlock(att);
		}
		if (index == UINT_MAX)
			break;
		index++;
	}
	if (match == UINT_MAX) {
		ret = -ENODEV;
		goto out_srcu;
	}
	att = attestation_instance_lock(match);
	if (IS_ERR(att)) {
		ret = PTR_ERR(att);
		goto out_srcu;
	}
	if (!attestation_target_matches(att, family_name, bus_name, device_name,
					mr_id, &mr)) {
		ret = -ENODEV;
		goto out_unlock;
	}
	if (mr->state_flags & CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE) {
		ret = -EBUSY;
		goto out_unlock;
	}
	ret = attestation_preflight(att, false, true);
	if (ret)
		goto out_unlock;
	mr->state_flags |= CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE;
	attestation_mr_changed(att);
	target->index = match;
	target->mr_id = mr_id;
	target->start_generation = mr->start_generation;
	target->ima_owner = true;
	attestation_instance_unlock(att);
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return target;

out_unlock:
	attestation_instance_unlock(att);
out_srcu:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	kfree(target);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_get, "ATTESTATION");

void attestation_mr_target_put(struct attestation_mr_target *target)
{
	struct attestation_mr *mr;
	struct attestation *att;
	int srcu_index;

	if (!target)
		return;
	if (!target->ima_owner)
		goto out_free;
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	att = attestation_instance_lock(target->index);
	if (IS_ERR(att))
		goto out_srcu;
	mr = xa_load(&att->mrs, target->mr_id);
	if (mr && mr->start_generation == target->start_generation &&
	    (mr->state_flags & CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE)) {
		mr->state_flags &=
			~CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE;
		if (!attestation_preflight(att, false, true))
			attestation_mr_changed(att);
	}
	attestation_instance_unlock(att);
out_srcu:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
out_free:
	kfree(target);
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_put, "ATTESTATION");

int attestation_mr_target_check(const struct attestation_mr_target *target,
				const char *family_name, const char *bus_name,
				const char *device_name, unsigned int mr_id)
{
	struct attestation_mr *mr;
	struct attestation *att;
	int srcu_index;
	int ret;

	if (!target || mr_id != target->mr_id)
		return -EINVAL;
	ret = attestation_selector_validate(family_name, bus_name, device_name);
	if (ret)
		return ret;
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	att = attestation_instance_lock(target->index);
	if (IS_ERR(att)) {
		ret = PTR_ERR(att);
		goto out_srcu;
	}
	mr = xa_load(&att->mrs, target->mr_id);
	if (!mr || mr->start_generation != target->start_generation)
		ret = -ENODEV;
	else if (!attestation_family_matches(att, family_name) ||
		 !attestation_parent_matches(att, bus_name, device_name))
		ret = -EINVAL;
	else
		ret = 0;
	attestation_instance_unlock(att);
out_srcu:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_check, "ATTESTATION");

struct attestation_mr_target *
attestation_mr_target_clone(const struct attestation_mr_target *target)
{
	struct attestation_mr_target *clone;

	if (!target)
		return ERR_PTR(-EINVAL);
	clone = kmemdup(target, sizeof(*target), GFP_KERNEL);
	if (!clone)
		return ERR_PTR(-ENOMEM);
	clone->ima_owner = false;
	return clone;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_clone, "ATTESTATION");

unsigned int
attestation_mr_target_start_generation(const struct attestation_mr_target *target)
{
	return target->start_generation;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_start_generation, "ATTESTATION");

enum hash_algo *
attestation_mr_target_algorithms(const struct attestation_mr_target *target,
				 size_t *algorithms_count)
{
	struct attestation_mr *mr;
	struct attestation *att;
	enum hash_algo *algorithms;
	int srcu_index;

	if (!target || !algorithms_count)
		return ERR_PTR(-EINVAL);
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	att = attestation_instance_lock(target->index);
	if (IS_ERR(att)) {
		algorithms = ERR_CAST(att);
		goto out;
	}
	mr = xa_load(&att->mrs, target->mr_id);
	if (!mr || mr->start_generation != target->start_generation) {
		algorithms = ERR_PTR(-ENODEV);
		goto out_unlock;
	}
	algorithms = kmemdup_array(mr->props->algorithms,
				   mr->props->algorithms_count,
				   sizeof(*algorithms), GFP_KERNEL);
	if (algorithms)
		*algorithms_count = mr->props->algorithms_count;
	else
		algorithms = ERR_PTR(-ENOMEM);
out_unlock:
	attestation_instance_unlock(att);
out:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return algorithms;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_algorithms, "ATTESTATION");

int attestation_mr_target_extend(const struct attestation_mr_target *target,
				 const struct attestation_digest_set *values,
				 const char *template_name,
				 const u8 *template_data,
				 size_t template_data_length, bool violation)
{
	struct attestation_mr_change *change;
	struct attestation_mr *mr;
	struct attestation *att;
	const void *entry;
	int srcu_index;
	int ret;

	if (!target || !target->ima_owner)
		return -EINVAL;
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	att = attestation_instance_lock(target->index);
	if (IS_ERR(att)) {
		ret = PTR_ERR(att);
		goto out;
	}
	mr = xa_load(&att->mrs, target->mr_id);
	if (!mr || mr->start_generation != target->start_generation) {
		ret = -ENODEV;
		goto out_unlock;
	}
	ret = attestation_mr_values_validate(mr, values);
	if (ret)
		goto out_unlock;
	change = attestation_mr_ima_change_prepare(
		att, mr, values, template_name, template_data,
		template_data_length, violation);
	if (IS_ERR(change)) {
		ret = PTR_ERR(change);
		goto out_unlock;
	}
	entry = attestation_mr_extend_locked(att, change);
	ret = IS_ERR(entry) ? PTR_ERR(entry) : 0;
	attestation_mr_change_free(change);
out_unlock:
	attestation_instance_unlock(att);
out:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_target_extend, "ATTESTATION");

int attestation_mr_log_end_generation(const struct attestation_mr_target *target,
				      unsigned int *end_generation)
{
	const struct attestation_mr_log_entry *entry;
	struct attestation *att;
	unsigned long index;
	int srcu_index;
	int ret = 0;

	if (!target || !end_generation)
		return -EINVAL;
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	att = attestation_instance_lock(target->index);
	if (IS_ERR(att)) {
		ret = PTR_ERR(att);
		goto out;
	}
	*end_generation = att->log->generation;
	if (!xa_load(&att->log->entries, target->start_generation)) {
		ret = -ESTALE;
		goto out_unlock;
	}
	index = target->start_generation;
	xa_for_each_start(&att->log->entries, index, entry,
			  target->start_generation + 1) {
		if (entry->type == ATTESTATION_MR_LOG_ENTRY_INFO &&
		    entry->mr_id == target->mr_id) {
			*end_generation = entry->mr_generation - 1;
			break;
		}
	}
out_unlock:
	attestation_instance_unlock(att);
out:
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_end_generation, "ATTESTATION");

const struct attestation_mr_log_entry *
attestation_mr_log_iter_seek(const struct attestation_mr_target *target,
			     unsigned int generation,
			     unsigned int end_generation)
{
	const struct attestation_mr_log_entry *entry;
	struct attestation *att;
	unsigned long index;

	xa_lock(&attestation_instances);
	att = xa_load(&attestation_instances, target->index);
	if (!att || !xa_get_mark(&attestation_instances, target->index,
				 ATTESTATION_REGISTERED)) {
		xa_unlock(&attestation_instances);
		return ERR_PTR(-ENODEV);
	}
	xa_unlock(&attestation_instances);
	entry = xa_load(&att->log->entries, target->start_generation);
	if (!entry || entry->type != ATTESTATION_MR_LOG_ENTRY_INFO ||
	    entry->mr_id != target->mr_id)
		return ERR_PTR(-ESTALE);
	if (generation >= end_generation)
		return NULL;
	if (generation < target->start_generation) {
		index = target->start_generation;
		return xa_find(&att->log->entries, &index, end_generation,
			       XA_PRESENT);
	}
	index = generation;
	return xa_find_after(&att->log->entries, &index, end_generation,
			     XA_PRESENT);
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_iter_seek, "ATTESTATION");

const struct attestation_mr_log_entry *
attestation_mr_log_iter_next(const struct attestation_mr_log_entry *entry,
			     unsigned int end_generation)
{
	const struct attestation_mr_log_entry *next;

	if (entry->mr_generation >= end_generation)
		return NULL;
	next = srcu_dereference(entry->next, &attestation_instances_srcu);
	if (!next || next->mr_generation > end_generation)
		return NULL;
	return next;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_iter_next, "ATTESTATION");

bool attestation_mr_log_entry_selected(const struct attestation_mr_target *target,
				       const struct attestation_mr_log_entry *entry)
{
	return target->mr_id == entry->mr_id &&
	       entry->mr_generation >= target->start_generation;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_selected, "ATTESTATION");

enum _ctlv_attestation_mr_log_event
attestation_mr_log_entry_event(const struct attestation_mr_log_entry *entry)
{
	return entry->type == ATTESTATION_MR_LOG_ENTRY_INFO ?
		CTLV_ATTESTATION_MR_LOG_EVENT_INFO :
		CTLV_ATTESTATION_MR_LOG_EVENT_EXTEND;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_event, "ATTESTATION");

unsigned int
attestation_mr_log_entry_generation(const struct attestation_mr_log_entry *entry)
{
	return entry->mr_generation;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_generation, "ATTESTATION");

const struct attestation_digest *
attestation_mr_log_entry_values(const struct attestation_mr_log_entry *entry,
				size_t *values_count)
{
	const struct attestation_mr_log_entry_extend *extend;

	if (entry->type == ATTESTATION_MR_LOG_ENTRY_INFO) {
		*values_count = 0;
		return NULL;
	}
	extend = container_of(entry, typeof(*extend), entry);
	*values_count = extend->values.values_count;
	return extend->values.values;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_values, "ATTESTATION");

const char *
attestation_mr_log_entry_template_name(const struct attestation_mr_log_entry *entry)
{
	const struct attestation_mr_log_entry_ima_extend *ima;

	if (entry->type != ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND)
		return NULL;
	ima = container_of(entry, typeof(*ima), extend.entry);
	return ima->template_name;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_template_name, "ATTESTATION");

const u8 *
attestation_mr_log_entry_template_data(const struct attestation_mr_log_entry *entry,
				       size_t *length)
{
	const struct attestation_mr_log_entry_ima_extend *ima;

	if (entry->type != ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND) {
		*length = 0;
		return NULL;
	}
	ima = container_of(entry, typeof(*ima), extend.entry);
	*length = ima->template_data_length;
	return ima->template_data;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_template_data, "ATTESTATION");

bool
attestation_mr_log_entry_violation(const struct attestation_mr_log_entry *entry)
{
	const struct attestation_mr_log_entry_ima_extend *ima;

	if (entry->type != ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND)
		return false;
	ima = container_of(entry, typeof(*ima), extend.entry);
	return ima->violation;
}
EXPORT_SYMBOL_NS_GPL(attestation_mr_log_entry_violation, "ATTESTATION");
