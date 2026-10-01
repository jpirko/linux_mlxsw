// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <kunit/static_stub.h>
#include <kunit/visibility.h>
#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "internal.h"

int attestation_mr_values_validate(const struct attestation_mr *mr,
				   const struct attestation_digest_set *values)
{
	size_t index;

	if (!values || !values->values || values->values_count != mr->props->algorithms_count)
		return -EINVAL;
	for (index = 0; index < values->values_count; index++)
		if (values->values[index].algorithm != mr->props->algorithms[index])
			return -EINVAL;
	return 0;
}

static int attestation_digest_set_copy(struct attestation_digest_set *dst,
				       const struct attestation_digest_set *src)
{
	size_t index;

	if (!src || !src->values || !src->values_count)
		return -EINVAL;
	for (index = 0; index < src->values_count; index++) {
		enum hash_algo algorithm = src->values[index].algorithm;

		if (algorithm >= HASH_ALGO__LAST || !hash_digest_size[algorithm] ||
		    (index && src->values[index - 1].algorithm >= algorithm))
			return -EINVAL;
	}
	dst->values =
		kmemdup_array(src->values, src->values_count, sizeof(*src->values), GFP_KERNEL);
	if (!dst->values)
		return -ENOMEM;
	dst->values_count = src->values_count;
	return 0;
}

static void attestation_digest_set_free(struct attestation_digest_set *set)
{
	kfree(set->values);
	set->values = NULL;
	set->values_count = 0;
}

static bool attestation_identifier_valid(const char *value, size_t max_length)
{
	const unsigned char *at = (const unsigned char *)value;
	size_t length = 0;

	if (!value || !*value)
		return false;
	for (; *at; at++, length++)
		if (*at < 0x21 || *at > 0x7e || length == max_length)
			return false;
	return true;
}

struct attestation_mr_log_entry_info *attestation_mr_info_alloc(const struct attestation_mr *mr)
{
	struct attestation_mr_log_entry_info *info;
	enum hash_algo *algorithms;
	char *name;

	KUNIT_STATIC_STUB_REDIRECT(attestation_mr_info_alloc, mr);

	info = kzalloc_obj(*info, GFP_KERNEL);
	if (!info)
		return ERR_PTR(-ENOMEM);
	name = kstrdup(mr->props->name, GFP_KERNEL);
	if (!name)
		goto err_info;
	algorithms = kmemdup_array(mr->props->algorithms, mr->props->algorithms_count,
				   sizeof(*algorithms), GFP_KERNEL);
	if (!algorithms)
		goto err_name;
	info->entry.type = ATTESTATION_MR_LOG_ENTRY_INFO;
	info->entry.mr_id = mr->props->id;
	info->props = *mr->props;
	info->props.name = name;
	info->props.algorithms = algorithms;
	return info;

err_name:
	kfree(name);
err_info:
	kfree(info);
	return ERR_PTR(-ENOMEM);
}
EXPORT_SYMBOL_IF_KUNIT(attestation_mr_info_alloc);

void attestation_mr_log_entry_free(struct attestation_mr_log_entry *entry)
{
	if (!entry)
		return;
	switch (entry->type) {
	case ATTESTATION_MR_LOG_ENTRY_INFO: {
		struct attestation_mr_log_entry_info *info =
			container_of(entry, typeof(*info), entry);

		kfree(info->props.algorithms);
		kfree(info->props.name);
		kfree(info);
		break;
	}
	case ATTESTATION_MR_LOG_ENTRY_EXTEND: {
		struct attestation_mr_log_entry_extend *extend =
			container_of(entry, typeof(*extend), entry);

		attestation_digest_set_free(&extend->values);
		kfree(extend);
		break;
	}
	case ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND: {
		struct attestation_mr_log_entry_record_extend *record =
			container_of(entry, typeof(*record), extend.entry);

		attestation_digest_set_free(&record->extend.values);
		kfree(record->record_data);
		kfree(record->record_format);
		kfree(record);
		break;
	}
	case ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND: {
		struct attestation_mr_log_entry_ima_extend *ima =
			container_of(entry, typeof(*ima), extend.entry);

		attestation_digest_set_free(&ima->extend.values);
		kfree(ima->template_data);
		kfree(ima->template_name);
		kfree(ima);
		break;
	}
	}
}

void attestation_mr_log_free(struct attestation_mr_log *log)
{
	struct attestation_mr_log_entry *entry;
	unsigned long index;

	if (!log)
		return;
	xa_for_each(&log->entries, index, entry) {
		xa_erase(&log->entries, index);
		attestation_mr_log_entry_free(entry);
	}
	xa_destroy(&log->entries);
	kfree(log);
}

void attestation_mr_log_append(struct attestation *att, struct attestation_mr_log_entry *entry)
{
	struct attestation_mr_log *log = att->log;

	RCU_INIT_POINTER(entry->next, NULL);
	xa_store(&log->entries, entry->mr_generation, entry, GFP_NOWAIT);
	if (log->tail)
		rcu_assign_pointer(log->tail->next, entry);
	log->tail = entry;
}

int attestation_mr_log_init(struct attestation *att)
{
	struct attestation_mr_log_entry_info **infos;
	struct attestation_mr_log *log;
	struct attestation_mr *mr;
	unsigned long mr_id;
	size_t count = 0;
	size_t index = 0;
	int ret = -ENOMEM;

	xa_for_each(&att->mrs, mr_id, mr)
		count++;
	log = kzalloc_obj(*log, GFP_KERNEL);
	if (!log)
		return -ENOMEM;
	xa_init(&log->entries);
	infos = kcalloc(count, sizeof(*infos), GFP_KERNEL);
	if (count && !infos)
		goto err_log;
	xa_for_each(&att->mrs, mr_id, mr) {
		infos[index] = attestation_mr_info_alloc(mr);
		if (IS_ERR(infos[index])) {
			ret = PTR_ERR(infos[index]);
			infos[index] = NULL;
			goto err_infos;
		}
		index++;
	}

	att->log = log;
	index = 0;
	xa_for_each(&att->mrs, mr_id, mr) {
		struct attestation_mr_log_entry *entry = &infos[index]->entry;

		if (log->generation == UINT_MAX) {
			ret = -EOVERFLOW;
			goto err_attached;
		}
		entry->mr_generation = ++log->generation;
		entry->device_generation = att->device_generation;
		ret = xa_insert(&log->entries, entry->mr_generation, entry, GFP_KERNEL);
		if (ret)
			goto err_attached;
		if (log->tail)
			rcu_assign_pointer(log->tail->next, entry);
		log->tail = entry;
		mr->start_generation = entry->mr_generation;
		infos[index++] = NULL;
	}
	kfree(infos);
	return 0;

err_attached:
	att->log = NULL;
	attestation_mr_log_free(log);
	log = NULL;
err_infos:
	for (index = 0; index < count; index++)
		if (infos[index])
			attestation_mr_log_entry_free(&infos[index]->entry);
	kfree(infos);
err_log:
	attestation_mr_log_free(log);
	return ret;
}

static struct attestation_mr_log_entry_extend *
attestation_extend_alloc(enum attestation_mr_log_entry_type type, size_t size,
			 struct attestation_mr *mr, const struct attestation_digest_set *values)
{
	struct attestation_mr_log_entry_extend *extend;
	int ret;

	extend = kzalloc(size, GFP_KERNEL);
	if (!extend)
		return ERR_PTR(-ENOMEM);
	extend->entry.type = type;
	extend->entry.mr_id = mr->props->id;
	ret = attestation_digest_set_copy(&extend->values, values);
	if (ret) {
		kfree(extend);
		return ERR_PTR(ret);
	}
	return extend;
}

struct attestation_mr_change *
attestation_mr_change_prepare(struct attestation *att, struct attestation_mr *mr,
			      const struct attestation_digest_set *values,
			      const char *record_format, const u8 *record_data,
			      size_t record_data_length)
{
	struct attestation_mr_change *change;
	int ret;

	lockdep_assert_held(&att->lock);
	change = kzalloc_obj(*change, GFP_KERNEL);
	if (!change)
		return ERR_PTR(-ENOMEM);
	change->mr = mr;
	if (!record_format) {
		if (record_data || record_data_length) {
			ret = -EINVAL;
			goto err_change;
		}
		change->entry = attestation_extend_alloc(ATTESTATION_MR_LOG_ENTRY_EXTEND,
							 sizeof(*change->entry), mr, values);
	} else {
		struct attestation_mr_log_entry_extend *extend;
		struct attestation_mr_log_entry_record_extend *record;

		if (!attestation_identifier_valid(record_format, 128) ||
		    record_data_length > 4096) {
			ret = -EINVAL;
			goto err_change;
		}
		extend = attestation_extend_alloc(ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND,
						  sizeof(*record), mr, values);
		if (IS_ERR(extend)) {
			ret = PTR_ERR(extend);
			goto err_change;
		}
		record = container_of(extend, typeof(*record), extend);
		record->record_format = kstrdup(record_format, GFP_KERNEL);
		record->record_data = kmemdup(record_data, record_data_length, GFP_KERNEL);
		record->record_data_length = record_data_length;
		if (!record->record_format || (record_data_length && !record->record_data)) {
			attestation_mr_log_entry_free(&record->extend.entry);
			ret = -ENOMEM;
			goto err_change;
		}
		change->entry = &record->extend;
	}
	if (IS_ERR(change->entry)) {
		ret = PTR_ERR(change->entry);
		goto err_change;
	}
	ret = attestation_events_prepare(att, &change->events, false, false, true);
	if (ret)
		goto err_entry;
	return change;

err_entry:
	attestation_mr_log_entry_free(&change->entry->entry);
err_change:
	kfree(change);
	return ERR_PTR(ret);
}

struct attestation_mr_change *
attestation_mr_ima_change_prepare(struct attestation *att, struct attestation_mr *mr,
				  const struct attestation_digest_set *values,
				  const char *template_name, const u8 *template_data,
				  size_t template_data_length, bool violation)
{
	struct attestation_mr_log_entry_extend *extend;
	struct attestation_mr_log_entry_ima_extend *ima;
	struct attestation_mr_change *change;
	int ret;

	lockdep_assert_held(&att->lock);
	if (!attestation_identifier_valid(template_name, 128) || template_data_length > 65536)
		return ERR_PTR(-EINVAL);
	change = kzalloc_obj(*change, GFP_KERNEL);
	if (!change)
		return ERR_PTR(-ENOMEM);
	extend = attestation_extend_alloc(ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND, sizeof(*ima), mr,
					  values);
	if (IS_ERR(extend)) {
		ret = PTR_ERR(extend);
		goto err_change;
	}
	ima = container_of(extend, typeof(*ima), extend);
	ima->template_name = kstrdup(template_name, GFP_KERNEL);
	ima->template_data = kmemdup(template_data, template_data_length, GFP_KERNEL);
	ima->template_data_length = template_data_length;
	ima->violation = violation;
	if (!ima->template_name || (template_data_length && !ima->template_data)) {
		attestation_mr_log_entry_free(&ima->extend.entry);
		ret = -ENOMEM;
		goto err_change;
	}
	change->mr = mr;
	change->entry = &ima->extend;
	ret = attestation_events_prepare(att, &change->events, false, false, true);
	if (ret)
		goto err_entry;
	return change;

err_entry:
	attestation_mr_log_entry_free(&change->entry->entry);
err_change:
	kfree(change);
	return ERR_PTR(ret);
}

const struct attestation_mr_log_entry *
attestation_mr_extend_locked(struct attestation *att, struct attestation_mr_change *change,
			     const struct ctlv_cursor *mr_options)
{
	struct attestation_mr_log_entry_extend *extend = change->entry;
	struct attestation_mr_log_entry *entry = &extend->entry;
	int ret;

	lockdep_assert_held(&att->lock);
	if (att->log->frozen)
		return ERR_PTR(-EBUSY);
	ret = attestation_preflight(att, false, false, true);
	if (ret)
		return ERR_PTR(ret);
	entry->mr_generation = att->log->generation + 1;
	ret = xa_reserve(&att->log->entries, entry->mr_generation, GFP_KERNEL);
	if (ret)
		return ERR_PTR(ret);
	ret = att->provider->mr_extend(att, change->mr->props->id, mr_options, &extend->values);
	if (ret) {
		xa_release(&att->log->entries, entry->mr_generation);
		return ERR_PTR(ret);
	}
	attestation_mr_changed(att);
	entry->device_generation = att->device_generation;
	attestation_mr_log_append(att, entry);
	attestation_mr_log_event_emit(&change->events, entry);
	change->entry = NULL;
	return entry;
}

void attestation_mr_change_free(struct attestation_mr_change *change)
{
	if (!change)
		return;
	attestation_events_abort(&change->events);
	if (change->entry)
		attestation_mr_log_entry_free(&change->entry->entry);
	kfree(change);
}

int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr,
				bool include_id)
{
	u64 caps = 0;
	size_t index;
	bool ok = true;

	if (include_id)
		ok &= ctlv_attestation_mr_id_put(reply, mr->props->id);
	ok &= ctlv_attestation_mr_name_put(reply, mr->props->name);
	for (index = 0; index < mr->props->algorithms_count; index++)
		ok &= ctlv_attestation_mr_supported_hash_algo_put(reply,
								  mr->props->algorithms[index]);
	if (mr->props->has_register_index)
		ok &= ctlv_attestation_mr_register_index_put(reply, mr->props->register_index);
	if (mr->props->readable)
		caps |= CTLV_ATTESTATION_MR_CAPS_READ;
	if (mr->props->extendable)
		caps |= CTLV_ATTESTATION_MR_CAPS_EXTEND;
	if (mr->removable)
		caps |= CTLV_ATTESTATION_MR_CAPS_REMOVABLE;
	ok &= ctlv_attestation_mr_caps_put(reply, caps);
	ok &= ctlv_attestation_mr_state_flags_put(reply, mr->state_flags);
	return ok ? 0 : -EMSGSIZE;
}

static int attestation_values_encode(struct ctlv_cursor *reply,
				     const struct attestation_digest_set *values)
{
	struct ctlv_cursor nested;
	size_t index;
	bool ok = true;

	for (index = 0; index < values->values_count; index++) {
		const struct attestation_digest *value = &values->values[index];

		if (!ctlv_attestation_mr_value_start(reply, &nested))
			return -EMSGSIZE;
		ok &= ctlv_attestation_mr_hash_algo_put(&nested, value->algorithm);
		ok &= ctlv_attestation_mr_data_put(&nested, value->data,
						   hash_digest_size[value->algorithm]);
		ctlv_nest_end(&nested);
	}
	return ok ? 0 : -EMSGSIZE;
}

static int attestation_mr_info_encode(struct ctlv_cursor *reply,
				      const struct attestation_mr_props *props)
{
	u64 caps = 0;
	size_t index;
	bool ok;

	ok = ctlv_attestation_mr_name_put(reply, props->name);
	for (index = 0; index < props->algorithms_count; index++)
		ok &= ctlv_attestation_mr_supported_hash_algo_put(reply,
								  props->algorithms[index]);
	if (props->has_register_index)
		ok &= ctlv_attestation_mr_register_index_put(reply, props->register_index);
	if (props->readable)
		caps |= CTLV_ATTESTATION_MR_CAPS_READ;
	if (props->extendable)
		caps |= CTLV_ATTESTATION_MR_CAPS_EXTEND;
	ok &= ctlv_attestation_mr_caps_put(reply, caps);
	return ok ? 0 : -EMSGSIZE;
}

int attestation_mr_log_entry_encode(struct ctlv_cursor *reply,
				    const struct attestation_mr_log_entry *entry)
{
	struct ctlv_cursor nested;
	bool ok;
	int ret;

	ok = ctlv_attestation_mr_generation_put(reply, entry->mr_generation);
	ok &= ctlv_attestation_device_generation_put(reply, entry->device_generation);
	ok &= ctlv_attestation_mr_id_put(reply, entry->mr_id);
	ok &= ctlv_attestation_log_event_put(reply, entry->type == ATTESTATION_MR_LOG_ENTRY_INFO ?
							    CTLV_ATTESTATION_MR_LOG_EVENT_INFO :
							    CTLV_ATTESTATION_MR_LOG_EVENT_EXTEND);
	if (!ok)
		return -EMSGSIZE;

	if (entry->type == ATTESTATION_MR_LOG_ENTRY_INFO) {
		const struct attestation_mr_log_entry_info *info =
			container_of(entry, typeof(*info), entry);

		if (!ctlv_attestation_log_mr_start(reply, &nested))
			return -EMSGSIZE;
		ret = attestation_mr_info_encode(&nested, &info->props);
		ctlv_nest_end(&nested);
		return ret;
	}

	ret = attestation_values_encode(reply,
					&container_of(entry,
						      const struct attestation_mr_log_entry_extend,
						      entry)
						 ->values);
	if (ret)
		return ret;
	if (entry->type == ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND) {
		const struct attestation_mr_log_entry_record_extend *record =
			container_of(entry, typeof(*record), extend.entry);

		if (!ctlv_attestation_record_item_start(reply, &nested))
			return -EMSGSIZE;
		ok = ctlv_attestation_record_format_put(&nested, record->record_format);
		ok &= ctlv_attestation_record_data_put(&nested, record->record_data,
						       record->record_data_length);
		ctlv_nest_end(&nested);
		return ok ? 0 : -EMSGSIZE;
	}
	if (entry->type == ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND) {
		const struct attestation_mr_log_entry_ima_extend *ima =
			container_of(entry, typeof(*ima), extend.entry);

		if (!ctlv_attestation_log_ima_start(reply, &nested))
			return -EMSGSIZE;
		ok = ctlv_attestation_log_template_name_put(&nested, ima->template_name);
		ok &= ctlv_attestation_log_template_data_put(&nested, ima->template_data,
							     ima->template_data_length);
		ok &= ctlv_attestation_log_violation_put(&nested, ima->violation);
		ctlv_nest_end(&nested);
		return ok ? 0 : -EMSGSIZE;
	}
	return 0;
}
