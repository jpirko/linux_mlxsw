// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_test_gen.h"

static const struct ctlv_schema_node noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node probe_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node probe_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* probe.extension */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_ref probe_get_reply_stubs[] = {
	/* extension-point */
	{
		.family_id = 3,
		.set_id = 1,
		.node = 2,
	},
};

static const struct ctlv_schema_node dynamic_noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node dynamic_toggle_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node dynamic_toggle_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node pinged_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node echo_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 12,
		.required_children_count = 1,
	},
	/* echo.u */
	{
		.attr_id = 0x0003000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0003000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
	/* echo.flag */
	{
		.attr_id = 0x0003000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0003000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0003000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0003000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0003000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0003000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0003000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 13,
		.children_count = 3,
		.required_children_count = 1,
	},
	/* echo.region */
	{
		.attr_id = 0x000300020000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 4096,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
	/* echo.page */
	{
		.attr_id = 0x000300020000000bULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 65536,
			.alignment = 4096,
			.backings = BIT(CTLV_BLOB_TYPE_MEMFD),
			.direction = CTLV_BLOB_DIRECTION_OUTPUT,
		},
	},
	/* probe.extension */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0003000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 16,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_ref echo_request_stubs[] = {
	/* extension-point */
	{
		.family_id = 3,
		.set_id = 1,
		.node = 12,
	},
};

static const struct ctlv_schema_node echo_reply_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 12,
		.required_children_count = 1,
	},
	/* echo.u */
	{
		.attr_id = 0x0003000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0003000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
	/* echo.flag */
	{
		.attr_id = 0x0003000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0003000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0003000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0003000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0003000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0003000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0003000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 13,
		.children_count = 3,
		.required_children_count = 1,
	},
	/* echo.region */
	{
		.attr_id = 0x000300020000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 4096,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
	/* echo.page */
	{
		.attr_id = 0x000300020000000bULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 65536,
			.alignment = 4096,
			.backings = BIT(CTLV_BLOB_TYPE_MEMFD),
			.direction = CTLV_BLOB_DIRECTION_OUTPUT,
		},
	},
	/* probe.extension */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0003000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 16,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_ref echo_reply_request_stubs[] = {
	/* extension-point */
	{
		.family_id = 3,
		.set_id = 1,
		.node = 12,
	},
};

static const struct ctlv_schema_node echo_reply_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 10,
		.required_children_count = 0,
	},
	/* echo.u */
	{
		.attr_id = 0x0003000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0003000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
	/* echo.flag */
	{
		.attr_id = 0x0003000200000003ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* echo.mode */
	{
		.attr_id = 0x0003000200000004ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0003000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0003000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0003000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0003000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0003000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 11,
		.children_count = 3,
		.required_children_count = 1,
	},
	/* probe.extension */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0003000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 14,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_ref echo_reply_reply_stubs[] = {
	/* extension-point */
	{
		.family_id = 3,
		.set_id = 1,
		.node = 10,
	},
};

static const struct ctlv_schema_node series_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 4,
		.required_children_count = 0,
	},
	/* series.chunks */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.items */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.values */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.totals */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
};

static const struct ctlv_schema_node series_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 0,
	},
	/* series.chunk */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 3,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* series.total */
	{
		.attr_id = 0x0003000400000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.range = { .min = 0, .max = U64_MAX },
	},
	/* series.index */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.item */
	{
		.attr_id = 0x0003000400000006ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 5,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* series.index */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.value */
	{
		.attr_id = 0x0003000400000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node ping_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 5,
		.required_children_count = 0,
	},
	/* ping.count */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 64 },
	},
	/* ping.serial */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* ping.payload */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 1024 },
	},
	/* ping.abort */
	{
		.attr_id = 0x0003000500000004ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* ping.empty */
	{
		.attr_id = 0x0003000500000005ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node beeped_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* ping.serial */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* ping.filler */
	{
		.attr_id = 0x0003000500000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 1024 },
	},
};

static const struct ctlv_schema_node transfer_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 8,
		.required_children_count = 0,
	},
	/* transfer.source */
	{
		.attr_id = 0x0003000600000001ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 268435456,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
	/* transfer.target */
	{
		.attr_id = 0x0003000600000002ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 268435456,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_OUTPUT,
		},
	},
	/* transfer.scratch */
	{
		.attr_id = 0x0003000600000003ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 268435456,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INOUT,
		},
	},
	/* transfer.piece */
	{
		.attr_id = 0x0003000600000004ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 4,
		.blob = {
			.max_len = 1048576,
			.alignment = 1,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
	/* transfer.fill */
	{
		.attr_id = 0x0003000600000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 255 },
	},
	/* transfer.skip */
	{
		.attr_id = 0x0003000600000006ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* transfer.twice */
	{
		.attr_id = 0x0003000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* transfer.hold */
	{
		.attr_id = 0x0003000600000008ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node transfer_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 3,
		.required_children_count = 0,
	},
	/* transfer.sum */
	{
		.attr_id = 0x0003000600000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* transfer.written */
	{
		.attr_id = 0x000300060000000aULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* transfer.distinct */
	{
		.attr_id = 0x000300060000000bULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_test_ctlv_op_schemas[] = {
	{
		.op_id = 0x0003000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = noop_request_nodes,
			.nodes_count =
				ARRAY_SIZE(noop_request_nodes),
		},
		.sizes = {
			.request_max_len = 16,
		},
	},
	{
		.op_id = 0x0003000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.pre_post = true,
		.request = {
			.nodes = probe_get_request_nodes,
			.nodes_count =
				ARRAY_SIZE(probe_get_request_nodes),
		},
		.reply = {
			.nodes = probe_get_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(probe_get_reply_nodes),
			.stubs = probe_get_reply_stubs,
			.stubs_count =
				ARRAY_SIZE(probe_get_reply_stubs),
		},
		.sizes = {
			.request_max_len = 40,
			.query_reply_min_len = 40,
			.query_reply_suggested_len = 65536,
		},
	},
	{
		.op_id = 0x0003000000000003ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.pre_post = true,
		.request = {
			.nodes = dynamic_noop_request_nodes,
			.nodes_count =
				ARRAY_SIZE(dynamic_noop_request_nodes),
		},
		.sizes = {
			.request_max_len = 16,
		},
	},
	{
		.op_id = 0x0003000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = {
			.nodes = dynamic_toggle_request_nodes,
			.nodes_count =
				ARRAY_SIZE(dynamic_toggle_request_nodes),
		},
		.reply = {
			.nodes = dynamic_toggle_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(dynamic_toggle_reply_nodes),
		},
		.sizes = {
			.request_max_len = 40,
			.action_reply_max_len = 40,
		},
	},
	{
		.op_id = 0x0003000000000005ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = {
			.nodes = pinged_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(pinged_reply_nodes),
		},
		.sizes = {
			.event_record_max_len = 40,
		},
	},
	{
		.op_id = 0x0003000000000006ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = echo_request_nodes,
			.nodes_count =
				ARRAY_SIZE(echo_request_nodes),
			.stubs = echo_request_stubs,
			.stubs_count =
				ARRAY_SIZE(echo_request_stubs),
		},
		.sizes = {
			.request_max_len = 736,
		},
	},
	{
		.op_id = 0x0003000000000007ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = echo_reply_request_nodes,
			.nodes_count =
				ARRAY_SIZE(echo_reply_request_nodes),
			.stubs = echo_reply_request_stubs,
			.stubs_count =
				ARRAY_SIZE(echo_reply_request_stubs),
		},
		.reply = {
			.nodes = echo_reply_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(echo_reply_reply_nodes),
			.stubs = echo_reply_reply_stubs,
			.stubs_count =
				ARRAY_SIZE(echo_reply_reply_stubs),
		},
		.sizes = {
			.request_max_len = 736,
			.action_reply_max_len = 512,
		},
	},
	{
		.op_id = 0x0003000000000008ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.pre_post = true,
		.request = {
			.nodes = series_dump_request_nodes,
			.nodes_count =
				ARRAY_SIZE(series_dump_request_nodes),
		},
		.reply = {
			.nodes = series_dump_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(series_dump_reply_nodes),
		},
		.sizes = {
			.request_max_len = 112,
			.query_reply_min_len = 72,
			.query_reply_suggested_len = 4096,
		},
	},
	{
		.op_id = 0x0003000000000009ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = ping_request_nodes,
			.nodes_count =
				ARRAY_SIZE(ping_request_nodes),
		},
		.sizes = {
			.request_max_len = 136,
		},
	},
	{
		.op_id = 0x000300000000000aULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = {
			.nodes = beeped_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(beeped_reply_nodes),
		},
		.sizes = {
			.event_record_max_len = 1104,
		},
	},
	{
		.op_id = 0x000300000000000bULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = transfer_request_nodes,
			.nodes_count =
				ARRAY_SIZE(transfer_request_nodes),
		},
		.reply = {
			.nodes = transfer_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(transfer_reply_nodes),
		},
		.sizes = {
			.request_max_len = 896,
			.action_reply_max_len = 88,
		},
	},
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_ctlv_schema = {
	.name = "ctlv-test",
	.family_id = 3,
	.ops = ctlv_test_ctlv_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_test_ctlv_op_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(93 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(1104 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(2 <= CTLV_MAX_DECLARED_NEST_DEPTH);

/* Local action and query bindings in declaration order. */
static const struct ctlv_op ctlv_test_ctlv_ops[] = {
	{
		.action = ctlv_test_noop_action,
	},
	{
		.query = ctlv_test_probe_get_query,
		.pre = ctlv_test_probe_get_pre,
		.post = ctlv_test_probe_get_post,
	},
	{
		.action = ctlv_test_dynamic_noop_action,
		.pre = ctlv_test_dynamic_noop_pre,
		.post = ctlv_test_dynamic_noop_post,
	},
	{
		.action = ctlv_test_dynamic_toggle_action,
	},
	{
		.action = ctlv_test_echo_action,
	},
	{
		.action = ctlv_test_echo_reply_action,
	},
	{
		.query = ctlv_test_series_dump_query,
		.pre = ctlv_test_series_dump_pre,
		.post = ctlv_test_series_dump_post,
	},
	{
		.action = ctlv_test_ping_action,
	},
	{
		.action = ctlv_test_transfer_action,
	},
};

/* Family descriptor used by ctlv_create(). */
const struct ctlv_family ctlv_test_ctlv_family = {
	.schema = &ctlv_test_ctlv_schema,
	.parent = &ctlv_ctlv_family,
	.ops = ctlv_test_ctlv_ops,
	.ops_count = ARRAY_SIZE(ctlv_test_ctlv_ops),
	.pre = ctlv_test_op_pre,
	.post = ctlv_test_op_post,
	.priv_size = ctlv_test_priv_size,
	.init = ctlv_test_init,
	.post_init = ctlv_test_post_init,
	.pre_fini = ctlv_test_pre_fini,
	.fini = ctlv_test_fini,
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_ctlv_family, "CTLV");
