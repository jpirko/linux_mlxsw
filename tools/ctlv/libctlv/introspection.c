// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Root-family introspection.
 */

#include "internal.h"

enum op_info_kind {
	OP_INFO_U32,
	OP_INFO_U64,
	OP_INFO_BOOL,
};

enum op_info_type {
	OP_INFO_ACTION = 1,
	OP_INFO_QUERY = 2,
	OP_INFO_EVENT = 4,
};

static int op_info_read(struct ctlv_op_info *info,
			const struct ctlv_attr *entry)
{
	static const struct {
		__u64 attr_id;
		size_t offset;
		unsigned int applies;
		enum op_info_kind kind;
	} fields[] = {
		{ CTLV_ATTR_OP_FD_ACCESS,
		  offsetof(struct ctlv_op_info, fd_access),
		  OP_INFO_ACTION | OP_INFO_QUERY, OP_INFO_U32 },
		{ CTLV_ATTR_OP_AVAILABILITY,
		  offsetof(struct ctlv_op_info, availability),
		  OP_INFO_ACTION | OP_INFO_QUERY, OP_INFO_U32 },
		{ CTLV_ATTR_OP_DISABLED,
		  offsetof(struct ctlv_op_info, disabled),
		  OP_INFO_ACTION | OP_INFO_QUERY, OP_INFO_BOOL },
		{ CTLV_ATTR_OP_REQUEST_MAX_LEN,
		  offsetof(struct ctlv_op_info, request_max_len),
		  OP_INFO_ACTION | OP_INFO_QUERY, OP_INFO_U64 },
		{ CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
		  offsetof(struct ctlv_op_info, action_reply_max_len),
		  OP_INFO_ACTION, OP_INFO_U64 },
		{ CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
		  offsetof(struct ctlv_op_info, query_reply_min_len),
		  OP_INFO_QUERY, OP_INFO_U64 },
		{ CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
		  offsetof(struct ctlv_op_info, query_reply_suggested_len),
		  OP_INFO_QUERY, OP_INFO_U64 },
		{ CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
		  offsetof(struct ctlv_op_info, event_record_max_len),
		  OP_INFO_EVENT, OP_INFO_U64 },
	};
	const struct ctlv_attr *attr;
	unsigned int applies = 0;
	unsigned int i;
	__u64 value;
	bool boolean;

	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_ID);
	if (!attr || ctlv_get_u64(attr, &info->op_id))
		return -EPROTO;
	attr = ctlv_attr_find(entry, CTLV_ATTR_OP_TYPE);
	if (!attr || ctlv_get_u64(attr, &value) || value > UINT32_MAX)
		return -EPROTO;
	info->type = value;
	if (value == CTLV_OP_TYPE_ACTION)
		applies = OP_INFO_ACTION;
	else if (value == CTLV_OP_TYPE_QUERY)
		applies = OP_INFO_QUERY;
	else if (value == CTLV_OP_TYPE_EVENT)
		applies = OP_INFO_EVENT;

	for (i = 0; i < CTLV_ARRAY_SIZE(fields); i++) {
		attr = ctlv_attr_find(entry, fields[i].attr_id);
		if (applies && !!attr != !!(fields[i].applies & applies))
			return -EPROTO;
		if (!attr)
			continue;
		if (fields[i].kind == OP_INFO_BOOL) {
			if (ctlv_get_bool(attr, &boolean))
				return -EPROTO;
			*(bool *)((char *)info + fields[i].offset) = boolean;
			continue;
		}
		if (ctlv_get_u64(attr, &value))
			return -EPROTO;
		if (fields[i].kind == OP_INFO_U32) {
			if (value > UINT32_MAX)
				return -EPROTO;
			*(__u32 *)((char *)info + fields[i].offset) = value;
		} else {
			*(__u64 *)((char *)info + fields[i].offset) = value;
		}
	}
	if ((applies & (OP_INFO_ACTION | OP_INFO_QUERY)) &&
	    info->availability == CTLV_OP_AVAILABILITY_FIXED &&
	    info->disabled)
		return -EPROTO;
	if (applies == OP_INFO_QUERY &&
	    info->query_reply_suggested_len < info->query_reply_min_len)
		return -EPROTO;
	return 0;
}

static int op_id_compare(const void *left, const void *right)
{
	const __u64 *a = left;
	const __u64 *b = right;

	return (*a > *b) - (*a < *b);
}

int ctlv_ops_parse(const struct ctlv_attr *root, __u32 len,
		   __u64 generation, struct ctlv_ops **opsp)
{
	const struct ctlv_attr *entry;
	struct ctlv_ops *ops;
	__u64 *ids = NULL;
	unsigned int i;
	unsigned int n = 0;
	int ret;

	ret = ctlv_bootstrap_validate(root, len, CTLV_OP_OPS_DUMP);
	if (ret)
		return ret;

	ops = calloc(1, sizeof(*ops));
	if (!ops)
		return -ENOMEM;
	n = ctlv_attr_count(root, CTLV_ATTR_OP_ENTRY);
	if (n) {
		ops->ops = calloc(n, sizeof(*ops->ops));
		if (!ops->ops) {
			ret = -ENOMEM;
			goto out_free_ops;
		}
		ids = calloc(n, sizeof(*ids));
		if (!ids) {
			ret = -ENOMEM;
			goto out_free_op;
		}
	}
	ctlv_for_each_id(entry, root, CTLV_ATTR_OP_ENTRY) {
		ret = op_info_read(&ops->ops[ops->ops_count], entry);
		if (ret)
			goto out_free_ids;
		ids[ops->ops_count] = ops->ops[ops->ops_count].op_id;
		ops->ops_count++;
	}
	if (n > 1)
		qsort(ids, n, sizeof(*ids), op_id_compare);
	for (i = 1; i < n; i++) {
		if (ids[i] == ids[i - 1]) {
			ret = -EPROTO;
			goto out_free_ids;
		}
	}
	free(ids);
	ops->generation = generation;

	*opsp = ops;
	return 0;

out_free_ids:
	free(ids);
out_free_op:
	free(ops->ops);
out_free_ops:
	free(ops);
	return ret;
}

int ctlv_ops_dump(struct ctlv_dev *dev, struct ctlv_ops **opsp,
		  struct ctlv_error *error)
{
	struct ctlv_reply *reply = NULL;
	int ret;

	/* Sized, or the dump kept for op lengths would ask for itself. */
	ret = ctlv_query_sized(dev, CTLV_OP_OPS_DUMP, NULL, 0, &reply, error);
	if (ret)
		return ret;

	ret = ctlv_ops_parse(ctlv_reply_root(reply), reply->msg->len,
			     ctlv_reply_generation(reply), opsp);
	if (ret)
		ctlv_error_set(error, ret);
	ctlv_reply_free(reply);
	return ret;
}

void ctlv_ops_free(struct ctlv_ops *ops)
{
	if (!ops)
		return;
	free(ops->ops);
	free(ops);
}

const struct ctlv_op_info *ctlv_ops_find(const struct ctlv_ops *ops,
					 __u64 op_id)
{
	unsigned int i;

	if (!ops)
		return NULL;
	for (i = 0; i < ops->ops_count; i++)
		if (ops->ops[i].op_id == op_id)
			return &ops->ops[i];
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

	ret = ctlv_schema_parse(ctlv_reply_root(reply), reply->msg->len,
				schemap);
	if (ret)
		ctlv_error_set(error, ret);
	ctlv_reply_free(reply);
	return ret;
}
