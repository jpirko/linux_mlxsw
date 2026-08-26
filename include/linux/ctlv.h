/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family API.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/types.h>

#include <uapi/linux/ctlv.h>

struct ctlv_device;
struct ctlv_family;
struct ctlv_msg;
struct ctlv_op_ctx;
struct ctlv_schema_tree;

/**
 * struct ctlv_cursor - One attr position in one message.
 * @msg: the message this position is in, core-private bookkeeping
 * @attr: the attr itself, or NULL when a lookup found nothing
 * @parent: reading, the nest containing @attr, NULL at the root
 * @up: writing, cursor of the nest containing @attr, NULL at the root. The
 *	chain of them is the open-nest stack
 * @start: writing, where @attr begins in the message. An append reaches what
 *	it lengthens through the buffer rather than through @attr, that buffer
 *	being the core's own
 * @tree: schema tree the node belongs to, which is a fill's own tree for an
 *	attr that arrived through one
 * @node: index of the node within @tree
 * @depth: nesting levels above this position, the root being zero
 * @index_nest_scope: schema scope used to read members of the physical nest
 * @index_scope: core-private schema scope containing this attr
 *
 * Fixed-size, so family code can put a child cursor on the stack. The members
 * belong to the core: a family reaches a message through what was generated
 * for the op it answers. One direction at a time, a helper of the other
 * direction refusing the cursor.
 */
struct ctlv_cursor {
	struct ctlv_msg *msg;
	const struct ctlv_attr *attr;
	const struct ctlv_attr *parent;
	struct ctlv_cursor *up;
	size_t start;
	const struct ctlv_schema_tree *tree;
	u32 node;
	u32 depth;
	u32 index_nest_scope;
	u32 index_scope;
};

#endif /* _LINUX_CTLV_H */
