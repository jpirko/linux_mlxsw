// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Family-descriptor validation and lifecycle tests.
 */

#include <kunit/static_stub.h>
#include <kunit/device.h>
#include <kunit/test.h>
#include <linux/align.h>
#include <linux/array_size.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/completion.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/minmax.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <ctlv/ctlv_fixture_child_gen.h>
#include <ctlv/ctlv_fixture_gen.h>
#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "../event.h"
#include "../internal.h"
#include "ctlv_fixture_child_gen.h"
#include "ctlv_fixture_gen.h"

/* Room for the extension points the fixture chain fills, and one to spare. */
#define CTLV_REG_TEST_MAX_EXT_OPS 4

/* A callback has to exist to be present; nothing here ever calls one. */
static int ctlv_reg_test_callback(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request,
				  struct ctlv_cursor *reply)
{
	return 0;
}

static int ctlv_reg_test_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
	return 0;
}

static void ctlv_reg_test_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
}

static const struct ctlv_hooks ctlv_reg_test_hooks = {
	.pre = ctlv_reg_test_pre,
	.post = ctlv_reg_test_post,
};

static int ctlv_reg_test_fill(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *record)
{
	return 0;
}

static void ctlv_reg_test_read(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *record)
{
}

/* Mutable descriptors and bindings for one copied family chain. */
struct ctlv_reg_test_table {
	struct ctlv_family family;
	struct ctlv_family parent_family;
	/* A descriptor of the family's own schema, for a case that misnames one. */
	struct ctlv_family stranger;
	struct ctlv_family_schema schema;
	struct ctlv_family_schema parent;
	struct ctlv_op_schema *ops;
	struct ctlv_op_schema *parent_ops;
	/* One spare entry each, for the case that binds one op too many. */
	struct ctlv_op *bindings;
	struct ctlv_op *parent_bindings;
	struct ctlv_ext_ops ext_ops[CTLV_REG_TEST_MAX_EXT_OPS];
};

/* Build local bindings independently of the core walk. */
static u32 ctlv_reg_test_bind(struct ctlv_op *bindings,
			     const struct ctlv_op_schema *ops, u32 ops_count)
{
	u32 index, bound = 0;

	for (index = 0; index < ops_count; index++) {
		const struct ctlv_op_schema *op = &ops[index];
		struct ctlv_op *binding;

		if (op->type == CTLV_OP_TYPE_EVENT)
			continue;
		binding = &bindings[bound++];
		binding->action = ctlv_reg_test_callback;
		if (op->pre_post) {
			binding->pre = ctlv_reg_test_pre;
			binding->post = ctlv_reg_test_post;
		}
	}
	return bound;
}

/* What a family answers with in each extension point its schema fills. */
static u32 ctlv_reg_test_ext_ops(struct kunit *test,
				 struct ctlv_reg_test_table *table,
				 const struct ctlv_family_schema *schema)
{
	u32 index;

	KUNIT_ASSERT_LE(test, schema->ext_fill_schemas_count,
			CTLV_REG_TEST_MAX_EXT_OPS);
	for (index = 0; index < schema->ext_fill_schemas_count; index++)
		table->ext_ops[index] = (struct ctlv_ext_ops){
			.family_id =
				schema->ext_fill_schemas[index].family_id,
			.ext_id = schema->ext_fill_schemas[index].ext_id,
			.fill = ctlv_reg_test_fill,
			.read = ctlv_reg_test_read,
		};
	return schema->ext_fill_schemas_count;
}

static void ctlv_reg_test_build(struct kunit *test,
			       struct ctlv_reg_test_table *table)
{
	memset(table, 0, sizeof(*table));
	table->parent = ctlv_fixture_schema;
	table->schema = ctlv_fixture_child_schema;

	table->ops =
		kunit_kcalloc(test, table->schema.ops_count,
			      sizeof(*table->ops), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table->ops);
	memcpy(table->ops, ctlv_fixture_child_op_schemas,
	       array_size(table->schema.ops_count, sizeof(*table->ops)));
	table->parent_ops =
		kunit_kcalloc(test, table->parent.ops_count,
			      sizeof(*table->parent_ops), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table->parent_ops);
	memcpy(table->parent_ops, ctlv_fixture_op_schemas,
	       array_size(table->parent.ops_count, sizeof(*table->parent_ops)));
	table->bindings =
		kunit_kcalloc(test, table->schema.ops_count + 1,
			      sizeof(*table->bindings), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table->bindings);
	table->parent_bindings =
		kunit_kcalloc(test, table->parent.ops_count + 1,
			      sizeof(*table->parent_bindings), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table->parent_bindings);

	table->parent.ops = table->parent_ops;
	table->schema.ops = table->ops;

	table->parent_family = (struct ctlv_family){
		.schema = &table->parent,
		.parent = &ctlv_family,
		.ops = table->parent_bindings,
		.ops_count = ctlv_reg_test_bind(table->parent_bindings,
					   table->parent_ops,
					   table->parent.ops_count),
	};
	table->family = (struct ctlv_family){
		.schema = &table->schema,
		.parent = &table->parent_family,
		.ops = table->bindings,
		.ops_count = ctlv_reg_test_bind(table->bindings, table->ops,
					   table->schema.ops_count),
		.ext_ops = table->ext_ops,
		.ext_ops_count =
			ctlv_reg_test_ext_ops(test, table, &table->schema),
	};
	table->stranger = table->family;
	table->stranger.parent = NULL;
}

static struct ctlv_op_schema *
ctlv_reg_test_find(struct kunit *test, struct ctlv_reg_test_table *table,
		  enum _ctlv_op_availability availability,
		  enum _ctlv_op_type type)
{
	u32 index;

	for (index = 0; index < table->schema.ops_count; index++)
		if (table->ops[index].availability == availability &&
		    table->ops[index].type == type)
			return &table->ops[index];
	for (index = 0; index < table->parent.ops_count; index++)
		if (table->parent_ops[index].availability == availability &&
		    table->parent_ops[index].type == type)
			return &table->parent_ops[index];
	KUNIT_FAIL_AND_ABORT(test, "the fixture has no such op to break");
	return NULL;
}

/* What answers one op: the binding of the family that declared it. */
static struct ctlv_op *
ctlv_reg_test_binding(struct kunit *test, struct ctlv_reg_test_table *table,
		     const struct ctlv_op_schema *op)
{
	const struct ctlv_op_schema *ops = table->ops;
	struct ctlv_op *bindings = table->bindings;
	u32 index, ops_count = table->schema.ops_count, bound = 0;

	if (op < ops || op >= &ops[ops_count]) {
		ops = table->parent_ops;
		bindings = table->parent_bindings;
		ops_count = table->parent.ops_count;
	}
	for (index = 0; index < ops_count; index++) {
		if (ops[index].type == CTLV_OP_TYPE_EVENT)
			continue;
		if (&ops[index] == op)
			return &bindings[bound];
		bound++;
	}
	KUNIT_FAIL_AND_ABORT(test, "that op has no binding");
	return NULL;
}

/*
 * Named rather than kfree() cast to an action's signature, kfree() taking a
 * const pointer where an action takes a mutable one.
 */
static void ctlv_reg_test_free_ops(void *ops)
{
	kfree(ops);
}

static void ctlv_reg_test_free_bitmap(void *bitmap)
{
	bitmap_free(bitmap);
}

static struct ctlv_op_entry *
ctlv_reg_test_find_locked(struct ctlv *ctlv, u64 op_id)
{
	u32 index;

	if (IS_ENABLED(CONFIG_SMP) &&
	    spin_trylock(&ctlv->transition_lock)) {
		spin_unlock(&ctlv->transition_lock);
		kunit_fail_current_test("op lookup ran outside transition lock");
	}
	for (index = 0; index < ctlv->ops_count; index++)
		if (ctlv->ops[index].schema->op_id == op_id)
			return &ctlv->ops[index];
	return NULL;
}

static void ctlv_reg_test_break_nothing(struct kunit *test, struct ctlv_reg_test_table *table)
{
}

/* A family directly below the core still names the core descriptor. */
static void ctlv_reg_test_break_to_root(struct kunit *test,
			  struct ctlv_reg_test_table *table)
{
	table->family = table->parent_family;
}

static void ctlv_reg_test_break_schema(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->family.schema = NULL;
}

static void ctlv_reg_test_break_name(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->schema.name = "";
}

static void ctlv_reg_test_break_bindings(struct kunit *test,
			   struct ctlv_reg_test_table *table)
{
	table->family.ops = NULL;
}

static void ctlv_reg_test_break_emptiness(struct kunit *test,
			    struct ctlv_reg_test_table *table)
{
	table->family.ops_count = 0;
}

/* The descriptor of the family this one inherits, absent and mistaken. */
static void ctlv_reg_test_break_no_parent(struct kunit *test,
			    struct ctlv_reg_test_table *table)
{
	table->family.parent = NULL;
}

static void ctlv_reg_test_break_other_parent(struct kunit *test,
			       struct ctlv_reg_test_table *table)
{
	table->family.parent = &table->stranger;
}

/* A direct child's parent must be the core descriptor, not itself. */
static void ctlv_reg_test_break_root_parent(struct kunit *test,
			      struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_break_to_root(test, table);
	table->family.parent = &table->parent_family;
}

/* An extension point is answered by the family whose schema fills it. */
static void ctlv_reg_test_break_no_ext_ops(struct kunit *test,
					   struct ctlv_reg_test_table *table)
{
	KUNIT_ASSERT_GT(test, table->family.ext_ops_count, 0);
	table->family.ext_ops_count = 0;
}

static void ctlv_reg_test_break_other_ext_ops(struct kunit *test,
					      struct ctlv_reg_test_table *table)
{
	KUNIT_ASSERT_GT(test, table->family.ext_ops_count, 0);
	table->ext_ops[0].ext_id++;
}

static struct ctlv_op *
ctlv_reg_test_action(struct kunit *test, struct ctlv_reg_test_table *table,
		    enum _ctlv_op_availability availability)
{
	return ctlv_reg_test_binding(test, table,
				    ctlv_reg_test_find(test, table,
						      availability,
						      CTLV_OP_TYPE_ACTION));
}

static void ctlv_reg_test_break_callback(struct kunit *test,
			   struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->action = NULL;
}

/* A disabled op still needs its callback: a device may enable it again. */
static void ctlv_reg_test_break_dynamic_callback(struct kunit *test,
				   struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_action(test, table,
			    CTLV_OP_AVAILABILITY_DYNAMIC)->action = NULL;
}

/* The binding of the op that asked for a pair of its own. */
static struct ctlv_op *
ctlv_reg_test_marked(struct kunit *test, struct ctlv_reg_test_table *table)
{
	u32 index;

	for (index = 0; index < table->schema.ops_count; index++)
		if (table->ops[index].pre_post)
			return ctlv_reg_test_binding(test, table,
						    &table->ops[index]);
	for (index = 0; index < table->parent.ops_count; index++)
		if (table->parent_ops[index].pre_post)
			return ctlv_reg_test_binding(test, table,
						    &table->parent_ops[index]);
	KUNIT_FAIL_AND_ABORT(test, "the fixture has no marked op to break");
	return NULL;
}

/* A descriptor is the only place a hook pair can contradict the schema. */
static void ctlv_reg_test_break_unasked_hooks(struct kunit *test,
				struct ctlv_reg_test_table *table)
{
	struct ctlv_op *binding =
		ctlv_reg_test_action(test, table, CTLV_OP_AVAILABILITY_FIXED);

	binding->pre = ctlv_reg_test_pre;
	binding->post = ctlv_reg_test_post;
}

static void ctlv_reg_test_break_missing_hooks(struct kunit *test,
				struct ctlv_reg_test_table *table)
{
	struct ctlv_op *binding = ctlv_reg_test_marked(test, table);

	binding->pre = NULL;
	binding->post = NULL;
}

/* Either half alone would leave the core unwinding what it never entered. */
static void ctlv_reg_test_break_op_pre(struct kunit *test, struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->pre = ctlv_reg_test_pre;
}

static void ctlv_reg_test_break_op_post(struct kunit *test, struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->post =
		ctlv_reg_test_post;
}

/* One op of the chain with nothing bound to it, and one binding too many. */
static void ctlv_reg_test_break_too_few_bindings(struct kunit *test,
				   struct ctlv_reg_test_table *table)
{
	table->family.ops_count--;
}

static void ctlv_reg_test_break_too_many_bindings(struct kunit *test,
				    struct ctlv_reg_test_table *table)
{
	table->bindings[table->family.ops_count].action = ctlv_reg_test_callback;
	table->family.ops_count++;
}

static void ctlv_reg_test_break_order(struct kunit *test, struct ctlv_reg_test_table *table)
{
	swap(table->parent_ops[0], table->parent_ops[1]);
}

static void ctlv_reg_test_break_uniqueness(struct kunit *test,
			     struct ctlv_reg_test_table *table)
{
	table->parent_ops[1].op_id = table->parent_ops[0].op_id;
}

/* An op of a family that is not the one whose array it sits in. */
static void ctlv_reg_test_break_owner(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->ops[0].op_id = CTLV_OP(CTLV_FAMILY_FIXTURE,
				      table->parent.ops_count + 1);
}

static void ctlv_reg_test_break_number(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->ops[0].op_id = CTLV_OP(CTLV_FAMILY_FIXTURE_CHILD, 0);
}

static void ctlv_reg_test_break_op_type(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->ops[0].type = CTLV_OP_TYPE_EVENT + 1;
}

static void ctlv_reg_test_break_availability(struct kunit *test,
			       struct ctlv_reg_test_table *table)
{
	table->ops[0].availability = CTLV_OP_AVAILABILITY_DYNAMIC + 1;
}

static void ctlv_reg_test_break_fd_access(struct kunit *test,
			    struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_ACTION)->fd_access =
		CTLV_OP_FD_ACCESS_READ_WRITE + 1;
}

static void ctlv_reg_test_break_event_fd_access(struct kunit *test,
				  struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->fd_access =
		CTLV_OP_FD_ACCESS_READ;
}

/* An event is produced by the family, so no device can take it away. */
static void ctlv_reg_test_break_dynamic_event(struct kunit *test,
				struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->availability =
		CTLV_OP_AVAILABILITY_DYNAMIC;
}

/* Nor opt in to one, for the same reason. */
static void ctlv_reg_test_break_optional_event(struct kunit *test,
				 struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->optional = true;
}

/* A message no buffer of this core could hold. */
static const struct ctlv_node_schema ctlv_reg_test_huge[] = {
	{
		.type = CTLV_TYPE_ROOT,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.first_child = 1,
		.children_count = 1,
	},
	{
		.type = CTLV_TYPE(CTLV_FAMILY_FIXTURE_CHILD, 1, 1),
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .max_len = CTLV_MAX_INLINE_MESSAGE_LEN },
	},
};

static void ctlv_reg_test_break_length(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->ops[0].request.node_schemas = ctlv_reg_test_huge;
	table->ops[0].request.node_schemas_count = ARRAY_SIZE(ctlv_reg_test_huge);
}

/* A graph whose chain is not one of the core's. */
static void ctlv_reg_test_break_chain(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->parent_family.parent = NULL;
}

static void ctlv_reg_test_break_core_id(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->schema.family_id = CTLV_FAMILY_ROOT;
}

/* A child is named its parent's whole name and a suffix of its own. */
static void ctlv_reg_test_break_naming(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->schema.name = "unrelated";
}

static void ctlv_reg_test_break_suffix(struct kunit *test, struct ctlv_reg_test_table *table)
{
	table->schema.name = "ctlv-fixturechild";
}

static void ctlv_reg_test_break_parent_name(struct kunit *test,
			      struct ctlv_reg_test_table *table)
{
	table->parent.name = NULL;
}

struct ctlv_reg_test_case {
	const char *name;
	void (*damage)(struct kunit *test, struct ctlv_reg_test_table *table);
	int expected;
};

static const struct ctlv_reg_test_case ctlv_reg_scenarios[] = {
	{ "a generated descriptor", ctlv_reg_test_break_nothing, 0 },
	{ "a family directly below the core", ctlv_reg_test_break_to_root, 0 },
	{ "no schema", ctlv_reg_test_break_schema, -EINVAL },
	{ "no name", ctlv_reg_test_break_name, -EINVAL },
	{ "no bindings", ctlv_reg_test_break_bindings, -EINVAL },
	{ "no binding at all", ctlv_reg_test_break_emptiness, -EINVAL },
	{ "no descriptor of the family it inherits", ctlv_reg_test_break_no_parent, -EINVAL },
	{ "the descriptor of another family", ctlv_reg_test_break_other_parent, -EINVAL },
	{ "itself as its parent descriptor", ctlv_reg_test_break_root_parent, -EINVAL },
	{ "no answer to an extension point it fills", ctlv_reg_test_break_no_ext_ops,
	  -EINVAL },
	{ "an answer to an extension point it does not fill",
	  ctlv_reg_test_break_other_ext_ops,
	  -EINVAL },
	{ "no callback", ctlv_reg_test_break_callback, -EINVAL },
	{ "no callback on a disabled op", ctlv_reg_test_break_dynamic_callback, -EINVAL },
	{ "an op pre hook with no post", ctlv_reg_test_break_op_pre, -EINVAL },
	{ "an op post hook with no pre", ctlv_reg_test_break_op_post, -EINVAL },
	{ "a hook pair the op did not ask for", ctlv_reg_test_break_unasked_hooks, -EINVAL },
	{ "no hook pair where the op asked for one", ctlv_reg_test_break_missing_hooks,
	  -EINVAL },
	{ "one binding too few", ctlv_reg_test_break_too_few_bindings, -EINVAL },
	{ "one binding too many", ctlv_reg_test_break_too_many_bindings, -EINVAL },
	{ "ops out of order", ctlv_reg_test_break_order, -EINVAL },
	{ "two ops with one ID", ctlv_reg_test_break_uniqueness, -EINVAL },
	{ "an op of another family", ctlv_reg_test_break_owner, -EINVAL },
	{ "an op numbered zero", ctlv_reg_test_break_number, -EINVAL },
	{ "a type of its own invention", ctlv_reg_test_break_op_type, -EINVAL },
	{ "an fd access of its own invention", ctlv_reg_test_break_fd_access, -EINVAL },
	{ "an event with fd access", ctlv_reg_test_break_event_fd_access, -EINVAL },
	{ "an availability of its own invention", ctlv_reg_test_break_availability, -EINVAL },
	{ "an event a device could disable", ctlv_reg_test_break_dynamic_event, -EINVAL },
	{ "an event a device could omit", ctlv_reg_test_break_optional_event, -EINVAL },
	{ "a message this core cannot carry", ctlv_reg_test_break_length, -EINVAL },
	{ "a chain that is not the core's", ctlv_reg_test_break_chain, -EINVAL },
	{ "the core family ID", ctlv_reg_test_break_core_id, -EINVAL },
	{ "a name of its own", ctlv_reg_test_break_naming, -EINVAL },
	{ "a name with no separator", ctlv_reg_test_break_suffix, -EINVAL },
	{ "an ancestor with no name", ctlv_reg_test_break_parent_name, -EINVAL },
};

KUNIT_ARRAY_PARAM_DESC(registration, ctlv_reg_scenarios, name);

static void ctlv_reg_test_check(struct kunit *test)
{
	const struct ctlv_reg_test_case *entry = test->param_value;
	struct ctlv_reg_test_table *table;
	struct ctlv ctlv = {};

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);

	ctlv_reg_test_build(test, table);
	entry->damage(test, table);
	KUNIT_EXPECT_EQ(test, ctlv_op_resolution_init(&ctlv, &table->family),
			entry->expected);
	kfree(ctlv.ops);
}

/*
 * A chain of names long enough to crowd the uevent budget, which no chain of
 * this tree comes near, is refused before a device exists to announce it.
 */
static void ctlv_reg_test_uevent(struct kunit *test)
{
	static const char name[] =
		"fixture-with-a-name-as-long-as-a-family-may-ever-report-of-self";
	struct ctlv_reg_test_table *table;
	struct ctlv_family_schema *schemas;
	struct ctlv_family *chain;
	u32 levels, level;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv_reg_test_build(test, table);
	KUNIT_ASSERT_EQ(test, ctlv_uevent_check(&table->family), 0);

	levels = CTLV_UEVENT_MAX_LEN / sizeof(name) + 1;
	chain = kunit_kcalloc(test, levels, sizeof(*chain), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, chain);
	schemas = kunit_kcalloc(test, levels, sizeof(*schemas), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, schemas);
	for (level = 0; level < levels; level++) {
		schemas[level].name = name;
		schemas[level].family_id = level + 1;
		chain[level].schema = &schemas[level];
		chain[level].parent = level ? &chain[level - 1] :
					     &ctlv_family;
	}
	KUNIT_EXPECT_EQ(test, ctlv_uevent_check(&chain[levels - 1]), -E2BIG);
}

/* Verify the index assignments across the root-first walk. */
static void ctlv_reg_test_resolved(struct kunit *test)
{
	struct ctlv_reg_test_table *table;
	struct ctlv ctlv = {};
	u32 dynamic = 0, events = 0;
	u32 index;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv_reg_test_build(test, table);
	KUNIT_ASSERT_EQ(test, ctlv_op_resolution_init(&ctlv, &table->family), 0);
	kunit_add_action(test, ctlv_reg_test_free_ops, ctlv.ops);

	KUNIT_ASSERT_EQ(test, ctlv.ops_count,
			ctlv_family.schema->ops_count +
			table->parent.ops_count + table->schema.ops_count);
	for (index = 0; index < ctlv.ops_count; index++) {
		const struct ctlv_op_entry *entry = &ctlv.ops[index];

		if (index)
			KUNIT_EXPECT_GT(test, entry->schema->op_id,
					ctlv.ops[index - 1].schema->op_id);
		/* The core answers root ops, each family its own. */
		if (entry->schema->type == CTLV_OP_TYPE_EVENT) {
			KUNIT_EXPECT_NULL(test, entry->binding);
			KUNIT_EXPECT_EQ(test, entry->event_index, events++);
		} else {
			KUNIT_EXPECT_NOT_NULL(test, entry->binding);
		}
		/* A filled extension point only adds to the declared sizes. */
		KUNIT_EXPECT_GE(test, entry->sizes.request_max_len,
				entry->schema->sizes.request_max_len);
		KUNIT_EXPECT_GE(test, entry->sizes.query_reply_min_len,
				entry->schema->sizes.query_reply_min_len);
		KUNIT_EXPECT_GE(test, entry->sizes.action_reply_max_len,
				entry->schema->sizes.action_reply_max_len);
		if (entry->schema->availability == CTLV_OP_AVAILABILITY_DYNAMIC)
			KUNIT_EXPECT_EQ(test, entry->state_index, dynamic++);
		else
			KUNIT_EXPECT_EQ(test, entry->state_index,
					CTLV_OP_STATE_NONE);
	}
	KUNIT_EXPECT_EQ(test, ctlv.dynamic_ops_count, dynamic);
	KUNIT_EXPECT_GT(test, dynamic, 0);
	KUNIT_EXPECT_EQ(test, ctlv.events_count, events);
	/* The root's three, which every chain of this core inherits. */
	KUNIT_EXPECT_GE(test, events, 3);
}

/* Publication drops unprovided optional ops and preserves order. */
static void ctlv_reg_test_published(struct kunit *test)
{
	struct ctlv_reg_test_table *table;
	struct ctlv ctlv = {};
	u32 resolved, absent, index;
	struct ctlv_op_schema *taken;
	bool kept = false;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv_reg_test_build(test, table);

	/* An op no device could ever disable, made one it need not have. */
	taken = ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
				  CTLV_OP_TYPE_ACTION);
	/* Resolution reads this, so it has to be set before resolving. */
	taken->optional = true;

	spin_lock_init(&ctlv.transition_lock);
	ctlv.family = &table->family;
	KUNIT_ASSERT_EQ(test, ctlv_op_resolution_init(&ctlv, &table->family), 0);
	kunit_add_action(test, ctlv_reg_test_free_ops, ctlv.ops);
	resolved = ctlv.ops_count;

	/* This one and however many the fixture declares optional itself. */
	absent = 0;
	for (index = 0; index < resolved; index++)
		if (ctlv.ops[index].absent)
			absent++;
	KUNIT_ASSERT_GT(test, absent, 0);

	ctlv_op_provide(&ctlv, taken->op_id);
	ctlv_op_provide(&ctlv, taken->op_id);

	ctlv_op_publish_all(&ctlv);
	KUNIT_EXPECT_TRUE(test, ctlv.published);
	KUNIT_EXPECT_EQ(test, ctlv.ops_count, resolved - (absent - 1));
	for (index = 0; index < ctlv.ops_count; index++) {
		const struct ctlv_op_entry *entry = &ctlv.ops[index];

		if (index)
			KUNIT_EXPECT_GT(test, entry->schema->op_id,
					ctlv.ops[index - 1].schema->op_id);
		KUNIT_EXPECT_FALSE(test, entry->absent);
		if (entry->schema->op_id == taken->op_id)
			kept = true;
	}
	KUNIT_EXPECT_TRUE(test, kept);
}

static void ctlv_reg_test_state_after_publish(struct kunit *test)
{
	struct ctlv_reg_test_table *table;
	struct ctlv_op_schema *provided;
	struct ctlv_op_schema *omitted;
	struct ctlv_op_schema *dynamic;
	struct ctlv_op_entry *target;
	struct ctlv ctlv = {};
	u64 generation;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv_reg_test_build(test, table);

	/*
	 * Drop an op before the dynamic target, so publication moves the
	 * target over another entry rather than leaving its old slot intact.
	 */
	omitted = ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
				    CTLV_OP_TYPE_ACTION);
	omitted->optional = true;
	dynamic = ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_DYNAMIC,
				    CTLV_OP_TYPE_ACTION);
	provided = ctlv_reg_test_find(test, table, CTLV_OP_AVAILABILITY_DYNAMIC,
				     CTLV_OP_TYPE_QUERY);
	KUNIT_ASSERT_TRUE(test, provided->optional);

	spin_lock_init(&ctlv.transition_lock);
	ctlv_event_device_init(&ctlv);
	ctlv.family = &table->family;
	KUNIT_ASSERT_EQ(test, ctlv_op_resolution_init(&ctlv, &table->family), 0);
	kunit_add_action(test, ctlv_reg_test_free_ops, ctlv.ops);
	ctlv.disabled = bitmap_zalloc(ctlv.dynamic_ops_count, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, ctlv.disabled);
	kunit_add_action(test, ctlv_reg_test_free_bitmap, ctlv.disabled);

	kunit_activate_static_stub(test, ctlv_op_find,
				   ctlv_reg_test_find_locked);
	ctlv_op_provide(&ctlv, provided->op_id);
	ctlv_op_state_set(&ctlv, dynamic->op_id, true);
	KUNIT_EXPECT_EQ(test, ctlv.state_generation, 0);
	ctlv_op_state_set(&ctlv, dynamic->op_id, false);
	KUNIT_EXPECT_EQ(test, ctlv.state_generation, 0);
	kunit_deactivate_static_stub(test, ctlv_op_find);
	ctlv_op_publish_all(&ctlv);
	target = ctlv_op_find(&ctlv, dynamic->op_id);
	KUNIT_ASSERT_NOT_NULL(test, target);
	KUNIT_ASSERT_NE(test, target->state_index, CTLV_OP_STATE_NONE);

	kunit_activate_static_stub(test, ctlv_op_find,
				   ctlv_reg_test_find_locked);
	ctlv_op_state_set(&ctlv, dynamic->op_id, true);
	kunit_deactivate_static_stub(test, ctlv_op_find);
	KUNIT_EXPECT_TRUE(test,
			  test_bit(target->state_index, ctlv.disabled));
	generation = ctlv.state_generation;

	scoped_guard(spinlock, &ctlv.transition_lock)
		ctlv.destroying = true;
	ctlv_op_state_set(&ctlv, dynamic->op_id, false);
	KUNIT_EXPECT_TRUE(test,
			  test_bit(target->state_index, ctlv.disabled));
	KUNIT_EXPECT_EQ(test, ctlv.state_generation, generation);
}

/* Leave a claim that agrees with the real family loaded after KUnit. */
static void ctlv_reg_test_build_named(struct kunit *test,
				      struct ctlv_reg_test_table *table)
{
	ctlv_reg_test_build(test, table);
	table->parent.name = "ctlv-test";
	table->schema.name = "ctlv-test-child";
}

static void ctlv_reg_test_release(void *ctlv)
{
	ctlv_fini(ctlv);
}

/* Initialized on a device of its own, not published, released at the end. */
static struct ctlv *ctlv_reg_test_setup(struct kunit *test, const char *name,
					struct ctlv_reg_test_table *table)
{
	struct ctlv *ctlv;
	struct device *dev;

	ctlv = kunit_kzalloc(test, sizeof(*ctlv), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, ctlv);
	dev = kunit_device_register(test, name);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	ctlv_reg_test_build_named(test, table);
	KUNIT_ASSERT_EQ(test, ctlv_init(ctlv, dev, &table->family,
					&ctlv_reg_test_hooks), 0);
	KUNIT_ASSERT_EQ(test, kunit_add_action_or_reset(test,
							ctlv_reg_test_release,
							ctlv), 0);
	return ctlv;
}

/* A thread may not exit before kthread_stop(), which still needs its task. */
static void ctlv_reg_test_wait_stop(void)
{
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
}

struct ctlv_reg_test_unpublish {
	struct ctlv *ctlv;
	struct completion done;
	struct task_struct *task;
};

static int ctlv_reg_test_unpublish_thread(void *arg)
{
	struct ctlv_reg_test_unpublish *race = arg;

	ctlv_unpublish(race->ctlv);
	complete(&race->done);
	ctlv_reg_test_wait_stop();
	return 0;
}

struct ctlv_reg_test_producer {
	struct ctlv *ctlv;
	struct ctlv_file cfile;
	struct completion entered;
	struct completion release;
	struct completion done;
	struct task_struct *task;
	int result;
};

static int ctlv_reg_test_producer_thread(void *arg)
{
	struct ctlv_reg_test_producer *producer = arg;
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	complete(&producer->entered);
	wait_for_completion(&producer->release);
	ctx = ctlv_event_create(producer->ctlv, CTLV_FIXTURE_OP_PINGED,
				&payload, GFP_KERNEL);
	if (IS_ERR(ctx))
		producer->result = PTR_ERR(ctx);
	else
		producer->result = ctlv_event_emit(ctx);
	complete(&producer->done);
	ctlv_reg_test_wait_stop();
	return 0;
}

static int ctlv_reg_test_producer_start(struct ctlv_reg_test_producer *producer)
{
	producer->task = kthread_run(ctlv_reg_test_producer_thread, producer,
				     "ctlv-kernel-producer");
	if (IS_ERR(producer->task))
		return PTR_ERR(producer->task);
	wait_for_completion(&producer->entered);
	return 0;
}

/* Lets the producer emit, and waits until it has. */
static void ctlv_reg_test_producer_emit(struct kunit *test,
					struct ctlv_reg_test_producer *producer)
{
	KUNIT_EXPECT_TRUE(test, producer->ctlv->events_live);
	KUNIT_EXPECT_FALSE(test, producer->ctlv->dead_announced);
	if (IS_ERR_OR_NULL(producer->task))
		return;
	complete(&producer->release);
	wait_for_completion(&producer->done);
}

/* A file open on @ctlv and subscribed to the fixture's event. */
static void ctlv_reg_test_producer_init(struct kunit *test,
					struct ctlv_reg_test_producer *producer,
					struct ctlv *ctlv)
{
	const struct ctlv_op_entry *event;

	init_completion(&producer->entered);
	init_completion(&producer->release);
	init_completion(&producer->done);
	producer->ctlv = ctlv;
	producer->cfile.ctlv = ctlv;
	KUNIT_ASSERT_EQ(test, ctlv_event_file_init(&producer->cfile), 0);
	event = ctlv_op_find(ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_ASSERT_NOT_NULL(test, event);
	set_bit(event->event_index, producer->cfile.subscribed);
}

static void ctlv_reg_test_producer_fini(struct ctlv_reg_test_producer *producer)
{
	if (!IS_ERR_OR_NULL(producer->task))
		kthread_stop(producer->task);
	ctlv_event_file_exit(&producer->cfile);
}

static u32 ctlv_reg_test_event_ids(struct ctlv_reg_test_producer *producer,
				  u64 *ids, u32 ids_count)
{
	struct ctlv_event *event;
	u32 count = 0;

	guard(spinlock)(&producer->ctlv->events_lock);
	list_for_each_entry(event, &producer->cfile.events, node) {
		if (count < ids_count)
			ids[count] = event->hdr.op_id;
		count++;
	}
	return count;
}

static void ctlv_reg_test_expect_dead(struct kunit *test, struct ctlv *ctlv)
{
	KUNIT_EXPECT_FALSE(test, ctlv->registered);
	KUNIT_EXPECT_FALSE(test, ctlv->events_live);
	KUNIT_EXPECT_TRUE(test, ctlv->dead_announced);
	KUNIT_EXPECT_EQ(test, ctlv->active_event_contexts, 0);
	KUNIT_EXPECT_NULL(test, ctlv_family_of(ctlv));
}

static void ctlv_reg_test_publish_unpublish(struct kunit *test)
{
	struct ctlv_reg_test_table *table;
	struct ctlv *ctlv;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv = ctlv_reg_test_setup(test, "ctlv-publish", table);

	ctlv_publish(ctlv);
	KUNIT_EXPECT_TRUE(test, ctlv->registered);
	KUNIT_EXPECT_TRUE(test, ctlv->published);
	KUNIT_EXPECT_TRUE(test, ctlv->events_live);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_family_of(ctlv), &table->family);

	ctlv_unpublish(ctlv);
	ctlv_reg_test_expect_dead(test, ctlv);
}

/* Refused before anything is set up, so there is nothing to release. */
static void ctlv_reg_test_init_args(struct kunit *test)
{
	static const struct ctlv_hooks pre_only = {
		.pre = ctlv_reg_test_pre,
	};
	static const struct ctlv_hooks post_only = {
		.post = ctlv_reg_test_post,
	};
	struct ctlv_reg_test_table *table;
	struct ctlv *ctlv;
	struct device *dev;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv = kunit_kzalloc(test, sizeof(*ctlv), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, ctlv);
	dev = kunit_device_register(test, "ctlv-init-args");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, dev);
	ctlv_reg_test_build_named(test, table);

	KUNIT_EXPECT_EQ(test, ctlv_init(ctlv, NULL, &table->family,
					NULL), -EINVAL);
	KUNIT_EXPECT_EQ(test, ctlv_init(ctlv, dev, NULL, NULL), -EINVAL);
	KUNIT_EXPECT_EQ(test, ctlv_init(ctlv, dev, &table->family,
					&pre_only), -EINVAL);
	KUNIT_EXPECT_EQ(test, ctlv_init(ctlv, dev, &table->family,
					&post_only), -EINVAL);
	KUNIT_EXPECT_NULL(test, ctlv->ops);
	KUNIT_EXPECT_NULL(test, ctlv_family_of(ctlv));

	KUNIT_ASSERT_EQ(test, ctlv_init(ctlv, dev, &table->family, NULL), 0);
	KUNIT_EXPECT_NULL(test, ctlv->hooks);
	ctlv_fini(ctlv);
}

/* Events work from publication on, and arrive before the removal. */
static void ctlv_reg_test_events_until_unpublish(struct kunit *test)
{
	struct ctlv_reg_test_producer *producer;
	struct ctlv_reg_test_table *table;
	struct ctlv *ctlv;
	u64 ids[2] = {};

	producer = kunit_kzalloc(test, sizeof(*producer), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, producer);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv = ctlv_reg_test_setup(test, "ctlv-kernel-producer", table);
	ctlv_reg_test_producer_init(test, producer, ctlv);
	ctlv_publish(ctlv);
	KUNIT_EXPECT_EQ(test, ctlv_reg_test_producer_start(producer), 0);

	ctlv_reg_test_producer_emit(test, producer);
	ctlv_unpublish(ctlv);

	KUNIT_EXPECT_EQ(test, producer->result, 0);
	ctlv_reg_test_expect_dead(test, ctlv);
	KUNIT_EXPECT_EQ(test, ctlv_reg_test_event_ids(producer, ids,
						      ARRAY_SIZE(ids)), 2);
	KUNIT_EXPECT_EQ(test, ids[0], CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, ids[1], CTLV_OP_DEVICE_REMOVED);
	ctlv_reg_test_producer_fini(producer);
}

/* An admitted callback holds the unpublish off, its events still working. */
static void ctlv_reg_test_unpublish_waits(struct kunit *test)
{
	struct ctlv_reg_test_unpublish *race;
	struct ctlv_reg_test_table *table;
	struct ctlv *ctlv;
	struct ctlv_op_ctx *ctx;
	struct ctlv_cursor *payload;
	int tries;

	race = kunit_kzalloc(test, sizeof(*race), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, race);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	ctlv = ctlv_reg_test_setup(test, "ctlv-unpublish-waits", table);
	ctlv_publish(ctlv);
	race->ctlv = ctlv;
	init_completion(&race->done);

	/* Models an admitted userspace callback holding the read side. */
	down_read(&ctlv->registration_lock);
	race->task = kthread_run(ctlv_reg_test_unpublish_thread, race,
				 "ctlv-unpublish");
	if (IS_ERR(race->task)) {
		up_read(&ctlv->registration_lock);
		ctlv_unpublish(ctlv);
		KUNIT_FAIL(test, "failed to start unpublish thread");
		return;
	}
	for (tries = 0; tries < 50; tries++) {
		if (rwsem_is_contended(&ctlv->registration_lock))
			break;
		msleep(20);
	}
	KUNIT_EXPECT_TRUE(test, rwsem_is_contended(&ctlv->registration_lock));
	KUNIT_EXPECT_TRUE(test, ctlv->registered);
	KUNIT_EXPECT_FALSE(test, ctlv->destroying);
	KUNIT_EXPECT_TRUE(test, ctlv->events_live);

	ctx = ctlv_event_create(ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	if (IS_ERR(ctx))
		KUNIT_FAIL(test, "admitted callback could not create an event");
	else
		ctlv_event_abort(ctx);

	up_read(&ctlv->registration_lock);
	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&race->done, HZ),
			0UL);
	kthread_stop(race->task);
	ctlv_reg_test_expect_dead(test, ctlv);
}

static void ctlv_reg_test_root_family(struct kunit *test)
{
	struct ctlv ctlv = {};

	KUNIT_ASSERT_EQ(test, ctlv_op_resolution_init(&ctlv, &ctlv_family), 0);
	kunit_add_action(test, ctlv_reg_test_free_ops, ctlv.ops);
	KUNIT_EXPECT_EQ(test, ctlv.ops_count, ctlv_family.schema->ops_count);
}

static struct kunit_case ctlv_reg_test_cases[] = {
	KUNIT_CASE_PARAM(ctlv_reg_test_check,
			 registration_gen_params),
	KUNIT_CASE(ctlv_reg_test_uevent),
	KUNIT_CASE(ctlv_reg_test_resolved),
	KUNIT_CASE(ctlv_reg_test_published),
	KUNIT_CASE(ctlv_reg_test_state_after_publish),
	KUNIT_CASE(ctlv_reg_test_publish_unpublish),
	KUNIT_CASE(ctlv_reg_test_init_args),
	KUNIT_CASE(ctlv_reg_test_events_until_unpublish),
	KUNIT_CASE(ctlv_reg_test_unpublish_waits),
	KUNIT_CASE(ctlv_reg_test_root_family),
	{},
};

static void ctlv_reg_test_suite_exit(struct kunit_suite *suite)
{
	ctlv_family_claim_remove(ctlv_fixture_child_schema.family_id,
				 "ctlv-test-child");
	ctlv_family_claim_remove(ctlv_fixture_schema.family_id,
				 "ctlv-test");
}

static struct kunit_suite ctlv_reg_test_suite = {
	.name = "ctlv_registration",
	.test_cases = ctlv_reg_test_cases,
	.suite_exit = ctlv_reg_test_suite_exit,
};

kunit_test_suite(ctlv_reg_test_suite);
