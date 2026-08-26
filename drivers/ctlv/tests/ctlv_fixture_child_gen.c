// SPDX-License-Identifier: GPL-2.0
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture-child.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_fixture_child_gen.h"

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);

/* child-info-get request */
static const struct ctlv_schema_node child_info_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 1,
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

/* child-info-get reply */
static const struct ctlv_schema_node child_info_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 1,
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

/* child-dump request */
static const struct ctlv_schema_node child_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* child-dump reply */
static const struct ctlv_schema_node child_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* child.dump */
	{
		.attr_id = 0x0002000100000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
};

/* extension-point of ctlv-fixture, filled here */
static const struct ctlv_schema_node extension_point_fill_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 1,
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
		.members = CTLV_SCHEMA_TREE(extension_point_fill_nodes),
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema
	ctlv_fixture_child_ctlv_op_schemas[CTLV_FIXTURE_CHILD_N_OPS] = {
	/* child-info-get */
	{
		.op_id = 0x0002000000000001ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(child_info_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(child_info_get_reply_nodes),
		.sizes = {
			.request_max_len = 64,
			.query_reply_min_len = 64,
			.query_reply_suggested_len = 65536,
		},
	},
	/* child-dump */
	{
		.op_id = 0x0002000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(child_dump_request_nodes),
		.reply = CTLV_SCHEMA_TREE(child_dump_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 4128,
			.query_reply_suggested_len = 65536,
		},
	},
};

static_assert(ARRAY_SIZE(ctlv_fixture_child_ctlv_op_schemas) ==
	      CTLV_FIXTURE_CHILD_N_OPS);

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_fixture_child_ctlv_schema = {
	.name = "ctlv-fixture-child",
	.family_id = 2,
	.ops = ctlv_fixture_child_ctlv_op_schemas,
	.n_ops = CTLV_FIXTURE_CHILD_N_OPS,
	.fills = ctlv_fixture_child_ctlv_fills,
	.n_fills = ARRAY_SIZE(ctlv_fixture_child_ctlv_fills),
};

/* What this family and the chain below it cost a uevent. */
static_assert(142 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* The generated numbers against the limits they were computed from. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
