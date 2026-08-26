/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV in-kernel interface for generated schema metadata, family descriptors,
 * and device callbacks.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/array_size.h>
#include <linux/cache.h>
#include <linux/container_of.h>
#include <linux/gfp_types.h>
#include <linux/limits.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

/*
 * One short of the transport limit: op-schema-get describes a message with a
 * tree one level deeper, so a schema spending the whole limit is unreportable.
 */
#define CTLV_MAX_DECLARED_NEST_DEPTH (CTLV_MAX_NEST_DEPTH - 1)

/* error.message excludes its terminating NUL. */
#define CTLV_ERROR_MESSAGE_LEN 256

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
 * Resolved against the device family; an unfilled stub is nothing.
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
 * @request_max_len: longest request made only of schema-known attrs, including
 *	its root header; zero for an event
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
 * @fd_access: enum ctlv_op_fd_access required to execute an action/query
 * @availability: whether a family may enable and disable this op, an
 *	enum ctlv_op_availability value
 * @optional: whether family init must opt in before publication to have this op
 * @pre_post: whether this op has a hook pair of its own
 * @request: request schema; empty for an event
 * @reply: reply schema; empty for an ACK-only action
 * @sizes: generated lengths of this op, computed with every stub of it empty
 *
 * @pre_post is checked against a family descriptor during device creation: a
 * pair is mandatory for an op that declared one and refused for an op that did
 * not.
 *
 * The two enums are the root family's, from <uapi/linux/ctlv_gen.h>, and are
 * stored as bytes because that header includes this one.
 */
struct ctlv_op_schema {
	u64 op_id;
	u8 type;
	u8 fd_access;
	u8 availability;
	bool optional;
	bool pre_post;
	struct ctlv_schema_tree request;
	struct ctlv_schema_tree reply;
	struct ctlv_op_sizes sizes;
};

/**
 * struct ctlv_family_schema - Generated metadata of one family.
 * @name: stable family name, which a device's uevents carry and the core's
 *	family dump reports
 * @family_id: permanent family ID
 * @ops: ops this family declares, in declaration order, or NULL
 * @n_ops: number of entries in @ops
 * @fills: extension points this family fills for its ancestors, or NULL
 * @n_fills: number of entries in @fills
 *
 * Holds only what this family declares. The family descriptor links this to
 * its ancestors; an inherited op or attr remains stored only by its owner.
 */
struct ctlv_family_schema {
	const char *name;
	u16 family_id;
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

struct ctlv_device;
struct ctlv_msg;
struct ctlv_op_ctx;

/**
 * struct ctlv_input_ref - Generated binding of one request attr placement.
 * @tree: schema tree containing the attr
 * @attr_id: complete attr ID
 * @node: containing nest node within @tree
 * @slot: dense local slot within @node
 * @kind: enum ctlv_schema_kind expected at @slot
 */
struct ctlv_input_ref {
	const struct ctlv_schema_tree *tree;
	u64 attr_id;
	u32 node;
	u32 slot;
	u8 kind;
};

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
	u32 index_nest_scope;
	u32 index_scope;
};

typedef int (*ctlv_action_fn)(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);

typedef int (*ctlv_query_fn)(void *priv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);

/*
 * The core calls, in order: the device family pre, the declaring op's pre,
 * the availability check, its callback and post, then the device family
 * post. Core ops use the core family instead. Every successful pre has exactly
 * one post. Both may sleep, neither may keep @ctx.
 */
typedef int (*ctlv_op_pre_fn)(void *priv, struct ctlv_op_ctx *ctx);
typedef void (*ctlv_op_post_fn)(void *priv, struct ctlv_op_ctx *ctx);

/**
 * struct ctlv_op - Owner-local implementation of one declared op.
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

/*
 * Every family priv is aligned this far. An over-aligned object is allocated
 * by family init and referenced from its core-allocated priv.
 */
#define CTLV_PRIV_ALIGN SMP_CACHE_BYTES

/* Core-owned links immediately preceding every family priv. */
struct ctlv_priv_container {
	struct ctlv_priv_container *parent;
	struct ctlv_priv_container *child;
	u8 priv[] __aligned(CTLV_PRIV_ALIGN);
};

static inline const struct ctlv_priv_container *
__ctlv_priv_container(const void *priv)
{
	return container_of_const(priv, struct ctlv_priv_container, priv);
}

typedef size_t (*ctlv_family_priv_size_fn)(void);
typedef const void *(*ctlv_family_pre_init_fn)(const void *init_info);
typedef int (*ctlv_family_init_fn)(struct ctlv_device *ctlvdev,
				   const void *init_info, void *priv);
typedef int (*ctlv_family_post_init_fn)(struct ctlv_device *ctlvdev,
					void *priv);
typedef void (*ctlv_family_pre_fini_fn)(struct ctlv_device *ctlvdev,
					void *priv);
typedef void (*ctlv_family_fini_fn)(struct ctlv_device *ctlvdev, void *priv);

/**
 * ctlv_priv_parent - Direct parent family's priv.
 * @priv: core-allocated priv passed to a family callback
 *
 * Return: the parent's priv, or NULL for the root family.
 */
static inline void *ctlv_priv_parent(const void *priv)
{
	const struct ctlv_priv_container *container =
		__ctlv_priv_container(priv);

	return container->parent ? container->parent->priv : NULL;
}

/**
 * ctlv_priv_child - Direct child family's priv on this device.
 * @priv: core-allocated priv passed to a family callback
 *
 * During this family's init the child is not initialized yet; it is by the
 * time post_init and pre_fini run. During this family's fini the child has
 * already been finalized.
 *
 * Return: the child's priv, or NULL for the device family.
 */
static inline void *ctlv_priv_child(const void *priv)
{
	const struct ctlv_priv_container *container =
		__ctlv_priv_container(priv);

	return container->child ? container->child->priv : NULL;
}

/*
 * The two halves of one extension point, entered from the implementation that
 * owns the message rather than from the wire. @request is the same nest of the
 * request being answered, or NULL when the message being built answers none.
 */
typedef int (*ctlv_stub_fill_fn)(void *priv, struct ctlv_op_ctx *ctx,
				 const struct ctlv_cursor *request,
				 struct ctlv_cursor *nest);
typedef void (*ctlv_stub_read_fn)(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *nest);

/**
 * struct ctlv_stub_ops - What one family does with an extension point it fills.
 * @family_id: family that declared the stub
 * @set_id: attr set within that family, which is the stub
 * @fill: appends this family's members to a nest an ancestor opened
 * @read: reads them out of a nest an ancestor was handed
 *
 * One entry per fill of this family's schema, in the same order, so the two
 * describe one extension point together.
 */
struct ctlv_stub_ops {
	u16 family_id;
	u32 set_id;
	ctlv_stub_fill_fn fill;
	ctlv_stub_read_fn read;
};

/**
 * struct ctlv_family - Generated descriptor of one family.
 * @schema: this family's owner-local schema
 * @parent: descriptor of the direct parent, NULL only in the root family
 * @ops: one entry per action and query this family declares, in declaration
 *	order
 * @n_ops: number of entries in @ops
 * @pre: outer hook for every action/query when this is the device family, or
 *	for every core action/query in the root descriptor
 * @post: matching outer hook after the op
 * @priv_size: size of this family's priv
 * @pre_init: optionally translates this family's init info into its parent's
 * @init: initializes the core-allocated priv
 * @post_init: optionally completes initialization or exposes the provider to
 *	kernel consumers after every priv is ready and event admission starts
 * @pre_fini: optionally withdraws and drains kernel consumers while all privs
 *	and event emission remain available
 * @fini: releases resources held by the priv
 * @stubs: what this family puts into and takes out of each extension point its
 *	schema fills, or NULL
 * @n_stubs: number of entries in @stubs
 *
 * Passed to ctlv_create() and never constructed by hand. A family binds
 * the ops it declared and no others: an inherited op is answered through
 * @parent, by the driver that wrote it, and that driver's module cannot unload
 * while a module inheriting it is loaded.
 *
 * @pre and @post are mandatory, empty implementations included. They wrap
 * every effective non-core op when this descriptor is the device family;
 * ancestor family-wide pairs do not stack.
 */
struct ctlv_family {
	const struct ctlv_family_schema *schema;
	const struct ctlv_family *parent;
	const struct ctlv_op *ops;
	u32 n_ops;
	ctlv_op_pre_fn pre;
	ctlv_op_post_fn post;
	ctlv_family_priv_size_fn priv_size;
	ctlv_family_pre_init_fn pre_init;
	ctlv_family_init_fn init;
	ctlv_family_post_init_fn post_init;
	ctlv_family_pre_fini_fn pre_fini;
	ctlv_family_fini_fn fini;
	const struct ctlv_stub_ops *stubs;
	u32 n_stubs;
};

extern const struct ctlv_family ctlv_ctlv_family;

struct device;

bool ctlv_family_is_a(const struct ctlv_family *family,
		      const struct ctlv_family *ancestor);
const struct ctlv_family *
ctlv_device_family(const struct ctlv_device *ctlvdev);
struct device *ctlv_device_parent(const struct ctlv_device *ctlvdev);

struct ctlv_device *ctlv_create(struct device *parent_dev,
				const struct ctlv_family *family,
				const void *init_info);
void ctlv_destroy(struct ctlv_device *ctlvdev);

void ctlv_device_op_provide(struct ctlv_device *ctlvdev, u64 op_id);
void ctlv_device_op_state_set(struct ctlv_device *ctlvdev, u64 op_id,
			      bool disabled);

/*
 * What generated request readers wrap. A placement reference binds its schema,
 * local slot, attr ID and kind. Reading a value where there is none is kernel
 * misuse: it warns once and answers zero.
 */
struct ctlv_cursor *
ctlv_input_attr(struct ctlv_cursor *cursor, const struct ctlv_cursor *nest,
		struct ctlv_input_ref ref);
bool ctlv_input_cursor(const struct ctlv_cursor *cursor,
		       struct ctlv_input_ref ref);
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
 * An extension point is declared by one family and filled by another, so the
 * implementation that owns the message hands the nest standing for it to
 * whichever family of this device's chain filled it. Both take that nest and
 * nothing about the family on the other side; an extension point nothing filled
 * is nothing to do. ctlv_stub_fill() returns what the fill returned, which an
 * implementation answers with.
 */
int ctlv_stub_fill(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		   struct ctlv_cursor *nest);
void ctlv_stub_read(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *nest);

/*
 * Return @error after recording the first family diagnostic for this
 * execution. The message is copied and may be temporary.
 */
int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message);
__printf(3, 4)
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...);

/*
 * A caller with too little room for the whole reply repeats the op with the
 * same request, handing back the generation and cursor of the fragment before
 * it; the framework keeps nothing in between, so a callback resumes from what
 * it is told. Both are the family's to define: a cursor has to be nonzero,
 * which is what says there is more, and not the one the fragment started from.
 * A continuing fragment must also complete a logical leaf. A continuation
 * whose generation changed is refused with -ESTALE.
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
 *
 * Calls for one blob are serialized in the callback's task. A hardware family
 * may ask to map it without inspecting its backing; non-dma-buf backing returns
 * -EOPNOTSUPP. @dma_skip bytes of the borrowed, immutable whole-buffer mapping
 * precede the blob. Direction is inferred from the schema. After preparing its
 * resources and unsignaled non-container completion fence, the family calls
 * begin, attempts to queue the work, then calls submit_done with the result. A
 * submission error signals the fence immediately; otherwise hardware signals
 * it after stopping. Unmap then tears down the mapping. One op may hold one
 * device mapping at a time.
 */
struct ctlv_blob;
struct ctlv_blob_dma;
struct dma_fence;
struct sg_table;

struct ctlv_blob *ctlv_blob_resolve(struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *blob);
int ctlv_blob_put(struct ctlv_blob *blob);
u64 ctlv_blob_len(const struct ctlv_blob *blob);
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len);
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len);
struct ctlv_blob_dma *ctlv_blob_dma_map(struct ctlv_blob *blob,
					struct device *dev,
					struct sg_table **sgt, u64 *dma_skip);
int ctlv_blob_dma_begin(struct ctlv_blob_dma *dma, struct dma_fence *fence);
void ctlv_blob_dma_submit_done(struct ctlv_blob_dma *dma, int status);
void ctlv_blob_dma_unmap(struct ctlv_blob_dma *dma, int status);

/*
 * A family builds an event instead of being called to answer one. The context
 * is the caller's between creating it and ending it, and exactly one of
 * ctlv_op_emit() and ctlv_op_abort() ends it. No hook pair runs around an
 * event. @gfp covers the whole of it, subscriber copies included, so an atomic
 * emitter passes GFP_NOWAIT or GFP_ATOMIC and lives with the drops. Delivery is
 * per open file description and a full queue drops a record rather than
 * waiting, so emitting succeeds whether or not anyone was listening.
 *
 * When an event occurred but its context could not be allocated,
 * ctlv_event_lost() records the missing notification without allocation or
 * sleeping. The caller keeps the device alive and orders this report against
 * normal emission. It must not report subscriber-copy or queue drops already
 * accounted for by ctlv_op_emit(), or call it from interrupt context.
 */
struct ctlv_op_ctx *ctlv_event_create(struct ctlv_device *ctlvdev, u64 op_id,
				      struct ctlv_cursor **event, gfp_t gfp);
void ctlv_event_lost(struct ctlv_device *ctlvdev, u64 op_id);
int ctlv_op_emit(struct ctlv_op_ctx *ctx);
void ctlv_op_abort(struct ctlv_op_ctx *ctx);

#endif /* _LINUX_CTLV_H */
