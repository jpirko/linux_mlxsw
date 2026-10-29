// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-tdx.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_attestation_tdx_user_gen.h"

/* Effective request and reply schemas. */

static const struct ctlv_node_schema device_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* device-get caps.evidence.objects.challenge-lengths, optional fields */
static const struct ctlv_node_schema
	device_get_reply_caps_evidence_objects_challenge_lengths_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 4096,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 4096,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX |
			   CTLV_NODE_SCHEMA_HAS_UNIT | CTLV_NODE_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 4096,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

/* device-get caps.evidence.objects, fields */
static const struct ctlv_node_schema device_get_reply_caps_evidence_objects_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 15,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* device-get caps.evidence.objects, optional fields */
static const struct ctlv_node_schema
	device_get_reply_caps_evidence_objects_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_FORMATS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 4,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 8,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_VENDOR_FORMATS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 4,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CHALLENGE_LENGTHS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 8,
		.optional_fields =
	device_get_reply_caps_evidence_objects_challenge_lengths_optional_field_node_schemas,
		.optional_fields_count = 3,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_INSTANCE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_TCB_DIGEST_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
};

/* device-get caps.evidence, fields */
static const struct ctlv_node_schema device_get_reply_caps_evidence_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_OBJECTS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 16,
		.fields = device_get_reply_caps_evidence_objects_field_node_schemas,
		.fields_count = 3,
		.optional_fields =
			device_get_reply_caps_evidence_objects_optional_field_node_schemas,
		.optional_fields_count = 5,
	},
};

/* device-get caps.evidence, optional fields */
static const struct ctlv_node_schema
	device_get_reply_caps_evidence_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 15,
	},
};

/* device-get caps.mr, fields */
static const struct ctlv_node_schema device_get_reply_caps_mr_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
};

/* device-get caps, optional fields */
static const struct ctlv_node_schema device_get_reply_caps_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = device_get_reply_caps_evidence_field_node_schemas,
		.fields_count = 1,
		.optional_fields = device_get_reply_caps_evidence_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = device_get_reply_caps_mr_field_node_schemas,
		.fields_count = 1,
	},
};

/* device-get info, fields */
static const struct ctlv_node_schema device_get_reply_info_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MRCONFIGID,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
	{
		.type = CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNER,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
	{
		.type = CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNERCONFIG,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
};

/* root, fields */
static const struct ctlv_node_schema device_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_CAPS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.optional_fields = device_get_reply_caps_optional_field_node_schemas,
		.optional_fields_count = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GET_INFO,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = device_get_reply_info_field_node_schemas,
		.fields_count = 3,
	},
};

static const struct ctlv_node_schema device_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = device_get_reply_field_node_schemas,
	.fields_count = 3,
};

/* evidence-get objects, fields */
static const struct ctlv_node_schema evidence_get_request_objects_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 15,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0xffffffffULL,
		.blob_alignment = 1,
	},
};

/* evidence-get selection, fields */
static const struct ctlv_node_schema evidence_get_request_selection_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
};

/* root, fields */
static const struct ctlv_node_schema evidence_get_request_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 16,
		.fields = evidence_get_request_objects_field_node_schemas,
		.fields_count = 2,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema evidence_get_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 4096,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 256,
		.fields = evidence_get_request_selection_field_node_schemas,
		.fields_count = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_PRIVLEVEL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_PROVIDER,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_GUID,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 16,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_MANIFEST_VERSION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_OPTIONS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema evidence_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = evidence_get_request_field_node_schemas,
	.fields_count = 1,
	.optional_fields = evidence_get_request_optional_field_node_schemas,
	.optional_fields_count = 8,
};

/* evidence-get results.tcb-digest, fields */
static const struct ctlv_node_schema evidence_get_reply_results_tcb_digest_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* evidence-get results, fields */
static const struct ctlv_node_schema evidence_get_reply_results_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* evidence-get results, optional fields */
static const struct ctlv_node_schema evidence_get_reply_results_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 8,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_TCB_DIGEST,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = evidence_get_reply_results_tcb_digest_field_node_schemas,
		.fields_count = 2,
	},
};

/* root, fields */
static const struct ctlv_node_schema evidence_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 16,
		.fields = evidence_get_reply_results_field_node_schemas,
		.fields_count = 1,
		.optional_fields = evidence_get_reply_results_optional_field_node_schemas,
		.optional_fields_count = 3,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema evidence_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_GET_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = evidence_get_reply_field_node_schemas,
	.fields_count = 2,
	.optional_fields = evidence_get_reply_optional_field_node_schemas,
	.optional_fields_count = 2,
};

/* root, optional fields */
static const struct ctlv_node_schema mr_get_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GET_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema mr_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = mr_get_request_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* mr-get items, fields */
static const struct ctlv_node_schema mr_get_reply_items_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GET_ITEMS_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_DESC_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 23,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_DESC_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
};

/* mr-get items, optional fields */
static const struct ctlv_node_schema mr_get_reply_items_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_DESC_STATE_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* root, fields */
static const struct ctlv_node_schema mr_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GET_ITEMS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = mr_get_reply_items_field_node_schemas,
		.fields_count = 4,
		.optional_fields = mr_get_reply_items_optional_field_node_schemas,
		.optional_fields_count = 2,
	},
};

static const struct ctlv_node_schema mr_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_get_reply_field_node_schemas,
	.fields_count = 1,
};

/* root, fields */
static const struct ctlv_node_schema mr_read_request_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_READ_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema mr_read_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_READ_OPTIONS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema mr_read_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_read_request_field_node_schemas,
	.fields_count = 1,
	.optional_fields = mr_read_request_optional_field_node_schemas,
	.optional_fields_count = 2,
};

/* mr-values, fields */
static const struct ctlv_node_schema mr_read_reply_mr_values_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* root, fields */
static const struct ctlv_node_schema mr_read_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 23,
		.fields = mr_read_reply_mr_values_field_node_schemas,
		.fields_count = 2,
	},
};

static const struct ctlv_node_schema mr_read_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_read_reply_field_node_schemas,
	.fields_count = 3,
};

/* mr-values, fields */
static const struct ctlv_node_schema mr_extend_request_mr_values_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* mr-extend record, fields */
static const struct ctlv_node_schema mr_extend_request_record_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* root, fields */
static const struct ctlv_node_schema mr_extend_request_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema mr_extend_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 23,
		.fields = mr_extend_request_mr_values_field_node_schemas,
		.fields_count = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_RAW_DATA,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_INPUT,
		.blob_backings = 14,
		.blob_max_length = 0xffffffffULL,
		.blob_alignment = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_OPTIONS,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_EXTEND_RECORD,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_extend_request_record_field_node_schemas,
		.fields_count = 2,
	},
};

static const struct ctlv_node_schema mr_extend_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_extend_request_field_node_schemas,
	.fields_count = 1,
	.optional_fields = mr_extend_request_optional_field_node_schemas,
	.optional_fields_count = 5,
};

/* mr-values, fields */
static const struct ctlv_node_schema mr_extend_reply_mr_values_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* root, fields */
static const struct ctlv_node_schema mr_extend_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 23,
		.fields = mr_extend_reply_mr_values_field_node_schemas,
		.fields_count = 2,
	},
};

static const struct ctlv_node_schema mr_extend_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_extend_reply_field_node_schemas,
	.fields_count = 3,
};

/* root, fields */
static const struct ctlv_node_schema device_state_changed_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema device_state_changed_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = device_state_changed_reply_field_node_schemas,
	.fields_count = 1,
};

/* mr-values, fields */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_mr_values_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* mr-log-entry.mr, fields */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_mr_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 23,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
};

/* mr-log-entry.mr, optional fields */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_mr_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-log-entry.ima, fields */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_ima_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* mr-log-entry.record, fields */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_record_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* mr-log-entry-appended entry, fields */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* mr-log-entry-appended entry, optional fields */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 23,
		.fields = mr_log_entry_appended_reply_entry_mr_values_field_node_schemas,
		.fields_count = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_entry_appended_reply_entry_mr_field_node_schemas,
		.fields_count = 3,
		.optional_fields = mr_log_entry_appended_reply_entry_mr_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_entry_appended_reply_entry_ima_field_node_schemas,
		.fields_count = 3,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_entry_appended_reply_entry_record_field_node_schemas,
		.fields_count = 2,
	},
};

/* root, fields */
static const struct ctlv_node_schema mr_log_entry_appended_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_entry_appended_reply_entry_field_node_schemas,
		.fields_count = 4,
		.optional_fields = mr_log_entry_appended_reply_entry_optional_field_node_schemas,
		.optional_fields_count = 4,
	},
};

static const struct ctlv_node_schema mr_log_entry_appended_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = mr_log_entry_appended_reply_field_node_schemas,
	.fields_count = 1,
};

/* root, fields */
static const struct ctlv_node_schema evidence_accept_request_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_ACCEPT_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_accept_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = evidence_accept_request_field_node_schemas,
	.fields_count = 2,
};

/* root, fields */
static const struct ctlv_node_schema evidence_accept_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_accept_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = evidence_accept_reply_field_node_schemas,
	.fields_count = 1,
};

static const struct ctlv_node_schema devsec_state_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
};

/* root, fields */
static const struct ctlv_node_schema devsec_state_get_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema devsec_state_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVSEC_STATE_GET_TDISP_STATE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4,
	},
};

static const struct ctlv_node_schema devsec_state_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = devsec_state_get_reply_field_node_schemas,
	.fields_count = 1,
	.optional_fields = devsec_state_get_reply_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* root, fields */
static const struct ctlv_node_schema evidence_state_changed_reply_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema evidence_state_changed_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_EVIDENCE_STATE_CHANGED_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_state_changed_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.fields = evidence_state_changed_reply_field_node_schemas,
	.fields_count = 1,
	.optional_fields = evidence_state_changed_reply_optional_field_node_schemas,
	.optional_fields_count = 1,
};

/* mr-log-get selection, fields */
static const struct ctlv_node_schema mr_log_get_request_selection_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema mr_log_get_request_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 256,
		.fields = mr_log_get_request_selection_field_node_schemas,
		.fields_count = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_START_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_END_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema mr_log_get_request_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = mr_log_get_request_optional_field_node_schemas,
	.optional_fields_count = 3,
};

/* mr-values, fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_mr_values_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_VALUE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* mr-log-entry.mr, fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_mr_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 23,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = HASH_ALGO_MD4,
		.max = HASH_ALGO__LAST - 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 3,
	},
};

/* mr-log-entry.mr, optional fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_mr_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-log-entry.ima, fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_ima_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* mr-log-entry.record, fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_record_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* mr-log-get entries, fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* mr-log-get entries, optional fields */
static const struct ctlv_node_schema mr_log_get_reply_entries_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_VALUES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 23,
		.fields = mr_log_get_reply_entries_mr_values_field_node_schemas,
		.fields_count = 2,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_get_reply_entries_mr_field_node_schemas,
		.fields_count = 3,
		.optional_fields = mr_log_get_reply_entries_mr_optional_field_node_schemas,
		.optional_fields_count = 1,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_get_reply_entries_ima_field_node_schemas,
		.fields_count = 3,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 1,
		.fields = mr_log_get_reply_entries_record_field_node_schemas,
		.fields_count = 2,
	},
};

/* root, optional fields */
static const struct ctlv_node_schema mr_log_get_reply_optional_field_node_schemas[] = {
	{
		.type = CTLV_ATTESTATION_T_MR_START_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_END_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.type = CTLV_ATTESTATION_T_MR_LOG_GET_ENTRIES,
		.kind = CTLV_SCHEMA_RECORD,
		.max_occurrences = 0, /* unlimited */
		.fields = mr_log_get_reply_entries_field_node_schemas,
		.fields_count = 4,
		.optional_fields = mr_log_get_reply_entries_optional_field_node_schemas,
		.optional_fields_count = 4,
	},
};

static const struct ctlv_node_schema mr_log_get_reply_node_schema = {
	.type = CTLV_TYPE_ROOT,
	.kind = CTLV_SCHEMA_RECORD,
	.max_occurrences = 1,
	.optional_fields = mr_log_get_reply_optional_field_node_schemas,
	.optional_fields_count = 3,
};

/* Effective ops in declaration order. */
static const struct ctlv_op_spec ctlv_attestation_tdx_op_specs[] = {
	{
		.op_id = CTLV_ATTESTATION_OP_DEVICE_GET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.action_reply_max_len = 16856,
		.error_info_max_len = 456,
		.request = &device_get_request_node_schema,
		.reply = &device_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_GET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 23216,
		.action_reply_max_len = 3416,
		.error_info_max_len = 456,
		.request = &evidence_get_request_node_schema,
		.reply = &evidence_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 40,
		.query_reply_min_len = 184,
		.query_reply_suggested_len = 4096,
		.error_info_max_len = 456,
		.request = &mr_get_request_node_schema,
		.reply = &mr_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_READ,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 80,
		.query_reply_min_len = 112,
		.query_reply_suggested_len = 4096,
		.error_info_max_len = 456,
		.request = &mr_read_request_node_schema,
		.reply = &mr_read_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_EXTEND,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 7232,
		.action_reply_max_len = 2824,
		.error_info_max_len = 456,
		.request = &mr_extend_request_node_schema,
		.reply = &mr_extend_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_max_len = 64,
		.reply = &device_state_changed_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_max_len = 73704,
		.reply = &mr_log_entry_appended_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 64,
		.action_reply_max_len = 40,
		.error_info_max_len = 456,
		.request = &evidence_accept_request_node_schema,
		.reply = &evidence_accept_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_DEVSEC_STATE_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.query_reply_min_len = 40,
		.query_reply_suggested_len = 1024,
		.error_info_max_len = 456,
		.request = &devsec_state_get_request_node_schema,
		.reply = &devsec_state_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_max_len = 88,
		.reply = &evidence_state_changed_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_LOG_GET,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 10304,
		.query_reply_min_len = 65600,
		.query_reply_suggested_len = 131072,
		.error_info_max_len = 456,
		.request = &mr_log_get_request_node_schema,
		.reply = &mr_log_get_reply_node_schema,
	},
};

/* Family specification descriptor. */
const struct ctlv_family_spec ctlv_attestation_tdx_spec = {
	.name = "ctlv-attestation-tdx",
	.family_id = 2,
	.parent = &ctlv_attestation_spec,
	.ops = ctlv_attestation_tdx_op_specs,
	.ops_count = 11,
};
