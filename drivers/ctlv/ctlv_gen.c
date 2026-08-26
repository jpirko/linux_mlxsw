// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include <linux/ctlv_gen.h>

static const struct ctlv_node_schema families_dump_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema families_dump_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_NEST(CTLV_ATTR_FAMILY_ENTRY, CTLV_NODE_SCHEMA_UNLIMITED, 2, 2, 2),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_FAMILY_ID, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_STRING(CTLV_ATTR_FAMILY_NAME, 1, 0, 63),
};

static const struct ctlv_node_schema ops_dump_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema ops_dump_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_NEST(CTLV_ATTR_OP_ENTRY, CTLV_NODE_SCHEMA_UNLIMITED, 2, 10, 2),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_ID, 1, 1, U64_MAX),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_OP_TYPE, 1, 3),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_OP_AVAILABILITY, 1, 2),
	CTLV_NODE_SCHEMA_BOOL(CTLV_ATTR_OP_DISABLED, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_REQUEST_MAX_LEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_OP_FD_ACCESS, 1, 2),
};

static const struct ctlv_node_schema op_schema_get_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_ID, 1, 1, U64_MAX),
};

static const struct ctlv_node_schema op_schema_get_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 0),
	CTLV_NODE_SCHEMA_NEST(CTLV_ATTR_SCHEMA_REQUEST, 1, 3, 17, 3),
	CTLV_NODE_SCHEMA_NEST(CTLV_ATTR_SCHEMA_REPLY, 1, 20, 17, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_ATTR_ID, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_KIND, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MIN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MIN_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_FLAGS_MASK, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_UNIT, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_SINT(CTLV_ATTR_SCHEMA_SCALE, 1, S64_MIN, S64_MAX),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_SCHEMA_BLOB_DIRECTION, 1, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_BACKINGS, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_REENTERS, 1, 1, U64_MAX),
	CTLV_NODE_SCHEMA_NEST_REENTERS(CTLV_ATTR_SCHEMA_ATTRS, CTLV_NODE_SCHEMA_UNLIMITED, 3, 17,
				       3),
	CTLV_NODE_SCHEMA_NEST_REENTERS(CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS, CTLV_NODE_SCHEMA_UNLIMITED,
				       3, 17, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_ATTR_ID, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_KIND, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MIN, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MIN_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_MAX_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_FLAGS_MASK, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_UNIT, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_SINT(CTLV_ATTR_SCHEMA_SCALE, 1, S64_MIN, S64_MAX),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_SCHEMA_BLOB_DIRECTION, 1, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_BACKINGS, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_SCHEMA_REENTERS, 1, 1, U64_MAX),
	CTLV_NODE_SCHEMA_NEST_REENTERS(CTLV_ATTR_SCHEMA_ATTRS, CTLV_NODE_SCHEMA_UNLIMITED, 20, 17,
				       3),
	CTLV_NODE_SCHEMA_NEST_REENTERS(CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS, CTLV_NODE_SCHEMA_UNLIMITED,
				       20, 17, 3),
};

static const struct ctlv_node_schema subscriptions_add_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_EVENT_OP_ID, 256, 1, U64_MAX),
};

static const struct ctlv_node_schema subscriptions_remove_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_EVENT_OP_ID, 256, 1, U64_MAX),
};

static const struct ctlv_node_schema event_queue_get_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema event_queue_get_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 2, 2),
	CTLV_NODE_SCHEMA_UINT_UNIT(CTLV_ATTR_EVENT_QUEUE_LEN, 1, 1, 16777216, CTLV_UNIT_BYTES, 0),
	CTLV_NODE_SCHEMA_UINT_UNIT(CTLV_ATTR_EVENT_QUEUE_USED, 1, 0, U64_MAX, CTLV_UNIT_BYTES, 0),
};

static const struct ctlv_node_schema event_queue_set_request_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_UINT_UNIT(CTLV_ATTR_EVENT_QUEUE_LEN, 1, 1, 16777216, CTLV_UNIT_BYTES, 0),
};

static const struct ctlv_node_schema op_changed_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 3, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_ID, 1, 1, U64_MAX),
	CTLV_NODE_SCHEMA_BOOL(CTLV_ATTR_OP_DISABLED, 1),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_OP_STATE_GENERATION, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema event_overflow_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 3, 3),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_EVENT_FIRST_SEQUENCE, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_EVENT_LAST_SEQUENCE, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_EVENT_DROPPED_COUNT, 1, 0, U64_MAX),
};

static const struct ctlv_node_schema device_removed_reply_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(0, 0, 0),
};

static const struct ctlv_node_schema error_info_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 7, 0),
	CTLV_NODE_SCHEMA_ENUM(CTLV_ATTR_ERROR_REASON, 1, 12),
	CTLV_NODE_SCHEMA_STRING(CTLV_ATTR_ERROR_MESSAGE, 1, 0, 255),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_ERROR_BAD_ATTR_OFFSET, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_ERROR_NEST_OFFSET, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_ERROR_MISSING_ATTR_ID, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_ERROR_EXPECTED_GENERATION, 1, 0, U64_MAX),
	CTLV_NODE_SCHEMA_UINT(CTLV_ATTR_ERROR_CURRENT_GENERATION, 1, 0, U64_MAX),
};

const struct ctlv_schema_tree ctlv_error_info_schema = {
	.node_schemas = error_info_node_schemas,
	.node_schemas_count =
		ARRAY_SIZE(error_info_node_schemas),
};

static_assert(CTLV_ERROR_INFO_MAX_LEN <= CTLV_MAX_ERROR_LEN);

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_op_schemas[] = {
	CTLV_OP_SCHEMA_QUERY(CTLV_OP_FAMILIES_DUMP, CTLV_OP_FD_ACCESS_READ,
			     CTLV_OP_AVAILABILITY_FIXED, 16, 112, 65536,
			     CTLV_SCHEMA_TREE(families_dump_request),
			     CTLV_SCHEMA_TREE(families_dump_reply)),
	CTLV_OP_SCHEMA_QUERY(CTLV_OP_OPS_DUMP, CTLV_OP_FD_ACCESS_READ, CTLV_OP_AVAILABILITY_FIXED,
			     16, 56, 65536, CTLV_SCHEMA_TREE(ops_dump_request),
			     CTLV_SCHEMA_TREE(ops_dump_reply)),
	CTLV_OP_SCHEMA_QUERY(CTLV_OP_OP_SCHEMA_GET, CTLV_OP_FD_ACCESS_READ,
			     CTLV_OP_AVAILABILITY_FIXED, 40, 552, 65536,
			     CTLV_SCHEMA_TREE(op_schema_get_request),
			     CTLV_SCHEMA_TREE(op_schema_get_reply)),
	CTLV_OP_SCHEMA_ACTION(CTLV_OP_SUBSCRIPTIONS_ADD, CTLV_OP_FD_ACCESS_READ,
			      CTLV_OP_AVAILABILITY_FIXED, 6160, 0,
			      CTLV_SCHEMA_TREE(subscriptions_add_request), CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_ACTION(CTLV_OP_SUBSCRIPTIONS_REMOVE, CTLV_OP_FD_ACCESS_READ,
			      CTLV_OP_AVAILABILITY_FIXED, 6160, 0,
			      CTLV_SCHEMA_TREE(subscriptions_remove_request),
			      CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_QUERY(CTLV_OP_EVENT_QUEUE_GET, CTLV_OP_FD_ACCESS_READ,
			     CTLV_OP_AVAILABILITY_FIXED, 16, 40, 65536,
			     CTLV_SCHEMA_TREE(event_queue_get_request),
			     CTLV_SCHEMA_TREE(event_queue_get_reply)),
	CTLV_OP_SCHEMA_ACTION(CTLV_OP_EVENT_QUEUE_SET, CTLV_OP_FD_ACCESS_READ,
			      CTLV_OP_AVAILABILITY_FIXED, 40, 0,
			      CTLV_SCHEMA_TREE(event_queue_set_request), CTLV_SCHEMA_TREE_NONE()),
	CTLV_OP_SCHEMA_EVENT(CTLV_OP_OP_CHANGED, 112, CTLV_SCHEMA_TREE(op_changed_reply)),
	CTLV_OP_SCHEMA_EVENT(CTLV_OP_EVENT_OVERFLOW, 112, CTLV_SCHEMA_TREE(event_overflow_reply)),
	CTLV_OP_SCHEMA_EVENT(CTLV_OP_DEVICE_REMOVED, 40, CTLV_SCHEMA_TREE(device_removed_reply)),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_schema = {
	.name = "ctlv",
	.family_id = 0,
	.ops = ctlv_op_schemas,
	.ops_count = ARRAY_SIZE(ctlv_op_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(60 <= CTLV_UEVENT_MAX_LEN);
static_assert(3 <= CTLV_UEVENT_MAX_VARS);

/* Guards for generated maxima. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(112 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(2 <= CTLV_MAX_DECLARED_NEST_DEPTH);

/* Local action and query bindings in declaration order. */
static const struct ctlv_op ctlv_ops[] = {
	CTLV_OP_QUERY(ctlv_families_dump),
	CTLV_OP_QUERY(ctlv_ops_dump),
	CTLV_OP_QUERY(ctlv_op_schema_get),
	CTLV_OP_ACTION(ctlv_subscriptions_add),
	CTLV_OP_ACTION(ctlv_subscriptions_remove),
	CTLV_OP_QUERY(ctlv_event_queue_get),
	CTLV_OP_ACTION(ctlv_event_queue_set),
};

/* Root-family descriptor used by the CTLV core. */
const struct ctlv_family ctlv_family = {
	.schema = &ctlv_schema,
	.ops = ctlv_ops,
	.ops_count = ARRAY_SIZE(ctlv_ops),
	.pre = ctlv_op_pre,
	.post = ctlv_op_post,
	.priv_size = ctlv_priv_size,
	.init = ctlv_init,
	.fini = ctlv_fini,
};
EXPORT_SYMBOL_NS_GPL(ctlv_family, "CTLV");
