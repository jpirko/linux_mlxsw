// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * libctlv and generated-binding tests across base and inheriting families.
 */
#include <stdbool.h>
#include <sys/mman.h>

#include "../kselftest_harness.h"
#include "ctlv_module.h"
#include "ctlv_tlv.h"
#include "ctlv_test_child_user_gen.h"
#include "internal.h"

/* Where a comparison of two schemas writes what it disagreed about. */
#define DIFF_LEN 512

/* As much of one of the module's logs as there can be. */
#define LOG_LEN 1024

/* Long enough for a record to arrive on a machine under load. */
#define EVENT_WAIT_MS 5000

/* Long enough to be sure none did, without stalling a run. */
#define EVENT_NONE_MS 100

static const char *why(const struct ctlv_error *error)
{
	static char text[DIFF_LEN];

	return ctlv_error_str(error, text, sizeof(text));
}

/*
 * FNV-1a, as the transfer op folds every byte it reads. Order-sensitive, so the
 * checksum says which blob the kernel read first and not only that it read it.
 */
#define FNV_OFFSET 0xcbf29ce484222325ULL
#define FNV_PRIME 0x100000001b3ULL

static __u64 fold(__u64 sum, const void *buf, size_t len)
{
	const __u8 *bytes = buf;
	size_t i;

	for (i = 0; i < len; i++)
		sum = (sum ^ bytes[i]) * FNV_PRIME;
	return sum;
}

TEST(schema_render_stays_within_its_buffer)
{
	struct ctlv_node_schema node = {
		.attr_id = 0xffffffffffffffffULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 12345,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_MAX_LENGTH |
			   CTLV_NODE_SCHEMA_HAS_FLAGS_MASK | CTLV_NODE_SCHEMA_HAS_UNIT |
			   CTLV_NODE_SCHEMA_HAS_BLOB | CTLV_NODE_SCHEMA_HAS_REENTERS,
		.min = 1,
		.max = 2,
		.min_length = 3,
		.max_length = 4,
		.flags_mask = 5,
		.unit = CTLV_UNIT_BYTES,
		.scale = -6,
		.blob_direction = CTLV_BLOB_DIRECTION_INOUT,
		.blob_backings = 7,
		.blob_max_length = 8,
		.blob_alignment = 4096,
		.reenters = 1,
	};
	unsigned char storage[18];
	size_t at, len;

	ctlv_schema_render(&node, 0, NULL, 0);
	for (len = 0; len <= sizeof(storage) - 2; len++) {
		memset(storage, 0xa5, sizeof(storage));
		storage[0] = 0x12;
		storage[sizeof(storage) - 1] = 0x34;

		ctlv_schema_render(&node, 0, (char *)&storage[1], len);

		EXPECT_EQ(0x12, storage[0]);
		EXPECT_EQ(0x34, storage[sizeof(storage) - 1]);
		if (len)
			EXPECT_NE(NULL, memchr(&storage[1], '\0', len));
		for (at = len; at < sizeof(storage) - 2; at++)
			EXPECT_EQ(0xa5, storage[at + 1]);
	}
}

TEST(schema_comparison_bounds_a_maximum_depth_path)
{
	const unsigned int count = CTLV_MAX_NEST_DEPTH;
	struct ctlv_node_schema *expected, *actual;
	char diff[1024] = "";
	unsigned int i;

	expected = calloc(count, sizeof(*expected));
	actual = calloc(count, sizeof(*actual));
	ASSERT_NE(NULL, expected);
	ASSERT_NE(NULL, actual);

	for (i = 0; i < count; i++) {
		expected[i].attr_id = i + 1;
		expected[i].kind = CTLV_SCHEMA_NEST;
		expected[i].max_occurrences = 1;
		if (i + 1 < count) {
			expected[i].optional_attrs = &expected[i + 1];
			expected[i].optional_attrs_count = 1;
		}
		actual[i] = expected[i];
		if (i)
			actual[i - 1].optional_attrs = &actual[i];
	}
	expected[count - 1].kind = CTLV_SCHEMA_UINT;
	actual[count - 1].kind = CTLV_SCHEMA_SINT;

	EXPECT_FALSE(ctlv_schema_equal(expected, actual, diff, sizeof(diff)));
	EXPECT_NE(NULL, strstr(diff, "kind"));
	free(actual);
	free(expected);
}

static int bootstrap_validate(struct ctlv_tlv *tlv, __u64 op_id)
{
	__u32 len = ctlv_tlv_done(tlv);

	if (tlv->overflow)
		return -EOVERFLOW;
	return ctlv_bootstrap_validate(tlv->buf, len, op_id);
}

static void schema_description(struct ctlv_tlv *tlv, unsigned int depth)
{
	unsigned int i;

	ctlv_tlv_init(tlv);
	for (i = 0; i < depth; i++) {
		ctlv_tlv_nest_start(tlv, i ? CTLV_ATTR_SCHEMA_ATTRS :
				    CTLV_ATTR_SCHEMA_REQUEST);
		ctlv_tlv_u64(tlv, CTLV_ATTR_SCHEMA_ATTR_ID, i);
		ctlv_tlv_u64(tlv, CTLV_ATTR_SCHEMA_KIND,
			     CTLV_SCHEMA_NEST);
		ctlv_tlv_u64(tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	}
	ctlv_tlv_done(tlv);
}

TEST(introspection_rejects_a_scalar_schema_node)
{
	struct ctlv_schema *schema = NULL;
	struct ctlv_tlv tlv;
	__u32 scalar;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_REQUEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 0);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_NEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	scalar = ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_ATTRS);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_UINT);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_attr(&tlv, scalar)->flags = 0;
	ctlv_tlv_done(&tlv);

	EXPECT_EQ(-EPROTO,
		  ctlv_schema_parse((const struct ctlv_attr *)tlv.buf,
				    tlv.len, &schema));
	EXPECT_EQ(NULL, schema);
}

TEST(introspection_rejects_invalid_schema_forms)
{
	struct ctlv_schema *schema = NULL;
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_REQUEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 0);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_NEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_ATTRS);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_FLAGS);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_FLAGS_MASK, 0);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);

	EXPECT_EQ(-EPROTO,
		  ctlv_schema_parse((const struct ctlv_attr *)tlv.buf,
				    tlv.len, &schema));
	EXPECT_EQ(NULL, schema);

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_REQUEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 0);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_NEST);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_ATTRS);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_BOOL);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_ATTR_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_BOOL);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);

	EXPECT_EQ(-EPROTO,
		  ctlv_schema_parse((const struct ctlv_attr *)tlv.buf,
				    tlv.len, &schema));
	EXPECT_EQ(NULL, schema);
}

TEST(introspection_enforces_requiredness_and_cardinality)
{
	struct ctlv_ops *ops = NULL;
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_FAMILY_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_FAMILY_ID, 0);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(-EPROTO,
		  bootstrap_validate(&tlv, CTLV_OP_FAMILIES_DUMP));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_ACTION);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);
	EXPECT_EQ(-EPROTO,
		  ctlv_ops_parse((const struct ctlv_attr *)tlv.buf,
				 tlv.len, 0, &ops));
	EXPECT_EQ(NULL, ops);

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_ACTION);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_FD_ACCESS, CTLV_OP_FD_ACCESS_READ);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_AVAILABILITY,
		     CTLV_OP_AVAILABILITY_FIXED);
	ctlv_tlv_bool(&tlv, CTLV_ATTR_OP_DISABLED, true);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_REQUEST_MAX_LEN, 16);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN, 0);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);
	EXPECT_EQ(-EPROTO,
		  ctlv_ops_parse((const struct ctlv_attr *)tlv.buf,
				 tlv.len, 0, &ops));
	EXPECT_EQ(NULL, ops);

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 2);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_EVENT);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, 16);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(-EPROTO, bootstrap_validate(&tlv, CTLV_OP_OPS_DUMP));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_EVENT);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, 16);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_EVENT);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, 16);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);
	EXPECT_EQ(-EPROTO,
		  ctlv_ops_parse((const struct ctlv_attr *)tlv.buf,
				 tlv.len, 0, &ops));
	EXPECT_EQ(NULL, ops);
}

TEST(introspection_accepts_unknown_output)
{
	const __u64 unknown_attr = CTLV_ATTR(UINT16_MAX, 1, 1);
	struct ctlv_ops *ops = NULL;
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_FAMILY_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_FAMILY_ID, 0);
	ctlv_tlv_str(&tlv, CTLV_ATTR_FAMILY_NAME, "ctlv");
	ctlv_tlv_u64(&tlv, unknown_attr, 1);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, bootstrap_validate(&tlv, CTLV_OP_FAMILIES_DUMP));

	/* An old reader also tolerates an ID newly placed in this output. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_FAMILY_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_FAMILY_ID, 0);
	ctlv_tlv_str(&tlv, CTLV_ATTR_FAMILY_NAME, "ctlv");
	ctlv_tlv_u64(&tlv, CTLV_ATTR_ERROR_REASON, 1);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, bootstrap_validate(&tlv, CTLV_OP_FAMILIES_DUMP));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_EVENT);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, 16);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_FAMILY_ID, 0);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, bootstrap_validate(&tlv, CTLV_OP_OPS_DUMP));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	EXPECT_EQ(0, bootstrap_validate(&tlv, CTLV_OP_OP_SCHEMA_GET));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_EVENT + 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_nest_start(&tlv, CTLV_ATTR_OP_ENTRY);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, 2);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_TYPE, CTLV_OP_TYPE_ACTION);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_FD_ACCESS,
		     CTLV_OP_FD_ACCESS_READ_WRITE + 1);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_AVAILABILITY,
		     CTLV_OP_AVAILABILITY_DYNAMIC + 1);
	ctlv_tlv_bool(&tlv, CTLV_ATTR_OP_DISABLED, false);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_REQUEST_MAX_LEN, 16);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN, 0);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);
	ASSERT_EQ(0, ctlv_ops_parse((const struct ctlv_attr *)tlv.buf,
				    tlv.len, 7, &ops));
	ASSERT_NE(NULL, ops);
	ASSERT_EQ(2, ops->ops_count);
	EXPECT_EQ(CTLV_OP_TYPE_EVENT + 1, ops->ops[0].type);
	EXPECT_EQ(CTLV_OP_FD_ACCESS_READ_WRITE + 1,
		  ops->ops[1].fd_access);
	EXPECT_EQ(CTLV_OP_AVAILABILITY_DYNAMIC + 1,
		  ops->ops[1].availability);
	EXPECT_EQ(7, ops->generation);
	ctlv_ops_free(ops);
}

TEST(introspection_bounds_recursive_schemas)
{
	struct ctlv_schema *schema = NULL;
	struct ctlv_tlv tlv;

	schema_description(&tlv, CTLV_MAX_NEST_DEPTH);
	ASSERT_EQ(0, ctlv_schema_parse((const struct ctlv_attr *)tlv.buf,
				       tlv.len, &schema));
	ASSERT_NE(NULL, schema);
	EXPECT_EQ(CTLV_MAX_NEST_DEPTH, schema->node_schemas_count);
	ctlv_schema_free(schema);
	schema = NULL;

	schema_description(&tlv, CTLV_MAX_NEST_DEPTH + 1);
	EXPECT_EQ(-ELOOP,
		  ctlv_schema_parse((const struct ctlv_attr *)tlv.buf,
				    tlv.len, &schema));
	EXPECT_EQ(NULL, schema);
}

#define FRAG_OUTER_ID 0x0001000100000001ULL
#define FRAG_INNER_ID 0x0001000200000001ULL
#define FRAG_OTHER_ID 0x0001000300000001ULL
#define FRAG_VALUE_ID 0x0001000400000001ULL

static const struct ctlv_attr *
fragment_path(struct ctlv_tlv *tlv, __u64 outer_id, __u64 inner_id,
	      bool inner_nested, bool more)
{
	__u32 outer, inner;

	ctlv_tlv_init(tlv);
	outer = ctlv_tlv_nest_start(tlv, outer_id);
	if (inner_nested) {
		inner = ctlv_tlv_nest_start(tlv, inner_id);
		ctlv_tlv_u64(tlv, FRAG_VALUE_ID, 1);
		ctlv_tlv_nest_end(tlv);
	} else {
		inner = ctlv_tlv_u64(tlv, inner_id, 1);
	}
	ctlv_tlv_nest_end(tlv);
	ctlv_tlv_done(tlv);
	if (more) {
		ctlv_tlv_attr(tlv, 0)->flags |= CTLV_ATTR_F_FRAG_MORE;
		ctlv_tlv_attr(tlv, outer)->flags |= CTLV_ATTR_F_FRAG_MORE;
		ctlv_tlv_attr(tlv, inner)->flags |= CTLV_ATTR_F_FRAG_MORE;
	}
	return (const struct ctlv_attr *)tlv->buf;
}

static int fragment_seed(struct ctlv_msg *out, struct ctlv_frag_path *path)
{
	struct ctlv_tlv tlv;
	const struct ctlv_attr *root;

	root = fragment_path(&tlv, FRAG_OUTER_ID, FRAG_INNER_ID, true, true);
	return ctlv_frag_splice(out, root, path, NULL);
}

TEST(a_valid_continuation_repeats_the_open_path)
{
	struct ctlv_frag_path path = {};
	struct ctlv_msg *out = NULL;
	struct ctlv_tlv tlv;
	const struct ctlv_attr *root;
	bool progress;

	ASSERT_EQ(0, ctlv_msg_new(64, &out));
	ASSERT_EQ(0, fragment_seed(out, &path));
	ASSERT_EQ(2, path.depth);
	root = fragment_path(&tlv, FRAG_OUTER_ID, FRAG_INNER_ID, true, false);
	EXPECT_EQ(0, ctlv_frag_splice(out, root, &path, &progress));
	EXPECT_TRUE(progress);
	EXPECT_EQ(0, path.depth);
	EXPECT_EQ(1, out->depth);
	EXPECT_EQ(0, ctlv_msg_done(out));
	EXPECT_EQ(0, ctlv_msg_check(out->buf, out->len));
	ctlv_msg_free(out);
}

TEST(a_continuation_rejects_a_different_or_scalar_path)
{
	static const struct {
		__u64 outer_id;
		__u64 inner_id;
		bool inner_nested;
	} cases[] = {
		{ FRAG_OTHER_ID, FRAG_INNER_ID, true },
		{ FRAG_OUTER_ID, FRAG_OTHER_ID, true },
		{ FRAG_OUTER_ID, FRAG_INNER_ID, false },
	};
	unsigned int i;

	for (i = 0; i < CTLV_ARRAY_SIZE(cases); i++) {
		struct ctlv_frag_path path = {};
		struct ctlv_msg *out = NULL;
		struct ctlv_tlv tlv;
		const struct ctlv_attr *root;

		ASSERT_EQ(0, ctlv_msg_new(64, &out));
		ASSERT_EQ(0, fragment_seed(out, &path));
		root = fragment_path(&tlv, cases[i].outer_id,
				     cases[i].inner_id,
				     cases[i].inner_nested, false);
		EXPECT_EQ(-EPROTO,
			  ctlv_frag_splice(out, root, &path, NULL));
		EXPECT_EQ(2, path.depth);
		ctlv_msg_free(out);
	}
}

TEST(a_continuation_rejects_a_truncated_or_excess_path)
{
	struct ctlv_frag_path path = {};
	struct ctlv_msg *out = NULL;
	struct ctlv_tlv tlv;
	const struct ctlv_attr *root;

	ASSERT_EQ(0, ctlv_msg_new(64, &out));
	ASSERT_EQ(0, fragment_seed(out, &path));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, FRAG_OUTER_ID);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_done(&tlv);
	root = (const struct ctlv_attr *)tlv.buf;
	EXPECT_EQ(-EPROTO, ctlv_frag_splice(out, root, &path, NULL));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, FRAG_OUTER_ID);
	ctlv_tlv_nest_start(&tlv, FRAG_OTHER_ID);
	ctlv_tlv_nest_start(&tlv, FRAG_INNER_ID);
	ctlv_tlv_u64(&tlv, FRAG_VALUE_ID, 2);
	ctlv_tlv_done(&tlv);
	root = (const struct ctlv_attr *)tlv.buf;
	EXPECT_EQ(-EPROTO, ctlv_frag_splice(out, root, &path, NULL));

	EXPECT_EQ(2, path.depth);
	ctlv_msg_free(out);
}

TEST(continuation_markers_follow_the_open_path)
{
	struct ctlv_frag_path path = {};
	struct ctlv_msg *out = NULL;
	struct ctlv_tlv tlv;
	const struct ctlv_attr *root;
	__u32 outer, inner;

	ASSERT_EQ(0, ctlv_msg_new(64, &out));
	ASSERT_EQ(0, fragment_seed(out, &path));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_nest_start(&tlv, FRAG_OUTER_ID);
	inner = ctlv_tlv_nest_start(&tlv, FRAG_INNER_ID);
	ctlv_tlv_u64(&tlv, FRAG_VALUE_ID, 2);
	ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, 0)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctlv_tlv_attr(&tlv, inner)->flags |= CTLV_ATTR_F_FRAG_MORE;
	root = (const struct ctlv_attr *)tlv.buf;
	EXPECT_EQ(-EPROTO, ctlv_frag_splice(out, root, &path, NULL));

	ctlv_tlv_init(&tlv);
	outer = ctlv_tlv_nest_start(&tlv, FRAG_OUTER_ID);
	ctlv_tlv_nest_start(&tlv, FRAG_INNER_ID);
	ctlv_tlv_u64(&tlv, FRAG_VALUE_ID, 3);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_u64(&tlv, FRAG_VALUE_ID, 4);
	ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, 0)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctlv_tlv_attr(&tlv, outer)->flags |= CTLV_ATTR_F_FRAG_MORE;

	root = (const struct ctlv_attr *)tlv.buf;
	EXPECT_EQ(-EPROTO, ctlv_frag_splice(out, root, &path, NULL));
	EXPECT_EQ(2, path.depth);
	ctlv_msg_free(out);
}

TEST(a_continuation_of_only_wrappers_makes_no_progress)
{
	struct ctlv_frag_path path = {};
	struct ctlv_msg *out = NULL;
	struct ctlv_tlv tlv;
	const struct ctlv_attr *root;
	__u32 outer, inner;
	bool progress = true;

	ASSERT_EQ(0, ctlv_msg_new(64, &out));
	ASSERT_EQ(0, fragment_seed(out, &path));

	ctlv_tlv_init(&tlv);
	outer = ctlv_tlv_nest_start(&tlv, FRAG_OUTER_ID);
	inner = ctlv_tlv_nest_start(&tlv, FRAG_INNER_ID);
	ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, 0)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctlv_tlv_attr(&tlv, outer)->flags |= CTLV_ATTR_F_FRAG_MORE;
	ctlv_tlv_attr(&tlv, inner)->flags |= CTLV_ATTR_F_FRAG_MORE;

	root = (const struct ctlv_attr *)tlv.buf;
	EXPECT_EQ(0, ctlv_frag_splice(out, root, &path, &progress));
	EXPECT_FALSE(progress);
	EXPECT_EQ(2, path.depth);
	ctlv_msg_free(out);
}

TEST(query_guard_rejects_no_progress_and_cursor_cycles)
{
	struct ctlv_query_guard guard = {};

	EXPECT_EQ(-EPROTO, ctlv_query_guard_next(&guard, 1, false));
	EXPECT_EQ(0, guard.cursors_count);

	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 1, true));
	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 2, true));
	EXPECT_EQ(-ELOOP, ctlv_query_guard_next(&guard, 1, true));
	ctlv_query_guard_free(&guard);

	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 1, true));
	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 2, true));
	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 3, true));
	EXPECT_EQ(-ELOOP, ctlv_query_guard_next(&guard, 1, true));
	ctlv_query_guard_free(&guard);

	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 0, false));
	ctlv_query_guard_free(&guard);
}

TEST(query_guard_accepts_a_long_finite_reply)
{
	struct ctlv_query_guard guard = {};
	unsigned int cursor;

	for (cursor = 1; cursor <= 10000; cursor++)
		ASSERT_EQ(0, ctlv_query_guard_next(&guard, cursor, true));
	EXPECT_EQ(10000, guard.cursors_count);
	EXPECT_EQ(0, ctlv_query_guard_next(&guard, 0, false));
	ctlv_query_guard_free(&guard);

	guard.cursors_count = CTLV_QUERY_FRAGMENTS_MAX - 1;
	EXPECT_EQ(-E2BIG, ctlv_query_guard_next(&guard, 1, true));
	ctlv_query_guard_free(&guard);
}

/* A sealed memory file, the one backing every blob attr allows. */
static int memfd_new(size_t len)
{
	int fd = memfd_create("ctlv-blob", MFD_CLOEXEC | MFD_ALLOW_SEALING);

	if (fd < 0)
		return -errno;
	if (ftruncate(fd, len) ||
	    fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL)) {
		close(fd);
		return -errno;
	}
	return fd;
}

FIXTURE(device) {
	struct ctlv_dev *dev;
	struct ctlv_error error;
};

FIXTURE_VARIANT(device) {
	const char *family;
	const struct ctlv_family_spec *spec;
	/* The other family's, which no device of this one answers as. */
	const struct ctlv_family_spec *other;
	/* How many families a device of this one resolves through. */
	unsigned int chain_len;
	/* Whether the inheriting family's own op is one of this family's. */
	bool inheriting;
	/* What dynamic-noop answers before anything has toggled it. */
	bool dynamic_disabled;
};

/* The family that declares the extension point and leaves it empty. */
FIXTURE_VARIANT_ADD(device, parent) {
	.family = "ctlv-test",
	.spec = &ctlv_test_spec,
	.other = &ctlv_test_child_spec,
	.chain_len = 2,
	.inheriting = false,
	.dynamic_disabled = true,
};

/* And the one that fills it, so every message touching it is longer. */
FIXTURE_VARIANT_ADD(device, child) {
	.family = "ctlv-test-child",
	.spec = &ctlv_test_child_spec,
	.other = &ctlv_test_spec,
	.chain_len = 3,
	.inheriting = true,
	.dynamic_disabled = false,
};

FIXTURE_SETUP(device)
{
	int ret;

	self->dev = NULL;
	ret = ctlv_open(variant->family, 0, O_RDWR, &self->dev);
	if (ret)
		SKIP(return, "no %s device: %s", variant->family,
		     strerror(-ret));
}

FIXTURE_TEARDOWN(device)
{
	if (self->dev)
		ctlv_close(self->dev);
	/* A case that ended early must not leave the module rearranged. */
	ctlv_instances_set(1);
}

/* Flips dynamic-noop on one device, the change every event here reports. */
static int toggle(struct ctlv_dev *dev, struct ctlv_error *error)
{
	return ctlv_test_dynamic_toggle(dev, NULL, NULL, error);
}

/* What executing dynamic-noop answers in one state. */
static int dynamic_result(bool disabled)
{
	return disabled ? -EOPNOTSUPP : 0;
}

TEST_F(device, the_chain_a_device_belongs_to)
{
	unsigned int level;

	ASSERT_EQ(variant->chain_len, ctlv_chain_len(self->dev));
	EXPECT_STREQ("ctlv", ctlv_chain_name(self->dev, 0));
	EXPECT_EQ(CTLV_FAMILY_ROOT, ctlv_chain_id(self->dev, 0));

	/* The device family is the one it answers as. */
	EXPECT_STREQ(variant->family, ctlv_dev_family(self->dev));
	EXPECT_STREQ(variant->family,
		     ctlv_chain_name(self->dev, variant->chain_len - 1));
	EXPECT_EQ(variant->spec->family_id,
		  ctlv_chain_id(self->dev, variant->chain_len - 1));
	EXPECT_STREQ(variant->spec->name, ctlv_dev_family(self->dev));

	/* Every family of the chain is inherited, and nothing outside it. */
	for (level = 0; level < variant->chain_len; level++)
		EXPECT_TRUE(ctlv_dev_inherits(self->dev,
					      ctlv_chain_name(self->dev,
							      level)));
	EXPECT_TRUE(ctlv_dev_inherits(self->dev, "ctlv-test"));
	EXPECT_EQ(variant->inheriting,
		  ctlv_dev_inherits(self->dev, "ctlv-test-child"));
	EXPECT_FALSE(ctlv_dev_inherits(self->dev, "ctlv-nothing"));
}

/* The only check that program and kernel were generated from one revision. */
TEST_F(device, the_specification_this_program_was_built_from)
{
	char diff[DIFF_LEN] = "";

	ASSERT_EQ(0, ctlv_spec_check(self->dev, variant->spec, diff,
				     sizeof(diff), &self->error)) {
		TH_LOG("%s", diff[0] ? diff : why(&self->error));
	}
}

/* An ancestor's binding drives such a device, but its lengths differ. */
TEST_F(device, a_specification_of_another_family)
{
	char diff[DIFF_LEN] = "";

	EXPECT_EQ(-EINVAL, ctlv_spec_check(self->dev, variant->other, diff,
					   sizeof(diff), NULL));
	EXPECT_NE('\0', diff[0]);
}

/* The negative control: the comparison above would notice a drifted kernel. */
TEST_F(device, a_length_the_device_reports_differently)
{
	struct ctlv_family_spec spec = *variant->spec;
	struct ctlv_op_spec ops[16];
	char diff[DIFF_LEN] = "";
	unsigned int i;

	ASSERT_GE(ARRAY_SIZE(ops), spec.ops_count);
	memcpy(ops, spec.ops, spec.ops_count * sizeof(*ops));
	for (i = 0; i < spec.ops_count; i++)
		if (ops[i].op_id == CTLV_TEST_OP_ECHO)
			ops[i].request_max_len += 8;
	spec.ops = ops;

	EXPECT_EQ(-ENOMSG, ctlv_spec_check(self->dev, &spec, diff,
					   sizeof(diff), NULL));
	EXPECT_NE(NULL, strstr(diff, "request-max-len")) {
		TH_LOG("%s", diff);
	}
}

/* The same for the trees, where a placement fewer is a different op. */
TEST_F(device, a_placement_the_device_reports_differently)
{
	struct ctlv_family_spec spec = *variant->spec;
	struct ctlv_node_schema root;
	struct ctlv_op_spec ops[16];
	char diff[DIFF_LEN] = "";
	unsigned int i;

	ASSERT_GE(ARRAY_SIZE(ops), spec.ops_count);
	memcpy(ops, spec.ops, spec.ops_count * sizeof(*ops));
	for (i = 0; i < spec.ops_count; i++) {
		if (ops[i].op_id != CTLV_TEST_OP_ECHO)
			continue;
		root = *ops[i].request;
		ASSERT_LT(0, root.optional_attrs_count);
		root.optional_attrs_count--;
		ops[i].request = &root;
	}
	spec.ops = ops;

	EXPECT_EQ(-ENOMSG, ctlv_spec_check(self->dev, &spec, diff,
					   sizeof(diff), NULL));
	EXPECT_NE(NULL, strstr(diff, "request")) {
		TH_LOG("%s", diff);
	}
}

/* The first node of @node's subtree that re-enters a level, or NULL. */
static const struct ctlv_node_schema *
reentering(const struct ctlv_node_schema *node)
{
	const struct ctlv_node_schema *found;
	unsigned int i;

	if (node->reenters)
		return node;
	for (i = 0; i < node->attrs_count; i++) {
		found = reentering(&node->attrs[i]);
		if (found)
			return found;
	}
	for (i = 0; i < node->optional_attrs_count; i++) {
		found = reentering(&node->optional_attrs[i]);
		if (found)
			return found;
	}
	return NULL;
}

/*
 * The op that describes an op, describing itself: a reader following members
 * alone would never reach the end of it, so a node says how far up they are.
 */
TEST_F(device, a_tree_that_reenters_a_level_it_has_already_described)
{
	const struct ctlv_node_schema *reply, *node;
	struct ctlv_schema *schema;

	ASSERT_EQ(0, ctlv_op_schema_get(self->dev, CTLV_OP_OP_SCHEMA_GET,
					&schema, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	EXPECT_NE(NULL, ctlv_schema_request(schema));
	reply = ctlv_schema_reply(schema);
	ASSERT_NE(NULL, reply);

	node = reentering(reply);
	ASSERT_NE(NULL, node);
	EXPECT_EQ(CTLV_SCHEMA_NEST, node->kind);
	EXPECT_TRUE(node->present & CTLV_NODE_SCHEMA_HAS_REENTERS);
	/* Its members are the ones a level above, so it lists none itself. */
	EXPECT_EQ(1, node->reenters);
	EXPECT_EQ(0, node->attrs_count);
	EXPECT_EQ(0, node->optional_attrs_count);
	ctlv_schema_free(schema);
}

/* An optional op may be absent; treating it as mandatory exposes the drift. */
TEST_F(device, only_optional_ops_may_be_absent)
{
	struct ctlv_family_spec required = *variant->spec;
	struct ctlv_op_spec required_ops[16];
	struct ctlv_dev *narrowed = NULL;
	char diff[DIFF_LEN] = "";
	struct ctlv_ops *ops;
	bool found = false;
	unsigned int i;
	int ret;

	/* The next device of each family gives two ops up for good. */
	ASSERT_EQ(0, ctlv_instances_set_narrowed(2));
	ret = ctlv_open(variant->family, 1, O_RDWR, &narrowed);
	ASSERT_EQ(0, ret) {
		TH_LOG("second %s device: %s", variant->family,
		       strerror(-ret));
	}

	EXPECT_EQ(0, ctlv_spec_check(narrowed, variant->spec, diff,
				     sizeof(diff), NULL)) {
		TH_LOG("%s", diff);
	}

	ASSERT_EQ(0, ctlv_ops_dump(narrowed, &ops, NULL));
	EXPECT_EQ(NULL, ctlv_ops_find(ops, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(NULL, ctlv_ops_find(ops, CTLV_TEST_OP_DYNAMIC_TOGGLE));
	EXPECT_NE(NULL, ctlv_ops_find(ops, CTLV_TEST_OP_ECHO));
	ctlv_ops_free(ops);

	ASSERT_GE(ARRAY_SIZE(required_ops), required.ops_count);
	memcpy(required_ops, required.ops,
	       required.ops_count * sizeof(*required_ops));
	for (i = 0; i < required.ops_count; i++) {
		if (required_ops[i].op_id != CTLV_TEST_OP_DYNAMIC_NOOP)
			continue;
		required_ops[i].optional = false;
		found = true;
	}
	ASSERT_TRUE(found);
	required.ops = required_ops;
	EXPECT_EQ(-ENOMSG, ctlv_spec_check(narrowed, &required, diff,
					   sizeof(diff), NULL));
	EXPECT_NE(NULL, strstr(diff, "mandatory")) {
		TH_LOG("%s", diff);
	}

	EXPECT_EQ(-EOPNOTSUPP, ctlv_test_dynamic_noop(narrowed, NULL, NULL));
	ctlv_close(narrowed);
}

TEST_F(device, a_mandatory_core_op_may_not_be_absent)
{
	struct ctlv_family_spec chain[3], *root;
	struct ctlv_op_spec ops[16];
	const struct ctlv_family_spec *walk;
	char diff[DIFF_LEN] = "";
	unsigned int n = 0;

	for (walk = variant->spec; walk; walk = walk->parent) {
		ASSERT_LT(n, ARRAY_SIZE(chain));
		chain[n] = *walk;
		if (n)
			chain[n - 1].parent = &chain[n];
		n++;
	}
	ASSERT_EQ(variant->chain_len, n);
	root = &chain[n - 1];
	ASSERT_GE(ARRAY_SIZE(ops), root->ops_count + 1);
	memcpy(ops, root->ops, root->ops_count * sizeof(*ops));
	ops[root->ops_count] = (struct ctlv_op_spec){
		.op_id = CTLV_OP(CTLV_FAMILY_ROOT, UINT32_MAX),
	};
	root->ops = ops;
	root->ops_count++;

	EXPECT_EQ(-ENOMSG, ctlv_spec_check(self->dev, &chain[0], diff,
					   sizeof(diff), NULL));
	EXPECT_NE(NULL, strstr(diff, "mandatory")) {
		TH_LOG("%s", diff);
	}
}

TEST_F(device, an_action_that_answers_with_nothing)
{
	struct ctlv_msg *request;

	/* No request either, both buffers being the caller's to leave out. */
	ASSERT_EQ(0, ctlv_test_noop(self->dev, NULL, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}

	/* And an empty one, which an op with no attrs admits. */
	ASSERT_EQ(0, ctlv_test_noop_request_new(&request));
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(0, ctlv_test_noop(self->dev, request, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);
}

TEST_F(device, an_action_only_the_inheriting_family_has)
{
	int expected = variant->inheriting ? 0 : -EOPNOTSUPP;

	EXPECT_EQ(expected, ctlv_test_child_extra_noop(self->dev, NULL,
						       &self->error));
}

/* An op a device takes away and gives back, and the action that does it. */
TEST_F(device, an_op_whose_availability_changes)
{
	bool disabled = variant->dynamic_disabled;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;

	EXPECT_EQ(dynamic_result(disabled),
		  ctlv_test_dynamic_noop(self->dev, NULL, NULL));

	/* Its reply is empty, but room for one is checked before the flip. */
	ASSERT_EQ(0, ctlv_test_dynamic_toggle_request_new(&request));
	ctlv_test_probe_token_put(request, 9);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_dynamic_toggle(self->dev, request, &reply,
					      &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	EXPECT_FALSE(ctlv_test_probe_token_present(ctlv_reply_root(reply)));
	ctlv_reply_free(reply);

	EXPECT_EQ(dynamic_result(!disabled),
		  ctlv_test_dynamic_noop(self->dev, NULL, NULL));

	/* Back where it started, whichever state that was. */
	ASSERT_EQ(0, ctlv_test_dynamic_toggle(self->dev, request, NULL,
					      &self->error));
	EXPECT_EQ(dynamic_result(disabled),
		  ctlv_test_dynamic_noop(self->dev, NULL, NULL));
	ctlv_msg_free(request);
}

/* A query whose answer fits in one fragment, which is most of them. */
TEST_F(device, a_query_answered_whole)
{
	const struct ctlv_attr *extension;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	__u64 token = 0, extra = 0;

	ASSERT_EQ(0, ctlv_test_probe_get_request_new(&request));
	ctlv_test_probe_token_put(request, 7);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_test_probe_get(self->dev, request, &reply,
					 &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	EXPECT_EQ(1, ctlv_reply_fragments(reply));
	EXPECT_TRUE(ctlv_test_probe_token_get(ctlv_reply_root(reply), &token));
	EXPECT_EQ(7, token);

	/* The nest is on both families and this op fills it on neither. */
	extension = ctlv_test_probe_extension_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, extension);
	EXPECT_FALSE(ctlv_test_child_child_extra_get(extension, &extra));
	ctlv_reply_free(reply);
}

/* How much of a series to answer with, and to hold the reply against. */
#define SERIES_CHUNKS 8
#define SERIES_ITEMS 8
#define SERIES_VALUES 8
#define SERIES_TOTALS 8

/*
 * An answer no buffer holds. Reassembly hides the boundaries: a nest one fell
 * inside is one nest here, with every occurrence in the order it was written.
 */
TEST_F(device, a_query_answered_in_fragments)
{
	const struct ctlv_attr *chunk, *item, *value, *total;
	__u64 chunks = 0, items, values, totals = 0, index = 0;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;

	ASSERT_EQ(0, ctlv_test_series_dump_request_new(&request));
	ctlv_test_series_chunks_put(request, SERIES_CHUNKS);
	ctlv_test_series_items_put(request, SERIES_ITEMS);
	ctlv_test_series_values_put(request, SERIES_VALUES);
	ctlv_test_series_totals_put(request, SERIES_TOTALS);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_test_series_dump(self->dev, request, &reply,
					   &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	EXPECT_LT(1, ctlv_reply_fragments(reply));
	EXPECT_EQ(ctlv_generation_read(), ctlv_reply_generation(reply));

	ctlv_test_series_for_each_chunk(chunk, ctlv_reply_root(reply)) {
		chunks++;
		EXPECT_TRUE(ctlv_test_series_index_get(chunk, &index));
		EXPECT_EQ(chunks, index);
		items = 0;
		ctlv_test_series_for_each_item(item, chunk) {
			items++;
			EXPECT_TRUE(ctlv_test_series_index_get(item, &index));
			EXPECT_EQ(items, index);
			values = 0;
			ctlv_test_series_for_each_value(value, item) {
				__u64 got = 0;

				values++;
				EXPECT_TRUE(ctlv_test_series_value_get(value,
								       &got));
				EXPECT_EQ(chunks * 100 + items * 10 + values,
					  got);
			}
			EXPECT_EQ(SERIES_VALUES, values);
		}
		EXPECT_EQ(SERIES_ITEMS, items);
	}
	EXPECT_EQ(SERIES_CHUNKS, chunks);

	/* The array beside the outermost of the three, after all of them. */
	ctlv_test_series_for_each_total(total, ctlv_reply_root(reply)) {
		__u64 got = 0;

		totals++;
		EXPECT_TRUE(ctlv_test_series_total_get(total, &got));
		EXPECT_EQ(totals, got);
	}
	EXPECT_EQ(SERIES_TOTALS, totals);
	ctlv_reply_free(reply);
}

/*
 * One request carrying every form an attr can take, at both cardinalities and
 * all three nesting depths. The two cases below differ only in the answer.
 */
#define ECHO_U 4096
#define ECHO_S (-100)
#define ECHO_TEXT "echo"
#define ECHO_LABEL "first"
#define ECHO_EXTRA 42

static const __u8 echo_data[] = { 0xde, 0xad, 0xbe, 0xef };

static void echo_build(struct ctlv_msg *request, int page, bool child)
{
	/* Eight bytes of anything, aligned as the placement insists. */
	static const __u64 region = 0x0123456789abcdefULL;
	__u64 count;

	ctlv_test_echo_u_put(request, ECHO_U);
	ctlv_test_echo_s_put(request, ECHO_S);
	ctlv_test_echo_flag_put(request, true);
	ctlv_test_echo_mode_put(request, CTLV_TEST_ECHO_MODE_AUTO);
	ctlv_test_echo_quirks_put(request, CTLV_TEST_ECHO_QUIRK_SLOW |
					   CTLV_TEST_ECHO_QUIRK_NOISY);
	ctlv_test_echo_text_put(request, ECHO_TEXT);
	ctlv_test_echo_data_put(request, echo_data, sizeof(echo_data));
	for (count = 1; count <= 3; count++)
		ctlv_test_echo_count_put(request, count);

	/* One nest carrying the nest inside it, and one carrying neither. */
	ctlv_test_echo_item_start(request);
	ctlv_test_item_index_put(request, 1);
	ctlv_test_item_label_put(request, ECHO_LABEL);
	ctlv_test_item_leaf_start(request);
	ctlv_test_item_index_put(request, 2);
	ctlv_nest_end(request);
	ctlv_nest_end(request);
	ctlv_test_echo_item_start(request);
	ctlv_test_item_index_put(request, 3);
	ctlv_nest_end(request);

	/* Descriptors, whose arrival is all this op reports of them. */
	ctlv_test_echo_region_put_va(request, &region, sizeof(region));
	ctlv_test_echo_page_put_memfd(request, page, 0, 4096);

	/* The nest is the parent family's and its member the child's. */
	ctlv_test_probe_extension_start(request);
	if (child)
		ctlv_test_child_child_extra_put(request, ECHO_EXTRA);
	ctlv_nest_end(request);
}

/* Parent and filling families log only the attrs each owns. */
static const char *const echo_report[] = {
	"u=4096", "s=-100", "flag=1", "mode=3", "quirks=0x3", "text=echo",
	"data=deadbeef", "count=1", "count=2", "count=3", "item.index=1",
	"item.label=first", "leaf.index=2", "item.index=3", "region", "page",
	"extension",
};

static void echo_expected(char *out, size_t len)
{
	unsigned int i;
	size_t at = 0;

	for (i = 0; i < ARRAY_SIZE(echo_report) && at < len; i++)
		at += snprintf(out + at, len - at, "%s%s", i ? " " : "",
			       echo_report[i]);
}

TEST_F(device, every_form_an_attr_can_take)
{
	char expected[LOG_LEN], actual[LOG_LEN];
	struct ctlv_msg *request;
	int page;

	page = memfd_new(4096);
	ASSERT_LE(0, page) {
		TH_LOG("memfd: %s", strerror(-page));
	}
	ASSERT_EQ(0, ctlv_test_echo_request_new(&request));
	echo_build(request, page, variant->inheriting);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_echo_clear());
	ASSERT_EQ(0, ctlv_test_echo(self->dev, request, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);
	close(page);

	echo_expected(expected, sizeof(expected));
	ASSERT_EQ(0, ctlv_echo_read(actual, sizeof(actual)));
	EXPECT_STREQ(expected, actual);

	/* And the one member of the extension point, where it was read. */
	if (!variant->inheriting || !ctlv_child_logs_available())
		return;
	ASSERT_EQ(0, ctlv_child_echo_read(actual, sizeof(actual)));
	EXPECT_STREQ("child.extra=42", actual);
}

/* One generated request reaches devices with and without the child fill. */
TEST_F(device, an_attr_a_device_may_never_have_heard_of)
{
	struct ctlv_msg *request;

	ASSERT_EQ(0, ctlv_test_echo_request_new(&request));
	ctlv_test_echo_u_put(request, ECHO_U);
	ctlv_test_probe_extension_start(request);
	ctlv_test_child_child_extra_put(request, ECHO_EXTRA);
	ASSERT_EQ(0, ctlv_msg_ignore_if_unknown(request));
	ctlv_nest_end(request);
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(0, ctlv_test_echo(self->dev, request, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	/* Unmarked, it is refused wherever the ID means nothing. */
	ASSERT_EQ(0, ctlv_test_echo_request_new(&request));
	ctlv_test_echo_u_put(request, ECHO_U);
	ctlv_test_probe_extension_start(request);
	ctlv_test_child_child_extra_put(request, ECHO_EXTRA);
	ctlv_nest_end(request);
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(variant->inheriting ? 0 : -EINVAL,
		  ctlv_test_echo(self->dev, request, &self->error));
	ctlv_msg_free(request);
}

/* The same request answered rather than logged: serializers against getters. */
TEST_F(device, an_answer_read_back_through_the_parsers)
{
	const struct ctlv_attr *root, *item, *leaf, *count, *extension;
	__u64 u = 0, mode = 0, quirks = 0, unknown = 1, index = 0;
	__u64 extra = 0, seen = 0;
	const char *text = NULL, *label = NULL;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	const void *data = NULL;
	bool flag = false, known = false;
	__u32 data_len = 0;
	__s64 s = 0;
	int page;

	page = memfd_new(4096);
	ASSERT_LE(0, page) {
		TH_LOG("memfd: %s", strerror(-page));
	}
	ASSERT_EQ(0, ctlv_test_echo_reply_request_new(&request));
	echo_build(request, page, variant->inheriting);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_test_echo_reply(self->dev, request, &reply,
					  &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);
	close(page);
	root = ctlv_reply_root(reply);

	EXPECT_TRUE(ctlv_test_echo_u_get(root, &u));
	EXPECT_EQ(ECHO_U, u);
	EXPECT_TRUE(ctlv_test_echo_s_get(root, &s));
	EXPECT_EQ(ECHO_S, s);
	EXPECT_TRUE(ctlv_test_echo_flag_get(root, &flag));
	EXPECT_TRUE(flag);
	EXPECT_TRUE(ctlv_test_echo_mode_get(root, &mode, &known));
	EXPECT_EQ(CTLV_TEST_ECHO_MODE_AUTO, mode);
	EXPECT_TRUE(known);
	EXPECT_TRUE(ctlv_test_echo_quirks_get(root, &quirks, &unknown));
	EXPECT_EQ(CTLV_TEST_ECHO_QUIRK_SLOW | CTLV_TEST_ECHO_QUIRK_NOISY,
		  quirks);
	EXPECT_EQ(0, unknown);
	EXPECT_TRUE(ctlv_test_echo_text_get(root, &text));
	EXPECT_STREQ(ECHO_TEXT, text);
	EXPECT_TRUE(ctlv_test_echo_data_get(root, &data, &data_len));
	ASSERT_EQ(sizeof(echo_data), data_len);
	EXPECT_EQ(0, memcmp(echo_data, data, data_len));

	/* Three occurrences of one attr, in the order they were sent. */
	ctlv_test_echo_for_each_count(count, root) {
		__u64 got = 0;

		seen++;
		EXPECT_TRUE(ctlv_test_echo_count_get(count, &got));
		EXPECT_EQ(seen, got);
	}
	EXPECT_EQ(3, seen);

	/* And two of a nest, one carrying the nest inside it. */
	seen = 0;
	ctlv_test_echo_for_each_item(item, root) {
		seen++;
		EXPECT_TRUE(ctlv_test_item_index_get(item, &index));
		if (seen == 1) {
			EXPECT_EQ(1, index);
			EXPECT_TRUE(ctlv_test_item_label_get(item, &label));
			EXPECT_STREQ(ECHO_LABEL, label);
			leaf = ctlv_test_item_leaf_get(item);
			ASSERT_NE(NULL, leaf);
			EXPECT_TRUE(ctlv_test_item_index_get(leaf, &index));
			EXPECT_EQ(2, index);
		} else {
			EXPECT_EQ(3, index);
			EXPECT_FALSE(ctlv_test_item_label_present(item));
			EXPECT_FALSE(ctlv_test_item_leaf_present(item));
		}
	}
	EXPECT_EQ(2, seen);

	/* What the device family added to its parent's extension point. */
	extension = ctlv_test_probe_extension_get(root);
	ASSERT_NE(NULL, extension);
	EXPECT_EQ(variant->inheriting,
		  ctlv_test_child_child_extra_get(extension, &extra));
	if (variant->inheriting)
		EXPECT_EQ(ECHO_EXTRA, extra);
	ctlv_reply_free(reply);
}

/*
 * The device folds every readable byte into a checksum and writes a pattern
 * over every writable one, so the answer and the buffers are both oracles.
 */
#define BLOB_LEN 4096
#define PIECE_LEN 64
#define TRANSFER_FILL 0xa5

TEST_F(device, memory_a_request_describes)
{
	/* Aligned as every placement here insists, being what a u64 is. */
	static __u64 source[BLOB_LEN / sizeof(__u64)];
	static __u64 piece[2][PIECE_LEN / sizeof(__u64)];
	__u8 *bytes = (__u8 *)source;
	__u8 got[BLOB_LEN];
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	__u64 sum = 0, written = 0;
	bool distinct = false;
	unsigned int i;
	int target;

	for (i = 0; i < BLOB_LEN; i++)
		bytes[i] = i;
	memset(piece[0], 0x40, PIECE_LEN);
	memset(piece[1], 0x80, PIECE_LEN);

	target = memfd_new(BLOB_LEN);
	ASSERT_LE(0, target) {
		TH_LOG("memfd: %s", strerror(-target));
	}

	ASSERT_EQ(0, ctlv_test_transfer_request_new(&request));
	ctlv_test_transfer_source_put_va(request, source, BLOB_LEN);
	ctlv_test_transfer_target_put_memfd(request, target, 0, BLOB_LEN);
	ctlv_test_transfer_piece_put_va(request, piece[0], PIECE_LEN);
	ctlv_test_transfer_piece_put_va(request, piece[1], PIECE_LEN);
	ctlv_test_transfer_fill_put(request, TRANSFER_FILL);
	ctlv_test_transfer_twice_put(request, true);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_test_transfer(self->dev, request, &reply,
					&self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	/* In read order: source before pieces, each piece where appended. */
	EXPECT_TRUE(ctlv_test_transfer_sum_get(ctlv_reply_root(reply), &sum));
	EXPECT_EQ(fold(fold(fold(FNV_OFFSET, source, BLOB_LEN),
			    piece[0], PIECE_LEN),
		       piece[1], PIECE_LEN), sum);
	EXPECT_TRUE(ctlv_test_transfer_written_get(ctlv_reply_root(reply),
						   &written));
	EXPECT_EQ(BLOB_LEN, written);

	/* Resolving one descriptor twice is two objects, not one shared. */
	EXPECT_TRUE(ctlv_test_transfer_distinct_get(ctlv_reply_root(reply),
						    &distinct));
	EXPECT_TRUE(distinct);
	ctlv_reply_free(reply);

	ASSERT_EQ(BLOB_LEN, pread(target, got, BLOB_LEN, 0));
	for (i = 0; i < BLOB_LEN; i++)
		ASSERT_EQ(TRANSFER_FILL, got[i]) {
			TH_LOG("byte %u", i);
		}
	close(target);
}

/* Descriptors valid and never resolved: the attr is absent, not zero. */
TEST_F(device, memory_a_request_describes_and_leaves_alone)
{
	static __u64 source[BLOB_LEN / sizeof(__u64)];
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	__u64 sum = 0;

	ASSERT_EQ(0, ctlv_test_transfer_request_new(&request));
	ctlv_test_transfer_source_put_va(request, source, BLOB_LEN);
	ctlv_test_transfer_skip_put(request, true);
	ASSERT_EQ(0, ctlv_msg_err(request));

	ASSERT_EQ(0, ctlv_test_transfer(self->dev, request, &reply,
					&self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	EXPECT_FALSE(ctlv_test_transfer_sum_present(ctlv_reply_root(reply)));
	EXPECT_FALSE(ctlv_test_transfer_sum_get(ctlv_reply_root(reply), &sum));
	ctlv_reply_free(reply);
}

/* One burst, and what each record of it carries. */
#define BURST 3
#define PAYLOAD 64
#define SERIAL 100

TEST_F(device, a_burst_of_records)
{
	struct ctlv_event *event;
	struct ctlv_msg *request;
	unsigned int i, j;

	/* Nothing arrives unasked, so both events are asked for. */
	ASSERT_EQ(0, ctlv_test_beeped_subscribe(self->dev, &self->error));
	ASSERT_EQ(0, ctlv_test_pinged_subscribe(self->dev, &self->error));

	ASSERT_EQ(0, ctlv_test_ping_request_new(&request));
	ctlv_test_ping_count_put(request, BURST);
	ctlv_test_ping_serial_put(request, SERIAL);
	ctlv_test_ping_payload_put(request, PAYLOAD);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_ping(self->dev, request, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ctlv_msg_free(request);

	for (i = 0; i < BURST; i++) {
		const void *filler = NULL;
		const __u8 *bytes;
		__u64 serial = 0;
		__u32 len = 0;

		ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
		ASSERT_EQ(0, ctlv_event_read(self->dev, &event));

		/* Which event it is, and which of the burst. */
		EXPECT_TRUE(ctlv_test_beeped_is(event));
		EXPECT_EQ(i + 1, ctlv_event_sequence(event));
		EXPECT_TRUE(ctlv_test_ping_serial_get(ctlv_event_root(event),
						      &serial));
		EXPECT_EQ(SERIAL + i, serial);

		/* Whose filler is the position of each of its bytes. */
		EXPECT_TRUE(ctlv_test_ping_filler_get(ctlv_event_root(event),
						      &filler, &len));
		ASSERT_EQ(PAYLOAD, len);
		bytes = filler;
		for (j = 0; j < len; j++)
			ASSERT_EQ((__u8)j, bytes[j]) {
				TH_LOG("record %u byte %u", i, j);
			}
		ctlv_event_free(event);
	}

	/* Nothing else was queued by a burst of exactly that many. */
	EXPECT_EQ(-ETIMEDOUT, ctlv_event_wait(self->dev, EVENT_NONE_MS));
}

/* The record that carries nothing at all, and one that never happened. */
TEST_F(device, a_record_with_nothing_in_it_and_one_that_was_not_emitted)
{
	struct ctlv_event *event;
	struct ctlv_msg *request;
	__u64 serial = 0;

	ASSERT_EQ(0, ctlv_test_beeped_subscribe(self->dev, &self->error));
	ASSERT_EQ(0, ctlv_test_pinged_subscribe(self->dev, &self->error));

	ASSERT_EQ(0, ctlv_test_ping_request_new(&request));
	ctlv_test_ping_empty_put(request, true);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_ping(self->dev, request, &self->error));
	ctlv_msg_free(request);

	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	EXPECT_TRUE(ctlv_test_pinged_is(event));
	EXPECT_FALSE(ctlv_test_ping_serial_get(ctlv_event_root(event),
					       &serial));
	ctlv_event_free(event);

	/* An aborted burst is one a reader sees nothing of. */
	ASSERT_EQ(0, ctlv_test_ping_request_new(&request));
	ctlv_test_ping_count_put(request, BURST);
	ctlv_test_ping_abort_put(request, true);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_ping(self->dev, request, &self->error));
	ctlv_msg_free(request);
	EXPECT_EQ(-ETIMEDOUT, ctlv_event_wait(self->dev, EVENT_NONE_MS));

	/* As is a burst nobody is subscribed to any more. */
	ASSERT_EQ(0, ctlv_test_pinged_unsubscribe(self->dev, &self->error));
	ASSERT_EQ(0, ctlv_test_ping_request_new(&request));
	ctlv_test_ping_empty_put(request, true);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_ping(self->dev, request, &self->error));
	ctlv_msg_free(request);
	EXPECT_EQ(-ETIMEDOUT, ctlv_event_wait(self->dev, EVENT_NONE_MS));
}

/*
 * Subscribe first so nothing from now on is lost, then dump, then apply the
 * changes the dump does not already account for. The generation says which.
 */
TEST_F(device, subscribe_then_dump_then_the_changes_after_it)
{
	const __u64 changes = CTLV_OP_OP_CHANGED;
	const struct ctlv_op_info *info;
	struct ctlv_op_change change;
	struct ctlv_event *event;
	struct ctlv_ops *ops;
	__u64 generation;
	bool disabled;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, &changes, 1, &self->error));

	/* A change between subscribing and dumping, which both then report. */
	ASSERT_EQ(0, toggle(self->dev, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	ASSERT_EQ(0, ctlv_ops_dump(self->dev, &ops, &self->error));
	info = ctlv_ops_find(ops, CTLV_TEST_OP_DYNAMIC_NOOP);
	ASSERT_NE(NULL, info);
	generation = ops->generation;
	disabled = info->disabled;
	ctlv_ops_free(ops);
	EXPECT_EQ(!variant->dynamic_disabled, disabled);

	/* And one the dump cannot have known about. */
	ASSERT_EQ(0, toggle(self->dev, &self->error));

	/* The first record is one the dump already accounted for. */
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	ASSERT_EQ(0, ctlv_event_op_change(event, &change));
	ctlv_event_free(event);
	EXPECT_EQ(CTLV_TEST_OP_DYNAMIC_NOOP, change.op_id);
	EXPECT_GE(generation, change.generation);
	EXPECT_EQ(disabled, change.disabled);

	/* The second is newer than the dump, so it is what a reader applies. */
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	ASSERT_EQ(0, ctlv_event_op_change(event, &change));
	ctlv_event_free(event);
	EXPECT_EQ(CTLV_TEST_OP_DYNAMIC_NOOP, change.op_id);
	EXPECT_EQ(generation + 1, change.generation);
	EXPECT_EQ(variant->dynamic_disabled, change.disabled);

	/* Which is where the device is: two changes undid each other. */
	ASSERT_EQ(0, ctlv_ops_dump(self->dev, &ops, &self->error));
	info = ctlv_ops_find(ops, CTLV_TEST_OP_DYNAMIC_NOOP);
	ASSERT_NE(NULL, info);
	EXPECT_EQ(change.generation, ops->generation);
	EXPECT_EQ(change.disabled, info->disabled);
	ctlv_ops_free(ops);
}

/* A queue only a few of one event's records fit in, and more of them sent. */
#define QUEUE_LEN (4 * CTLV_TEST_OP_BEEPED_EVENT_RECORD_MAX_LEN)
#define FLOOD 16
#define FLOOD_PAYLOAD 1024

/*
 * Reading the marker is what ends the epoch it reports, so a reader that could
 * not keep up resynchronizes by dumping rather than by reopening the device.
 */
TEST_F(device, an_overflow_says_what_was_lost)
{
	const __u64 changes = CTLV_OP_OP_CHANGED;
	struct ctlv_overflow overflow;
	struct ctlv_op_change change;
	struct ctlv_event *event;
	struct ctlv_msg *request;
	struct ctlv_ops *ops;
	__u64 len = 0, used = 1, generation, last = 0;
	unsigned int seen = 0;

	ASSERT_EQ(0, ctlv_test_beeped_subscribe(self->dev, &self->error));
	ASSERT_EQ(0, ctlv_subscribe(self->dev, &changes, 1, &self->error));

	ASSERT_EQ(0, ctlv_queue_set(self->dev, QUEUE_LEN, &self->error)) {
		TH_LOG("%s", why(&self->error));
	}
	/* Not empty: the marker and device-gone record are charged at open. */
	ASSERT_EQ(0, ctlv_queue_get(self->dev, &len, &used, &self->error));
	EXPECT_EQ(QUEUE_LEN, len);
	EXPECT_LT(0, used);
	EXPECT_GT(len, used);

	/* More records than that, each as long as this event can be. */
	ASSERT_EQ(0, ctlv_test_ping_request_new(&request));
	ctlv_test_ping_count_put(request, FLOOD);
	ctlv_test_ping_serial_put(request, 1);
	ctlv_test_ping_payload_put(request, FLOOD_PAYLOAD);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_test_ping(self->dev, request, &self->error));
	ctlv_msg_free(request);

	/* What arrived, up to the marker saying the rest did not. */
	for (;;) {
		ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
		ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
		if (ctlv_event_op_id(event) == CTLV_OP_EVENT_OVERFLOW)
			break;
		EXPECT_TRUE(ctlv_test_beeped_is(event));
		last = ctlv_event_sequence(event);
		EXPECT_EQ(++seen, last);
		ctlv_event_free(event);
		ASSERT_GT(FLOOD, seen);
	}
	ASSERT_EQ(0, ctlv_event_overflow(event, &overflow));
	ctlv_event_free(event);
	EXPECT_LT(0, seen);

	/* Every record this reader missed, accounted for exactly once. */
	EXPECT_EQ(last + 1, overflow.first_sequence);
	EXPECT_EQ(FLOOD, overflow.last_sequence);
	EXPECT_EQ(FLOOD - seen, overflow.dropped_count);
	EXPECT_EQ(overflow.last_sequence - overflow.first_sequence + 1,
		  overflow.dropped_count);

	/* The queue takes records again, so a dump is where to start over. */
	ASSERT_EQ(0, ctlv_ops_dump(self->dev, &ops, &self->error));
	generation = ops->generation;
	ctlv_ops_free(ops);
	ASSERT_EQ(0, toggle(self->dev, &self->error));

	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	EXPECT_EQ(0, ctlv_event_op_change(event, &change));
	/* Numbered after everything the marker spoke for. */
	EXPECT_EQ(FLOOD + 1, ctlv_event_sequence(event));
	EXPECT_EQ(generation + 1, change.generation);
	ctlv_event_free(event);

	ASSERT_EQ(0, toggle(self->dev, &self->error));
}

/*
 * An attr, an enum value or a flag bit added after a reader was built is not a
 * failure. None of this needs a device: what a message means is the message's.
 */
TEST(what_a_reader_has_never_heard_of)
{
	const struct ctlv_attr *root, *attr;
	__u64 value = 0, unknown = 0, seen = 0;
	struct ctlv_msg *msg;
	bool known = true;

	ASSERT_EQ(0, ctlv_msg_new(CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN, &msg));

	/* An enum value and a flag bit above everything declared. */
	ctlv_test_echo_mode_put(msg, CTLV_TEST_ECHO_MODE_AUTO + 1);
	ctlv_test_echo_quirks_put(msg, CTLV_TEST_ECHO_QUIRK_SLOW |
				       (CTLV_TEST_ECHO_QUIRK_MASK + 1));
	/* And an attr of a family this reader knows nothing about. */
	ctlv_test_child_child_extra_put(msg, 1);
	ctlv_test_echo_u_put(msg, ECHO_U);
	ASSERT_EQ(0, ctlv_msg_done(msg));
	root = ctlv_msg_root(msg);

	EXPECT_TRUE(ctlv_test_echo_mode_get(root, &value, &known));
	EXPECT_EQ(CTLV_TEST_ECHO_MODE_AUTO + 1, value);
	EXPECT_FALSE(known);

	EXPECT_TRUE(ctlv_test_echo_quirks_get(root, &value, &unknown));
	EXPECT_EQ(CTLV_TEST_ECHO_QUIRK_SLOW, value & CTLV_TEST_ECHO_QUIRK_MASK);
	EXPECT_EQ(CTLV_TEST_ECHO_QUIRK_MASK + 1, unknown);

	/* The attr nobody here declared is in the nest and in no getter. */
	ctlv_for_each(attr, root)
		seen++;
	EXPECT_EQ(4, seen);
	EXPECT_TRUE(ctlv_test_echo_u_get(root, &value));
	EXPECT_EQ(ECHO_U, value);
	ctlv_msg_free(msg);
}

/* The ignore mark applies to the last attr, including a completed nest. */
TEST(marking_an_attr_a_kernel_may_skip)
{
	const struct ctlv_attr *root, *attr;
	struct ctlv_msg *msg;

	ASSERT_EQ(0, ctlv_msg_new(CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN, &msg));
	/* With nothing written yet, what came last is the root. */
	EXPECT_EQ(-EINVAL, ctlv_msg_ignore_if_unknown(msg));
	ctlv_msg_reset(msg);

	ctlv_test_echo_u_put(msg, ECHO_U);
	ctlv_test_child_child_extra_put(msg, ECHO_EXTRA);
	EXPECT_EQ(0, ctlv_msg_ignore_if_unknown(msg));
	ctlv_test_echo_item_start(msg);
	ctlv_test_item_index_put(msg, 1);
	ctlv_nest_end(msg);
	EXPECT_EQ(0, ctlv_msg_ignore_if_unknown(msg));
	ASSERT_EQ(0, ctlv_msg_done(msg));

	root = ctlv_msg_root(msg);
	EXPECT_EQ(CTLV_ATTR_F_NESTED, root->flags);
	attr = ctlv_attr_find(root, CTLV_TEST_ATTR_ECHO_U);
	ASSERT_NE(NULL, attr);
	EXPECT_EQ(0, attr->flags);
	attr = ctlv_attr_find(root, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA);
	ASSERT_NE(NULL, attr);
	EXPECT_EQ(CTLV_ATTR_F_IGNORE_IF_UNKNOWN, attr->flags);
	/* A nest is marked whole, its members having said nothing. */
	attr = ctlv_attr_find(root, CTLV_TEST_ATTR_ECHO_ITEM);
	ASSERT_NE(NULL, attr);
	EXPECT_EQ(CTLV_ATTR_F_NESTED | CTLV_ATTR_F_IGNORE_IF_UNKNOWN,
		  attr->flags);
	attr = ctlv_attr_find(attr, CTLV_TEST_ATTR_ITEM_INDEX);
	ASSERT_NE(NULL, attr);
	EXPECT_EQ(0, attr->flags);
	ctlv_msg_free(msg);
}

int main(int argc, char **argv)
{
	/* Nothing else in a test image loads what these devices come from. */
	ctlv_test_module_load();
	return test_harness_run(argc, argv);
}
