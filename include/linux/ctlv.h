/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV in-kernel interface: the generated schema metadata a family compiles,
 * and the descriptor it registers. Nothing here is hand-written for a family;
 * it is generated from its specification under Documentation/ctlv/specs/.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/array_size.h>
#include <linux/gfp_types.h>
#include <linux/limits.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

/*
 * One short of the transport limit: op-schema-get describes a message with a
 * tree one level deeper, so a schema spending the whole limit is unreportable.
 */
#define CTLV_MAX_DECLARED_NEST_DEPTH (CTLV_MAX_NEST_DEPTH - 1)

/**
 * struct ctlv_schema_node - One node of a flattened op schema tree.
 * @attr_id: complete attr ID, or CTLV_ATTR_ID_ROOT in the root node
 * @kind: enum ctlv_schema_kind, selecting the live union member
 * @unit: enum ctlv_unit of an integer payload, CTLV_UNIT_NONE when absent
 * @scale: decimal exponent of an integer payload that declares a unit
 * @max_occurrences: occurrences allowed at this placement, zero for unlimited
 * @reenters: whether @first_child names a level this node is itself part of,
 *	which is what lets a message nest as deep as the transport allows
 * @first_child: index of the first child in the same node array
 * @n_children: children of this node, the required ones first
 * @n_required_children: how many of those children are required
 * @range: inclusive bounds of an integer or enum payload, raw 64-bit values
 *	interpreted according to @kind
 * @flags_mask: bits that a flags payload may set
 * @length: inclusive payload length bounds of a string or binary attr,
 *	excluding a string's NUL terminator
 * @blob: descriptor policy of a blob attr: maximum external payload length,
 *	required alignment, an enum ctlv_blob_direction, and a mask of
 *	BIT(enum ctlv_blob_type) backings
 *
 * One placement of one attr, so an attr placed in two ops is two nodes and may
 * differ in occurrences and requirement between them.
 */
struct ctlv_schema_node {
	u64 attr_id;
	u8 kind;
	u8 unit;
	s8 scale;
	bool reenters;
	u32 max_occurrences;
	u32 first_child;
	u32 n_children;
	u32 n_required_children;
	union {
		struct {
			u64 min;
			u64 max;
		} range;
		u64 flags_mask;
		struct {
			u32 min_len;
			u32 max_len;
		} length;
		struct {
			u64 max_len;
			u32 alignment;
			u16 backings;
			u8 direction;
		} blob;
	};
};

/**
 * struct ctlv_stub_ref - One extension point of one message.
 * @family_id: family that declared the attr set
 * @set_id: attr set within that family, which is a stub
 * @node: index of the node whose children the fill joins
 * @optional: whether every filled member is optional at this placement
 *
 * Resolved against the registered leaf; an unfilled stub is nothing.
 */
struct ctlv_stub_ref {
	u16 family_id;
	u32 set_id;
	u32 node;
	bool optional;
};

/**
 * struct ctlv_schema_tree - Flattened schema of one message.
 * @nodes: nodes in declaration order, root first, or NULL when the message
 *	does not exist
 * @n_nodes: number of nodes, including the root
 * @stubs: extension points selected by this message, or NULL
 * @n_stubs: number of entries in @stubs
 */
struct ctlv_schema_tree {
	const struct ctlv_schema_node *nodes;
	u32 n_nodes;
	const struct ctlv_stub_ref *stubs;
	u32 n_stubs;
};

#define CTLV_SCHEMA_TREE(_nodes) \
	{ .nodes = (_nodes), .n_nodes = ARRAY_SIZE(_nodes) }

#define CTLV_SCHEMA_TREE_STUBS(_nodes, _stubs)			\
	{ .nodes = (_nodes), .n_nodes = ARRAY_SIZE(_nodes),	\
	  .stubs = (_stubs), .n_stubs = ARRAY_SIZE(_stubs) }

/**
 * struct ctlv_stub_fill - The members one family puts into an inherited stub.
 * @family_id: family that declared the stub
 * @set_id: attr set within that family
 * @members: the fill, whose root stands for the stub, so its children are
 *	what splices into a message
 *
 * At most one family of a chain fills a stub.
 */
struct ctlv_stub_fill {
	u16 family_id;
	u32 set_id;
	struct ctlv_schema_tree members;
};

/**
 * struct ctlv_op_sizes - Generated message lengths of one op.
 * @request_max_len: longest valid request root including its header; zero for
 *	an event
 * @action_reply_max_len: longest action reply root including its header; zero
 *	for an ACK-only action
 * @query_reply_min_len: reply buffer length that is always enough to make
 *	progress, being the largest indivisible leaf plus the nests above it
 * @query_reply_suggested_len: reply buffer length a caller should offer
 * @event_record_max_len: longest complete event record, header and padded
 *	payload root included
 *
 * Bytes throughout; zero means the length does not apply to this op's type.
 */
struct ctlv_op_sizes {
	u32 request_max_len;
	u32 action_reply_max_len;
	u32 query_reply_min_len;
	u32 query_reply_suggested_len;
	u32 event_record_max_len;
};

/**
 * struct ctlv_op_schema - Generated metadata of one declared op.
 * @op_id: complete op ID
 * @type: what kind of op this is, an enum ctlv_op_type value
 * @availability: whether a family may enable and disable this op, an
 *	enum ctlv_op_availability value
 * @pre_post: whether this op has a hook pair of its own
 * @request: request schema; empty for an event
 * @reply: reply schema; empty for an ACK-only action
 * @sizes: generated lengths of this op, computed with every stub of it empty
 *
 * @pre_post is checked against a registered descriptor: a pair is mandatory
 * for an op that declared one and refused for an op that did not.
 *
 * The two enums are the root family's, from <uapi/linux/ctlv_gen.h>, and are
 * stored as bytes because that header includes this one.
 */
struct ctlv_op_schema {
	u64 op_id;
	u8 type;
	u8 availability;
	bool pre_post;
	struct ctlv_schema_tree request;
	struct ctlv_schema_tree reply;
	struct ctlv_op_sizes sizes;
};

/**
 * struct ctlv_family_schema - Generated metadata of one family.
 * @name: stable family name, which a device's uevents carry and the core's
 *	family dump reports
 * @family_id: registered family ID
 * @parent: family this one inherits, which is the core's own schema for a
 *	family that declares no parent, and NULL in the core's
 * @ops: ops this family declares, in declaration order, or NULL
 * @n_ops: number of entries in @ops
 * @fills: extension points this family fills for its ancestors, or NULL
 * @n_fills: number of entries in @fills
 *
 * One node of the schema graph, holding only what this family declares: an
 * inherited op or attr is stored once, by its own family, and reached through
 * @parent.
 *
 * The effective schema of a device is this graph read from its leaf towards the
 * core with each stub replaced by its fill. Nothing composes that into a second
 * representation.
 */
struct ctlv_family_schema {
	const char *name;
	u16 family_id;
	const struct ctlv_family_schema *parent;
	const struct ctlv_op_schema *ops;
	u32 n_ops;
	const struct ctlv_stub_fill *fills;
	u32 n_fills;
};

/*
 * Half of what a uevent environment allows, the device model's own additions
 * being unpredictable. Spelled out rather than taken from <linux/kobject.h>,
 * which nothing generated includes; drivers/ctlv/core.c checks they agree.
 */
#define CTLV_UEVENT_MAX_LEN 1024
#define CTLV_UEVENT_MAX_VARS 32

struct ctlv_msg;
struct ctlv_op_ctx;

/**
 * struct ctlv_cursor - One attr position in one message.
 * @msg: the message this position is in, core-private bookkeeping
 * @attr: the attr itself, or NULL when a lookup found nothing
 * @parent: reading, the nest containing @attr and where a further occurrence
 *	of a repeated attr is looked for, NULL at the root
 * @up: writing, cursor of the nest containing @attr, NULL at the root. The
 *	chain of them is the open-nest stack
 * @start: writing, where @attr begins in the message. An append reaches what
 *	it lengthens through the buffer rather than through @attr, that buffer
 *	being the core's own
 * @tree: schema tree the node belongs to, which is a fill's own tree for an
 *	attr that arrived through one
 * @node: index of the node within @tree
 * @depth: nesting levels above this position, the root being zero
 *
 * Fixed-size, so family code can put a child cursor on the stack. The members
 * are the framework's: a family reaches a message through what was generated
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
};

typedef int (*ctlv_action_fn)(struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);

typedef int (*ctlv_query_fn)(struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);

/*
 * The core calls, in order: the family's pre, the op's pre, the availability
 * check, the op's callback, the op's post, the family's post. Every pre that
 * succeeded has exactly one post, so a failing pre undoes its own work and
 * skips what is inside it. A pre may refuse the op and its errno becomes the
 * result; a post cannot change the answer, and runs even for an op the device
 * had disabled. Both may sleep, neither may keep @ctx.
 */
typedef int (*ctlv_op_pre_fn)(struct ctlv_op_ctx *ctx);
typedef void (*ctlv_op_post_fn)(struct ctlv_op_ctx *ctx);

/**
 * struct ctlv_op - What answers one op of a family's effective schema.
 * @action: implementation of an action op
 * @query: implementation of a query op
 * @pre: hook entered before this op alone, or NULL
 * @post: hook entered after this op alone, or NULL
 *
 * Function pointers and nothing else; everything else about the op is in the
 * schema of the family that declared it. A callback is mandatory even for an
 * op some device disables or leaves out, one descriptor serving every device of
 * the family. Events have no entry at all. @pre and @post are both set or both
 * NULL.
 */
struct ctlv_op {
	union {
		ctlv_action_fn action;
		ctlv_query_fn query;
	};
	ctlv_op_pre_fn pre;
	ctlv_op_post_fn post;
};

/**
 * struct ctlv_family - Generated descriptor of one registrable family.
 * @schema: this family's node of the schema graph, which is also its identity
 * @ops: one entry per effective action and query, in the order the core walks
 *	the chain: an ancestor's ops before this family's, and within a family
 *	its declaration order
 * @n_ops: number of entries in @ops
 * @pre: hook entered before every op this family answers
 * @post: hook entered after every op this family answers
 *
 * Passed to ctlv_device_alloc() and never constructed by hand. A child family
 * repeats none of its parent's metadata and still answers every op it
 * inherited: the parent's callbacks belong to the parent's devices.
 *
 * @pre and @post are mandatory, empty implementations included. Only the
 * registered family's pair runs; inheriting a schema does not inherit a hook.
 */
struct ctlv_family {
	const struct ctlv_family_schema *schema;
	const struct ctlv_op *ops;
	u32 n_ops;
	ctlv_op_pre_fn pre;
	ctlv_op_post_fn post;
};

struct ctlv_device;
struct device;

bool ctlv_family_is_a(const struct ctlv_family *family,
		      const struct ctlv_family_schema *ancestor);

struct ctlv_device *ctlv_device_alloc(struct device *parent_dev,
				      const struct ctlv_family *family,
				      void *priv);
int ctlv_device_register(struct ctlv_device *ctlvdev);
void ctlv_device_unregister(struct ctlv_device *ctlvdev);
void ctlv_device_put(struct ctlv_device *ctlvdev);

void ctlv_device_op_remove(struct ctlv_device *ctlvdev, u64 op_id);
void ctlv_device_op_state_set(struct ctlv_device *ctlvdev, u64 op_id,
			      bool disabled);

void *ctlv_op_priv(struct ctlv_op_ctx *ctx);

/*
 * What the getters, _present() helpers and iterators generated for a family's
 * attrs wrap. They take a complete attr ID and a kind, so a generated caller
 * that disagrees with its metadata is caught rather than misreading a payload.
 * Reading a value where there is none is kernel misuse: it warns once and
 * answers zero.
 */
void ctlv_attr_find(struct ctlv_cursor *cursor, const struct ctlv_cursor *nest,
		    u64 attr_id, u8 kind);
void ctlv_attr_next(struct ctlv_cursor *cursor);
bool ctlv_attr_ok(const struct ctlv_cursor *cursor);
u64 ctlv_attr_u64(const struct ctlv_cursor *cursor);
s64 ctlv_attr_s64(const struct ctlv_cursor *cursor);
bool ctlv_attr_bool(const struct ctlv_cursor *cursor);
const char *ctlv_attr_string(const struct ctlv_cursor *cursor);
const void *ctlv_attr_binary(const struct ctlv_cursor *cursor, u32 *len);

/*
 * The write half, wrapped by a family's generated serializers. An append is
 * atomic: true means the whole padded attr is in the message, false means the
 * message is unchanged, and which of the two reasons it was is not reported. A
 * query out of room answers the next fragment; an action or event out of room
 * exceeds its own schema, which warns and abandons the message. A nest is
 * appended to the innermost cursor still open.
 */
bool ctlv_put_u64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, u64 value);
bool ctlv_put_s64(struct ctlv_cursor *nest, u64 attr_id, u8 kind, s64 value);
bool ctlv_put_bool(struct ctlv_cursor *nest, u64 attr_id, u8 kind, bool value);
bool ctlv_put_string(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
		     const char *value);
bool ctlv_put_binary(struct ctlv_cursor *nest, u64 attr_id, u8 kind,
		     const void *value, u32 len);
bool ctlv_nest_start(struct ctlv_cursor *nest, struct ctlv_cursor *child,
		     u64 attr_id, u8 kind);
void ctlv_nest_end(struct ctlv_cursor *child);

/*
 * A caller with too little room for the whole reply repeats the op with the
 * same request, handing back the generation and cursor of the fragment before
 * it; the framework keeps nothing in between, so a callback resumes from what
 * it is told. Both are the family's to define: a cursor has only to be nonzero,
 * which is what says there is more, and not the one the fragment started from,
 * and a continuation whose generation changed is refused with -ESTALE.
 *
 * ctlv_op_query_more() marks the boundary through the nests still open, so
 * their cursors have to be alive when it is called. Both return the result of
 * the op, so a callback returns what they return.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx);
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx);
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor);
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation);

/*
 * A blob attr carries no payload: it describes memory of the caller's, which
 * the generated resolver acquires and ctlv_blob_put() releases, once per
 * resolve and before the callback returns. Direction is the schema's, so
 * ctlv_blob_write() refuses a blob declared input. Resolving fails for ordinary
 * reasons, so its errno is the caller's to see; resolving outside the callback
 * the request was handed to warns.
 */
struct ctlv_blob;

struct ctlv_blob *ctlv_blob_resolve(struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *blob);
void ctlv_blob_put(struct ctlv_blob *blob);
u64 ctlv_blob_len(const struct ctlv_blob *blob);
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len);
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len);

/*
 * A family builds an event instead of being called to answer one. The context
 * is the caller's between creating it and ending it, and exactly one of
 * ctlv_op_emit() and ctlv_op_abort() ends it. No hook pair runs around an
 * event. @gfp covers the whole of it, subscriber copies included, so an atomic
 * emitter passes GFP_NOWAIT or GFP_ATOMIC and lives with the drops. Delivery is
 * per open file description and a full queue drops a record rather than
 * waiting, so emitting succeeds whether or not anyone was listening.
 */
struct ctlv_op_ctx *ctlv_event_create(struct ctlv_device *ctlvdev, u64 op_id,
				      struct ctlv_cursor **event, gfp_t gfp);
int ctlv_op_emit(struct ctlv_op_ctx *ctx);
void ctlv_op_abort(struct ctlv_op_ctx *ctx);

#endif /* _LINUX_CTLV_H */
