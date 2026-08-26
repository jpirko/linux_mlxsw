/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV messages, core-side: what the core keeps of one message and how it
 * steps through a nest. A family sees none of this.
 */
#ifndef _CTLV_MESSAGE_H
#define _CTLV_MESSAGE_H

#include <linux/ctlv.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

#define CTLV_INPUT_INDEX_NONE U32_MAX

/* Attr identity remains in the generated schema, not in this request index. */
struct ctlv_input_slot {
	u32 first_offset;
	u32 end_offset;
	u32 count;
	u32 excess_offset;
	u32 first_nest;
	u32 last_nest;
};

struct ctlv_input_scope {
	const struct ctlv_schema_tree *tree;
	u32 nest;
	u32 node;
	u32 first_slot;
	u32 n_slots;
	bool optional;
};

struct ctlv_input_nest {
	u32 offset;
	u32 first_scope;
	u32 n_scopes;
	u32 next;
};

struct ctlv_input_index {
	struct ctlv_input_nest *nests;
	struct ctlv_input_scope *scopes;
	struct ctlv_input_slot *slots;
	u32 n_nests;
	u32 nests_capacity;
	u32 n_scopes;
	u32 scopes_capacity;
	u32 n_slots;
	u32 slots_capacity;
};

/**
 * struct ctlv_msg - One message the core has snapshotted or is building.
 * @root: the root attr, where the buffer begins
 * @len: bytes available from @root, to read in a request or write in an output
 * @tree: schema of this message, owned by the family that declared its op
 * @family: device family, which resolves @tree's extension points
 * @ctx: the execution this is the output of, NULL in a request
 * @index: validated request lookup data, NULL for an output
 * @request: whether a caller sent this, which decides the attr-header flags it
 *	may carry: forgiving what this kernel does not know is a caller's
 *	choice and never something an output of the core's own asks for
 * @poison: errno of the first write that failed. A poisoned message is
 *	abandoned: later serializers do nothing and completion returns this
 *
 * The wire is the only representation, so the attr lengths in the buffer stay
 * authoritative and the root's own length is how much has been written.
 */
struct ctlv_msg {
	struct ctlv_attr *root;
	size_t len;
	const struct ctlv_schema_tree *tree;
	const struct ctlv_family *family;
	struct ctlv_op_ctx *ctx;
	struct ctlv_input_index *index;
	bool request;
	int poison;
};

/**
 * enum ctlv_step - What one step through the children of a nest found.
 * @CTLV_STEP_ATTR: one more child, fitting its nest with a usable header
 * @CTLV_STEP_END: the nest ended exactly where its length said
 * @CTLV_STEP_BAD: the nest is malformed and nothing further may be read
 */
enum ctlv_step {
	CTLV_STEP_ATTR,
	CTLV_STEP_END,
	CTLV_STEP_BAD,
};

enum ctlv_step ctlv_msg_step(const struct ctlv_attr *nest,
			     const struct ctlv_attr **child);
u32 ctlv_msg_offset(const struct ctlv_msg *msg, const struct ctlv_attr *attr);
void ctlv_msg_root(struct ctlv_cursor *cursor, struct ctlv_msg *msg);
void ctlv_msg_output(struct ctlv_msg *msg, struct ctlv_op_ctx *ctx, void *buf,
		     size_t len, const struct ctlv_schema_tree *tree);
void ctlv_out_frag_mark(struct ctlv_op_ctx *ctx);
void ctlv_msg_index_free(struct ctlv_msg *msg);
bool ctlv_input_rebind(struct ctlv_cursor *nest,
		       const struct ctlv_schema_tree *tree, u32 node);

static inline u32 ctlv_attr_payload_len(const struct ctlv_attr *attr)
{
	return attr->len - sizeof(*attr);
}

static inline const void *ctlv_attr_payload(const struct ctlv_attr *attr)
{
	return attr + 1;
}

#endif /* _CTLV_MESSAGE_H */
