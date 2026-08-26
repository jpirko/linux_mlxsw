/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
#ifndef CTLVCTL_SPEC_H
#define CTLVCTL_SPEC_H

#include <linux/types.h>
#include <stdbool.h>

struct ctlv_dev;

struct ctlvctl_value_name {
	__u64 value;
	char *name;
};

struct ctlvctl_definition {
	char *name;
	__u32 kind;
	struct ctlvctl_value_name *values;
	unsigned int values_count;
};

struct ctlvctl_attr_name {
	__u64 id;
	__u32 kind;
	char *name;
	char *short_name;
	const struct ctlvctl_definition *definition;
};

struct ctlvctl_op_name {
	__u64 id;
	__u32 type;
	__u32 fd_access;
	__u32 availability;
	__u64 request_max_len;
	__u64 action_reply_max_len;
	__u64 query_reply_min_len;
	__u64 query_reply_suggested_len;
	__u64 event_record_max_len;
	char *name;
};

struct ctlvctl_spec;

int ctlvctl_spec_load(const char *prog, const char *dir,
		      const struct ctlv_dev *dev, struct ctlvctl_spec **specp);
void ctlvctl_spec_free(struct ctlvctl_spec *spec);

const struct ctlvctl_op_name *
ctlvctl_spec_op_id(const struct ctlvctl_spec *spec, __u64 id);
const struct ctlvctl_op_name *
ctlvctl_spec_op_name(const struct ctlvctl_spec *spec, const char *name);
const struct ctlvctl_attr_name *
ctlvctl_spec_attr_id(const struct ctlvctl_spec *spec, __u64 id);

const char *ctlvctl_spec_value_name(const struct ctlvctl_attr_name *attr,
				    __u64 value);
int ctlvctl_spec_value(const struct ctlvctl_attr_name *attr, const char *name,
		       __u64 *value);

#endif /* CTLVCTL_SPEC_H */
