// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * KUnit checks over what device creation refuses.
 *
 * Each case builds the descriptor a correct generator would emit for the
 * inheriting fixture, breaks one thing about it, and requires the resolution
 * the core runs before it indexes anything to say no. The schema graph is
 * copied first because the generated metadata is const.
 */

#include <kunit/static_stub.h>
#include <kunit/device.h>
#include <kunit/test.h>
#include <linux/align.h>
#include <linux/array_size.h>
#include <linux/bitmap.h>
#include <linux/completion.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/ctlv_fixture_child_gen.h>
#include <linux/ctlv_fixture_gen.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/minmax.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/vmalloc.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "../ctlv_private.h"
#include "ctlv_fixture_child_gen.h"
#include "ctlv_fixture_gen.h"

/* Every op the fixture chain declares; the framework answers the root's. */
#define REGISTRATION_N_OPS (CTLV_FIXTURE_N_OPS + CTLV_FIXTURE_CHILD_N_OPS)

/* Room for the extension points the fixture chain fills, and one to spare. */
#define REGISTRATION_MAX_STUBS 4

/* A callback has to exist to be present; nothing here ever calls one. */
static int registration_callback(void *priv, struct ctlv_op_ctx *ctx,
				 const struct ctlv_cursor *request,
				 struct ctlv_cursor *reply)
{
	return 0;
}

static int registration_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return 0;
}

static void registration_post(void *priv, struct ctlv_op_ctx *ctx)
{
}

static size_t registration_priv_size(void)
{
	return 0;
}

static int registration_init(struct ctlv_device *ctlvdev,
			     const void *init_info, void *priv)
{
	return 0;
}

static void registration_fini(struct ctlv_device *ctlvdev, void *priv)
{
}

static int registration_fill(void *priv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *nest)
{
	return 0;
}

static void registration_read(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *nest)
{
}

/*
 * The descriptors of a whole chain over a copy of it, which a case may modify.
 * A family binds the ops it declared, so there is one descriptor per family and
 * one binding table per descriptor.
 */
struct registration_table {
	struct ctlv_family family;
	struct ctlv_family parent_family;
	/* A descriptor of the family's own schema, for a case that misnames one. */
	struct ctlv_family stranger;
	struct ctlv_family_schema schema;
	struct ctlv_family_schema parent;
	struct ctlv_op_schema ops[CTLV_FIXTURE_CHILD_N_OPS];
	struct ctlv_op_schema parent_ops[CTLV_FIXTURE_N_OPS];
	/* One spare entry each, for the case that binds one op too many. */
	struct ctlv_op bindings[CTLV_FIXTURE_CHILD_N_OPS + 1];
	struct ctlv_op parent_bindings[CTLV_FIXTURE_N_OPS + 1];
	struct ctlv_stub_ops stubs[REGISTRATION_MAX_STUBS];
};

/*
 * The binding table a generator emits for one family: its own actions and
 * queries in declaration order, built from the schema array rather than from
 * the core's own walk, so that the two orders have to agree.
 */
static u32 registration_bind(struct ctlv_op *bindings,
			     const struct ctlv_op_schema *ops, u32 n_ops)
{
	u32 index, bound = 0;

	for (index = 0; index < n_ops; index++) {
		const struct ctlv_op_schema *op = &ops[index];
		struct ctlv_op *binding;

		if (op->type == CTLV_OP_TYPE_EVENT)
			continue;
		binding = &bindings[bound++];
		binding->action = registration_callback;
		if (op->pre_post) {
			binding->pre = registration_pre;
			binding->post = registration_post;
		}
	}
	return bound;
}

/* What a family answers with in each extension point its schema fills. */
static u32 registration_stubs(struct kunit *test,
			      struct registration_table *table,
			      const struct ctlv_family_schema *schema)
{
	u32 index;

	KUNIT_ASSERT_LE(test, schema->n_fills, REGISTRATION_MAX_STUBS);
	for (index = 0; index < schema->n_fills; index++)
		table->stubs[index] = (struct ctlv_stub_ops){
			.family_id = schema->fills[index].family_id,
			.set_id = schema->fills[index].set_id,
			.fill = registration_fill,
			.read = registration_read,
		};
	return schema->n_fills;
}

static void registration_build(struct kunit *test,
			       struct registration_table *table)
{
	memset(table, 0, sizeof(*table));
	memcpy(table->ops, ctlv_fixture_child_ctlv_op_schemas,
	       sizeof(table->ops));
	memcpy(table->parent_ops, ctlv_fixture_ctlv_op_schemas,
	       sizeof(table->parent_ops));

	table->parent = ctlv_fixture_ctlv_schema;
	table->parent.ops = table->parent_ops;
	table->schema = ctlv_fixture_child_ctlv_schema;
	table->schema.ops = table->ops;

	/* One generated lifecycle and core-allocated priv per family. */
	table->parent_family = (struct ctlv_family){
		.schema = &table->parent,
		.parent = &ctlv_ctlv_family,
		.ops = table->parent_bindings,
		.n_ops = registration_bind(table->parent_bindings,
					   table->parent_ops,
					   CTLV_FIXTURE_N_OPS),
		.pre = registration_pre,
		.post = registration_post,
		.priv_size = registration_priv_size,
		.init = registration_init,
		.fini = registration_fini,
	};
	table->family = (struct ctlv_family){
		.schema = &table->schema,
		.parent = &table->parent_family,
		.ops = table->bindings,
		.n_ops = registration_bind(table->bindings, table->ops,
					   CTLV_FIXTURE_CHILD_N_OPS),
		.pre = registration_pre,
		.post = registration_post,
		.priv_size = registration_priv_size,
		.init = registration_init,
		.fini = registration_fini,
		.stubs = table->stubs,
		.n_stubs = registration_stubs(test, table, &table->schema),
	};
	table->stranger = table->family;
	table->stranger.parent = NULL;
}

static struct ctlv_op_schema *
registration_find(struct kunit *test, struct registration_table *table,
		  enum ctlv_op_availability availability,
		  enum ctlv_op_type type)
{
	u32 index;

	for (index = 0; index < CTLV_FIXTURE_CHILD_N_OPS; index++)
		if (table->ops[index].availability == availability &&
		    table->ops[index].type == type)
			return &table->ops[index];
	for (index = 0; index < CTLV_FIXTURE_N_OPS; index++)
		if (table->parent_ops[index].availability == availability &&
		    table->parent_ops[index].type == type)
			return &table->parent_ops[index];
	KUNIT_FAIL_AND_ABORT(test, "the fixture has no such op to break");
	return NULL;
}

/* What answers one op: the binding of the family that declared it. */
static struct ctlv_op *registration_binding(struct kunit *test,
					    struct registration_table *table,
					    const struct ctlv_op_schema *op)
{
	const struct ctlv_op_schema *ops = table->ops;
	struct ctlv_op *bindings = table->bindings;
	u32 index, n_ops = CTLV_FIXTURE_CHILD_N_OPS, bound = 0;

	if (op < ops || op >= &ops[n_ops]) {
		ops = table->parent_ops;
		bindings = table->parent_bindings;
		n_ops = CTLV_FIXTURE_N_OPS;
	}
	for (index = 0; index < n_ops; index++) {
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
static void registration_free_ops(void *ops)
{
	kfree(ops);
}

static void registration_free_priv_block(void *priv_block)
{
	kvfree(priv_block);
}

static void registration_free_privs(void *privs)
{
	kfree(privs);
}

static void registration_free_bitmap(void *bitmap)
{
	bitmap_free(bitmap);
}

static struct ctlv_op_entry *
registration_find_locked(struct ctlv_device *ctlvdev, u64 op_id)
{
	u32 index;

	if (IS_ENABLED(CONFIG_SMP) &&
	    spin_trylock(&ctlvdev->transition_lock)) {
		spin_unlock(&ctlvdev->transition_lock);
		kunit_fail_current_test("op lookup ran outside transition lock");
	}
	for (index = 0; index < ctlvdev->n_ops; index++)
		if (ctlvdev->ops[index].schema->op_id == op_id)
			return &ctlvdev->ops[index];
	return NULL;
}

static void break_nothing(struct kunit *test, struct registration_table *table)
{
}

/* A family directly below the framework still names the core descriptor. */
static void break_to_root(struct kunit *test,
			  struct registration_table *table)
{
	table->family = table->parent_family;
}

static void break_schema(struct kunit *test, struct registration_table *table)
{
	table->family.schema = NULL;
}

static void break_name(struct kunit *test, struct registration_table *table)
{
	table->schema.name = "";
}

static void break_bindings(struct kunit *test,
			   struct registration_table *table)
{
	table->family.ops = NULL;
}

static void break_emptiness(struct kunit *test,
			    struct registration_table *table)
{
	table->family.n_ops = 0;
}

/* The descriptor of the family this one inherits, absent and mistaken. */
static void break_no_parent(struct kunit *test,
			    struct registration_table *table)
{
	table->family.parent = NULL;
}

static void break_other_parent(struct kunit *test,
			       struct registration_table *table)
{
	table->family.parent = &table->stranger;
}

/* A direct child's parent must be the core descriptor, not itself. */
static void break_root_parent(struct kunit *test,
			      struct registration_table *table)
{
	break_to_root(test, table);
	table->family.parent = &table->parent_family;
}

static void break_priv_size(struct kunit *test,
			    struct registration_table *table)
{
	table->parent_family.priv_size = NULL;
}

static void break_init(struct kunit *test, struct registration_table *table)
{
	table->parent_family.init = NULL;
}

static void break_fini(struct kunit *test, struct registration_table *table)
{
	table->parent_family.fini = NULL;
}

static size_t registration_overflow_priv_size(void)
{
	return SIZE_MAX;
}

static void break_priv_overflow(struct kunit *test,
				struct registration_table *table)
{
	table->family.priv_size = registration_overflow_priv_size;
}

/* An extension point is answered by the family whose schema fills it. */
static void break_no_stubs(struct kunit *test,
			   struct registration_table *table)
{
	KUNIT_ASSERT_GT(test, table->family.n_stubs, 0);
	table->family.n_stubs = 0;
}

static void break_other_stub(struct kunit *test,
			     struct registration_table *table)
{
	KUNIT_ASSERT_GT(test, table->family.n_stubs, 0);
	table->stubs[0].set_id++;
}

static struct ctlv_op *
registration_action(struct kunit *test, struct registration_table *table,
		    enum ctlv_op_availability availability)
{
	return registration_binding(test, table,
				    registration_find(test, table,
						      availability,
						      CTLV_OP_TYPE_ACTION));
}

static void break_callback(struct kunit *test,
			   struct registration_table *table)
{
	registration_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->action = NULL;
}

/* A disabled op still needs its callback: a device may enable it again. */
static void break_dynamic_callback(struct kunit *test,
				   struct registration_table *table)
{
	registration_action(test, table,
			    CTLV_OP_AVAILABILITY_DYNAMIC)->action = NULL;
}

/* The binding of the op that asked for a pair of its own. */
static struct ctlv_op *registration_marked(struct kunit *test,
					   struct registration_table *table)
{
	u32 index;

	for (index = 0; index < CTLV_FIXTURE_CHILD_N_OPS; index++)
		if (table->ops[index].pre_post)
			return registration_binding(test, table,
						    &table->ops[index]);
	for (index = 0; index < CTLV_FIXTURE_N_OPS; index++)
		if (table->parent_ops[index].pre_post)
			return registration_binding(test, table,
						    &table->parent_ops[index]);
	KUNIT_FAIL_AND_ABORT(test, "the fixture has no marked op to break");
	return NULL;
}

/* A descriptor is the only place a hook pair can contradict the schema. */
static void break_unasked_hooks(struct kunit *test,
				struct registration_table *table)
{
	struct ctlv_op *binding =
		registration_action(test, table, CTLV_OP_AVAILABILITY_FIXED);

	binding->pre = registration_pre;
	binding->post = registration_post;
}

static void break_missing_hooks(struct kunit *test,
				struct registration_table *table)
{
	struct ctlv_op *binding = registration_marked(test, table);

	binding->pre = NULL;
	binding->post = NULL;
}

/* Either half alone would leave the core unwinding what it never entered. */
static void break_op_pre(struct kunit *test, struct registration_table *table)
{
	registration_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->pre = registration_pre;
}

static void break_op_post(struct kunit *test, struct registration_table *table)
{
	registration_action(test, table,
			    CTLV_OP_AVAILABILITY_FIXED)->post =
		registration_post;
}

static void break_family_pre(struct kunit *test,
			     struct registration_table *table)
{
	table->family.pre = NULL;
}

static void break_family_post(struct kunit *test,
			      struct registration_table *table)
{
	table->family.post = NULL;
}

/* One op of the chain with nothing bound to it, and one binding too many. */
static void break_too_few_bindings(struct kunit *test,
				   struct registration_table *table)
{
	table->family.n_ops--;
}

static void break_too_many_bindings(struct kunit *test,
				    struct registration_table *table)
{
	table->bindings[table->family.n_ops].action = registration_callback;
	table->family.n_ops++;
}

static void break_order(struct kunit *test, struct registration_table *table)
{
	swap(table->parent_ops[0], table->parent_ops[1]);
}

static void break_uniqueness(struct kunit *test,
			     struct registration_table *table)
{
	table->parent_ops[1].op_id = table->parent_ops[0].op_id;
}

/* An op of a family that is not the one whose array it sits in. */
static void break_owner(struct kunit *test, struct registration_table *table)
{
	table->ops[0].op_id = CTLV_OP_ID(CTLV_FIXTURE_FAMILY_ID,
					 CTLV_FIXTURE_N_OPS + 1);
}

static void break_number(struct kunit *test, struct registration_table *table)
{
	table->ops[0].op_id = CTLV_OP_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, 0);
}

static void break_op_type(struct kunit *test, struct registration_table *table)
{
	table->ops[0].type = CTLV_OP_TYPE_EVENT + 1;
}

static void break_availability(struct kunit *test,
			       struct registration_table *table)
{
	table->ops[0].availability = CTLV_OP_AVAILABILITY_DYNAMIC + 1;
}

static void break_fd_access(struct kunit *test,
			    struct registration_table *table)
{
	registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_ACTION)->fd_access =
		CTLV_OP_FD_ACCESS_READ_WRITE + 1;
}

static void break_event_fd_access(struct kunit *test,
				  struct registration_table *table)
{
	registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->fd_access =
		CTLV_OP_FD_ACCESS_READ;
}

/* An event is produced by the family, so no device can take it away. */
static void break_dynamic_event(struct kunit *test,
				struct registration_table *table)
{
	registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->availability =
		CTLV_OP_AVAILABILITY_DYNAMIC;
}

/* Nor opt in to one, for the same reason. */
static void break_optional_event(struct kunit *test,
				 struct registration_table *table)
{
	registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
			  CTLV_OP_TYPE_EVENT)->optional = true;
}

/* A message no buffer of this framework could hold. */
static const struct ctlv_schema_node registration_huge[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
	},
	{
		.attr_id = CTLV_ATTR_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, 1, 1),
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .max_len = CTLV_MAX_INLINE_MESSAGE_LEN },
	},
};

static void break_length(struct kunit *test, struct registration_table *table)
{
	table->ops[0].request.nodes = registration_huge;
	table->ops[0].request.n_nodes = ARRAY_SIZE(registration_huge);
}

/* A graph whose chain is not one of the framework's. */
static void break_chain(struct kunit *test, struct registration_table *table)
{
	table->parent_family.parent = NULL;
}

static void break_core_id(struct kunit *test, struct registration_table *table)
{
	table->schema.family_id = CTLV_FAMILY_ID;
}

/* A child is named its parent's whole name and a suffix of its own. */
static void break_naming(struct kunit *test, struct registration_table *table)
{
	table->schema.name = "unrelated";
}

static void break_suffix(struct kunit *test, struct registration_table *table)
{
	table->schema.name = "ctlv-fixturechild";
}

static void break_parent_name(struct kunit *test,
			      struct registration_table *table)
{
	table->parent.name = NULL;
}

struct registration_case {
	const char *name;
	void (*damage)(struct kunit *test, struct registration_table *table);
	int expected;
};

static const struct registration_case registration_cases[] = {
	{ "a generated descriptor", break_nothing, 0 },
	{ "a family directly below the core", break_to_root, 0 },
	{ "no schema", break_schema, -EINVAL },
	{ "no name", break_name, -EINVAL },
	{ "no bindings", break_bindings, -EINVAL },
	{ "no binding at all", break_emptiness, -EINVAL },
	{ "no descriptor of the family it inherits", break_no_parent, -EINVAL },
	{ "the descriptor of another family", break_other_parent, -EINVAL },
	{ "itself as its parent descriptor", break_root_parent, -EINVAL },
	{ "no priv size callback", break_priv_size, -EINVAL },
	{ "no init callback", break_init, -EINVAL },
	{ "no fini callback", break_fini, -EINVAL },
	{ "a priv size that overflows", break_priv_overflow, -EOVERFLOW },
	{ "no answer to an extension point it fills", break_no_stubs,
	  -EINVAL },
	{ "an answer to an extension point it does not fill", break_other_stub,
	  -EINVAL },
	{ "no callback", break_callback, -EINVAL },
	{ "no callback on a disabled op", break_dynamic_callback, -EINVAL },
	{ "an op pre hook with no post", break_op_pre, -EINVAL },
	{ "an op post hook with no pre", break_op_post, -EINVAL },
	{ "a hook pair the op did not ask for", break_unasked_hooks, -EINVAL },
	{ "no hook pair where the op asked for one", break_missing_hooks,
	  -EINVAL },
	{ "no family pre hook", break_family_pre, -EINVAL },
	{ "no family post hook", break_family_post, -EINVAL },
	{ "one binding too few", break_too_few_bindings, -EINVAL },
	{ "one binding too many", break_too_many_bindings, -EINVAL },
	{ "ops out of order", break_order, -EINVAL },
	{ "two ops with one ID", break_uniqueness, -EINVAL },
	{ "an op of another family", break_owner, -EINVAL },
	{ "an op numbered zero", break_number, -EINVAL },
	{ "a type of its own invention", break_op_type, -EINVAL },
	{ "an fd access of its own invention", break_fd_access, -EINVAL },
	{ "an event with fd access", break_event_fd_access, -EINVAL },
	{ "an availability of its own invention", break_availability, -EINVAL },
	{ "an event a device could disable", break_dynamic_event, -EINVAL },
	{ "an event a device could omit", break_optional_event, -EINVAL },
	{ "a message this framework cannot carry", break_length, -EINVAL },
	{ "a chain that is not the core's", break_chain, -EINVAL },
	{ "the core family ID", break_core_id, -EINVAL },
	{ "a name of its own", break_naming, -EINVAL },
	{ "a name with no separator", break_suffix, -EINVAL },
	{ "an ancestor with no name", break_parent_name, -EINVAL },
};

KUNIT_ARRAY_PARAM_DESC(registration, registration_cases, name);

static void ctlv_registration_test_check(struct kunit *test)
{
	const struct registration_case *entry = test->param_value;
	struct registration_table *table;
	struct ctlv_device ctlvdev = {};

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);

	registration_build(test, table);
	entry->damage(test, table);
	KUNIT_EXPECT_EQ(test, ctlv_ops_resolve(&ctlvdev, &table->family),
			entry->expected);
	kvfree(ctlvdev.priv_block);
	kfree(ctlvdev.privs);
	kfree(ctlvdev.ops);
}

/*
 * A chain of names long enough to crowd the uevent budget, which no chain of
 * this tree comes near, is refused before a device exists to announce it.
 */
static void ctlv_registration_test_uevent(struct kunit *test)
{
	static const char name[] =
		"fixture-with-a-name-as-long-as-a-family-may-ever-report-of-self";
	struct registration_table *table;
	struct ctlv_family_schema *schemas;
	struct ctlv_family *chain;
	u32 levels, level;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	registration_build(test, table);
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
					     &ctlv_ctlv_family;
	}
	KUNIT_EXPECT_EQ(test, ctlv_uevent_check(&chain[levels - 1]), -E2BIG);
}

/*
 * Every op of the chain, the root's included, ascending, answered by the family
 * that declared it and with that family's priv, with the availability
 * and subscription bits numbered as the walk met them.
 */
static void ctlv_registration_test_resolved(struct kunit *test)
{
	struct registration_table *table;
	struct ctlv_device ctlvdev = {};
	u32 dynamic = 0, events = 0;
	u32 index;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	registration_build(test, table);
	KUNIT_ASSERT_EQ(test, ctlv_ops_resolve(&ctlvdev, &table->family), 0);
	kunit_add_action(test, registration_free_ops, ctlvdev.ops);
	kunit_add_action(test, registration_free_privs, ctlvdev.privs);
	kunit_add_action(test, registration_free_priv_block,
			 ctlvdev.priv_block);

	KUNIT_ASSERT_EQ(test, ctlvdev.n_ops, CTLV_N_OPS + REGISTRATION_N_OPS);
	KUNIT_ASSERT_EQ(test, ctlvdev.n_privs, 3);
	for (index = 0; index < ctlvdev.n_privs; index++)
		KUNIT_EXPECT_TRUE(test,
				  IS_ALIGNED((unsigned long)
					     ctlvdev.privs[index].priv,
					     CTLV_PRIV_ALIGN));
	KUNIT_EXPECT_NULL(test, ctlv_priv_parent(ctlvdev.privs[0].priv));
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_parent(ctlvdev.privs[1].priv),
			    ctlvdev.privs[0].priv);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_parent(ctlvdev.privs[2].priv),
			    ctlvdev.privs[1].priv);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_child(ctlvdev.privs[0].priv),
			    ctlvdev.privs[1].priv);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_child(ctlvdev.privs[1].priv),
			    ctlvdev.privs[2].priv);
	KUNIT_EXPECT_NULL(test, ctlv_priv_child(ctlvdev.privs[2].priv));
	for (index = 0; index < ctlvdev.n_ops; index++) {
		const struct ctlv_op_entry *entry = &ctlvdev.ops[index];
		u16 declared = CTLV_OP_ID_FAMILY(entry->schema->op_id);
		void *priv = ctlvdev.privs[2].priv;

		if (declared == CTLV_FAMILY_ID)
			priv = ctlvdev.privs[0].priv;
		else if (declared == CTLV_FIXTURE_FAMILY_ID)
			priv = ctlvdev.privs[1].priv;
		if (index)
			KUNIT_EXPECT_GT(test, entry->schema->op_id,
					ctlvdev.ops[index - 1].schema->op_id);
		KUNIT_EXPECT_PTR_EQ(test, entry->priv, priv);
		/* The framework answers root ops, each family its own. */
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
	KUNIT_EXPECT_EQ(test, ctlvdev.n_dynamic_ops, dynamic);
	KUNIT_EXPECT_GT(test, dynamic, 0);
	KUNIT_EXPECT_EQ(test, ctlvdev.n_events, events);
	/* The root's three, which every chain of this framework inherits. */
	KUNIT_EXPECT_GE(test, events, 3);
}

/*
 * An optional op is absent until a device opts in, publication drops every one
 * that did not, opting in twice is opting in once, and what is left is still in
 * the walk's order.
 */
static void ctlv_registration_test_published(struct kunit *test)
{
	struct registration_table *table;
	struct ctlv_device ctlvdev = {};
	u32 resolved, absent, index;
	struct ctlv_op_schema *taken;
	bool kept = false;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	registration_build(test, table);

	/* An op no device could ever disable, made one it need not have. */
	taken = registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
				  CTLV_OP_TYPE_ACTION);
	/* Resolution reads this, so it has to be set before resolving. */
	taken->optional = true;

	spin_lock_init(&ctlvdev.transition_lock);
	ctlvdev.family = &table->family;
	KUNIT_ASSERT_EQ(test, ctlv_ops_resolve(&ctlvdev, &table->family), 0);
	kunit_add_action(test, registration_free_ops, ctlvdev.ops);
	kunit_add_action(test, registration_free_privs, ctlvdev.privs);
	kunit_add_action(test, registration_free_priv_block,
			 ctlvdev.priv_block);
	resolved = ctlvdev.n_ops;

	/* This one and however many the fixture declares optional itself. */
	absent = 0;
	for (index = 0; index < resolved; index++)
		if (ctlvdev.ops[index].absent)
			absent++;
	KUNIT_ASSERT_GT(test, absent, 0);

	ctlv_device_op_provide(&ctlvdev, taken->op_id);
	ctlv_device_op_provide(&ctlvdev, taken->op_id);

	ctlv_ops_publish(&ctlvdev);
	KUNIT_EXPECT_TRUE(test, ctlvdev.published);
	KUNIT_EXPECT_EQ(test, ctlvdev.n_ops, resolved - (absent - 1));
	for (index = 0; index < ctlvdev.n_ops; index++) {
		const struct ctlv_op_entry *entry = &ctlvdev.ops[index];

		if (index)
			KUNIT_EXPECT_GT(test, entry->schema->op_id,
					ctlvdev.ops[index - 1].schema->op_id);
		KUNIT_EXPECT_FALSE(test, entry->absent);
		if (entry->schema->op_id == taken->op_id)
			kept = true;
	}
	KUNIT_EXPECT_TRUE(test, kept);
}

static void ctlv_registration_test_state_after_publish(struct kunit *test)
{
	struct registration_table *table;
	struct ctlv_op_schema *provided;
	struct ctlv_op_schema *omitted;
	struct ctlv_op_schema *dynamic;
	struct ctlv_op_entry *target;
	struct ctlv_device ctlvdev = {};
	u64 generation;

	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	registration_build(test, table);

	/*
	 * Drop an op before the dynamic target, so publication moves the
	 * target over another entry rather than leaving its old slot intact.
	 */
	omitted = registration_find(test, table, CTLV_OP_AVAILABILITY_FIXED,
				    CTLV_OP_TYPE_ACTION);
	omitted->optional = true;
	dynamic = registration_find(test, table, CTLV_OP_AVAILABILITY_DYNAMIC,
				    CTLV_OP_TYPE_ACTION);
	provided = registration_find(test, table, CTLV_OP_AVAILABILITY_DYNAMIC,
				     CTLV_OP_TYPE_QUERY);
	KUNIT_ASSERT_TRUE(test, provided->optional);

	spin_lock_init(&ctlvdev.transition_lock);
	ctlv_events_device_init(&ctlvdev);
	ctlvdev.family = &table->family;
	KUNIT_ASSERT_EQ(test, ctlv_ops_resolve(&ctlvdev, &table->family), 0);
	kunit_add_action(test, registration_free_ops, ctlvdev.ops);
	kunit_add_action(test, registration_free_privs, ctlvdev.privs);
	kunit_add_action(test, registration_free_priv_block,
			 ctlvdev.priv_block);
	ctlvdev.disabled = bitmap_zalloc(ctlvdev.n_dynamic_ops, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, ctlvdev.disabled);
	kunit_add_action(test, registration_free_bitmap, ctlvdev.disabled);

	kunit_activate_static_stub(test, ctlv_op_find,
				   registration_find_locked);
	ctlv_device_op_provide(&ctlvdev, provided->op_id);
	ctlv_device_op_state_set(&ctlvdev, dynamic->op_id, true);
	KUNIT_EXPECT_EQ(test, ctlvdev.state_generation, 0);
	ctlv_device_op_state_set(&ctlvdev, dynamic->op_id, false);
	KUNIT_EXPECT_EQ(test, ctlvdev.state_generation, 0);
	kunit_deactivate_static_stub(test, ctlv_op_find);
	ctlv_ops_publish(&ctlvdev);
	target = ctlv_op_find(&ctlvdev, dynamic->op_id);
	KUNIT_ASSERT_NOT_NULL(test, target);
	KUNIT_ASSERT_NE(test, target->state_index, CTLV_OP_STATE_NONE);

	kunit_activate_static_stub(test, ctlv_op_find,
				   registration_find_locked);
	ctlv_device_op_state_set(&ctlvdev, dynamic->op_id, true);
	kunit_deactivate_static_stub(test, ctlv_op_find);
	KUNIT_EXPECT_TRUE(test,
			  test_bit(target->state_index, ctlvdev.disabled));
	generation = ctlvdev.state_generation;

	scoped_guard(spinlock, &ctlvdev.transition_lock)
		ctlvdev.destroying = true;
	ctlv_device_op_state_set(&ctlvdev, dynamic->op_id, false);
	KUNIT_EXPECT_TRUE(test,
			  test_bit(target->state_index, ctlvdev.disabled));
	KUNIT_EXPECT_EQ(test, ctlvdev.state_generation, generation);
}

struct registration_open_race {
	struct inode inode;
	struct file filp;
	struct completion started;
	struct completion done;
	struct task_struct *task;
	struct kunit *test;
	struct ctlv_device *ctlvdev;
	bool initialized;
	bool finalized;
	int result;
};

static int registration_open_thread(void *arg)
{
	struct registration_open_race *race = arg;

	current->kunit_test = race->test;
	race->result = ctlv_fops_open(&race->inode, &race->filp);
	current->kunit_test = NULL;
	complete(&race->done);
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

static void registration_open_pinned(struct ctlv_device *ctlvdev)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_open_race *race = test->priv;

	KUNIT_EXPECT_PTR_EQ(test, ctlvdev, race->ctlvdev);
	complete(&race->started);
}

static int registration_race_init(struct ctlv_device *ctlvdev,
				  const void *init_info, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_open_race *race = test->priv;

	race->initialized = true;
	return 0;
}

static void registration_race_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_open_race *race = test->priv;

	race->finalized = true;
}

static int registration_cdev_add_fail(struct ctlv_device *ctlvdev)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_open_race *race = test->priv;

	KUNIT_EXPECT_FALSE(test, ctlvdev->registered);
	race->ctlvdev = ctlvdev;
	race->inode.i_cdev = &ctlvdev->cdev;
	race->task = kthread_run(registration_open_thread, race,
				 "ctlv-register-open");
	if (IS_ERR(race->task))
		return PTR_ERR(race->task);

	wait_for_completion(&race->started);
	return -EINVAL;
}

static void ctlv_registration_test_failed_open(struct kunit *test)
{
	struct registration_open_race *race;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	race = kunit_kzalloc(test, sizeof(*race), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, race);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-create");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_build(test, table);
	/* Leave a claim that agrees with the real family loaded after KUnit. */
	table->parent.name = "ctlv-test";
	table->schema.name = "ctlv-test-child";
	table->parent_family.init = registration_race_init;
	table->parent_family.fini = registration_race_fini;
	init_completion(&race->started);
	init_completion(&race->done);
	race->test = test;

	test->priv = race;
	kunit_activate_static_stub(test, ctlv_fops_open_pinned,
				   registration_open_pinned);
	kunit_activate_static_stub(test, ctlv_cdev_device_add,
				   registration_cdev_add_fail);
	created = ctlv_create(parent, &table->family, NULL);
	kunit_deactivate_static_stub(test, ctlv_cdev_device_add);
	kunit_deactivate_static_stub(test, ctlv_fops_open_pinned);
	test->priv = NULL;

	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);
	if (!race->task || IS_ERR(race->task)) {
		KUNIT_FAIL(test, "failed to start racing open");
		return;
	}
	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&race->done, HZ),
			0UL);
	kthread_stop(race->task);

	KUNIT_EXPECT_EQ(test, race->result, -ENODEV);
	KUNIT_EXPECT_NULL(test, race->filp.private_data);
	if (race->filp.private_data) {
		struct ctlv_file *cfile = race->filp.private_data;

		ctlv_events_file_exit(cfile);
		kfree(cfile);
		race->filp.private_data = NULL;
		put_device(&race->ctlvdev->dev);
	}
	KUNIT_EXPECT_TRUE(test, race->initialized);
	KUNIT_EXPECT_TRUE(test, race->finalized);
}

struct registration_lifecycle {
	char calls[16];
	u32 n_calls;
	u8 parent_init_info;
	void *parent_priv;
	void *family_priv;
	bool family_initialized;
	bool parent_pre_finalized;
	bool family_pre_finalized;
};

static void registration_lifecycle_log(char call)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	lifecycle->calls[lifecycle->n_calls++] = call;
}

static size_t registration_parent_priv_size(void)
{
	return sizeof(u64);
}

static size_t registration_family_priv_size(void)
{
	return sizeof(u32);
}

static const void *registration_family_pre_init(const void *init_info)
{
	const struct registration_lifecycle *lifecycle = init_info;

	registration_lifecycle_log('l');
	return &lifecycle->parent_init_info;
}

static const void *registration_parent_pre_init(const void *init_info)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	KUNIT_EXPECT_PTR_EQ(test, init_info, &lifecycle->parent_init_info);
	registration_lifecycle_log('p');
	return NULL;
}

static int registration_parent_init(struct ctlv_device *ctlvdev,
				    const void *init_info, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	KUNIT_EXPECT_PTR_EQ(test, init_info, &lifecycle->parent_init_info);
	KUNIT_EXPECT_NOT_NULL(test, priv);
	KUNIT_EXPECT_NOT_NULL(test, ctlv_priv_parent(priv));
	lifecycle->parent_priv = priv;
	lifecycle->family_priv = ctlv_priv_child(priv);
	KUNIT_EXPECT_NOT_NULL(test, lifecycle->family_priv);
	registration_lifecycle_log('P');
	return 0;
}

static int registration_family_init(struct ctlv_device *ctlvdev,
				    const void *init_info, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	KUNIT_EXPECT_PTR_EQ(test, init_info, lifecycle);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_parent(priv),
			    lifecycle->parent_priv);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->family_priv);
	KUNIT_EXPECT_NULL(test, ctlv_priv_child(priv));
	lifecycle->family_initialized = true;
	registration_lifecycle_log('L');
	return 0;
}

static int registration_parent_post_init(struct ctlv_device *ctlvdev,
					 void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	KUNIT_EXPECT_TRUE(test, ctlvdev->published);
	KUNIT_EXPECT_TRUE(test, ctlvdev->events_live);
	KUNIT_EXPECT_FALSE(test, ctlvdev->registered);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->parent_priv);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_child(priv),
			    lifecycle->family_priv);
	registration_lifecycle_log('O');
	return 0;
}

static int registration_family_post_init(struct ctlv_device *ctlvdev,
					 void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	KUNIT_EXPECT_TRUE(test, ctlvdev->published);
	KUNIT_EXPECT_TRUE(test, ctlvdev->events_live);
	KUNIT_EXPECT_FALSE(test, ctlvdev->registered);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->family_priv);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_priv_parent(priv),
			    lifecycle->parent_priv);
	registration_lifecycle_log('o');
	return 0;
}

static void registration_expect_device_family(struct kunit *test,
					      struct ctlv_device *ctlvdev)
{
	const struct ctlv_family *family = ctlv_device_family(ctlvdev);

	KUNIT_ASSERT_NOT_NULL(test, family);
	KUNIT_EXPECT_STREQ(test, family->schema->name, "ctlv-test-child");
}

static void registration_parent_pre_fini(struct ctlv_device *ctlvdev,
					 void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	registration_expect_device_family(test, ctlvdev);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->parent_priv);
	KUNIT_EXPECT_TRUE(test, lifecycle->family_initialized);
	KUNIT_EXPECT_FALSE(test, lifecycle->parent_pre_finalized);
	KUNIT_EXPECT_FALSE(test, lifecycle->family_pre_finalized);
	lifecycle->parent_pre_finalized = true;
	registration_lifecycle_log('Q');
}

static void registration_family_pre_fini(struct ctlv_device *ctlvdev,
					 void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	registration_expect_device_family(test, ctlvdev);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->family_priv);
	KUNIT_EXPECT_TRUE(test, lifecycle->parent_pre_finalized);
	KUNIT_EXPECT_FALSE(test, lifecycle->family_pre_finalized);
	lifecycle->family_pre_finalized = true;
	registration_lifecycle_log('q');
}

static void registration_parent_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	registration_expect_device_family(test, ctlvdev);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->parent_priv);
	KUNIT_EXPECT_EQ(test, lifecycle->parent_pre_finalized,
			lifecycle->family_initialized);
	registration_lifecycle_log('F');
}

static void registration_family_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_lifecycle *lifecycle = test->priv;

	registration_expect_device_family(test, ctlvdev);
	KUNIT_EXPECT_PTR_EQ(test, priv, lifecycle->family_priv);
	KUNIT_EXPECT_TRUE(test, lifecycle->parent_pre_finalized);
	KUNIT_EXPECT_TRUE(test, lifecycle->family_pre_finalized);
	registration_lifecycle_log('f');
}

static int registration_cdev_add_refuse(struct ctlv_device *ctlvdev)
{
	return -EINVAL;
}

static void registration_lifecycle_build(struct kunit *test,
					 struct registration_table *table)
{
	registration_build(test, table);
	table->parent.name = "ctlv-test";
	table->schema.name = "ctlv-test-child";
	table->parent_family.priv_size = registration_parent_priv_size;
	table->parent_family.pre_init = registration_parent_pre_init;
	table->parent_family.init = registration_parent_init;
	table->parent_family.post_init = registration_parent_post_init;
	table->parent_family.pre_fini = registration_parent_pre_fini;
	table->parent_family.fini = registration_parent_fini;
	table->family.priv_size = registration_family_priv_size;
	table->family.pre_init = registration_family_pre_init;
	table->family.init = registration_family_init;
	table->family.post_init = registration_family_post_init;
	table->family.pre_fini = registration_family_pre_fini;
	table->family.fini = registration_family_fini;
}

struct registration_destroy_race {
	struct registration_lifecycle lifecycle;
	struct ctlv_device *ctlvdev;
	struct completion deleted;
	struct completion done;
	struct task_struct *task;
	struct kunit *test;
};

static void registration_cdev_del_wait(struct ctlv_device *ctlvdev)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_destroy_race *race = test->priv;

	cdev_device_del(&ctlvdev->cdev, &ctlvdev->dev);
	complete(&race->deleted);
}

static int registration_destroy_thread(void *arg)
{
	struct registration_destroy_race *race = arg;

	current->kunit_test = race->test;
	ctlv_destroy(race->ctlvdev);
	current->kunit_test = NULL;
	complete(&race->done);
	return 0;
}

struct registration_producer {
	struct registration_lifecycle lifecycle;
	struct ctlv_device *ctlvdev;
	struct ctlv_file cfile;
	struct completion entered;
	struct completion release;
	struct completion done;
	struct task_struct *task;
	int result;
	bool shutdown_before_fini;
};

static int registration_producer_thread(void *arg)
{
	struct registration_producer *producer = arg;
	struct ctlv_cursor *event;
	struct ctlv_op_ctx *ctx;

	complete(&producer->entered);
	wait_for_completion(&producer->release);
	ctx = ctlv_event_create(producer->ctlvdev, CTLV_FIXTURE_OP_PINGED,
				&event, GFP_KERNEL);
	if (IS_ERR(ctx))
		producer->result = PTR_ERR(ctx);
	else
		producer->result = ctlv_op_emit(ctx);
	complete(&producer->done);
	return 0;
}

static int registration_producer_start(struct registration_producer *producer)
{
	producer->task = kthread_run(registration_producer_thread, producer,
				     "ctlv-kernel-producer");
	if (IS_ERR(producer->task))
		return PTR_ERR(producer->task);
	wait_for_completion(&producer->entered);
	return 0;
}

static int
registration_producer_post_init(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_producer *producer = test->priv;
	int ret;

	ret = registration_family_post_init(ctlvdev, priv);
	if (ret)
		return ret;
	producer->ctlvdev = ctlvdev;
	return registration_producer_start(producer);
}

static int
registration_parent_post_init_fail(struct ctlv_device *ctlvdev, void *priv)
{
	int ret = registration_parent_post_init(ctlvdev, priv);

	return ret ? : -EUCLEAN;
}

static void
registration_producer_pre_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_producer *producer = test->priv;

	registration_family_pre_fini(ctlvdev, priv);
	KUNIT_EXPECT_TRUE(test, ctlvdev->events_live);
	KUNIT_EXPECT_FALSE(test, ctlvdev->destroying);
	KUNIT_EXPECT_FALSE(test, ctlvdev->dead_announced);
	if (IS_ERR_OR_NULL(producer->task))
		return;
	complete(&producer->release);
	wait_for_completion(&producer->done);
	KUNIT_EXPECT_EQ(test, producer->result, 0);
}

static void registration_producer_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct kunit *test = kunit_get_current_test();
	struct registration_producer *producer = test->priv;

	KUNIT_EXPECT_FALSE(test, ctlvdev->events_live);
	KUNIT_EXPECT_TRUE(test, ctlvdev->destroying);
	KUNIT_EXPECT_TRUE(test, ctlvdev->dead_announced);
	KUNIT_EXPECT_EQ(test, ctlvdev->active_event_contexts, 0);
	producer->shutdown_before_fini = true;
	registration_family_fini(ctlvdev, priv);
}

static void registration_producer_init(struct registration_producer *producer)
{
	init_completion(&producer->entered);
	init_completion(&producer->release);
	init_completion(&producer->done);
}

static u32 registration_event_ids(struct registration_producer *producer,
				  u64 *ids, u32 n_ids)
{
	struct ctlv_event_record *record;
	u32 count = 0;

	guard(spinlock)(&producer->ctlvdev->events_lock);
	list_for_each_entry(record, &producer->cfile.records, node) {
		if (count < n_ids)
			ids[count] = record->hdr.op_id;
		count++;
	}
	return count;
}

static void ctlv_registration_test_lifecycle(struct kunit *test)
{
	struct registration_lifecycle *lifecycle;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	lifecycle = kunit_kzalloc(test, sizeof(*lifecycle), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, lifecycle);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-lifecycle");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);

	test->priv = lifecycle;
	kunit_activate_static_stub(test, ctlv_cdev_device_add,
				   registration_cdev_add_refuse);
	created = ctlv_create(parent, &table->family, lifecycle);
	kunit_deactivate_static_stub(test, ctlv_cdev_device_add);
	test->priv = NULL;

	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);
	KUNIT_EXPECT_STREQ(test, lifecycle->calls, "lpPLoOQqfF");
}

static const void *registration_pre_init_fail(const void *init_info)
{
	registration_lifecycle_log('l');
	return ERR_PTR(-EUCLEAN);
}

static int registration_init_fail(struct ctlv_device *ctlvdev,
				  const void *init_info, void *priv)
{
	registration_lifecycle_log('L');
	return -EIO;
}

static int registration_post_init_fail(struct ctlv_device *ctlvdev, void *priv)
{
	registration_lifecycle_log('o');
	return -EUCLEAN;
}

static void ctlv_registration_test_pre_init_failure(struct kunit *test)
{
	struct registration_lifecycle *lifecycle;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	lifecycle = kunit_kzalloc(test, sizeof(*lifecycle), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, lifecycle);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-pre-init-fail");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	table->family.pre_init = registration_pre_init_fail;

	test->priv = lifecycle;
	created = ctlv_create(parent, &table->family, lifecycle);
	test->priv = NULL;

	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EUCLEAN);
	KUNIT_EXPECT_STREQ(test, lifecycle->calls, "l");
}

static void ctlv_registration_test_init_failure(struct kunit *test)
{
	struct registration_lifecycle *lifecycle;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	lifecycle = kunit_kzalloc(test, sizeof(*lifecycle), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, lifecycle);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-init-fail");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	table->family.init = registration_init_fail;

	test->priv = lifecycle;
	created = ctlv_create(parent, &table->family, lifecycle);
	test->priv = NULL;

	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EIO);
	KUNIT_EXPECT_STREQ(test, lifecycle->calls, "lpPLF");
}

static void ctlv_registration_test_post_init_failure(struct kunit *test)
{
	struct registration_lifecycle *lifecycle;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	lifecycle = kunit_kzalloc(test, sizeof(*lifecycle), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, lifecycle);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-post-init-fail");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	table->family.post_init = registration_post_init_fail;

	test->priv = lifecycle;
	kunit_activate_static_stub(test, ctlv_cdev_device_add,
				   registration_cdev_add_refuse);
	created = ctlv_create(parent, &table->family, lifecycle);
	kunit_deactivate_static_stub(test, ctlv_cdev_device_add);
	test->priv = NULL;

	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EUCLEAN);
	KUNIT_EXPECT_STREQ(test, lifecycle->calls, "lpPLoQqfF");
}

static void ctlv_registration_test_destroy(struct kunit *test)
{
	struct registration_lifecycle *lifecycle;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	lifecycle = kunit_kzalloc(test, sizeof(*lifecycle), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, lifecycle);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-destroy");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);

	test->priv = lifecycle;
	created = ctlv_create(parent, &table->family, lifecycle);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, created);
	KUNIT_EXPECT_PTR_EQ(test, ctlv_device_parent(created), parent);
	ctlv_destroy(created);
	test->priv = NULL;

	KUNIT_EXPECT_STREQ(test, lifecycle->calls, "lpPLoOQqfF");
}

static void ctlv_registration_test_kernel_producer_drain(struct kunit *test)
{
	struct registration_producer *producer;
	const struct ctlv_op_entry *event;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;
	u64 ids[2] = {};
	int ret;

	producer = kunit_kzalloc(test, sizeof(*producer), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, producer);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-kernel-producer");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	table->family.pre_fini = registration_producer_pre_fini;
	table->family.fini = registration_producer_fini;
	registration_producer_init(producer);

	test->priv = producer;
	created = ctlv_create(parent, &table->family, &producer->lifecycle);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, created);
	producer->ctlvdev = created;
	get_device(&created->dev);
	producer->cfile.ctlvdev = created;
	ret = ctlv_events_file_init(&producer->cfile);
	KUNIT_ASSERT_EQ(test, ret, 0);
	event = ctlv_op_find(created, CTLV_FIXTURE_OP_PINGED);
	KUNIT_ASSERT_NOT_NULL(test, event);
	set_bit(event->event_index, producer->cfile.subscribed);
	KUNIT_ASSERT_EQ(test, registration_producer_start(producer), 0);

	ctlv_destroy(created);
	KUNIT_EXPECT_STREQ(test, producer->lifecycle.calls, "lpPLoOQqfF");
	KUNIT_EXPECT_TRUE(test, producer->shutdown_before_fini);
	KUNIT_EXPECT_EQ(test, registration_event_ids(producer, ids,
						     ARRAY_SIZE(ids)), 2);
	KUNIT_EXPECT_EQ(test, ids[0], CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, ids[1], CTLV_OP_DEVICE_REMOVED);

	kthread_stop(producer->task);
	ctlv_events_file_exit(&producer->cfile);
	put_device(&created->dev);
	test->priv = NULL;
}

static void
ctlv_registration_test_post_init_exposure_failure(struct kunit *test)
{
	struct registration_producer *producer;
	struct registration_table *table;
	struct ctlv_device *created;
	struct device *parent;

	producer = kunit_kzalloc(test, sizeof(*producer), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, producer);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-post-init-exposure");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	table->family.post_init = registration_producer_post_init;
	table->parent_family.post_init = registration_parent_post_init_fail;
	table->family.pre_fini = registration_producer_pre_fini;
	table->family.fini = registration_producer_fini;
	registration_producer_init(producer);

	test->priv = producer;
	created = ctlv_create(parent, &table->family, &producer->lifecycle);
	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EUCLEAN);
	KUNIT_ASSERT_FALSE(test, IS_ERR_OR_NULL(producer->task));
	kthread_stop(producer->task);
	test->priv = NULL;

	KUNIT_EXPECT_EQ(test, producer->result, 0);
	KUNIT_EXPECT_TRUE(test, producer->shutdown_before_fini);
	KUNIT_EXPECT_STREQ(test, producer->lifecycle.calls, "lpPLoOQqfF");
}

static void ctlv_registration_test_destroy_closes_admission_first(struct kunit *test)
{
	struct registration_destroy_race *race;
	struct registration_table *table;
	struct ctlv_op_ctx *ctx;
	struct ctlv_cursor *event;
	struct device *parent;
	unsigned long waited;

	race = kunit_kzalloc(test, sizeof(*race), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, race);
	table = kunit_kzalloc(test, sizeof(*table), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, table);
	parent = kunit_device_register(test, "ctlv-destroy-admission");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	registration_lifecycle_build(test, table);
	init_completion(&race->deleted);
	init_completion(&race->done);
	race->test = test;

	test->priv = race;
	race->ctlvdev = ctlv_create(parent, &table->family,
				    &race->lifecycle);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, race->ctlvdev);

	/* Models an admitted userspace callback holding the read side. */
	down_read(&race->ctlvdev->registration_lock);
	kunit_activate_static_stub(test, ctlv_cdev_device_del,
				   registration_cdev_del_wait);
	race->task = kthread_run(registration_destroy_thread, race,
				 "ctlv-destroy-admission");
	if (IS_ERR(race->task)) {
		up_read(&race->ctlvdev->registration_lock);
		kunit_deactivate_static_stub(test, ctlv_cdev_device_del);
		ctlv_destroy(race->ctlvdev);
		test->priv = NULL;
		KUNIT_FAIL(test, "failed to start destroy thread");
		return;
	}

	waited = wait_for_completion_timeout(&race->deleted, HZ);
	KUNIT_EXPECT_NE(test, waited, 0UL);
	KUNIT_EXPECT_TRUE(test, race->ctlvdev->registered);
	KUNIT_EXPECT_FALSE(test, race->ctlvdev->destroying);
	KUNIT_EXPECT_TRUE(test, race->ctlvdev->events_live);
	KUNIT_EXPECT_FALSE(test, race->lifecycle.parent_pre_finalized);
	KUNIT_EXPECT_FALSE(test, race->lifecycle.family_pre_finalized);

	ctx = ctlv_event_create(race->ctlvdev, CTLV_FIXTURE_OP_PINGED,
				&event, GFP_KERNEL);
	if (IS_ERR(ctx))
		KUNIT_FAIL(test, "admitted callback could not create an event");
	else
		ctlv_op_abort(ctx);

	up_read(&race->ctlvdev->registration_lock);
	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&race->done, HZ),
			0UL);
	kunit_deactivate_static_stub(test, ctlv_cdev_device_del);
	kthread_stop(race->task);
	test->priv = NULL;
	KUNIT_EXPECT_STREQ(test, race->lifecycle.calls, "lpPLoOQqfF");
}

static void ctlv_registration_test_family_is_a(struct kunit *test)
{
	struct ctlv_family unrelated = {};
	struct ctlv_family child = {};
	struct ctlv_family parent = {};

	child.parent = &parent;
	parent.parent = &ctlv_ctlv_family;
	KUNIT_EXPECT_TRUE(test, ctlv_family_is_a(&child, &child));
	KUNIT_EXPECT_TRUE(test, ctlv_family_is_a(&child, &parent));
	KUNIT_EXPECT_TRUE(test, ctlv_family_is_a(&child, &ctlv_ctlv_family));
	KUNIT_EXPECT_FALSE(test, ctlv_family_is_a(&child, &unrelated));

	parent.parent = &child;
	KUNIT_EXPECT_FALSE(test, ctlv_family_is_a(&child, &unrelated));
}

static void ctlv_registration_test_root_family(struct kunit *test)
{
	struct ctlv_device ctlvdev = {};

	KUNIT_ASSERT_EQ(test,
			ctlv_ops_resolve(&ctlvdev, &ctlv_ctlv_family), 0);
	kunit_add_action(test, registration_free_ops, ctlvdev.ops);
	kunit_add_action(test, registration_free_privs, ctlvdev.privs);
	kunit_add_action(test, registration_free_priv_block,
			 ctlvdev.priv_block);
	KUNIT_EXPECT_EQ(test, ctlvdev.n_ops, CTLV_N_OPS);
	KUNIT_EXPECT_EQ(test, ctlvdev.n_privs, 1);
}

static void ctlv_registration_test_root_init_info(struct kunit *test)
{
	struct device *parent;
	struct ctlv_device *created;
	u8 init_info;

	parent = kunit_device_register(test, "ctlv-root-info");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	kunit_warning_suppress(test) {
		created = ctlv_create(parent, &ctlv_ctlv_family, &init_info);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_ASSERT_TRUE(test, IS_ERR(created));
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);
}

static struct kunit_case ctlv_registration_test_cases[] = {
	KUNIT_CASE_PARAM(ctlv_registration_test_check,
			 registration_gen_params),
	KUNIT_CASE(ctlv_registration_test_uevent),
	KUNIT_CASE(ctlv_registration_test_resolved),
	KUNIT_CASE(ctlv_registration_test_published),
	KUNIT_CASE(ctlv_registration_test_state_after_publish),
	KUNIT_CASE(ctlv_registration_test_failed_open),
	KUNIT_CASE(ctlv_registration_test_lifecycle),
	KUNIT_CASE(ctlv_registration_test_pre_init_failure),
	KUNIT_CASE(ctlv_registration_test_init_failure),
	KUNIT_CASE(ctlv_registration_test_post_init_failure),
	KUNIT_CASE(ctlv_registration_test_destroy),
	KUNIT_CASE(ctlv_registration_test_kernel_producer_drain),
	KUNIT_CASE(ctlv_registration_test_post_init_exposure_failure),
	KUNIT_CASE(ctlv_registration_test_destroy_closes_admission_first),
	KUNIT_CASE(ctlv_registration_test_family_is_a),
	KUNIT_CASE(ctlv_registration_test_root_family),
	KUNIT_CASE(ctlv_registration_test_root_init_info),
	{},
};

static struct kunit_suite ctlv_registration_test_suite = {
	.name = "ctlv_registration",
	.test_cases = ctlv_registration_test_cases,
};

kunit_test_suite(ctlv_registration_test_suite);
