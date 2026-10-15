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

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

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
	*msg->root = (struct ctlv_attr){
		.len = sizeof(*msg->root),
		.flags = CTLV_ATTR_F_NESTED,
		.attr_id = CTLV_ATTR_ROOT,
	};
}

/* The first thing that went wrong is what the op ends up failing with. */
static bool ctlv_message_output_poison(struct ctlv_cursor *nest, int error)
{
	if (!nest->msg->poison)
		nest->msg->poison = error;
	return false;
}

/* A poisoned message is refused silently: whatever poisoned it has warned. */
static bool ctlv_message_output_ready(struct ctlv_cursor *nest)
{
	if (WARN_ON_ONCE(!nest->msg || !nest->attr))
		return false;
	/* Also what refuses a cursor that reads a request. */
	if (WARN_ON_ONCE(!nest->msg->ctx))
		return false;
	if (nest->msg->poison)
		return false;
	if (WARN_ON_ONCE(nest->msg->ctx->deepest != nest))
		return ctlv_message_output_poison(nest, -EINVAL);
	return true;
}

/* What the schema places in this nest under one ID, as one kind. */
static const struct ctlv_node_schema *ctlv_message_output_member(struct ctlv_cursor *nest,
								 u64 attr_id, u8 kind,
								 struct ctlv_schema_pos *pos)
{
	const struct ctlv_node_schema *node;

	if (!ctlv_message_output_ready(nest))
		return NULL;
	if (WARN_ON_ONCE(!ctlv_schema_member(nest->msg->family, nest->tree,
					     nest->node, attr_id, pos))) {
		ctlv_message_output_poison(nest, -EINVAL);
		return NULL;
	}
	node = &pos->tree->node_schemas[pos->node];
	if (WARN_ON_ONCE(node->kind != kind)) {
		ctlv_message_output_poison(nest, -EINVAL);
		return NULL;
	}
	return node;
}

/*
 * A query is offered whatever room its caller had, so running out ends a
 * fragment. Anything else wrote a message its own schema does not describe.
 */
static bool ctlv_message_output_full(struct ctlv_cursor *nest)
{
	const struct ctlv_op_schema *op = nest->msg->ctx->op->schema;

	if (op->type == CTLV_OP_TYPE_QUERY)
		return false;
	WARN_ONCE(1, "ctlv: op %#018llx wrote past the reply length its schema declares\n",
		  op->op_id);
	return ctlv_message_output_poison(nest, -EMSGSIZE);
}

/* What one cursor stands on, as the buffer this direction may change. */
static struct ctlv_attr *ctlv_message_output_attr(const struct ctlv_cursor *nest)
{
	return (struct ctlv_attr *)((u8 *)nest->msg->root + nest->start);
}

/* The pad is written rather than left as it was: it belongs to the nest. */
static struct ctlv_attr *ctlv_message_output_write(struct ctlv_cursor *nest, u64 attr_id, u32 flags,
						   const void *payload, u32 payload_len)
{
	u32 len = sizeof(struct ctlv_attr) + payload_len;
	size_t step = ALIGN((size_t)len, CTLV_ALIGNTO);
	struct ctlv_msg *msg = nest->msg;
	struct ctlv_cursor *open;
	struct ctlv_attr *attr;
	size_t offset;

	offset = nest->start + ctlv_message_output_attr(nest)->len;
	if (offset + step > msg->len) {
		ctlv_message_output_full(nest);
		return NULL;
	}

	attr = (struct ctlv_attr *)((u8 *)msg->root + offset);
	*attr = (struct ctlv_attr){
		.len = len,
		.flags = flags,
		.attr_id = attr_id,
	};
	if (payload_len)
		memcpy(attr + 1, payload, payload_len);
	memset((u8 *)attr + len, 0, step - len);
	if (!(flags & CTLV_ATTR_F_NESTED))
		msg->ctx->frag.progress = true;

	/* Every nest the attr landed in is that much longer, the root too. */
	for (open = nest; open; open = open->up)
		ctlv_message_output_attr(open)->len += step;
	return attr;
}

/* The 64-bit payload that every integer, enum and flags attr carries. */
static bool ctlv_message_put_scalar(struct ctlv_cursor *nest, u64 attr_id, u8 kind, u64 value)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;

	node = ctlv_message_output_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	/* A value outside what the attr declares is one nothing may be told. */
	if (WARN_ON_ONCE(!ctlv_schema_value_ok(node, value)))
		return ctlv_message_output_poison(nest, -EINVAL);
	return ctlv_message_output_write(nest, attr_id, 0, &value, sizeof(value));
}

bool ctlv_put_u64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, u64 value)
{
	return ctlv_message_put_scalar(nest, attr_id, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_u64, "CTLV");

bool ctlv_put_s64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, s64 value)
{
	return ctlv_message_put_scalar(nest, attr_id, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_s64, "CTLV");

bool ctlv_put_bool(struct ctlv_cursor *nest, u64 attr_id, u8 kind, bool value)
{
	u8 payload = value;
	struct ctlv_schema_pos pos;

	if (!ctlv_message_output_member(nest, attr_id, kind, &pos))
		return false;
	return ctlv_message_output_write(nest, attr_id, 0, &payload, sizeof(payload));
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_bool, "CTLV");

bool ctlv_put_string(struct ctlv_cursor *nest, u64 attr_id, u8 kind, const char *value)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;
	size_t len;

	node = ctlv_message_output_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	len = strnlen(value, (size_t)node->length.max_len + 1);
	if (WARN_ON_ONCE(len > node->length.max_len || !ctlv_schema_length_ok(node, len)))
		return ctlv_message_output_poison(nest, -EINVAL);
	return ctlv_message_output_write(nest, attr_id, 0, value, len + 1);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_string, "CTLV");

bool ctlv_put_binary(struct ctlv_cursor *nest, u64 attr_id, u8 kind, const void *value, u32 len)
{
	const struct ctlv_node_schema *node;
	struct ctlv_schema_pos pos;

	node = ctlv_message_output_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	if (WARN_ON_ONCE(!ctlv_schema_length_ok(node, len)))
		return ctlv_message_output_poison(nest, -EINVAL);
	return ctlv_message_output_write(nest, attr_id, 0, value, len);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_binary, "CTLV");

bool ctlv_nest_start(struct ctlv_cursor *nest, struct ctlv_cursor *child, u64 attr_id, u8 kind)
{
	struct ctlv_schema_pos pos;
	struct ctlv_attr *attr;

	*child = (struct ctlv_cursor){};
	if (!ctlv_message_output_member(nest, attr_id, kind, &pos))
		return false;
	/* Unreachable through any schema the generator would have accepted. */
	if (WARN_ON_ONCE(nest->depth + 1 > CTLV_MAX_NEST_DEPTH))
		return ctlv_message_output_poison(nest, -EINVAL);

	attr = ctlv_message_output_write(nest, attr_id, CTLV_ATTR_F_NESTED, NULL, 0);
	if (!attr)
		return false;

	*child = (struct ctlv_cursor){
		.msg = nest->msg,
		.attr = attr,
		.up = nest,
		.start = ctlv_message_offset(nest->msg, attr),
		.tree = pos.tree,
		.node = pos.node,
		.depth = nest->depth + 1,
	};
	nest->msg->ctx->deepest = child;
	return true;
}
EXPORT_SYMBOL_NS_GPL(ctlv_nest_start, "CTLV");

void ctlv_nest_end(struct ctlv_cursor *child)
{
	struct ctlv_op_ctx *ctx;

	if (WARN_ON_ONCE(!child->msg || !child->msg->ctx || !child->up))
		return;
	ctx = child->msg->ctx;
	if (child->msg->poison)
		return;
	if (WARN_ON_ONCE(ctx->deepest != child)) {
		ctlv_message_output_poison(child, -EINVAL);
		return;
	}
	if (ctlv_message_output_attr(child)->len == sizeof(struct ctlv_attr))
		ctx->frag.progress = true;
	ctx->deepest = child->up;
}
EXPORT_SYMBOL_NS_GPL(ctlv_nest_end, "CTLV");

static bool ctlv_op_error_valid(int error)
{
	return !WARN_ON_ONCE(error >= 0 || error < -MAX_ERRNO);
}

int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message)
{
	if (!ctlv_op_error_valid(error))
		return error < 0 && error >= -MAX_ERRNO ? error : -EINVAL;
	if (!ctx->error.message[0])
		strscpy(ctx->error.message, message, sizeof(ctx->error.message));
	return error;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_error, "CTLV");

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

u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx)
{
	if (!ctlv_op_is_query(ctx))
		return 0;
	return ctx->frag.generation;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_generation, "CTLV");

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
		ctlv_message_output_attr(open)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctx->deepest = &ctx->output_root;
}
