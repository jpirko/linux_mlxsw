/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Private message representation and nest walking.
 */
#ifndef _CTLV_MESSAGE_H
#define _CTLV_MESSAGE_H

#include <linux/ctlv_family.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

#define CTLV_MESSAGE_INPUT_INDEX_NONE U32_MAX

/* Attr identity remains in the generated schema, not in this request index. */
struct ctlv_message_input_slot {
	u32 first_offset;
	u32 end_offset;
	u32 count;
	u32 excess_offset;
	u32 first_nest;
	u32 last_nest;
};

struct ctlv_message_input_scope {
	const struct ctlv_schema_tree *tree;
	u32 nest;
	u32 node;
	u32 first_slot;
	u32 slots_count;
	bool optional;
};

struct ctlv_message_input_nest {
	u32 offset;
	u32 first_scope;
	u32 scopes_count;
	u32 next;
};

struct ctlv_message_input_index {
	struct ctlv_message_input_nest *nests;
	struct ctlv_message_input_scope *scopes;
	struct ctlv_message_input_slot *slots;
	u32 nests_count;
	u32 nests_capacity;
	u32 scopes_count;
	u32 scopes_capacity;
	u32 slots_count;
	u32 slots_capacity;
};

struct ctlv_msg {
	struct ctlv_attr *root;
	size_t len;
	const struct ctlv_schema_tree *tree;
	const struct ctlv_family *family;
	struct ctlv_op_ctx *ctx;
	struct ctlv_message_input_index *index;
	bool request;
	int poison;
};

enum ctlv_message_step {
	CTLV_MESSAGE_STEP_ATTR,
	CTLV_MESSAGE_STEP_END,
	CTLV_MESSAGE_STEP_BAD,
};

enum ctlv_message_step ctlv_message_next(const struct ctlv_attr *nest,
					 const struct ctlv_attr **child);
u32 ctlv_message_offset(const struct ctlv_msg *msg,
			const struct ctlv_attr *attr);
void ctlv_message_root(struct ctlv_cursor *cursor, struct ctlv_msg *msg);
void ctlv_message_index_free(struct ctlv_msg *msg);
bool ctlv_message_input_rebind(struct ctlv_cursor *nest,
			       const struct ctlv_schema_tree *tree, u32 node);

static inline u32 ctlv_message_attr_payload_len(const struct ctlv_attr *attr)
{
	return attr->len - sizeof(*attr);
}

static inline const void *
ctlv_message_attr_payload(const struct ctlv_attr *attr)
{
	return attr + 1;
}

#endif /* _CTLV_MESSAGE_H */
