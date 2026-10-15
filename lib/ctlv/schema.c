// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Generated metadata remains owner-local; effective schemas walk the chain.
 */
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <linux/overflow.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "blob_desc.h"
#include "internal.h"
#include "message.h"
#include "schema.h"

/* Every 64-bit payload, and every blob descriptor member, is this long. */
#define CTLV_SCHEMA_SCALAR_TLV_LEN (sizeof(struct ctlv_hdr) + sizeof(u64))

/*
 * Where composing a tree's maximum length gives up, which keeps the check on a
 * composed length from being passed by a total that wrapped.
 */
#define CTLV_SCHEMA_MAX_COMPOSED_LEN CTLV_MAX_INLINE_MESSAGE_LEN

const struct ctlv_family *ctlv_family_ancestor(const struct ctlv_family *family, u32 up)
{
	while (up-- && family)
		family = family->parent;
	return family;
}

u32 ctlv_family_levels(const struct ctlv_family *family)
{
	const struct ctlv_family *ancestor;
	u32 levels = 0;

	for (ancestor = family; ancestor; ancestor = ancestor->parent)
		levels++;
	return levels;
}

void ctlv_schema_cursor_init(struct ctlv_schema_cursor *cursor, const struct ctlv_family *family)
{
	cursor->target = family;
	cursor->level = ctlv_family_levels(family);
	cursor->family = NULL;
	cursor->index = 0;
}

const struct ctlv_op_schema *ctlv_schema_next_op(struct ctlv_schema_cursor *cursor,
						 const struct ctlv_family **family)
{
	while (cursor->level) {
		const struct ctlv_family_schema *schema;

		if (!cursor->family)
			cursor->family = ctlv_family_ancestor(cursor->target, cursor->level - 1);
		schema = cursor->family->schema;
		if (cursor->index < schema->ops_count) {
			*family = cursor->family;
			return &schema->ops[cursor->index++];
		}
		cursor->level--;
		cursor->family = NULL;
		cursor->index = 0;
	}
	return NULL;
}

bool ctlv_schema_member(const struct ctlv_family *family, const struct ctlv_schema_tree *tree,
			u32 node, u64 type, struct ctlv_schema_pos *member)
{
	const struct ctlv_node_schema *record;
	u32 index;

	if (!tree->node_schemas || node >= tree->node_schemas_count)
		return false;
	record = &tree->node_schemas[node];

	for (index = 0; index < record->children_count; index++) {
		u32 child = record->first_child + index;

		if (tree->node_schemas[child].type == type) {
			*member = (struct ctlv_schema_pos){
				.tree = tree,
				.node = child,
			};
			return true;
		}
	}
	for (index = 0; index < tree->ext_schemas_count; index++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[index];
		const struct ctlv_ext_fill_schema *fill_schema;

		if (ext_schema->node != node)
			continue;
		fill_schema = ctlv_schema_fill(family, ext_schema->family_id, ext_schema->ext_id);
		/* The members are children of the fill schema's root. */
		if (fill_schema &&
		    ctlv_schema_member(family, &fill_schema->members, 0, type, member))
			return true;
	}
	return false;
}

const struct ctlv_ext_fill_schema *ctlv_schema_fill(const struct ctlv_family *family,
						    u16 family_id, u32 ext_id)
{
	const struct ctlv_family *owner;
	u32 index;

	for (owner = family; owner; owner = owner->parent) {
		const struct ctlv_family_schema *schema = owner->schema;

		for (index = 0; index < schema->ext_fill_schemas_count; index++)
			if (schema->ext_fill_schemas[index].family_id == family_id &&
			    schema->ext_fill_schemas[index].ext_id == ext_id)
				return &schema->ext_fill_schemas[index];
	}
	return NULL;
}

/**
 * ctlv_enum_ext_max - The highest value a family gives an enum extension point.
 * @family: the family of a device
 * @family_id: the family declaring the point
 * @ext_id: the point's <FAMILY>_ENUM_EXT_<NAME>
 *
 * Return: the highest value the chain of @family gives the point, the lowest
 * being 1, or 0 where it gives none.
 */
u32 ctlv_enum_ext_max(const struct ctlv_family *family, u16 family_id, u32 ext_id)
{
	const struct ctlv_family *owner;
	u32 index;

	for (owner = family; owner; owner = owner->parent) {
		const struct ctlv_family_schema *schema = owner->schema;

		for (index = 0; index < schema->enum_fill_schemas_count; index++)
			if (schema->enum_fill_schemas[index].family_id == family_id &&
			    schema->enum_fill_schemas[index].ext_id == ext_id)
				return schema->enum_fill_schemas[index].max;
	}
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_enum_ext_max, "CTLV");

/* The highest value an enum node takes on a device of family. */
u64 ctlv_schema_enum_max(const struct ctlv_family *family, const struct ctlv_node_schema *node)
{
	if (!node->is_enum_ext)
		return node->range.max;
	return ctlv_enum_ext_max(family, node->enum_ext.family_id, node->enum_ext.ext_id);
}

/*
 * The same arithmetic the generator did, over the tree it emitted plus this
 * family's fill schemas, since a fill makes an extension point longer.
 */

struct ctlv_schema_length_walk {
	const struct ctlv_family *family;
	bool bad;
};

static u64 ctlv_schema_children_max_len(struct ctlv_schema_length_walk *walk,
					const struct ctlv_schema_tree *tree, u32 index, u32 depth);

static u64 ctlv_schema_payload_max_len(struct ctlv_schema_length_walk *walk,
				       const struct ctlv_schema_tree *tree, u32 index, u32 depth)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];

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
		return (node->blob.backings == BIT(CTLV_BLOB_TYPE_VA) ? 3 : 4) *
		       ALIGN(CTLV_SCHEMA_SCALAR_TLV_LEN, CTLV_ALIGNTO);
	default:
		return ctlv_schema_children_max_len(walk, tree, index, depth);
	}
}

static u64 ctlv_schema_tlv_max_len(struct ctlv_schema_length_walk *walk,
				   const struct ctlv_schema_tree *tree, u32 index, u32 depth)
{
	return sizeof(struct ctlv_hdr) + ctlv_schema_payload_max_len(walk, tree, index, depth);
}

static u64 ctlv_schema_children_max_len(struct ctlv_schema_length_walk *walk,
					const struct ctlv_schema_tree *tree, u32 index, u32 depth)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	u64 total = 0;
	u32 i;

	if (depth > CTLV_MAX_DECLARED_RECORD_DEPTH) {
		walk->bad = true;
		return 0;
	}

	for (i = 0; i < node->children_count; i++) {
		u32 child = node->first_child + i;
		u64 each, all;

		/*
		 * Neither an unlimited node nor one that re-enters has a
		 * maximum, and only a query reply may hold either.
		 */
		if (tree->node_schemas[child].reenters ||
		    !tree->node_schemas[child].max_occurrences) {
			walk->bad = true;
			return 0;
		}

		each = ALIGN(ctlv_schema_tlv_max_len(walk, tree, child, depth + 1), CTLV_ALIGNTO);
		if (check_mul_overflow((u64)tree->node_schemas[child].max_occurrences, each,
				       &all) ||
		    check_add_overflow(total, all, &total) ||
		    total > CTLV_SCHEMA_MAX_COMPOSED_LEN) {
			walk->bad = true;
			return 0;
		}
	}

	for (i = 0; i < tree->ext_schemas_count; i++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[i];
		const struct ctlv_ext_fill_schema *fill_schema;

		if (ext_schema->node != index)
			continue;
		fill_schema =
			ctlv_schema_fill(walk->family, ext_schema->family_id, ext_schema->ext_id);
		if (!fill_schema)
			continue;
		if (check_add_overflow(total,
				       ctlv_schema_children_max_len(walk, &fill_schema->members, 0,
								    depth),
				       &total) ||
		    total > CTLV_SCHEMA_MAX_COMPOSED_LEN) {
			walk->bad = true;
			return 0;
		}
	}
	return total;
}

/* Buffer length that the largest indivisible leaf and its records need. */
static u64 ctlv_schema_min_len(struct ctlv_schema_length_walk *walk,
			       const struct ctlv_schema_tree *tree, u32 index, u32 above, u32 depth)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	bool has_children = false;
	u64 deepest = 0;
	u32 i;

	if (depth > CTLV_MAX_DECLARED_RECORD_DEPTH) {
		walk->bad = true;
		return 0;
	}

	for (i = 0; i < node->children_count; i++) {
		u32 child = node->first_child + i;

		has_children = true;
		deepest = max(deepest,
			      ctlv_schema_min_len(walk, tree, child,
						  above + sizeof(struct ctlv_hdr), depth + 1));
	}

	for (i = 0; i < tree->ext_schemas_count; i++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[i];
		const struct ctlv_ext_fill_schema *fill_schema;
		const struct ctlv_node_schema *root;
		u32 member;

		if (ext_schema->node != index)
			continue;
		fill_schema =
			ctlv_schema_fill(walk->family, ext_schema->family_id, ext_schema->ext_id);
		if (!fill_schema)
			continue;
		root = &fill_schema->members.node_schemas[0];
		for (member = 0; member < root->children_count; member++) {
			has_children = true;
			deepest = max(deepest, ctlv_schema_min_len(walk, &fill_schema->members,
								   root->first_child + member,
								   above + sizeof(struct ctlv_hdr),
								   depth + 1));
		}
	}

	if (has_children)
		return deepest;
	/* An empty root is a message of its own header alone. */
	if (!index)
		return above + sizeof(struct ctlv_hdr);
	return above + ALIGN(ctlv_schema_tlv_max_len(walk, tree, index, depth), CTLV_ALIGNTO);
}

static u64 ctlv_schema_root_max_len(struct ctlv_schema_length_walk *walk,
				    const struct ctlv_schema_tree *tree)
{
	return sizeof(struct ctlv_hdr) + ctlv_schema_children_max_len(walk, tree, 0, 0);
}

/* Size one deepest legal leaf and every record above it. */
static u64 ctlv_schema_reentering_min_len(struct ctlv_schema_length_walk *walk,
					  const struct ctlv_schema_tree *tree)
{
	u64 leaf = 0;
	u32 index;

	for (index = 1; index < tree->node_schemas_count; index++) {
		if (tree->node_schemas[index].kind == CTLV_SCHEMA_RECORD)
			continue;
		leaf = max(leaf,
			   ALIGN(ctlv_schema_tlv_max_len(walk, tree, index, 0), CTLV_ALIGNTO));
	}
	return (CTLV_MAX_RECORD_DEPTH + 1) * sizeof(struct ctlv_hdr) + leaf;
}

static bool ctlv_schema_tree_reenters(const struct ctlv_schema_tree *tree)
{
	u32 index;

	for (index = 0; index < tree->node_schemas_count; index++)
		if (tree->node_schemas[index].reenters)
			return true;
	return false;
}

int ctlv_schema_op_sizes(const struct ctlv_family *family, const struct ctlv_op_schema *op,
			 struct ctlv_op_sizes *sizes)
{
	struct ctlv_schema_length_walk walk = { .family = family };
	u64 request = 0, reply = 0;

	if (op->request.node_schemas)
		request = ctlv_schema_root_max_len(&walk, &op->request);

	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
		if (op->reply.node_schemas)
			reply = ctlv_schema_root_max_len(&walk, &op->reply);
		break;
	case CTLV_OP_TYPE_QUERY:
		if (ctlv_schema_tree_reenters(&op->reply)) {
			/* This size ignores extension points, which ctlvgen rejects here. */
			if (WARN_ON_ONCE(op->reply.ext_schemas_count))
				return -EINVAL;
			reply = ctlv_schema_reentering_min_len(&walk, &op->reply);
		} else {
			reply = ctlv_schema_min_len(&walk, &op->reply, 0, 0, 0);
		}
		break;
	default:
		return -EINVAL;
	}

	if (walk.bad || request > CTLV_MAX_INLINE_MESSAGE_LEN ||
	    reply > CTLV_MAX_INLINE_MESSAGE_LEN)
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
	}
	return 0;
}

/*
 * Only a completely valid request reaches family code; nothing is repaired or
 * normalized. The first failure is the one recorded, and the walk stops there.
 */

struct ctlv_schema_validation {
	struct ctlv_msg *msg;
	struct ctlv_error *error;
};

void ctlv_message_index_free(struct ctlv_msg *msg)
{
	struct ctlv_message_input_index *index;

	if (!msg || !msg->index)
		return;
	index = msg->index;
	kvfree(index->slots);
	kvfree(index->scopes);
	kvfree(index->records);
	kfree(index);
	msg->index = NULL;
}

/* kvrealloc() does not zero, so callers initialize every entry they add. */
static void *ctlv_schema_index_grow(void *items, u32 *capacity, u32 needed, size_t item_size)
{
	u32 grown = max3(needed, *capacity * 2, 16U);
	void *new;

	if (needed <= *capacity)
		return items;
	new = kvrealloc(items, array_size(grown, item_size), GFP_KERNEL_ACCOUNT);
	if (!new)
		return ERR_PTR(-ENOMEM);
	*capacity = grown;
	return new;
}

static int ctlv_schema_index_slots_add(struct ctlv_message_input_index *index, u32 count,
				       u32 *first)
{
	struct ctlv_message_input_slot *slots;
	u32 needed;
	u32 i;

	if (check_add_overflow(index->slots_count, count, &needed))
		return -EOVERFLOW;
	slots = ctlv_schema_index_grow(index->slots, &index->slots_capacity, needed,
				       sizeof(*slots));
	if (IS_ERR(slots))
		return PTR_ERR(slots);
	index->slots = slots;
	*first = index->slots_count;
	for (i = index->slots_count; i < needed; i++)
		index->slots[i] = (struct ctlv_message_input_slot){
			.excess_offset = CTLV_MESSAGE_INPUT_INDEX_NONE,
			.first_record = CTLV_MESSAGE_INPUT_INDEX_NONE,
			.last_record = CTLV_MESSAGE_INPUT_INDEX_NONE,
		};
	index->slots_count = needed;
	return 0;
}

static int ctlv_schema_index_scope_add(struct ctlv_schema_validation *v, u32 record_index,
				       const struct ctlv_schema_tree *tree, u32 node, bool optional)
{
	struct ctlv_message_input_index *index = v->msg->index;
	const struct ctlv_node_schema *parent = &tree->node_schemas[node];
	struct ctlv_message_input_scope *scopes;
	u32 needed;
	u32 first_slot;
	u32 scope_index;
	u32 i;
	int ret;

	if (check_add_overflow(index->scopes_count, 1U, &needed))
		return -EOVERFLOW;
	scopes = ctlv_schema_index_grow(index->scopes, &index->scopes_capacity, needed,
					sizeof(*scopes));
	if (IS_ERR(scopes))
		return PTR_ERR(scopes);
	index->scopes = scopes;

	ret = ctlv_schema_index_slots_add(index, parent->children_count, &first_slot);
	if (ret)
		return ret;

	scope_index = index->scopes_count++;
	index->scopes[scope_index] = (struct ctlv_message_input_scope){
		.tree = tree,
		.record = record_index,
		.node = node,
		.first_slot = first_slot,
		.slots_count = parent->children_count,
		.optional = optional,
	};
	index->records[record_index].scopes_count++;

	for (i = 0; i < tree->ext_schemas_count; i++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[i];
		const struct ctlv_ext_fill_schema *fill_schema;

		if (ext_schema->node != node)
			continue;
		fill_schema = ctlv_schema_fill(v->msg->family, ext_schema->family_id,
					       ext_schema->ext_id);
		if (!fill_schema)
			continue;
		ret = ctlv_schema_index_scope_add(v, record_index, &fill_schema->members, 0,
						  optional || ext_schema->optional);
		if (ret)
			return ret;
	}
	return 0;
}

static int ctlv_schema_index_record_add(struct ctlv_schema_validation *v,
					const struct ctlv_schema_tree *tree, u32 node,
					const struct ctlv_hdr *record_hdr, u32 *record_index)
{
	struct ctlv_message_input_index *index = v->msg->index;
	struct ctlv_message_input_record *records;
	u32 needed;
	u32 added;
	int ret;

	if (check_add_overflow(index->records_count, 1U, &needed))
		return -EOVERFLOW;
	records = ctlv_schema_index_grow(index->records, &index->records_capacity, needed,
					 sizeof(*records));
	if (IS_ERR(records))
		return PTR_ERR(records);
	index->records = records;
	added = index->records_count++;
	index->records[added] = (struct ctlv_message_input_record){
		.offset = ctlv_message_offset(v->msg, record_hdr),
		.first_scope = index->scopes_count,
		.next = CTLV_MESSAGE_INPUT_INDEX_NONE,
	};
	ret = ctlv_schema_index_scope_add(v, added, tree, node, false);
	if (ret)
		return ret;
	*record_index = added;
	return 0;
}

/* A request is never a fragment: only a reply of a query is ever incomplete. */
#define CTLV_SCHEMA_HDR_F_REQUEST (CTLV_HDR_F_RECORD | CTLV_HDR_F_IGNORE_IF_UNKNOWN)
#define CTLV_SCHEMA_HDR_F_REPLY CTLV_HDR_F_RECORD

/*
 * Record the first failure and refuse the message. @hdr is NULL when what is
 * wrong with the record is something it does not carry.
 */
static int ctlv_schema_bad(struct ctlv_schema_validation *v, u32 reason,
			   const struct ctlv_hdr *record, const struct ctlv_hdr *hdr)
{
	*v->error = (struct ctlv_error){
		.reason = reason,
		.bad_tlv_offset = ctlv_message_offset(v->msg, hdr ?: record),
		.record_offset = ctlv_message_offset(v->msg, record),
	};
	return -EINVAL;
}

/* The one failure that names a field the message does not contain. */
static int ctlv_schema_bad_missing(struct ctlv_schema_validation *v, const struct ctlv_hdr *record,
				   u64 type)
{
	ctlv_schema_bad(v, CTLV_VALIDATION_REASON_MISSING_FIELD, record, NULL);
	v->error->missing_type = type;
	return -EINVAL;
}

static struct ctlv_message_input_slot *ctlv_schema_index_slot(struct ctlv_schema_validation *v,
							      u32 record_index,
							      const struct ctlv_schema_pos *pos)
{
	struct ctlv_message_input_index *index = v->msg->index;
	const struct ctlv_message_input_record *record;
	u32 i;

	if (WARN_ON_ONCE(record_index >= index->records_count))
		return NULL;
	record = &index->records[record_index];
	for (i = 0; i < record->scopes_count; i++) {
		const struct ctlv_message_input_scope *scope =
			&index->scopes[record->first_scope + i];
		const struct ctlv_node_schema *parent;
		u32 slot;

		if (scope->tree != pos->tree)
			continue;
		parent = &scope->tree->node_schemas[scope->node];
		if (pos->node < parent->first_child ||
		    pos->node >= parent->first_child + scope->slots_count)
			continue;
		slot = pos->node - parent->first_child;
		return &index->slots[scope->first_slot + slot];
	}
	CTLV_ASSERT(0);
	return NULL;
}

static int ctlv_schema_index_tlv_add(struct ctlv_schema_validation *v, u32 record_index,
				     const struct ctlv_schema_pos *pos, const struct ctlv_hdr *hdr,
				     bool *seen)
{
	struct ctlv_message_input_slot *slot = ctlv_schema_index_slot(v, record_index, pos);
	const struct ctlv_node_schema *schema = &pos->tree->node_schemas[pos->node];
	u32 offset = ctlv_message_offset(v->msg, hdr);

	if (!slot)
		return -EINVAL;
	*seen = slot->count;
	if (!slot->count)
		slot->first_offset = offset;
	slot->count++;
	if (schema->max_occurrences && slot->count > schema->max_occurrences &&
	    slot->excess_offset == CTLV_MESSAGE_INPUT_INDEX_NONE)
		slot->excess_offset = offset;
	slot->end_offset = offset + ALIGN((size_t)hdr->len, CTLV_ALIGNTO);
	return 0;
}

static void ctlv_schema_index_record_link(struct ctlv_message_input_index *index,
					  struct ctlv_message_input_slot *slot, u32 record_index)
{
	if (slot->first_record == CTLV_MESSAGE_INPUT_INDEX_NONE)
		slot->first_record = record_index;
	else
		index->records[slot->last_record].next = record_index;
	slot->last_record = record_index;
}

static int ctlv_schema_index_required(struct ctlv_schema_validation *v, u32 record_index,
				      const struct ctlv_hdr *record_hdr)
{
	const struct ctlv_message_input_index *index = v->msg->index;
	const struct ctlv_message_input_record *record = &index->records[record_index];
	u32 i;

	for (i = 0; i < record->scopes_count; i++) {
		const struct ctlv_message_input_scope *scope =
			&index->scopes[record->first_scope + i];
		const struct ctlv_node_schema *parent = &scope->tree->node_schemas[scope->node];
		u32 slot;

		for (slot = 0; slot < scope->slots_count; slot++) {
			const struct ctlv_message_input_slot *indexed;
			const struct ctlv_node_schema *child =
				&scope->tree->node_schemas[parent->first_child + slot];
			const struct ctlv_hdr *excess;

			indexed = &index->slots[scope->first_slot + slot];
			if (!indexed->count && slot < parent->required_children_count &&
			    !scope->optional)
				return ctlv_schema_bad_missing(v, record_hdr, child->type);
			if (!child->max_occurrences || indexed->count <= child->max_occurrences)
				continue;
			if (WARN_ON_ONCE(indexed->excess_offset == CTLV_MESSAGE_INPUT_INDEX_NONE))
				return -EINVAL;
			excess = (const void *)((const u8 *)v->msg->root + indexed->excess_offset);
			return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES,
					       record_hdr, excess);
		}
	}
	return 0;
}

/* The bits a TLV of this message may carry, whichever way it travelled. */
static u32 ctlv_schema_flags_allowed(const struct ctlv_schema_validation *v)
{
	return v->msg->request ? CTLV_SCHEMA_HDR_F_REQUEST : CTLV_SCHEMA_HDR_F_REPLY;
}

static u64 ctlv_schema_scalar_value(const struct ctlv_hdr *hdr)
{
	/* Aligned by the message format: the root began an allocation. */
	return *(const u64 *)ctlv_message_hdr_payload(hdr);
}

bool ctlv_schema_value_ok(const struct ctlv_family *family,
			  const struct ctlv_node_schema *node, u64 value)
{
	switch (node->kind) {
	case CTLV_SCHEMA_SINT:
		return (s64)value >= (s64)node->range.min && (s64)value <= (s64)node->range.max;
	case CTLV_SCHEMA_FLAGS:
		/* A bit no family declared is one nothing can be told about. */
		return !(value & ~node->flags_mask);
	case CTLV_SCHEMA_BOOL:
		return value <= 1;
	case CTLV_SCHEMA_ENUM:
		/*
		 * An enum's values are numbered in declaration order and never
		 * skipped, so its range bounds it like an integer.
		 */
		if (node->is_enum_ext)
			return value >= 1 && value <= ctlv_schema_enum_max(family, node);
		return value >= node->range.min && value <= node->range.max;
	default:
		return value >= node->range.min && value <= node->range.max;
	}
}

bool ctlv_schema_length_ok(const struct ctlv_node_schema *node, u32 len)
{
	return len >= node->length.min_len && len <= node->length.max_len;
}

static int ctlv_schema_check_integer(struct ctlv_schema_validation *v,
				     const struct ctlv_node_schema *node,
				     const struct ctlv_hdr *record, const struct ctlv_hdr *hdr)
{
	u64 value;

	if (ctlv_message_hdr_payload_len(hdr) != sizeof(value))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, record, hdr);
	value = ctlv_schema_scalar_value(hdr);
	if (ctlv_schema_value_ok(v->msg->family, node, value))
		return 0;
	if (node->kind == CTLV_SCHEMA_FLAGS)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_FLAG, record, hdr);
	return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE, record, hdr);
}

static int ctlv_schema_check_bool(struct ctlv_schema_validation *v,
				  const struct ctlv_node_schema *node,
				  const struct ctlv_hdr *record,
				  const struct ctlv_hdr *hdr)
{
	const u8 *value = ctlv_message_hdr_payload(hdr);

	if (ctlv_message_hdr_payload_len(hdr) != sizeof(*value))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, record, hdr);
	if (!ctlv_schema_value_ok(v->msg->family, node, *value))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE, record, hdr);
	return 0;
}

static int ctlv_schema_check_string(struct ctlv_schema_validation *v,
				    const struct ctlv_node_schema *node,
				    const struct ctlv_hdr *record, const struct ctlv_hdr *hdr)
{
	u32 len = ctlv_message_hdr_payload_len(hdr);
	const char *value = ctlv_message_hdr_payload(hdr);

	/*
	 * One terminating NUL and no other, so a family may hand the payload
	 * to anything that takes a C string.
	 */
	if (!len || value[len - 1] || strnlen(value, len) != len - 1)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_STRING, record, hdr);
	if (!ctlv_schema_length_ok(node, len - 1))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, record, hdr);
	return 0;
}

static int ctlv_schema_check_binary(struct ctlv_schema_validation *v,
				    const struct ctlv_node_schema *node,
				    const struct ctlv_hdr *record, const struct ctlv_hdr *hdr)
{
	if (!ctlv_schema_length_ok(node, ctlv_message_hdr_payload_len(hdr)))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_LENGTH, record, hdr);
	return 0;
}

/*
 * The one shape the tree cannot describe: a record of core-owned fields
 * whose members depend on its type. Nothing here acquires anything.
 */
static int ctlv_schema_check_blob(struct ctlv_schema_validation *v,
				  const struct ctlv_node_schema *node, const struct ctlv_hdr *blob)
{
	const struct ctlv_hdr *bad = NULL;
	struct ctlv_blob_desc desc;
	u32 reason;
	int err;

	reason = ctlv_blob_desc_read(blob, &desc, &bad);
	if (reason)
		return ctlv_schema_bad(v, reason, blob, bad);
	err = ctlv_blob_desc_check(&desc, node);
	if (err)
		ctlv_schema_bad(v, CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR, blob, NULL);
	return err;
}

static int ctlv_schema_check_record(struct ctlv_schema_validation *v,
				    const struct ctlv_schema_tree *tree, u32 node,
				    const struct ctlv_hdr *record, u32 depth);
static int ctlv_schema_check_indexed_record(struct ctlv_schema_validation *v,
					    const struct ctlv_schema_tree *tree, u32 node,
					    const struct ctlv_hdr *record, u32 depth,
					    u32 *record_index);

static int ctlv_schema_check_tlv(struct ctlv_schema_validation *v,
				 const struct ctlv_schema_pos *pos, const struct ctlv_hdr *record,
				 const struct ctlv_hdr *hdr, u32 depth, u32 *record_index)
{
	const struct ctlv_node_schema *node = &pos->tree->node_schemas[pos->node];
	bool is_record = node->kind == CTLV_SCHEMA_RECORD || node->kind == CTLV_SCHEMA_BLOB;

	if (hdr->flags & ~ctlv_schema_flags_allowed(v))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_FLAG, record, hdr);
	/* The schema says whether a TLV is a record, not the sender. */
	if (!(hdr->flags & CTLV_HDR_F_RECORD) != !is_record)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE, record, hdr);

	switch (node->kind) {
	case CTLV_SCHEMA_BOOL:
		return ctlv_schema_check_bool(v, node, record, hdr);
	case CTLV_SCHEMA_STRING:
		return ctlv_schema_check_string(v, node, record, hdr);
	case CTLV_SCHEMA_BINARY:
		return ctlv_schema_check_binary(v, node, record, hdr);
	case CTLV_SCHEMA_BLOB:
		return ctlv_schema_check_blob(v, node, hdr);
	case CTLV_SCHEMA_RECORD:
		if (v->msg->request)
			return ctlv_schema_check_indexed_record(v, pos->tree, pos->node, hdr,
								depth + 1, record_index);
		return ctlv_schema_check_record(v, pos->tree, pos->node, hdr, depth + 1);
	default:
		return ctlv_schema_check_integer(v, node, record, hdr);
	}
}

/* How many times one member occurs, and where the one too many is. */
static u32 ctlv_schema_occurrences(const struct ctlv_hdr *record, u64 type, u32 max,
				   const struct ctlv_hdr **excess)
{
	const struct ctlv_hdr *child = NULL;
	u32 count = 0;

	while (ctlv_message_next(record, &child) == CTLV_MESSAGE_STEP_TLV) {
		if (child->type != type)
			continue;
		count++;
		if (max && count == max + 1)
			*excess = child;
	}
	return count;
}

/*
 * An optional extension point makes everything it was filled with optional
 * too: what a descendant added cannot be something an ancestor's caller had
 * to send.
 */
static int ctlv_schema_check_members(struct ctlv_schema_validation *v,
				     const struct ctlv_schema_tree *tree, u32 node,
				     const struct ctlv_hdr *record, bool optional)
{
	const struct ctlv_node_schema *parent = &tree->node_schemas[node];
	u32 index;

	for (index = 0; index < parent->children_count; index++) {
		const struct ctlv_hdr *excess = NULL;
		const struct ctlv_node_schema *child;
		u32 count;

		child = &tree->node_schemas[parent->first_child + index];
		count = ctlv_schema_occurrences(record, child->type, child->max_occurrences,
						&excess);
		if (!count) {
			if (index < parent->required_children_count && !optional)
				return ctlv_schema_bad_missing(v, record, child->type);
			continue;
		}
		/* An unlimited node, which only a reply may hold. */
		if (child->max_occurrences && count > child->max_occurrences)
			return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES,
					       record, excess);
	}

	for (index = 0; index < tree->ext_schemas_count; index++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[index];
		const struct ctlv_ext_fill_schema *fill_schema;
		int ret;

		if (ext_schema->node != node)
			continue;
		fill_schema = ctlv_schema_fill(v->msg->family, ext_schema->family_id,
					       ext_schema->ext_id);
		if (!fill_schema)
			continue;
		ret = ctlv_schema_check_members(v, &fill_schema->members, 0, record,
						optional || ext_schema->optional);
		if (ret)
			return ret;
	}
	return 0;
}

/* Whether one tree places one type anywhere, fills included. */
static bool ctlv_schema_tree_declares(const struct ctlv_family *family,
				      const struct ctlv_schema_tree *tree, u64 type)
{
	u32 index;

	for (index = 0; index < tree->node_schemas_count; index++)
		if (tree->node_schemas[index].type == type)
			return true;
	for (index = 0; index < tree->ext_schemas_count; index++) {
		const struct ctlv_ext_schema *ext_schema = &tree->ext_schemas[index];
		const struct ctlv_ext_fill_schema *fill_schema;

		fill_schema = ctlv_schema_fill(family, ext_schema->family_id, ext_schema->ext_id);
		if (fill_schema &&
		    ctlv_schema_tree_declares(family, &fill_schema->members, type))
			return true;
	}
	return false;
}

/* Search every type known anywhere in the effective schema. */
static bool ctlv_schema_declares(const struct ctlv_family *family, u64 type)
{
	const struct ctlv_family *owner;
	struct ctlv_schema_cursor cursor;
	const struct ctlv_op_schema *op;

	ctlv_schema_cursor_init(&cursor, family);
	while ((op = ctlv_schema_next_op(&cursor, &owner)))
		if (ctlv_schema_tree_declares(family, &op->request, type) ||
		    ctlv_schema_tree_declares(family, &op->reply, type))
			return true;
	return false;
}

/*
 * Unknown IDs remain reserved unless explicitly ignorable; ignored unknown
 * records are opaque.
 */
static int ctlv_schema_check_unknown(struct ctlv_schema_validation *v,
				     const struct ctlv_hdr *record,
				     const struct ctlv_hdr *hdr)
{
	/* Which refuses the bit itself on a reply, where it means nothing. */
	if (hdr->flags & ~ctlv_schema_flags_allowed(v))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_FLAG, record, hdr);
	if (!(hdr->flags & CTLV_HDR_F_IGNORE_IF_UNKNOWN) ||
	    ctlv_schema_declares(v->msg->family, hdr->type))
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_UNKNOWN_TYPE, record, hdr);
	return 0;
}

static int ctlv_schema_check_indexed_record(struct ctlv_schema_validation *v,
					    const struct ctlv_schema_tree *tree, u32 node,
					    const struct ctlv_hdr *record, u32 depth,
					    u32 *record_index)
{
	struct ctlv_message_input_index *index = v->msg->index;
	const struct ctlv_hdr *child = NULL;
	const struct ctlv_hdr *split = NULL;
	u64 previous = 0;
	bool previous_valid = false;
	u32 added;
	enum ctlv_message_step step;
	int ret;

	if (depth > CTLV_MAX_RECORD_DEPTH)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_RECORD_TOO_DEEP, record, NULL);
	ret = ctlv_schema_index_record_add(v, tree, node, record, &added);
	if (ret)
		return ret;

	while ((step = ctlv_message_next(record, &child)) == CTLV_MESSAGE_STEP_TLV) {
		struct ctlv_schema_pos pos;
		bool seen;

		if (!ctlv_schema_member(v->msg->family, tree, node, child->type, &pos)) {
			previous_valid = false;
			continue;
		}
		ret = ctlv_schema_index_tlv_add(v, added, &pos, child, &seen);
		if (ret)
			return ret;
		if (seen && (!previous_valid || previous != child->type) && !split)
			split = child;
		previous = child->type;
		previous_valid = true;
	}

	ret = ctlv_schema_index_required(v, added, record);
	if (ret)
		return ret;
	if (step == CTLV_MESSAGE_STEP_BAD)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE, record, NULL);

	child = NULL;
	while (ctlv_message_next(record, &child) == CTLV_MESSAGE_STEP_TLV) {
		struct ctlv_schema_pos pos;
		struct ctlv_message_input_slot *slot;
		u32 child_record = CTLV_MESSAGE_INPUT_INDEX_NONE;

		if (!ctlv_schema_member(v->msg->family, tree, node, child->type, &pos)) {
			ret = ctlv_schema_check_unknown(v, record, child);
			if (ret)
				return ret;
			continue;
		}
		if (child == split)
			return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_NONCONTIGUOUS_TYPE, record,
					       child);
		ret = ctlv_schema_check_tlv(v, &pos, record, child, depth, &child_record);
		if (ret)
			return ret;
		if (child_record != CTLV_MESSAGE_INPUT_INDEX_NONE) {
			slot = ctlv_schema_index_slot(v, added, &pos);
			if (!slot)
				return -EINVAL;
			ctlv_schema_index_record_link(index, slot, child_record);
		}
	}
	*record_index = added;
	return 0;
}

/*
 * Required members first, then every TLV in the order the record carries them,
 * so that what is reported is the first thing a caller has to fix.
 */
static int ctlv_schema_check_record(struct ctlv_schema_validation *v,
				    const struct ctlv_schema_tree *tree, u32 node,
				    const struct ctlv_hdr *record, u32 depth)
{
	const struct ctlv_hdr *child = NULL;
	enum ctlv_message_step step;
	int ret;

	if (depth > CTLV_MAX_RECORD_DEPTH)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_RECORD_TOO_DEEP, record, NULL);

	ret = ctlv_schema_check_members(v, tree, node, record, false);
	if (ret)
		return ret;

	while ((step = ctlv_message_next(record, &child)) == CTLV_MESSAGE_STEP_TLV) {
		struct ctlv_schema_pos pos;

		if (!ctlv_schema_member(v->msg->family, tree, node, child->type, &pos)) {
			ret = ctlv_schema_check_unknown(v, record, child);
			if (ret)
				return ret;
			/* Skipped whole: the step past it checked its length. */
			continue;
		}
		ret = ctlv_schema_check_tlv(v, &pos, record, child, depth, NULL);
		if (ret)
			return ret;
	}
	if (step == CTLV_MESSAGE_STEP_BAD)
		return ctlv_schema_bad(v, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE, record, NULL);
	return 0;
}

int ctlv_schema_validate(struct ctlv_msg *msg, struct ctlv_error *error)
{
	struct ctlv_schema_validation validate = { .msg = msg, .error = error };
	u32 root_index;
	int ret;

	if (!msg->request)
		return ctlv_schema_check_record(&validate, msg->tree, 0, msg->root, 0);
	if (WARN_ON_ONCE(msg->index))
		return -EINVAL;
	msg->index = kzalloc_obj(*msg->index, GFP_KERNEL_ACCOUNT);
	if (!msg->index)
		return -ENOMEM;
	ret = ctlv_schema_check_indexed_record(&validate, msg->tree, 0, msg->root, 0, &root_index);
	if (ret || WARN_ON_ONCE(root_index)) {
		ctlv_message_index_free(msg);
		return ret ?: -EINVAL;
	}
	return 0;
}
