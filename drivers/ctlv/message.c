// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Stepping through one message, and the reads a family's generated getters are
 * built out of.
 *
 * Every length in a request came from userspace, so one step through a nest is
 * where the wire format's containment and alignment rules are enforced, and the
 * only place a length becomes a pointer. Nothing checks alignment again: the
 * root begins an allocation and every step from it is aligned. Reads trust the
 * schema, validation having run, and check their caller instead.
 */
#include <kunit/visibility.h>
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/bug.h>
#include <linux/ctlv.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "ctlv_private.h"
#include "message.h"
#include "schema.h"

/**
 * ctlv_msg_step - The next child of one nest.
 * @nest: the containing attr, whose own length has been checked already
 * @child: the child to step from, NULL to start at the first one; set to the
 *	next child when one was found
 *
 * A nested payload is its children concatenated, each padded to CTLV_ALIGNTO,
 * so a step is the aligned length of the child it starts from.
 *
 * Return: what the step found, one of enum ctlv_step.
 */
enum ctlv_step ctlv_msg_step(const struct ctlv_attr *nest,
			     const struct ctlv_attr **child)
{
	const u8 *end = (const u8 *)nest + nest->len;
	const u8 *next;
	size_t room;

	if (nest->len < sizeof(*nest))
		return CTLV_STEP_BAD;
	if (*child)
		next = (const u8 *)*child +
		       ALIGN((size_t)(*child)->len, CTLV_ALIGNTO);
	else
		next = (const u8 *)(nest + 1);
	if (next >= end)
		return next == end ? CTLV_STEP_END : CTLV_STEP_BAD;

	room = end - next;
	if (room < sizeof(**child))
		return CTLV_STEP_BAD;
	*child = (const struct ctlv_attr *)next;
	/*
	 * Length before alignment, so that aligning a length this nest could
	 * never hold is not what overflows.
	 */
	if ((*child)->len < sizeof(**child) || (*child)->len > room)
		return CTLV_STEP_BAD;
	if (ALIGN((size_t)(*child)->len, CTLV_ALIGNTO) > room)
		return CTLV_STEP_BAD;
	return CTLV_STEP_ATTR;
}

/**
 * ctlv_msg_offset - Where one attr of a message begins.
 * @msg: the message
 * @attr: an attr within it
 *
 * Offsets outlive the snapshot the attr was found in, which pointers do not.
 *
 * Return: the byte offset of @attr from the message root.
 */
u32 ctlv_msg_offset(const struct ctlv_msg *msg, const struct ctlv_attr *attr)
{
	return (const u8 *)attr - (const u8 *)msg->root;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_msg_offset);

/**
 * ctlv_msg_root - The cursor addressing one message's root.
 * @cursor: cursor to initialize
 * @msg: the message
 */
void ctlv_msg_root(struct ctlv_cursor *cursor, struct ctlv_msg *msg)
{
	*cursor = (struct ctlv_cursor){
		.msg = msg,
		.attr = msg->root,
		.tree = msg->tree,
	};
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_msg_root);

/**
 * ctlv_msg_output - Begin one message in a buffer of the core's own.
 * @msg: the message to initialize
 * @ctx: the execution it is the output of
 * @buf: where to build it, aligned as an allocation is
 * @len: bytes that may be written from @buf
 * @tree: schema the message has to conform to
 */
void ctlv_msg_output(struct ctlv_msg *msg, struct ctlv_op_ctx *ctx, void *buf,
		     size_t len, const struct ctlv_schema_tree *tree)
{
	*msg = (struct ctlv_msg){
		.root = buf,
		.len = len,
		.tree = tree,
		.leaf = ctx->leaf,
		.ctx = ctx,
	};
	*msg->root = (struct ctlv_attr){
		.len = sizeof(*msg->root),
		.flags = CTLV_ATTR_F_NESTED,
		.attr_id = CTLV_ATTR_ID_ROOT,
	};
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_msg_output);

/* The first occurrence of one ID among the children of one nest. */
static const struct ctlv_attr *ctlv_attr_lookup(const struct ctlv_attr *nest,
						u64 attr_id)
{
	const struct ctlv_attr *child = NULL;

	while (ctlv_msg_step(nest, &child) == CTLV_STEP_ATTR)
		if (child->attr_id == attr_id)
			return child;
	return NULL;
}

/**
 * ctlv_attr_find - Position one cursor on one attr of one nest.
 * @cursor: cursor to fill in, which addresses nothing when the attr is absent
 * @nest: cursor addressing the nest to look in
 * @attr_id: complete attr ID of the member to find
 * @kind: enum ctlv_schema_kind the caller expects of it
 *
 * The first occurrence. A member the schema does not place in this nest, or
 * places as another kind, warns; an attr the caller left out does not, which
 * is what ctlv_attr_ok() distinguishes.
 */
void ctlv_attr_find(struct ctlv_cursor *cursor, const struct ctlv_cursor *nest,
		    u64 attr_id, u8 kind)
{
	struct ctlv_schema_pos member;

	*cursor = (struct ctlv_cursor){};
	if (WARN_ON_ONCE(!nest || !nest->attr || !nest->tree || !nest->msg))
		return;
	if (WARN_ON_ONCE(!ctlv_schema_member(nest->msg->leaf, nest->tree,
					     nest->node, attr_id, &member)))
		return;
	if (WARN_ON_ONCE(member.tree->nodes[member.node].kind != kind))
		return;

	*cursor = (struct ctlv_cursor){
		.msg = nest->msg,
		.parent = nest->attr,
		.attr = ctlv_attr_lookup(nest->attr, attr_id),
		.tree = member.tree,
		.node = member.node,
		.depth = nest->depth + 1,
	};
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_find, "CTLV");

/**
 * ctlv_attr_next - Move one cursor to the next occurrence of its attr.
 * @cursor: cursor from ctlv_attr_find(), which addresses nothing once the
 *	occurrences run out
 *
 * Only a repeated member has more than one, and only its iterator calls this.
 */
void ctlv_attr_next(struct ctlv_cursor *cursor)
{
	const struct ctlv_attr *child;
	u64 attr_id;

	if (WARN_ON_ONCE(!cursor || !cursor->attr || !cursor->parent))
		return;

	child = cursor->attr;
	attr_id = child->attr_id;
	cursor->attr = NULL;
	while (ctlv_msg_step(cursor->parent, &child) == CTLV_STEP_ATTR) {
		if (child->attr_id == attr_id) {
			cursor->attr = child;
			return;
		}
	}
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_next, "CTLV");

/**
 * ctlv_attr_ok - Whether one cursor addresses an attr at all.
 * @cursor: cursor to ask about
 *
 * What a generated _present() helper answers, and what ends an iterator.
 */
bool ctlv_attr_ok(const struct ctlv_cursor *cursor)
{
	return cursor && cursor->attr;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_ok, "CTLV");

/* The payload, checked against the kinds a reader of this C type accepts. */
static const void *ctlv_cursor_payload(const struct ctlv_cursor *cursor,
				       u32 kinds, u32 *len)
{
	const struct ctlv_schema_node *node;

	if (WARN_ON_ONCE(!cursor || !cursor->attr || !cursor->tree))
		return NULL;
	node = &cursor->tree->nodes[cursor->node];
	if (WARN_ON_ONCE(!(kinds & BIT(node->kind))))
		return NULL;
	*len = ctlv_attr_payload_len(cursor->attr);
	return ctlv_attr_payload(cursor->attr);
}

/* A length disagreeing with a validated kind is this core's mistake. */
static const void *ctlv_cursor_fixed(const struct ctlv_cursor *cursor,
				     u32 kinds, u32 expected)
{
	const void *payload;
	u32 len;

	payload = ctlv_cursor_payload(cursor, kinds, &len);
	if (!payload || WARN_ON_ONCE(len != expected))
		return NULL;
	return payload;
}

/**
 * ctlv_attr_u64 - The value of an unsigned integer, enum or flags attr.
 * @cursor: cursor addressing the attr
 *
 * Return: the payload, or zero if there is none to read.
 */
u64 ctlv_attr_u64(const struct ctlv_cursor *cursor)
{
	const u64 *value = ctlv_cursor_fixed(cursor,
					     BIT(CTLV_SCHEMA_UINT) |
					     BIT(CTLV_SCHEMA_ENUM) |
					     BIT(CTLV_SCHEMA_FLAGS),
					     sizeof(u64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_u64, "CTLV");

/**
 * ctlv_attr_s64 - The value of a signed integer attr.
 * @cursor: cursor addressing the attr
 *
 * Return: the payload, or zero if there is none to read.
 */
s64 ctlv_attr_s64(const struct ctlv_cursor *cursor)
{
	const s64 *value = ctlv_cursor_fixed(cursor, BIT(CTLV_SCHEMA_SINT),
					     sizeof(s64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_s64, "CTLV");

/**
 * ctlv_attr_bool - The value of a boolean attr.
 * @cursor: cursor addressing the attr
 *
 * An absent boolean and a present false one differ, and only _present() knows.
 *
 * Return: the payload, or false if there is none to read.
 */
bool ctlv_attr_bool(const struct ctlv_cursor *cursor)
{
	const u8 *value = ctlv_cursor_fixed(cursor, BIT(CTLV_SCHEMA_BOOL),
					    sizeof(u8));

	return value ? *value : false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_bool, "CTLV");

/**
 * ctlv_attr_string - The value of a string attr.
 * @cursor: cursor addressing the attr
 *
 * Validation proved the payload is one NUL-terminated string.
 *
 * Return: a pointer into the message, or NULL if there is no string to read.
 */
const char *ctlv_attr_string(const struct ctlv_cursor *cursor)
{
	const char *value;
	u32 len;

	value = ctlv_cursor_payload(cursor, BIT(CTLV_SCHEMA_STRING), &len);
	if (!value || WARN_ON_ONCE(!len || value[len - 1]))
		return NULL;
	return value;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_string, "CTLV");

/**
 * ctlv_attr_binary - The value of a binary attr.
 * @cursor: cursor addressing the attr
 * @len: where to store the payload length
 *
 * Return: a pointer into the message, or NULL if there is nothing to read, in
 * which case @len is zero.
 */
const void *ctlv_attr_binary(const struct ctlv_cursor *cursor, u32 *len)
{
	const void *value;

	*len = 0;
	value = ctlv_cursor_payload(cursor, BIT(CTLV_SCHEMA_BINARY), len);
	if (!value)
		*len = 0;
	return value;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_binary, "CTLV");

/*
 * The room is measured before anything is written, which is what leaves a
 * refused attr's message exactly as it was, padding included.
 */

/* The first thing that went wrong is what the op ends up failing with. */
static bool ctlv_out_poison(struct ctlv_cursor *nest, int error)
{
	if (!nest->msg->poison)
		nest->msg->poison = error;
	return false;
}

/* A poisoned message is refused silently: whatever poisoned it has warned. */
static bool ctlv_out_ready(struct ctlv_cursor *nest)
{
	if (WARN_ON_ONCE(!nest || !nest->msg || !nest->attr))
		return false;
	/* Also what refuses a cursor that reads a request. */
	if (WARN_ON_ONCE(!nest->msg->ctx))
		return false;
	if (nest->msg->poison)
		return false;
	if (WARN_ON_ONCE(nest->msg->ctx->deepest != nest))
		return ctlv_out_poison(nest, -EINVAL);
	return true;
}

/* What the schema places in this nest under one ID, as one kind. */
static const struct ctlv_schema_node *
ctlv_out_member(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
		struct ctlv_schema_pos *pos)
{
	const struct ctlv_schema_node *node;

	if (!ctlv_out_ready(nest))
		return NULL;
	if (WARN_ON_ONCE(!ctlv_schema_member(nest->msg->leaf, nest->tree,
					     nest->node, attr_id, pos))) {
		ctlv_out_poison(nest, -EINVAL);
		return NULL;
	}
	node = &pos->tree->nodes[pos->node];
	if (WARN_ON_ONCE(node->kind != kind)) {
		ctlv_out_poison(nest, -EINVAL);
		return NULL;
	}
	return node;
}

/*
 * A query is offered whatever room its caller had, so running out ends a
 * fragment. Anything else wrote a message its own schema does not describe.
 */
static bool ctlv_out_full(struct ctlv_cursor *nest)
{
	const struct ctlv_op_schema *op = nest->msg->ctx->op->schema;

	if (op->type == CTLV_OP_TYPE_QUERY)
		return false;
	WARN_ONCE(1, "ctlv: op %#018llx wrote past the reply length its schema declares\n",
		  op->op_id);
	return ctlv_out_poison(nest, -EMSGSIZE);
}

/* What one cursor stands on, as the buffer this direction may change. */
static struct ctlv_attr *ctlv_out_attr(const struct ctlv_cursor *nest)
{
	return (struct ctlv_attr *)((u8 *)nest->msg->root + nest->start);
}

/* The pad is written rather than left as it was: it belongs to the nest. */
static struct ctlv_attr *ctlv_out_write(struct ctlv_cursor *nest, u64 attr_id,
					u32 flags, const void *payload,
					u32 payload_len)
{
	u32 len = sizeof(struct ctlv_attr) + payload_len;
	size_t step = ALIGN((size_t)len, CTLV_ALIGNTO);
	struct ctlv_msg *msg = nest->msg;
	struct ctlv_cursor *open;
	struct ctlv_attr *attr;
	size_t offset;

	offset = nest->start + ctlv_out_attr(nest)->len;
	if (offset + step > msg->len) {
		ctlv_out_full(nest);
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

	/* Every nest the attr landed in is that much longer, the root too. */
	for (open = nest; open; open = open->up)
		ctlv_out_attr(open)->len += step;
	return attr;
}

/* The 64-bit payload that every integer, enum and flags attr carries. */
static bool ctlv_put_scalar(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
			    u64 value)
{
	const struct ctlv_schema_node *node;
	struct ctlv_schema_pos pos;

	node = ctlv_out_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	/* A value outside what the attr declares is one nothing may be told. */
	if (WARN_ON_ONCE(!ctlv_schema_value_ok(node, value)))
		return ctlv_out_poison(nest, -EINVAL);
	return ctlv_out_write(nest, attr_id, 0, &value, sizeof(value));
}

/**
 * ctlv_put_u64 - Append an unsigned integer, enum or flags attr.
 * @nest: cursor of the nest to append to
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 * @value: what to write
 *
 * Return: whether the attr is in the message.
 */
bool ctlv_put_u64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, u64 value)
{
	return ctlv_put_scalar(nest, attr_id, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_u64, "CTLV");

/**
 * ctlv_put_s64 - Append a signed integer attr.
 * @nest: cursor of the nest to append to
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 * @value: what to write
 *
 * Return: whether the attr is in the message.
 */
bool ctlv_put_s64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, s64 value)
{
	return ctlv_put_scalar(nest, attr_id, kind, value);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_s64, "CTLV");

/**
 * ctlv_put_bool - Append a boolean attr.
 * @nest: cursor of the nest to append to
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 * @value: what to write
 *
 * One payload byte, whose presence is the difference a reader can see.
 *
 * Return: whether the attr is in the message.
 */
bool ctlv_put_bool(struct ctlv_cursor *nest, u64 attr_id, u8 kind, bool value)
{
	u8 payload = value;
	struct ctlv_schema_pos pos;

	if (!ctlv_out_member(nest, attr_id, kind, &pos))
		return false;
	return ctlv_out_write(nest, attr_id, 0, &payload, sizeof(payload));
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_bool, "CTLV");

/**
 * ctlv_put_string - Append a NUL-terminated string attr.
 * @nest: cursor of the nest to append to
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 * @value: the string, whose terminator is part of what is written
 *
 * The declared bounds exclude the terminator, so a string is measured no
 * further than one byte past the longest allowed.
 *
 * Return: whether the attr is in the message.
 */
bool ctlv_put_string(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
		     const char *value)
{
	const struct ctlv_schema_node *node;
	struct ctlv_schema_pos pos;
	size_t len;

	node = ctlv_out_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	if (WARN_ON_ONCE(!value))
		return ctlv_out_poison(nest, -EINVAL);
	len = strnlen(value, (size_t)node->length.max_len + 1);
	if (WARN_ON_ONCE(len > node->length.max_len ||
			 !ctlv_schema_length_ok(node, len)))
		return ctlv_out_poison(nest, -EINVAL);
	return ctlv_out_write(nest, attr_id, 0, value, len + 1);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_string, "CTLV");

/**
 * ctlv_put_binary - Append an opaque attr.
 * @nest: cursor of the nest to append to
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 * @value: the bytes to write
 * @len: how many of them, which the attr's declared bounds have to allow
 *
 * Return: whether the attr is in the message.
 */
bool ctlv_put_binary(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
		     const void *value, u32 len)
{
	const struct ctlv_schema_node *node;
	struct ctlv_schema_pos pos;

	node = ctlv_out_member(nest, attr_id, kind, &pos);
	if (!node)
		return false;
	if (WARN_ON_ONCE(!value && len) ||
	    WARN_ON_ONCE(!ctlv_schema_length_ok(node, len)))
		return ctlv_out_poison(nest, -EINVAL);
	return ctlv_out_write(nest, attr_id, 0, value, len);
}
EXPORT_SYMBOL_NS_GPL(ctlv_put_binary, "CTLV");

/**
 * ctlv_nest_start - Open one nested attr and stand on it.
 * @nest: cursor of the nest to append to
 * @child: cursor to initialize, which every member of the new nest is written
 *	through and which ctlv_nest_end() closes
 * @attr_id: complete attr ID of the member
 * @kind: enum ctlv_schema_kind the caller expects of it
 *
 * A nest is appended empty and grows as its members are written, so the room it
 * needs is a header's. Nothing may be written to an outer nest until this one
 * is closed. @child addresses nothing when this fails, so ending a nest that
 * was never opened is caught.
 *
 * Return: whether the nest is in the message.
 */
bool ctlv_nest_start(struct ctlv_cursor *nest, struct ctlv_cursor *child,
		     u64 attr_id, u8 kind)
{
	struct ctlv_schema_pos pos;
	struct ctlv_attr *attr;

	*child = (struct ctlv_cursor){};
	if (!ctlv_out_member(nest, attr_id, kind, &pos))
		return false;
	/* Unreachable through any schema the generator would have accepted. */
	if (WARN_ON_ONCE(nest->depth + 1 > CTLV_MAX_NEST_DEPTH))
		return ctlv_out_poison(nest, -EINVAL);

	attr = ctlv_out_write(nest, attr_id, CTLV_ATTR_F_NESTED, NULL, 0);
	if (!attr)
		return false;

	*child = (struct ctlv_cursor){
		.msg = nest->msg,
		.attr = attr,
		.up = nest,
		.start = ctlv_msg_offset(nest->msg, attr),
		.tree = pos.tree,
		.node = pos.node,
		.depth = nest->depth + 1,
	};
	nest->msg->ctx->deepest = child;
	return true;
}
EXPORT_SYMBOL_NS_GPL(ctlv_nest_start, "CTLV");

/**
 * ctlv_nest_end - Close the nest one cursor stands on.
 * @child: cursor from ctlv_nest_start()
 *
 * Adds nothing to the message: every member already added its length to this
 * nest. Ending anything but the innermost open nest is misuse.
 */
void ctlv_nest_end(struct ctlv_cursor *child)
{
	struct ctlv_op_ctx *ctx;

	if (WARN_ON_ONCE(!child || !child->msg || !child->msg->ctx ||
			 !child->up))
		return;
	ctx = child->msg->ctx;
	if (child->msg->poison)
		return;
	if (WARN_ON_ONCE(ctx->deepest != child)) {
		ctlv_out_poison(child, -EINVAL);
		return;
	}
	ctx->deepest = child->up;
}
EXPORT_SYMBOL_NS_GPL(ctlv_nest_end, "CTLV");

/**
 * ctlv_out_frag_mark - Say that an output message ends where a fragment does.
 * @ctx: execution whose output message stops here
 *
 * The mark goes on every nest still open, and a reader takes it as the promise
 * that a continuation re-emits this path. The tracking is reset with it,
 * because a cursor of the frame that opened a nest is not one to follow again.
 */
void ctlv_out_frag_mark(struct ctlv_op_ctx *ctx)
{
	struct ctlv_cursor *open;

	for (open = ctx->deepest; open; open = open->up)
		ctlv_out_attr(open)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctx->deepest = &ctx->output_root;
}
