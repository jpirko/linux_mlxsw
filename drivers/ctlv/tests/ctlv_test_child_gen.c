// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/*
 * Generated from Documentation/ctlv/specs/ctlv-test-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/ctlv/ctlv_gen.h>
#include <uapi/ctlv/ctlv_test_child_gen.h>

#include "ctlv_test_child_gen.h"

static const struct ctlv_node_schema extra_noop_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema extension_point_fill_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA, 1, 0, U64_MAX),
};

/* The extension points this family fills. */
static const struct ctlv_ext_fill_schema ctlv_test_child_ext_fill_schemas[] = {
	CTLV_EXT_FILL_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_EXT_EXTENSION_POINT, extension_point),
};

/* The enum extension points this family fills. */
static const struct ctlv_enum_fill_schema ctlv_test_child_enum_fill_schemas[] = {
	CTLV_ENUM_FILL_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_ENUM_EXT_ECHO_VARIANT, 2),
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_test_child_op_schemas[] = {
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_CHILD_OP_EXTRA_NOOP, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 16, 0,
			      CTLV_SCHEMA_TREE(extra_noop_request), CTLV_SCHEMA_TREE_NONE()),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_child_schema = {
	.name = "ctlv-test-child",
	.family_id = 4,
	.ops = ctlv_test_child_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_test_child_op_schemas),
	.ext_fill_schemas = ctlv_test_child_ext_fill_schemas,
	.ext_fill_schemas_count = ARRAY_SIZE(ctlv_test_child_ext_fill_schemas),
	.enum_fill_schemas = ctlv_test_child_enum_fill_schemas,
	.enum_fill_schemas_count = ARRAY_SIZE(ctlv_test_child_enum_fill_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(133 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(16 <= CTLV_MAX_INLINE_MESSAGE_LEN);

/* Local action and query bindings in declaration order. */
static const struct ctlv_op ctlv_test_child_ops[] = {
	CTLV_OP_ACTION(ctlv_test_child_extra_noop),
};

/* Extension-point callbacks. */
static const struct ctlv_ext_ops ctlv_test_child_ext_ops[] = {
	CTLV_EXT_OPS(CTLV_FAMILY_TEST, CTLV_TEST_EXT_EXTENSION_POINT,
		     ctlv_test_child_extension_point),
};

/* Family descriptor used by ctlv_init(). */
const struct ctlv_family ctlv_test_child_family = {
	.schema = &ctlv_test_child_schema,
	.parent = &ctlv_test_family,
	.ops = ctlv_test_child_ops,
	.ops_count = ARRAY_SIZE(ctlv_test_child_ops),
	.ext_ops = ctlv_test_child_ext_ops,
	.ext_ops_count = ARRAY_SIZE(ctlv_test_child_ext_ops),
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_child_family, "CTLV");
