// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Generated-metadata and independent length-arithmetic tests.
 */

#include <kunit/test.h>
#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/ctlv_fixture_child_gen.h>
#include <linux/ctlv_fixture_gen.h>
#include <linux/log2.h>
#include <linux/minmax.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/linux/ctlv_gen.h>

#include "../schema.h"
#include "ctlv_fixture_child_gen.h"
#include "ctlv_fixture_gen.h"

/* One family's metadata; a family added later is covered by being listed. */
struct ctlv_meta_test_case {
	const char *name;
	const struct ctlv_family *family;
};

static const struct ctlv_family ctlv_meta_test_fixture_family = {
	.schema = &ctlv_fixture_schema,
	.parent = &ctlv_family,
};

static const struct ctlv_family ctlv_meta_test_fixture_child_family = {
	.schema = &ctlv_fixture_child_schema,
	.parent = &ctlv_meta_test_fixture_family,
};

static const struct ctlv_meta_test_case ctlv_meta_families[] = {
	{ "ctlv", &ctlv_family },
	{ "ctlv-fixture", &ctlv_meta_test_fixture_family },
	{ "ctlv-fixture-child", &ctlv_meta_test_fixture_child_family },
};

KUNIT_ARRAY_PARAM_DESC(metadata, ctlv_meta_families, name);

/* Index of the child of parent that carries attr_id, or -1. */
static int ctlv_meta_test_find_child(const struct ctlv_schema_tree *tree, u32 parent,
		      u64 attr_id)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[parent];
	u32 i;

	for (i = 0; i < node->children_count; i++)
		if (tree->node_schemas[node->first_child + i].attr_id == attr_id)
			return node->first_child + i;
	return -1;
}

/* One op a family declares itself. */
static const struct ctlv_op_schema *
ctlv_meta_test_find_op(const struct ctlv_family_schema *schema, u64 op_id)
{
	u32 i;

	for (i = 0; i < schema->ops_count; i++)
		if (schema->ops[i].op_id == op_id)
			return &schema->ops[i];
	return NULL;
}

/* One op of a whole chain, wherever in it the op was declared. */
static const struct ctlv_op_schema *
ctlv_meta_test_find_effective_op(const struct ctlv_family *family, u64 op_id)
{
	const struct ctlv_family *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;

	ctlv_schema_cursor_init(&cursor, family);
	while ((op = ctlv_schema_next_op(&cursor, &owner)))
		if (op->op_id == op_id)
			return op;
	return NULL;
}

/*
 * How deep the tree describes. A re-entering node ends the depth there: how
 * often such a level repeats is the transport's business, not the tree's.
 */
static u32 ctlv_meta_test_nest_depth(const struct ctlv_schema_tree *tree, u32 index)
{
	const struct ctlv_node_schema *node = &tree->node_schemas[index];
	u32 deepest = 0;
	u32 i;

	if (node->reenters)
		return 0;
	for (i = 0; i < node->children_count; i++) {
		u32 child = node->first_child + i;

		if (tree->node_schemas[child].kind != CTLV_SCHEMA_NEST)
			continue;
		deepest = max(deepest, 1 + ctlv_meta_test_nest_depth(tree, child));
	}
	return deepest;
}

static bool ctlv_meta_test_tree_has_unlimited(const struct ctlv_schema_tree *tree)
{
	u32 i;

	for (i = 0; i < tree->node_schemas_count; i++)
		if (!tree->node_schemas[i].max_occurrences)
			return true;
	return false;
}

static void ctlv_meta_test_op_ids(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	KUNIT_ASSERT_GT(test, schema->ops_count, 0);
	for (i = 0; i < schema->ops_count; i++) {
		u64 op_id = schema->ops[i].op_id;
		u16 family = CTLV_OP_FAMILY(op_id);
		u32 local = CTLV_OP_NUMBER(op_id);

		/* Zero is reserved, and the middle bits of an op ID are too. */
		KUNIT_EXPECT_EQ(test, CTLV_OP(family, local), op_id);
		/* Own ops only, numbered from one. */
		KUNIT_EXPECT_EQ(test, family, schema->family_id);
		KUNIT_EXPECT_EQ(test, local, i + 1);
	}
}

static void ctlv_meta_test_graph(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family *family = entry->family;
	const struct ctlv_family_schema *schema = family->schema;
	const struct ctlv_family *ancestor;
	u32 levels = ctlv_family_levels(family);

	KUNIT_EXPECT_NOT_NULL(test, schema->name);
	/* Every chain is headed by the root, and the root inherits nothing. */
	if (schema->family_id == CTLV_FAMILY_ROOT) {
		KUNIT_EXPECT_NULL(test, family->parent);
		KUNIT_EXPECT_EQ(test, levels, 1);
		return;
	}
	KUNIT_ASSERT_GT(test, levels, 1);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_family_ancestor(family, levels - 1),
			    &ctlv_family);

	/* A family is named after its parent, and its ID comes later. */
	for (ancestor = family->parent; ancestor; ancestor = ancestor->parent) {
		size_t len = strlen(ancestor->schema->name);

		KUNIT_EXPECT_LT(test, ancestor->schema->family_id,
				schema->family_id);
		KUNIT_ASSERT_GT(test, strlen(schema->name), len);
		KUNIT_EXPECT_MEMEQ(test, schema->name,
				   ancestor->schema->name, len);
		KUNIT_EXPECT_EQ(test, schema->name[len], '-');
	}
}

static void ctlv_meta_test_op_messages(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->ops_count; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		const struct ctlv_op_sizes *sizes = &op->sizes;

		switch (op->type) {
		case CTLV_OP_TYPE_ACTION:
			/* A request root always exists, a reply may not. */
			KUNIT_ASSERT_NOT_NULL(test, op->request.node_schemas);
			KUNIT_EXPECT_EQ(test, !!op->reply.node_schemas,
					!!sizes->action_reply_max_len);
			KUNIT_EXPECT_EQ(test, sizes->query_reply_min_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->event_record_max_len, 0);
			break;
		case CTLV_OP_TYPE_QUERY:
			KUNIT_ASSERT_NOT_NULL(test, op->request.node_schemas);
			KUNIT_ASSERT_NOT_NULL(test, op->reply.node_schemas);
			KUNIT_EXPECT_GT(test, sizes->query_reply_min_len, 0);
			KUNIT_EXPECT_GE(test,
					sizes->query_reply_suggested_len,
					sizes->query_reply_min_len);
			KUNIT_EXPECT_EQ(test, sizes->action_reply_max_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->event_record_max_len, 0);
			break;
		case CTLV_OP_TYPE_EVENT:
			/* An event is produced, never executed. */
			KUNIT_EXPECT_NULL(test, op->request.node_schemas);
			KUNIT_ASSERT_NOT_NULL(test, op->reply.node_schemas);
			KUNIT_EXPECT_EQ(test, sizes->request_max_len, 0);
			KUNIT_EXPECT_GT(test, sizes->event_record_max_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->action_reply_max_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->query_reply_min_len, 0);
			break;
		default:
			KUNIT_FAIL(test, "op %llu has type %d", op->op_id,
				   op->type);
			break;
		}
		if (op->type != CTLV_OP_TYPE_EVENT)
			KUNIT_EXPECT_GE(test, sizes->request_max_len,
					sizeof(struct ctlv_attr));
	}
}

static void ctlv_meta_test_assert_tree(struct kunit *test,
			const struct ctlv_schema_tree *tree)
{
	const struct ctlv_node_schema *node;
	u8 *claims;
	u32 i, j;

	KUNIT_ASSERT_GT(test, tree->node_schemas_count, 0);
	claims = kunit_kzalloc(test, tree->node_schemas_count, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, claims);

	node = &tree->node_schemas[0];
	KUNIT_EXPECT_EQ(test, node->attr_id, CTLV_ATTR_ROOT);
	KUNIT_EXPECT_EQ(test, node->kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, node->max_occurrences, 1);

	for (i = 0; i < tree->node_schemas_count; i++) {
		node = &tree->node_schemas[i];
		if (i)
			KUNIT_EXPECT_NE(test, node->attr_id,
					CTLV_ATTR_ROOT);
		KUNIT_EXPECT_LE(test, node->required_children_count,
				node->children_count);
		if (!node->children_count) {
			KUNIT_EXPECT_EQ(test, node->first_child, 0);
			KUNIT_EXPECT_FALSE(test, node->reenters);
			continue;
		}
		/* Only a nest has children. */
		KUNIT_EXPECT_EQ(test, node->kind, CTLV_SCHEMA_NEST);
		/*
		 * A re-entering node points back at a level it is in, whose
		 * members belong to the node that declared them.
		 */
		if (node->reenters)
			KUNIT_ASSERT_LT(test, node->first_child, i);
		else
			KUNIT_ASSERT_GT(test, node->first_child, i);
		KUNIT_ASSERT_LE(test, node->first_child + node->children_count,
				tree->node_schemas_count);
		if (node->reenters)
			continue;
		for (j = 0; j < node->children_count; j++)
			claims[node->first_child + j]++;
	}
	/* Every node but the root is the child of exactly one node. */
	KUNIT_EXPECT_EQ(test, claims[0], 0);
	for (i = 1; i < tree->node_schemas_count; i++)
		KUNIT_EXPECT_EQ(test, claims[i], 1);
	/* A message nests one level deeper than the tree describes. */
	KUNIT_EXPECT_LE(test, ctlv_meta_test_nest_depth(tree, 0),
			CTLV_MAX_DECLARED_NEST_DEPTH);

	/* An extension point widens a nest of this tree. */
	for (i = 0; i < tree->stub_schemas_count; i++) {
		const struct ctlv_stub_schema *stub_schema =
			&tree->stub_schemas[i];

		KUNIT_ASSERT_LT(test, stub_schema->node,
				tree->node_schemas_count);
		KUNIT_EXPECT_EQ(test,
				tree->node_schemas[stub_schema->node].kind,
				CTLV_SCHEMA_NEST);
		KUNIT_EXPECT_NE(test, stub_schema->set_id, 0);
	}
	if (!tree->stub_schemas_count)
		KUNIT_EXPECT_NULL(test, tree->stub_schemas);
}

static void ctlv_meta_test_tree_shape(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->ops_count; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		const struct ctlv_schema_tree *trees[] = {
			&op->request, &op->reply,
		};
		u32 j;

		for (j = 0; j < ARRAY_SIZE(trees); j++) {
			if (trees[j]->node_schemas)
				ctlv_meta_test_assert_tree(test, trees[j]);
			else
				KUNIT_EXPECT_EQ(test, trees[j]->node_schemas_count, 0);
		}
	}
	/* A fill schema is a tree of its own; its root stands for the stub. */
	for (i = 0; i < schema->stub_fill_schemas_count; i++) {
		const struct ctlv_stub_fill_schema *fill_schema =
			&schema->stub_fill_schemas[i];

		ctlv_meta_test_assert_tree(test, &fill_schema->members);
		KUNIT_EXPECT_GT(test,
				fill_schema->members.node_schemas[0].children_count,
				0);
		/* A family fills what an ancestor of it declared. */
		KUNIT_EXPECT_LT(test, fill_schema->family_id,
				schema->family_id);
		KUNIT_EXPECT_NE(test, fill_schema->set_id, 0);
	}
}

static void ctlv_meta_test_assert_constraints(struct kunit *test,
			       const struct ctlv_node_schema *node)
{
	bool integer = node->kind == CTLV_SCHEMA_UINT ||
		       node->kind == CTLV_SCHEMA_SINT;

	/* Only an integer payload can be expressed in a unit. */
	if (node->unit != CTLV_UNIT_NONE)
		KUNIT_EXPECT_TRUE(test, integer);
	else
		KUNIT_EXPECT_EQ(test, node->scale, 0);

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
		KUNIT_EXPECT_LE(test, node->range.min, node->range.max);
		break;
	case CTLV_SCHEMA_SINT:
		KUNIT_EXPECT_LE(test, (s64)node->range.min,
				(s64)node->range.max);
		break;
	case CTLV_SCHEMA_ENUM:
		/* Enum value zero is reserved, so the range starts at one. */
		KUNIT_EXPECT_EQ(test, node->range.min, 1);
		KUNIT_EXPECT_GE(test, node->range.max, node->range.min);
		break;
	case CTLV_SCHEMA_FLAGS:
		KUNIT_EXPECT_NE(test, node->flags_mask, 0);
		break;
	case CTLV_SCHEMA_STRING:
	case CTLV_SCHEMA_BINARY:
		KUNIT_EXPECT_LE(test, node->length.min_len,
				node->length.max_len);
		break;
	/* One byte holding zero or one leaves nothing to describe. */
	case CTLV_SCHEMA_BOOL:
	case CTLV_SCHEMA_NEST:
		break;
	case CTLV_SCHEMA_BLOB:
		KUNIT_EXPECT_GT(test, node->blob.max_len, 0);
		KUNIT_EXPECT_TRUE(test, is_power_of_2(node->blob.alignment));
		KUNIT_EXPECT_EQ(test, node->blob.max_len %
				node->blob.alignment, 0);
		KUNIT_EXPECT_NE(test, node->blob.backings, 0);
		KUNIT_EXPECT_EQ(test, node->blob.backings &
				~(BIT(CTLV_BLOB_TYPE_VA) |
				  BIT(CTLV_BLOB_TYPE_MEMFD) |
				  BIT(CTLV_BLOB_TYPE_DMA_BUF)), 0);
		KUNIT_EXPECT_GE(test, node->blob.direction,
				CTLV_BLOB_DIRECTION_INPUT);
		KUNIT_EXPECT_LE(test, node->blob.direction,
				CTLV_BLOB_DIRECTION_INOUT);
		break;
	default:
		KUNIT_FAIL(test, "attr %#llx has kind %d", node->attr_id,
			   node->kind);
		break;
	}
}

static void ctlv_meta_test_assert_tree_constraints(struct kunit *test,
				    const struct ctlv_schema_tree *tree)
{
	u32 node;

	for (node = 1; node < tree->node_schemas_count; node++)
		ctlv_meta_test_assert_constraints(test, &tree->node_schemas[node]);
}

static void ctlv_meta_test_constraints(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->ops_count; i++) {
		ctlv_meta_test_assert_tree_constraints(test, &schema->ops[i].request);
		ctlv_meta_test_assert_tree_constraints(test, &schema->ops[i].reply);
	}
	for (i = 0; i < schema->stub_fill_schemas_count; i++)
		ctlv_meta_test_assert_tree_constraints(test,
					&schema->stub_fill_schemas[i].members);
}

static void ctlv_meta_test_lengths(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	/*
	 * The generator's arithmetic against the core's, for an op with every
	 * extension point empty, which is what this family composes.
	 */
	for (i = 0; i < schema->ops_count; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		struct ctlv_op_sizes sizes;

		KUNIT_ASSERT_EQ(test,
				ctlv_schema_op_sizes(entry->family, op, &sizes),
				0);
		KUNIT_EXPECT_MEMEQ(test, &sizes, &op->sizes, sizeof(sizes));
	}
}

static void ctlv_meta_test_unlimited(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	/* Only a query reply, which may fragment, may be unbounded. */
	for (i = 0; i < schema->ops_count; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];

		KUNIT_EXPECT_FALSE(test, ctlv_meta_test_tree_has_unlimited(&op->request));
		if (op->type != CTLV_OP_TYPE_QUERY && op->reply.node_schemas)
			KUNIT_EXPECT_FALSE(test,
					   ctlv_meta_test_tree_has_unlimited(&op->reply));
	}
}

static void ctlv_meta_test_availability(struct kunit *test)
{
	const struct ctlv_meta_test_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->ops_count; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		bool valid_fd_access;

		/* A pair wraps an op somebody executes, so never an event. */
		if (op->type == CTLV_OP_TYPE_EVENT) {
			KUNIT_EXPECT_FALSE(test, op->pre_post);
			KUNIT_EXPECT_EQ(test, op->fd_access, 0);
		} else {
			valid_fd_access =
				op->fd_access == CTLV_OP_FD_ACCESS_READ ||
				op->fd_access == CTLV_OP_FD_ACCESS_READ_WRITE;
			KUNIT_EXPECT_TRUE(test, valid_fd_access);
		}

		switch (op->availability) {
		case CTLV_OP_AVAILABILITY_FIXED:
			break;
		case CTLV_OP_AVAILABILITY_DYNAMIC:
			/* An event is never executed, so never disabled. */
			KUNIT_EXPECT_NE(test, op->type, CTLV_OP_TYPE_EVENT);
			/* A core op can never be taken away. */
			KUNIT_EXPECT_NE(test, schema->family_id,
					CTLV_FAMILY_ROOT);
			break;
		default:
			KUNIT_FAIL(test, "op %#llx has availability %d",
				   op->op_id, op->availability);
			break;
		}
	}
}

static void ctlv_meta_test_core_ops(struct kunit *test)
{
	const struct ctlv_schema_tree *dump, *changed;
	const struct ctlv_op_schema *op;
	int entry, id, disabled;

	/* A dump entry and a change event use the same attrs. */
	op = ctlv_meta_test_find_op(ctlv_family.schema, CTLV_OP_OPS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	dump = &op->reply;
	op = ctlv_meta_test_find_op(ctlv_family.schema, CTLV_OP_OP_CHANGED);
	KUNIT_ASSERT_NOT_NULL(test, op);
	changed = &op->reply;

	entry = ctlv_meta_test_find_child(dump, 0, CTLV_ATTR_OP_ENTRY);
	KUNIT_ASSERT_GE(test, entry, 0);
	id = ctlv_meta_test_find_child(dump, entry, CTLV_ATTR_OP_ID);
	disabled = ctlv_meta_test_find_child(dump, entry, CTLV_ATTR_OP_DISABLED);
	KUNIT_ASSERT_GE(test, id, 0);
	KUNIT_ASSERT_GE(test, disabled, 0);
	KUNIT_EXPECT_EQ(test, dump->node_schemas[disabled].kind, CTLV_SCHEMA_BOOL);

	KUNIT_EXPECT_GE(test, ctlv_meta_test_find_child(changed, 0, CTLV_ATTR_OP_ID), 0);
	KUNIT_EXPECT_GE(test, ctlv_meta_test_find_child(changed, 0,
					 CTLV_ATTR_OP_DISABLED), 0);
	KUNIT_EXPECT_GE(test, ctlv_meta_test_find_child(changed, 0,
					 CTLV_ATTR_OP_STATE_GENERATION),
			0);

	/* A dump entry also reports the immutable mode beside the state. */
	KUNIT_EXPECT_GE(test, ctlv_meta_test_find_child(dump, entry,
					 CTLV_ATTR_OP_FD_ACCESS), 0);
	KUNIT_EXPECT_GE(test, ctlv_meta_test_find_child(dump, entry,
					 CTLV_ATTR_OP_AVAILABILITY), 0);
	KUNIT_EXPECT_LT(test, ctlv_meta_test_find_child(changed, 0,
					 CTLV_ATTR_OP_FD_ACCESS), 0);

	for (id = 0; id < ctlv_family.schema->ops_count; id++) {
		op = &ctlv_family.schema->ops[id];
		if (op->type != CTLV_OP_TYPE_EVENT)
			KUNIT_EXPECT_EQ(test, op->fd_access,
					CTLV_OP_FD_ACCESS_READ);
	}
}

/* One record per family of the chain, so the entry repeats unbounded. */
static void ctlv_meta_test_core_families(struct kunit *test)
{
	const struct ctlv_schema_tree *dump;
	const struct ctlv_op_schema *op;
	int entry, id, name;

	op = ctlv_meta_test_find_op(ctlv_family.schema, CTLV_OP_FAMILIES_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_EXPECT_EQ(test, op->type, CTLV_OP_TYPE_QUERY);
	dump = &op->reply;

	entry = ctlv_meta_test_find_child(dump, 0, CTLV_ATTR_FAMILY_ENTRY);
	KUNIT_ASSERT_GE(test, entry, 0);
	KUNIT_EXPECT_EQ(test, dump->node_schemas[entry].kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, dump->node_schemas[entry].max_occurrences, 0);

	id = ctlv_meta_test_find_child(dump, entry, CTLV_ATTR_FAMILY_ID);
	name = ctlv_meta_test_find_child(dump, entry, CTLV_ATTR_FAMILY_NAME);
	KUNIT_ASSERT_GE(test, id, 0);
	KUNIT_ASSERT_GE(test, name, 0);
	KUNIT_EXPECT_EQ(test, dump->node_schemas[id].kind, CTLV_SCHEMA_UINT);
	/* The root heads every chain, so family zero is reported like any. */
	KUNIT_EXPECT_EQ(test, dump->node_schemas[id].range.min, 0);
	KUNIT_EXPECT_EQ(test, dump->node_schemas[name].kind, CTLV_SCHEMA_STRING);
	KUNIT_EXPECT_GT(test, dump->node_schemas[name].length.max_len, 0);
	/* The record has nothing else in it. */
	KUNIT_EXPECT_EQ(test, dump->node_schemas[entry].children_count, 2);
}

static void ctlv_meta_test_dynamic_ops(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	u64 dynamic[] = {
		CTLV_FIXTURE_OP_SCALARS_SET, CTLV_FIXTURE_OP_COUNTS_GET,
		CTLV_FIXTURE_CHILD_OP_CHILD_DUMP,
	};
	u32 i;

	/* A descendant inherits availability and cannot disagree about it. */
	for (i = 0; i < ARRAY_SIZE(dynamic); i++) {
		op = ctlv_meta_test_find_effective_op(&ctlv_meta_test_fixture_child_family,
				       dynamic[i]);
		KUNIT_ASSERT_NOT_NULL(test, op);
		KUNIT_EXPECT_EQ(test, op->availability,
				CTLV_OP_AVAILABILITY_DYNAMIC);
	}
	op = ctlv_meta_test_find_effective_op(&ctlv_meta_test_fixture_child_family,
			       CTLV_FIXTURE_OP_NOOP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_EXPECT_EQ(test, op->availability, CTLV_OP_AVAILABILITY_FIXED);
}

static void ctlv_meta_test_uapi_constants(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	const struct ctlv_schema_tree *tree;
	int index;

	/* The op constants of a header name the entries of its table. */
	KUNIT_EXPECT_EQ(test, ctlv_family.schema->ops[0].op_id,
			CTLV_OP_FAMILIES_DUMP);
	KUNIT_EXPECT_EQ(test, ctlv_fixture_op_schemas[0].op_id,
			CTLV_FIXTURE_OP_NOOP);

	op = ctlv_meta_test_find_op(&ctlv_fixture_schema, CTLV_FIXTURE_OP_SCALARS_SET);
	KUNIT_ASSERT_NOT_NULL(test, op);
	tree = &op->request;

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_QUIRKS);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].kind, CTLV_SCHEMA_FLAGS);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].flags_mask,
			CTLV_FIXTURE_QUIRKS_MASK);

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_MODE);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].kind, CTLV_SCHEMA_ENUM);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].range.max, CTLV_FIXTURE_MODE_AUTO);

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_FREQ);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].unit,
			CTLV_FIXTURE_ATTR_SCALAR_FREQ_UNIT);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].scale,
			CTLV_FIXTURE_ATTR_SCALAR_FREQ_SCALE);

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_TEXT);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].kind, CTLV_SCHEMA_STRING);

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_FLAG);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].kind, CTLV_SCHEMA_BOOL);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].unit, CTLV_UNIT_NONE);
}

static void ctlv_meta_test_blob_policy(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	const struct ctlv_schema_tree *tree;
	int index;

	op = ctlv_meta_test_find_op(&ctlv_fixture_schema, CTLV_FIXTURE_OP_BLOBS_SUBMIT);
	KUNIT_ASSERT_NOT_NULL(test, op);
	tree = &op->request;

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_REGION_INPUT_VA);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].blob.direction,
			CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].blob.backings,
			BIT(CTLV_BLOB_TYPE_VA));
	/* A blob attr carries a descriptor, so it has no schema children. */
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].children_count, 0);

	index = ctlv_meta_test_find_child(tree, 0, CTLV_FIXTURE_ATTR_REGION_OUTPUT_FD);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].blob.direction,
			CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].blob.backings,
			BIT(CTLV_BLOB_TYPE_MEMFD) |
			BIT(CTLV_BLOB_TYPE_DMA_BUF));
	KUNIT_EXPECT_EQ(test, tree->node_schemas[index].blob.alignment, 4096);
}

/* One chain, oldest family first, to check the walk against. */
static const struct ctlv_family * const ctlv_meta_test_inherited_chain[] = {
	&ctlv_family,
	&ctlv_meta_test_fixture_family,
	&ctlv_meta_test_fixture_child_family,
};

static void ctlv_meta_test_inherited_ops(struct kunit *test)
{
	const struct ctlv_family *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;
	u32 level = 0, index = 0;
	u32 effective = 0;
	u32 i;

	/* A child repeats none of its parent's ops. */
	for (i = 0; i < ctlv_fixture_schema.ops_count; i++)
		KUNIT_EXPECT_NULL(test,
				  ctlv_meta_test_find_op(&ctlv_fixture_child_schema,
					  ctlv_fixture_op_schemas[i].op_id));

	/* The walk runs oldest ancestor down, the binding table's order. */
	ctlv_schema_cursor_init(&cursor, &ctlv_meta_test_fixture_child_family);
	while ((op = ctlv_schema_next_op(&cursor, &owner))) {
		const struct ctlv_family_schema *declared;

		KUNIT_ASSERT_LT(test, level, ARRAY_SIZE(ctlv_meta_test_inherited_chain));
		declared = ctlv_meta_test_inherited_chain[level]->schema;
		KUNIT_EXPECT_PTR_EQ(test, owner, ctlv_meta_test_inherited_chain[level]);
		KUNIT_EXPECT_EQ(test, op->op_id, declared->ops[index].op_id);
		effective++;
		if (++index == declared->ops_count) {
			index = 0;
			level++;
		}
	}
	KUNIT_EXPECT_EQ(test, effective,
			ctlv_family.schema->ops_count +
			ctlv_fixture_schema.ops_count +
			ctlv_fixture_child_schema.ops_count);
}

static void ctlv_meta_test_filled_stub(struct kunit *test)
{
	const struct ctlv_stub_fill_schema *fill_schema;
	const struct ctlv_schema_tree *reply;
	const struct ctlv_stub_schema *stub_schema;
	const struct ctlv_op_schema *op;
	int extension, tag, serial;

	op = ctlv_meta_test_find_op(&ctlv_fixture_schema, CTLV_FIXTURE_OP_NESTS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	reply = &op->reply;

	/* The family that declares a stub describes it as an empty nest. */
	extension = ctlv_meta_test_find_child(reply, 0, CTLV_FIXTURE_ATTR_NEST_EXTENSION);
	KUNIT_ASSERT_GE(test, extension, 0);
	KUNIT_EXPECT_EQ(test, reply->node_schemas[extension].kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, reply->node_schemas[extension].children_count, 0);

	/* The stub schema names which nest a fill would widen. */
	KUNIT_ASSERT_EQ(test, reply->stub_schemas_count, 1);
	stub_schema = &reply->stub_schemas[0];
	KUNIT_EXPECT_EQ(test, stub_schema->node, extension);
	KUNIT_EXPECT_EQ(test, stub_schema->family_id, CTLV_FAMILY_FIXTURE);
	KUNIT_EXPECT_EQ(test, stub_schema->set_id,
			CTLV_FIXTURE_SET_EXTENSION_POINT);

	/* Nothing fills it for a device of the family that declared it. */
	KUNIT_EXPECT_NULL(test, ctlv_schema_fill(&ctlv_meta_test_fixture_family,
						 stub_schema->family_id,
						 stub_schema->set_id));

	/* The members keep the IDs of the family that declared them. */
	fill_schema = ctlv_schema_fill(&ctlv_meta_test_fixture_child_family,
				       stub_schema->family_id,
				       stub_schema->set_id);
	KUNIT_ASSERT_NOT_NULL(test, fill_schema);
	KUNIT_EXPECT_EQ(test,
			fill_schema->members.node_schemas[0].children_count, 2);
	KUNIT_EXPECT_EQ(test,
			fill_schema->members.node_schemas[0].required_children_count,
			1);

	tag = ctlv_meta_test_find_child(&fill_schema->members, 0,
			 CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG);
	serial = ctlv_meta_test_find_child(&fill_schema->members, 0,
			    CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL);
	KUNIT_ASSERT_GE(test, tag, 0);
	KUNIT_ASSERT_GE(test, serial, 0);
	/* Required members come first, which is how presence is checked. */
	KUNIT_EXPECT_EQ(test, tag,
			fill_schema->members.node_schemas[0].first_child);
	KUNIT_EXPECT_EQ(test, serial, tag + 1);
}

/* Deeper than the fixture fill, but not its deepest path. */
static const struct ctlv_node_schema ctlv_meta_test_deep_members[] = {
	/* The root stands for the stub, so its children are the members. */
	{
		.attr_id = CTLV_ATTR_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
		.required_children_count = 1,
	},
	{
		.attr_id = CTLV_ATTR(CTLV_FAMILY_FIXTURE_CHILD, 1, 1),
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .max_len = 4096 },
	},
};

static const struct ctlv_stub_fill_schema ctlv_meta_test_deep_stub_fill_schemas[] = {
	{
		.family_id = CTLV_FAMILY_FIXTURE,
		.set_id = CTLV_FIXTURE_SET_EXTENSION_POINT,
		.members = {
			.node_schemas = ctlv_meta_test_deep_members,
			.node_schemas_count = ARRAY_SIZE(ctlv_meta_test_deep_members),
		},
	},
};

static const struct ctlv_family_schema ctlv_meta_test_deep_schema = {
	.name = "fixture-deep",
	.family_id = CTLV_FAMILY_FIXTURE_CHILD + 1,
	.stub_fill_schemas = ctlv_meta_test_deep_stub_fill_schemas,
	.stub_fill_schemas_count =
		ARRAY_SIZE(ctlv_meta_test_deep_stub_fill_schemas),
};

static const struct ctlv_family ctlv_meta_test_deep_family = {
	.schema = &ctlv_meta_test_deep_schema,
	.parent = &ctlv_meta_test_fixture_family,
};

static void ctlv_meta_test_filled_lengths(struct kunit *test)
{
	struct ctlv_op_sizes empty, filled, deep;
	const struct ctlv_op_schema *op;

	/* A fill changes only the lengths the core admits for this family. */
	op = ctlv_meta_test_find_op(&ctlv_fixture_schema, CTLV_FIXTURE_OP_NESTS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&ctlv_meta_test_fixture_family, op, &empty),
			0);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&ctlv_meta_test_fixture_child_family,
					     op, &filled),
			0);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&ctlv_meta_test_deep_family, op, &deep),
			0);
	KUNIT_EXPECT_EQ(test, empty.query_reply_min_len,
			op->sizes.query_reply_min_len);
	/*
	 * A fragment holds the deepest indivisible path, so a shallower fill
	 * asks for no more room and a deeper one asks for its own.
	 */
	KUNIT_EXPECT_EQ(test, filled.query_reply_min_len,
			empty.query_reply_min_len);
	KUNIT_EXPECT_GT(test, deep.query_reply_min_len,
			empty.query_reply_min_len);
	KUNIT_EXPECT_GE(test, deep.query_reply_suggested_len,
			deep.query_reply_min_len);

	/* An op with no extension point of its own is the same either way. */
	op = ctlv_meta_test_find_op(&ctlv_fixture_schema, CTLV_FIXTURE_OP_SCALARS_SET);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_ASSERT_EQ(test, ctlv_schema_op_sizes(&ctlv_meta_test_fixture_family, op,
						   &empty), 0);
	KUNIT_ASSERT_EQ(test, ctlv_schema_op_sizes(&ctlv_meta_test_deep_family, op,
						   &deep), 0);
	KUNIT_EXPECT_MEMEQ(test, &empty, &deep, sizeof(empty));
}

static struct kunit_case ctlv_meta_test_cases[] = {
	KUNIT_CASE_PARAM(ctlv_meta_test_op_ids, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_graph, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_op_messages, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_tree_shape, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_constraints, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_lengths, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_unlimited, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_meta_test_availability, metadata_gen_params),
	KUNIT_CASE(ctlv_meta_test_core_ops),
	KUNIT_CASE(ctlv_meta_test_core_families),
	KUNIT_CASE(ctlv_meta_test_dynamic_ops),
	KUNIT_CASE(ctlv_meta_test_uapi_constants),
	KUNIT_CASE(ctlv_meta_test_blob_policy),
	KUNIT_CASE(ctlv_meta_test_inherited_ops),
	KUNIT_CASE(ctlv_meta_test_filled_stub),
	KUNIT_CASE(ctlv_meta_test_filled_lengths),
	{},
};

static struct kunit_suite ctlv_meta_test_suite = {
	.name = "ctlv_metadata",
	.test_cases = ctlv_meta_test_cases,
};

kunit_test_suite(ctlv_meta_test_suite);
