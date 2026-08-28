// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_test_gen.h"

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);
static_assert(sizeof(struct ctlv_event_hdr) == 24);

/* noop request */
static const struct ctlv_schema_node noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* probe-get request */
static const struct ctlv_schema_node probe_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* probe-get reply */
static const struct ctlv_schema_node probe_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* probe-get reply extension points */
static const struct ctlv_stub_ref probe_get_reply_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.node = 2,
	},
};

/* dynamic-noop request */
static const struct ctlv_schema_node dynamic_noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* dynamic-toggle request */
static const struct ctlv_schema_node dynamic_toggle_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* dynamic-toggle reply */
static const struct ctlv_schema_node dynamic_toggle_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 0,
	},
	/* probe.token */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* pinged reply */
static const struct ctlv_schema_node pinged_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* echo request */
static const struct ctlv_schema_node echo_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 12,
		.n_required_children = 1,
	},
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
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
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 13,
		.n_children = 3,
		.n_required_children = 1,
	},
	/* echo.region */
	{
		.attr_id = 0x000100020000000aULL,
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
		.attr_id = 0x000100020000000bULL,
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
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 16,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* echo request extension points */
static const struct ctlv_stub_ref echo_request_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.node = 12,
	},
};

/* echo-reply request */
static const struct ctlv_schema_node echo_reply_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 12,
		.n_required_children = 1,
	},
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
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
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 13,
		.n_children = 3,
		.n_required_children = 1,
	},
	/* echo.region */
	{
		.attr_id = 0x000100020000000aULL,
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
		.attr_id = 0x000100020000000bULL,
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
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 16,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* echo-reply request extension points */
static const struct ctlv_stub_ref echo_reply_request_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.node = 12,
	},
};

/* echo-reply reply */
static const struct ctlv_schema_node echo_reply_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 10,
		.n_required_children = 0,
	},
	/* echo.u */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* echo.s */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
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
		.range = { .min = 1, .max = 3 },
	},
	/* echo.quirks */
	{
		.attr_id = 0x0001000200000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x3ULL,
	},
	/* echo.text */
	{
		.attr_id = 0x0001000200000006ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* echo.data */
	{
		.attr_id = 0x0001000200000007ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 32 },
	},
	/* echo.count */
	{
		.attr_id = 0x0001000200000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* echo.item */
	{
		.attr_id = 0x0001000200000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 11,
		.n_children = 3,
		.n_required_children = 1,
	},
	/* probe.extension */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
	/* item.label */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 7 },
	},
	/* item.leaf */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 14,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* item.index */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 7 },
	},
};

/* echo-reply reply extension points */
static const struct ctlv_stub_ref echo_reply_reply_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.node = 10,
	},
};

/* series-dump request */
static const struct ctlv_schema_node series_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 4,
		.n_required_children = 0,
	},
	/* series.chunks */
	{
		.attr_id = 0x0001000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.items */
	{
		.attr_id = 0x0001000400000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.values */
	{
		.attr_id = 0x0001000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.totals */
	{
		.attr_id = 0x0001000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
};

/* series-dump reply */
static const struct ctlv_schema_node series_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 0,
	},
	/* series.chunk */
	{
		.attr_id = 0x0001000400000005ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 3,
		.n_children = 2,
		.n_required_children = 1,
	},
	/* series.total */
	{
		.attr_id = 0x0001000400000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.range = { .min = 0, .max = U64_MAX },
	},
	/* series.index */
	{
		.attr_id = 0x0001000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.item */
	{
		.attr_id = 0x0001000400000006ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 5,
		.n_children = 2,
		.n_required_children = 1,
	},
	/* series.index */
	{
		.attr_id = 0x0001000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 8 },
	},
	/* series.value */
	{
		.attr_id = 0x0001000400000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* ping request */
static const struct ctlv_schema_node ping_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 5,
		.n_required_children = 0,
	},
	/* ping.count */
	{
		.attr_id = 0x0001000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 64 },
	},
	/* ping.serial */
	{
		.attr_id = 0x0001000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* ping.payload */
	{
		.attr_id = 0x0001000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 1024 },
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

/* beeped reply */
static const struct ctlv_schema_node beeped_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 1,
	},
	/* ping.serial */
	{
		.attr_id = 0x0001000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* ping.filler */
	{
		.attr_id = 0x0001000500000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 1024 },
	},
};

/* transfer request */
static const struct ctlv_schema_node transfer_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 8,
		.n_required_children = 0,
	},
	/* transfer.source */
	{
		.attr_id = 0x0001000600000001ULL,
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
		.attr_id = 0x0001000600000002ULL,
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
		.attr_id = 0x0001000600000003ULL,
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
		.attr_id = 0x0001000600000004ULL,
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
		.attr_id = 0x0001000600000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 255 },
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

/* transfer reply */
static const struct ctlv_schema_node transfer_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 0,
	},
	/* transfer.sum */
	{
		.attr_id = 0x0001000600000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* transfer.written */
	{
		.attr_id = 0x000100060000000aULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* transfer.distinct */
	{
		.attr_id = 0x000100060000000bULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_test_ctlv_op_schemas[CTLV_TEST_N_OPS] = {
	/* noop */
	{
		.op_id = 0x0001000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(noop_request_nodes),
		.sizes = {
			.request_max_len = 16,
		},
	},
	/* probe-get */
	{
		.op_id = 0x0001000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.pre_post = true,
		.request = CTLV_SCHEMA_TREE(probe_get_request_nodes),
		.reply =
			CTLV_SCHEMA_TREE_STUBS(probe_get_reply_nodes,
					       probe_get_reply_stubs),
		.sizes = {
			.request_max_len = 40,
			.query_reply_min_len = 40,
			.query_reply_suggested_len = 65536,
		},
	},
	/* dynamic-noop */
	{
		.op_id = 0x0001000000000003ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.pre_post = true,
		.request = CTLV_SCHEMA_TREE(dynamic_noop_request_nodes),
		.sizes = {
			.request_max_len = 16,
		},
	},
	/* dynamic-toggle */
	{
		.op_id = 0x0001000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(dynamic_toggle_request_nodes),
		.reply = CTLV_SCHEMA_TREE(dynamic_toggle_reply_nodes),
		.sizes = {
			.request_max_len = 40,
			.action_reply_max_len = 40,
		},
	},
	/* pinged */
	{
		.op_id = 0x0001000000000005ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(pinged_reply_nodes),
		.sizes = {
			.event_record_max_len = 40,
		},
	},
	/* echo */
	{
		.op_id = 0x0001000000000006ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request =
			CTLV_SCHEMA_TREE_STUBS(echo_request_nodes,
					       echo_request_stubs),
		.sizes = {
			.request_max_len = 736,
		},
	},
	/* echo-reply */
	{
		.op_id = 0x0001000000000007ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request =
			CTLV_SCHEMA_TREE_STUBS(echo_reply_request_nodes,
					       echo_reply_request_stubs),
		.reply =
			CTLV_SCHEMA_TREE_STUBS(echo_reply_reply_nodes,
					       echo_reply_reply_stubs),
		.sizes = {
			.request_max_len = 736,
			.action_reply_max_len = 512,
		},
	},
	/* series-dump */
	{
		.op_id = 0x0001000000000008ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.pre_post = true,
		.request = CTLV_SCHEMA_TREE(series_dump_request_nodes),
		.reply = CTLV_SCHEMA_TREE(series_dump_reply_nodes),
		.sizes = {
			.request_max_len = 112,
			.query_reply_min_len = 72,
			.query_reply_suggested_len = 4096,
		},
	},
	/* ping */
	{
		.op_id = 0x0001000000000009ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(ping_request_nodes),
		.sizes = {
			.request_max_len = 136,
		},
	},
	/* beeped */
	{
		.op_id = 0x000100000000000aULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(beeped_reply_nodes),
		.sizes = {
			.event_record_max_len = 1104,
		},
	},
	/* transfer */
	{
		.op_id = 0x000100000000000bULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(transfer_request_nodes),
		.reply = CTLV_SCHEMA_TREE(transfer_reply_nodes),
		.sizes = {
			.request_max_len = 896,
			.action_reply_max_len = 88,
		},
	},
};

static_assert(ARRAY_SIZE(ctlv_test_ctlv_op_schemas) == CTLV_TEST_N_OPS);

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_ctlv_schema = {
	.name = "ctlv-test",
	.family_id = 1,
	.ops = ctlv_test_ctlv_op_schemas,
	.n_ops = CTLV_TEST_N_OPS,
};

/* What this family and the chain below it cost a uevent. */
static_assert(93 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* The generated numbers against the limits they were computed from. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(1104 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(2 <= CTLV_MAX_DECLARED_NEST_DEPTH);

/* One entry per action and query this family declares, in declaration order. */
#define CTLV_TEST_N_BINDINGS 9

static const struct ctlv_op ctlv_test_ctlv_ops[CTLV_TEST_N_BINDINGS] = {
	/* noop */
	{
		.action = ctlv_test_noop_action,
	},
	/* probe-get */
	{
		.query = ctlv_test_probe_get_query,
		.pre = ctlv_test_probe_get_pre,
		.post = ctlv_test_probe_get_post,
	},
	/* dynamic-noop */
	{
		.action = ctlv_test_dynamic_noop_action,
		.pre = ctlv_test_dynamic_noop_pre,
		.post = ctlv_test_dynamic_noop_post,
	},
	/* dynamic-toggle */
	{
		.action = ctlv_test_dynamic_toggle_action,
	},
	/* echo */
	{
		.action = ctlv_test_echo_action,
	},
	/* echo-reply */
	{
		.action = ctlv_test_echo_reply_action,
	},
	/* series-dump */
	{
		.query = ctlv_test_series_dump_query,
		.pre = ctlv_test_series_dump_pre,
		.post = ctlv_test_series_dump_post,
	},
	/* ping */
	{
		.action = ctlv_test_ping_action,
	},
	/* transfer */
	{
		.action = ctlv_test_transfer_action,
	},
};

static_assert(ARRAY_SIZE(ctlv_test_ctlv_ops) == CTLV_TEST_N_BINDINGS);

/* What a driver creates devices with. */
const struct ctlv_family ctlv_test_ctlv_family = {
	.schema = &ctlv_test_ctlv_schema,
	.parent = &ctlv_ctlv_family,
	.ops = ctlv_test_ctlv_ops,
	.n_ops = CTLV_TEST_N_BINDINGS,
	.pre = ctlv_test_op_pre,
	.post = ctlv_test_op_post,
	.priv_size = ctlv_test_priv_size,
	.init = ctlv_test_init,
	.post_init = ctlv_test_post_init,
	.pre_fini = ctlv_test_pre_fini,
	.fini = ctlv_test_fini,
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_ctlv_family, "CTLV");
