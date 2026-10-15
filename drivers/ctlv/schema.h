/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Private generated-schema walkers.
 */
#ifndef _CTLV_SCHEMA_H
#define _CTLV_SCHEMA_H

#include <linux/ctlv_family.h>
#include <linux/types.h>

struct ctlv_schema_cursor {
	const struct ctlv_family *target;
	const struct ctlv_family *family;
	u32 level;
	u32 index;
};

struct ctlv_schema_pos {
	const struct ctlv_schema_tree *tree;
	u32 node;
};

const struct ctlv_family *
ctlv_family_ancestor(const struct ctlv_family *family, u32 up);
u32 ctlv_family_levels(const struct ctlv_family *family);
void ctlv_schema_cursor_init(struct ctlv_schema_cursor *cursor,
			     const struct ctlv_family *family);
const struct ctlv_op_schema *
ctlv_schema_next_op(struct ctlv_schema_cursor *cursor,
		    const struct ctlv_family **family);
const struct ctlv_stub_fill_schema *
ctlv_schema_fill(const struct ctlv_family *family, u16 family_id,
		 u32 set_id);
int ctlv_schema_op_sizes(const struct ctlv_family *family,
			 const struct ctlv_op_schema *op,
			 struct ctlv_op_sizes *sizes);
bool ctlv_schema_member(const struct ctlv_family *family,
			const struct ctlv_schema_tree *tree, u32 node,
			u64 attr_id, struct ctlv_schema_pos *member);
bool ctlv_schema_value_ok(const struct ctlv_node_schema *node, u64 value);
bool ctlv_schema_length_ok(const struct ctlv_node_schema *node, u32 len);

struct ctlv_error;
struct ctlv_msg;

int ctlv_schema_validate(struct ctlv_msg *msg, struct ctlv_error *error);

#endif /* _CTLV_SCHEMA_H */
