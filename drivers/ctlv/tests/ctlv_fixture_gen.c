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
#include <linux/ctlv_fixture_gen.h>

#include "ctlv_fixture_gen.h"

static const struct ctlv_node_schema noop_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema scalars_set_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 11, 11),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_FIXTURE_ATTR_SCALAR_S, 1, -100, 100),
	CTLV_NODE_SCHEMA_SINT(CTLV_FIXTURE_ATTR_SCALAR_DELTA, 1, S64_MIN, S64_MAX),
	CTLV_NODE_SCHEMA_UINT_UNIT(CTLV_FIXTURE_ATTR_SCALAR_FREQ, 1, 0, U64_MAX, CTLV_UNIT_HERTZ,
				   3),
	CTLV_NODE_SCHEMA_UINT_UNIT(CTLV_FIXTURE_ATTR_SCALAR_SIZE, 1, 0, U64_MAX, CTLV_UNIT_BYTES,
				   0),
	CTLV_NODE_SCHEMA_ENUM(CTLV_FIXTURE_ATTR_SCALAR_MODE, 1, 3),
	CTLV_NODE_SCHEMA_FLAGS(CTLV_FIXTURE_ATTR_SCALAR_QUIRKS, 1, CTLV_FIXTURE_QUIRKS_MASK),
	CTLV_NODE_SCHEMA_BOOL(CTLV_FIXTURE_ATTR_SCALAR_FLAG, 1),
	CTLV_NODE_SCHEMA_STRING(CTLV_FIXTURE_ATTR_SCALAR_TEXT, 1, 1, 15),
	CTLV_NODE_SCHEMA_BINARY(CTLV_FIXTURE_ATTR_SCALAR_FIXED, 1, 8, 8),
	CTLV_NODE_SCHEMA_BINARY(CTLV_FIXTURE_ATTR_SCALAR_BOUNDED, 1, 4, 64),
};

static const struct ctlv_node_schema scalars_set_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
};

static const struct ctlv_node_schema counts_get_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 3, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_COUNT_FEW, 3, 0, U64_MAX),
	CTLV_NODE_SCHEMA_ENUM(CTLV_FIXTURE_ATTR_SCALAR_MODE, 1, 3),
	CTLV_NODE_SCHEMA_NEST(CTLV_FIXTURE_ATTR_NEST_REQUEST, 2, 4, 2, 2),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_FIXTURE_ATTR_SCALAR_S, 1, -100, 100),
};

static const struct ctlv_node_schema counts_get_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_COUNT_MANY, CTLV_NODE_SCHEMA_UNLIMITED, 0, U64_MAX),
};

static const struct ctlv_node_schema nests_dump_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema nests_dump_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_NEST(CTLV_FIXTURE_ATTR_NEST_LEVEL1, 2, 3, 2, 1),
	CTLV_NODE_SCHEMA_NEST(CTLV_FIXTURE_ATTR_NEST_EXTENSION, 1, 0, 0, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_NEST(CTLV_FIXTURE_ATTR_NEST_LEVEL2, CTLV_NODE_SCHEMA_UNLIMITED, 5, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_NEST(CTLV_FIXTURE_ATTR_NEST_LEVEL3, CTLV_NODE_SCHEMA_UNLIMITED, 7, 2, 2),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_FIXTURE_ATTR_SCALAR_S, 1, -100, 100),
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_schema nests_dump_reply_stub_schemas[] = {
	CTLV_STUB_SCHEMA(CTLV_FAMILY_FIXTURE, CTLV_FIXTURE_SET_EXTENSION_POINT, 2),
};

static const struct ctlv_node_schema blobs_submit_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 4, 2),
	CTLV_NODE_SCHEMA_BLOB(CTLV_FIXTURE_ATTR_REGION_INPUT_VA, 1, 4096, 1,
			      BIT(CTLV_BLOB_TYPE_VA),
			      CTLV_BLOB_DIRECTION_INPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_FIXTURE_ATTR_REGION_OUTPUT_FD, 1, 1048576, 4096,
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_OUTPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_FIXTURE_ATTR_REGION_INOUT_ANY, 1, 65536, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INOUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_FIXTURE_ATTR_REGION_PIECES, 3, 4096, 1,
			      BIT(CTLV_BLOB_TYPE_VA),
			      CTLV_BLOB_DIRECTION_INPUT),
};

static const struct ctlv_node_schema collapse_check_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_ENUM(CTLV_FIXTURE_ATTR_SCALAR_MODE, 1, 3),
};

static const struct ctlv_node_schema thing_changed_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 2),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_FIXTURE_ATTR_SCALAR_S, 1, -100, 100),
};

static const struct ctlv_node_schema pinged_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_fixture_op_schemas[] = {
	CTLV_OP_SCHEMA_ACTION(CTLV_FIXTURE_OP_NOOP, CTLV_OP_FD_ACCESS_READ,
			      CTLV_OP_AVAILABILITY_FIXED, 16, 0, CTLV_SCHEMA_TREE(noop_request),
			      CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_ACTION(CTLV_FIXTURE_OP_SCALARS_SET, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_DYNAMIC, 344, 40,
			      CTLV_SCHEMA_TREE(scalars_set_request),
			      CTLV_SCHEMA_TREE(scalars_set_reply)),
	CTLV_OP_SCHEMA_QUERY(CTLV_FIXTURE_OP_COUNTS_GET, CTLV_OP_FD_ACCESS_READ,
			     CTLV_OP_AVAILABILITY_DYNAMIC, 240, 40, 65536,
			     CTLV_SCHEMA_TREE(counts_get_request),
			     CTLV_SCHEMA_TREE(counts_get_reply), .pre_post = true),
	CTLV_OP_SCHEMA_QUERY(CTLV_FIXTURE_OP_NESTS_DUMP, CTLV_OP_FD_ACCESS_READ_WRITE,
			     CTLV_OP_AVAILABILITY_FIXED, 16, 88, 4096,
			     CTLV_SCHEMA_TREE(nests_dump_request),
			     CTLV_SCHEMA_TREE_STUBS(nests_dump_reply)),
	CTLV_OP_SCHEMA_ACTION(CTLV_FIXTURE_OP_BLOBS_SUBMIT, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 592, 0,
			      CTLV_SCHEMA_TREE(blobs_submit_request), CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_ACTION(CTLV_FIXTURE_OP_COLLAPSE_CHECK, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 64, 0,
			      CTLV_SCHEMA_TREE(collapse_check_request), CTLV_SCHEMA_TREE_NONE(),
			      .optional = true),
	CTLV_OP_SCHEMA_EVENT(CTLV_FIXTURE_OP_THING_CHANGED, 88,
			     CTLV_SCHEMA_TREE(thing_changed_reply)),
	CTLV_OP_SCHEMA_EVENT(CTLV_FIXTURE_OP_PINGED, 40, CTLV_SCHEMA_TREE(pinged_reply)),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_fixture_schema = {
	.name = "ctlv-fixture",
	.family_id = 1,
	.ops = ctlv_fixture_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_fixture_op_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(99 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(88 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(3 <= CTLV_MAX_DECLARED_NEST_DEPTH);
