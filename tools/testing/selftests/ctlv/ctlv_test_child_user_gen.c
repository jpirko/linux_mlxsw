// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_test_child_user_gen.h"

/* Effective request and reply schemas. */

static const struct ctlv_node_schema noop_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, optional attrs */
static const struct ctlv_node_schema probe_get_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema probe_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = probe_get_request_optional_attr_node_schemas,
	.optional_attrs_count = 1,
};

/* probe.extension, optional attrs */
static const struct ctlv_node_schema probe_get_reply_extension_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_CHILD_ATTR_CHILD_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema probe_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_EXTENSION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = probe_get_reply_extension_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
};

static const struct ctlv_node_schema probe_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = probe_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

static const struct ctlv_node_schema dynamic_noop_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, optional attrs */
static const struct ctlv_node_schema dynamic_toggle_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema dynamic_toggle_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = dynamic_toggle_request_optional_attr_node_schemas,
	.optional_attrs_count = 1,
};

/* root, optional attrs */
static const struct ctlv_node_schema dynamic_toggle_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_TOKEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema dynamic_toggle_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = dynamic_toggle_reply_optional_attr_node_schemas,
	.optional_attrs_count = 1,
};

static const struct ctlv_node_schema pinged_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* item.leaf, attrs */
static const struct ctlv_node_schema echo_request_item_leaf_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_node_schema echo_request_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_node_schema echo_request_item_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LEAF,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_request_item_leaf_attr_node_schemas,
		.attrs_count = 1,
	},
};

/* probe.extension, optional attrs */
static const struct ctlv_node_schema echo_request_extension_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_CHILD_ATTR_CHILD_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, attrs */
static const struct ctlv_node_schema echo_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema echo_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_request_item_attr_node_schemas,
		.attrs_count = 1,
		.optional_attrs = echo_request_item_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_REGION,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_PAGE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_EXTENSION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = echo_request_extension_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
};

static const struct ctlv_node_schema echo_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = echo_request_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = echo_request_optional_attr_node_schemas,
	.optional_attrs_count = 11,
};

/* item.leaf, attrs */
static const struct ctlv_node_schema echo_reply_request_item_leaf_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_node_schema echo_reply_request_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_node_schema echo_reply_request_item_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LEAF,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_reply_request_item_leaf_attr_node_schemas,
		.attrs_count = 1,
	},
};

/* probe.extension, optional attrs */
static const struct ctlv_node_schema echo_reply_request_extension_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_CHILD_ATTR_CHILD_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, attrs */
static const struct ctlv_node_schema echo_reply_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema echo_reply_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_reply_request_item_attr_node_schemas,
		.attrs_count = 1,
		.optional_attrs = echo_reply_request_item_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_REGION,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_PAGE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_EXTENSION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = echo_reply_request_extension_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
};

static const struct ctlv_node_schema echo_reply_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = echo_reply_request_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = echo_reply_request_optional_attr_node_schemas,
	.optional_attrs_count = 11,
};

/* item.leaf, attrs */
static const struct ctlv_node_schema echo_reply_reply_item_leaf_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_node_schema echo_reply_reply_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_node_schema echo_reply_reply_item_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LABEL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ITEM_LEAF,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_reply_reply_item_leaf_attr_node_schemas,
		.attrs_count = 1,
	},
};

/* probe.extension, optional attrs */
static const struct ctlv_node_schema echo_reply_reply_extension_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_CHILD_ATTR_CHILD_EXTRA,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema echo_reply_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_S,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_FLAG,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_QUIRKS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_TEXT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_ECHO_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_reply_reply_item_attr_node_schemas,
		.attrs_count = 1,
		.optional_attrs = echo_reply_reply_item_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PROBE_EXTENSION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = echo_reply_reply_extension_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
};

static const struct ctlv_node_schema echo_reply_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = echo_reply_reply_optional_attr_node_schemas,
	.optional_attrs_count = 10,
};

/* root, optional attrs */
static const struct ctlv_node_schema series_dump_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_CHUNKS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_ITEMS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_VALUES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_TOTALS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

static const struct ctlv_node_schema series_dump_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = series_dump_request_optional_attr_node_schemas,
	.optional_attrs_count = 4,
};

/* series.item, attrs */
static const struct ctlv_node_schema series_dump_reply_chunk_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series.item, optional attrs */
static const struct ctlv_node_schema series_dump_reply_chunk_item_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_VALUE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* series.chunk, attrs */
static const struct ctlv_node_schema series_dump_reply_chunk_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series.chunk, optional attrs */
static const struct ctlv_node_schema series_dump_reply_chunk_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = series_dump_reply_chunk_item_attr_node_schemas,
		.attrs_count = 1,
		.optional_attrs = series_dump_reply_chunk_item_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema series_dump_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_CHUNK,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = series_dump_reply_chunk_attr_node_schemas,
		.attrs_count = 1,
		.optional_attrs = series_dump_reply_chunk_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_SERIES_TOTAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema series_dump_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = series_dump_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

/* root, optional attrs */
static const struct ctlv_node_schema ping_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PING_COUNT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 64,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PING_SERIAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PING_PAYLOAD,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 1024,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PING_ABORT,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_PING_EMPTY,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema ping_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = ping_request_optional_attr_node_schemas,
	.optional_attrs_count = 5,
};

/* root, attrs */
static const struct ctlv_node_schema beeped_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PING_SERIAL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema beeped_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_PING_FILLER,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 1024,
	},
};

static const struct ctlv_node_schema beeped_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = beeped_reply_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = beeped_reply_optional_attr_node_schemas,
	.optional_attrs_count = 1,
};

/* root, optional attrs */
static const struct ctlv_node_schema transfer_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_SOURCE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_TARGET,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_SCRATCH,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INOUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_PIECE,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 4,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x100000ULL,
		.blob_alignment = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_FILL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 255,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_SKIP,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_TWICE,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_HOLD,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema transfer_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = transfer_request_optional_attr_node_schemas,
	.optional_attrs_count = 8,
};

/* root, optional attrs */
static const struct ctlv_node_schema transfer_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_SUM,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_WRITTEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_TEST_ATTR_TRANSFER_DISTINCT,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema transfer_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = transfer_reply_optional_attr_node_schemas,
	.optional_attrs_count = 3,
};

static const struct ctlv_node_schema extra_noop_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
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
		.request = &dynamic_toggle_request_node_schema,
		.reply = &dynamic_toggle_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_PINGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 40,
		.reply = &pinged_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_ECHO,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 760,
		.request = &echo_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_ECHO_REPLY,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 760,
		.action_reply_max_len = 536,
		.request = &echo_reply_request_node_schema,
		.reply = &echo_reply_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_SERIES_DUMP,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 112,
		.query_reply_min_len = 72,
		.query_reply_suggested_len = 4096,
		.request = &series_dump_request_node_schema,
		.reply = &series_dump_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_PING,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 136,
		.request = &ping_request_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_BEEPED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 1104,
		.reply = &beeped_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_OP_TRANSFER,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 896,
		.action_reply_max_len = 88,
		.request = &transfer_request_node_schema,
		.reply = &transfer_reply_node_schema,
	},
	{
		.op_id = CTLV_TEST_CHILD_OP_EXTRA_NOOP,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
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
