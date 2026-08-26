// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_test_user_gen.h"

/* What each op's messages may hold, composed as the core composes them. */

/* noop request */
static const struct ctlv_schema_node noop_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, optional attrs */
static const struct ctlv_schema_node probe_get_request_optional_attrs[] = {
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* probe-get request */
static const struct ctlv_schema_node probe_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = probe_get_request_optional_attrs,
	.n_optional_attrs = 1,
};

/* root, optional attrs */
static const struct ctlv_schema_node probe_get_reply_optional_attrs[] = {
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* probe-get reply */
static const struct ctlv_schema_node probe_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = probe_get_reply_optional_attrs,
	.n_optional_attrs = 2,
};

/* dynamic-noop request */
static const struct ctlv_schema_node dynamic_noop_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, optional attrs */
static const struct ctlv_schema_node dynamic_toggle_request_optional_attrs[] = {
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* dynamic-toggle request */
static const struct ctlv_schema_node dynamic_toggle_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = dynamic_toggle_request_optional_attrs,
	.n_optional_attrs = 1,
};

/* root, optional attrs */
static const struct ctlv_schema_node dynamic_toggle_reply_optional_attrs[] = {
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* dynamic-toggle reply */
static const struct ctlv_schema_node dynamic_toggle_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = dynamic_toggle_reply_optional_attrs,
	.n_optional_attrs = 1,
};

/* pinged reply */
static const struct ctlv_schema_node pinged_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* item.leaf, attrs */
static const struct ctlv_schema_node echo_request_item_leaf_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_schema_node echo_request_item_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_schema_node echo_request_item_optional_attrs[] = {
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_request_item_leaf_attrs,
		.n_attrs = 1,
	},
};

/* root, attrs */
static const struct ctlv_schema_node echo_request_attrs[] = {
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node echo_request_optional_attrs[] = {
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	/* echo.flag */
	{
		.attr_id = 0x0001000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0001000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_request_item_attrs,
		.n_attrs = 1,
		.optional_attrs = echo_request_item_optional_attrs,
		.n_optional_attrs = 2,
	},
	/* echo.region */
	{
		.attr_id = 0x000100020000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	/* echo.page */
	{
		.attr_id = 0x000100020000000bULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* echo request */
static const struct ctlv_schema_node echo_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = echo_request_attrs,
	.n_attrs = 1,
	.optional_attrs = echo_request_optional_attrs,
	.n_optional_attrs = 11,
};

/* item.leaf, attrs */
static const struct ctlv_schema_node echo_reply_request_item_leaf_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_schema_node echo_reply_request_item_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_schema_node
	echo_reply_request_item_optional_attrs[] = {
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_reply_request_item_leaf_attrs,
		.n_attrs = 1,
	},
};

/* root, attrs */
static const struct ctlv_schema_node echo_reply_request_attrs[] = {
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node echo_reply_request_optional_attrs[] = {
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	/* echo.flag */
	{
		.attr_id = 0x0001000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0001000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_reply_request_item_attrs,
		.n_attrs = 1,
		.optional_attrs = echo_reply_request_item_optional_attrs,
		.n_optional_attrs = 2,
	},
	/* echo.region */
	{
		.attr_id = 0x000100020000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 4096,
		.blob_alignment = 8,
	},
	/* echo.page */
	{
		.attr_id = 0x000100020000000bULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 4,
		.blob_max_length = 0x10000ULL,
		.blob_alignment = 4096,
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* echo-reply request */
static const struct ctlv_schema_node echo_reply_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = echo_reply_request_attrs,
	.n_attrs = 1,
	.optional_attrs = echo_reply_request_optional_attrs,
	.n_optional_attrs = 11,
};

/* item.leaf, attrs */
static const struct ctlv_schema_node echo_reply_reply_item_leaf_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, attrs */
static const struct ctlv_schema_node echo_reply_reply_item_attrs[] = {
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 7,
	},
};

/* echo.item, optional attrs */
static const struct ctlv_schema_node echo_reply_reply_item_optional_attrs[] = {
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 7,
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = echo_reply_reply_item_leaf_attrs,
		.n_attrs = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node echo_reply_reply_optional_attrs[] = {
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4096,
	},
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = (__u64)(__s64)-100,
		.max = 100,
	},
	/* echo.flag */
	{
		.attr_id = 0x0001000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0001000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 15,
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 4,
		.max_length = 32,
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.attrs = echo_reply_reply_item_attrs,
		.n_attrs = 1,
		.optional_attrs = echo_reply_reply_item_optional_attrs,
		.n_optional_attrs = 2,
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* echo-reply reply */
static const struct ctlv_schema_node echo_reply_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = echo_reply_reply_optional_attrs,
	.n_optional_attrs = 10,
};

/* root, optional attrs */
static const struct ctlv_schema_node series_dump_request_optional_attrs[] = {
	/* series.chunks */
	{
		.attr_id = 0x0001000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	/* series.items */
	{
		.attr_id = 0x0001000400000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	/* series.values */
	{
		.attr_id = 0x0001000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
	/* series.totals */
	{
		.attr_id = 0x0001000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series-dump request */
static const struct ctlv_schema_node series_dump_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = series_dump_request_optional_attrs,
	.n_optional_attrs = 4,
};

/* series.item, attrs */
static const struct ctlv_schema_node series_dump_reply_chunk_item_attrs[] = {
	/* series.index */
	{
		.attr_id = 0x0001000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series.item, optional attrs */
static const struct ctlv_schema_node
	series_dump_reply_chunk_item_optional_attrs[] = {
	/* series.value */
	{
		.attr_id = 0x0001000400000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* series.chunk, attrs */
static const struct ctlv_schema_node series_dump_reply_chunk_attrs[] = {
	/* series.index */
	{
		.attr_id = 0x0001000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 8,
	},
};

/* series.chunk, optional attrs */
static const struct ctlv_schema_node
	series_dump_reply_chunk_optional_attrs[] = {
	/* series.item */
	{
		.attr_id = 0x0001000400000006ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = series_dump_reply_chunk_item_attrs,
		.n_attrs = 1,
		.optional_attrs = series_dump_reply_chunk_item_optional_attrs,
		.n_optional_attrs = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node series_dump_reply_optional_attrs[] = {
	/* series.chunk */
	{
		.attr_id = 0x0001000400000005ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = series_dump_reply_chunk_attrs,
		.n_attrs = 1,
		.optional_attrs = series_dump_reply_chunk_optional_attrs,
		.n_optional_attrs = 1,
	},
	/* series.total */
	{
		.attr_id = 0x0001000400000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* series-dump reply */
static const struct ctlv_schema_node series_dump_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = series_dump_reply_optional_attrs,
	.n_optional_attrs = 2,
};

/* root, optional attrs */
static const struct ctlv_schema_node ping_request_optional_attrs[] = {
	/* ping.count */
	{
		.attr_id = 0x0001000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 64,
	},
	/* ping.serial */
	{
		.attr_id = 0x0001000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* ping.payload */
	{
		.attr_id = 0x0001000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 1024,
	},
	/* ping.abort */
	{
		.attr_id = 0x0001000500000004ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* ping.empty */
	{
		.attr_id = 0x0001000500000005ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* ping request */
static const struct ctlv_schema_node ping_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = ping_request_optional_attrs,
	.n_optional_attrs = 5,
};

/* root, attrs */
static const struct ctlv_schema_node beeped_reply_attrs[] = {
	/* ping.serial */
	{
		.attr_id = 0x0001000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node beeped_reply_optional_attrs[] = {
	/* ping.filler */
	{
		.attr_id = 0x0001000500000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 1024,
	},
};

/* beeped reply */
static const struct ctlv_schema_node beeped_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = beeped_reply_attrs,
	.n_attrs = 1,
	.optional_attrs = beeped_reply_optional_attrs,
	.n_optional_attrs = 1,
};

/* root, optional attrs */
static const struct ctlv_schema_node transfer_request_optional_attrs[] = {
	/* transfer.source */
	{
		.attr_id = 0x0001000600000001ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	/* transfer.target */
	{
		.attr_id = 0x0001000600000002ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	/* transfer.scratch */
	{
		.attr_id = 0x0001000600000003ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INOUT,
		.blob_backings = 14,
		.blob_max_length = 0x10000000ULL,
		.blob_alignment = 8,
	},
	/* transfer.piece */
	{
		.attr_id = 0x0001000600000004ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 4,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0x100000ULL,
		.blob_alignment = 1,
	},
	/* transfer.fill */
	{
		.attr_id = 0x0001000600000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 255,
	},
	/* transfer.skip */
	{
		.attr_id = 0x0001000600000006ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* transfer.twice */
	{
		.attr_id = 0x0001000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* transfer.hold */
	{
		.attr_id = 0x0001000600000008ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* transfer request */
static const struct ctlv_schema_node transfer_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = transfer_request_optional_attrs,
	.n_optional_attrs = 8,
};

/* root, optional attrs */
static const struct ctlv_schema_node transfer_reply_optional_attrs[] = {
	/* transfer.sum */
	{
		.attr_id = 0x0001000600000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* transfer.written */
	{
		.attr_id = 0x000100060000000aULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* transfer.distinct */
	{
		.attr_id = 0x000100060000000bULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* transfer reply */
static const struct ctlv_schema_node transfer_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = transfer_reply_optional_attrs,
	.n_optional_attrs = 3,
};

/* Every op a device of this family answers, in chain declaration order. */
static const struct ctlv_op_spec ctlv_test_ctlv_op_specs[] = {
	/* noop */
	{
		.op_id = 0x0001000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.request = &noop_request,
	},
	/* probe-get */
	{
		.op_id = 0x0001000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.query_reply_min_len = 40,
		.query_reply_suggested_len = 65536,
		.request = &probe_get_request,
		.reply = &probe_get_reply,
	},
	/* dynamic-noop */
	{
		.op_id = 0x0001000000000003ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 16,
		.request = &dynamic_noop_request,
	},
	/* dynamic-toggle */
	{
		.op_id = 0x0001000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 40,
		.action_reply_max_len = 40,
		.request = &dynamic_toggle_request,
		.reply = &dynamic_toggle_reply,
	},
	/* pinged */
	{
		.op_id = 0x0001000000000005ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 40,
		.reply = &pinged_reply,
	},
	/* echo */
	{
		.op_id = 0x0001000000000006ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 736,
		.request = &echo_request,
	},
	/* echo-reply */
	{
		.op_id = 0x0001000000000007ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 736,
		.action_reply_max_len = 512,
		.request = &echo_reply_request,
		.reply = &echo_reply_reply,
	},
	/* series-dump */
	{
		.op_id = 0x0001000000000008ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 112,
		.query_reply_min_len = 72,
		.query_reply_suggested_len = 4096,
		.request = &series_dump_request,
		.reply = &series_dump_reply,
	},
	/* ping */
	{
		.op_id = 0x0001000000000009ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 136,
		.request = &ping_request,
	},
	/* beeped */
	{
		.op_id = 0x000100000000000aULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 1104,
		.reply = &beeped_reply,
	},
	/* transfer */
	{
		.op_id = 0x000100000000000bULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 896,
		.action_reply_max_len = 88,
		.request = &transfer_request,
		.reply = &transfer_reply,
	},
};

/* What a device of this family is, for a caller holding one against it. */
const struct ctlv_family_spec ctlv_test_ctlv_spec = {
	.name = "ctlv-test",
	.family_id = 1,
	.parent = &ctlv_ctlv_spec,
	.op = ctlv_test_ctlv_op_specs,
	.n_op = 11,
};
