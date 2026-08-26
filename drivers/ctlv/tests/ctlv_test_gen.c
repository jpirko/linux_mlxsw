// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>
#include <uapi/linux/ctlv_test_gen.h>

#include "ctlv_test_gen.h"

static const struct ctlv_node_schema noop_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema probe_get_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PROBE_TOKEN, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema probe_get_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PROBE_TOKEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_PROBE_EXTENSION, 1, 0, 0, 0),
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_schema probe_get_reply_stub_schemas[] = {
	CTLV_STUB_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT, 2),
};

static const struct ctlv_node_schema dynamic_noop_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema dynamic_toggle_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PROBE_TOKEN, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema dynamic_toggle_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PROBE_TOKEN, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema pinged_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema echo_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 12, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_TEST_ATTR_ECHO_S, 1, -100, 100),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_ECHO_FLAG, 1),
	CTLV_NODE_SCHEMA_ENUM(CTLV_TEST_ATTR_ECHO_MODE, 1, 3),
	CTLV_NODE_SCHEMA_FLAGS(CTLV_TEST_ATTR_ECHO_QUIRKS, 1, CTLV_TEST_ECHO_QUIRK_MASK),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ECHO_TEXT, 1, 1, 15),
	CTLV_NODE_SCHEMA_BINARY(CTLV_TEST_ATTR_ECHO_DATA, 1, 4, 32),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_COUNT, 3, 0, U64_MAX),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ECHO_ITEM, 2, 13, 3, 1),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_ECHO_REGION, 1, 4096, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_ECHO_PAGE, 1, 65536, 4096,
			      BIT(CTLV_BLOB_TYPE_MEMFD),
			      CTLV_BLOB_DIRECTION_OUTPUT),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_PROBE_EXTENSION, 1, 0, 0, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ITEM_LABEL, 1, 0, 7),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ITEM_LEAF, 1, 16, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_schema echo_request_stub_schemas[] = {
	CTLV_STUB_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT, 12),
};

static const struct ctlv_node_schema echo_reply_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 12, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_TEST_ATTR_ECHO_S, 1, -100, 100),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_ECHO_FLAG, 1),
	CTLV_NODE_SCHEMA_ENUM(CTLV_TEST_ATTR_ECHO_MODE, 1, 3),
	CTLV_NODE_SCHEMA_FLAGS(CTLV_TEST_ATTR_ECHO_QUIRKS, 1, CTLV_TEST_ECHO_QUIRK_MASK),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ECHO_TEXT, 1, 1, 15),
	CTLV_NODE_SCHEMA_BINARY(CTLV_TEST_ATTR_ECHO_DATA, 1, 4, 32),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_COUNT, 3, 0, U64_MAX),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ECHO_ITEM, 2, 13, 3, 1),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_ECHO_REGION, 1, 4096, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_ECHO_PAGE, 1, 65536, 4096,
			      BIT(CTLV_BLOB_TYPE_MEMFD),
			      CTLV_BLOB_DIRECTION_OUTPUT),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_PROBE_EXTENSION, 1, 0, 0, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ITEM_LABEL, 1, 0, 7),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ITEM_LEAF, 1, 16, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_schema echo_reply_request_stub_schemas[] = {
	CTLV_STUB_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT, 12),
};

static const struct ctlv_node_schema echo_reply_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 10, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_U, 1, 1, 4096),
	CTLV_NODE_SCHEMA_SINT(CTLV_TEST_ATTR_ECHO_S, 1, -100, 100),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_ECHO_FLAG, 1),
	CTLV_NODE_SCHEMA_ENUM(CTLV_TEST_ATTR_ECHO_MODE, 1, 3),
	CTLV_NODE_SCHEMA_FLAGS(CTLV_TEST_ATTR_ECHO_QUIRKS, 1, CTLV_TEST_ECHO_QUIRK_MASK),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ECHO_TEXT, 1, 1, 15),
	CTLV_NODE_SCHEMA_BINARY(CTLV_TEST_ATTR_ECHO_DATA, 1, 4, 32),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ECHO_COUNT, 3, 0, U64_MAX),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ECHO_ITEM, 2, 11, 3, 1),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_PROBE_EXTENSION, 1, 0, 0, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
	CTLV_NODE_SCHEMA_STRING(CTLV_TEST_ATTR_ITEM_LABEL, 1, 0, 7),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_ITEM_LEAF, 1, 14, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_ITEM_INDEX, 1, 0, 7),
};

/* Extension points selected by this tree. */
static const struct ctlv_stub_schema echo_reply_reply_stub_schemas[] = {
	CTLV_STUB_SCHEMA(CTLV_FAMILY_TEST, CTLV_TEST_SET_EXTENSION_POINT, 10),
};

static const struct ctlv_node_schema series_dump_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 4, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_CHUNKS, 1, 0, 8),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_ITEMS, 1, 0, 8),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_VALUES, 1, 0, 8),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_TOTALS, 1, 0, 8),
};

static const struct ctlv_node_schema series_dump_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 0),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_SERIES_CHUNK, CTLV_NODE_SCHEMA_UNLIMITED, 3, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_TOTAL, CTLV_NODE_SCHEMA_UNLIMITED, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_INDEX, 1, 0, 8),
	CTLV_NODE_SCHEMA_NEST(CTLV_TEST_ATTR_SERIES_ITEM, CTLV_NODE_SCHEMA_UNLIMITED, 5, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_INDEX, 1, 0, 8),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_SERIES_VALUE, CTLV_NODE_SCHEMA_UNLIMITED, 0, U64_MAX),
};

static const struct ctlv_node_schema ping_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 5, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PING_COUNT, 1, 0, 64),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PING_SERIAL, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PING_PAYLOAD, 1, 0, 1024),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_PING_ABORT, 1),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_PING_EMPTY, 1),
};

static const struct ctlv_node_schema beeped_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_PING_SERIAL, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_BINARY(CTLV_TEST_ATTR_PING_FILLER, 1, 0, 1024),
};

static const struct ctlv_node_schema transfer_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 8, 0),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_TRANSFER_SOURCE, 1, 268435456, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_TRANSFER_TARGET, 1, 268435456, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_OUTPUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_TRANSFER_SCRATCH, 1, 268435456, 8,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INOUT),
	CTLV_NODE_SCHEMA_BLOB(CTLV_TEST_ATTR_TRANSFER_PIECE, 4, 1048576, 1,
			      BIT(CTLV_BLOB_TYPE_VA) |
			      BIT(CTLV_BLOB_TYPE_MEMFD) |
			      BIT(CTLV_BLOB_TYPE_DMA_BUF),
			      CTLV_BLOB_DIRECTION_INPUT),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_TRANSFER_FILL, 1, 0, 255),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_TRANSFER_SKIP, 1),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_TRANSFER_TWICE, 1),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_TRANSFER_HOLD, 1),
};

static const struct ctlv_node_schema transfer_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 3, 0),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_TRANSFER_SUM, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_TEST_ATTR_TRANSFER_WRITTEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_BOOL(CTLV_TEST_ATTR_TRANSFER_DISTINCT, 1),
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_test_op_schemas[] = {
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_NOOP, CTLV_OP_FD_ACCESS_READ, CTLV_OP_AVAILABILITY_FIXED,
			      16, 0, CTLV_SCHEMA_TREE(noop_request), CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_QUERY(CTLV_TEST_OP_PROBE_GET, CTLV_OP_FD_ACCESS_READ,
			     CTLV_OP_AVAILABILITY_FIXED, 40, 40, 65536,
			     CTLV_SCHEMA_TREE(probe_get_request),
			     CTLV_SCHEMA_TREE_STUBS(probe_get_reply), .pre_post = true),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_DYNAMIC_NOOP, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_DYNAMIC, 16, 0,
			      CTLV_SCHEMA_TREE(dynamic_noop_request), CTLV_SCHEMA_TREE_NONE(),
			      .optional = true, .pre_post = true),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_DYNAMIC_TOGGLE, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 40, 40,
			      CTLV_SCHEMA_TREE(dynamic_toggle_request),
			      CTLV_SCHEMA_TREE(dynamic_toggle_reply), .optional = true),
	CTLV_OP_SCHEMA_EVENT(CTLV_TEST_OP_PINGED, 40, CTLV_SCHEMA_TREE(pinged_reply)),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_ECHO, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 736, 0,
			      CTLV_SCHEMA_TREE_STUBS(echo_request), CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_ECHO_REPLY, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 736, 512,
			      CTLV_SCHEMA_TREE_STUBS(echo_reply_request),
			      CTLV_SCHEMA_TREE_STUBS(echo_reply_reply)),
	CTLV_OP_SCHEMA_QUERY(CTLV_TEST_OP_SERIES_DUMP, CTLV_OP_FD_ACCESS_READ_WRITE,
			     CTLV_OP_AVAILABILITY_FIXED, 112, 72, 4096,
			     CTLV_SCHEMA_TREE(series_dump_request),
			     CTLV_SCHEMA_TREE(series_dump_reply), .pre_post = true),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_PING, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 136, 0, CTLV_SCHEMA_TREE(ping_request),
			      CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_EVENT(CTLV_TEST_OP_BEEPED, 1104, CTLV_SCHEMA_TREE(beeped_reply)),
	CTLV_OP_SCHEMA_ACTION(CTLV_TEST_OP_TRANSFER, CTLV_OP_FD_ACCESS_READ_WRITE,
			      CTLV_OP_AVAILABILITY_FIXED, 896, 88,
			      CTLV_SCHEMA_TREE(transfer_request), CTLV_SCHEMA_TREE(transfer_reply)),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_test_schema = {
	.name = "ctlv-test",
	.family_id = 3,
	.ops = ctlv_test_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_test_op_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(93 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(1104 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(2 <= CTLV_MAX_DECLARED_NEST_DEPTH);

/* Local action and query bindings in declaration order. */
static const struct ctlv_op ctlv_test_ops[] = {
	CTLV_OP_ACTION(ctlv_test_noop),
	CTLV_OP_QUERY_PRE_POST(ctlv_test_probe_get),
	CTLV_OP_ACTION_PRE_POST(ctlv_test_dynamic_noop),
	CTLV_OP_ACTION(ctlv_test_dynamic_toggle),
	CTLV_OP_ACTION(ctlv_test_echo),
	CTLV_OP_ACTION(ctlv_test_echo_reply),
	CTLV_OP_QUERY_PRE_POST(ctlv_test_series_dump),
	CTLV_OP_ACTION(ctlv_test_ping),
	CTLV_OP_ACTION(ctlv_test_transfer),
};

/* Family descriptor used by ctlv_create(). */
const struct ctlv_family ctlv_test_family = {
	.schema = &ctlv_test_schema,
	.parent = &ctlv_family,
	.ops = ctlv_test_ops,
	.ops_count = ARRAY_SIZE(ctlv_test_ops),
	.pre = ctlv_test_op_pre,
	.post = ctlv_test_op_post,
	.priv_size = ctlv_test_priv_size,
	.init = ctlv_test_init,
	.post_init = ctlv_test_post_init,
	.pre_fini = ctlv_test_pre_fini,
	.fini = ctlv_test_fini,
};
EXPORT_SYMBOL_NS_GPL(ctlv_test_family, "CTLV");
