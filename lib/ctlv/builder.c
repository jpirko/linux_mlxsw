// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Schema-aware reply construction and operation completion.
 */
#include <linux/align.h>
#include <linux/bug.h>
#include <linux/ctlv.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/stdarg.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "builder.h"
#include "internal.h"
#include "message.h"
#include "schema.h"

static void ctlv_message_output_frag_mark(struct ctlv_op_ctx *ctx);

void ctlv_message_output_init(struct ctlv_msg *msg, struct ctlv_op_ctx *ctx, void *buf, size_t len,
			      const struct ctlv_schema_tree *tree)
{
	*msg = (struct ctlv_msg){
		.root = buf,
		.len = len,
		.tree = tree,
		.family = ctx->family,
		.ctx = ctx,
	};
	*msg->root = (struct ctlv_hdr){
		.len = sizeof(*msg->root),
		.flags = CTLV_HDR_F_RECORD,
		.type = CTLV_TYPE_ROOT,
	};
}

/* The first thing that went wrong is what the op ends up failing with. */
static bool ctlv_message_output_poison(struct ctlv_cursor *record, int error)
{
	if (!record->msg->poison)
		record->msg->poison = error;
	return false;
}

/* A poisoned message is refused silently: whatever poisoned it has warned. */
static bool ctlv_message_output_ready(struct ctlv_cursor *record)
{
	if (WARN_ON_ONCE(!record->msg || !record->hdr))
		return false;
	/* Also what refuses a cursor that reads a request. */
	if (WARN_ON_ONCE(!record->msg->ctx))
		return false;
	if (record->msg->poison)
		return false;
	if (WARN_ON_ONCE(record->msg->ctx->deepest != record))
		return ctlv_message_output_poison(record, -EINVAL);
	return true;
}

/* What the schema places in this record under one type, as one schema kind. */
static const struct ctlv_node_schema *ctlv_message_output_member(struct ctlv_cursor *record,
								 u64 type, u8 kind,
								 struct ctlv_schema_pos *pos)
{
	const struct ctlv_node_schema *node;

	if (!ctlv_message_output_ready(record))
		return NULL;
	if (WARN_ON_ONCE(!ctlv_schema_member(record->msg->family, record->tree,
					     record->node, type, pos))) {
		ctlv_message_output_poison(record, -EINVAL);
		return NULL;
	}
	node = &pos->tree->node_schemas[pos->node];
	if (WARN_ON_ONCE(node->kind != kind)) {
		ctlv_message_output_poison(record, -EINVAL);
		return NULL;
	}
	return node;
}

/*
 * A query is offered whatever room its caller had, so running out ends a
 * fragment. Anything else wrote a message its own schema does not describe.
 */
static bool ctlv_message_output_full(struct ctlv_cursor *record)
{
	const struct ctlv_op_schema *op = record->msg->ctx->op->schema;

	if (op->type == CTLV_OP_TYPE_QUERY)
		return false;
	CTLV_ASSERT(0);
	return ctlv_message_output_poison(record, -EMSGSIZE);
}

/* What one cursor stands on, as the buffer this direction may change. */
static struct ctlv_hdr *ctlv_message_output_hdr(const struct ctlv_cursor *record)
{
	return (struct ctlv_hdr *)((u8 *)record->msg->root + record->start);
}

/* The pad is written rather than left as it was: it belongs to the record. */
static struct ctlv_hdr *ctlv_message_output_write(struct ctlv_cursor *record, u64 type, u32 flags,
						  const void *payload, u32 payload_len)
{
	u32 len = sizeof(struct ctlv_hdr) + payload_len;
	size_t step = ALIGN((size_t)len, CTLV_ALIGNTO);
	struct ctlv_msg *msg = record->msg;
	struct ctlv_cursor *open;
	struct ctlv_hdr *hdr;
	size_t offset;

	offset = record->start + ctlv_message_output_hdr(record)->len;
	if (offset + step > msg->len) {
		ctlv_message_output_full(record);
		return NULL;
	}

	hdr = (struct ctlv_hdr *)((u8 *)msg->root + offset);
	*hdr = (struct ctlv_hdr){
		.len = len,
		.flags = flags,
		.type = type,
	};
	if (payload_len)
		memcpy(hdr + 1, payload, payload_len);
	memset((u8 *)hdr + len, 0, step - len);
	if (!(flags & CTLV_HDR_F_RECORD))
		msg->ctx->frag.progress = true;

	/* Every record the TLV landed in is that much longer, the root too. */
	for (open = record; open; open = open->up)
		ctlv_message_output_hdr(open)->len += step;
	return hdr;
}

/* The 64-bit payload that every integer, enum and flags field carries. */
static bool ctlv_message_put_scalar(struct ctlv_cursor *record, u64 type, u8 kind, u64 value)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;

	node = ctlv_message_output_member(record, type, kind, &pos);
	if (!node)
		return false;
	/* A value outside what the field declares is one nothing may be told. */
	if (WARN_ON_ONCE(!ctlv_schema_value_ok(record->msg->family, node, value)))
		return ctlv_message_output_poison(record, -EINVAL);
	return ctlv_message_output_write(record, type, 0, &value, sizeof(value));
}

bool ctlv_put_u64(struct ctlv_cursor *record, u64 type, u8 kind, u64 value)
{
	return ctlv_message_put_scalar(record, type, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_u64, "CTLV");

bool ctlv_put_s64(struct ctlv_cursor *record, u64 type, u8 kind, s64 value)
{
	return ctlv_message_put_scalar(record, type, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_s64, "CTLV");

bool ctlv_put_bool(struct ctlv_cursor *record, u64 type, u8 kind, bool value)
{
	u8 payload = value;
	struct ctlv_schema_pos pos;

	if (!ctlv_message_output_member(record, type, kind, &pos))
		return false;
	return ctlv_message_output_write(record, type, 0, &payload, sizeof(payload));
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_bool, "CTLV");

bool ctlv_put_string(struct ctlv_cursor *record, u64 type, u8 kind, const char *value)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;
	size_t len;

	node = ctlv_message_output_member(record, type, kind, &pos);
	if (!node)
		return false;
	len = strnlen(value, (size_t)node->length.max_len + 1);
	if (WARN_ON_ONCE(len > node->length.max_len || !ctlv_schema_length_ok(node, len)))
		return ctlv_message_output_poison(record, -EINVAL);
	return ctlv_message_output_write(record, type, 0, value, len + 1);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_string, "CTLV");

bool ctlv_put_binary(struct ctlv_cursor *record, u64 type, u8 kind, const void *value, u32 len)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;

	node = ctlv_message_output_member(record, type, kind, &pos);
	if (!node)
		return false;
	if (WARN_ON_ONCE(!ctlv_schema_length_ok(node, len)))
		return ctlv_message_output_poison(record, -EINVAL);
	return ctlv_message_output_write(record, type, 0, value, len);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_binary, "CTLV");

bool ctlv_record_start(struct ctlv_cursor *record, struct ctlv_cursor *child, u64 type, u8 kind)
{
	struct ctlv_schema_pos pos;
	struct ctlv_hdr *hdr;

	*child = (struct ctlv_cursor){};
	if (!ctlv_message_output_member(record, type, kind, &pos))
		return false;
	/* Unreachable through any schema the generator would have accepted. */
	if (WARN_ON_ONCE(record->depth + 1 > CTLV_MAX_RECORD_DEPTH))
		return ctlv_message_output_poison(record, -EINVAL);

	hdr = ctlv_message_output_write(record, type, CTLV_HDR_F_RECORD, NULL, 0);
	if (!hdr)
		return false;

	*child = (struct ctlv_cursor){
		.msg = record->msg,
		.hdr = hdr,
		.up = record,
		.start = ctlv_message_offset(record->msg, hdr),
		.tree = pos.tree,
		.node = pos.node,
		.depth = record->depth + 1,
		.progress = record->msg->ctx->frag.progress,
	};
	record->msg->ctx->deepest = child;
	return true;
}
EXPORT_SYMBOL_NS_GPL(ctlv_record_start, "CTLV");

/* Only the innermost open record of an unpoisoned message can be closed. */
static struct ctlv_op_ctx *ctlv_message_output_closing(struct ctlv_cursor *child)
{
	struct ctlv_op_ctx *ctx;

	if (WARN_ON_ONCE(!child->msg || !child->msg->ctx || !child->up))
		return NULL;
	ctx = child->msg->ctx;
	if (child->msg->poison)
		return NULL;
	if (WARN_ON_ONCE(ctx->deepest != child)) {
		ctlv_message_output_poison(child, -EINVAL);
		return NULL;
	}
	return ctx;
}

/**
 * ctlv_record_end - Finish an output record.
 * @child: record cursor returned by a generated start helper
 */
void ctlv_record_end(struct ctlv_cursor *child)
{
	struct ctlv_op_ctx *ctx = ctlv_message_output_closing(child);

	if (!ctx)
		return;
	if (ctlv_message_output_hdr(child)->len == sizeof(struct ctlv_hdr))
		ctx->frag.progress = true;
	ctx->deepest = child->up;
}
EXPORT_SYMBOL_NS_GPL(ctlv_record_end, "CTLV");

/**
 * ctlv_record_cancel - Drop an output record and everything written into it.
 * @child: innermost open record cursor returned by a generated start helper
 *
 * For a query that ran out of room partway through a record: the fragment then
 * ends before it, and the next fragment writes it again.
 */
void ctlv_record_cancel(struct ctlv_cursor *child)
{
	struct ctlv_op_ctx *ctx = ctlv_message_output_closing(child);
	struct ctlv_cursor *open;
	u32 len;

	if (!ctx)
		return;
	/* The record lengthened each one holding it by exactly its length. */
	len = ctlv_message_output_hdr(child)->len;
	for (open = child->up; open; open = open->up)
		ctlv_message_output_hdr(open)->len -= len;
	ctx->frag.progress = child->progress;
	ctx->deepest = child->up;
}
EXPORT_SYMBOL_NS_GPL(ctlv_record_cancel, "CTLV");

static bool ctlv_op_error_valid(int error)
{
	return !WARN_ON_ONCE(error >= 0 || error < -MAX_ERRNO);
}

/**
 * ctlv_op_error - Record an operation error message.
 * @ctx: operation context
 * @error: negative error to return
 * @message: message copied into the operation result
 *
 * Return: @error, or -EINVAL if @error is invalid.
 */
int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message)
{
	if (!ctlv_op_error_valid(error))
		return error < 0 && error >= -MAX_ERRNO ? error : -EINVAL;
	if (!ctx->error.message[0])
		strscpy(ctx->error.message, message, sizeof(ctx->error.message));
	return error;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_error, "CTLV");

/**
 * ctlv_op_errorf - Format an operation error message.
 * @ctx: operation context
 * @error: negative error to return
 * @fmt: printf format and arguments
 *
 * Return: @error, or -EINVAL if @error is invalid.
 */
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...)
{
	va_list args;

	if (!ctlv_op_error_valid(error))
		return error < 0 && error >= -MAX_ERRNO ? error : -EINVAL;
	if (ctx->error.message[0])
		return error;
	va_start(args, fmt);
	vscnprintf(ctx->error.message, sizeof(ctx->error.message), fmt, args);
	va_end(args);
	return error;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_errorf, "CTLV");

/* Continuation is a query's, so asking about one anywhere else is a bug. */
static bool ctlv_op_is_query(const struct ctlv_op_ctx *ctx)
{
	return !WARN_ON_ONCE(ctx->op->schema->type != CTLV_OP_TYPE_QUERY);
}

/**
 * ctlv_op_query_generation - Get a query continuation generation.
 * @ctx: query operation context
 *
 * Return: generation supplied by the previous fragment, or zero initially.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx)
{
	if (!ctlv_op_is_query(ctx))
		return 0;
	return ctx->frag.generation;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_generation, "CTLV");

/**
 * ctlv_op_query_cursor - Get a query continuation cursor.
 * @ctx: query operation context
 *
 * Return: cursor supplied by the previous fragment, or zero initially.
 */
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx)
{
	if (!ctlv_op_is_query(ctx))
		return 0;
	return ctx->frag.cursor;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_cursor, "CTLV");

/*
 * What both endings check: a query, one ending, and a generation that has not
 * moved under a continuation, which no fragment can be delivered across.
 */
static int ctlv_query_fragment_end(struct ctlv_op_ctx *ctx, u64 generation)
{
	if (!ctlv_op_is_query(ctx))
		return -EINVAL;
	if (WARN_ON_ONCE(ctx->frag.answered))
		return -EINVAL;
	ctx->frag.answered = true;
	/* Whatever the builder gave up on has warned about it already. */
	if (ctx->output.poison)
		return ctx->output.poison;
	if (ctx->frag.cursor && generation != ctx->frag.generation) {
		ctx->error.reason = CTLV_VALIDATION_REASON_STALE_GENERATION;
		ctx->error.expected_generation = ctx->frag.generation;
		ctx->error.current_generation = generation;
		return -ESTALE;
	}
	ctx->frag.reported = generation;
	return 0;
}

/**
 * ctlv_op_query_more - Complete a query fragment with more data pending.
 * @ctx: query operation context
 * @generation: current data generation
 * @next_cursor: nonzero cursor for the next fragment
 *
 * Return: 0, or a negative error.
 */
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation, u64 next_cursor)
{
	int ret = ctlv_query_fragment_end(ctx, generation);

	if (!ret &&
	    WARN_ON_ONCE(!ctx->frag.progress || !next_cursor || next_cursor == ctx->frag.cursor))
		ret = -EINVAL;
	if (!ret) {
		ctlv_message_output_frag_mark(ctx);
		ctx->frag.next_cursor = next_cursor;
	}
	ctx->frag.result = ret;
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_more, "CTLV");

/**
 * ctlv_op_query_done - Complete the last query fragment.
 * @ctx: query operation context
 * @generation: current data generation
 *
 * Return: 0, or a negative error.
 */
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation)
{
	int ret = ctlv_query_fragment_end(ctx, generation);

	if (!ret && WARN_ON_ONCE(ctx->deepest != &ctx->output_root)) {
		/* A cursor of a frame that has returned is not followed. */
		ctx->deepest = &ctx->output_root;
		ret = -EINVAL;
	}
	ctx->frag.result = ret;
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_done, "CTLV");

static void ctlv_message_output_frag_mark(struct ctlv_op_ctx *ctx)
{
	struct ctlv_cursor *open;

	for (open = ctx->deepest; open; open = open->up)
		ctlv_message_output_hdr(open)->flags |= CTLV_HDR_F_FRAG_MORE;
	ctx->deepest = &ctx->output_root;
}
