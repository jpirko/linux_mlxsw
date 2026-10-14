// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Root-family discovery queries and their fragmentation.
 */

#include <linux/bitmap.h>
#include <linux/bits.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "device.h"
#include "internal.h"
#include "schema.h"

/* A cursor packs the record and member where fragmentation resumes. */
#define CTLV_INTROSPECTION_MEMBER_SHIFT 48
#define CTLV_INTROSPECTION_RECORD_MASK GENMASK_ULL(CTLV_INTROSPECTION_MEMBER_SHIFT - 1, 0)

struct ctlv_introspection_dump_position {
	u64 record;
	u32 member;
};

static void ctlv_introspection_dump_resume(struct ctlv_introspection_dump_position *pos, u64 cursor)
{
	pos->record = cursor & CTLV_INTROSPECTION_RECORD_MASK;
	pos->member = cursor >> CTLV_INTROSPECTION_MEMBER_SHIFT;
}

static u64 ctlv_introspection_dump_cursor(const struct ctlv_introspection_dump_position *pos)
{
	return pos->record | (u64)pos->member << CTLV_INTROSPECTION_MEMBER_SHIFT;
}

/* The immutable root-first chain always has generation zero. */
static bool ctlv_introspection_family_record(struct ctlv_cursor *reply, struct ctlv_cursor *entry,
					     const struct ctlv_family *family,
					     struct ctlv_introspection_dump_position *pos)
{
	const struct ctlv_family_schema *schema = family->schema;

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

int ctlv_families_dump_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply)
{
	const struct ctlv_family *family = ctx->family;
	u32 levels = ctlv_family_levels(family);
	struct ctlv_cursor entry;
	struct ctlv_introspection_dump_position pos;

	ctlv_introspection_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	for (; pos.record < levels; pos.record++) {
		const struct ctlv_family *ancestor =
			ctlv_family_ancestor(family, levels - 1 - pos.record);

		if (!ctlv_introspection_family_record(reply, &entry, ancestor, &pos))
			return ctlv_op_query_more(ctx, 0, ctlv_introspection_dump_cursor(&pos));
	}
	return ctlv_op_query_done(ctx, 0);
}

/* Snapshot published-op state and generation once per fragment. */
struct ctlv_op_snapshot {
	unsigned long *disabled;
	u64 generation;
};

static int ctlv_introspection_snapshot_take(struct ctlv_device *ctlvdev,
					    struct ctlv_op_snapshot *snapshot)
{
	if (ctlvdev->dynamic_ops_count) {
		snapshot->disabled = bitmap_zalloc(ctlvdev->dynamic_ops_count, GFP_KERNEL);
		if (!snapshot->disabled)
			return -ENOMEM;
	}
	guard(spinlock)
		(&ctlvdev->transition_lock);
	snapshot->generation = ctlvdev->state_generation;
	if (snapshot->disabled)
		bitmap_copy(snapshot->disabled, ctlvdev->disabled, ctlvdev->dynamic_ops_count);
	return 0;
}

static bool ctlv_introspection_op_disabled(const struct ctlv_op_snapshot *snapshot,
					   const struct ctlv_op_entry *op)
{
	if (op->state_index == CTLV_OP_STATE_NONE)
		return false;
	return test_bit(op->state_index, snapshot->disabled);
}

/* Report effective lengths, including descendant fills. */
enum ctlv_introspection_op_field {
	CTLV_INTROSPECTION_OP_FIELD_ID,
	CTLV_INTROSPECTION_OP_FIELD_TYPE,
	CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY,
	CTLV_INTROSPECTION_OP_FIELD_DISABLED,
	CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN,
	CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN,
	CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_MIN_LEN,
	CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_SUGGESTED_LEN,
	CTLV_INTROSPECTION_OP_FIELD_EVENT_RECORD_MAX_LEN,
	CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS,
	CTLV_INTROSPECTION_OP_FIELD_END,
};

static bool ctlv_introspection_op_field_put(struct ctlv_cursor *entry,
					    const struct ctlv_op_snapshot *snapshot,
					    const struct ctlv_op_entry *op, u32 member)
{
	const struct ctlv_op_sizes *sizes = &op->sizes;

	switch (member) {
	case CTLV_INTROSPECTION_OP_FIELD_ID:
		return ctlv_op_id_put(entry, op->schema->op_id);
	case CTLV_INTROSPECTION_OP_FIELD_TYPE:
		return ctlv_op_type_put(entry, op->schema->type);
	case CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY:
		return ctlv_op_availability_put(entry, op->schema->availability);
	case CTLV_INTROSPECTION_OP_FIELD_DISABLED:
		return ctlv_op_disabled_put(entry, ctlv_introspection_op_disabled(snapshot, op));
	case CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN:
		return ctlv_op_request_max_len_put(entry, sizes->request_max_len);
	case CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN:
		/* Zero is a reply that acknowledges and no more. */
		return ctlv_op_action_reply_max_len_put(entry, sizes->action_reply_max_len);
	case CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_MIN_LEN:
		return ctlv_op_query_reply_min_len_put(entry, sizes->query_reply_min_len);
	case CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_SUGGESTED_LEN:
		return ctlv_op_query_reply_suggested_len_put(entry,
							     sizes->query_reply_suggested_len);
	case CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS:
		return ctlv_op_fd_access_put(entry, op->schema->fd_access);
	default:
		return ctlv_op_event_record_max_len_put(entry, sizes->event_record_max_len);
	}
}

static bool ctlv_introspection_op_field_applies(const struct ctlv_op_entry *op, u32 member)
{
	switch (member) {
	case CTLV_INTROSPECTION_OP_FIELD_ID:
	case CTLV_INTROSPECTION_OP_FIELD_TYPE:
		return true;
	case CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY:
	case CTLV_INTROSPECTION_OP_FIELD_DISABLED:
	case CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN:
	case CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS:
		return op->schema->type != CTLV_OP_TYPE_EVENT;
	case CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN:
		return op->schema->type == CTLV_OP_TYPE_ACTION;
	case CTLV_INTROSPECTION_OP_FIELD_EVENT_RECORD_MAX_LEN:
		return op->schema->type == CTLV_OP_TYPE_EVENT;
	default:
		return op->schema->type == CTLV_OP_TYPE_QUERY;
	}
}

static bool ctlv_introspection_op_record(struct ctlv_cursor *reply, struct ctlv_cursor *entry,
					 const struct ctlv_op_snapshot *snapshot,
					 const struct ctlv_op_entry *op,
					 struct ctlv_introspection_dump_position *pos)
{
	if (!ctlv_op_entry_start(reply, entry))
		return false;
	for (; pos->member < CTLV_INTROSPECTION_OP_FIELD_END; pos->member++) {
		if (!ctlv_introspection_op_field_applies(op, pos->member))
			continue;
		if (!ctlv_introspection_op_field_put(entry, snapshot, op, pos->member))
			return false;
	}
	ctlv_nest_end(entry);
	pos->member = 0;
	return true;
}

int ctlv_ops_dump_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			struct ctlv_cursor *reply)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_op_snapshot snapshot = {};
	struct ctlv_cursor entry;
	struct ctlv_introspection_dump_position pos;
	int ret;

	ret = ctlv_introspection_snapshot_take(ctlvdev, &snapshot);
	if (ret)
		return ret;
	unsigned long *disabled __free(bitmap) = snapshot.disabled;

	/* The published index is immutable, so a cursor into it holds. */
	ctlv_introspection_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	for (; pos.record < ctlvdev->ops_count; pos.record++)
		if (!ctlv_introspection_op_record(reply, &entry, &snapshot,
						  &ctlvdev->ops[pos.record], &pos))
			return ctlv_op_query_more(ctx, snapshot.generation,
						  ctlv_introspection_dump_cursor(&pos));
	return ctlv_op_query_done(ctx, snapshot.generation);
}

/* A leaf-count cursor permits a fragment boundary at any tree depth. */

/* One nest of the description: the placement of one node of the schema. */
struct ctlv_introspection_frame {
	struct ctlv_cursor nest;
	u64 attr_id;
	bool open;
	/* Which node this stands for, so that a re-entry can be measured. */
	const struct ctlv_schema_tree *tree;
	u32 node;
};

/* The frames outlive the walk, a fragment's open nests having to stay alive. */
struct ctlv_introspection_walk {
	const struct ctlv_family *family;
	struct ctlv_cursor *root;
	struct ctlv_introspection_frame *frames;
	u64 ordinal;
	u64 resume;
	int error;
};

/* One member of one node, which is every leaf this reply has. */
struct ctlv_introspection_field {
	u64 attr_id;
	u8 kind;
	u64 value;
};

/* Three every node has, the widest bounds, a unit, a scale, a re-entry. */
#define CTLV_INTROSPECTION_MAX_FIELDS 10

struct ctlv_introspection_fields {
	struct ctlv_introspection_field fields[CTLV_INTROSPECTION_MAX_FIELDS];
	u32 fields_count;
};

static void ctlv_introspection_field(struct ctlv_introspection_fields *fields, u64 attr_id, u8 kind,
				     u64 value)
{
	if (WARN_ON_ONCE(fields->fields_count == CTLV_INTROSPECTION_MAX_FIELDS))
		return;
	fields->fields[fields->fields_count++] = (struct ctlv_introspection_field){
		.attr_id = attr_id,
		.kind = kind,
		.value = value,
	};
}

static void ctlv_introspection_uint(struct ctlv_introspection_fields *fields, u64 attr_id,
				    u64 value)
{
	ctlv_introspection_field(fields, attr_id, CTLV_SCHEMA_UINT, value);
}

/* Emit only constraints for this node's payload kind. */
static void ctlv_introspection_node_fields(struct ctlv_introspection_fields *fields,
					   const struct ctlv_node_schema *node, u32 reenters)
{
	ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_ATTR_ID, node->attr_id);
	ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_KIND, node->kind);
	ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, node->max_occurrences);

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_SINT:
	case CTLV_SCHEMA_ENUM:
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_MIN, node->range.min);
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_MAX, node->range.max);
		break;
	case CTLV_SCHEMA_FLAGS:
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_FLAGS_MASK, node->flags_mask);
		break;
	case CTLV_SCHEMA_STRING:
	case CTLV_SCHEMA_BINARY:
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_MIN_LENGTH, node->length.min_len);
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_MAX_LENGTH, node->length.max_len);
		break;
	case CTLV_SCHEMA_BLOB:
		ctlv_introspection_field(fields, CTLV_ATTR_SCHEMA_BLOB_DIRECTION, CTLV_SCHEMA_ENUM,
					 node->blob.direction);
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
					node->blob.backings);
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
					node->blob.max_len);
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
					node->blob.alignment);
		break;
	default:
		break;
	}

	if (node->unit != CTLV_UNIT_NONE) {
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_UNIT, node->unit);
		ctlv_introspection_field(fields, CTLV_ATTR_SCHEMA_SCALE, CTLV_SCHEMA_SINT,
					 node->scale);
	}
	if (reenters)
		ctlv_introspection_uint(fields, CTLV_ATTR_SCHEMA_REENTERS, reenters);
}

/* A nest is opened by the first thing written inside it. */
static bool ctlv_introspection_open(struct ctlv_introspection_walk *walk, u32 depth)
{
	struct ctlv_introspection_frame *frame = &walk->frames[depth];
	struct ctlv_cursor *up = walk->root;

	if (frame->open)
		return true;
	if (depth) {
		if (!ctlv_introspection_open(walk, depth - 1))
			return false;
		up = &walk->frames[depth - 1].nest;
	}
	if (!ctlv_nest_start(up, &frame->nest, frame->attr_id, CTLV_SCHEMA_NEST))
		return false;
	frame->open = true;
	return true;
}

/* Skipped leaves advance the ordinal; a failed write remains the resume point. */
static bool ctlv_introspection_put(struct ctlv_introspection_walk *walk, u32 depth,
				   const struct ctlv_introspection_field *field)
{
	struct ctlv_cursor *nest = &walk->frames[depth].nest;
	bool ok;

	if (walk->ordinal < walk->resume) {
		walk->ordinal++;
		return true;
	}
	if (!ctlv_introspection_open(walk, depth))
		return false;
	if (field->kind == CTLV_SCHEMA_SINT)
		ok = ctlv_put_s64(nest, field->attr_id, field->kind, field->value);
	else
		ok = ctlv_put_u64(nest, field->attr_id, field->kind, field->value);
	if (!ok)
		return false;
	walk->ordinal++;
	return true;
}

/* Re-entry targets an ancestor with the same child range. */
static u32 ctlv_introspection_reenters(struct ctlv_introspection_walk *walk, u32 depth,
				       const struct ctlv_schema_tree *tree,
				       const struct ctlv_node_schema *node)
{
	u32 levels;

	for (levels = 1; levels <= depth; levels++) {
		const struct ctlv_introspection_frame *up = &walk->frames[depth - levels];

		if (up->tree == tree &&
		    tree->node_schemas[up->node].first_child == node->first_child)
			return levels;
	}
	/* A back edge to nothing is metadata this core cannot describe. */
	WARN_ON_ONCE(1);
	walk->error = -EINVAL;
	return 0;
}

static bool ctlv_introspection_node(struct ctlv_introspection_walk *walk, u32 depth,
				    const struct ctlv_schema_tree *tree, u32 index);

/* Filled stub members inherit the stub placement's optionality. */
static bool ctlv_introspection_children(struct ctlv_introspection_walk *walk, u32 depth,
					const struct ctlv_schema_tree *tree, u32 index,
					bool force_optional)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	u32 i;

	if (WARN_ON_ONCE(depth + 1 >= CTLV_MAX_NEST_DEPTH)) {
		/* Deeper than a specification may declare, so unreachable. */
		walk->error = -EINVAL;
		return false;
	}
	for (i = 0; i < node->children_count; i++) {
		u32 child = node->first_child + i;
		bool required = !force_optional && i < node->required_children_count;

		walk->frames[depth + 1] = (struct ctlv_introspection_frame){
			.attr_id = required ? CTLV_ATTR_SCHEMA_ATTRS :
					      CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
			.tree = tree,
			.node = child,
		};
		if (!ctlv_introspection_node(walk, depth + 1, tree, child))
			return false;
	}
	for (i = 0; i < tree->stub_schemas_count; i++) {
		const struct ctlv_stub_schema *stub_schema = &tree->stub_schemas[i];
		const struct ctlv_stub_fill_schema *fill_schema;

		if (stub_schema->node != index)
			continue;
		fill_schema =
			ctlv_schema_fill(walk->family, stub_schema->family_id, stub_schema->set_id);
		/* Nothing of this chain filled it, so it contributes none. */
		if (!fill_schema)
			continue;
		/* A fill schema's root is the stub, its children the members. */
		if (!ctlv_introspection_children(walk, depth, &fill_schema->members, 0,
						 force_optional || stub_schema->optional))
			return false;
	}
	return true;
}

/* One node, into the nest that stands for it. */
static bool ctlv_introspection_node(struct ctlv_introspection_walk *walk, u32 depth,
				    const struct ctlv_schema_tree *tree, u32 index)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	struct ctlv_introspection_fields fields = {};
	u32 reenters = 0;
	u32 i;

	if (node->reenters) {
		reenters = ctlv_introspection_reenters(walk, depth, tree, node);
		if (!reenters)
			return false;
	}
	ctlv_introspection_node_fields(&fields, node, reenters);
	for (i = 0; i < fields.fields_count; i++)
		if (!ctlv_introspection_put(walk, depth, &fields.fields[i]))
			return false;
	/*
	 * A node that re-enters a level says so instead of listing that level's
	 * members again, which is what makes an unbounded nesting describable.
	 */
	if (!node->reenters && !ctlv_introspection_children(walk, depth, tree, index, false))
		return false;
	if (walk->frames[depth].open)
		ctlv_nest_end(&walk->frames[depth].nest);
	return true;
}

/* One of the two messages of an op, the root of it being a node like any. */
static bool ctlv_introspection_message(struct ctlv_introspection_walk *walk, u64 attr_id,
				       const struct ctlv_schema_tree *tree)
{
	/* An op without such a message has nothing to describe. */
	if (!tree->node_schemas)
		return true;
	walk->frames[0] = (struct ctlv_introspection_frame){
		.attr_id = attr_id,
		.tree = tree,
	};
	return ctlv_introspection_node(walk, 0, tree, 0);
}

int ctlv_op_schema_get_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply)
{
	struct ctlv_introspection_walk walk = {
		.family = ctx->family,
		.root = reply,
		.resume = ctlv_op_query_cursor(ctx),
	};
	const struct ctlv_op_entry *op;

	/* Describe published ops regardless of current dynamic state. */
	op = ctlv_op_find(ctx->ctlvdev, ctlv_op_schema_get_op_id_get(request));
	if (!op)
		return -EOPNOTSUPP;

	/*
	 * One frame per level the reply may nest, too many for the stack: a
	 * description is one level deeper than the message it describes.
	 */
	struct ctlv_introspection_frame *frames __free(kfree) =
		kcalloc(CTLV_MAX_NEST_DEPTH, sizeof(*frames), GFP_KERNEL);

	if (!frames)
		return -ENOMEM;
	walk.frames = frames;

	if (!ctlv_introspection_message(&walk, CTLV_ATTR_SCHEMA_REQUEST, &op->schema->request) ||
	    !ctlv_introspection_message(&walk, CTLV_ATTR_SCHEMA_REPLY, &op->schema->reply)) {
		if (walk.error)
			return walk.error;
		/* A schema is immutable, so there is nothing to be stale. */
		return ctlv_op_query_more(ctx, 0, walk.ordinal);
	}
	return ctlv_op_query_done(ctx, 0);
}
