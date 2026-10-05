// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "internal.h"

static bool attestation_challenge_length_put(struct ctlv_cursor *object, size_t min, size_t max)
{
	struct ctlv_cursor entry;
	bool ok;

	if (!ctlv_attestation_device_get_objects_challenge_lengths_start(object, &entry))
		return false;
	if (min == max) {
		ok = ctlv_attestation_device_get_challenge_lengths_length_put(&entry, min);
	} else {
		ok = ctlv_attestation_device_get_challenge_lengths_min_length_put(&entry, min);
		ok &= ctlv_attestation_device_get_challenge_lengths_max_length_put(&entry, max);
	}
	ctlv_record_end(&entry);
	return ok;
}

static bool attestation_evidence_object_put(struct ctlv_cursor *object, size_t id,
					    const struct attestation_evidence_supported_object *cap)
{
	size_t index;
	bool ok;

	ok = ctlv_attestation_device_get_objects_id_put(object, id);
	ok &= ctlv_attestation_device_get_objects_class_put(object, cap->evidence_class);
	for (index = 0; index < cap->formats_count; index++) {
		u32 value = cap->formats[index].value;

		if (cap->formats[index].vendor)
			ok &= ctlv_attestation_device_get_objects_vendor_formats_put(object, value);
		else
			ok &= ctlv_attestation_device_get_objects_formats_put(object, value);
	}
	ok &= ctlv_attestation_device_get_objects_max_length_put(object, cap->max_length);
	for (index = 0; index < cap->challenge_lengths_count; index++)
		ok &= attestation_challenge_length_put(object, cap->challenge_lengths[index].min,
						       cap->challenge_lengths[index].max);
	return ok;
}

static int attestation_device_caps_put(struct attestation_dev *att, struct ctlv_cursor *reply)
{
	const struct attestation_provider *provider = att->provider;
	struct ctlv_cursor caps_record, group, object;
	bool ok = true;
	size_t index;
	u64 flags;

	if (!ctlv_attestation_device_get_caps_start(reply, &caps_record))
		return -EMSGSIZE;

	if (provider->evidence_caps) {
		const struct attestation_evidence_caps *caps = provider->evidence_caps;

		if (!ctlv_attestation_device_get_caps_evidence_start(&caps_record, &group))
			return -EMSGSIZE;

		flags = 0;
		if (provider->evidence_accept)
			flags |= CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT;
		ok &= ctlv_attestation_device_get_evidence_flags_put(&group, flags);

		for (index = 0; index < caps->objects_count; index++) {
			if (!ctlv_attestation_device_get_evidence_objects_start(&group, &object)) {
				ok = false;
				break;
			}
			ok &= attestation_evidence_object_put(&object, index,
							      &caps->objects[index]);
			ctlv_record_end(&object);
		}
		ctlv_record_end(&group);
	}

	if (provider->mrs_count) {
		if (!ctlv_attestation_device_get_caps_mr_start(&caps_record, &group))
			return -EMSGSIZE;

		flags = 0;
		if (provider->mr_extend)
			flags |= CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND;
		ok &= ctlv_attestation_device_get_mr_flags_put(&group, flags);
		ctlv_record_end(&group);
	}

	ctlv_record_end(&caps_record);
	return ok ? 0 : -EMSGSIZE;
}

int ctlv_attestation_device_get_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	struct ctlv_cursor info;
	int ret;

	ret = ctlv_attestation_device_generation_put(reply, att->device_generation) ? 0 : -EMSGSIZE;
	if (!ret)
		ret = attestation_device_caps_put(att, reply);
	if (ret)
		return ret;
	if (!ctlv_attestation_device_get_info_start(reply, &info))
		return -EMSGSIZE;
	ret = ctlv_ext_fill(ctx, request, &info);
	ctlv_record_end(&info);
	return ret;
}

static void attestation_evidence_request_free(struct attestation_evidence_object_request *objects,
					      size_t count)
{
	while (count) {
		count--;
		if (objects[count].output)
			ctlv_blob_put(objects[count].output);
	}
	kfree(objects);
}

static int
attestation_evidence_decode_objects(struct attestation_dev *att, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request,
				    struct attestation_evidence_request *decoded,
				    struct attestation_evidence_object_request **objects_out)
{
	const struct attestation_evidence_caps *caps = att->provider->evidence_caps;
	struct attestation_evidence_object_request *objects;
	struct ctlv_cursor object;
	size_t count = 0, index = 0, previous;
	int ret;

	ctlv_attestation_evidence_get_objects_for_each(&object, request) count++;
	objects = kcalloc(count, sizeof(*objects), GFP_KERNEL);
	if (!objects)
		return -ENOMEM;

	ctlv_attestation_evidence_get_objects_for_each(&object, request)
	{
		const struct attestation_evidence_supported_object *supported;
		u64 id;

		id = ctlv_attestation_evidence_get_objects_id_get(&object);
		if (id >= caps->objects_count) {
			ret = -ENOENT;
			goto err_free;
		}
		supported = &caps->objects[id];
		for (previous = 0; previous < index; previous++)
			if (objects[previous].supported_object == supported) {
				ret = -EINVAL;
				goto err_free;
			}

		objects[index].supported_object = supported;
		objects[index].output =
			ctlv_attestation_evidence_get_objects_data_resolve(ctx, &object);
		if (IS_ERR(objects[index].output)) {
			ret = PTR_ERR(objects[index].output);
			objects[index].output = NULL;
			goto err_free;
		}
		index++;
	}

	decoded->objects = objects;
	decoded->objects_count = count;
	*objects_out = objects;
	return 0;

err_free:
	attestation_evidence_request_free(objects, count);
	return ret;
}

static bool attestation_ranges_required(const struct attestation_dev *att)
{
	const struct attestation_evidence_caps *caps = att->provider->evidence_caps;
	size_t index;

	for (index = 0; index < caps->objects_count; index++)
		if (caps->objects[index].binds_mrs)
			return true;
	return false;
}

static struct attestation_evidence_range *
attestation_evidence_range_tail(struct attestation_dev *att, bool mr_dependent)
{
	struct attestation_evidence_range *range;
	unsigned long index;

	xa_for_each(&att->evidence_ranges, index, range) {
		if (range->end_generation != att->evidence_generation ||
		    range->mr_dependent != mr_dependent)
			continue;
		if (mr_dependent && range->mr_generation != att->log->generation)
			continue;
		return range;
	}
	return NULL;
}

static bool
attestation_challenge_length_supported(const struct attestation_evidence_supported_object *object,
				       size_t length)
{
	size_t index;

	for (index = 0; index < object->challenge_lengths_count; index++) {
		if (length >= object->challenge_lengths[index].min &&
		    length <= object->challenge_lengths[index].max)
			return true;
	}
	return false;
}

/* A challenge goes with signed objects only, and has to suit each. */
static int attestation_evidence_challenge_check(const struct attestation_evidence_request *decoded)
{
	bool signed_named = false;
	size_t index;

	for (index = 0; index < decoded->objects_count; index++) {
		const struct attestation_evidence_supported_object *object =
			decoded->objects[index].supported_object;

		if (object->evidence_class != ATTESTATION_EVIDENCE_SIGNED)
			continue;
		signed_named = true;
		if (!decoded->challenge ||
		    !attestation_challenge_length_supported(object, decoded->challenge_length))
			return -EINVAL;
	}
	return !signed_named && decoded->challenge ? -EINVAL : 0;
}

static int attestation_evidence_request_decode(struct attestation_dev *att, struct ctlv_op_ctx *ctx,
					       const struct ctlv_cursor *request,
					       struct attestation_evidence_request *decoded,
					       struct attestation_evidence_object_request **objects)
{
	u32 challenge_length;
	int ret;

	if (ctlv_attestation_evidence_get_challenge_present(request)) {
		decoded->challenge =
			ctlv_attestation_evidence_get_challenge_get(request, &challenge_length);
		decoded->challenge_length = challenge_length;
	}

	ret = attestation_evidence_decode_objects(att, ctx, request, decoded, objects);
	if (ret)
		return ret;
	return attestation_evidence_challenge_check(decoded);
}

static bool attestation_format_in(const struct attestation_evidence_supported_object *object,
				  const struct attestation_evidence_format *format)
{
	size_t index;

	for (index = 0; index < object->formats_count; index++)
		if (object->formats[index].vendor == format->vendor &&
		    object->formats[index].value == format->value)
			return true;
	return false;
}

static bool
attestation_evidence_result_put(struct ctlv_cursor *reply,
				const struct attestation_evidence_supported_object *object,
				const struct attestation_evidence_object_result *produced)
{
	u32 value = produced->format.value;
	struct ctlv_cursor entry;
	bool ok;

	if (!ctlv_attestation_evidence_get_results_start(reply, &entry))
		return false;
	if (produced->format.vendor)
		ok = ctlv_attestation_evidence_get_results_vendor_format_put(&entry, value);
	else
		ok = ctlv_attestation_evidence_get_results_format_put(&entry, value);
	ok &= ctlv_attestation_evidence_get_results_length_put(&entry, produced->length);
	ctlv_record_end(&entry);
	return ok;
}

int ctlv_attestation_evidence_get_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *reply)
{
	struct attestation_evidence_object_request *objects = NULL;
	struct attestation_evidence_request decoded = {};
	struct attestation_evidence_result result = {};
	struct attestation_evidence_range *range = NULL;
	struct attestation_evidence_range *new_range = NULL;
	struct attestation_event_batch events = {};
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	bool mr_dependent = false;
	size_t index;
	int put_ret;
	int ret;

	if (ctlv_attestation_device_generation_present(request) &&
	    ctlv_attestation_device_generation_get(request) != att->device_generation)
		return -ESTALE;

	ret = attestation_evidence_request_decode(att, ctx, request, &decoded, &objects);
	if (ret)
		goto out;

	result.objects = kcalloc(decoded.objects_count, sizeof(*result.objects), GFP_KERNEL);
	if (!result.objects) {
		ret = -ENOMEM;
		goto out;
	}

	for (index = 0; index < decoded.objects_count; index++)
		mr_dependent |= decoded.objects[index].supported_object->binds_mrs;
	if (att->provider->evidence_accept) {
		ret = attestation_preflight(att, false, true, false);
		if (ret)
			goto out;

		if (attestation_ranges_required(att)) {
			range = attestation_evidence_range_tail(att, mr_dependent);
			if (!range) {
				range = kzalloc_obj(*range, GFP_KERNEL);
				if (!range) {
					ret = -ENOMEM;
					goto out;
				}
				range->start_generation = att->evidence_generation + 1;
				range->end_generation = range->start_generation;
				range->mr_dependent = mr_dependent;
				range->mr_generation = att->log ? att->log->generation : 0;
				new_range = range;
				ret = xa_reserve(&att->evidence_ranges, range->start_generation,
						 GFP_KERNEL);
				if (ret)
					goto out;
			}
		}

		ret = attestation_events_prepare(att, &events, false, true, false);
		if (ret)
			goto out;
	}

	ret = att->provider->evidence_get(att, &decoded, &result);
	if (ret)
		goto out;

	for (index = 0; index < decoded.objects_count; index++) {
		const struct attestation_evidence_object_result *produced = &result.objects[index];

		if (produced->length > decoded.objects[index].supported_object->max_length ||
		    produced->length > ctlv_blob_len(decoded.objects[index].output)) {
			ret = -E2BIG;
			goto out;
		}
		if (!attestation_format_in(decoded.objects[index].supported_object,
					   &produced->format)) {
			ret = -EPROTO;
			goto out;
		}
	}

	for (index = 0; index < decoded.objects_count; index++) {
		put_ret = ctlv_blob_put(decoded.objects[index].output);
		objects[index].output = NULL;
		if (!ret)
			ret = put_ret;
	}
	if (ret)
		goto out;

	if (!ctlv_attestation_device_generation_put(reply, att->device_generation)) {
		ret = -EMSGSIZE;
		goto out;
	}
	if (att->provider->evidence_accept) {
		if (!ctlv_attestation_evidence_get_evidence_generation_put(
			    reply, att->evidence_generation + 1)) {
			ret = -EMSGSIZE;
			goto out;
		}
	}
	if (mr_dependent && !ctlv_attestation_mr_generation_put(reply, att->log->generation)) {
		ret = -EMSGSIZE;
		goto out;
	}
	for (index = 0; index < decoded.objects_count; index++)
		if (!attestation_evidence_result_put(reply, decoded.objects[index].supported_object,
						     &result.objects[index])) {
			ret = -EMSGSIZE;
			goto out;
		}

	if (att->provider->evidence_accept) {
		att->evidence_generation++;
		if (range)
			range->end_generation = att->evidence_generation;
		if (new_range) {
			xa_store(&att->evidence_ranges, new_range->start_generation, new_range,
				 GFP_NOWAIT);
			new_range = NULL;
		}
		attestation_evidence_event_emit(&events, att);
	}

out:
	if (new_range) {
		xa_release(&att->evidence_ranges, new_range->start_generation);
		kfree(new_range);
	}
	attestation_events_abort(&events);
	kfree(result.objects);
	attestation_evidence_request_free(objects, decoded.objects_count);
	return ret;
}

static int attestation_evidence_validate(struct attestation_dev *att,
					 unsigned int device_generation,
					 unsigned int evidence_generation)
{
	struct attestation_evidence_range *range;
	unsigned long index;

	if (device_generation != att->device_generation ||
	    evidence_generation <= att->evidence_floor ||
	    evidence_generation > att->evidence_generation)
		return -ESTALE;
	if (!attestation_ranges_required(att))
		return 0;
	xa_for_each(&att->evidence_ranges, index, range) {
		if (evidence_generation < range->start_generation)
			break;
		if (evidence_generation > range->end_generation)
			continue;
		if (range->mr_dependent &&
		    (!att->log || range->mr_generation != att->log->generation))
			return -ESTALE;
		return 0;
	}
	return -ESTALE;
}

int ctlv_attestation_evidence_accept_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply)
{
	struct attestation_event_batch events = {};
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	u64 device, evidence;
	int ret;

	device = ctlv_attestation_device_generation_get(request);
	evidence = ctlv_attestation_evidence_accept_evidence_generation_get(request);
	ret = attestation_evidence_validate(att, device, evidence);
	if (ret)
		return ret;

	ret = attestation_preflight(att, true, false, false);
	if (ret)
		return ret;
	ret = attestation_events_prepare(att, &events, true, true, false);
	if (ret)
		return ret;

	/* Before the callback, so nothing after a device runs can fail. */
	if (!ctlv_attestation_device_generation_put(reply, att->device_generation + 1)) {
		ret = -EMSGSIZE;
		goto out;
	}

	ret = att->provider->evidence_accept(att);
	if (ret)
		goto out;
	attestation_device_changed(att);
	attestation_device_event_emit(&events, att->device_generation);
	attestation_evidence_event_emit(&events, att);

out:
	attestation_events_abort(&events);
	return ret;
}

int ctlv_attestation_mr_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	struct attestation_mr *mr;
	struct ctlv_cursor inner;
	unsigned long id, first = 0, last = UINT_MAX;
	u64 cursor = ctlv_op_query_cursor(ctx);
	unsigned int generation = att->log->generation;
	bool progress = false;
	int ret;

	if (cursor && ctlv_op_query_generation(ctx) != generation)
		return -ESTALE;
	if (ctlv_attestation_mr_get_id_present(request)) {
		first = ctlv_attestation_mr_get_id_get(request);
		mr = attestation_mr_lookup_live(att, first);
		if (IS_ERR(mr))
			return PTR_ERR(mr);
		last = first;
	} else if (cursor) {
		first = cursor - 1;
	}

	xa_for_each_range(&att->mrs, id, mr, first, last) {
		if (!ctlv_attestation_mr_get_items_start(reply, &inner))
			break;
		ret = attestation_mr_props_encode(&inner, mr);
		if (ret == -EMSGSIZE) {
			ctlv_record_cancel(&inner);
			break;
		}
		ctlv_record_end(&inner);
		if (ret)
			return ret;
		progress = true;
	}

	if (!mr)
		return ctlv_op_query_done(ctx, generation);
	if (!progress)
		return -EMSGSIZE;
	return ctlv_op_query_more(ctx, generation, id + 1);
}

static int attestation_mr_values_put(struct ctlv_cursor *reply,
				     const struct attestation_digest_set *values)
{
	const struct attestation_digest *value;
	struct ctlv_cursor inner;
	size_t index;
	bool ok;

	for (index = 0; index < values->values_count; index++) {
		value = &values->values[index];
		if (!ctlv_attestation_mr_values_start(reply, &inner))
			return -EMSGSIZE;
		ok = ctlv_attestation_mr_value_hash_algo_put(&inner, value->algorithm);
		ok &= ctlv_attestation_mr_value_data_put(&inner, value->data,
							 hash_digest_size[value->algorithm]);
		ctlv_record_end(&inner);
		if (!ok)
			return -EMSGSIZE;
	}
	return 0;
}

int ctlv_attestation_mr_read_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_digest_set values = {};
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	struct attestation_mr *mr;
	u64 id;
	int ret;

	if (ctlv_op_query_cursor(ctx) &&
	    ctlv_op_query_generation(ctx) != att->mr_read_generation)
		return -ESTALE;
	if (ctlv_attestation_device_generation_present(request) &&
	    ctlv_attestation_device_generation_get(request) != att->device_generation)
		return -ESTALE;

	id = ctlv_attestation_mr_read_id_get(request);
	mr = attestation_mr_lookup_live(att, id);
	if (IS_ERR(mr))
		return PTR_ERR(mr);

	values.values_count = mr->props->algorithms_count;
	values.values = kcalloc(values.values_count, sizeof(*values.values), GFP_KERNEL);
	if (!values.values)
		return -ENOMEM;
	for (id = 0; id < values.values_count; id++)
		values.values[id].algorithm = mr->props->algorithms[id];
	ret = att->provider->mr_read(att, mr->props->id, &values);
	if (!ret)
		ret = attestation_mr_values_validate(mr->props, &values, true);
	if (!ret && (!ctlv_attestation_device_generation_put(reply, att->device_generation) ||
		     !ctlv_attestation_mr_generation_put(reply, att->log->generation)))
		ret = -EMSGSIZE;
	if (!ret)
		ret = attestation_mr_values_put(reply, &values);
	kfree(values.values);
	if (ret)
		return ret;
	return ctlv_op_query_done(ctx, att->mr_read_generation);
}

static int attestation_mr_extend_decode(const struct ctlv_cursor *request,
					const struct attestation_mr *mr,
					struct attestation_digest_set *values)
{
	size_t banks = mr->props->algorithms_count, bank;
	DECLARE_BITMAP(seen, HASH_ALGO__LAST) = {};
	struct ctlv_cursor item;

	values->values = kcalloc(banks, sizeof(*values->values), GFP_KERNEL);
	if (!values->values)
		return -ENOMEM;
	ctlv_attestation_mr_values_for_each(&item, request)
	{
		u64 algorithm = ctlv_attestation_mr_value_hash_algo_get(&item);
		struct attestation_digest *value;
		const void *data;
		u32 length;

		for (bank = 0; bank < banks; bank++)
			if (mr->props->algorithms[bank] == algorithm)
				break;
		data = ctlv_attestation_mr_value_data_get(&item, &length);
		if (bank == banks || __test_and_set_bit(bank, seen) ||
		    length != hash_digest_size[algorithm])
			return -EINVAL;
		value = &values->values[bank];
		value->algorithm = algorithm;
		memcpy(value->data, data, length);
	}
	/* Close up the banks left out. */
	for_each_set_bit(bank, seen, banks)
		values->values[values->values_count++] = values->values[bank];
	return 0;
}

int ctlv_attestation_mr_extend_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_digest_set values = {};
	struct attestation_mr_change *change;
	const struct attestation_mr_log_entry *entry;
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	struct attestation_mr *mr;
	u64 id;
	int ret;

	if (ctlv_attestation_device_generation_present(request) &&
	    ctlv_attestation_device_generation_get(request) != att->device_generation)
		return -ESTALE;
	id = ctlv_attestation_mr_extend_id_get(request);
	mr = attestation_mr_lookup_live(att, id);
	if (IS_ERR(mr))
		return PTR_ERR(mr);

	if (!mr->props->extendable)
		return -EOPNOTSUPP;
	ret = attestation_mr_extend_decode(request, mr, &values);
	if (ret)
		goto out_values;
	ret = attestation_mr_values_validate(mr->props, &values, false);
	if (ret)
		goto out_values;
	change = attestation_mr_change_prepare(att, mr, &values);
	if (IS_ERR(change)) {
		ret = PTR_ERR(change);
		goto out_values;
	}

	entry = attestation_mr_extend_locked(att, change);
	if (IS_ERR(entry))
		ret = PTR_ERR(entry);
	else if (!ctlv_attestation_device_generation_put(reply, att->device_generation) ||
		 !ctlv_attestation_mr_generation_put(reply, entry->mr_generation))
		ret = -EMSGSIZE;
	attestation_mr_change_free(change);

out_values:
	kfree(values.values);
	return ret;
}

static bool attestation_log_selected(const unsigned int *ids, size_t count, unsigned int id)
{
	size_t index;

	if (!count)
		return true;
	for (index = 0; index < count; index++)
		if (ids[index] == id)
			return true;
	return false;
}

int ctlv_attestation_mr_log_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	struct attestation_mr_log_entry *entry;
	struct ctlv_cursor item, inner;
	unsigned int ids[256];
	size_t ids_count = 0, index;
	unsigned int start, end;
	unsigned long xa_index;
	u64 boundary;
	u64 cursor = ctlv_op_query_cursor(ctx);
	bool progress = false;
	int ret;

	ctlv_attestation_mr_log_get_selection_for_each(&item, request)
	{
		u64 id = ctlv_attestation_mr_log_get_selection_id_get(&item);

		for (index = 0; index < ids_count; index++)
			if (ids[index] == id)
				return -EINVAL;
		if (WARN_ON_ONCE(ids_count == ARRAY_SIZE(ids)))
			return -EINVAL;
		ids[ids_count++] = id;
	}

	if (cursor)
		boundary = ctlv_op_query_generation(ctx);
	else
		boundary = att->log->generation;
	if (boundary > UINT_MAX || boundary > att->log->generation)
		return -ESTALE;

	end = boundary;
	if (ctlv_attestation_mr_end_generation_present(request)) {
		u64 requested_end = ctlv_attestation_mr_end_generation_get(request);

		end = min_t(u64, end, requested_end);
	}

	start = 1;
	if (ctlv_attestation_mr_start_generation_present(request))
		start = ctlv_attestation_mr_start_generation_get(request);
	if (att->log->generation && start > end)
		return -EINVAL;

	if (!cursor && att->log->generation) {
		if (!ctlv_attestation_mr_start_generation_put(reply, start) ||
		    !ctlv_attestation_mr_end_generation_put(reply, end))
			return -EMSGSIZE;
		progress = true;
	}

	xa_for_each_range(&att->log->entries, xa_index, entry, cursor ? cursor : start, end) {
		if (!attestation_log_selected(ids, ids_count, entry->mr_id))
			continue;
		if (!ctlv_attestation_mr_log_get_entries_start(reply, &inner))
			break;
		ret = attestation_mr_log_entry_encode(&inner, entry);
		if (ret == -EMSGSIZE) {
			ctlv_record_cancel(&inner);
			break;
		}
		ctlv_record_end(&inner);
		if (ret)
			return ret;
		progress = true;
	}

	if (!entry)
		return ctlv_op_query_done(ctx, boundary);
	if (!progress)
		return -EMSGSIZE;
	return ctlv_op_query_more(ctx, boundary, entry->mr_generation);
}

int ctlv_attestation_devsec_state_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply)
{
	struct attestation_dev *att = attestation_from_ctlv(ctlv);
	enum attestation_tdisp_state state;
	u64 version = att->devsec_version;
	int ret;
	bool ok;

	if (ctlv_op_query_cursor(ctx) && ctlv_op_query_generation(ctx) != version)
		return -ESTALE;
	ok = ctlv_attestation_device_generation_put(reply, att->device_generation);
	if (att->provider->tdisp_state_get) {
		ret = att->provider->tdisp_state_get(att, &state);
		if (ret)
			return ret;
		ok &= ctlv_attestation_devsec_state_get_tdisp_state_put(reply, state);
	}
	if (!ok)
		return -EMSGSIZE;
	return ctlv_op_query_done(ctx, version);
}
