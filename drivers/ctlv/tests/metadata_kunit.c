// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * KUnit checks over the generated CTLV schema metadata.
 *
 * Only rules the core relies on without re-checking them. The lengths are
 * composed here by the core's own arithmetic and compared with the
 * generator's, so a mistake in either does not agree with itself.
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
struct metadata_case {
	const char *name;
	const struct ctlv_family *family;
};

static const struct ctlv_family metadata_fixture_family = {
	.schema = &ctlv_fixture_ctlv_schema,
	.parent = &ctlv_ctlv_family,
};

static const struct ctlv_family metadata_fixture_child_family = {
	.schema = &ctlv_fixture_child_ctlv_schema,
	.parent = &metadata_fixture_family,
};

static const struct metadata_case metadata_cases[] = {
	{ "ctlv", &ctlv_ctlv_family },
	{ "ctlv-fixture", &metadata_fixture_family },
	{ "ctlv-fixture-child", &metadata_fixture_child_family },
};

KUNIT_ARRAY_PARAM_DESC(metadata, metadata_cases, name);

/* Index of the child of parent that carries attr_id, or -1. */
static int find_child(const struct ctlv_schema_tree *tree, u32 parent,
		      u64 attr_id)
{
	const struct ctlv_schema_node *node = &tree->nodes[parent];
	u32 i;

	for (i = 0; i < node->n_children; i++)
		if (tree->nodes[node->first_child + i].attr_id == attr_id)
			return node->first_child + i;
	return -1;
}

/* One op a family declares itself. */
static const struct ctlv_op_schema *
find_op(const struct ctlv_family_schema *schema, u64 op_id)
{
	u32 i;

	for (i = 0; i < schema->n_ops; i++)
		if (schema->ops[i].op_id == op_id)
			return &schema->ops[i];
	return NULL;
}

/* One op of a whole chain, wherever in it the op was declared. */
static const struct ctlv_op_schema *
find_effective_op(const struct ctlv_family *family, u64 op_id)
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
static u32 nest_depth(const struct ctlv_schema_tree *tree, u32 index)
{
	const struct ctlv_schema_node *node = &tree->nodes[index];
	u32 deepest = 0;
	u32 i;

	if (node->reenters)
		return 0;
	for (i = 0; i < node->n_children; i++) {
		u32 child = node->first_child + i;

		if (tree->nodes[child].kind != CTLV_SCHEMA_NEST)
			continue;
		deepest = max(deepest, 1 + nest_depth(tree, child));
	}
	return deepest;
}

static bool tree_has_unlimited(const struct ctlv_schema_tree *tree)
{
	u32 i;

	for (i = 0; i < tree->n_nodes; i++)
		if (!tree->nodes[i].max_occurrences)
			return true;
	return false;
}

static void ctlv_metadata_test_op_ids(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	KUNIT_ASSERT_GT(test, schema->n_ops, 0);
	for (i = 0; i < schema->n_ops; i++) {
		u64 op_id = schema->ops[i].op_id;
		u16 family = CTLV_OP_ID_FAMILY(op_id);
		u32 local = CTLV_OP_ID_NUMBER(op_id);

		/* Zero is reserved, and the middle bits of an op ID are too. */
		KUNIT_EXPECT_EQ(test, CTLV_OP_ID(family, local), op_id);
		/* Own ops only, numbered from one. */
		KUNIT_EXPECT_EQ(test, family, schema->family_id);
		KUNIT_EXPECT_EQ(test, local, i + 1);
	}
}

static void ctlv_metadata_test_graph(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family *family = entry->family;
	const struct ctlv_family_schema *schema = family->schema;
	const struct ctlv_family *ancestor;
	u32 levels = ctlv_family_levels(family);

	KUNIT_EXPECT_NOT_NULL(test, schema->name);
	/* Every chain is headed by the root, and the root inherits nothing. */
	if (schema->family_id == CTLV_FAMILY_ID) {
		KUNIT_EXPECT_NULL(test, family->parent);
		KUNIT_EXPECT_EQ(test, levels, 1);
		return;
	}
	KUNIT_ASSERT_GT(test, levels, 1);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_family_ancestor(family, levels - 1),
			    &ctlv_ctlv_family);
	KUNIT_EXPECT_TRUE(test, ctlv_family_is_a(family,
						 &ctlv_ctlv_family));
	KUNIT_EXPECT_TRUE(test, ctlv_family_is_a(family, family));

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

static void ctlv_metadata_test_op_messages(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->n_ops; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		const struct ctlv_op_sizes *sizes = &op->sizes;

		switch (op->type) {
		case CTLV_OP_TYPE_ACTION:
			/* A request root always exists, a reply may not. */
			KUNIT_ASSERT_NOT_NULL(test, op->request.nodes);
			KUNIT_EXPECT_EQ(test, !!op->reply.nodes,
					!!sizes->action_reply_max_len);
			KUNIT_EXPECT_EQ(test, sizes->query_reply_min_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->event_record_max_len, 0);
			break;
		case CTLV_OP_TYPE_QUERY:
			KUNIT_ASSERT_NOT_NULL(test, op->request.nodes);
			KUNIT_ASSERT_NOT_NULL(test, op->reply.nodes);
			KUNIT_EXPECT_GT(test, sizes->query_reply_min_len, 0);
			KUNIT_EXPECT_GE(test,
					sizes->query_reply_suggested_len,
					sizes->query_reply_min_len);
			KUNIT_EXPECT_EQ(test, sizes->action_reply_max_len, 0);
			KUNIT_EXPECT_EQ(test, sizes->event_record_max_len, 0);
			break;
		case CTLV_OP_TYPE_EVENT:
			/* An event is produced, never executed. */
			KUNIT_EXPECT_NULL(test, op->request.nodes);
			KUNIT_ASSERT_NOT_NULL(test, op->reply.nodes);
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

static void assert_tree(struct kunit *test,
			const struct ctlv_schema_tree *tree)
{
	const struct ctlv_schema_node *node;
	u8 *claims;
	u32 i, j;

	KUNIT_ASSERT_GT(test, tree->n_nodes, 0);
	claims = kunit_kzalloc(test, tree->n_nodes, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, claims);

	node = &tree->nodes[0];
	KUNIT_EXPECT_EQ(test, node->attr_id, CTLV_ATTR_ID_ROOT);
	KUNIT_EXPECT_EQ(test, node->kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, node->max_occurrences, 1);

	for (i = 0; i < tree->n_nodes; i++) {
		node = &tree->nodes[i];
		if (i)
			KUNIT_EXPECT_NE(test, node->attr_id,
					CTLV_ATTR_ID_ROOT);
		KUNIT_EXPECT_LE(test, node->n_required_children,
				node->n_children);
		if (!node->n_children) {
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
		KUNIT_ASSERT_LE(test, node->first_child + node->n_children,
				tree->n_nodes);
		if (node->reenters)
			continue;
		for (j = 0; j < node->n_children; j++)
			claims[node->first_child + j]++;
	}
	/* Every node but the root is the child of exactly one node. */
	KUNIT_EXPECT_EQ(test, claims[0], 0);
	for (i = 1; i < tree->n_nodes; i++)
		KUNIT_EXPECT_EQ(test, claims[i], 1);
	/* A message nests one level deeper than the tree describes. */
	KUNIT_EXPECT_LE(test, nest_depth(tree, 0),
			CTLV_MAX_DECLARED_NEST_DEPTH);

	/* An extension point widens a nest of this tree. */
	for (i = 0; i < tree->n_stubs; i++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[i];

		KUNIT_ASSERT_LT(test, ref->node, tree->n_nodes);
		KUNIT_EXPECT_EQ(test, tree->nodes[ref->node].kind,
				CTLV_SCHEMA_NEST);
		KUNIT_EXPECT_NE(test, ref->set_id, 0);
	}
	if (!tree->n_stubs)
		KUNIT_EXPECT_NULL(test, tree->stubs);
}

static void ctlv_metadata_test_tree_shape(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->n_ops; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		const struct ctlv_schema_tree *trees[] = {
			&op->request, &op->reply,
		};
		u32 j;

		for (j = 0; j < ARRAY_SIZE(trees); j++) {
			if (trees[j]->nodes)
				assert_tree(test, trees[j]);
			else
				KUNIT_EXPECT_EQ(test, trees[j]->n_nodes, 0);
		}
	}
	/* A fill is a tree of its own, whose root stands for the stub. */
	for (i = 0; i < schema->n_fills; i++) {
		const struct ctlv_stub_fill *fill = &schema->fills[i];

		assert_tree(test, &fill->members);
		KUNIT_EXPECT_GT(test, fill->members.nodes[0].n_children, 0);
		/* A family fills what an ancestor of it declared. */
		KUNIT_EXPECT_LT(test, fill->family_id, schema->family_id);
		KUNIT_EXPECT_NE(test, fill->set_id, 0);
	}
}

static void assert_constraints(struct kunit *test,
			       const struct ctlv_schema_node *node)
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

static void assert_tree_constraints(struct kunit *test,
				    const struct ctlv_schema_tree *tree)
{
	u32 node;

	for (node = 1; node < tree->n_nodes; node++)
		assert_constraints(test, &tree->nodes[node]);
}

static void ctlv_metadata_test_constraints(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->n_ops; i++) {
		assert_tree_constraints(test, &schema->ops[i].request);
		assert_tree_constraints(test, &schema->ops[i].reply);
	}
	for (i = 0; i < schema->n_fills; i++)
		assert_tree_constraints(test, &schema->fills[i].members);
}

static void ctlv_metadata_test_lengths(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	/*
	 * The generator's arithmetic against the core's, for an op with every
	 * extension point empty, which is what this family composes.
	 */
	for (i = 0; i < schema->n_ops; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];
		struct ctlv_op_sizes sizes;

		KUNIT_ASSERT_EQ(test,
				ctlv_schema_op_sizes(entry->family, op, &sizes),
				0);
		KUNIT_EXPECT_MEMEQ(test, &sizes, &op->sizes, sizeof(sizes));
	}
}

static void ctlv_metadata_test_unlimited(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	/* Only a query reply, which may fragment, may be unbounded. */
	for (i = 0; i < schema->n_ops; i++) {
		const struct ctlv_op_schema *op = &schema->ops[i];

		KUNIT_EXPECT_FALSE(test, tree_has_unlimited(&op->request));
		if (op->type != CTLV_OP_TYPE_QUERY && op->reply.nodes)
			KUNIT_EXPECT_FALSE(test,
					   tree_has_unlimited(&op->reply));
	}
}

static void ctlv_metadata_test_availability(struct kunit *test)
{
	const struct metadata_case *entry = test->param_value;
	const struct ctlv_family_schema *schema = entry->family->schema;
	u32 i;

	for (i = 0; i < schema->n_ops; i++) {
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
					CTLV_FAMILY_ID);
			break;
		default:
			KUNIT_FAIL(test, "op %#llx has availability %d",
				   op->op_id, op->availability);
			break;
		}
	}
}

static void ctlv_metadata_test_core_ops(struct kunit *test)
{
	const struct ctlv_schema_tree *dump, *changed;
	const struct ctlv_op_schema *op;
	int entry, id, disabled;

	/* A dump entry and a change event use the same attrs. */
	op = find_op(ctlv_ctlv_family.schema, CTLV_OP_OPS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	dump = &op->reply;
	op = find_op(ctlv_ctlv_family.schema, CTLV_OP_OP_CHANGED);
	KUNIT_ASSERT_NOT_NULL(test, op);
	changed = &op->reply;

	entry = find_child(dump, 0, CTLV_ATTR_OP_ENTRY);
	KUNIT_ASSERT_GE(test, entry, 0);
	id = find_child(dump, entry, CTLV_ATTR_OP_ID);
	disabled = find_child(dump, entry, CTLV_ATTR_OP_DISABLED);
	KUNIT_ASSERT_GE(test, id, 0);
	KUNIT_ASSERT_GE(test, disabled, 0);
	KUNIT_EXPECT_EQ(test, dump->nodes[disabled].kind, CTLV_SCHEMA_BOOL);

	KUNIT_EXPECT_GE(test, find_child(changed, 0, CTLV_ATTR_OP_ID), 0);
	KUNIT_EXPECT_GE(test, find_child(changed, 0,
					 CTLV_ATTR_OP_DISABLED), 0);
	KUNIT_EXPECT_GE(test, find_child(changed, 0,
					 CTLV_ATTR_OP_STATE_GENERATION),
			0);

	/* A dump entry also reports the immutable mode beside the state. */
	KUNIT_EXPECT_GE(test, find_child(dump, entry,
					 CTLV_ATTR_OP_FD_ACCESS), 0);
	KUNIT_EXPECT_GE(test, find_child(dump, entry,
					 CTLV_ATTR_OP_AVAILABILITY), 0);
	KUNIT_EXPECT_LT(test, find_child(changed, 0,
					 CTLV_ATTR_OP_FD_ACCESS), 0);

	for (id = 0; id < ctlv_ctlv_family.schema->n_ops; id++) {
		op = &ctlv_ctlv_family.schema->ops[id];
		if (op->type != CTLV_OP_TYPE_EVENT)
			KUNIT_EXPECT_EQ(test, op->fd_access,
					CTLV_OP_FD_ACCESS_READ);
	}
}

/* One record per family of the chain, so the entry repeats unbounded. */
static void ctlv_metadata_test_core_families(struct kunit *test)
{
	const struct ctlv_schema_tree *dump;
	const struct ctlv_op_schema *op;
	int entry, id, name;

	op = find_op(ctlv_ctlv_family.schema, CTLV_OP_FAMILIES_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_EXPECT_EQ(test, op->type, CTLV_OP_TYPE_QUERY);
	dump = &op->reply;

	entry = find_child(dump, 0, CTLV_ATTR_FAMILY_ENTRY);
	KUNIT_ASSERT_GE(test, entry, 0);
	KUNIT_EXPECT_EQ(test, dump->nodes[entry].kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, dump->nodes[entry].max_occurrences, 0);

	id = find_child(dump, entry, CTLV_ATTR_FAMILY_ID);
	name = find_child(dump, entry, CTLV_ATTR_FAMILY_NAME);
	KUNIT_ASSERT_GE(test, id, 0);
	KUNIT_ASSERT_GE(test, name, 0);
	KUNIT_EXPECT_EQ(test, dump->nodes[id].kind, CTLV_SCHEMA_UINT);
	/* The root heads every chain, so family zero is reported like any. */
	KUNIT_EXPECT_EQ(test, dump->nodes[id].range.min, 0);
	KUNIT_EXPECT_EQ(test, dump->nodes[name].kind, CTLV_SCHEMA_STRING);
	KUNIT_EXPECT_GT(test, dump->nodes[name].length.max_len, 0);
	/* The record has nothing else in it. */
	KUNIT_EXPECT_EQ(test, dump->nodes[entry].n_children, 2);
}

static void ctlv_metadata_test_dynamic_ops(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	u64 dynamic[] = {
		CTLV_FIXTURE_OP_SCALARS_SET, CTLV_FIXTURE_OP_COUNTS_GET,
		CTLV_FIXTURE_CHILD_OP_CHILD_DUMP,
	};
	u32 i;

	/* A descendant inherits availability and cannot disagree about it. */
	for (i = 0; i < ARRAY_SIZE(dynamic); i++) {
		op = find_effective_op(&metadata_fixture_child_family,
				       dynamic[i]);
		KUNIT_ASSERT_NOT_NULL(test, op);
		KUNIT_EXPECT_EQ(test, op->availability,
				CTLV_OP_AVAILABILITY_DYNAMIC);
	}
	op = find_effective_op(&metadata_fixture_child_family,
			       CTLV_FIXTURE_OP_NOOP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_EXPECT_EQ(test, op->availability, CTLV_OP_AVAILABILITY_FIXED);
}

static void ctlv_metadata_test_uapi_constants(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	const struct ctlv_schema_tree *tree;
	int index;

	/* The op constants of a header name the entries of its table. */
	KUNIT_EXPECT_EQ(test, ctlv_ctlv_family.schema->ops[0].op_id,
			CTLV_OP_FAMILIES_DUMP);
	KUNIT_EXPECT_EQ(test, ctlv_fixture_ctlv_op_schemas[0].op_id,
			CTLV_FIXTURE_OP_NOOP);

	op = find_op(&ctlv_fixture_ctlv_schema, CTLV_FIXTURE_OP_SCALARS_SET);
	KUNIT_ASSERT_NOT_NULL(test, op);
	tree = &op->request;

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_QUIRKS);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].kind, CTLV_SCHEMA_FLAGS);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].flags_mask,
			CTLV_FIXTURE_QUIRKS_MASK);

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_MODE);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].kind, CTLV_SCHEMA_ENUM);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].range.max, CTLV_FIXTURE_MODE_AUTO);

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_FREQ);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].unit,
			CTLV_FIXTURE_ATTR_SCALAR_FREQ_UNIT);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].scale,
			CTLV_FIXTURE_ATTR_SCALAR_FREQ_SCALE);

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_TEXT);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].kind, CTLV_SCHEMA_STRING);

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_SCALAR_FLAG);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].kind, CTLV_SCHEMA_BOOL);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].unit, CTLV_UNIT_NONE);
}

static void ctlv_metadata_test_blob_policy(struct kunit *test)
{
	const struct ctlv_op_schema *op;
	const struct ctlv_schema_tree *tree;
	int index;

	op = find_op(&ctlv_fixture_ctlv_schema, CTLV_FIXTURE_OP_BLOBS_SUBMIT);
	KUNIT_ASSERT_NOT_NULL(test, op);
	tree = &op->request;

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_REGION_INPUT_VA);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].blob.direction,
			CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].blob.backings,
			BIT(CTLV_BLOB_TYPE_VA));
	/* A blob attr carries a descriptor, so it has no schema children. */
	KUNIT_EXPECT_EQ(test, tree->nodes[index].n_children, 0);

	index = find_child(tree, 0, CTLV_FIXTURE_ATTR_REGION_OUTPUT_FD);
	KUNIT_ASSERT_GE(test, index, 0);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].blob.direction,
			CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_EXPECT_EQ(test, tree->nodes[index].blob.backings,
			BIT(CTLV_BLOB_TYPE_MEMFD) |
			BIT(CTLV_BLOB_TYPE_DMA_BUF));
	KUNIT_EXPECT_EQ(test, tree->nodes[index].blob.alignment, 4096);
}

/* One chain, oldest family first, to check the walk against. */
static const struct ctlv_family * const inherited_chain[] = {
	&ctlv_ctlv_family,
	&metadata_fixture_family,
	&metadata_fixture_child_family,
};

static void ctlv_metadata_test_inherited_ops(struct kunit *test)
{
	const struct ctlv_family *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;
	u32 level = 0, index = 0;
	u32 effective = 0;
	u32 i;

	/* A child repeats none of its parent's ops. */
	for (i = 0; i < CTLV_FIXTURE_N_OPS; i++)
		KUNIT_EXPECT_NULL(test,
				  find_op(&ctlv_fixture_child_ctlv_schema,
					  ctlv_fixture_ctlv_op_schemas[i].op_id));

	/* The walk runs oldest ancestor down, the binding table's order. */
	ctlv_schema_cursor_init(&cursor, &metadata_fixture_child_family);
	while ((op = ctlv_schema_next_op(&cursor, &owner))) {
		const struct ctlv_family_schema *declared;

		KUNIT_ASSERT_LT(test, level, ARRAY_SIZE(inherited_chain));
		declared = inherited_chain[level]->schema;
		KUNIT_EXPECT_PTR_EQ(test, owner, inherited_chain[level]);
		KUNIT_EXPECT_EQ(test, op->op_id, declared->ops[index].op_id);
		effective++;
		if (++index == declared->n_ops) {
			index = 0;
			level++;
		}
	}
	KUNIT_EXPECT_EQ(test, effective, CTLV_N_OPS + CTLV_FIXTURE_N_OPS +
			CTLV_FIXTURE_CHILD_N_OPS);
}

static void ctlv_metadata_test_filled_stub(struct kunit *test)
{
	const struct ctlv_stub_fill *fill;
	const struct ctlv_schema_tree *reply;
	const struct ctlv_op_schema *op;
	const struct ctlv_stub_ref *ref;
	int extension, tag, serial;

	op = find_op(&ctlv_fixture_ctlv_schema, CTLV_FIXTURE_OP_NESTS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	reply = &op->reply;

	/* The family that declares a stub describes it as an empty nest. */
	extension = find_child(reply, 0, CTLV_FIXTURE_ATTR_NEST_EXTENSION);
	KUNIT_ASSERT_GE(test, extension, 0);
	KUNIT_EXPECT_EQ(test, reply->nodes[extension].kind, CTLV_SCHEMA_NEST);
	KUNIT_EXPECT_EQ(test, reply->nodes[extension].n_children, 0);

	/* The stub ref names which nest a fill would widen. */
	KUNIT_ASSERT_EQ(test, reply->n_stubs, 1);
	ref = &reply->stubs[0];
	KUNIT_EXPECT_EQ(test, ref->node, extension);
	KUNIT_EXPECT_EQ(test, ref->family_id, CTLV_FIXTURE_FAMILY_ID);
	KUNIT_EXPECT_EQ(test, ref->set_id, CTLV_FIXTURE_SET_EXTENSION_POINT);

	/* Nothing fills it for a device of the family that declared it. */
	KUNIT_EXPECT_NULL(test, ctlv_schema_fill(&metadata_fixture_family,
						 ref->family_id, ref->set_id));

	/* The members keep the IDs of the family that declared them. */
	fill = ctlv_schema_fill(&metadata_fixture_child_family, ref->family_id,
				ref->set_id);
	KUNIT_ASSERT_NOT_NULL(test, fill);
	KUNIT_EXPECT_EQ(test, fill->members.nodes[0].n_children, 2);
	KUNIT_EXPECT_EQ(test, fill->members.nodes[0].n_required_children, 1);

	tag = find_child(&fill->members, 0, CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG);
	serial = find_child(&fill->members, 0, CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL);
	KUNIT_ASSERT_GE(test, tag, 0);
	KUNIT_ASSERT_GE(test, serial, 0);
	/* Required members come first, which is how presence is checked. */
	KUNIT_EXPECT_EQ(test, tag, fill->members.nodes[0].first_child);
	KUNIT_EXPECT_EQ(test, serial, tag + 1);
}

/*
 * A hand-built fill deeper than anything the fixture describes: the generated
 * one is shallower than the deepest path already there, so it changes nothing.
 */
static const struct ctlv_schema_node metadata_deep_members[] = {
	/* The root stands for the stub, so its children are the members. */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	{
		.attr_id = CTLV_ATTR_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, 1, 1),
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .max_len = 4096 },
	},
};

static const struct ctlv_stub_fill metadata_deep_fills[] = {
	{
		.family_id = CTLV_FIXTURE_FAMILY_ID,
		.set_id = CTLV_FIXTURE_SET_EXTENSION_POINT,
		.members = CTLV_SCHEMA_TREE(metadata_deep_members),
	},
};

static const struct ctlv_family_schema metadata_deep_schema = {
	.name = "fixture-deep",
	.family_id = CTLV_FIXTURE_CHILD_FAMILY_ID + 1,
	.fills = metadata_deep_fills,
	.n_fills = ARRAY_SIZE(metadata_deep_fills),
};

static const struct ctlv_family metadata_deep_family = {
	.schema = &metadata_deep_schema,
	.parent = &metadata_fixture_family,
};

static void ctlv_metadata_test_filled_lengths(struct kunit *test)
{
	struct ctlv_op_sizes empty, filled, deep;
	const struct ctlv_op_schema *op;

	/* A fill changes only the lengths the core admits for this family. */
	op = find_op(&ctlv_fixture_ctlv_schema, CTLV_FIXTURE_OP_NESTS_DUMP);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&metadata_fixture_family, op, &empty),
			0);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&metadata_fixture_child_family,
					     op, &filled),
			0);
	KUNIT_ASSERT_EQ(test,
			ctlv_schema_op_sizes(&metadata_deep_family, op, &deep),
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
	op = find_op(&ctlv_fixture_ctlv_schema, CTLV_FIXTURE_OP_SCALARS_SET);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_ASSERT_EQ(test, ctlv_schema_op_sizes(&metadata_fixture_family, op,
						   &empty), 0);
	KUNIT_ASSERT_EQ(test, ctlv_schema_op_sizes(&metadata_deep_family, op,
						   &deep), 0);
	KUNIT_EXPECT_MEMEQ(test, &empty, &deep, sizeof(empty));
}

static struct kunit_case ctlv_metadata_test_cases[] = {
	KUNIT_CASE_PARAM(ctlv_metadata_test_op_ids, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_graph, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_op_messages, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_tree_shape, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_constraints, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_lengths, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_unlimited, metadata_gen_params),
	KUNIT_CASE_PARAM(ctlv_metadata_test_availability, metadata_gen_params),
	KUNIT_CASE(ctlv_metadata_test_core_ops),
	KUNIT_CASE(ctlv_metadata_test_core_families),
	KUNIT_CASE(ctlv_metadata_test_dynamic_ops),
	KUNIT_CASE(ctlv_metadata_test_uapi_constants),
	KUNIT_CASE(ctlv_metadata_test_blob_policy),
	KUNIT_CASE(ctlv_metadata_test_inherited_ops),
	KUNIT_CASE(ctlv_metadata_test_filled_stub),
	KUNIT_CASE(ctlv_metadata_test_filled_lengths),
	{},
};

static struct kunit_suite ctlv_metadata_test_suite = {
	.name = "ctlv_metadata",
	.test_cases = ctlv_metadata_test_cases,
};

kunit_test_suite(ctlv_metadata_test_suite);
