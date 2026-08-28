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

#include "ctlv_test_child_gen.h"

static const struct ctlv_schema_node extra_noop_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_schema_node extension_point_fill_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 0,
	},
	/* child.extra */
	{
		.attr_id = 0x0004000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill ctlv_test_child_ctlv_fills[] = {
	/* extension-point */
	{
		.family_id = 3,
		.set_id = 1,
		.members = {
			.nodes = extension_point_fill_nodes,
			.nodes_count =
				ARRAY_SIZE(extension_point_fill_nodes),
		},
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_test_child_ctlv_op_schemas[] = {
	{
		.op_id = 0x0004000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = {
			.nodes = extra_noop_request_nodes,
			.nodes_count =
				ARRAY_SIZE(extra_noop_request_nodes),
		},
		.sizes = {
			.request_max_len = 16,
		},
	},
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_child_ctlv_schema = {
	.name = "ctlv-test-child",
	.family_id = 4,
	.ops = ctlv_test_child_ctlv_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_test_child_ctlv_op_schemas),
	.fills = ctlv_test_child_ctlv_fills,
	.fills_count = ARRAY_SIZE(ctlv_test_child_ctlv_fills),
};

/* Maximum uevent cost for this family chain. */
static_assert(133 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(16 <= CTLV_MAX_INLINE_MESSAGE_LEN);

/* Local action and query bindings in declaration order. */
static const struct ctlv_op ctlv_test_child_ctlv_ops[] = {
	{
		.action = ctlv_test_child_extra_noop_action,
	},
};

/* Extension-point callbacks. */
static const struct ctlv_stub_ops ctlv_test_child_ctlv_stubs[] = {
	{
		.family_id = 3,
		.set_id = 1,
		.fill = ctlv_test_child_extension_point_fill,
		.read = ctlv_test_child_extension_point_read,
	},
};

/* Family descriptor used by ctlv_create(). */
const struct ctlv_family ctlv_test_child_ctlv_family = {
	.schema = &ctlv_test_child_ctlv_schema,
	.parent = &ctlv_test_ctlv_family,
	.ops = ctlv_test_child_ctlv_ops,
	.ops_count = ARRAY_SIZE(ctlv_test_child_ctlv_ops),
	.pre = ctlv_test_child_op_pre,
	.post = ctlv_test_child_op_post,
	.priv_size = ctlv_test_child_priv_size,
	.pre_init = ctlv_test_child_pre_init,
	.init = ctlv_test_child_init,
	.pre_fini = ctlv_test_child_pre_fini,
	.fini = ctlv_test_child_fini,
	.stubs = ctlv_test_child_ctlv_stubs,
	.stubs_count = ARRAY_SIZE(ctlv_test_child_ctlv_stubs),
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_child_ctlv_family, "CTLV");
