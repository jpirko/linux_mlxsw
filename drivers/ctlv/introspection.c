// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * What a device answers about itself: the families of its chain, the ops it
 * published, and the request and reply ABI of one of them. All three are
 * ordinary queries of the root family, answered from the same generated graph
 * validation and serialization read. Only the chain carries names; a schema is
 * numbers. Each can end a fragment mid-record, so the cursor of every nest one
 * leaves open lives in the frame that ends the fragment.
 */

#include <linux/bitmap.h>
#include <linux/bits.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "ctlv_family_gen.h"
#include "ctlv_private.h"
#include "schema.h"

/*
 * The buffer a dump promises to fit is one leaf and the nests above it, not a
 * whole record, so a cursor says which record comes next and how many of its
 * members were already delivered.
 */
#define CTLV_DUMP_MEMBER_SHIFT 48
#define CTLV_DUMP_RECORD_MASK GENMASK_ULL(CTLV_DUMP_MEMBER_SHIFT - 1, 0)

struct ctlv_dump_pos {
	u64 record;
	u32 member;
};

static void ctlv_dump_resume(struct ctlv_dump_pos *pos, u64 cursor)
{
	pos->record = cursor & CTLV_DUMP_RECORD_MASK;
	pos->member = cursor >> CTLV_DUMP_MEMBER_SHIFT;
}

static u64 ctlv_dump_cursor(const struct ctlv_dump_pos *pos)
{
	return pos->record | (u64)pos->member << CTLV_DUMP_MEMBER_SHIFT;
}

/*
 * One record per family, the root first and the registered family last. The
 * chain is immutable for the device's lifetime, which is why every fragment
 * reports generation zero.
 */
static bool ctlv_family_record(struct ctlv_cursor *reply,
			       struct ctlv_cursor *entry,
			       const struct ctlv_family_schema *schema,
			       struct ctlv_dump_pos *pos)
{
	if (!ctlv_family_entry_start(reply, entry))
		return false;
	if (pos->member == 0) {
		if (!ctlv_family_id_put(entry, schema->family_id))
			return false;
		pos->member = 1;
	}
	if (pos->member == 1) {
		if (!ctlv_family_name_put(entry, schema->name))
			return false;
		pos->member = 2;
	}
	ctlv_nest_end(entry);
	pos->member = 0;
	return true;
}

int ctlv_families_dump(struct ctlv_op_ctx *ctx,
		       const struct ctlv_cursor *request,
		       struct ctlv_cursor *reply)
{
	const struct ctlv_family_schema *leaf = ctx->leaf;
	u32 levels = ctlv_schema_levels(leaf);
	struct ctlv_cursor entry;
	struct ctlv_dump_pos pos;

	ctlv_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	for (; pos.record < levels; pos.record++) {
		const struct ctlv_family_schema *schema =
			ctlv_schema_ancestor(leaf, levels - 1 - pos.record);

		if (!ctlv_family_record(reply, &entry, schema, &pos))
			return ctlv_op_query_more(ctx, 0,
						  ctlv_dump_cursor(&pos));
	}
	return ctlv_op_query_done(ctx, 0);
}

/*
 * One entry per op of the published index: an op removed before the device
 * registered has no entry, a disabled one has an entry saying so. The bits and
 * the generation are read together, once per fragment, so no entry describes a
 * device no single moment saw.
 */
struct ctlv_ops_snapshot {
	unsigned long *disabled;
	u64 generation;
};

static int ctlv_ops_snapshot_take(struct ctlv_device *ctlvdev,
				  struct ctlv_ops_snapshot *snapshot)
{
	if (ctlvdev->n_dynamic_ops) {
		snapshot->disabled = bitmap_zalloc(ctlvdev->n_dynamic_ops,
						   GFP_KERNEL);
		if (!snapshot->disabled)
			return -ENOMEM;
	}
	guard(spinlock)(&ctlvdev->transition_lock);
	snapshot->generation = ctlvdev->state_generation;
	if (snapshot->disabled)
		bitmap_copy(snapshot->disabled, ctlvdev->disabled,
			    ctlvdev->n_dynamic_ops);
	return 0;
}

static bool ctlv_ops_snapshot_disabled(const struct ctlv_ops_snapshot *snapshot,
				       const struct ctlv_op_entry *op)
{
	if (op->state_index == CTLV_OP_STATE_NONE)
		return false;
	return test_bit(op->state_index, snapshot->disabled);
}

/*
 * The members of one entry, in the order a fragment writes them. The lengths
 * are this device's own, a descendant's fill included, because they are what
 * the core admits messages by.
 */
enum ctlv_op_member {
	CTLV_OP_MEMBER_ID,
	CTLV_OP_MEMBER_TYPE,
	CTLV_OP_MEMBER_AVAILABILITY,
	CTLV_OP_MEMBER_DISABLED,
	CTLV_OP_MEMBER_REQUEST_MAX_LEN,
	CTLV_OP_MEMBER_ACTION_REPLY_MAX_LEN,
	CTLV_OP_MEMBER_QUERY_REPLY_MIN_LEN,
	CTLV_OP_MEMBER_QUERY_REPLY_SUGGESTED_LEN,
	CTLV_OP_MEMBER_EVENT_RECORD_MAX_LEN,
	CTLV_OP_MEMBER_END,
};

static bool ctlv_op_member(struct ctlv_cursor *entry,
			   const struct ctlv_ops_snapshot *snapshot,
			   const struct ctlv_op_entry *op, u32 member)
{
	const struct ctlv_op_sizes *sizes = &op->sizes;

	switch (member) {
	case CTLV_OP_MEMBER_ID:
		return ctlv_op_id_put(entry, op->schema->op_id);
	case CTLV_OP_MEMBER_TYPE:
		return ctlv_op_type_put(entry, op->schema->type);
	case CTLV_OP_MEMBER_AVAILABILITY:
		return ctlv_op_availability_put(entry,
						op->schema->availability);
	case CTLV_OP_MEMBER_DISABLED:
		return ctlv_op_disabled_put(entry,
			ctlv_ops_snapshot_disabled(snapshot, op));
	case CTLV_OP_MEMBER_REQUEST_MAX_LEN:
		return ctlv_op_request_max_len_put(entry,
						   sizes->request_max_len);
	case CTLV_OP_MEMBER_ACTION_REPLY_MAX_LEN:
		/* Zero is a reply that acknowledges and no more. */
		return ctlv_op_action_reply_max_len_put(entry,
					sizes->action_reply_max_len);
	case CTLV_OP_MEMBER_QUERY_REPLY_MIN_LEN:
		return ctlv_op_query_reply_min_len_put(entry,
					sizes->query_reply_min_len);
	case CTLV_OP_MEMBER_QUERY_REPLY_SUGGESTED_LEN:
		return ctlv_op_query_reply_suggested_len_put(entry,
					sizes->query_reply_suggested_len);
	default:
		return ctlv_op_event_record_max_len_put(entry,
					sizes->event_record_max_len);
	}
}

static bool ctlv_op_member_applies(const struct ctlv_op_entry *op, u32 member)
{
	switch (member) {
	case CTLV_OP_MEMBER_ID:
	case CTLV_OP_MEMBER_TYPE:
		return true;
	case CTLV_OP_MEMBER_AVAILABILITY:
	case CTLV_OP_MEMBER_DISABLED:
	case CTLV_OP_MEMBER_REQUEST_MAX_LEN:
		return op->schema->type != CTLV_OP_TYPE_EVENT;
	case CTLV_OP_MEMBER_ACTION_REPLY_MAX_LEN:
		return op->schema->type == CTLV_OP_TYPE_ACTION;
	case CTLV_OP_MEMBER_EVENT_RECORD_MAX_LEN:
		return op->schema->type == CTLV_OP_TYPE_EVENT;
	default:
		return op->schema->type == CTLV_OP_TYPE_QUERY;
	}
}

static bool ctlv_op_record(struct ctlv_cursor *reply,
			   struct ctlv_cursor *entry,
			   const struct ctlv_ops_snapshot *snapshot,
			   const struct ctlv_op_entry *op,
			   struct ctlv_dump_pos *pos)
{
	if (!ctlv_op_entry_start(reply, entry))
		return false;
	for (; pos->member < CTLV_OP_MEMBER_END; pos->member++) {
		if (!ctlv_op_member_applies(op, pos->member))
			continue;
		if (!ctlv_op_member(entry, snapshot, op, pos->member))
			return false;
	}
	ctlv_nest_end(entry);
	pos->member = 0;
	return true;
}

int ctlv_ops_dump(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		  struct ctlv_cursor *reply)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_ops_snapshot snapshot = {};
	struct ctlv_cursor entry;
	struct ctlv_dump_pos pos;
	int ret;

	ret = ctlv_ops_snapshot_take(ctlvdev, &snapshot);
	if (ret)
		return ret;
	unsigned long *disabled __free(bitmap) = snapshot.disabled;

	/* The published index is immutable, so a cursor into it holds. */
	ctlv_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	for (; pos.record < ctlvdev->n_ops; pos.record++)
		if (!ctlv_op_record(reply, &entry, &snapshot,
				    &ctlvdev->ops[pos.record], &pos))
			return ctlv_op_query_more(ctx, snapshot.generation,
						  ctlv_dump_cursor(&pos));
	return ctlv_op_query_done(ctx, snapshot.generation);
}

/*
 * A description of a message is a message: one nest per placement, holding the
 * numbers that say what may appear there. The tree walked is the effective one,
 * so a descendant's fill is described as a member of the nest it filled. Where
 * a fragment stopped is a count of leaves, the nests above a leaf not being
 * positions of their own, which places a boundary anywhere in a tree of any
 * shape.
 */

/* One nest of the description: the placement of one node of the schema. */
struct ctlv_intro_frame {
	struct ctlv_cursor nest;
	u64 attr_id;
	bool open;
	/* Which node this stands for, so that a re-entry can be measured. */
	const struct ctlv_schema_tree *tree;
	u32 node;
};

/* The frames outlive the walk, a fragment's open nests having to stay alive. */
struct ctlv_intro_walk {
	const struct ctlv_family_schema *leaf;
	struct ctlv_cursor *root;
	struct ctlv_intro_frame *frames;
	u64 ordinal;
	u64 resume;
	int error;
};

/* One member of one node, which is every leaf this reply has. */
struct ctlv_intro_field {
	u64 attr_id;
	u8 kind;
	u64 value;
};

/* Three every node has, the widest bounds, a unit, a scale, a re-entry. */
#define CTLV_INTRO_MAX_FIELDS 10

struct ctlv_intro_fields {
	struct ctlv_intro_field field[CTLV_INTRO_MAX_FIELDS];
	u32 n;
};

static void ctlv_intro_field(struct ctlv_intro_fields *fields, u64 attr_id,
			     u8 kind, u64 value)
{
	if (WARN_ON_ONCE(fields->n == CTLV_INTRO_MAX_FIELDS))
		return;
	fields->field[fields->n++] = (struct ctlv_intro_field){
		.attr_id = attr_id,
		.kind = kind,
		.value = value,
	};
}

static void ctlv_intro_uint(struct ctlv_intro_fields *fields, u64 attr_id,
			    u64 value)
{
	ctlv_intro_field(fields, attr_id, CTLV_SCHEMA_UINT, value);
}

/*
 * What one node says about itself: where it may appear, the form its payload
 * takes, and the bounds of that form and of no other.
 */
static void ctlv_intro_node_fields(struct ctlv_intro_fields *fields,
				   const struct ctlv_schema_node *node,
				   u32 reenters)
{
	ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_ATTR_ID, node->attr_id);
	ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_KIND, node->kind);
	ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
			node->max_occurrences);

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_SINT:
	case CTLV_SCHEMA_ENUM:
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_MIN, node->range.min);
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_MAX, node->range.max);
		break;
	case CTLV_SCHEMA_FLAGS:
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_FLAGS_MASK,
				node->flags_mask);
		break;
	case CTLV_SCHEMA_STRING:
	case CTLV_SCHEMA_BINARY:
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_MIN_LENGTH,
				node->length.min_len);
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_MAX_LENGTH,
				node->length.max_len);
		break;
	case CTLV_SCHEMA_BLOB:
		ctlv_intro_field(fields, CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
				 CTLV_SCHEMA_ENUM, node->blob.direction);
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
				node->blob.backings);
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
				node->blob.max_len);
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
				node->blob.alignment);
		break;
	default:
		break;
	}

	if (node->unit != CTLV_UNIT_NONE) {
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_UNIT, node->unit);
		ctlv_intro_field(fields, CTLV_ATTR_SCHEMA_SCALE,
				 CTLV_SCHEMA_SINT, node->scale);
	}
	if (reenters)
		ctlv_intro_uint(fields, CTLV_ATTR_SCHEMA_REENTERS, reenters);
}

/* A nest is opened by the first thing written inside it. */
static bool ctlv_intro_open(struct ctlv_intro_walk *walk, u32 depth)
{
	struct ctlv_intro_frame *frame = &walk->frames[depth];
	struct ctlv_cursor *up = walk->root;

	if (frame->open)
		return true;
	if (depth) {
		if (!ctlv_intro_open(walk, depth - 1))
			return false;
		up = &walk->frames[depth - 1].nest;
	}
	if (!ctlv_nest_start(up, &frame->nest, frame->attr_id,
			     CTLV_SCHEMA_NEST))
		return false;
	frame->open = true;
	return true;
}

/*
 * Everything before the cursor was delivered earlier and is counted rather
 * than written; a write that does not fit leaves the count where it resumes.
 */
static bool ctlv_intro_put(struct ctlv_intro_walk *walk, u32 depth,
			   const struct ctlv_intro_field *field)
{
	struct ctlv_cursor *nest = &walk->frames[depth].nest;
	bool ok;

	if (walk->ordinal < walk->resume) {
		walk->ordinal++;
		return true;
	}
	if (!ctlv_intro_open(walk, depth))
		return false;
	if (field->kind == CTLV_SCHEMA_SINT)
		ok = ctlv_put_s64(nest, field->attr_id, field->kind,
				  field->value);
	else
		ok = ctlv_put_u64(nest, field->attr_id, field->kind,
				  field->value);
	if (!ok)
		return false;
	walk->ordinal++;
	return true;
}

/*
 * How far up the node whose members are this one's as well: both are
 * placements of the same attr set, so both point at the same children.
 */
static u32 ctlv_intro_reenters(struct ctlv_intro_walk *walk, u32 depth,
			       const struct ctlv_schema_tree *tree,
			       const struct ctlv_schema_node *node)
{
	u32 levels;

	for (levels = 1; levels <= depth; levels++) {
		const struct ctlv_intro_frame *up =
			&walk->frames[depth - levels];

		if (up->tree == tree &&
		    tree->nodes[up->node].first_child == node->first_child)
			return levels;
	}
	/* A back edge to nothing is metadata this core cannot describe. */
	WARN_ON_ONCE(1);
	walk->error = -EINVAL;
	return 0;
}

static bool ctlv_intro_node(struct ctlv_intro_walk *walk, u32 depth,
			    const struct ctlv_schema_tree *tree, u32 index);

/*
 * The placements one level in: what the nest declares, then what each extension
 * point of it was filled with. A stub optional at this placement makes
 * everything it was filled with optional.
 */
static bool ctlv_intro_children(struct ctlv_intro_walk *walk, u32 depth,
				const struct ctlv_schema_tree *tree, u32 index,
				bool force_optional)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];
	u32 i;

	if (WARN_ON_ONCE(depth + 1 >= CTLV_MAX_NEST_DEPTH)) {
		/* Deeper than a specification may declare, so unreachable. */
		walk->error = -EINVAL;
		return false;
	}
	for (i = 0; i < node->n_children; i++) {
		u32 child = node->first_child + i;
		bool required = !force_optional &&
				i < node->n_required_children;

		walk->frames[depth + 1] = (struct ctlv_intro_frame){
			.attr_id = required ? CTLV_ATTR_SCHEMA_ATTRS :
					      CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
			.tree = tree,
			.node = child,
		};
		if (!ctlv_intro_node(walk, depth + 1, tree, child))
			return false;
	}
	for (i = 0; i < tree->n_stubs; i++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[i];
		const struct ctlv_stub_fill *fill;

		if (ref->node != index)
			continue;
		fill = ctlv_schema_fill(walk->leaf, ref->family_id,
					ref->set_id);
		/* Nothing of this chain filled it, so it contributes none. */
		if (!fill)
			continue;
		/* A fill's root is the stub, its children the members. */
		if (!ctlv_intro_children(walk, depth, &fill->members, 0,
					 force_optional || ref->optional))
			return false;
	}
	return true;
}

/* One node, into the nest that stands for it. */
static bool ctlv_intro_node(struct ctlv_intro_walk *walk, u32 depth,
			    const struct ctlv_schema_tree *tree, u32 index)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];
	struct ctlv_intro_fields fields = {};
	u32 reenters = 0;
	u32 i;

	if (node->reenters) {
		reenters = ctlv_intro_reenters(walk, depth, tree, node);
		if (!reenters)
			return false;
	}
	ctlv_intro_node_fields(&fields, node, reenters);
	for (i = 0; i < fields.n; i++)
		if (!ctlv_intro_put(walk, depth, &fields.field[i]))
			return false;
	/*
	 * A node that re-enters a level says so instead of listing that level's
	 * members again, which is what makes an unbounded nesting describable.
	 */
	if (!node->reenters &&
	    !ctlv_intro_children(walk, depth, tree, index, false))
		return false;
	if (walk->frames[depth].open)
		ctlv_nest_end(&walk->frames[depth].nest);
	return true;
}

/* One of the two messages of an op, the root of it being a node like any. */
static bool ctlv_intro_message(struct ctlv_intro_walk *walk, u64 attr_id,
			       const struct ctlv_schema_tree *tree)
{
	/* An op without such a message has nothing to describe. */
	if (!tree->nodes)
		return true;
	walk->frames[0] = (struct ctlv_intro_frame){
		.attr_id = attr_id,
		.tree = tree,
	};
	return ctlv_intro_node(walk, 0, tree, 0);
}

int ctlv_op_schema_get(struct ctlv_op_ctx *ctx,
		       const struct ctlv_cursor *request,
		       struct ctlv_cursor *reply)
{
	struct ctlv_intro_walk walk = {
		.leaf = ctx->leaf,
		.root = reply,
		.resume = ctlv_op_query_cursor(ctx),
	};
	const struct ctlv_op_entry *op;

	/*
	 * The published index, so an op taken away before the device registered
	 * is as absent as one no family declared. What is left is described
	 * whether or not it can be called right now.
	 */
	op = ctlv_op_find(ctx->ctlvdev, ctlv_op_id_get(request));
	if (!op)
		return -EOPNOTSUPP;

	/*
	 * One frame per level the reply may nest, too many for the stack: a
	 * description is one level deeper than the message it describes.
	 */
	struct ctlv_intro_frame *frames __free(kfree) =
		kcalloc(CTLV_MAX_NEST_DEPTH, sizeof(*frames), GFP_KERNEL);

	if (!frames)
		return -ENOMEM;
	walk.frames = frames;

	if (!ctlv_intro_message(&walk, CTLV_ATTR_SCHEMA_REQUEST,
				&op->schema->request) ||
	    !ctlv_intro_message(&walk, CTLV_ATTR_SCHEMA_REPLY,
				&op->schema->reply)) {
		if (walk.error)
			return walk.error;
		/* A schema is immutable, so there is nothing to be stale. */
		return ctlv_op_query_more(ctx, 0, walk.ordinal);
	}
	return ctlv_op_query_done(ctx, 0);
}
