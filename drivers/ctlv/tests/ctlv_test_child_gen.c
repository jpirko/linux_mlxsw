// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>
#include <uapi/linux/ctlv_test_child_gen.h>

#include "ctlv_test_child_gen.h"

static const struct ctlv_node_schema extra_noop_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema extension_point_fill_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, 1, 0, U64_MAX),
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill_schema ctlv_test_child_stub_fill_schemas[] = {
	CTLV_STUB_FILL_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT, extension_point),
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
	.stub_fill_schemas = ctlv_test_child_stub_fill_schemas,
	.stub_fill_schemas_count = ARRAY_SIZE(ctlv_test_child_stub_fill_schemas),
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
static const struct ctlv_stub_ops ctlv_test_child_stub_ops[] = {
	CTLV_STUB_OPS(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT,
		      ctlv_test_child_extension_point),
};

/* Family descriptor used by ctlv_create(). */
const struct ctlv_family ctlv_test_child_family = {
	.schema = &ctlv_test_child_schema,
	.parent = &ctlv_test_family,
	.ops = ctlv_test_child_ops,
	.ops_count = ARRAY_SIZE(ctlv_test_child_ops),
	.pre = ctlv_test_child_op_pre,
	.post = ctlv_test_child_op_post,
	.priv_size = ctlv_test_child_priv_size,
	.pre_init = ctlv_test_child_pre_init,
	.init = ctlv_test_child_init,
	.pre_fini = ctlv_test_child_pre_fini,
	.fini = ctlv_test_child_fini,
	.stub_ops = ctlv_test_child_stub_ops,
	.stub_ops_count = ARRAY_SIZE(ctlv_test_child_stub_ops),
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_child_family, "CTLV");
