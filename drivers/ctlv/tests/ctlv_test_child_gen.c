// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_test_child_gen.h"

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);

/* extra-noop request */
static const struct ctlv_schema_node extra_noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* extension-point of ctlv-test, filled here */
static const struct ctlv_schema_node extension_point_fill_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 0,
	},
	/* child.extra */
	{
		.attr_id = 0x0002000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill ctlv_test_child_ctlv_fills[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.members = CTLV_SCHEMA_TREE(extension_point_fill_nodes),
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema
	ctlv_test_child_ctlv_op_schemas[CTLV_TEST_CHILD_N_OPS] = {
	/* extra-noop */
	{
		.op_id = 0x0002000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(extra_noop_request_nodes),
		.sizes = {
			.request_max_len = 16,
		},
	},
};

static_assert(ARRAY_SIZE(ctlv_test_child_ctlv_op_schemas) ==
	      CTLV_TEST_CHILD_N_OPS);

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_child_ctlv_schema = {
	.name = "ctlv-test-child",
	.family_id = 2,
	.ops = ctlv_test_child_ctlv_op_schemas,
	.n_ops = CTLV_TEST_CHILD_N_OPS,
	.fills = ctlv_test_child_ctlv_fills,
	.n_fills = ARRAY_SIZE(ctlv_test_child_ctlv_fills),
};

/* What this family and the chain below it cost a uevent. */
static_assert(133 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* The generated numbers against the limits they were computed from. */
static_assert(16 <= CTLV_MAX_INLINE_MESSAGE_LEN);

/* One entry per action and query this family declares, in declaration order. */
#define CTLV_TEST_CHILD_N_BINDINGS 1

static const struct ctlv_op
	ctlv_test_child_ctlv_ops[CTLV_TEST_CHILD_N_BINDINGS] = {
	/* extra-noop */
	{
		.action = ctlv_test_child_extra_noop_action,
	},
};

static_assert(ARRAY_SIZE(ctlv_test_child_ctlv_ops) ==
	      CTLV_TEST_CHILD_N_BINDINGS);

/* What this family does with each extension point it fills. */
static const struct ctlv_stub_ops ctlv_test_child_ctlv_stubs[] = {
	/* extension-point */
	{
		.family_id = 1,
		.set_id = 1,
		.fill = ctlv_test_child_extension_point_fill,
		.read = ctlv_test_child_extension_point_read,
	},
};

/* What a driver creates devices with. */
const struct ctlv_family ctlv_test_child_ctlv_family = {
	.schema = &ctlv_test_child_ctlv_schema,
	.parent = &ctlv_test_ctlv_family,
	.ops = ctlv_test_child_ctlv_ops,
	.n_ops = CTLV_TEST_CHILD_N_BINDINGS,
	.pre = ctlv_test_child_op_pre,
	.post = ctlv_test_child_op_post,
	.priv_size = ctlv_test_child_priv_size,
	.pre_init = ctlv_test_child_pre_init,
	.init = ctlv_test_child_init,
	.pre_fini = ctlv_test_child_pre_fini,
	.fini = ctlv_test_child_fini,
	.stubs = ctlv_test_child_ctlv_stubs,
	.n_stubs = ARRAY_SIZE(ctlv_test_child_ctlv_stubs),
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_child_ctlv_family, "CTLV");
