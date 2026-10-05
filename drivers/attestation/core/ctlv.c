// SPDX-License-Identifier: GPL-2.0-only
#include <crypto/hash_info.h>
#include <linux/cleanup.h>
#include <linux/err.h>
#include <linux/limits.h>
#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/string.h>

#include "internal.h"

static const struct attestation_evidence_supported_object *
attestation_evidence_object_find(const struct attestation_evidence_caps *caps, const char *format)
{
	size_t index;

	for (index = 0; index < caps->objects_count; index++) {
		const struct attestation_evidence_supported_object *object = &caps->objects[index];

		if (!strcmp(object->format, format))
			return object;
	}
	return NULL;
}

static int attestation_device_caps_put(struct attestation *att, struct ctlv_cursor *reply)
{
	const struct attestation_provider *provider = att->provider;
	struct ctlv_cursor caps_nest, group, object;
	bool ok = true;
	size_t index;
	u64 flags;

	if (!ctlv_attestation_device_caps_start(reply, &caps_nest))
		return -EMSGSIZE;
	if (provider->evidence_caps) {
		const struct attestation_evidence_caps *caps = provider->evidence_caps;

		if (!ctlv_attestation_cap_evidence_start(&caps_nest, &group))
			return -EMSGSIZE;
		ok &= ctlv_attestation_cap_min_challenge_length_put(&group,
								    caps->min_challenge_length);
		ok &= ctlv_attestation_cap_max_challenge_length_put(&group,
								    caps->max_challenge_length);
		ok &= ctlv_attestation_cap_max_total_output_length_put(&group,
								       caps->max_total_output_length);
		flags = 0;
		if (provider->evidence_accept)
			flags |= CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT;
		ok &= ctlv_attestation_cap_evidence_flags_put(&group, flags);
		for (index = 0; index < caps->objects_count; index++) {
			const struct attestation_evidence_supported_object *cap =
				&caps->objects[index];

			if (!ctlv_attestation_cap_object_start(&group, &object)) {
				ok = false;
				break;
			}
			ok &= ctlv_attestation_cap_blob_class_put(&object,
								  CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY);
			ok &= ctlv_attestation_cap_format_put(&object, cap->format);
			ok &= ctlv_attestation_cap_representation_put(&object,
								      CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
			ok &= ctlv_attestation_cap_max_length_put(&object, cap->max_length);
			ctlv_nest_end(&object);
		}
		ctlv_nest_end(&group);
	}
	if (provider->mrs_count) {
		if (!ctlv_attestation_cap_mr_start(&caps_nest, &group))
			return -EMSGSIZE;
		flags = 0;
		if (provider->mr_read)
			flags |= CTLV_ATTESTATION_MR_DEVICE_CAPS_READ;
		if (provider->mr_extend)
			flags |= CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND;
		ok &= ctlv_attestation_cap_mr_flags_put(&group, flags);
		ctlv_nest_end(&group);
	}
	if (provider->devsec_run) {
		if (!ctlv_attestation_cap_devsec_start(&caps_nest, &group))
			return -EMSGSIZE;
		ok &= ctlv_attestation_cap_devsec_flags_put(&group,
							    CTLV_ATTESTATION_DEVSEC_CAPS_RUN);
		ctlv_nest_end(&group);
	}
	ctlv_nest_end(&caps_nest);
	return ok ? 0 : -EMSGSIZE;
}

int ctlv_attestation_device_get_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation *att = priv;
	struct ctlv_cursor info;
	int ret;

	ret = ctlv_attestation_device_generation_put(reply, att->device_generation) ? 0 : -EMSGSIZE;
	if (!ret)
		ret = attestation_device_caps_put(att, reply);
	if (ret)
		return ret;
	if (!ctlv_attestation_device_info_start(reply, &info))
		return -EMSGSIZE;
	ret = ctlv_stub_fill(ctx, request, &info);
	ctlv_nest_end(&info);
	return ret;
}

static int
attestation_evidence_decode_objects(struct attestation *att, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request,
				    struct attestation_evidence_request *decoded,
				    struct attestation_evidence_object_request **objects_out)
{
	const struct attestation_evidence_caps *caps = att->provider->evidence_caps;
	struct attestation_evidence_object_request *objects;
	struct ctlv_cursor object;
	size_t count = 0, index = 0, previous;

	ctlv_attestation_evidence_get_evidence_object_for_each(&object, request) count++;
	objects = kcalloc(count, sizeof(*objects), GFP_KERNEL);
	if (!objects)
		return -ENOMEM;
	ctlv_attestation_evidence_get_evidence_object_for_each(&object, request)
	{
		const struct attestation_evidence_supported_object *supported;
		const char *format;
		size_t total = 0;

		format = ctlv_attestation_evidence_get_object_evidence_format_get(&object);
		supported = attestation_evidence_object_find(caps, format);
		if (!supported) {
			kfree(objects);
			return -EOPNOTSUPP;
		}
		for (previous = 0; previous < index; previous++)
			if (objects[previous].supported_object == supported) {
				kfree(objects);
				return -EINVAL;
			}
		objects[index].supported_object = supported;
		objects[index].output =
			ctlv_attestation_evidence_get_object_evidence_data_resolve(ctx, &object);
		if (IS_ERR(objects[index].output)) {
			int ret = PTR_ERR(objects[index].output);

			objects[index].output = NULL;
			while (index)
				ctlv_blob_put(objects[--index].output);
			kfree(objects);
			return ret;
		}
		if (ctlv_blob_len(objects[index].output) < supported->max_length) {
			for (previous = 0; previous <= index; previous++)
				ctlv_blob_put(objects[previous].output);
			kfree(objects);
			return -EMSGSIZE;
		}
		for (previous = 0; previous <= index; previous++) {
			if (check_add_overflow(total, ctlv_blob_len(objects[previous].output),
					       &total) ||
			    total > caps->max_total_output_length) {
				for (previous = 0; previous <= index; previous++)
					ctlv_blob_put(objects[previous].output);
				kfree(objects);
				return -E2BIG;
			}
		}
		index++;
	}
	decoded->objects = objects;
	decoded->objects_count = count;
	*objects_out = objects;
	return 0;
}

static bool attestation_ranges_required(const struct attestation *att)
{
	const struct attestation_evidence_caps *caps = att->provider->evidence_caps;
	size_t index;

	for (index = 0; index < caps->objects_count; index++)
		if (caps->objects[index].binds_mrs)
			return true;
	return false;
}

static struct attestation_evidence_range *attestation_evidence_range_tail(struct attestation *att,
									  bool mr_dependent)
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

static int attestation_evidence_request_decode(struct attestation *att, struct ctlv_op_ctx *ctx,
					       const struct ctlv_cursor *request,
					       struct attestation_evidence_request *decoded,
					       struct attestation_evidence_object_request **objects)
{
	const struct attestation_evidence_caps *caps = att->provider->evidence_caps;
	u32 challenge_length;

	decoded->challenge =
		ctlv_attestation_evidence_get_evidence_challenge_get(request, &challenge_length);
	decoded->challenge_length = challenge_length;
	if (challenge_length < caps->min_challenge_length ||
	    challenge_length > caps->max_challenge_length)
		return -EINVAL;
	return attestation_evidence_decode_objects(att, ctx, request, decoded, objects);
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

int ctlv_attestation_evidence_get_action(void *priv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *reply)
{
	struct attestation_evidence_object_request *objects = NULL;
	struct attestation_evidence_request decoded = {};
	struct attestation_evidence_result result = {};
	struct attestation_evidence_range *range = NULL;
	struct attestation_evidence_range *new_range = NULL;
	struct attestation_event_batch events = {};
	struct attestation *att = priv;
	bool mr_dependent = false;
	size_t index;
	int put_ret;
	int ret;

	if (ctlv_attestation_evidence_get_device_generation_present(request) &&
	    ctlv_attestation_evidence_get_device_generation_get(request) != att->device_generation)
		return -ESTALE;
	ret = attestation_evidence_request_decode(att, ctx, request, &decoded, &objects);
	if (ret)
		goto out;
	result.object_lengths =
		kcalloc(decoded.objects_count, sizeof(*result.object_lengths), GFP_KERNEL);
	if (!result.object_lengths) {
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
		if (result.object_lengths[index] >
			    decoded.objects[index].supported_object->max_length ||
		    result.object_lengths[index] > ctlv_blob_len(decoded.objects[index].output)) {
			ret = -E2BIG;
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
		if (!ctlv_attestation_evidence_generation_put(reply,
							      att->evidence_generation + 1)) {
			ret = -EMSGSIZE;
			goto out;
		}
	}
	if (mr_dependent && !ctlv_attestation_mr_generation_put(reply, att->log->generation)) {
		ret = -EMSGSIZE;
		goto out;
	}
	for (index = 0; index < decoded.objects_count; index++)
		if (!ctlv_attestation_evidence_object_length_put(reply,
								 result.object_lengths[index])) {
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
		att->devsec_version++;
		attestation_evidence_event_emit(&events, att);
	}
out:
	if (new_range) {
		xa_release(&att->evidence_ranges, new_range->start_generation);
		kfree(new_range);
	}
	attestation_events_abort(&events);
	kfree(result.object_lengths);
	attestation_evidence_request_free(objects, decoded.objects_count);
	return ret;
}

static int attestation_evidence_validate(struct attestation *att, unsigned int device_generation,
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

int ctlv_attestation_evidence_accept_action(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply)
{
	struct attestation_event_batch events = {};
	struct attestation *att = priv;
	u64 device, evidence;
	int ret;

	device = ctlv_attestation_evidence_accept_device_generation_get(request);
	evidence = ctlv_attestation_evidence_accept_evidence_generation_get(request);
	if (device > UINT_MAX || evidence > UINT_MAX)
		return -ESTALE;
	ret = attestation_evidence_validate(att, device, evidence);
	if (ret)
		return ret;
	if (att->verdict_recorded && att->accepted_device_generation == device &&
	    att->accepted_evidence_generation == evidence)
		return 0;
	ret = attestation_events_prepare(att, &events, false, true, false);
	if (ret)
		return ret;
	ret = att->provider->evidence_accept(att);
	if (!ret) {
		att->verdict_recorded = true;
		att->accepted_device_generation = device;
		att->accepted_evidence_generation = evidence;
		attestation_evidence_verdict_changed(att);
		attestation_evidence_event_emit(&events, att);
	}
	attestation_events_abort(&events);
	return ret;
}

int ctlv_attestation_evidence_accept_clear_action(void *priv, struct ctlv_op_ctx *ctx,
						  const struct ctlv_cursor *request,
						  struct ctlv_cursor *reply)
{
	struct attestation_event_batch events = {};
	struct attestation *att = priv;
	int ret;

	if (!att->verdict_recorded)
		return 0;
	ret = attestation_events_prepare(att, &events, false, true, false);
	if (ret)
		return ret;
	attestation_evidence_accept_clear_locked(att);
	attestation_evidence_verdict_changed(att);
	attestation_evidence_event_emit(&events, att);
	attestation_events_abort(&events);
	return 0;
}

int ctlv_attestation_mr_get_action(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation *att = priv;
	struct attestation_mr *mr;
	u64 id = ctlv_attestation_mr_get_mr_id_get(request);

	if (id > UINT_MAX)
		return -ERANGE;
	mr = attestation_mr_lookup_live(att, id);
	return IS_ERR(mr) ? PTR_ERR(mr) : attestation_mr_props_encode(reply, mr, false);
}

int ctlv_attestation_mr_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation *att = priv;
	struct attestation_mr *mr;
	struct ctlv_cursor nested;
	unsigned long id;
	u64 cursor = ctlv_op_query_cursor(ctx);
	unsigned int generation = att->log->generation;
	int ret;

	if (cursor && ctlv_op_query_generation(ctx) != generation)
		return -ESTALE;
	id = cursor ? cursor - 1 : 0;
	mr = xa_find(&att->mrs, &id, UINT_MAX, XA_PRESENT);
	while (mr) {
		if (!ctlv_attestation_mr_item_start(reply, &nested))
			return ctlv_op_query_more(ctx, generation, id + 1);
		ret = attestation_mr_props_encode(&nested, mr, true);
		ctlv_nest_end(&nested);
		if (ret)
			return ret;
		mr = xa_find_after(&att->mrs, &id, UINT_MAX, XA_PRESENT);
	}
	return ctlv_op_query_done(ctx, generation);
}

static int attestation_mr_values_put(struct ctlv_cursor *reply,
				     const struct attestation_digest_set *values)
{
	struct ctlv_cursor nested;
	size_t index;
	bool ok;

	for (index = 0; index < values->values_count; index++) {
		if (!ctlv_attestation_mr_value_start(reply, &nested))
			return -EMSGSIZE;
		ok = ctlv_attestation_mr_hash_algo_put(&nested, values->values[index].algorithm);
		ok &= ctlv_attestation_mr_data_put(&nested, values->values[index].data,
						   hash_digest_size[values->values[index].algorithm]);
		ctlv_nest_end(&nested);
		if (!ok)
			return -EMSGSIZE;
	}
	return 0;
}

int ctlv_attestation_mr_read_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_digest_set values = {};
	struct attestation *att = priv;
	struct attestation_mr *mr;
	u64 id;
	int ret;

	if (ctlv_op_query_cursor(ctx) &&
	    ctlv_op_query_generation(ctx) != att->mr_read_generation)
		return -ESTALE;
	if (ctlv_attestation_mr_read_device_generation_present(request) &&
	    ctlv_attestation_mr_read_device_generation_get(request) != att->device_generation)
		return -ESTALE;
	id = ctlv_attestation_mr_read_mr_id_get(request);
	if (id > UINT_MAX)
		return -ERANGE;
	mr = attestation_mr_lookup_live(att, id);
	if (IS_ERR(mr))
		return PTR_ERR(mr);
	if (!mr->props->readable)
		return -EOPNOTSUPP;
	values.values_count = mr->props->algorithms_count;
	values.values = kcalloc(values.values_count, sizeof(*values.values), GFP_KERNEL);
	if (!values.values)
		return -ENOMEM;
	for (id = 0; id < values.values_count; id++)
		values.values[id].algorithm = mr->props->algorithms[id];
	ret = att->provider->mr_read(att, mr->props->id, &values);
	if (!ret)
		ret = attestation_mr_values_validate(mr, &values);
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

static int attestation_digest_compare(const void *left, const void *right)
{
	const struct attestation_digest *a = left;
	const struct attestation_digest *b = right;

	return (int)a->algorithm - (int)b->algorithm;
}

static int attestation_mr_extend_decode(const struct ctlv_cursor *request,
					struct attestation_digest_set *values)
{
	struct ctlv_cursor item;
	size_t count = 0, index = 0;

	ctlv_attestation_mr_extend_mr_value_for_each(&item, request) count++;
	values->values = kcalloc(count, sizeof(*values->values), GFP_KERNEL);
	if (!values->values)
		return -ENOMEM;
	values->values_count = count;
	ctlv_attestation_mr_extend_mr_value_for_each(&item, request)
	{
		struct attestation_digest *value = &values->values[index++];
		u32 length;
		const void *data;
		u64 algorithm;

		algorithm =
			ctlv_attestation_mr_extend_value_mr_hash_algo_get(&item);
		if (algorithm >= HASH_ALGO__LAST || !hash_digest_size[algorithm])
			return -EINVAL;
		data = ctlv_attestation_mr_extend_value_mr_data_get(&item, &length);
		if (length != hash_digest_size[algorithm])
			return -EINVAL;
		value->algorithm = algorithm;
		memcpy(value->data, data, length);
	}
	sort(values->values, values->values_count, sizeof(*values->values),
	     attestation_digest_compare, NULL);
	return 0;
}

int ctlv_attestation_mr_extend_action(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_digest_set values = {};
	struct attestation_mr_change *change;
	const struct attestation_mr_log_entry *entry;
	struct attestation *att = priv;
	struct attestation_mr *mr;
	u64 id;
	int ret;

	if (ctlv_attestation_mr_extend_device_generation_present(request) &&
	    ctlv_attestation_mr_extend_device_generation_get(request) != att->device_generation)
		return -ESTALE;
	id = ctlv_attestation_mr_extend_mr_id_get(request);
	if (id > UINT_MAX)
		return -ERANGE;
	mr = attestation_mr_lookup_live(att, id);
	if (IS_ERR(mr))
		return PTR_ERR(mr);
	if (!mr->props->extendable)
		return -EOPNOTSUPP;
	ret = attestation_mr_extend_decode(request, &values);
	if (ret)
		goto out_values;
	ret = attestation_mr_values_validate(mr, &values);
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

int ctlv_attestation_mr_log_get_query(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation *att = priv;
	struct attestation_mr_log_entry *entry;
	struct ctlv_cursor item, nested;
	unsigned int ids[256];
	size_t ids_count = 0, index;
	unsigned int start, end;
	unsigned long xa_index;
	u64 boundary;
	u64 cursor = ctlv_op_query_cursor(ctx);
	int ret;

	ctlv_attestation_mr_log_get_mr_selection_for_each(&item, request)
	{
		u64 id = ctlv_attestation_mr_log_get_selection_mr_id_get(&item);

		if (id > UINT_MAX)
			return -ERANGE;
		for (index = 0; index < ids_count; index++)
			if (ids[index] == id)
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
	if (ctlv_attestation_mr_log_get_mr_end_generation_present(request)) {
		u64 requested_end = ctlv_attestation_mr_log_get_mr_end_generation_get(request);

		end = min_t(u64, end, requested_end);
	}
	start = 1;
	if (ctlv_attestation_mr_log_get_mr_start_generation_present(request)) {
		u64 value = ctlv_attestation_mr_log_get_mr_start_generation_get(request);

		if (value > UINT_MAX)
			return -EINVAL;
		start = value;
	}
	if (att->log->generation && start > end)
		return -EINVAL;
	if (!cursor && att->log->generation) {
		if (!ctlv_attestation_mr_start_generation_put(reply, start) ||
		    !ctlv_attestation_mr_end_generation_put(reply, end))
			return -EMSGSIZE;
	}
	xa_index = cursor ? cursor : start;
	entry = xa_find(&att->log->entries, &xa_index, end, XA_PRESENT);
	while (entry && !attestation_log_selected(ids, ids_count, entry->mr_id))
		entry = xa_find_after(&att->log->entries, &xa_index, end, XA_PRESENT);
	if (!entry)
		return ctlv_op_query_done(ctx, boundary);
	if (!cursor)
		return ctlv_op_query_more(ctx, boundary, entry->mr_generation);
	if (!ctlv_attestation_log_entry_start(reply, &nested))
		return -EMSGSIZE;
	ret = attestation_mr_log_entry_encode(&nested, entry);
	ctlv_nest_end(&nested);
	if (ret)
		return ret;
	entry = xa_find_after(&att->log->entries, &xa_index, end, XA_PRESENT);
	while (entry && !attestation_log_selected(ids, ids_count, entry->mr_id))
		entry = xa_find_after(&att->log->entries, &xa_index, end, XA_PRESENT);
	if (entry)
		return ctlv_op_query_more(ctx, boundary, entry->mr_generation);
	return ctlv_op_query_done(ctx, boundary);
}

int ctlv_attestation_devsec_state_get_query(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply)
{
	struct attestation *att = priv;
	enum attestation_device_trust_state trust;
	enum attestation_tdisp_state state;
	u64 version = att->devsec_version;
	int ret;
	bool ok;

	if (ctlv_op_query_cursor(ctx) && ctlv_op_query_generation(ctx) != version)
		return -ESTALE;
	ok = ctlv_attestation_device_generation_put(reply, att->device_generation);
	if (att->evidence_generation > att->evidence_floor)
		ok &= ctlv_attestation_evidence_generation_put(reply, att->evidence_generation);
	if (att->verdict_recorded)
		ok &= ctlv_attestation_evidence_accepted_generation_put(reply,
									att->accepted_evidence_generation);
	if (att->provider->tdisp_state_get) {
		ret = att->provider->tdisp_state_get(att, &state);
		if (ret)
			return ret;
		ok &= ctlv_attestation_devsec_tdisp_state_put(reply, state);
	}
	if (att->provider->device_trust_get) {
		ret = att->provider->device_trust_get(att, &trust);
		if (ret)
			return ret;
		ok &= ctlv_attestation_devsec_device_trust_put(reply, trust);
	}
	if (!ok)
		return -EMSGSIZE;
	return ctlv_op_query_done(ctx, version);
}

int ctlv_attestation_devsec_run_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct attestation_event_batch events = {};
	struct attestation *att = priv;
	u64 device, evidence;
	int ret;

	device = ctlv_attestation_devsec_run_device_generation_get(request);
	evidence = ctlv_attestation_devsec_run_evidence_generation_get(request);
	if (device > UINT_MAX || evidence > UINT_MAX)
		return -ESTALE;
	if (!att->verdict_recorded || att->accepted_device_generation != device ||
	    att->accepted_evidence_generation != evidence)
		return -ESTALE;
	ret = attestation_evidence_validate(att, device, evidence);
	if (ret)
		return ret;
	ret = attestation_preflight(att, true, false, false);
	if (ret)
		return ret;
	ret = attestation_events_prepare(att, &events, true, true, false);
	if (ret)
		return ret;
	ret = att->provider->devsec_run(att);
	if (ret)
		goto out;
	attestation_device_changed(att);
	attestation_device_event_emit(&events, att->device_generation);
	attestation_evidence_event_emit(&events, att);
	if (!ctlv_attestation_device_generation_put(reply, att->device_generation))
		ret = -EMSGSIZE;
out:
	attestation_events_abort(&events);
	return ret;
}
