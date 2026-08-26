// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/*
 * Generated from Documentation/ctlv/specs/ctlv.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

/* Effective request and reply schemas. */

static const struct ctlv_node_schema family_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* family-get families, fields */
static const struct ctlv_node_schema family_get_reply_families_field_node_schemas[] = {
	{
		.type = CTLV_T_FAMILY_GET_FAMILIES_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_FAMILY_GET_FAMILIES_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 63,
	},
};

/* root, fields */
static const struct ctlv_node_schema family_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_T_FAMILY_GET_FAMILIES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = family_get_reply_families_field_node_schemas,
		.fields_count = 2,
	},
};

static const struct ctlv_node_schema family_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = family_get_reply_field_node_schemas,
	.fields_count = 1,
};

static const struct ctlv_node_schema op_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* op-get ops, fields */
static const struct ctlv_node_schema op_get_reply_ops_field_node_schemas[] = {
	{
		.type = CTLV_T_OP_GET_OPS_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_OP_GET_OPS_TYPE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* op-get ops, optional fields */
static const struct ctlv_node_schema op_get_reply_ops_optional_field_node_schemas[] = {
	{
		.type = CTLV_T_OP_GET_OPS_AVAILABILITY,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_T_OP_GET_OPS_DISABLED,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_T_OP_GET_OPS_REQUEST_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_OP_GET_OPS_ACTION_REPLY_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_OP_GET_OPS_QUERY_REPLY_MIN_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_OP_GET_OPS_QUERY_REPLY_SUGGESTED_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_OP_GET_OPS_FD_ACCESS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_T_OP_GET_OPS_ERROR_INFO_MAX_LEN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, fields */
static const struct ctlv_node_schema op_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_T_OP_GET_OPS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = op_get_reply_ops_field_node_schemas,
		.fields_count = 2,
		.optional_fields = op_get_reply_ops_optional_field_node_schemas,
		.optional_fields_count = 8,
	},
};

static const struct ctlv_node_schema op_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = op_get_reply_field_node_schemas,
	.fields_count = 1,
};

/* root, fields */
static const struct ctlv_node_schema op_schema_get_request_field_node_schemas[] = {
	{
		.type = CTLV_T_OP_SCHEMA_GET_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

static const struct ctlv_node_schema op_schema_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = op_schema_get_request_field_node_schemas,
	.fields_count = 1,
};

/* op-schema-get request, fields */
static const struct ctlv_node_schema op_schema_get_reply_request_field_node_schemas[] = {
	{
		.type = CTLV_T_SCHEMA_NODE_TYPE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_KIND,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX_OCCURRENCES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* op-schema-get request, optional fields */
static const struct ctlv_node_schema op_schema_get_reply_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_T_SCHEMA_NODE_MIN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MIN_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_FLAGS_MASK,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_UNIT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_SCALE,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_DIRECTION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_BACKINGS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_ALIGNMENT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_REENTERS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_FIELDS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_OPTIONAL_FIELDS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* op-schema-get reply, fields */
static const struct ctlv_node_schema op_schema_get_reply_reply_field_node_schemas[] = {
	{
		.type = CTLV_T_SCHEMA_NODE_TYPE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_KIND,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX_OCCURRENCES,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* op-schema-get reply, optional fields */
static const struct ctlv_node_schema op_schema_get_reply_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_T_SCHEMA_NODE_MIN,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MIN_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_FLAGS_MASK,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_UNIT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_SCALE,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_DIRECTION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_BACKINGS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_BLOB_ALIGNMENT,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_REENTERS,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_FIELDS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	{
		.type = CTLV_T_SCHEMA_NODE_OPTIONAL_FIELDS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_NODE_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema op_schema_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_T_OP_SCHEMA_GET_REQUEST,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = op_schema_get_reply_request_field_node_schemas,
		.fields_count = 3,
		.optional_fields = op_schema_get_reply_request_optional_field_node_schemas,
		.optional_fields_count = 14,
	},
	{
		.type = CTLV_T_OP_SCHEMA_GET_REPLY,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = op_schema_get_reply_reply_field_node_schemas,
		.fields_count = 3,
		.optional_fields = op_schema_get_reply_reply_optional_field_node_schemas,
		.optional_fields_count = 14,
	},
};

static const struct ctlv_node_schema op_schema_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = op_schema_get_reply_optional_field_node_schemas,
	.optional_fields_count = 2,
};

/* Effective ops in declaration order. */
static const struct ctlv_op_spec ctlv_op_specs[] = {
	{
		.op_id = CTLV_OP_FAMILY_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 112,
		.query_reply_suggested_len = 65536,
		.error_info_max_len = 456,
		.request = &family_get_request_node_schema,
		.reply = &family_get_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_OP_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 56,
		.query_reply_suggested_len = 65536,
		.error_info_max_len = 456,
		.request = &op_get_request_node_schema,
		.reply = &op_get_reply_node_schema,
	},
	{
		.op_id = CTLV_OP_OP_SCHEMA_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.query_reply_min_len = 168,
		.query_reply_suggested_len = 65536,
		.error_info_max_len = 456,
		.request = &op_schema_get_request_node_schema,
		.reply = &op_schema_get_reply_node_schema,
	},
};

/* Family specification descriptor. */
const struct ctlv_family_spec ctlv_spec = {
	.name = "ctlv",
	.family_id = 0,
	.ops = ctlv_op_specs,
	.ops_count = 3,
};
