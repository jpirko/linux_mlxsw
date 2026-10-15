// SPDX-License-Identifier: GPL-2.0
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture-child.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>
#include <linux/ctlv_fixture_child_gen.h>

#include "ctlv_fixture_child_gen.h"

static const struct ctlv_node_schema child_info_get_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_ATTR_SCALAR_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema child_info_get_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_STRING(CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG, 1, 0, 31),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema child_dump_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema child_dump_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_BINARY(CTLV_FIXTURE_CHILD_ATTR_CHILD_DUMP, 1, 0, 4096),
};

static const struct ctlv_node_schema extension_point_fill_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_STRING(CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG, 1, 0, 31),
	CTLV_NODE_SCHEMA_UINT(CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL, 1, 0, U64_MAX),
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill_schema ctlv_fixture_child_stub_fill_schemas[] = {
	CTLV_STUB_FILL_SCHEMA(CTLV_FAMILY_FIXTURE, CTLV_FIXTURE_SET_EXTENSION_POINT,
			      extension_point),
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_fixture_child_op_schemas[] = {
	CTLV_OP_SCHEMA_QUERY(CTLV_FIXTURE_CHILD_OP_CHILD_INFO_GET, CTLV_OP_FD_ACCESS_READ_WRITE,
			     CTLV_OP_AVAILABILITY_FIXED, 64, 64, 65536,
			     CTLV_SCHEMA_TREE(child_info_get_request),
			     CTLV_SCHEMA_TREE(child_info_get_reply)),
	CTLV_OP_SCHEMA_QUERY(CTLV_FIXTURE_CHILD_OP_CHILD_DUMP, CTLV_OP_FD_ACCESS_READ_WRITE,
			     CTLV_OP_AVAILABILITY_DYNAMIC, 16, 4128, 65536,
			     CTLV_SCHEMA_TREE(child_dump_request),
			     CTLV_SCHEMA_TREE(child_dump_reply), .optional = true),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_fixture_child_schema = {
	.name = "ctlv-fixture-child",
	.family_id = 2,
	.ops = ctlv_fixture_child_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_fixture_child_op_schemas),
	.stub_fill_schemas = ctlv_fixture_child_stub_fill_schemas,
	.stub_fill_schemas_count = ARRAY_SIZE(ctlv_fixture_child_stub_fill_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(142 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
