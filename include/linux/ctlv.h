/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family API.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>

struct ctlv_family;
struct ctlv;
struct ctlv_msg;
struct ctlv_op_ctx;
struct ctlv_schema_tree;

/**
 * struct ctlv_cursor - One TLV position in one message.
 * @msg: the message this position is in, core-private bookkeeping
 * @hdr: the TLV itself, or NULL when a lookup found nothing
 * @parent: reading, the record containing @hdr, NULL at the root
 * @up: writing, cursor of the record containing @hdr, NULL at the root. The
 *	chain of them is the open-record stack, so a cursor must not outlive the
 *	cursor of its record
 * @start: writing, where @hdr begins in the message. An append reaches what
 *	it lengthens through the buffer rather than through @hdr, that buffer
 *	being the core's own
 * @tree: schema tree the node belongs to, which is a fill's own tree for a
 *	TLV that arrived through one
 * @node: index of the node within @tree
 * @depth: record levels above this position, the root being zero
 * @index_record_scope: schema scope used to read the members of @hdr, a record
 * @index_scope: core-private schema scope containing this TLV
 * @progress: writing, whether the fragment had progressed before this record
 *	opened
 *
 * Fixed-size, so family code can put a child cursor on the stack.
 * ctlv_record_start() opens at most CTLV_MAX_RECORD_DEPTH levels, which bounds
 * that stack. The members belong to the core: a family reaches a message
 * through what was generated for the op it answers. One direction at a time,
 * a helper of the other direction refusing the cursor.
 */
struct ctlv_cursor {
	struct ctlv_msg *msg;
	const struct ctlv_hdr *hdr;
	const struct ctlv_hdr *parent;
	struct ctlv_cursor *up;
	size_t start;
	const struct ctlv_schema_tree *tree;
	u32 node;
	u32 depth;
	u32 index_record_scope;
	u32 index_scope;
	bool progress;
};

void ctlv_record_end(struct ctlv_cursor *child);
void ctlv_record_cancel(struct ctlv_cursor *child);

int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message);
__printf(3, 4)
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...);

/*
 * The core retains no query state. A continuation repeats the request and
 * passes back the family's generation and nonzero cursor. It fails with
 * -ESTALE if the generation changed. Each fragment must complete a logical
 * leaf and return a cursor different from its input.
 *
 * ctlv_op_query_more() marks still-open records, whose cursors must stay live.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx);
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx);
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor);
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation);

struct ctlv_blob;

int ctlv_blob_put(struct ctlv_blob *blob);
u64 ctlv_blob_len(const struct ctlv_blob *blob);
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len);
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len);
int ctlv_blob_too_short(struct ctlv_blob *blob, u64 len);

#endif /* _LINUX_CTLV_H */
