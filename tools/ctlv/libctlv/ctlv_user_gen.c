// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

/* Effective request and reply schemas. */

static const struct ctlv_node_schema families_dump_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* family.entry, attrs */
static const struct ctlv_node_schema families_dump_reply_entry_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_FAMILY_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_FAMILY_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 63,
	},
};

/* root, attrs */
static const struct ctlv_node_schema families_dump_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_FAMILY_ENTRY,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = families_dump_reply_entry_attr_node_schemas,
		.attrs_count = 2,
	},
};

static const struct ctlv_node_schema families_dump_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = families_dump_reply_attr_node_schemas,
	.attrs_count = 1,
};

static const struct ctlv_node_schema ops_dump_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* op.entry, attrs */
static const struct ctlv_node_schema ops_dump_reply_entry_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_OP_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_TYPE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
};

/* op.entry, optional attrs */
static const struct ctlv_node_schema ops_dump_reply_entry_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_OP_AVAILABILITY,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.attr_id = CTLV_ATTR_OP_DISABLED,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_ATTR_OP_REQUEST_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_FD_ACCESS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* root, attrs */
static const struct ctlv_node_schema ops_dump_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_OP_ENTRY,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = ops_dump_reply_entry_attr_node_schemas,
		.attrs_count = 2,
		.optional_attrs = ops_dump_reply_entry_optional_attr_node_schemas,
		.optional_attrs_count = 8,
	},
};

static const struct ctlv_node_schema ops_dump_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = ops_dump_reply_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema op_schema_get_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_OP_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema op_schema_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = op_schema_get_request_attr_node_schemas,
	.attrs_count = 1,
};

/* schema.request, attrs */
static const struct ctlv_node_schema op_schema_get_reply_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_SCHEMA_ATTR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_KIND,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* schema.request, optional attrs */
static const struct ctlv_node_schema op_schema_get_reply_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_SCHEMA_MIN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MIN_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_FLAGS_MASK,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_UNIT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_SCALE,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_REENTERS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_ATTRS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* schema.reply, attrs */
static const struct ctlv_node_schema op_schema_get_reply_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_SCHEMA_ATTR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_KIND,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* schema.reply, optional attrs */
static const struct ctlv_node_schema op_schema_get_reply_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_SCHEMA_MIN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MIN_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_FLAGS_MASK,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_UNIT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_SCALE,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_REENTERS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_ATTRS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema op_schema_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_SCHEMA_REQUEST,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = op_schema_get_reply_request_attr_node_schemas,
		.attrs_count = 3,
		.optional_attrs = op_schema_get_reply_request_optional_attr_node_schemas,
		.optional_attrs_count = 14,
	},
	{
		.attr_id = CTLV_ATTR_SCHEMA_REPLY,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = op_schema_get_reply_reply_attr_node_schemas,
		.attrs_count = 3,
		.optional_attrs = op_schema_get_reply_reply_optional_attr_node_schemas,
		.optional_attrs_count = 14,
	},
};

static const struct ctlv_node_schema op_schema_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = op_schema_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

/* root, attrs */
static const struct ctlv_node_schema subscriptions_add_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_EVENT_OP_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema subscriptions_add_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = subscriptions_add_request_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema subscriptions_remove_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_EVENT_OP_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema subscriptions_remove_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = subscriptions_remove_request_attr_node_schemas,
	.attrs_count = 1,
};

static const struct ctlv_node_schema event_queue_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, attrs */
static const struct ctlv_node_schema event_queue_get_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_EVENT_QUEUE_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 0x1000000ULL,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	{
		.attr_id = CTLV_ATTR_EVENT_QUEUE_USED,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 0,
		.max = UINT64_MAX,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

static const struct ctlv_node_schema event_queue_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_queue_get_reply_attr_node_schemas,
	.attrs_count = 2,
};

/* root, attrs */
static const struct ctlv_node_schema event_queue_set_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_EVENT_QUEUE_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 0x1000000ULL,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

static const struct ctlv_node_schema event_queue_set_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_queue_set_request_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema op_changed_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_OP_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_OP_DISABLED,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_ATTR_OP_STATE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema op_changed_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = op_changed_reply_attr_node_schemas,
	.attrs_count = 3,
};

/* root, attrs */
static const struct ctlv_node_schema event_overflow_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTR_EVENT_FIRST_SEQUENCE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_EVENT_LAST_SEQUENCE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTR_EVENT_DROPPED_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema event_overflow_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_overflow_reply_attr_node_schemas,
	.attrs_count = 3,
};

static const struct ctlv_node_schema device_removed_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* Effective ops in declaration order. */
static const struct ctlv_op_spec ctlv_op_specs[] = {
	{
		.op_id = CTLV_OP_FAMILIES_DUMP,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 112,
		.query_reply_suggested_len = 65536,
		.request = &families_dump_request_node_schema,
		.reply = &families_dump_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_OPS_DUMP,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 56,
		.query_reply_suggested_len = 65536,
		.request = &ops_dump_request_node_schema,
		.reply = &ops_dump_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_OP_SCHEMA_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.query_reply_min_len = 552,
		.query_reply_suggested_len = 65536,
		.request = &op_schema_get_request_node_schema,
		.reply = &op_schema_get_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_SUBSCRIPTIONS_ADD,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 6160,
		.request = &subscriptions_add_request_node_schema,
	},
	{
		.op_id = CTLV_OP_SUBSCRIPTIONS_REMOVE,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 6160,
		.request = &subscriptions_remove_request_node_schema,
	},
	{
		.op_id = CTLV_OP_EVENT_QUEUE_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 40,
		.query_reply_suggested_len = 65536,
		.request = &event_queue_get_request_node_schema,
		.reply = &event_queue_get_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_EVENT_QUEUE_SET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.request = &event_queue_set_request_node_schema,
	},
	{
		.op_id = CTLV_OP_OP_CHANGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
		.reply = &op_changed_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_EVENT_OVERFLOW,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
		.reply = &event_overflow_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_DEVICE_REMOVED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 40,
		.reply = &device_removed_reply_node_schema,
	},
};

/* Family specification descriptor. */
const struct ctlv_family_spec ctlv_spec = {
	.name = "ctlv",
	.family_id = 0,
	.ops = ctlv_op_specs,
	.ops_count = 10,
};
