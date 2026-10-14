// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Root-family discovery queries and their fragmentation.
 */

#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/bits.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>

#include "internal.h"
#include "schema.h"

/* A cursor packs the entry and member where fragmentation resumes. */
#define CTLV_INTROSPECTION_MEMBER_MASK GENMASK_U64(63, 48)
#define CTLV_INTROSPECTION_ENTRY_MASK GENMASK_U64(47, 0)

/*
 * family-get: the entry is a family's position in the chain from the root,
 * the member its next field. op-get: the entry indexes the published ops,
 * the member is the next CTLV_INTROSPECTION_OP_FIELD_*.
 */
struct ctlv_introspection_dump_position {
	u64 entry;
	u32 member;
};

static void ctlv_introspection_dump_resume(struct ctlv_introspection_dump_position *pos, u64 cursor)
{
	pos->entry = FIELD_GET(CTLV_INTROSPECTION_ENTRY_MASK, cursor);
	pos->member = FIELD_GET(CTLV_INTROSPECTION_MEMBER_MASK, cursor);
}

static u64 ctlv_introspection_dump_cursor(const struct ctlv_introspection_dump_position *pos)
{
	return FIELD_PREP(CTLV_INTROSPECTION_ENTRY_MASK, pos->entry) |
	       FIELD_PREP(CTLV_INTROSPECTION_MEMBER_MASK, pos->member);
}

static bool ctlv_introspection_family_entry(struct ctlv_cursor *reply, struct ctlv_cursor *entry,
					    const struct ctlv_family *family,
					    struct ctlv_introspection_dump_position *pos)
{
	const struct ctlv_family_schema *schema = family->schema;

	if (!ctlv_family_get_families_start(reply, entry))
		return false;
	if (pos->member == 0) {
		if (!ctlv_family_get_families_id_put(entry, schema->family_id))
			return false;
		pos->member = 1;
	}
	if (pos->member == 1) {
		if (!ctlv_family_get_families_name_put(entry, schema->name))
			return false;
		pos->member = 2;
	}
	ctlv_record_end(entry);
	pos->member = 0;
	return true;
}

int ctlv_family_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	const struct ctlv_family *family = ctx->family;
	u32 levels = ctlv_family_levels(family);
	struct ctlv_cursor entry;
	struct ctlv_introspection_dump_position pos;

	ctlv_introspection_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	/* The immutable root-first chain always has generation zero. */
	for (; pos.entry < levels; pos.entry++) {
		const struct ctlv_family *ancestor =
			ctlv_family_ancestor(family, levels - 1 - pos.entry);

		if (!ctlv_introspection_family_entry(reply, &entry, ancestor, &pos))
			return ctlv_op_query_more(ctx, 0, ctlv_introspection_dump_cursor(&pos));
	}
	return ctlv_op_query_done(ctx, 0);
}

/* Snapshot published-op state and generation once per fragment. */
struct ctlv_op_snapshot {
	unsigned long *disabled;
	u64 generation;
};

static int ctlv_introspection_snapshot_take(struct ctlv *ctlv,
					    struct ctlv_op_snapshot *snapshot)
{
	if (ctlv->dynamic_ops_count) {
		snapshot->disabled = bitmap_zalloc(ctlv->dynamic_ops_count, GFP_KERNEL);
		if (!snapshot->disabled)
			return -ENOMEM;
	}
	guard(spinlock)(&ctlv->transition_lock);
	snapshot->generation = ctlv->state_generation;
	if (snapshot->disabled)
		bitmap_copy(snapshot->disabled, ctlv->disabled, ctlv->dynamic_ops_count);
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
	CTLV_INTROSPECTION_OP_FIELD_ID = 0,
	CTLV_INTROSPECTION_OP_FIELD_TYPE = 1,
	CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY = 2,
	CTLV_INTROSPECTION_OP_FIELD_DISABLED = 3,
	CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN = 4,
	CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN = 5,
	CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_MIN_LEN = 6,
	CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_SUGGESTED_LEN = 7,
	CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS = 8,
	CTLV_INTROSPECTION_OP_FIELD_ERROR_INFO_MAX_LEN = 9,
	CTLV_INTROSPECTION_OP_FIELD_END = 10,
};

static bool ctlv_introspection_op_field_put(struct ctlv_cursor *entry,
					    const struct ctlv_op_snapshot *snapshot,
					    const struct ctlv_op_entry *op, u32 member)
{
	const struct ctlv_op_sizes *sizes = &op->sizes;

	switch (member) {
	case CTLV_INTROSPECTION_OP_FIELD_ID:
		return ctlv_op_get_ops_id_put(entry, op->schema->op_id);
	case CTLV_INTROSPECTION_OP_FIELD_TYPE:
		return ctlv_op_get_ops_type_put(entry, op->schema->type);
	case CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY:
		return ctlv_op_get_ops_availability_put(entry, op->schema->availability);
	case CTLV_INTROSPECTION_OP_FIELD_DISABLED:
		return ctlv_op_get_ops_disabled_put(entry,
						    ctlv_introspection_op_disabled(snapshot, op));
	case CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN:
		return ctlv_op_get_ops_request_max_len_put(entry, sizes->request_max_len);
	case CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN:
		/* Zero is a reply that acknowledges and no more. */
		return ctlv_op_get_ops_action_reply_max_len_put(entry,
								sizes->action_reply_max_len);
	case CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_MIN_LEN:
		return ctlv_op_get_ops_query_reply_min_len_put(entry, sizes->query_reply_min_len);
	case CTLV_INTROSPECTION_OP_FIELD_QUERY_REPLY_SUGGESTED_LEN:
		return ctlv_op_get_ops_query_reply_suggested_len_put(
			entry, sizes->query_reply_suggested_len);
	case CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS:
		return ctlv_op_get_ops_fd_access_put(entry, op->schema->fd_access);
	default:
		return ctlv_op_get_ops_error_info_max_len_put(entry, CTLV_ERROR_INFO_MAX_LEN);
	}
}

static bool ctlv_introspection_op_field_applies(const struct ctlv_op_entry *op, u32 member)
{
	switch (member) {
	case CTLV_INTROSPECTION_OP_FIELD_ID:
	case CTLV_INTROSPECTION_OP_FIELD_TYPE:
	case CTLV_INTROSPECTION_OP_FIELD_AVAILABILITY:
	case CTLV_INTROSPECTION_OP_FIELD_DISABLED:
	case CTLV_INTROSPECTION_OP_FIELD_REQUEST_MAX_LEN:
	case CTLV_INTROSPECTION_OP_FIELD_FD_ACCESS:
	case CTLV_INTROSPECTION_OP_FIELD_ERROR_INFO_MAX_LEN:
		return true;
	case CTLV_INTROSPECTION_OP_FIELD_ACTION_REPLY_MAX_LEN:
		return op->schema->type == CTLV_OP_TYPE_ACTION;
	default:
		return op->schema->type == CTLV_OP_TYPE_QUERY;
	}
}

static bool ctlv_introspection_op_entry(struct ctlv_cursor *reply, struct ctlv_cursor *entry,
					const struct ctlv_op_snapshot *snapshot,
					const struct ctlv_op_entry *op,
					struct ctlv_introspection_dump_position *pos)
{
	if (!ctlv_op_get_ops_start(reply, entry))
		return false;
	for (; pos->member < CTLV_INTROSPECTION_OP_FIELD_END; pos->member++) {
		if (!ctlv_introspection_op_field_applies(op, pos->member))
			continue;
		if (!ctlv_introspection_op_field_put(entry, snapshot, op, pos->member))
			return false;
	}
	ctlv_record_end(entry);
	pos->member = 0;
	return true;
}

int ctlv_op_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
		      const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct ctlv_op_snapshot snapshot = {};
	struct ctlv_cursor entry;
	struct ctlv_introspection_dump_position pos;
	int ret;

	ret = ctlv_introspection_snapshot_take(ctlv, &snapshot);
	if (ret)
		return ret;
	unsigned long *disabled __free(bitmap) = snapshot.disabled;

	/* The published index is immutable, so a cursor into it holds. */
	ctlv_introspection_dump_resume(&pos, ctlv_op_query_cursor(ctx));
	for (; pos.entry < ctlv->ops_count; pos.entry++)
		if (!ctlv_introspection_op_entry(reply, &entry, &snapshot,
						 &ctlv->ops[pos.entry], &pos))
			return ctlv_op_query_more(ctx, snapshot.generation,
						  ctlv_introspection_dump_cursor(&pos));
	return ctlv_op_query_done(ctx, snapshot.generation);
}

/* A leaf-count cursor permits a fragment boundary at any tree depth. */

/* One record of the description, standing for one node of the schema. */
struct ctlv_introspection_frame {
	struct ctlv_cursor record;
	u64 type;
	bool open;
	/* Which node this stands for, so that a re-entry can be measured. */
	const struct ctlv_schema_tree *tree;
	u32 node;
};

/* The frames outlive the walk, as a fragment's open records must stay alive. */
struct ctlv_introspection_walk {
	const struct ctlv_family *family;
	struct ctlv_cursor *root;
	struct ctlv_introspection_frame *frames;
	u64 ordinal;
	u64 resume;
	int error;
};

/* A record is opened by the first thing written inside it. */
static bool ctlv_introspection_open(struct ctlv_introspection_walk *walk, u32 depth)
{
	struct ctlv_introspection_frame *frame = &walk->frames[depth];
	struct ctlv_cursor *up = walk->root;

	if (frame->open)
		return true;
	if (depth) {
		if (!ctlv_introspection_open(walk, depth - 1))
			return false;
		up = &walk->frames[depth - 1].record;
	}
	if (!ctlv_record_start(up, &frame->record, frame->type, CTLV_SCHEMA_RECORD))
		return false;
	frame->open = true;
	return true;
}

/* Skipped leaves advance the ordinal; a failed write remains the resume point. */
static bool ctlv_introspection_put(struct ctlv_introspection_walk *walk, u32 depth, u64 type,
				   u8 kind, u64 value)
{
	struct ctlv_cursor *record = &walk->frames[depth].record;
	bool ok;

	if (walk->ordinal < walk->resume) {
		walk->ordinal++;
		return true;
	}
	if (!ctlv_introspection_open(walk, depth))
		return false;
	if (kind == CTLV_SCHEMA_SINT)
		ok = ctlv_put_s64(record, type, kind, value);
	else
		ok = ctlv_put_u64(record, type, kind, value);
	if (!ok)
		return false;
	walk->ordinal++;
	return true;
}

static bool ctlv_introspection_uint(struct ctlv_introspection_walk *walk, u32 depth, u64 type,
				    u64 value)
{
	return ctlv_introspection_put(walk, depth, type, CTLV_SCHEMA_UINT, value);
}

/* Emit only constraints for this node's schema kind. */
static bool ctlv_introspection_node_fields(struct ctlv_introspection_walk *walk, u32 depth,
					   const struct ctlv_node_schema *node, u32 reenters)
{
	if (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_TYPE, node->type) ||
	    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_KIND, node->kind) ||
	    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MAX_OCCURRENCES,
				     node->max_occurrences))
		return false;

	switch (node->kind) {
	case CTLV_SCHEMA_ENUM:
		/* An extension point's maximum is 0 when nothing filled it. */
		if (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MIN,
					     node->is_enum_ext ? 1 : node->range.min) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MAX,
					     ctlv_schema_enum_max(walk->family, node)))
			return false;
		break;
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_SINT:
		if (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MIN,
					     node->range.min) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MAX,
					     node->range.max))
			return false;
		break;
	case CTLV_SCHEMA_FLAGS:
		if (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_FLAGS_MASK,
					     node->flags_mask))
			return false;
		break;
	case CTLV_SCHEMA_STRING:
	case CTLV_SCHEMA_BINARY:
		if (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MIN_LENGTH,
					     node->length.min_len) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_MAX_LENGTH,
					     node->length.max_len))
			return false;
		break;
	case CTLV_SCHEMA_BLOB:
		if (!ctlv_introspection_put(walk, depth, CTLV_T_SCHEMA_NODE_BLOB_DIRECTION,
					    CTLV_SCHEMA_ENUM, node->blob.direction) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_BLOB_BACKINGS,
					     node->blob.backings) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_BLOB_MAX_LENGTH,
					     node->blob.max_len) ||
		    !ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_BLOB_ALIGNMENT,
					     node->blob.alignment))
			return false;
		break;
	default:
		break;
	}

	if (node->unit != CTLV_UNIT_NONE &&
	    (!ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_UNIT, node->unit) ||
	     !ctlv_introspection_put(walk, depth, CTLV_T_SCHEMA_NODE_SCALE, CTLV_SCHEMA_SINT,
				     node->scale)))
		return false;
	return !reenters ||
	       ctlv_introspection_uint(walk, depth, CTLV_T_SCHEMA_NODE_REENTERS, reenters);
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
	CTLV_ASSERT(0);
	walk->error = -EINVAL;
	return 0;
}

static bool ctlv_introspection_node(struct ctlv_introspection_walk *walk, u32 depth,
				    const struct ctlv_schema_tree *tree, u32 index);

/* The members of a fill inherit the optionality of its extension point. */
static bool ctlv_introspection_children(struct ctlv_introspection_walk *walk, u32 depth,
					const struct ctlv_schema_tree *tree, u32 index,
					bool force_optional)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	u32 i;

	if (WARN_ON_ONCE(depth + 1 >= CTLV_MAX_RECORD_DEPTH)) {
		/* Deeper than a specification may declare, so unreachable. */
		walk->error = -EINVAL;
		return false;
	}
	for (i = 0; i < node->children_count; i++) {
		u32 child = node->first_child + i;
		bool required = !force_optional && i < node->required_children_count;

		walk->frames[depth + 1] = (struct ctlv_introspection_frame){
			.type = required ? CTLV_T_SCHEMA_NODE_FIELDS :
					      CTLV_T_SCHEMA_NODE_OPTIONAL_FIELDS,
			.tree = tree,
			.node = child,
		};
		if (!ctlv_introspection_node(walk, depth + 1, tree, child))
			return false;
	}
	for (i = 0; i < tree->ext_schemas_count; i++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[i];
		const struct ctlv_ext_fill_schema *fill_schema;

		if (ext_schema->node != index)
			continue;
		fill_schema =
			ctlv_schema_fill(walk->family, ext_schema->family_id, ext_schema->ext_id);
		/* Nothing of this chain filled it, so it contributes none. */
		if (!fill_schema)
			continue;
		/* Its root is the extension point, its children the members. */
		if (!ctlv_introspection_children(walk, depth, &fill_schema->members, 0,
						 force_optional || ext_schema->optional))
			return false;
	}
	return true;
}

/* One node, into the record that stands for it. */
static bool ctlv_introspection_node(struct ctlv_introspection_walk *walk, u32 depth,
				    const struct ctlv_schema_tree *tree, u32 index)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	u32 reenters = 0;

	if (node->reenters) {
		reenters = ctlv_introspection_reenters(walk, depth, tree, node);
		if (!reenters)
			return false;
	}
	if (!ctlv_introspection_node_fields(walk, depth, node, reenters))
		return false;
	/*
	 * A node that re-enters a level says so instead of listing that level's
	 * members again, which is what makes unbounded depth describable.
	 */
	if (!node->reenters && !ctlv_introspection_children(walk, depth, tree, index, false))
		return false;
	if (walk->frames[depth].open)
		ctlv_record_end(&walk->frames[depth].record);
	return true;
}

/* One of the two messages of an op, the root of it being a node like any. */
static bool ctlv_introspection_message(struct ctlv_introspection_walk *walk, u64 type,
				       const struct ctlv_schema_tree *tree)
{
	/* An op without such a message has nothing to describe. */
	if (!tree->node_schemas)
		return true;
	walk->frames[0] = (struct ctlv_introspection_frame){
		.type = type,
		.tree = tree,
	};
	return ctlv_introspection_node(walk, 0, tree, 0);
}

int ctlv_op_schema_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct ctlv_introspection_walk walk = {
		.family = ctx->family,
		.root = reply,
		.resume = ctlv_op_query_cursor(ctx),
	};
	const struct ctlv_op_entry *op;

	/* Describe published ops regardless of current dynamic state. */
	op = ctlv_op_find(ctx->ctlv, ctlv_op_schema_get_id_get(request));
	if (!op)
		return -EOPNOTSUPP;

	/*
	 * One frame per level the reply may reach, too many for the stack: a
	 * description is one level deeper than the message it describes.
	 */
	struct ctlv_introspection_frame *frames __free(kfree) =
		kcalloc(CTLV_MAX_RECORD_DEPTH, sizeof(*frames), GFP_KERNEL);

	if (!frames)
		return -ENOMEM;
	walk.frames = frames;

	if (!ctlv_introspection_message(&walk, CTLV_T_OP_SCHEMA_GET_REQUEST,
					&op->schema->request) ||
	    !ctlv_introspection_message(&walk, CTLV_T_OP_SCHEMA_GET_REPLY,
					&op->schema->reply)) {
		if (walk.error)
			return walk.error;
		/* A schema is immutable, so there is nothing to be stale. */
		return ctlv_op_query_more(ctx, 0, walk.ordinal);
	}
	return ctlv_op_query_done(ctx, 0);
}
