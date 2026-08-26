// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * KUnit checks over one message in either direction: what validation records
 * about a message it refuses, what a misused accessor or serializer does, and
 * what ending a query's fragment does to the message it ends.
 *
 * Misuse of an accessor or a serializer is a driver bug, not anything a caller
 * can send, so the warning is deliberate and the cases suppress it.
 */

#include <kunit/static_stub.h>
#include <kunit/test.h>
#include <linux/align.h>
#include <linux/array_size.h>
#include <linux/ctlv.h>
#include <linux/ctlv_fixture_gen.h>
#include <linux/string.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "../ctlv_private.h"
#include "../message.h"
#include "../schema.h"
#include "ctlv_fixture_gen.h"

/*
 * Every attr is given its ID and its payload length, so a message this builds
 * may be one no caller should be able to send.
 */

/* Enough for a nest chain past the depth the framework allows. */
#define MSG_LEN 1024
#define MSG_DEPTH (CTLV_MAX_NEST_DEPTH + 4)

static const struct ctlv_family message_fixture_family = {
	.schema = &ctlv_fixture_ctlv_schema,
	.parent = &ctlv_ctlv_family,
};

struct msg {
	u32 len;
	u32 open[MSG_DEPTH];
	u32 depth;
	u64 buf[MSG_LEN / sizeof(u64)];
};

static struct ctlv_attr *msg_attr(struct msg *msg, u32 offset)
{
	return (struct ctlv_attr *)((u8 *)msg->buf + offset);
}

/* One attr, aligned and complete, at the offset this returns. */
static u32 msg_put(struct msg *msg, u64 attr_id, u32 flags,
		   const void *payload, u32 len)
{
	u32 offset = ALIGN(msg->len, CTLV_ALIGNTO);
	struct ctlv_attr *attr = msg_attr(msg, offset);

	if (offset + sizeof(*attr) + len > MSG_LEN)
		return offset;
	memset((u8 *)msg->buf + msg->len, 0, offset - msg->len);
	attr->len = sizeof(*attr) + len;
	attr->flags = flags;
	attr->attr_id = attr_id;
	memcpy(attr + 1, payload, len);
	msg->len = offset + attr->len;
	return offset;
}

static u32 msg_u64(struct msg *msg, u64 attr_id, u64 value)
{
	return msg_put(msg, attr_id, 0, &value, sizeof(value));
}

static u32 msg_nest_start(struct msg *msg, u64 attr_id)
{
	u32 offset = msg_put(msg, attr_id, CTLV_ATTR_F_NESTED, NULL, 0);

	msg->open[msg->depth++] = offset;
	return offset;
}

/* A nest is as long as the aligned spans of the children it contains. */
static void msg_nest_end(struct msg *msg)
{
	u32 offset = msg->open[--msg->depth];

	msg->len = ALIGN(msg->len, CTLV_ALIGNTO);
	msg_attr(msg, offset)->len = msg->len - offset;
}

static void msg_init(struct msg *msg)
{
	memset(msg, 0, sizeof(*msg));
	msg_nest_start(msg, CTLV_ATTR_ID_ROOT);
}

static void msg_done(struct msg *msg)
{
	while (msg->depth)
		msg_nest_end(msg);
}

/* One op of the fixture family, whose schema is what a message is read by. */
static const struct ctlv_op_schema *fixture_op(u64 op_id)
{
	u32 index;

	for (index = 0; index < ctlv_fixture_ctlv_schema.n_ops; index++)
		if (ctlv_fixture_ctlv_schema.ops[index].op_id == op_id)
			return &ctlv_fixture_ctlv_schema.ops[index];
	return NULL;
}

static const struct ctlv_schema_tree *request_tree(u64 op_id)
{
	return &fixture_op(op_id)->request;
}

static struct ctlv_input_ref input_ref(const struct ctlv_schema_tree *tree,
				       u32 node, u32 slot)
{
	const struct ctlv_schema_node *parent = &tree->nodes[node];
	const struct ctlv_schema_node *child =
		&tree->nodes[parent->first_child + slot];

	return (struct ctlv_input_ref){
		.tree = tree,
		.attr_id = child->attr_id,
		.node = node,
		.slot = slot,
		.kind = child->kind,
	};
}

/*
 * A reply tree describes a message the same way a request tree does, and the
 * fixture's deepest nest chain is in one.
 */
static const struct ctlv_schema_tree *reply_tree(u64 op_id)
{
	return &fixture_op(op_id)->reply;
}

static int validate_indexed_family(struct msg *msg,
				   const struct ctlv_schema_tree *tree,
				   const struct ctlv_family *family,
				   struct ctlv_msg *message,
				   struct ctlv_error *error)
{
	msg_done(msg);
	*message = (struct ctlv_msg){
		.root = msg_attr(msg, 0),
		.len = msg->len,
		.tree = tree,
		.family = family,
		.request = true,
	};
	*error = (struct ctlv_error){};
	return ctlv_schema_validate(message, error);
}

static int validate_indexed(struct msg *msg,
			    const struct ctlv_schema_tree *tree,
			    struct ctlv_msg *message, struct ctlv_error *error)
{
	return validate_indexed_family(msg, tree, &message_fixture_family,
				       message, error);
}

static int validate_as(struct msg *msg, const struct ctlv_schema_tree *tree,
		       bool request, struct ctlv_error *error)
{
	struct ctlv_msg message;
	int ret;

	if (request) {
		ret = validate_indexed(msg, tree, &message, error);
	} else {
		msg_done(msg);
		message = (struct ctlv_msg){
			.root = msg_attr(msg, 0),
			.len = msg->len,
			.tree = tree,
			.family = &message_fixture_family,
		};
		*error = (struct ctlv_error){};
		ret = ctlv_schema_validate(&message, error);
	}
	ctlv_msg_index_free(&message);
	return ret;
}

/* A message as a caller sent it, which is what a request is. */
static int validate(struct msg *msg, const struct ctlv_schema_tree *tree,
		    struct ctlv_error *error)
{
	return validate_as(msg, tree, true, error);
}

/* The same bytes as an output, which the core builds and never receives. */
static int validate_reply(struct msg *msg, const struct ctlv_schema_tree *tree,
			  struct ctlv_error *error)
{
	return validate_as(msg, tree, false, error);
}

/*
 * Collapse-check requires scalar.u and allows scalar.mode, so the failures
 * below are each one line away from a message it accepts.
 */
static void message_test_a_message_it_accepts(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, 0, error.reason);
}

static void message_test_a_missing_attr_is_named(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	/* Requiredness is reported before this present attr's bad value. */
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_AUTO + 1);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_MISSING_ATTR,
			error.reason);
	/* The only failure that names an attr the message does not carry. */
	KUNIT_EXPECT_EQ(test, CTLV_FIXTURE_ATTR_SCALAR_U,
			error.missing_attr_id);
	/* The offsets are the nest that should have carried it. */
	KUNIT_EXPECT_EQ(test, 0, error.nest_offset);
	KUNIT_EXPECT_EQ(test, 0, error.bad_attr_offset);
}

/*
 * Every other failure names the attr and the nest as offsets from the root,
 * because an offset survives the snapshot the attr was found in.
 */
static void message_test_a_bad_attr_is_located(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 mode;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	/* One past the highest value the enum declares. */
	mode = msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		       CTLV_FIXTURE_MODE_AUTO + 1);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE,
			error.reason);
	KUNIT_EXPECT_EQ(test, mode, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, 0, error.nest_offset);
	KUNIT_EXPECT_EQ(test, 0, error.missing_attr_id);
}

/* An attr inside a nest is located by both, which is not the same offset. */
static void message_test_a_nest_is_located_too(struct kunit *test)
{
	struct ctlv_error error;
	u32 level1, level2, u;
	struct msg msg;

	msg_init(&msg);
	level1 = msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_LEVEL1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	level2 = msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_LEVEL2);
	/* Above the maximum of the attr, wherever the attr is placed. */
	u = msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 4097);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE,
			error.reason);
	KUNIT_EXPECT_EQ(test, u, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, level2, error.nest_offset);
	KUNIT_EXPECT_NE(test, level1, level2);
}

static void message_test_the_occurrence_too_many(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 excess;
	u32 index;

	msg_init(&msg);
	/* Three is what the placement allows, and the fourth is reported. */
	for (index = 0; index < 3; index++)
		msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, index);
	excess = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 3);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES,
			error.reason);
	KUNIT_EXPECT_EQ(test, excess, error.bad_attr_offset);
}

/* The one failure found by walking, so what it names is the nest. */
static void message_test_a_malformed_nest(struct kunit *test)
{
	struct ctlv_msg message;
	struct ctlv_error error;
	struct msg msg;
	u32 attr;
	int ret;

	msg_init(&msg);
	attr = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_done(&msg);
	msg_attr(&msg, attr)->len = 0;
	ret = validate_indexed(&msg,
			       request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
			       &message, &error);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	KUNIT_EXPECT_NULL(test, message.index);
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_MALFORMED_MESSAGE,
			error.reason);
	KUNIT_EXPECT_EQ(test, 0, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, 0, error.nest_offset);
}

/*
 * An ID of a family nothing registered, and one whose number this family never
 * declared: neither can mean anything to this kernel.
 */
#define ATTR_FOREIGN CTLV_ATTR_ID(0xfffe, 1, 1)
#define ATTR_UNDECLARED \
	CTLV_ATTR_ID(CTLV_FIXTURE_FAMILY_ID, CTLV_FIXTURE_GROUP_SCALAR, 99)

/* What a caller built against a newer schema says it can do without. */
static u32 msg_ignorable(struct msg *msg, u64 attr_id, u64 value)
{
	return msg_put(msg, attr_id, CTLV_ATTR_F_IGNORE_IF_UNKNOWN, &value,
		       sizeof(value));
}

static const struct ctlv_schema_node precedence_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 1,
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_SCALAR_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_COUNT_FEW,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_SCALAR_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = {
			.min = CTLV_FIXTURE_MODE_DISABLED,
			.max = CTLV_FIXTURE_MODE_AUTO,
		},
	},
};

static const struct ctlv_schema_tree precedence_tree =
	CTLV_SCHEMA_TREE(precedence_nodes);

static const struct ctlv_schema_node nested_run_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_NEST_REQUEST,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 2,
		.n_children = 2,
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_COUNT_FEW,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 3,
		.range = { .min = 0, .max = U64_MAX },
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_SCALAR_MODE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = {
			.min = CTLV_FIXTURE_MODE_DISABLED,
			.max = CTLV_FIXTURE_MODE_AUTO,
		},
	},
};

static const struct ctlv_schema_tree nested_run_tree =
	CTLV_SCHEMA_TREE(nested_run_nodes);

static const struct ctlv_schema_node optional_base_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_stub_ref optional_base_stubs[] = {
	{ .family_id = 1, .set_id = 1, .node = 0, .optional = true },
};

static const struct ctlv_schema_tree optional_base_tree =
	CTLV_SCHEMA_TREE_STUBS(optional_base_nodes, optional_base_stubs);

static const struct ctlv_schema_node optional_middle_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_stub_ref optional_middle_stubs[] = {
	{ .family_id = 2, .set_id = 1, .node = 0 },
};

static const struct ctlv_schema_node optional_leaf_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_SCALAR_U,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

static const struct ctlv_stub_fill optional_fills[] = {
	{
		.family_id = 1,
		.set_id = 1,
		.members =
			CTLV_SCHEMA_TREE_STUBS(optional_middle_nodes,
					       optional_middle_stubs),
	},
	{
		.family_id = 2,
		.set_id = 1,
		.members = CTLV_SCHEMA_TREE(optional_leaf_nodes),
	},
};

static const struct ctlv_family_schema optional_schema = {
	.name = "ctlv-optional-index",
	.family_id = 3,
	.fills = optional_fills,
	.n_fills = ARRAY_SIZE(optional_fills),
};

static const struct ctlv_family optional_family = {
	.schema = &optional_schema,
	.parent = &ctlv_ctlv_family,
};

static void message_test_nested_fill_optionality_is_inherited(struct kunit *test)
{
	struct ctlv_msg message;
	struct ctlv_error error;
	struct msg msg;
	int ret;

	msg_init(&msg);
	ret = validate_indexed_family(&msg, &optional_base_tree,
				      &optional_family, &message, &error);
	KUNIT_EXPECT_EQ(test, ret, 0);
	ctlv_msg_index_free(&message);
}

static void message_test_contiguous_occurrences(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	/* A lower numeric ID may follow the repeated run. */
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				 &error));

	msg_init(&msg);
	/* Schema order is not wire order either. */
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				 &error));
}

static void message_test_a_split_occurrence_run(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 split;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	split = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_NONCONTIGUOUS_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, split, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, 0, error.nest_offset);
}

static void message_test_required_attr_precedes_a_split_run(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &precedence_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_MISSING_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, CTLV_FIXTURE_ATTR_SCALAR_U,
			error.missing_attr_id);
}

static void message_test_cardinality_precedes_a_split_run(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 excess;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 3);
	excess = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 4);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &precedence_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES,
			error.reason);
	KUNIT_EXPECT_EQ(test, excess, error.bad_attr_offset);
}

static void message_test_wire_error_precedes_a_split_run(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 bad;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	bad = msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		      CTLV_FIXTURE_MODE_AUTO + 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &precedence_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE,
			error.reason);
	KUNIT_EXPECT_EQ(test, bad, error.bad_attr_offset);

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	bad = msg_u64(&msg, ATTR_FOREIGN, 7);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &precedence_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_UNKNOWN_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, bad, error.bad_attr_offset);
}

static void message_test_an_unknown_attr_splits_a_run(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 split;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_ignorable(&msg, ATTR_FOREIGN, 7);
	split = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_NONCONTIGUOUS_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, split, error.bad_attr_offset);
}

static void message_test_a_nested_run_is_local_to_its_nest(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 nest;
	u32 split;

	msg_init(&msg);
	nest = msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_REQUEST);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	split = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 2);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &nested_run_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_NONCONTIGUOUS_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, split, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, nest, error.nest_offset);
}

static enum ctlv_step
message_step_must_not_run(const struct ctlv_attr *nest,
			  const struct ctlv_attr **child)
{
	KUNIT_FAIL(kunit_get_current_test(), "request accessor rescanned TLVs");
	return CTLV_STEP_BAD;
}

static void message_test_indexed_access_does_not_rescan(struct kunit *test)
{
	struct ctlv_input_scope *root_scope;
	struct ctlv_input_slot *few_slot;
	struct ctlv_cursor root, attr, nested;
	struct ctlv_msg message;
	struct ctlv_error error;
	struct msg msg;
	u32 first, mode;
	u32 n_few = 0;
	u32 n_nests = 0;
	u32 root_scope_index;
	u32 few_slot_index;
	int ret;

	msg_init(&msg);
	first = msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 11);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 22);
	mode = msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		       CTLV_FIXTURE_MODE_ENABLED);
	msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_REQUEST);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_S, 2);
	msg_nest_end(&msg);
	msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_REQUEST);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 3);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_S, 4);
	msg_nest_end(&msg);
	msg_ignorable(&msg, ATTR_FOREIGN, 7);

	ret = validate_indexed(&msg,
			       request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
			       &message, &error);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_NOT_NULL(test, message.index);
	KUNIT_ASSERT_EQ(test, message.index->n_nests, 3);
	root_scope_index = message.index->nests[0].first_scope;
	root_scope = &message.index->scopes[root_scope_index];
	KUNIT_ASSERT_EQ(test, root_scope->n_slots, 3);
	few_slot_index = root_scope->first_slot;
	few_slot = &message.index->slots[few_slot_index];
	KUNIT_EXPECT_EQ(test, few_slot->count, 2);
	KUNIT_EXPECT_EQ(test, few_slot->first_offset, first);
	KUNIT_EXPECT_EQ(test, few_slot->end_offset, mode);
	/* The ignored attr made neither a scope nor a slot. */
	KUNIT_EXPECT_EQ(test, message.index->n_scopes, 3);
	KUNIT_EXPECT_EQ(test, message.index->n_slots, 7);

	ctlv_msg_root(&root, &message);
	kunit_activate_static_stub(test, ctlv_msg_step,
				   message_step_must_not_run);

	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 0));
	while (ctlv_attr_ok(&attr)) {
		KUNIT_EXPECT_NE(test, ctlv_attr_u64(&attr), 0);
		n_few++;
		ctlv_attr_next(&attr);
	}
	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 1));
	KUNIT_EXPECT_EQ(test, ctlv_attr_u64(&attr),
			CTLV_FIXTURE_MODE_ENABLED);

	ctlv_input_attr(&nested, &root, input_ref(root.tree, root.node, 2));
	while (ctlv_attr_ok(&nested)) {
		ctlv_input_attr(&attr, &nested,
				input_ref(nested.tree, nested.node, 0));
		KUNIT_EXPECT_NE(test, ctlv_attr_u64(&attr), 0);
		n_nests++;
		ctlv_attr_next(&nested);
	}
	kunit_deactivate_static_stub(test, ctlv_msg_step);

	KUNIT_EXPECT_EQ(test, n_few, 2);
	KUNIT_EXPECT_EQ(test, n_nests, 2);
	ctlv_msg_index_free(&message);
}

static void message_test_optional_attrs_have_empty_slots(struct kunit *test)
{
	struct ctlv_cursor root, attr;
	struct ctlv_msg message;
	struct ctlv_error error;
	struct msg msg;
	int ret;

	msg_init(&msg);
	ret = validate_indexed(&msg,
			       request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
			       &message, &error);
	KUNIT_ASSERT_EQ(test, ret, 0);
	ctlv_msg_root(&root, &message);
	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 0));
	KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));
	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 1));
	KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));
	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 2));
	KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));
	ctlv_msg_index_free(&message);
}

static void message_test_an_attr_a_caller_can_do_without(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_ignorable(&msg, ATTR_FOREIGN, 7);
	msg_ignorable(&msg, ATTR_UNDECLARED, 7);
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, 0, error.reason);
}

/* Anywhere a nest may carry members, and not only at the root. */
static void message_test_an_attr_a_nest_can_do_without(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_nest_start(&msg, CTLV_FIXTURE_ATTR_NEST_LEVEL1);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	msg_ignorable(&msg, ATTR_FOREIGN, 7);
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP),
				 &error));
	KUNIT_EXPECT_EQ(test, 0, error.reason);
}

/* Without the bit the same attr is what it has always been: a refusal. */
static void message_test_an_attr_nobody_asked_to_ignore(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 unknown;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	unknown = msg_u64(&msg, ATTR_FOREIGN, 7);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_UNKNOWN_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, unknown, error.bad_attr_offset);
	KUNIT_EXPECT_EQ(test, 0, error.nest_offset);
}

/*
 * An ID this schema gives a meaning to somewhere is one a caller sent to the
 * wrong place, which is a mistake worth hearing about however forgiving the
 * caller offered to be.
 */
static void message_test_an_attr_declared_somewhere_else(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 misplaced;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	/* A member of the counts-get request, and of nothing else. */
	misplaced = msg_ignorable(&msg, CTLV_FIXTURE_ATTR_COUNT_FEW, 1);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_UNKNOWN_ATTR,
			error.reason);
	KUNIT_EXPECT_EQ(test, misplaced, error.bad_attr_offset);
}

/*
 * An ignored attr is stepped over as one object, so what it holds is whatever
 * the newer schema it came from said and none of this kernel's business.
 */
static void message_test_nothing_inside_an_ignored_attr(struct kunit *test)
{
	struct ctlv_attr garbage = { .attr_id = ATTR_UNDECLARED };
	struct ctlv_error error;
	struct msg msg;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	/* A child no walk could make sense of, in an attr that contains it. */
	msg_put(&msg, ATTR_FOREIGN,
		CTLV_ATTR_F_NESTED | CTLV_ATTR_F_IGNORE_IF_UNKNOWN, &garbage,
		sizeof(garbage));
	KUNIT_EXPECT_EQ(test, 0,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, 0, error.reason);
}

/*
 * Being forgiven is a caller's to ask for. An output says what this kernel
 * knows, so the bit means nothing there and is one it does not declare.
 */
static void message_test_the_bit_where_it_means_nothing(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 unknown;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	unknown = msg_ignorable(&msg, ATTR_FOREIGN, 7);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate_reply(&msg,
				       request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				       &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_UNKNOWN_FLAG,
			error.reason);
	KUNIT_EXPECT_EQ(test, unknown, error.bad_attr_offset);
}

/* A bit nothing declares, on an attr nothing declares either. */
static void message_test_a_bit_no_version_declares(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;
	u32 unknown;

	msg_init(&msg);
	msg_u64(&msg, CTLV_FIXTURE_ATTR_SCALAR_U, 1);
	unknown = msg_put(&msg, ATTR_FOREIGN,
			  CTLV_ATTR_F_IGNORE_IF_UNKNOWN | (1U << 3), NULL, 0);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, request_tree(CTLV_FIXTURE_OP_COLLAPSE_CHECK),
				 &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_UNKNOWN_FLAG,
			error.reason);
	KUNIT_EXPECT_EQ(test, unknown, error.bad_attr_offset);
}

/*
 * The generator refuses a schema that nests deeper than the framework allows,
 * so what the limit guards against is recursive metadata: this tree.
 */

static const struct ctlv_schema_node recursive_nodes[] = {
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
	},
	{
		.attr_id = CTLV_FIXTURE_ATTR_NEST_LEVEL1,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		/* Its own child, so the tree describes any depth at all. */
		.first_child = 1,
		.n_children = 1,
	},
};

static const struct ctlv_schema_tree recursive_tree =
	CTLV_SCHEMA_TREE(recursive_nodes);

/* One message of @levels nests below its root, each inside the last. */
static void nest_chain(struct msg *msg, u32 levels)
{
	u32 level;

	msg_init(msg);
	for (level = 0; level < levels; level++)
		msg_nest_start(msg, CTLV_FIXTURE_ATTR_NEST_LEVEL1);
}

static void message_test_as_deep_as_the_framework_allows(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	nest_chain(&msg, CTLV_MAX_NEST_DEPTH);
	KUNIT_EXPECT_EQ(test, 0, validate(&msg, &recursive_tree, &error));
}

static void message_test_one_nest_deeper(struct kunit *test)
{
	struct ctlv_error error;
	struct msg msg;

	nest_chain(&msg, CTLV_MAX_NEST_DEPTH + 1);
	KUNIT_EXPECT_EQ(test, -EINVAL,
			validate(&msg, &recursive_tree, &error));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_NESTING_TOO_DEEP,
			error.reason);
}

/*
 * No two cases here may hit the same warning: the second time a WARN_ON_ONCE
 * fires it is silent. The message is deliberately not one validation would
 * accept, these being checks on what happens when that assumption breaks.
 */

struct misuse {
	struct msg msg;
	struct ctlv_msg message;
	struct ctlv_cursor root;
	u32 size;
	u32 text;
};

static void message_index_free(void *arg)
{
	ctlv_msg_index_free(arg);
}

/* A validated scalars-set request, which a misuse case may corrupt later. */
static struct misuse *misuse_init(struct kunit *test)
{
	static const u8 fixed[8] = {};
	static const u8 bounded[4] = {};
	struct misuse *m = kunit_kzalloc(test, sizeof(*m), GFP_KERNEL);
	struct ctlv_error error = {};
	u8 flag = 0;

	KUNIT_ASSERT_NOT_NULL(test, m);
	msg_init(&m->msg);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_U, 7);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_S, 0);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_DELTA, 0);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_FREQ, 0);
	m->size = msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_SIZE, 0);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_MODE,
		CTLV_FIXTURE_MODE_ENABLED);
	msg_u64(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_QUIRKS, 0);
	msg_put(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_FLAG, 0, &flag,
		sizeof(flag));
	m->text = msg_put(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_TEXT, 0, "x", 2);
	msg_put(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_FIXED, 0, fixed,
		sizeof(fixed));
	msg_put(&m->msg, CTLV_FIXTURE_ATTR_SCALAR_BOUNDED, 0, bounded,
		sizeof(bounded));
	msg_done(&m->msg);

	m->message = (struct ctlv_msg){
		.root = msg_attr(&m->msg, 0),
		.len = m->msg.len,
		.tree = request_tree(CTLV_FIXTURE_OP_SCALARS_SET),
		.family = &message_fixture_family,
		.request = true,
	};
	KUNIT_ASSERT_NOT_NULL(test, m->message.tree);
	KUNIT_ASSERT_EQ(test, ctlv_schema_validate(&m->message, &error), 0);
	KUNIT_ASSERT_EQ(test,
			kunit_add_action_or_reset(test, message_index_free,
						  &m->message), 0);
	ctlv_msg_root(&m->root, &m->message);
	return m;
}

static void message_test_asking_for_what_is_not_placed_here(struct kunit *test)
{
	struct misuse *m = misuse_init(test);
	struct ctlv_cursor attr;
	struct ctlv_input_ref ref;

	kunit_warning_suppress(test) {
		/* No nest to look in. */
		ref = input_ref(m->root.tree, m->root.node, 0);
		ctlv_input_attr(&attr, NULL, ref);
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));

		/* An attr this op's schema places somewhere else. */
		ref = input_ref(request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
				0, 0);
		ctlv_input_attr(&attr, &m->root, ref);
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));

		/* And one it does place here, read as another form. */
		ref = input_ref(m->root.tree, m->root.node, 0);
		ref.kind = CTLV_SCHEMA_STRING;
		ctlv_input_attr(&attr, &m->root, ref);
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));

		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 3);
	}
}

static void message_test_reading_what_is_not_there(struct kunit *test)
{
	struct ctlv_cursor root, attr;
	struct ctlv_msg message;
	struct ctlv_error error;
	struct msg msg;
	u64 value;
	int ret;

	msg_init(&msg);
	ret = validate_indexed(&msg,
			       request_tree(CTLV_FIXTURE_OP_COUNTS_GET),
			       &message, &error);
	KUNIT_ASSERT_EQ(test, ret, 0);
	ctlv_msg_root(&root, &message);
	/* An attr the message does not carry is found without complaint. */
	ctlv_input_attr(&attr, &root, input_ref(root.tree, root.node, 1));
	KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));

	kunit_warning_suppress(test) {
		/* Reading it anyway is what _present() exists to prevent. */
		value = ctlv_attr_u64(&attr);
		KUNIT_EXPECT_EQ(test, value, 0);
		/* As is stepping from it. */
		ctlv_attr_next(&attr);
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&attr));

		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 2);
	}
	ctlv_msg_index_free(&message);
}

static void message_test_reading_one_form_as_another(struct kunit *test)
{
	struct misuse *m = misuse_init(test);
	struct ctlv_cursor attr;
	struct ctlv_cursor found;
	struct ctlv_cursor other;
	struct ctlv_cursor wrong;
	struct ctlv_input_ref ref;

	ctlv_input_attr(&attr, &m->root,
			input_ref(m->root.tree, m->root.node, 0));
	KUNIT_ASSERT_TRUE(test, ctlv_attr_ok(&attr));
	KUNIT_EXPECT_EQ(test, 7, ctlv_attr_u64(&attr));
	wrong = attr;
	wrong.node++;
	other = m->root;
	other.tree = request_tree(CTLV_FIXTURE_OP_COUNTS_GET);
	ref = input_ref(request_tree(CTLV_FIXTURE_OP_COUNTS_GET), 0, 0);

	kunit_warning_suppress(test) {
		/* A cursor's schema node and dense local slot are one binding. */
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&wrong));
		/* Another nest schema cannot address this root's index. */
		ctlv_input_attr(&found, &other, ref);
		KUNIT_EXPECT_FALSE(test, ctlv_attr_ok(&found));
		/* An unsigned payload read by the reader of a boolean. */
		KUNIT_EXPECT_FALSE(test, ctlv_attr_bool(&attr));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 3);
	}
}

/*
 * A payload no validation would admit. The accessors check it anyway, a wrong
 * answer there being one a family would act on.
 */
static void message_test_a_payload_validation_would_refuse(struct kunit *test)
{
	struct ctlv_cursor attr;
	struct misuse *m;
	const char *text;
	u64 value;

	m = misuse_init(test);
	msg_attr(&m->msg, m->size)->len = sizeof(struct ctlv_attr) + 3;
	memcpy(msg_attr(&m->msg, m->size) + 1, "abc", 3);
	memcpy(msg_attr(&m->msg, m->text) + 1, "xx", 2);

	kunit_warning_suppress(test) {
		/* An integer of half a payload. */
		ctlv_input_attr(&attr, &m->root,
				input_ref(m->root.tree, m->root.node, 4));
		KUNIT_EXPECT_EQ(test, m->size,
				ctlv_msg_offset(&m->message, attr.attr));
		value = ctlv_attr_u64(&attr);
		KUNIT_EXPECT_EQ(test, value, 0);

		/* And a string that is not one. */
		ctlv_input_attr(&attr, &m->root,
				input_ref(m->root.tree, m->root.node, 8));
		text = ctlv_attr_string(&attr);
		KUNIT_EXPECT_NULL(test, text);

		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 2);
	}
}

/*
 * The builder writes into an execution, not a buffer: the kind of op decides
 * whether running out of room ends a fragment or is a bug.
 */

/* Longer than any of these writes, so a length is the case's to set. */
#define OUT_LEN 512

struct out {
	struct ctlv_family family;
	struct ctlv_device ctlvdev;
	struct ctlv_file file;
	struct ctlv_op_schema schema;
	struct ctlv_op_entry entry;
	struct ctlv_op_ctx ctx;
	u64 buf[OUT_LEN / sizeof(u64)];
};

/*
 * One execution of an op of @type, answering through @tree into @len bytes,
 * with everything the builder reaches through the context set up.
 */
static struct out *out_init(struct kunit *test, u8 type,
			    const struct ctlv_schema_tree *tree, size_t len)
{
	struct out *o = kunit_kzalloc(test, sizeof(*o), GFP_KERNEL);

	KUNIT_ASSERT_NOT_NULL(test, o);
	KUNIT_ASSERT_NOT_NULL(test, tree);
	KUNIT_ASSERT_LE(test, len, sizeof(o->buf));

	o->family.schema = &ctlv_fixture_ctlv_schema;
	o->ctlvdev.family = &o->family;
	o->file.ctlvdev = &o->ctlvdev;
	o->schema.type = type;
	o->entry.schema = &o->schema;
	o->ctx.file = &o->file;
	o->ctx.op = &o->entry;

	/* Poisoned, so what a case finds written is what the builder wrote. */
	memset(o->buf, 0xa5, sizeof(o->buf));
	ctlv_msg_output(&o->ctx.output, &o->ctx, o->buf, len, tree);
	ctlv_msg_root(&o->ctx.output_root, &o->ctx.output);
	o->ctx.deepest = &o->ctx.output_root;
	return o;
}

static struct ctlv_cursor *out_root(struct out *o)
{
	return &o->ctx.output_root;
}

static u32 out_len(struct out *o)
{
	return o->ctx.output.root->len;
}

/* One attr of every form, at the bounds of what each declares. */
static void message_test_every_form_appended(struct kunit *test)
{
	static const u8 fixed[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
	struct ctlv_cursor *root;
	struct out *o;

	o = out_init(test, CTLV_OP_TYPE_ACTION,
		     request_tree(CTLV_FIXTURE_OP_SCALARS_SET), OUT_LEN);
	root = out_root(o);

	KUNIT_EXPECT_TRUE(test, ctlv_put_u64(root, CTLV_FIXTURE_ATTR_SCALAR_U,
					     CTLV_SCHEMA_UINT, 4096));
	KUNIT_EXPECT_TRUE(test, ctlv_put_s64(root, CTLV_FIXTURE_ATTR_SCALAR_S,
					     CTLV_SCHEMA_SINT, -100));
	KUNIT_EXPECT_TRUE(test, ctlv_put_u64(root,
					     CTLV_FIXTURE_ATTR_SCALAR_MODE,
					     CTLV_SCHEMA_ENUM,
					     CTLV_FIXTURE_MODE_AUTO));
	KUNIT_EXPECT_TRUE(test, ctlv_put_bool(root,
					      CTLV_FIXTURE_ATTR_SCALAR_FLAG,
					      CTLV_SCHEMA_BOOL, true));
	KUNIT_EXPECT_TRUE(test, ctlv_put_string(root,
						CTLV_FIXTURE_ATTR_SCALAR_TEXT,
						CTLV_SCHEMA_STRING,
						"fifteen-chars!!"));
	KUNIT_EXPECT_TRUE(test, ctlv_put_binary(root,
						CTLV_FIXTURE_ATTR_SCALAR_FIXED,
						CTLV_SCHEMA_BINARY, fixed,
						sizeof(fixed)));
	KUNIT_EXPECT_EQ(test, 0, o->ctx.output.poison);
	/* The root, five attrs of one aligned unit, and a string of two. */
	KUNIT_EXPECT_EQ(test, 16 + 5 * 24 + 32, out_len(o));
}

/* Padding is zeroed, so what was in that memory does not go out. */
static void message_test_padding_is_written_as_zeroes(struct kunit *test)
{
	const struct ctlv_attr *text;
	struct out *o;

	o = out_init(test, CTLV_OP_TYPE_ACTION,
		     request_tree(CTLV_FIXTURE_OP_SCALARS_SET), OUT_LEN);

	/* Five bytes of string, three of padding. */
	KUNIT_ASSERT_TRUE(test, ctlv_put_string(out_root(o),
						CTLV_FIXTURE_ATTR_SCALAR_TEXT,
						CTLV_SCHEMA_STRING, "four"));
	text = (const struct ctlv_attr *)((u8 *)o->buf + 16);
	KUNIT_EXPECT_EQ(test, sizeof(*text) + 5, text->len);
	KUNIT_EXPECT_EQ(test, 40, out_len(o));
	KUNIT_EXPECT_MEMEQ(test, (u8 *)o->buf + 37, "\0\0\0", 3);
}

/* A nest costs what holds it the whole of itself, up to the root. */
static void message_test_a_nest_and_what_it_costs(struct kunit *test)
{
	struct ctlv_cursor level1, level2;
	struct out *o;

	o = out_init(test, CTLV_OP_TYPE_QUERY,
		     reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP), OUT_LEN);

	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(out_root(o), &level1,
						CTLV_FIXTURE_ATTR_NEST_LEVEL1,
						CTLV_SCHEMA_NEST));
	KUNIT_EXPECT_EQ(test, 32, out_len(o));
	KUNIT_ASSERT_TRUE(test, ctlv_put_u64(&level1,
					     CTLV_FIXTURE_ATTR_SCALAR_U,
					     CTLV_SCHEMA_UINT, 1));
	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(&level1, &level2,
						CTLV_FIXTURE_ATTR_NEST_LEVEL2,
						CTLV_SCHEMA_NEST));
	KUNIT_ASSERT_TRUE(test, ctlv_put_u64(&level2,
					     CTLV_FIXTURE_ATTR_SCALAR_U,
					     CTLV_SCHEMA_UINT, 2));
	ctlv_nest_end(&level2);
	ctlv_nest_end(&level1);

	KUNIT_EXPECT_EQ(test, 0, o->ctx.output.poison);
	/* level1 holds its own header, an attr, and the whole of level2. */
	KUNIT_EXPECT_EQ(test, 16 + 24 + 40, o->ctx.output.root[1].len);
	KUNIT_EXPECT_EQ(test, 16 + 80, out_len(o));
}

/* The deepest cursor is what says whether a callback left a nest open. */
static void message_test_the_deepest_cursor_moves_with_the_nests(struct kunit *test)
{
	struct ctlv_cursor level1, level2;
	struct out *o;

	o = out_init(test, CTLV_OP_TYPE_QUERY,
		     reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP), OUT_LEN);
	KUNIT_EXPECT_PTR_EQ(test, out_root(o), o->ctx.deepest);

	/* An append does not move it, and opening a nest moves it inwards. */
	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(out_root(o), &level1,
						CTLV_FIXTURE_ATTR_NEST_LEVEL1,
						CTLV_SCHEMA_NEST));
	KUNIT_EXPECT_PTR_EQ(test, &level1, o->ctx.deepest);
	KUNIT_ASSERT_TRUE(test, ctlv_put_u64(&level1,
					     CTLV_FIXTURE_ATTR_SCALAR_U,
					     CTLV_SCHEMA_UINT, 1));
	KUNIT_EXPECT_PTR_EQ(test, &level1, o->ctx.deepest);

	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(&level1, &level2,
						CTLV_FIXTURE_ATTR_NEST_LEVEL2,
						CTLV_SCHEMA_NEST));
	KUNIT_EXPECT_PTR_EQ(test, &level2, o->ctx.deepest);

	/* And closing one moves it back out, one nest at a time. */
	ctlv_nest_end(&level2);
	KUNIT_EXPECT_PTR_EQ(test, &level1, o->ctx.deepest);
	ctlv_nest_end(&level1);
	KUNIT_EXPECT_PTR_EQ(test, out_root(o), o->ctx.deepest);
}

/*
 * A query is offered whatever room its caller had, so the end of it ends a
 * fragment. An action or an event was given what its schema allows, so the
 * end of one means the family wrote what that schema does not describe.
 */

static void message_test_an_append_that_does_not_fit_is_not_half_written(struct kunit *test)
{
	u64 before[OUT_LEN / sizeof(u64)];
	struct out *o;

	/* Room for the root and one attr, and two are appended. */
	o = out_init(test, CTLV_OP_TYPE_QUERY,
		     reply_tree(CTLV_FIXTURE_OP_COUNTS_GET), 40);
	KUNIT_ASSERT_TRUE(test, ctlv_put_u64(out_root(o),
					     CTLV_FIXTURE_ATTR_COUNT_MANY,
					     CTLV_SCHEMA_UINT, 1));
	memcpy(before, o->buf, sizeof(before));

	KUNIT_EXPECT_FALSE(test, ctlv_put_u64(out_root(o),
					      CTLV_FIXTURE_ATTR_COUNT_MANY,
					      CTLV_SCHEMA_UINT, 2));
	KUNIT_EXPECT_MEMEQ(test, before, o->buf, sizeof(before));
	KUNIT_EXPECT_EQ(test, 40, out_len(o));
}

static void message_test_a_query_that_runs_out(struct kunit *test)
{
	struct ctlv_cursor level1;
	struct out *o;

	o = out_init(test, CTLV_OP_TYPE_QUERY,
		     reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP), 40);
	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(out_root(o), &level1,
						CTLV_FIXTURE_ATTR_NEST_LEVEL1,
						CTLV_SCHEMA_NEST));

	/* The nest fit, its member does not, and the nest stays open. */
	KUNIT_EXPECT_FALSE(test, ctlv_put_u64(&level1,
					      CTLV_FIXTURE_ATTR_SCALAR_U,
					      CTLV_SCHEMA_UINT, 1));
	KUNIT_EXPECT_EQ(test, 0, o->ctx.output.poison);
	KUNIT_EXPECT_PTR_EQ(test, &level1, o->ctx.deepest);

	/* What has been written is still a message, and still growable. */
	ctlv_nest_end(&level1);
	KUNIT_EXPECT_EQ(test, 32, out_len(o));
	KUNIT_EXPECT_PTR_EQ(test, out_root(o), o->ctx.deepest);
}

/* A message of one attr, in a buffer with room for exactly that. */
static struct out *out_full(struct kunit *test, u8 type)
{
	struct out *o = out_init(test, type,
				 reply_tree(CTLV_FIXTURE_OP_COUNTS_GET), 40);

	KUNIT_ASSERT_TRUE(test, ctlv_put_u64(out_root(o),
					     CTLV_FIXTURE_ATTR_COUNT_MANY,
					     CTLV_SCHEMA_UINT, 1));
	return o;
}

static bool out_one_more(struct out *o)
{
	return ctlv_put_u64(out_root(o), CTLV_FIXTURE_ATTR_COUNT_MANY,
			    CTLV_SCHEMA_UINT, 2);
}

static void message_test_an_action_that_runs_out(struct kunit *test)
{
	struct out *o = out_full(test, CTLV_OP_TYPE_ACTION);

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_FALSE(test, out_one_more(o));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, -EMSGSIZE, o->ctx.output.poison);

	/* An abandoned message takes nothing more, without a word. */
	KUNIT_EXPECT_FALSE(test, out_one_more(o));
	KUNIT_EXPECT_EQ(test, 40, out_len(o));
}

/*
 * The same mistake for an event. The case above spent the WARN_ON_ONCE, so
 * what is left to see here is the poison.
 */
static void message_test_an_event_that_runs_out(struct kunit *test)
{
	struct out *o = out_full(test, CTLV_OP_TYPE_EVENT);

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_FALSE(test, out_one_more(o));
	}
	KUNIT_EXPECT_EQ(test, -EMSGSIZE, o->ctx.output.poison);
	KUNIT_EXPECT_EQ(test, 40, out_len(o));
}

/*
 * Each misuse warns, poisons the message and leaves the buffer as it was. One
 * misuse per message, because the first one abandons it.
 */

/* One warning, one poison, and not one byte written. */
#define EXPECT_MISUSE(_test, _o, _length, _call) do {			\
	kunit_warning_suppress(_test) {					\
		KUNIT_EXPECT_FALSE(_test, _call);			\
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(_test, 1);	\
	}								\
	KUNIT_EXPECT_EQ(_test, -EINVAL, (_o)->ctx.output.poison);	\
	KUNIT_EXPECT_EQ(_test, _length, out_len(_o));			\
} while (0)

static struct out *out_scalars(struct kunit *test)
{
	return out_init(test, CTLV_OP_TYPE_ACTION,
			request_tree(CTLV_FIXTURE_OP_SCALARS_SET), OUT_LEN);
}

static struct out *out_nests(struct kunit *test)
{
	return out_init(test, CTLV_OP_TYPE_ACTION,
			reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP), OUT_LEN);
}

static void message_test_writing_what_is_not_placed_here(struct kunit *test)
{
	struct ctlv_cursor level1;
	struct out *o;

	/* An attr this schema places somewhere else. */
	o = out_nests(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_put_u64(out_root(o), CTLV_FIXTURE_ATTR_SCALAR_U,
				   CTLV_SCHEMA_UINT, 1));

	/* And one it does place here, written as another form. */
	o = out_nests(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_nest_start(out_root(o), &level1,
				      CTLV_FIXTURE_ATTR_NEST_LEVEL1,
				      CTLV_SCHEMA_UINT));
}

static void message_test_writing_what_the_attr_does_not_allow(struct kunit *test)
{
	struct out *o;

	/* One to 4096 is what scalar.u declares. */
	o = out_scalars(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_put_u64(out_root(o), CTLV_FIXTURE_ATTR_SCALAR_U,
				   CTLV_SCHEMA_UINT, 4097));

	/* Eight bytes is the whole of what scalar.fixed is. */
	o = out_scalars(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_put_binary(out_root(o),
				      CTLV_FIXTURE_ATTR_SCALAR_FIXED,
				      CTLV_SCHEMA_BINARY, "abc", 3));

	/* A string longer than the bound scalar.text declares. */
	o = out_scalars(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_put_string(out_root(o),
				      CTLV_FIXTURE_ATTR_SCALAR_TEXT,
				      CTLV_SCHEMA_STRING, "sixteen chars!!!"));

	/* And no string at all. */
	o = out_scalars(test);
	EXPECT_MISUSE(test, o, 16,
		      ctlv_put_string(out_root(o),
				      CTLV_FIXTURE_ATTR_SCALAR_TEXT,
				      CTLV_SCHEMA_STRING, NULL));
}

/* A cursor from further out is one the builder has moved on from. */
static void message_test_writing_through_a_cursor_left_behind(struct kunit *test)
{
	struct ctlv_cursor level1, level2;
	struct out *o;

	o = out_nests(test);
	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(out_root(o), &level1,
						CTLV_FIXTURE_ATTR_NEST_LEVEL1,
						CTLV_SCHEMA_NEST));
	EXPECT_MISUSE(test, o, 32,
		      ctlv_nest_start(out_root(o), &level2,
				      CTLV_FIXTURE_ATTR_NEST_LEVEL1,
				      CTLV_SCHEMA_NEST));

	/* An abandoned message takes nothing more, without a word. */
	KUNIT_EXPECT_FALSE(test, ctlv_put_u64(&level1,
					      CTLV_FIXTURE_ATTR_SCALAR_U,
					      CTLV_SCHEMA_UINT, 1));
	KUNIT_EXPECT_EQ(test, 32, out_len(o));
}

static void message_test_ending_a_nest_that_is_not_open(struct kunit *test)
{
	struct ctlv_cursor level1;
	struct out *o;

	o = out_nests(test);
	KUNIT_ASSERT_TRUE(test, ctlv_nest_start(out_root(o), &level1,
						CTLV_FIXTURE_ATTR_NEST_LEVEL1,
						CTLV_SCHEMA_NEST));
	ctlv_nest_end(&level1);
	KUNIT_EXPECT_EQ(test, 0, o->ctx.output.poison);

	kunit_warning_suppress(test) {
		ctlv_nest_end(&level1);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, -EINVAL, o->ctx.output.poison);
	KUNIT_EXPECT_EQ(test, 32, out_len(o));
}

/*
 * What ending a fragment does to the message: the marks along the path it
 * stopped on, the cursor tracking reset, and the refusals below.
 */

/* A query answering through the fixture's nest chain, with room to spare. */
static struct out *out_dump(struct kunit *test)
{
	return out_init(test, CTLV_OP_TYPE_QUERY,
			reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP), OUT_LEN);
}

static bool out_more(struct out *o, u32 offset)
{
	const struct ctlv_attr *attr;

	attr = (const struct ctlv_attr *)((u8 *)o->buf + offset);
	return attr->flags & CTLV_ATTR_F_FRAG_MORE;
}

static bool out_nest(struct out *o, struct ctlv_cursor *nest, u64 attr_id,
		     struct ctlv_cursor *into)
{
	return ctlv_nest_start(nest, into, attr_id, CTLV_SCHEMA_NEST);
}

/*
 * A boundary is marked on the root and on every nest open at it, those being
 * the nests a continuation re-emits.
 */
static void message_test_a_boundary_marks_what_is_open(struct kunit *test)
{
	struct ctlv_cursor closed, level1, level2;
	struct out *o = out_dump(test);

	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &closed));
	ctlv_nest_end(&closed);
	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &level1));
	KUNIT_ASSERT_TRUE(test, out_nest(o, &level1,
					 CTLV_FIXTURE_ATTR_NEST_LEVEL2,
					 &level2));

	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_more(&o->ctx, 7, 42));
	KUNIT_EXPECT_TRUE(test, out_more(o, 0));
	KUNIT_EXPECT_FALSE(test, out_more(o, 16));
	KUNIT_EXPECT_TRUE(test, out_more(o, 32));
	KUNIT_EXPECT_TRUE(test, out_more(o, 48));
	/* A mark is not a member, so the message is as long as it was. */
	KUNIT_EXPECT_EQ(test, 64, out_len(o));

	KUNIT_EXPECT_EQ(test, 7, o->ctx.frag.reported);
	KUNIT_EXPECT_EQ(test, 42, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_PTR_EQ(test, out_root(o), o->ctx.deepest);
}

static void message_test_a_boundary_without_progress(struct kunit *test)
{
	struct ctlv_cursor level1, level2;
	struct out *o = out_dump(test);

	o->ctx.frag.generation = 7;
	o->ctx.frag.cursor = 9;
	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &level1));
	KUNIT_ASSERT_TRUE(test, out_nest(o, &level1,
					 CTLV_FIXTURE_ATTR_NEST_LEVEL2,
					 &level2));

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_query_more(&o->ctx, 7, 10));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));
	KUNIT_EXPECT_FALSE(test, out_more(o, 16));
	KUNIT_EXPECT_FALSE(test, out_more(o, 32));
}

static void message_test_the_fragment_that_completes_a_reply(struct kunit *test)
{
	struct out *o = out_dump(test);
	struct ctlv_cursor level1;

	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &level1));
	ctlv_nest_end(&level1);

	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_done(&o->ctx, 7));
	KUNIT_EXPECT_EQ(test, 7, o->ctx.frag.reported);
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));
	KUNIT_EXPECT_FALSE(test, out_more(o, 16));
}

/*
 * A fragment ending where its own invocation began, or nowhere at all, is a
 * reply that never ends. Either is refused before anything is marked.
 */
static void message_test_a_boundary_that_does_not_move(struct kunit *test)
{
	struct out *o = out_dump(test);

	o->ctx.frag.generation = 7;
	o->ctx.frag.cursor = 9;
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_query_more(&o->ctx, 7, 9));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));

	o = out_dump(test);
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_query_more(&o->ctx, 7, 0));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));
}

/* A reply that is complete has no nest left open, whatever it says. */
static void message_test_a_last_fragment_with_a_nest_open(struct kunit *test)
{
	struct out *o = out_dump(test);
	struct ctlv_cursor level1;

	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &level1));
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL, ctlv_op_query_done(&o->ctx, 7));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	/* That cursor is not followed, its frame being about to return. */
	KUNIT_EXPECT_PTR_EQ(test, out_root(o), o->ctx.deepest);
}

/* One invocation is one fragment, so it has one ending. */
static void message_test_a_fragment_ended_twice(struct kunit *test)
{
	struct out *o = out_dump(test);

	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_done(&o->ctx, 7));
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_query_more(&o->ctx, 7, 1));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, -EINVAL, o->ctx.frag.result);
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));
}

/*
 * A generation that has moved on is a race and not a bug, so nothing warns
 * and both generations are recorded for the caller.
 */
static void message_test_a_continuation_of_data_that_moved_on(struct kunit *test)
{
	struct out *o = out_dump(test);

	o->ctx.frag.generation = 7;
	o->ctx.frag.cursor = 9;
	KUNIT_EXPECT_EQ(test, -ESTALE, ctlv_op_query_more(&o->ctx, 8, 10));
	KUNIT_EXPECT_EQ(test, CTLV_VALIDATION_REASON_STALE_GENERATION,
			o->ctx.error.reason);
	KUNIT_EXPECT_EQ(test, 7, o->ctx.error.expected_generation);
	KUNIT_EXPECT_EQ(test, 8, o->ctx.error.current_generation);
	/* Nothing of it is delivered, so nothing of it is marked either. */
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.reported);
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));

	/* A fragment that would have been the last is as stale. */
	o = out_dump(test);
	o->ctx.frag.generation = 7;
	o->ctx.frag.cursor = 9;
	KUNIT_EXPECT_EQ(test, -ESTALE, ctlv_op_query_done(&o->ctx, 8));
}

/* A first fragment is told no cursor and no generation. */
static void message_test_the_first_fragment_of_a_reply(struct kunit *test)
{
	struct out *o = out_dump(test);
	struct ctlv_cursor level1;

	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_generation(&o->ctx));
	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_cursor(&o->ctx));
	KUNIT_ASSERT_TRUE(test, out_nest(o, out_root(o),
					 CTLV_FIXTURE_ATTR_NEST_LEVEL1,
					 &level1));
	ctlv_nest_end(&level1);
	KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_more(&o->ctx, 8, 10));
	KUNIT_EXPECT_EQ(test, 8, o->ctx.frag.reported);
	KUNIT_EXPECT_EQ(test, 10, o->ctx.frag.next_cursor);

	/* Where a continuation of it is told both. */
	o = out_dump(test);
	o->ctx.frag.generation = 8;
	o->ctx.frag.cursor = 10;
	KUNIT_EXPECT_EQ(test, 8, ctlv_op_query_generation(&o->ctx));
	KUNIT_EXPECT_EQ(test, 10, ctlv_op_query_cursor(&o->ctx));
}

/* Only a query is answered in fragments, so each of these is a bug. */
static void message_test_continuation_asked_about_an_action(struct kunit *test)
{
	struct out *o = out_init(test, CTLV_OP_TYPE_ACTION,
				 reply_tree(CTLV_FIXTURE_OP_NESTS_DUMP),
				 OUT_LEN);

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_generation(&o->ctx));
		KUNIT_EXPECT_EQ(test, 0, ctlv_op_query_cursor(&o->ctx));
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_query_more(&o->ctx, 1, 2));
		KUNIT_EXPECT_EQ(test, -EINVAL, ctlv_op_query_done(&o->ctx, 1));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 4);
	}
	KUNIT_EXPECT_EQ(test, 0, o->ctx.frag.next_cursor);
	KUNIT_EXPECT_FALSE(test, out_more(o, 0));
}

static void message_test_family_error_message(struct kunit *test)
{
	struct ctlv_op_ctx ctx = {};
	char long_message[CTLV_ERROR_MESSAGE_LEN + 32];

	KUNIT_EXPECT_EQ(test, -EBUSY,
			ctlv_op_error(&ctx, -EBUSY, "first failure"));
	KUNIT_EXPECT_STREQ(test, ctx.error.message, "first failure");
	KUNIT_EXPECT_EQ(test, -EINVAL,
			ctlv_op_errorf(&ctx, -EINVAL, "later failure %u", 2));
	KUNIT_EXPECT_STREQ(test, ctx.error.message, "first failure");

	ctx = (struct ctlv_op_ctx){};
	KUNIT_EXPECT_EQ(test, -ERANGE,
			ctlv_op_errorf(&ctx, -ERANGE, "value %u", 7));
	KUNIT_EXPECT_STREQ(test, ctx.error.message, "value 7");

	memset(long_message, 'x', sizeof(long_message));
	long_message[sizeof(long_message) - 1] = '\0';
	ctx = (struct ctlv_op_ctx){};
	KUNIT_EXPECT_EQ(test, -E2BIG,
			ctlv_op_error(&ctx, -E2BIG, long_message));
	KUNIT_EXPECT_EQ(test, strlen(ctx.error.message),
			CTLV_ERROR_MESSAGE_LEN - 1);

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, -EINVAL,
				ctlv_op_error(&ctx, 0, "not an error"));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
}

static struct kunit_case message_test_cases[] = {
	KUNIT_CASE(message_test_a_message_it_accepts),
	KUNIT_CASE(message_test_a_missing_attr_is_named),
	KUNIT_CASE(message_test_a_bad_attr_is_located),
	KUNIT_CASE(message_test_a_nest_is_located_too),
	KUNIT_CASE(message_test_the_occurrence_too_many),
	KUNIT_CASE(message_test_a_malformed_nest),
	KUNIT_CASE(message_test_nested_fill_optionality_is_inherited),
	KUNIT_CASE(message_test_contiguous_occurrences),
	KUNIT_CASE(message_test_a_split_occurrence_run),
	KUNIT_CASE(message_test_required_attr_precedes_a_split_run),
	KUNIT_CASE(message_test_cardinality_precedes_a_split_run),
	KUNIT_CASE(message_test_wire_error_precedes_a_split_run),
	KUNIT_CASE(message_test_an_unknown_attr_splits_a_run),
	KUNIT_CASE(message_test_a_nested_run_is_local_to_its_nest),
	KUNIT_CASE(message_test_indexed_access_does_not_rescan),
	KUNIT_CASE(message_test_optional_attrs_have_empty_slots),
	KUNIT_CASE(message_test_an_attr_a_caller_can_do_without),
	KUNIT_CASE(message_test_an_attr_a_nest_can_do_without),
	KUNIT_CASE(message_test_an_attr_nobody_asked_to_ignore),
	KUNIT_CASE(message_test_an_attr_declared_somewhere_else),
	KUNIT_CASE(message_test_nothing_inside_an_ignored_attr),
	KUNIT_CASE(message_test_the_bit_where_it_means_nothing),
	KUNIT_CASE(message_test_a_bit_no_version_declares),
	KUNIT_CASE(message_test_as_deep_as_the_framework_allows),
	KUNIT_CASE(message_test_one_nest_deeper),
	KUNIT_CASE(message_test_asking_for_what_is_not_placed_here),
	KUNIT_CASE(message_test_reading_what_is_not_there),
	KUNIT_CASE(message_test_reading_one_form_as_another),
	KUNIT_CASE(message_test_a_payload_validation_would_refuse),
	KUNIT_CASE(message_test_every_form_appended),
	KUNIT_CASE(message_test_padding_is_written_as_zeroes),
	KUNIT_CASE(message_test_a_nest_and_what_it_costs),
	KUNIT_CASE(message_test_the_deepest_cursor_moves_with_the_nests),
	KUNIT_CASE(message_test_an_append_that_does_not_fit_is_not_half_written),
	KUNIT_CASE(message_test_a_query_that_runs_out),
	KUNIT_CASE(message_test_an_action_that_runs_out),
	KUNIT_CASE(message_test_an_event_that_runs_out),
	KUNIT_CASE(message_test_writing_what_is_not_placed_here),
	KUNIT_CASE(message_test_writing_what_the_attr_does_not_allow),
	KUNIT_CASE(message_test_writing_through_a_cursor_left_behind),
	KUNIT_CASE(message_test_ending_a_nest_that_is_not_open),
	KUNIT_CASE(message_test_a_boundary_marks_what_is_open),
	KUNIT_CASE(message_test_a_boundary_without_progress),
	KUNIT_CASE(message_test_the_fragment_that_completes_a_reply),
	KUNIT_CASE(message_test_a_boundary_that_does_not_move),
	KUNIT_CASE(message_test_a_last_fragment_with_a_nest_open),
	KUNIT_CASE(message_test_a_fragment_ended_twice),
	KUNIT_CASE(message_test_a_continuation_of_data_that_moved_on),
	KUNIT_CASE(message_test_the_first_fragment_of_a_reply),
	KUNIT_CASE(message_test_continuation_asked_about_an_action),
	KUNIT_CASE(message_test_family_error_message),
	{}
};

static struct kunit_suite message_test_suite = {
	.name = "ctlv_message",
	.test_cases = message_test_cases,
};

kunit_test_suite(message_test_suite);
