// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Message walking, indexing, and generated-accessor primitives.
 */
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/bug.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/export.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "internal.h"
#include "message.h"

enum ctlv_message_step ctlv_message_next(const struct ctlv_attr *nest,
					 const struct ctlv_attr **child)
{
	const u8 *end = (const u8 *)nest + nest->len;
	const u8 *next;
	size_t room;

	if (nest->len < sizeof(*nest))
		return CTLV_MESSAGE_STEP_BAD;
	if (*child)
		next = (const u8 *)*child + ALIGN((size_t)(*child)->len, CTLV_ALIGNTO);
	else
		next = (const u8 *)(nest + 1);
	if (next >= end)
		return next == end ? CTLV_MESSAGE_STEP_END : CTLV_MESSAGE_STEP_BAD;

	room = end - next;
	if (room < sizeof(**child))
		return CTLV_MESSAGE_STEP_BAD;
	*child = (const struct ctlv_attr *)next;
	/*
	 * Length before alignment, so that aligning a length this nest could
	 * never hold is not what overflows.
	 */
	if ((*child)->len < sizeof(**child) || (*child)->len > room)
		return CTLV_MESSAGE_STEP_BAD;
	if (ALIGN((size_t)(*child)->len, CTLV_ALIGNTO) > room)
		return CTLV_MESSAGE_STEP_BAD;
	return CTLV_MESSAGE_STEP_ATTR;
}

u32 ctlv_message_offset(const struct ctlv_msg *msg, const struct ctlv_attr *attr)
{
	return (const u8 *)attr - (const u8 *)msg->root;
}

void ctlv_message_root(struct ctlv_cursor *cursor, struct ctlv_msg *msg)
{
	*cursor = (struct ctlv_cursor){
		.msg = msg,
		.attr = msg->root,
		.tree = msg->tree,
		.index_nest_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
	};
	if (msg->request && msg->index)
		cursor->index_nest_scope = msg->index->nests[0].first_scope;
}

static bool ctlv_message_nest_index_ok(const struct ctlv_cursor *nest)
{
	const struct ctlv_message_input_index *index = nest->msg->index;
	const struct ctlv_message_input_nest *indexed;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_attr *attr;

	if (WARN_ON_ONCE(!nest->msg->request || !index ||
			 nest->index_nest_scope >= index->scopes_count))
		return false;
	scope = &index->scopes[nest->index_nest_scope];
	if (WARN_ON_ONCE(scope->nest >= index->nests_count))
		return false;
	indexed = &index->nests[scope->nest];
	attr = (const void *)((const u8 *)nest->msg->root + indexed->offset);
	if (WARN_ON_ONCE(attr != nest->attr))
		return false;
	return !WARN_ON_ONCE(scope->tree != nest->tree || scope->node != nest->node);
}

static bool ctlv_message_input_scope_ok(const struct ctlv_message_input_index *index,
					const struct ctlv_message_input_scope *scope)
{
	return !WARN_ON_ONCE(scope->first_slot > index->slots_count ||
			     scope->slots_count > index->slots_count - scope->first_slot);
}

static const struct ctlv_node_schema *ctlv_message_input_ref_node(struct ctlv_input_ref ref)
{
	const struct ctlv_node_schema *parent;
	const struct ctlv_node_schema *node;

	if (WARN_ON_ONCE(!ref.tree->node_schemas || ref.node >= ref.tree->node_schemas_count))
		return NULL;
	parent = &ref.tree->node_schemas[ref.node];
	if (WARN_ON_ONCE(ref.slot >= parent->children_count ||
			 parent->first_child + ref.slot >= ref.tree->node_schemas_count))
		return NULL;
	node = &ref.tree->node_schemas[parent->first_child + ref.slot];
	if (WARN_ON_ONCE(node->attr_id != ref.attr_id || node->kind != ref.kind))
		return NULL;
	return node;
}

static const struct ctlv_message_input_slot *
ctlv_message_cursor_index(const struct ctlv_cursor *cursor)
{
	const struct ctlv_message_input_index *index;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_node_schema *parent;
	const struct ctlv_node_schema *node;
	const struct ctlv_message_input_slot *slot;
	const struct ctlv_message_input_nest *nest;
	u32 local_slot;
	u32 offset;

	if (WARN_ON_ONCE(!cursor->msg || !cursor->tree || !cursor->msg->request ||
			 !cursor->msg->index))
		return NULL;
	index = cursor->msg->index;
	if (WARN_ON_ONCE(cursor->index_scope >= index->scopes_count))
		return NULL;
	scope = &index->scopes[cursor->index_scope];
	if (!ctlv_message_input_scope_ok(index, scope))
		return NULL;
	parent = &scope->tree->node_schemas[scope->node];
	if (WARN_ON_ONCE(scope->tree != cursor->tree || cursor->node < parent->first_child ||
			 cursor->node >= parent->first_child + scope->slots_count))
		return NULL;
	local_slot = cursor->node - parent->first_child;
	node = &cursor->tree->node_schemas[cursor->node];
	slot = &index->slots[scope->first_slot + local_slot];
	if (!cursor->attr)
		return slot;
	if (WARN_ON_ONCE(cursor->attr->attr_id != node->attr_id))
		return NULL;
	if (WARN_ON_ONCE(scope->nest >= index->nests_count))
		return NULL;
	nest = &index->nests[scope->nest];
	if (WARN_ON_ONCE(cursor->parent !=
			 (const void *)((const u8 *)cursor->msg->root + nest->offset)))
		return NULL;
	offset = ctlv_message_offset(cursor->msg, cursor->attr);
	if (WARN_ON_ONCE(offset < slot->first_offset || offset >= slot->end_offset ||
			 offset + ALIGN((size_t)cursor->attr->len, CTLV_ALIGNTO) >
				 slot->end_offset))
		return NULL;
	if (node->kind == CTLV_SCHEMA_NEST) {
		const struct ctlv_message_input_scope *nest_scope;

		if (WARN_ON_ONCE(cursor->index_nest_scope >= index->scopes_count))
			return NULL;
		nest_scope = &index->scopes[cursor->index_nest_scope];
		if (WARN_ON_ONCE(nest_scope->nest >= index->nests_count ||
				 index->nests[nest_scope->nest].offset != offset ||
				 nest_scope->tree != cursor->tree ||
				 nest_scope->node != cursor->node))
			return NULL;
	} else if (WARN_ON_ONCE(cursor->index_nest_scope != CTLV_MESSAGE_INPUT_INDEX_NONE)) {
		return NULL;
	}
	return slot;
}

bool ctlv_message_input_rebind(struct ctlv_cursor *nest, const struct ctlv_schema_tree *tree,
			       u32 node)
{
	const struct ctlv_message_input_scope *bound_scope;
	const struct ctlv_message_input_nest *indexed;
	u32 i;

	if (WARN_ON_ONCE(!nest->attr || !nest->msg || !nest->msg->index))
		return false;
	if (!ctlv_message_nest_index_ok(nest))
		return false;
	bound_scope = &nest->msg->index->scopes[nest->index_nest_scope];
	indexed = &nest->msg->index->nests[bound_scope->nest];
	for (i = 0; i < indexed->scopes_count; i++) {
		u32 scope_index = indexed->first_scope + i;
		const struct ctlv_message_input_scope *scope =
			&nest->msg->index->scopes[scope_index];

		if (scope->tree != tree || scope->node != node)
			continue;
		nest->tree = tree;
		nest->node = node;
		nest->index_nest_scope = scope_index;
		return true;
	}
	return false;
}

struct ctlv_cursor *ctlv_input_attr(struct ctlv_cursor *cursor, const struct ctlv_cursor *nest,
				    struct ctlv_input_ref ref)
{
	const struct ctlv_message_input_index *index;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_message_input_slot *indexed;
	const struct ctlv_node_schema *schema;

	*cursor = (struct ctlv_cursor){
		.index_nest_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
	};
	if (WARN_ON_ONCE(!nest->attr || !nest->msg))
		return cursor;
	if (!ctlv_message_nest_index_ok(nest))
		return cursor;
	index = nest->msg->index;
	if (WARN_ON_ONCE(nest->index_nest_scope >= index->scopes_count))
		return cursor;
	scope = &index->scopes[nest->index_nest_scope];
	if (!ctlv_message_input_scope_ok(index, scope))
		return cursor;
	if (WARN_ON_ONCE(scope->tree != ref.tree || scope->node != ref.node ||
			 nest->tree != ref.tree || nest->node != ref.node ||
			 ref.slot >= scope->slots_count))
		return cursor;
	schema = ctlv_message_input_ref_node(ref);
	if (!schema)
		return cursor;
	indexed = &index->slots[scope->first_slot + ref.slot];
	*cursor = (struct ctlv_cursor){
		.msg = nest->msg,
		.parent = nest->attr,
		.attr = indexed->count ? (const void *)((const u8 *)nest->msg->root +
							indexed->first_offset) :
					 NULL,
		.tree = ref.tree,
		.node = ref.tree->node_schemas[ref.node].first_child + ref.slot,
		.depth = nest->depth + 1,
		.index_nest_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = nest->index_nest_scope,
	};
	if (indexed->count && schema->kind == CTLV_SCHEMA_NEST) {
		if (WARN_ON_ONCE(indexed->first_nest >= index->nests_count))
			return cursor;
		cursor->index_nest_scope = index->nests[indexed->first_nest].first_scope;
	}
	return cursor;
}
EXPORT_SYMBOL_NS_GPL(ctlv_input_attr, "CTLV");

bool ctlv_input_cursor(const struct ctlv_cursor *cursor, struct ctlv_input_ref ref)
{
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_node_schema *schema;

	if (!ctlv_message_cursor_index(cursor))
		return false;
	schema = ctlv_message_input_ref_node(ref);
	if (!schema)
		return false;
	scope = &cursor->msg->index->scopes[cursor->index_scope];
	return !WARN_ON_ONCE(scope->tree != ref.tree ||
			     scope->node != ref.node ||
			     cursor->tree != ref.tree ||
			     cursor->node !=
				     ref.tree->node_schemas[ref.node].first_child +
					     ref.slot);
}
EXPORT_SYMBOL_NS_GPL(ctlv_input_cursor, "CTLV");

void ctlv_attr_next(struct ctlv_cursor *cursor)
{
	const struct ctlv_message_input_slot *slot;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_node_schema *node;
	const struct ctlv_attr *next;
	u32 next_nest;
	u32 next_offset;

	slot = ctlv_message_cursor_index(cursor);
	if (!slot || WARN_ON_ONCE(!cursor->attr))
		return;
	next = (const void *)((const u8 *)cursor->attr +
			      ALIGN((size_t)cursor->attr->len, CTLV_ALIGNTO));
	next_offset = ctlv_message_offset(cursor->msg, next);
	if (next_offset == slot->end_offset) {
		cursor->attr = NULL;
		cursor->index_nest_scope = CTLV_MESSAGE_INPUT_INDEX_NONE;
		return;
	}
	if (WARN_ON_ONCE(next_offset > slot->end_offset))
		return;
	cursor->attr = next;
	node = &cursor->tree->node_schemas[cursor->node];
	if (node->kind == CTLV_SCHEMA_NEST) {
		if (WARN_ON_ONCE(cursor->index_nest_scope >= cursor->msg->index->scopes_count))
			return;
		scope = &cursor->msg->index->scopes[cursor->index_nest_scope];
		if (WARN_ON_ONCE(scope->nest >= cursor->msg->index->nests_count))
			return;
		next_nest = cursor->msg->index->nests[scope->nest].next;
		if (WARN_ON_ONCE(next_nest == CTLV_MESSAGE_INPUT_INDEX_NONE ||
				 next_nest >= cursor->msg->index->nests_count))
			return;
		cursor->index_nest_scope = cursor->msg->index->nests[next_nest].first_scope;
	}
	WARN_ON_ONCE(!ctlv_message_cursor_index(cursor));
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_next, "CTLV");

bool ctlv_attr_ok(const struct ctlv_cursor *cursor)
{
	return cursor && cursor->attr && ctlv_message_cursor_index(cursor);
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_ok, "CTLV");

/* The payload, checked against the kinds a reader of this C type accepts. */
static const void *ctlv_message_cursor_payload(const struct ctlv_cursor *cursor, u32 kinds,
					       u32 *len)
{
	const struct ctlv_node_schema *node;

	if (!ctlv_message_cursor_index(cursor) || WARN_ON_ONCE(!cursor->attr))
		return NULL;
	node = &cursor->tree->node_schemas[cursor->node];
	if (WARN_ON_ONCE(!(kinds & BIT(node->kind))))
		return NULL;
	*len = ctlv_message_attr_payload_len(cursor->attr);
	return ctlv_message_attr_payload(cursor->attr);
}

/* A length disagreeing with a validated kind is this core's mistake. */
static const void *ctlv_message_cursor_fixed(const struct ctlv_cursor *cursor, u32 kinds,
					     u32 expected)
{
	const void *payload;
	u32 len;

	payload = ctlv_message_cursor_payload(cursor, kinds, &len);
	if (!payload || WARN_ON_ONCE(len != expected))
		return NULL;
	return payload;
}

u64 ctlv_attr_u64(const struct ctlv_cursor *cursor)
{
	const u64 *value = ctlv_message_cursor_fixed(cursor,
						     BIT(CTLV_SCHEMA_UINT) | BIT(CTLV_SCHEMA_ENUM) |
							     BIT(CTLV_SCHEMA_FLAGS),
						     sizeof(u64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_u64, "CTLV");

s64 ctlv_attr_s64(const struct ctlv_cursor *cursor)
{
	const s64 *value = ctlv_message_cursor_fixed(cursor, BIT(CTLV_SCHEMA_SINT), sizeof(s64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_s64, "CTLV");

bool ctlv_attr_bool(const struct ctlv_cursor *cursor)
{
	const u8 *value = ctlv_message_cursor_fixed(cursor, BIT(CTLV_SCHEMA_BOOL), sizeof(u8));

	return value ? *value : false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_bool, "CTLV");

const char *ctlv_attr_string(const struct ctlv_cursor *cursor)
{
	const char *value;
	u32 len;

	value = ctlv_message_cursor_payload(cursor, BIT(CTLV_SCHEMA_STRING), &len);
	if (!value || WARN_ON_ONCE(!len || value[len - 1]))
		return NULL;
	return value;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_string, "CTLV");

const void *ctlv_attr_binary(const struct ctlv_cursor *cursor, u32 *len)
{
	const void *value;

	*len = 0;
	value = ctlv_message_cursor_payload(cursor, BIT(CTLV_SCHEMA_BINARY), len);
	if (!value)
		*len = 0;
	return value;
}
EXPORT_SYMBOL_NS_GPL(ctlv_attr_binary, "CTLV");

/*
 * The room is measured before anything is written, which is what leaves a
 * refused attr's message exactly as it was, padding included.
 */
