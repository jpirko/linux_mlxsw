// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Reading the schema graph of one leaf: the ops it answers, the members that
 * fill its inherited extension points, its message bounds, and whether one
 * message it was sent is one of them.
 *
 * Generated metadata describes one family each and is never copied downwards,
 * so everything effective is a walk from a leaf towards the root. What a walk
 * leaves behind is pointers into the graph, never a copy of it.
 */
#include <kunit/visibility.h>
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/ctlv.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <linux/overflow.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "blob.h"
#include "ctlv_private.h"
#include "message.h"
#include "schema.h"

/* Every 64-bit payload, and every blob descriptor member, is this long. */
#define CTLV_SCALAR_ATTR_LEN (sizeof(struct ctlv_attr) + sizeof(u64))

/*
 * Where composing a tree's maximum length gives up, which keeps the check on a
 * composed length from being passed by a total that wrapped.
 */
#define CTLV_MAX_COMPOSED_LEN CTLV_MAX_EVENT_QUEUE_LEN

/**
 * ctlv_schema_ancestor - The family a number of levels above one leaf.
 * @leaf: family to count from
 * @up: levels to climb, zero being @leaf itself
 *
 * Return: the family, or NULL when the chain is shorter than that.
 */
const struct ctlv_family_schema *
ctlv_schema_ancestor(const struct ctlv_family_schema *leaf, u32 up)
{
	while (up-- && leaf)
		leaf = leaf->parent;
	return leaf;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_ancestor);

/**
 * ctlv_schema_levels - How many families one chain has.
 * @leaf: family to count from
 *
 * Return: the number of families from @leaf up to and including the root.
 */
u32 ctlv_schema_levels(const struct ctlv_family_schema *leaf)
{
	const struct ctlv_family_schema *schema;
	u32 levels = 0;

	for (schema = leaf; schema; schema = schema->parent)
		levels++;
	return levels;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_levels);

/**
 * ctlv_schema_is_a - Whether one chain contains one family.
 * @schema: leaf to walk from
 * @ancestor: family to look for, @schema itself counting as one
 */
bool ctlv_schema_is_a(const struct ctlv_family_schema *schema,
		      const struct ctlv_family_schema *ancestor)
{
	for (; schema; schema = schema->parent)
		if (schema == ancestor)
			return true;
	return false;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_is_a);

/**
 * ctlv_schema_cursor_init - Start visiting the effective ops of one leaf.
 * @cursor: cursor to initialize
 * @leaf: family whose effective schema to walk
 *
 * The walk runs from the root of @leaf's chain down to @leaf, each family in
 * declaration order, which is the order a generated binding table is in.
 */
void ctlv_schema_cursor_init(struct ctlv_schema_cursor *cursor,
			     const struct ctlv_family_schema *leaf)
{
	cursor->leaf = leaf;
	cursor->level = ctlv_schema_levels(leaf);
	cursor->family = NULL;
	cursor->index = 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_cursor_init);

/**
 * ctlv_schema_next_op - The next effective op, with the family that owns it.
 * @cursor: cursor from ctlv_schema_cursor_init()
 * @family: where to store the family that declares the returned op
 *
 * Return: the next op schema, or NULL once the whole chain has been visited.
 */
const struct ctlv_op_schema *
ctlv_schema_next_op(struct ctlv_schema_cursor *cursor,
		    const struct ctlv_family_schema **family)
{
	while (cursor->level) {
		if (!cursor->family)
			cursor->family = ctlv_schema_ancestor(cursor->leaf,
							      cursor->level - 1);
		if (cursor->family && cursor->index < cursor->family->n_ops) {
			*family = cursor->family;
			return &cursor->family->ops[cursor->index++];
		}
		cursor->level--;
		cursor->family = NULL;
		cursor->index = 0;
	}
	return NULL;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_next_op);

/**
 * ctlv_schema_member - The member of one nest that carries one attr ID.
 * @leaf: family the device answers as, which resolves the extension points
 * @tree: tree the nest belongs to
 * @node: index of the nest within @tree
 * @attr_id: complete attr ID to look for
 * @member: where to store what was found
 *
 * Return: whether the nest may carry that attr.
 */
bool ctlv_schema_member(const struct ctlv_family_schema *leaf,
			const struct ctlv_schema_tree *tree, u32 node,
			u64 attr_id, struct ctlv_schema_pos *member)
{
	const struct ctlv_schema_node *nest;
	u32 index;

	if (!tree->nodes || node >= tree->n_nodes)
		return false;
	nest = &tree->nodes[node];

	for (index = 0; index < nest->n_children; index++) {
		u32 child = nest->first_child + index;

		if (tree->nodes[child].attr_id == attr_id) {
			*member = (struct ctlv_schema_pos){
				.tree = tree,
				.node = child,
			};
			return true;
		}
	}
	for (index = 0; index < tree->n_stubs; index++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[index];
		const struct ctlv_stub_fill *fill;

		if (ref->node != node)
			continue;
		fill = ctlv_schema_fill(leaf, ref->family_id, ref->set_id);
		/* Members of a filled stub are children of the fill's root. */
		if (fill && ctlv_schema_member(leaf, &fill->members, 0,
					       attr_id, member))
			return true;
	}
	return false;
}

/**
 * ctlv_schema_fill - The members one leaf puts into one extension point.
 * @leaf: family whose chain to search
 * @family_id: family that declared the stub
 * @set_id: attr set within that family
 *
 * A stub is filled by at most one family of a chain, so the first fill found
 * on the way from @leaf towards the core is the fill.
 *
 * Return: the fill, or NULL when nothing in this chain filled the stub.
 */
const struct ctlv_stub_fill *
ctlv_schema_fill(const struct ctlv_family_schema *leaf, u16 family_id,
		 u32 set_id)
{
	const struct ctlv_family_schema *schema;
	u32 index;

	for (schema = leaf; schema; schema = schema->parent)
		for (index = 0; index < schema->n_fills; index++)
			if (schema->fills[index].family_id == family_id &&
			    schema->fills[index].set_id == set_id)
				return &schema->fills[index];
	return NULL;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_fill);

/*
 * The same arithmetic the generator did, over the tree it emitted plus this
 * leaf's fills, since filling an extension point makes it longer.
 */

struct ctlv_length_walk {
	const struct ctlv_family_schema *leaf;
	bool bad;
};

static u64 ctlv_children_max_len(struct ctlv_length_walk *walk,
				 const struct ctlv_schema_tree *tree,
				 u32 index, u32 depth);

static u64 ctlv_payload_max_len(struct ctlv_length_walk *walk,
				const struct ctlv_schema_tree *tree,
				u32 index, u32 depth)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_SINT:
	case CTLV_SCHEMA_ENUM:
	case CTLV_SCHEMA_FLAGS:
		return sizeof(u64);
	case CTLV_SCHEMA_BOOL:
		return 1;
	case CTLV_SCHEMA_STRING:
		/*
		 * One terminating NUL, which the declared maximum excludes.
		 * Widened first so that a maximum at U32_MAX cannot wrap.
		 */
		return (u64)node->length.max_len + 1;
	case CTLV_SCHEMA_BINARY:
		return node->length.max_len;
	case CTLV_SCHEMA_BLOB:
		/*
		 * Type, length and an address; anything but a bare VA is one
		 * member wider, an fd with an optional offset.
		 */
		return (node->blob.backings == BIT(CTLV_BLOB_TYPE_VA) ?
			3 : 4) * ALIGN(CTLV_SCALAR_ATTR_LEN, CTLV_ALIGNTO);
	default:
		return ctlv_children_max_len(walk, tree, index, depth);
	}
}

static u64 ctlv_attr_max_len(struct ctlv_length_walk *walk,
			     const struct ctlv_schema_tree *tree, u32 index,
			     u32 depth)
{
	return sizeof(struct ctlv_attr) +
	       ctlv_payload_max_len(walk, tree, index, depth);
}

static u64 ctlv_children_max_len(struct ctlv_length_walk *walk,
				 const struct ctlv_schema_tree *tree,
				 u32 index, u32 depth)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];
	u64 total = 0;
	u32 i;

	if (depth > CTLV_MAX_DECLARED_NEST_DEPTH) {
		walk->bad = true;
		return 0;
	}
	for (i = 0; i < node->n_children; i++) {
		u32 child = node->first_child + i;
		u64 each, all;

		/*
		 * Neither an unlimited placement nor one that re-enters has a
		 * maximum, and only a query reply may hold either.
		 */
		if (tree->nodes[child].reenters ||
		    !tree->nodes[child].max_occurrences) {
			walk->bad = true;
			return 0;
		}
		each = ALIGN(ctlv_attr_max_len(walk, tree, child, depth + 1),
			     CTLV_ALIGNTO);
		if (check_mul_overflow((u64)tree->nodes[child].max_occurrences,
				       each, &all) ||
		    check_add_overflow(total, all, &total) ||
		    total > CTLV_MAX_COMPOSED_LEN) {
			walk->bad = true;
			return 0;
		}
	}
	for (i = 0; i < tree->n_stubs; i++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[i];
		const struct ctlv_stub_fill *fill;

		if (ref->node != index)
			continue;
		fill = ctlv_schema_fill(walk->leaf, ref->family_id,
					ref->set_id);
		if (!fill)
			continue;
		if (check_add_overflow(total,
				       ctlv_children_max_len(walk,
							     &fill->members, 0,
							     depth),
				       &total) ||
		    total > CTLV_MAX_COMPOSED_LEN) {
			walk->bad = true;
			return 0;
		}
	}
	return total;
}

/* Buffer length that the largest indivisible leaf and its nests need. */
static u64 ctlv_min_len(struct ctlv_length_walk *walk,
			const struct ctlv_schema_tree *tree, u32 index,
			u32 above, u32 depth)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];
	bool has_children = false;
	u64 deepest = 0;
	u32 i;

	if (depth > CTLV_MAX_DECLARED_NEST_DEPTH) {
		walk->bad = true;
		return 0;
	}
	for (i = 0; i < node->n_children; i++) {
		u32 child = node->first_child + i;

		has_children = true;
		deepest = max(deepest,
			      ctlv_min_len(walk, tree, child,
					   above + sizeof(struct ctlv_attr),
					   depth + 1));
	}
	for (i = 0; i < tree->n_stubs; i++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[i];
		const struct ctlv_stub_fill *fill;
		const struct ctlv_schema_node *root;
		u32 member;

		if (ref->node != index)
			continue;
		fill = ctlv_schema_fill(walk->leaf, ref->family_id,
					ref->set_id);
		if (!fill)
			continue;
		root = &fill->members.nodes[0];
		for (member = 0; member < root->n_children; member++) {
			has_children = true;
			deepest = max(deepest,
				      ctlv_min_len(walk, &fill->members,
						   root->first_child + member,
						   above +
						   sizeof(struct ctlv_attr),
						   depth + 1));
		}
	}
	if (has_children)
		return deepest;
	/* An empty root is a message of its own header alone. */
	if (!index)
		return above + sizeof(struct ctlv_attr);
	return above + ALIGN(ctlv_attr_max_len(walk, tree, index, depth),
			     CTLV_ALIGNTO);
}

static u64 ctlv_root_max_len(struct ctlv_length_walk *walk,
			     const struct ctlv_schema_tree *tree)
{
	return sizeof(struct ctlv_attr) +
	       ctlv_children_max_len(walk, tree, 0, 0);
}

/*
 * A re-entering level repeats, so any leaf of the tree can sit at the depth
 * limit: one fragment needs the root's header, one per nest below it, and the
 * longest placement that cannot be split.
 */
static u64 ctlv_reentering_min_len(struct ctlv_length_walk *walk,
				   const struct ctlv_schema_tree *tree)
{
	u64 leaf = 0;
	u32 index;

	for (index = 1; index < tree->n_nodes; index++) {
		if (tree->nodes[index].kind == CTLV_SCHEMA_NEST)
			continue;
		leaf = max(leaf, ALIGN(ctlv_attr_max_len(walk, tree, index, 0),
				       CTLV_ALIGNTO));
	}
	return (CTLV_MAX_NEST_DEPTH + 1) * sizeof(struct ctlv_attr) + leaf;
}

static bool ctlv_tree_reenters(const struct ctlv_schema_tree *tree)
{
	u32 index;

	for (index = 0; index < tree->n_nodes; index++)
		if (tree->nodes[index].reenters)
			return true;
	return false;
}

/**
 * ctlv_schema_op_sizes - The lengths one op needs on one leaf.
 * @leaf: family the device answers as
 * @op: op of @leaf's effective schema
 * @sizes: where to store the composed lengths
 *
 * Generated lengths leave every extension point empty, since a descendant may
 * fill one. These are the composed lengths messages are admitted by.
 *
 * Return: 0, or -EINVAL when a composed length is one this framework could
 * never carry.
 */
int ctlv_schema_op_sizes(const struct ctlv_family_schema *leaf,
			 const struct ctlv_op_schema *op,
			 struct ctlv_op_sizes *sizes)
{
	struct ctlv_length_walk walk = { .leaf = leaf };
	u64 request = 0, reply = 0, record = 0;

	if (op->request.nodes)
		request = ctlv_root_max_len(&walk, &op->request);

	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
		if (op->reply.nodes)
			reply = ctlv_root_max_len(&walk, &op->reply);
		break;
	case CTLV_OP_TYPE_QUERY:
		if (ctlv_tree_reenters(&op->reply))
			reply = ctlv_reentering_min_len(&walk, &op->reply);
		else
			reply = ctlv_min_len(&walk, &op->reply, 0, 0, 0);
		break;
	case CTLV_OP_TYPE_EVENT:
		record = sizeof(struct ctlv_event_hdr) +
			 ALIGN(ctlv_root_max_len(&walk, &op->reply),
			       CTLV_ALIGNTO);
		break;
	default:
		return -EINVAL;
	}

	if (walk.bad || request > CTLV_MAX_INLINE_MESSAGE_LEN ||
	    reply > CTLV_MAX_INLINE_MESSAGE_LEN ||
	    record > CTLV_MAX_EVENT_QUEUE_LEN)
		return -EINVAL;

	*sizes = (struct ctlv_op_sizes){ .request_max_len = request };
	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
		sizes->action_reply_max_len = reply;
		break;
	case CTLV_OP_TYPE_QUERY:
		sizes->query_reply_min_len = reply;
		/*
		 * What the specification asked for, unless a filled extension
		 * point made one fragment longer than that.
		 */
		sizes->query_reply_suggested_len =
			max_t(u32, op->sizes.query_reply_suggested_len, reply);
		break;
	default:
		sizes->event_record_max_len = record;
		break;
	}
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_op_sizes);

/*
 * Only a completely valid request reaches family code; nothing is repaired or
 * normalized. The first failure is the one recorded, and the walk stops there.
 */

struct ctlv_validate {
	const struct ctlv_msg *msg;
	struct ctlv_error *error;
};

/* A request is never a fragment: only a reply of a query is ever incomplete. */
#define CTLV_ATTR_F_REQUEST CTLV_ATTR_F_NESTED

/*
 * Record the first failure and refuse the message. @attr is NULL when what is
 * wrong with the nest is something it does not carry.
 */
static int ctlv_bad(struct ctlv_validate *v, u32 reason,
		    const struct ctlv_attr *nest, const struct ctlv_attr *attr)
{
	*v->error = (struct ctlv_error){
		.reason = reason,
		.bad_attr_offset = ctlv_msg_offset(v->msg, attr ?: nest),
		.nest_offset = ctlv_msg_offset(v->msg, nest),
	};
	return -EINVAL;
}

/* The one failure that names an attr the message does not contain. */
static int ctlv_bad_missing(struct ctlv_validate *v,
			    const struct ctlv_attr *nest, u64 attr_id)
{
	ctlv_bad(v, CTLV_VALIDATION_REASON_MISSING_ATTR, nest, NULL);
	v->error->missing_attr_id = attr_id;
	return -EINVAL;
}

static u64 ctlv_scalar_value(const struct ctlv_attr *attr)
{
	/* Aligned by the wire format: the root began an allocation. */
	return *(const u64 *)ctlv_attr_payload(attr);
}

/**
 * ctlv_schema_value_ok - Whether one node may carry one scalar value.
 * @node: the schema node
 * @value: the payload, as the raw bits its kind interprets
 *
 * The one place the rule lives: a value from userspace and one a family hands
 * a serializer are checked against the same declaration.
 */
bool ctlv_schema_value_ok(const struct ctlv_schema_node *node, u64 value)
{
	switch (node->kind) {
	case CTLV_SCHEMA_SINT:
		return (s64)value >= (s64)node->range.min &&
		       (s64)value <= (s64)node->range.max;
	case CTLV_SCHEMA_FLAGS:
		/* A bit no family declared is one nothing can be told about. */
		return !(value & ~node->flags_mask);
	case CTLV_SCHEMA_BOOL:
		return value <= 1;
	default:
		/*
		 * An enum's values are numbered from one in declaration order
		 * and never skipped, so its range bounds it like an integer.
		 */
		return value >= node->range.min && value <= node->range.max;
	}
}

/**
 * ctlv_schema_length_ok - Whether one node may carry a payload this long.
 * @node: the schema node, a string or a binary attr
 * @len: the payload length, a string's terminator excluded
 */
bool ctlv_schema_length_ok(const struct ctlv_schema_node *node, u32 len)
{
	return len >= node->length.min_len && len <= node->length.max_len;
}

static int ctlv_check_integer(struct ctlv_validate *v,
			      const struct ctlv_schema_node *node,
			      const struct ctlv_attr *nest,
			      const struct ctlv_attr *attr)
{
	u64 value;

	if (ctlv_attr_payload_len(attr) != sizeof(value))
		return ctlv_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, nest,
				attr);
	value = ctlv_scalar_value(attr);
	if (ctlv_schema_value_ok(node, value))
		return 0;
	if (node->kind == CTLV_SCHEMA_FLAGS)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_FLAG, nest,
				attr);
	return ctlv_bad(v, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE, nest,
			attr);
}

static int ctlv_check_bool(struct ctlv_validate *v,
			   const struct ctlv_schema_node *node,
			   const struct ctlv_attr *nest,
			   const struct ctlv_attr *attr)
{
	const u8 *value = ctlv_attr_payload(attr);

	if (ctlv_attr_payload_len(attr) != sizeof(*value))
		return ctlv_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, nest,
				attr);
	if (!ctlv_schema_value_ok(node, *value))
		return ctlv_bad(v, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE,
				nest, attr);
	return 0;
}

static int ctlv_check_string(struct ctlv_validate *v,
			     const struct ctlv_schema_node *node,
			     const struct ctlv_attr *nest,
			     const struct ctlv_attr *attr)
{
	u32 len = ctlv_attr_payload_len(attr);
	const char *value = ctlv_attr_payload(attr);

	/*
	 * One terminating NUL and no other, so a family may hand the payload
	 * to anything that takes a C string.
	 */
	if (!len || value[len - 1] || strnlen(value, len) != len - 1)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_BAD_STRING, nest,
				attr);
	if (!ctlv_schema_length_ok(node, len - 1))
		return ctlv_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, nest,
				attr);
	return 0;
}

static int ctlv_check_binary(struct ctlv_validate *v,
			     const struct ctlv_schema_node *node,
			     const struct ctlv_attr *nest,
			     const struct ctlv_attr *attr)
{
	if (!ctlv_schema_length_ok(node, ctlv_attr_payload_len(attr)))
		return ctlv_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, nest,
				attr);
	return 0;
}

/*
 * The one shape the tree cannot describe: a nest of framework-owned attrs
 * whose members depend on its type. Nothing here acquires anything.
 */
static int ctlv_check_blob(struct ctlv_validate *v,
			   const struct ctlv_schema_node *node,
			   const struct ctlv_attr *blob)
{
	const struct ctlv_attr *bad = NULL;
	struct ctlv_blob_desc desc;
	u32 reason;

	reason = ctlv_blob_desc_read(blob, &desc, &bad);
	if (!reason)
		reason = ctlv_blob_desc_check(&desc, node);
	if (reason)
		return ctlv_bad(v, reason, blob, bad);
	return 0;
}

static int ctlv_check_nest(struct ctlv_validate *v,
			   const struct ctlv_schema_tree *tree, u32 node,
			   const struct ctlv_attr *nest, u32 depth);

static int ctlv_check_attr(struct ctlv_validate *v,
			   const struct ctlv_schema_pos *pos,
			   const struct ctlv_attr *nest,
			   const struct ctlv_attr *attr, u32 depth)
{
	const struct ctlv_schema_node *node = &pos->tree->nodes[pos->node];
	bool nested = node->kind == CTLV_SCHEMA_NEST ||
		      node->kind == CTLV_SCHEMA_BLOB;

	if (attr->flags & ~CTLV_ATTR_F_REQUEST)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_FLAG, nest,
				attr);
	/* Whether an attr is a nest is the schema's to say, not the sender's. */
	if (!(attr->flags & CTLV_ATTR_F_NESTED) != !nested)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE,
				nest, attr);

	switch (node->kind) {
	case CTLV_SCHEMA_BOOL:
		return ctlv_check_bool(v, node, nest, attr);
	case CTLV_SCHEMA_STRING:
		return ctlv_check_string(v, node, nest, attr);
	case CTLV_SCHEMA_BINARY:
		return ctlv_check_binary(v, node, nest, attr);
	case CTLV_SCHEMA_BLOB:
		return ctlv_check_blob(v, node, attr);
	case CTLV_SCHEMA_NEST:
		return ctlv_check_nest(v, pos->tree, pos->node, attr,
				       depth + 1);
	default:
		return ctlv_check_integer(v, node, nest, attr);
	}
}

/* How many times one member occurs, and where the one too many is. */
static u32 ctlv_occurrences(const struct ctlv_attr *nest, u64 attr_id, u32 max,
			    const struct ctlv_attr **excess)
{
	const struct ctlv_attr *child = NULL;
	u32 count = 0;

	while (ctlv_msg_step(nest, &child) == CTLV_STEP_ATTR) {
		if (child->attr_id != attr_id)
			continue;
		count++;
		if (max && count == max + 1)
			*excess = child;
	}
	return count;
}

/*
 * An optional stub makes everything it was filled with optional too: what a
 * descendant added cannot be something an ancestor's caller had to send.
 */
static int ctlv_check_members(struct ctlv_validate *v,
			      const struct ctlv_schema_tree *tree, u32 node,
			      const struct ctlv_attr *nest, bool optional)
{
	const struct ctlv_schema_node *parent = &tree->nodes[node];
	u32 index;

	for (index = 0; index < parent->n_children; index++) {
		const struct ctlv_attr *excess = NULL;
		const struct ctlv_schema_node *child;
		u32 count;

		child = &tree->nodes[parent->first_child + index];
		count = ctlv_occurrences(nest, child->attr_id,
					 child->max_occurrences, &excess);
		if (!count) {
			if (index < parent->n_required_children && !optional)
				return ctlv_bad_missing(v, nest,
							child->attr_id);
			continue;
		}
		/* An unlimited placement, which only a reply may hold. */
		if (child->max_occurrences && count > child->max_occurrences)
			return ctlv_bad(v,
					CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES,
					nest, excess);
	}

	for (index = 0; index < tree->n_stubs; index++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[index];
		const struct ctlv_stub_fill *fill;
		int ret;

		if (ref->node != node)
			continue;
		fill = ctlv_schema_fill(v->msg->leaf, ref->family_id,
					ref->set_id);
		if (!fill)
			continue;
		ret = ctlv_check_members(v, &fill->members, 0, nest,
					 optional || ref->optional);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * Required members first, then every attr in the order the nest carries them,
 * so that what is reported is the first thing a caller has to fix.
 */
static int ctlv_check_nest(struct ctlv_validate *v,
			   const struct ctlv_schema_tree *tree, u32 node,
			   const struct ctlv_attr *nest, u32 depth)
{
	const struct ctlv_attr *child = NULL;
	enum ctlv_step step;
	int ret;

	if (depth > CTLV_MAX_NEST_DEPTH)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_NESTING_TOO_DEEP,
				nest, NULL);

	ret = ctlv_check_members(v, tree, node, nest, false);
	if (ret)
		return ret;

	while ((step = ctlv_msg_step(nest, &child)) == CTLV_STEP_ATTR) {
		struct ctlv_schema_pos pos;

		if (!ctlv_schema_member(v->msg->leaf, tree, node,
					child->attr_id, &pos))
			return ctlv_bad(v,
					CTLV_VALIDATION_REASON_UNKNOWN_ATTR,
					nest, child);
		ret = ctlv_check_attr(v, &pos, nest, child, depth);
		if (ret)
			return ret;
	}
	if (step == CTLV_STEP_BAD)
		return ctlv_bad(v, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE,
				nest, NULL);
	return 0;
}

/**
 * ctlv_schema_validate - Check one message against the schema of its op.
 * @msg: the message, whose root has been framed already
 * @error: where to record what failed first
 *
 * Over exactly the bytes the root claims and none of what follows. A message
 * this accepts is one every generated getter can read without checking.
 *
 * Return: 0, or -EINVAL with @error describing the first failure.
 */
int ctlv_schema_validate(const struct ctlv_msg *msg, struct ctlv_error *error)
{
	struct ctlv_validate validate = { .msg = msg, .error = error };

	return ctlv_check_nest(&validate, msg->tree, 0, msg->root, 0);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_schema_validate);
