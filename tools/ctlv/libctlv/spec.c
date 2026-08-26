// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Holding a device against the specification it was described by, so a test
 * can say a kernel and a userspace built from one specification have not
 * drifted. Publishing fewer ops than the family describes is a device's right;
 * publishing one it never described, or describing one differently, is drift.
 * The family compared is the one the device registered as, an ancestor's
 * specification not being what a descendant's device answers.
 */

#include <stdio.h>

#include "internal.h"

/* Where a difference is written, long enough for a path and two numbers. */
#define DIFF_LEN 512

#define UNEQUAL(_what, _expected, _actual) do {				\
	if ((_expected) != (_actual)) {					\
		snprintf(diff, len, "op %#llx: %s %llu != %llu",		\
			 (unsigned long long)spec->op_id, _what,		\
			 (unsigned long long)(_expected),		\
			 (unsigned long long)(_actual));		\
		return -ENOMSG;						\
	}								\
} while (0)

const struct ctlv_op_spec *ctlv_spec_find(const struct ctlv_family_spec *spec,
					  __u64 op_id)
{
	unsigned int i;

	for (; spec; spec = spec->parent)
		for (i = 0; i < spec->n_op; i++)
			if (spec->op[i].op_id == op_id)
				return &spec->op[i];
	return NULL;
}

/* What an ops dump reports about an op, against what the family says it is. */
static int info_check(const struct ctlv_op_spec *spec,
		      const struct ctlv_op_info *info, char *diff, size_t len)
{
	UNEQUAL("type", spec->type, info->type);
	UNEQUAL("availability", spec->availability, info->availability);
	UNEQUAL("request-max-len", spec->request_max_len,
		info->request_max_len);
	UNEQUAL("action-reply-max-len", spec->action_reply_max_len,
		info->action_reply_max_len);
	UNEQUAL("query-reply-min-len", spec->query_reply_min_len,
		info->query_reply_min_len);
	UNEQUAL("query-reply-suggested-len", spec->query_reply_suggested_len,
		info->query_reply_suggested_len);
	UNEQUAL("event-record-max-len", spec->event_record_max_len,
		info->event_record_max_len);
	return 0;
}

/* Both trees of one op, as the device describes them. */
static int schema_check(struct ctlv_dev *dev, const struct ctlv_op_spec *spec,
			char *diff, size_t len, struct ctlv_error *error)
{
	char where[DIFF_LEN];
	struct ctlv_schema *schema;
	int ret;

	ret = ctlv_op_schema_get(dev, spec->op_id, &schema, error);
	if (ret)
		return ret;

	ret = 0;
	if (!ctlv_schema_equal(spec->request, ctlv_schema_request(schema),
			       where, sizeof(where))) {
		snprintf(diff, len, "op %#llx request: %s",
			 (unsigned long long)spec->op_id, where);
		ret = -ENOMSG;
	} else if (!ctlv_schema_equal(spec->reply, ctlv_schema_reply(schema),
				      where, sizeof(where))) {
		snprintf(diff, len, "op %#llx reply: %s",
			 (unsigned long long)spec->op_id, where);
		ret = -ENOMSG;
	}
	ctlv_schema_free(schema);
	return ret;
}

/* Whether the chain @spec describes is the chain @dev resolved. */
static int chain_check(struct ctlv_dev *dev,
		       const struct ctlv_family_spec *spec, char *diff,
		       size_t len)
{
	const struct ctlv_family_spec *walk;
	unsigned int level;

	if (strcmp(spec->name, ctlv_dev_family(dev))) {
		snprintf(diff, len, "%s describes a %s, not a %s",
			 spec->name, spec->name, ctlv_dev_family(dev));
		return -EINVAL;
	}

	level = ctlv_chain_len(dev);
	for (walk = spec; walk; walk = walk->parent) {
		if (!level--) {
			snprintf(diff, len,
				 "the device's chain never reaches %s",
				 walk->name);
			return -ENOMSG;
		}
		if (walk->family_id != ctlv_chain_id(dev, level) ||
		    strcmp(walk->name, ctlv_chain_name(dev, level))) {
			snprintf(diff, len,
				 "chain level %u: %s (%u) != %s (%u)", level,
				 walk->name, walk->family_id,
				 ctlv_chain_name(dev, level),
				 ctlv_chain_id(dev, level));
			return -ENOMSG;
		}
	}
	if (level) {
		snprintf(diff, len, "%s describes %u families too few",
			 spec->name, level);
		return -ENOMSG;
	}
	return 0;
}

int ctlv_spec_check(struct ctlv_dev *dev, const struct ctlv_family_spec *spec,
		    char *diff, size_t len, struct ctlv_error *error)
{
	struct ctlv_ops *ops;
	unsigned int i;
	int ret;

	if (diff && len)
		*diff = '\0';
	ret = chain_check(dev, spec, diff, len);
	if (ret)
		return ret;
	ret = ctlv_ops_dump(dev, &ops, error);
	if (ret)
		return ret;

	for (i = 0; i < ops->n; i++) {
		const struct ctlv_op_info *info = &ops->op[i];
		const struct ctlv_op_spec *described;

		described = ctlv_spec_find(spec, info->op_id);
		if (!described) {
			snprintf(diff, len,
				 "op %#llx: published, and %s has no such op",
				 (unsigned long long)info->op_id, spec->name);
			ret = -ENOMSG;
			break;
		}
		ret = info_check(described, info, diff, len);
		if (ret)
			break;
		ret = schema_check(dev, described, diff, len, error);
		if (ret)
			break;
	}
	ctlv_ops_free(ops);
	return ret;
}
