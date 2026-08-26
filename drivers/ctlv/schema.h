/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Reading the generated schema graph: the families of one chain, the ops they
 * declare, the members filling their extension points, and one family's message
 * lengths. The generated types are public; walking them is the framework's
 * alone, so this stays here.
 */
#ifndef _CTLV_SCHEMA_H
#define _CTLV_SCHEMA_H

#include <linux/ctlv.h>
#include <linux/types.h>

/**
 * struct ctlv_schema_cursor - Position of a walk over one family's ops.
 * @target: family being walked
 * @family: family whose ops are being visited, or NULL before the first
 * @level: levels of @target's chain left to visit, @family's included
 * @index: next op of @family
 */
struct ctlv_schema_cursor {
	const struct ctlv_family *target;
	const struct ctlv_family *family;
	u32 level;
	u32 index;
};

/**
 * struct ctlv_schema_pos - One node of one tree.
 * @tree: tree the node belongs to, a fill's own tree for a contributed member
 * @node: index of the node within @tree
 *
 * A member of a nest is not always a node of the nest's own tree.
 */
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
const struct ctlv_stub_fill *
ctlv_schema_fill(const struct ctlv_family *family, u16 family_id,
		 u32 set_id);
int ctlv_schema_op_sizes(const struct ctlv_family *family,
			 const struct ctlv_op_schema *op,
			 struct ctlv_op_sizes *sizes);
bool ctlv_schema_member(const struct ctlv_family *family,
			const struct ctlv_schema_tree *tree, u32 node,
			u64 attr_id, struct ctlv_schema_pos *member);
bool ctlv_schema_value_ok(const struct ctlv_schema_node *node, u64 value);
bool ctlv_schema_length_ok(const struct ctlv_schema_node *node, u32 len);

struct ctlv_error;
struct ctlv_msg;

int ctlv_schema_validate(struct ctlv_msg *msg, struct ctlv_error *error);

#endif /* _CTLV_SCHEMA_H */
