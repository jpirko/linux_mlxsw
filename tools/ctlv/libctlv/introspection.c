// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * What every device answers about itself, whatever family it belongs to: which
 * ops exist, whether they can be called and how big their messages are, all in
 * numbers, so the same code works against a family that did not exist when it
 * was written. The generation the dump carries orders it against the
 * op-changed events that follow.
 */

#include "internal.h"

static void op_info_read(struct ctlv_op_info *info,
			 const struct ctlv_attr *entry)
{
	static const struct {
		__u64 attr_id;
		size_t offset;
	} lengths[] = {
		{ CTLV_ATTR_OP_REQUEST_MAX_LEN,
		  offsetof(struct ctlv_op_info, request_max_len) },
		{ CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
		  offsetof(struct ctlv_op_info, action_reply_max_len) },
		{ CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
		  offsetof(struct ctlv_op_info, query_reply_min_len) },
		{ CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
		  offsetof(struct ctlv_op_info, query_reply_suggested_len) },
		{ CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
		  offsetof(struct ctlv_op_info, event_record_max_len) },
	};
	const struct ctlv_attr *attr;
	unsigned int i;
	__u64 value;
	bool flag;

	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_ID);
	if (attr && !ctlv_get_u64(attr, &value))
		info->op_id = value;
	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_TYPE);
	if (attr && !ctlv_get_enum(attr, CTLV_OP_TYPE_EVENT, &value, NULL))
		info->type = value;
	/* Absent on an event, which is never executed. */
	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_AVAILABILITY);
	if (attr && !ctlv_get_enum(attr, CTLV_OP_AVAILABILITY_DYNAMIC, &value,
				   NULL))
		info->availability = value;
	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_DISABLED);
	if (attr && !ctlv_get_bool(attr, &flag))
		info->disabled = flag;

	/* The lengths that do not apply to this op's type are simply absent. */
	for (i = 0; i < CTLV_ARRAY_SIZE(lengths); i++) {
		attr = ctlv_attr_find(entry, lengths[i].attr_id);
		if (attr && !ctlv_get_u64(attr, &value))
			*(__u64 *)((char *)info + lengths[i].offset) = value;
	}
}

int ctlv_ops_dump(struct ctlv_dev *dev, struct ctlv_ops **opsp,
		  struct ctlv_error *error)
{
	const struct ctlv_attr *root, *entry;
	struct ctlv_reply *reply = NULL;
	struct ctlv_ops *ops;
	unsigned int n = 0;
	int ret;

	/* Sized, or the dump kept for op lengths would ask for itself. */
	ret = ctlv_query_sized(dev, CTLV_OP_OPS_DUMP, NULL, 0, &reply, error);
	if (ret)
		return ret;

	root = ctlv_reply_root(reply);
	ops = calloc(1, sizeof(*ops));
	if (!ops) {
		ret = -ENOMEM;
		goto out;
	}
	n = ctlv_attr_count(root, CTLV_ATTR_OP_ENTRY);
	if (n) {
		ops->op = calloc(n, sizeof(*ops->op));
		if (!ops->op) {
			ret = -ENOMEM;
			goto out_free_ops;
		}
	}
	ctlv_for_each_id(entry, root, CTLV_ATTR_OP_ENTRY)
		op_info_read(&ops->op[ops->n++], entry);

	/*
	 * One generation for the whole dump: a change between two fragments
	 * would have failed the continuation rather than reached here.
	 */
	ops->generation = ctlv_reply_generation(reply);

	*opsp = ops;
	ctlv_reply_free(reply);
	return 0;

out_free_ops:
	free(ops);
out:
	ctlv_error_set(error, ret);
	ctlv_reply_free(reply);
	return ret;
}

void ctlv_ops_free(struct ctlv_ops *ops)
{
	if (!ops)
		return;
	free(ops->op);
	free(ops);
}

const struct ctlv_op_info *ctlv_ops_find(const struct ctlv_ops *ops,
					 __u64 op_id)
{
	unsigned int i;

	if (!ops)
		return NULL;
	for (i = 0; i < ops->n; i++)
		if (ops->op[i].op_id == op_id)
			return &ops->op[i];
	return NULL;
}

int ctlv_op_schema_get(struct ctlv_dev *dev, __u64 op_id,
		       struct ctlv_schema **schemap, struct ctlv_error *error)
{
	struct ctlv_reply *reply = NULL;
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_msg_new(64, &request);
	if (ret) {
		ctlv_error_set(error, ret);
		return ret;
	}
	ctlv_put_u64(request, CTLV_ATTR_OP_ID, op_id);

	ret = ctlv_query(dev, CTLV_OP_OP_SCHEMA_GET, request, &reply, error);
	ctlv_msg_free(request);
	if (ret)
		return ret;

	ret = ctlv_schema_parse(ctlv_reply_root(reply), schemap);
	if (ret)
		ctlv_error_set(error, ret);
	ctlv_reply_free(reply);
	return ret;
}
