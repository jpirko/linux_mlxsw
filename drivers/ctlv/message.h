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

/**
 * struct ctlv_msg - One message the core has snapshotted or is building.
 * @root: the root attr, where the buffer begins
 * @len: bytes available from @root, to read in a request or write in an output
 * @tree: schema of this message, owned by the family that declared its op
 * @leaf: family the device answers as, which resolves @tree's extension points
 * @ctx: the execution this is the output of, NULL in a request
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
	const struct ctlv_family_schema *leaf;
	struct ctlv_op_ctx *ctx;
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

static inline u32 ctlv_attr_payload_len(const struct ctlv_attr *attr)
{
	return attr->len - sizeof(*attr);
}

static inline const void *ctlv_attr_payload(const struct ctlv_attr *attr)
{
	return attr + 1;
}

#endif /* _CTLV_MESSAGE_H */
