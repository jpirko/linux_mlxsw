// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/*
 * Generated from Documentation/ctlv/specs/ctlv-test-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_test_child_user_gen.h"

/* Effective request and reply schemas. */

static const struct ctlv_node_schema noop_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* root, optional fields */
static const struct ctlv_node_schema probe_get_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TOKEN_INFO_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema probe_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = probe_get_request_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* probe-get extension, optional fields */
static const struct ctlv_node_schema probe_get_reply_extension_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema probe_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TOKEN_INFO_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_PROBE_GET_EXTENSION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.optional_fields = probe_get_reply_extension_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
};

static const struct ctlv_node_schema probe_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = probe_get_reply_optional_field_node_schemas,
	.optional_fields_count = 2,
};

static const struct ctlv_node_schema dynamic_noop_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* root, optional fields */
static const struct ctlv_node_schema dynamic_toggle_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TOKEN_INFO_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema dynamic_toggle_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = dynamic_toggle_request_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* root, optional fields */
static const struct ctlv_node_schema dynamic_toggle_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TOKEN_INFO_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema dynamic_toggle_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = dynamic_toggle_reply_optional_field_node_schemas,
	.optional_fields_count = 1,
};

static const struct ctlv_node_schema pinged_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* echo-values.item.leaf, fields */
static const struct ctlv_node_schema echo_request_item_leaf_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, fields */
static const struct ctlv_node_schema echo_request_item_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, optional fields */
static const struct ctlv_node_schema echo_request_item_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = echo_request_item_leaf_field_node_schemas,
		.fields_count = 1,
	},
};

/* echo-values.extension, optional fields */
static const struct ctlv_node_schema echo_request_extension_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, fields */
static const struct ctlv_node_schema echo_request_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema echo_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 2,
		.fields = echo_request_item_field_node_schemas,
		.fields_count = 1,
		.optional_fields = echo_request_item_optional_field_node_schemas,
		.optional_fields_count = 2,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_EXTENSION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.optional_fields = echo_request_extension_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_VARIANT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_TEST_T_ECHO_MESSAGE_REGION,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	{
		.type = CTLV_TEST_T_ECHO_MESSAGE_PAGE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
};

static const struct ctlv_node_schema echo_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = echo_request_field_node_schemas,
	.fields_count = 1,
	.optional_fields = echo_request_optional_field_node_schemas,
	.optional_fields_count = 12,
};

/* echo-values.item.leaf, fields */
static const struct ctlv_node_schema echo_reply_request_item_leaf_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, fields */
static const struct ctlv_node_schema echo_reply_request_item_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, optional fields */
static const struct ctlv_node_schema echo_reply_request_item_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = echo_reply_request_item_leaf_field_node_schemas,
		.fields_count = 1,
	},
};

/* echo-values.extension, optional fields */
static const struct ctlv_node_schema echo_reply_request_extension_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, fields */
static const struct ctlv_node_schema echo_reply_request_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema echo_reply_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 2,
		.fields = echo_reply_request_item_field_node_schemas,
		.fields_count = 1,
		.optional_fields = echo_reply_request_item_optional_field_node_schemas,
		.optional_fields_count = 2,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_EXTENSION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.optional_fields = echo_reply_request_extension_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_VARIANT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_TEST_T_ECHO_MESSAGE_REGION,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	{
		.type = CTLV_TEST_T_ECHO_MESSAGE_PAGE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
};

static const struct ctlv_node_schema echo_reply_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = echo_reply_request_field_node_schemas,
	.fields_count = 1,
	.optional_fields = echo_reply_request_optional_field_node_schemas,
	.optional_fields_count = 12,
};

/* echo-values.item.leaf, fields */
static const struct ctlv_node_schema echo_reply_reply_item_leaf_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, fields */
static const struct ctlv_node_schema echo_reply_reply_item_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo-values.item, optional fields */
static const struct ctlv_node_schema echo_reply_reply_item_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = echo_reply_reply_item_leaf_field_node_schemas,
		.fields_count = 1,
	},
};

/* echo-values.extension, optional fields */
static const struct ctlv_node_schema echo_reply_reply_extension_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema echo_reply_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_ECHO_VALUES_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_ITEM,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 2,
		.fields = echo_reply_reply_item_field_node_schemas,
		.fields_count = 1,
		.optional_fields = echo_reply_reply_item_optional_field_node_schemas,
		.optional_fields_count = 2,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_EXTENSION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.optional_fields = echo_reply_reply_extension_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_TEST_T_ECHO_VALUES_VARIANT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

static const struct ctlv_node_schema echo_reply_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = echo_reply_reply_optional_field_node_schemas,
	.optional_fields_count = 11,
};

/* root, optional fields */
static const struct ctlv_node_schema series_get_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_CHUNKS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.type = CTLV_TEST_T_SERIES_GET_ITEMS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.type = CTLV_TEST_T_SERIES_GET_VALUES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.type = CTLV_TEST_T_SERIES_GET_TOTALS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

static const struct ctlv_node_schema series_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = series_get_request_optional_field_node_schemas,
	.optional_fields_count = 4,
};

/* series-get chunk.item, fields */
static const struct ctlv_node_schema series_get_reply_chunk_item_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series-get chunk.item, optional fields */
static const struct ctlv_node_schema series_get_reply_chunk_item_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_ITEM_VALUE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* series-get chunk, fields */
static const struct ctlv_node_schema series_get_reply_chunk_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_CHUNK_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series-get chunk, optional fields */
static const struct ctlv_node_schema series_get_reply_chunk_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_CHUNK_ITEM,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = series_get_reply_chunk_item_field_node_schemas,
		.fields_count = 1,
		.optional_fields = series_get_reply_chunk_item_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema series_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_SERIES_GET_CHUNK,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = series_get_reply_chunk_field_node_schemas,
		.fields_count = 1,
		.optional_fields = series_get_reply_chunk_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_TEST_T_SERIES_GET_TOTAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema series_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = series_get_reply_optional_field_node_schemas,
	.optional_fields_count = 2,
};

/* root, optional fields */
static const struct ctlv_node_schema ping_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_PING_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 64,
	},
	{
		.type = CTLV_TEST_T_PING_SERIAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_PING_PAYLOAD,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 1024,
	},
	{
		.type = CTLV_TEST_T_PING_ABORT,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_PING_EMPTY,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema ping_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = ping_request_optional_field_node_schemas,
	.optional_fields_count = 5,
};

/* root, fields */
static const struct ctlv_node_schema beeped_reply_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_BEEPED_SERIAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema beeped_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_BEEPED_FILLER,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 1024,
	},
};

static const struct ctlv_node_schema beeped_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = beeped_reply_field_node_schemas,
	.fields_count = 1,
	.optional_fields = beeped_reply_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* root, optional fields */
static const struct ctlv_node_schema transfer_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TRANSFER_SOURCE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_TARGET,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_SCRATCH,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INOUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_PIECE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 4,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x100000ULL,
		.blob_alignment = 1,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_FILL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 255,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_SKIP,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_TWICE,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_HOLD,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema transfer_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = transfer_request_optional_field_node_schemas,
	.optional_fields_count = 8,
};

/* root, optional fields */
static const struct ctlv_node_schema transfer_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_TEST_T_TRANSFER_SUM,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_WRITTEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_TEST_T_TRANSFER_DISTINCT,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema transfer_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = transfer_reply_optional_field_node_schemas,
	.optional_fields_count = 3,
};

static const struct ctlv_node_schema extra_noop_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* Effective ops in declaration order. */
static const struct ctlv_op_spec ctlv_test_child_op_specs[] = {
	{
		.op_id = CTLV_TEST_OP_NOOP,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.error_info_max_len = 456,
		.request = &noop_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.query_reply_min_len = 56,
		.query_reply_suggested_len = 65536,
		.error_info_max_len = 456,
		.request = &probe_get_request_node_schema,
		.reply = &probe_get_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_DYNAMIC_NOOP,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 16,
		.error_info_max_len = 456,
		.request = &dynamic_noop_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_DYNAMIC_TOGGLE,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 40,
		.action_reply_max_len = 40,
		.error_info_max_len = 456,
		.request = &dynamic_toggle_request_node_schema,
		.reply = &dynamic_toggle_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_PINGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_max_len = 40,
		.reply = &pinged_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_ECHO,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 784,
		.error_info_max_len = 456,
		.request = &echo_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_ECHO_REPLY,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 784,
		.action_reply_max_len = 560,
		.error_info_max_len = 456,
		.request = &echo_reply_request_node_schema,
		.reply = &echo_reply_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_SERIES_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 112,
		.query_reply_min_len = 72,
		.query_reply_suggested_len = 4096,
		.error_info_max_len = 456,
		.request = &series_get_request_node_schema,
		.reply = &series_get_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_PING,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 136,
		.error_info_max_len = 456,
		.request = &ping_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_BEEPED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_max_len = 1104,
		.reply = &beeped_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_TRANSFER,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 896,
		.action_reply_max_len = 88,
		.error_info_max_len = 456,
		.request = &transfer_request_node_schema,
		.reply = &transfer_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_CHILD_OP_EXTRA_NOOP,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.error_info_max_len = 456,
		.request = &extra_noop_request_node_schema,
	},
};

/* Family specification descriptor. */
const struct ctlv_family_spec ctlv_test_child_spec = {
	.name = "ctlv-test-child",
	.family_id = 4,
	.parent = &ctlv_test_spec,
	.ops = ctlv_test_child_op_specs,
	.ops_count = 12,
};
