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

#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "internal.h"
#include "message.h"

enum ctlv_message_step ctlv_message_next(const struct ctlv_hdr *record,
					 const struct ctlv_hdr **child)
{
	const u8 *end = (const u8 *)record + record->len;
	const u8 *next;
	size_t room;

	if (record->len < sizeof(*record))
		return CTLV_MESSAGE_STEP_BAD;
	if (*child)
		next = (const u8 *)*child + ALIGN((size_t)(*child)->len, CTLV_ALIGNTO);
	else
		next = (const u8 *)(record + 1);
	if (next >= end)
		return next == end ? CTLV_MESSAGE_STEP_END : CTLV_MESSAGE_STEP_BAD;

	room = end - next;
	if (room < sizeof(**child))
		return CTLV_MESSAGE_STEP_BAD;
	*child = (const struct ctlv_hdr *)next;
	/*
	 * Length before alignment, so that aligning a length this record could
	 * never hold is not what overflows.
	 */
	if ((*child)->len < sizeof(**child) || (*child)->len > room)
		return CTLV_MESSAGE_STEP_BAD;
	if (ALIGN((size_t)(*child)->len, CTLV_ALIGNTO) > room)
		return CTLV_MESSAGE_STEP_BAD;
	return CTLV_MESSAGE_STEP_TLV;
}

u32 ctlv_message_offset(const struct ctlv_msg *msg, const struct ctlv_hdr *hdr)
{
	return (const u8 *)hdr - (const u8 *)msg->root;
}

void ctlv_message_root(struct ctlv_cursor *cursor, struct ctlv_msg *msg)
{
	*cursor = (struct ctlv_cursor){
		.msg = msg,
		.hdr = msg->root,
		.tree = msg->tree,
		.index_record_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
	};
	if (msg->request && msg->index)
		cursor->index_record_scope = msg->index->records[0].first_scope;
}

static bool ctlv_message_record_index_ok(const struct ctlv_cursor *record)
{
	const struct ctlv_message_input_index *index = record->msg->index;
	const struct ctlv_message_input_record *indexed;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_hdr *hdr;

	if (WARN_ON_ONCE(!record->msg->request || !index ||
			 record->index_record_scope >= index->scopes_count))
		return false;
	scope = &index->scopes[record->index_record_scope];
	if (WARN_ON_ONCE(scope->record >= index->records_count))
		return false;
	indexed = &index->records[scope->record];
	hdr = (const void *)record->msg->root + indexed->offset;
	if (WARN_ON_ONCE(hdr != record->hdr))
		return false;
	return !WARN_ON_ONCE(scope->tree != record->tree || scope->node != record->node);
}

static bool ctlv_message_input_scope_ok(const struct ctlv_message_input_index *index,
					const struct ctlv_message_input_scope *scope)
{
	return !WARN_ON_ONCE(scope->first_slot > index->slots_count ||
			     scope->slots_count > index->slots_count - scope->first_slot);
}

/* A reader names a field its record lacks only by a family's mistake. */
static bool ctlv_message_input_slot(const struct ctlv_cursor *record, struct ctlv_input_ref ref,
				    u32 *slot)
{
	const struct ctlv_schema_tree *tree = record->tree;
	const struct ctlv_node_schema *parent;
	const struct ctlv_node_schema *node;

	if (WARN_ON_ONCE(!tree || !tree->node_schemas || record->node >= tree->node_schemas_count))
		return false;
	parent = &tree->node_schemas[record->node];
	if (WARN_ON_ONCE(parent->first_child > tree->node_schemas_count ||
			 parent->children_count > tree->node_schemas_count - parent->first_child))
		return false;
	for (*slot = 0; *slot < parent->children_count; (*slot)++) {
		node = &tree->node_schemas[parent->first_child + *slot];
		if (node->type == ref.type)
			return !WARN_ON_ONCE(node->kind != ref.kind);
	}
	WARN_ON_ONCE(1);
	return false;
}

static const struct ctlv_message_input_slot *
ctlv_message_cursor_index(const struct ctlv_cursor *cursor)
{
	const struct ctlv_message_input_index *index;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_node_schema *parent;
	const struct ctlv_node_schema *node;
	const struct ctlv_message_input_slot *slot;
	const struct ctlv_message_input_record *record;
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
	if (!cursor->hdr)
		return slot;

	if (WARN_ON_ONCE(cursor->hdr->type != node->type))
		return NULL;
	if (WARN_ON_ONCE(scope->record >= index->records_count))
		return NULL;
	record = &index->records[scope->record];
	if (WARN_ON_ONCE(cursor->parent != (const void *)cursor->msg->root + record->offset))
		return NULL;
	offset = ctlv_message_offset(cursor->msg, cursor->hdr);
	if (WARN_ON_ONCE(offset < slot->first_offset || offset >= slot->end_offset ||
			 offset + ALIGN((size_t)cursor->hdr->len, CTLV_ALIGNTO) >
				 slot->end_offset))
		return NULL;

	if (node->kind == CTLV_SCHEMA_RECORD) {
		const struct ctlv_message_input_scope *record_scope;

		if (WARN_ON_ONCE(cursor->index_record_scope >= index->scopes_count))
			return NULL;
		record_scope = &index->scopes[cursor->index_record_scope];
		if (WARN_ON_ONCE(record_scope->record >= index->records_count ||
				 index->records[record_scope->record].offset != offset ||
				 record_scope->tree != cursor->tree ||
				 record_scope->node != cursor->node))
			return NULL;
	} else if (WARN_ON_ONCE(cursor->index_record_scope != CTLV_MESSAGE_INPUT_INDEX_NONE)) {
		return NULL;
	}
	return slot;
}

bool ctlv_message_input_rebind(struct ctlv_cursor *record, const struct ctlv_schema_tree *tree,
			       u32 node)
{
	const struct ctlv_message_input_scope *bound_scope;
	const struct ctlv_message_input_record *indexed;

	if (WARN_ON_ONCE(!record->hdr || !record->msg || !record->msg->index))
		return false;
	if (!ctlv_message_record_index_ok(record))
		return false;
	bound_scope = &record->msg->index->scopes[record->index_record_scope];
	indexed = &record->msg->index->records[bound_scope->record];
	for (u32 i = 0; i < indexed->scopes_count; i++) {
		u32 scope_index = indexed->first_scope + i;
		const struct ctlv_message_input_scope *scope =
			&record->msg->index->scopes[scope_index];

		if (scope->tree != tree || scope->node != node)
			continue;
		record->tree = tree;
		record->node = node;
		record->index_record_scope = scope_index;
		return true;
	}
	return false;
}

struct ctlv_cursor *ctlv_input_field(struct ctlv_cursor *cursor, const struct ctlv_cursor *record,
				     struct ctlv_input_ref ref)
{
	const struct ctlv_message_input_index *index;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_message_input_slot *indexed;
	u32 slot;

	*cursor = (struct ctlv_cursor){
		.index_record_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
	};

	if (WARN_ON_ONCE(!record->hdr || !record->msg))
		return cursor;
	if (!ctlv_message_record_index_ok(record))
		return cursor;

	index = record->msg->index;
	if (WARN_ON_ONCE(record->index_record_scope >= index->scopes_count))
		return cursor;
	scope = &index->scopes[record->index_record_scope];
	if (!ctlv_message_input_scope_ok(index, scope))
		return cursor;
	if (!ctlv_message_input_slot(record, ref, &slot) ||
	    WARN_ON_ONCE(slot >= scope->slots_count))
		return cursor;

	indexed = &index->slots[scope->first_slot + slot];
	*cursor = (struct ctlv_cursor){
		.msg = record->msg,
		.parent = record->hdr,
		.hdr = indexed->count ?
			(const void *)record->msg->root + indexed->first_offset : NULL,
		.tree = record->tree,
		.node = record->tree->node_schemas[record->node].first_child + slot,
		.depth = record->depth + 1,
		.index_record_scope = CTLV_MESSAGE_INPUT_INDEX_NONE,
		.index_scope = record->index_record_scope,
	};

	if (indexed->count && ref.kind == CTLV_SCHEMA_RECORD) {
		if (WARN_ON_ONCE(indexed->first_record >= index->records_count))
			return cursor;
		cursor->index_record_scope = index->records[indexed->first_record].first_scope;
	}
	return cursor;
}
EXPORT_SYMBOL_NS_GPL(ctlv_input_field, "CTLV");

bool ctlv_input_cursor(const struct ctlv_cursor *cursor, struct ctlv_input_ref ref)
{
	const struct ctlv_node_schema *node;

	if (!ctlv_message_cursor_index(cursor))
		return false;
	node = &cursor->tree->node_schemas[cursor->node];
	return !WARN_ON_ONCE(node->type != ref.type || node->kind != ref.kind);
}
EXPORT_SYMBOL_NS_GPL(ctlv_input_cursor, "CTLV");

void ctlv_cursor_next(struct ctlv_cursor *cursor)
{
	const struct ctlv_message_input_slot *slot;
	const struct ctlv_message_input_scope *scope;
	const struct ctlv_node_schema *node;
	const struct ctlv_hdr *next;
	u32 next_record;
	u32 next_offset;

	slot = ctlv_message_cursor_index(cursor);
	if (!slot || WARN_ON_ONCE(!cursor->hdr))
		return;

	next = (const void *)cursor->hdr +
	       ALIGN((size_t)cursor->hdr->len, CTLV_ALIGNTO);
	next_offset = ctlv_message_offset(cursor->msg, next);
	if (next_offset >= slot->end_offset) {
		CTLV_ASSERT(next_offset == slot->end_offset);
		cursor->hdr = NULL;
		cursor->index_record_scope = CTLV_MESSAGE_INPUT_INDEX_NONE;
		return;
	}

	cursor->hdr = next;
	node = &cursor->tree->node_schemas[cursor->node];
	if (node->kind == CTLV_SCHEMA_RECORD) {
		if (WARN_ON_ONCE(cursor->index_record_scope >= cursor->msg->index->scopes_count))
			return;
		scope = &cursor->msg->index->scopes[cursor->index_record_scope];
		if (WARN_ON_ONCE(scope->record >= cursor->msg->index->records_count))
			return;
		next_record = cursor->msg->index->records[scope->record].next;
		if (WARN_ON_ONCE(next_record == CTLV_MESSAGE_INPUT_INDEX_NONE ||
				 next_record >= cursor->msg->index->records_count))
			return;
		cursor->index_record_scope = cursor->msg->index->records[next_record].first_scope;
	}
	CTLV_ASSERT(ctlv_message_cursor_index(cursor));
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_next, "CTLV");

bool ctlv_cursor_ok(const struct ctlv_cursor *cursor)
{
	return cursor && cursor->hdr && ctlv_message_cursor_index(cursor);
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_ok, "CTLV");

/* The payload, checked against the schema kinds a reader of this C type accepts. */
static const void *ctlv_message_cursor_payload(const struct ctlv_cursor *cursor, u32 kinds,
					       u32 *len)
{
	const struct ctlv_node_schema *node;

	if (!ctlv_message_cursor_index(cursor) || WARN_ON_ONCE(!cursor->hdr))
		return NULL;
	node = &cursor->tree->node_schemas[cursor->node];
	if (WARN_ON_ONCE(!(kinds & BIT(node->kind))))
		return NULL;
	*len = ctlv_message_hdr_payload_len(cursor->hdr);
	return ctlv_message_hdr_payload(cursor->hdr);
}

/* A length disagreeing with a validated schema kind is this core's mistake. */
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

u64 ctlv_cursor_u64(const struct ctlv_cursor *cursor)
{
	const u64 *value = ctlv_message_cursor_fixed(cursor,
						     BIT(CTLV_SCHEMA_UINT) | BIT(CTLV_SCHEMA_ENUM) |
							     BIT(CTLV_SCHEMA_FLAGS),
						     sizeof(u64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_u64, "CTLV");

s64 ctlv_cursor_s64(const struct ctlv_cursor *cursor)
{
	const s64 *value = ctlv_message_cursor_fixed(cursor, BIT(CTLV_SCHEMA_SINT), sizeof(s64));

	return value ? *value : 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_s64, "CTLV");

bool ctlv_cursor_bool(const struct ctlv_cursor *cursor)
{
	const u8 *value = ctlv_message_cursor_fixed(cursor, BIT(CTLV_SCHEMA_BOOL), sizeof(u8));

	return value ? *value : false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_bool, "CTLV");

const char *ctlv_cursor_string(const struct ctlv_cursor *cursor)
{
	const char *value;
	u32 len;

	value = ctlv_message_cursor_payload(cursor, BIT(CTLV_SCHEMA_STRING), &len);
	if (!value || WARN_ON_ONCE(!len || value[len - 1]))
		return NULL;
	return value;
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_string, "CTLV");

const void *ctlv_cursor_binary(const struct ctlv_cursor *cursor, u32 *len)
{
	*len = 0;
	return ctlv_message_cursor_payload(cursor, BIT(CTLV_SCHEMA_BINARY), len);
}
EXPORT_SYMBOL_NS_GPL(ctlv_cursor_binary, "CTLV");
