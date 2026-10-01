// SPDX-License-Identifier: GPL-2.0
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture-child.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_fixture_child_gen.h"

static const struct ctlv_schema_node child_info_get_request_nodes[] = {
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
	/* child.serial */
	{
		.attr_id = 0x0002000100000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node child_info_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* child.tag */
	{
		.attr_id = 0x0002000100000001ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 31 },
	},
	/* child.serial */
	{
		.attr_id = 0x0002000100000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_schema_node child_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node child_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 1,
	},
	/* child.dump */
	{
		.attr_id = 0x0002000100000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
};

static const struct ctlv_schema_node extension_point_fill_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 2,
		.required_children_count = 1,
	},
	/* child.tag */
	{
		.attr_id = 0x0002000100000001ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 31 },
	},
	/* child.serial */
	{
		.attr_id = 0x0002000100000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill ctlv_fixture_child_ctlv_fills[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.members = {
			.nodes = extension_point_fill_nodes,
			.nodes_count =
				ARRAY_SIZE(extension_point_fill_nodes),
		},
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_fixture_child_ctlv_op_schemas[] = {
	{
		.op_id = 0x0002000000000001ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = child_info_get_request_nodes,
			.nodes_count =
				ARRAY_SIZE(child_info_get_request_nodes),
		},
		.reply = {
			.nodes = child_info_get_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(child_info_get_reply_nodes),
		},
		.sizes = {
			.request_max_len = 64,
			.query_reply_min_len = 64,
			.query_reply_suggested_len = 65536,
		},
	},
	{
		.op_id = 0x0002000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request = {
			.nodes = child_dump_request_nodes,
			.nodes_count =
				ARRAY_SIZE(child_dump_request_nodes),
		},
		.reply = {
			.nodes = child_dump_reply_nodes,
			.nodes_count =
				ARRAY_SIZE(child_dump_reply_nodes),
		},
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 4128,
			.query_reply_suggested_len = 65536,
		},
	},
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_fixture_child_ctlv_schema = {
	.name = "ctlv-fixture-child",
	.family_id = 2,
	.ops = ctlv_fixture_child_ctlv_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_fixture_child_ctlv_op_schemas),
	.fills = ctlv_fixture_child_ctlv_fills,
	.fills_count = ARRAY_SIZE(ctlv_fixture_child_ctlv_fills),
};

/* Maximum uevent cost for this family chain. */
static_assert(142 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
