// SPDX-License-Identifier: GPL-2.0
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_fixture_gen.h"

static const struct ctlv_schema_node noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node scalars_set_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 11,
		.required_children_count = 11,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* scalar.s */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
	/* scalar.delta */
	{
		.attr_id = 0x0001000100000003ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = S64_MIN, .max = S64_MAX },
	},
	/* scalar.freq */
	{
		.attr_id = 0x0001000100000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
		.unit = CTLV_UNIT_HERTZ,
		.scale = 3,
	},
	/* scalar.size */
	{
		.attr_id = 0x0001000100000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	/* scalar.mode */
	{
		.attr_id = 0x0001000100000006ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* scalar.quirks */
	{
		.attr_id = 0x0001000100000007ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* scalar.flag */
	{
		.attr_id = 0x0001000100000008ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* scalar.text */
	{
		.attr_id = 0x0001000100000009ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 15 },
	},
	/* scalar.fixed */
	{
		.attr_id = 0x000100010000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 8, .max_len = 8 },
	},
	/* scalar.bounded */
	{
		.attr_id = 0x000100010000000bULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 4, .max_len = 64 },
	},
};

static const struct ctlv_schema_node scalars_set_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
};

static const struct ctlv_schema_node counts_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 3,
		.required_children_count = 0,
	},
	/* count.few */
	{
		.attr_id = 0x0001000200000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* scalar.mode */
	{
		.attr_id = 0x0001000100000006ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* nest.request */
	{
		.attr_id = 0x0001000300000005ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 4,
		.children_count = 2,
		.required_children_count = 2,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* scalar.s */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
};

static const struct ctlv_schema_node counts_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* count.many */
	{
		.attr_id = 0x0001000200000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 0, /* unlimited */
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node nests_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node nests_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* nest.level1 */
	{
		.attr_id = 0x0001000300000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 2,
		.first_child = 3,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* nest.extension */
	{
		.attr_id = 0x0001000300000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* nest.level2 */
	{
		.attr_id = 0x0001000300000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 5,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* nest.level3 */
	{
		.attr_id = 0x0001000300000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 7,
		.children_count = 2,
		.required_children_count = 2,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* scalar.s */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
	},
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_ref nests_dump_reply_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.node = 2,
	},
};

static const struct ctlv_schema_node blobs_submit_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 4,
		.required_children_count = 2,
	},
	/* region.input-va */
	{
		.attr_id = 0x0001000400000001ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 4096,
			.alignment = 1,
			.backings = BIT(CTLV_BLOB_TYPE_VA),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
	/* region.output-fd */
	{
		.attr_id = 0x0001000400000002ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 1048576,
			.alignment = 4096,
			.backings = BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_OUTPUT,
		},
	},
	/* region.inout-any */
	{
		.attr_id = 0x0001000400000003ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 65536,
			.alignment = 8,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_INOUT,
		},
	},
	/* region.pieces */
	{
		.attr_id = 0x0001000400000004ULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 3,
		.blob = {
			.max_len = 4096,
			.alignment = 1,
			.backings = BIT(CTLV_BLOB_TYPE_VA),
			.direction = CTLV_BLOB_DIRECTION_INPUT,
		},
	},
};

static const struct ctlv_schema_node collapse_check_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* scalar.mode */
	{
		.attr_id = 0x0001000100000006ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
};

static const struct ctlv_schema_node thing_changed_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 2,
	},
	/* scalar.u */
	{
		.attr_id = 0x0001000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4096 },
	},
	/* scalar.s */
	{
		.attr_id = 0x0001000100000002ULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = -100, .max = 100 },
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

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_fixture_ctlv_op_schemas[] = {
	{
		.op_id = 0x0001000000000001ULL,
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
		.op_id = 0x0001000000000002ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.request = {
			.nodes = scalars_set_request_nodes,
			.nodes_count =
				ARRAY_SIZE(scalars_set_request_nodes),
		},
		.reply = {
			.nodes = scalars_set_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(scalars_set_reply_nodes),
		},
		.sizes = {
			.request_max_len = 344,
			.action_reply_max_len = 40,
		},
	},
	{
		.op_id = 0x0001000000000003ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.pre_post = true,
		.request = {
			.nodes = counts_get_request_nodes,
			.nodes_count =
				ARRAY_SIZE(counts_get_request_nodes),
		},
		.reply = {
			.nodes = counts_get_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(counts_get_reply_nodes),
		},
		.sizes = {
			.request_max_len = 240,
			.query_reply_min_len = 40,
			.query_reply_suggested_len = 65536,
		},
	},
	{
		.op_id = 0x0001000000000004ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = nests_dump_request_nodes,
			.nodes_count =
				ARRAY_SIZE(nests_dump_request_nodes),
		},
		.reply = {
			.nodes = nests_dump_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(nests_dump_reply_nodes),
			.stubs = nests_dump_reply_stubs,
			.stubs_count =
				ARRAY_SIZE(nests_dump_reply_stubs),
		},
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 88,
			.query_reply_suggested_len = 4096,
		},
	},
	{
		.op_id = 0x0001000000000005ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = blobs_submit_request_nodes,
			.nodes_count =
				ARRAY_SIZE(blobs_submit_request_nodes),
		},
		.sizes = {
			.request_max_len = 592,
		},
	},
	{
		.op_id = 0x0001000000000006ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = {
			.nodes = collapse_check_request_nodes,
			.nodes_count =
				ARRAY_SIZE(collapse_check_request_nodes),
		},
		.sizes = {
			.request_max_len = 64,
		},
	},
	{
		.op_id = 0x0001000000000007ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = {
			.nodes = thing_changed_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(thing_changed_reply_nodes),
		},
		.sizes = {
			.event_record_max_len = 88,
		},
	},
	{
		.op_id = 0x0001000000000008ULL,
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
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_fixture_ctlv_schema = {
	.name = "ctlv-fixture",
	.family_id = 1,
	.ops = ctlv_fixture_ctlv_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_fixture_ctlv_op_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(99 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(88 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(3 <= CTLV_MAX_DECLARED_NEST_DEPTH);
