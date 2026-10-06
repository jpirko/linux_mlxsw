// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_attestation_sim_user_gen.h"

/* Effective request and reply schemas. */

static const struct ctlv_node_schema device_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* cap.object, attrs */
static const struct ctlv_node_schema device_get_reply_caps_evidence_object_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_BLOB_CLASS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 9,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_REPRESENTATION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 2,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MAX_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* cap.object, optional attrs */
static const struct ctlv_node_schema
	device_get_reply_caps_evidence_object_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_BLOB_INSTANCE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* cap.evidence, attrs */
static const struct ctlv_node_schema device_get_reply_caps_evidence_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MIN_CHALLENGE_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MAX_CHALLENGE_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MAX_TOTAL_OUTPUT_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_OBJECT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.attrs = device_get_reply_caps_evidence_object_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = device_get_reply_caps_evidence_object_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
};

/* cap.evidence, optional attrs */
static const struct ctlv_node_schema device_get_reply_caps_evidence_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_EVIDENCE_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 15,
	},
};

/* cap.mr, attrs */
static const struct ctlv_node_schema device_get_reply_caps_mr_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MR_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* cap.devsec, attrs */
static const struct ctlv_node_schema device_get_reply_caps_devsec_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_DEVSEC_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* device.caps, optional attrs */
static const struct ctlv_node_schema device_get_reply_caps_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_EVIDENCE,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_caps_evidence_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = device_get_reply_caps_evidence_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_MR,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_caps_mr_attr_node_schemas,
		.attrs_count = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_CAP_DEVSEC,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_caps_devsec_attr_node_schemas,
		.attrs_count = 1,
	},
};

/* device.info, attrs */
static const struct ctlv_node_schema device_get_reply_info_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_SIM_ATTR_SIM_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 63,
	},
};

/* root, attrs */
static const struct ctlv_node_schema device_get_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_CAPS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = device_get_reply_caps_optional_attr_node_schemas,
		.optional_attrs_count = 3,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_INFO,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_info_attr_node_schemas,
		.attrs_count = 1,
	},
};

static const struct ctlv_node_schema device_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = device_get_reply_attr_node_schemas,
	.attrs_count = 3,
};

/* evidence.object, attrs */
static const struct ctlv_node_schema evidence_get_request_object_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 9,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_DATA,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0xffffffffULL,
		.blob_alignment = 1,
	},
};

/* evidence.object, optional attrs */
static const struct ctlv_node_schema evidence_get_request_object_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* evidence.selection, attrs */
static const struct ctlv_node_schema evidence_get_request_selection_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* root, attrs */
static const struct ctlv_node_schema evidence_get_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.attrs = evidence_get_request_object_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = evidence_get_request_object_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema evidence_get_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.attrs = evidence_get_request_selection_attr_node_schemas,
		.attrs_count = 2,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 16,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema evidence_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_get_request_attr_node_schemas,
	.attrs_count = 2,
	.optional_attrs = evidence_get_request_optional_attr_node_schemas,
	.optional_attrs_count = 7,
};

/* root, attrs */
static const struct ctlv_node_schema evidence_get_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT_LENGTH,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 16,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema evidence_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_get_reply_attr_node_schemas,
	.attrs_count = 2,
	.optional_attrs = evidence_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

/* root, attrs */
static const struct ctlv_node_schema mr_get_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema mr_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_get_request_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema mr_get_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema mr_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

static const struct ctlv_node_schema mr_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_get_reply_attr_node_schemas,
	.attrs_count = 3,
	.optional_attrs = mr_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

static const struct ctlv_node_schema mr_dump_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* mr.item, attrs */
static const struct ctlv_node_schema mr_dump_reply_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* mr.item, optional attrs */
static const struct ctlv_node_schema mr_dump_reply_item_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* root, attrs */
static const struct ctlv_node_schema mr_dump_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = mr_dump_reply_item_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = mr_dump_reply_item_optional_attr_node_schemas,
		.optional_attrs_count = 2,
	},
};

static const struct ctlv_node_schema mr_dump_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_dump_reply_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema mr_read_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema mr_read_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_OPTIONS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

static const struct ctlv_node_schema mr_read_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_read_request_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = mr_read_request_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

/* mr.value, attrs */
static const struct ctlv_node_schema mr_read_reply_value_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* root, attrs */
static const struct ctlv_node_schema mr_read_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_VALUE,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_read_reply_value_attr_node_schemas,
		.attrs_count = 2,
	},
};

static const struct ctlv_node_schema mr_read_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_read_reply_attr_node_schemas,
	.attrs_count = 3,
};

/* mr.value, attrs */
static const struct ctlv_node_schema mr_extend_request_value_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* record.item, attrs */
static const struct ctlv_node_schema mr_extend_request_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* root, attrs */
static const struct ctlv_node_schema mr_extend_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_VALUE,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_extend_request_value_attr_node_schemas,
		.attrs_count = 2,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema mr_extend_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_OPTIONS,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_extend_request_item_attr_node_schemas,
		.attrs_count = 2,
	},
};

static const struct ctlv_node_schema mr_extend_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_extend_request_attr_node_schemas,
	.attrs_count = 2,
	.optional_attrs = mr_extend_request_optional_attr_node_schemas,
	.optional_attrs_count = 3,
};

/* root, attrs */
static const struct ctlv_node_schema mr_extend_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema mr_extend_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_extend_reply_attr_node_schemas,
	.attrs_count = 2,
};

/* root, attrs */
static const struct ctlv_node_schema device_state_changed_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema device_state_changed_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = device_state_changed_reply_attr_node_schemas,
	.attrs_count = 1,
};

/* mr.value, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_value_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* log.mr, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_mr_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* log.mr, optional attrs */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_mr_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* log.ima, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_ima_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_VIOLATION,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* record.item, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* notification.entry, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_entry_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_EVENT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* notification.entry, optional attrs */
static const struct ctlv_node_schema
	mr_log_entry_appended_reply_entry_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_VALUE,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_log_entry_appended_reply_entry_value_attr_node_schemas,
		.attrs_count = 2,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_MR,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_mr_attr_node_schemas,
		.attrs_count = 3,
		.optional_attrs = mr_log_entry_appended_reply_entry_mr_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_IMA,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_ima_attr_node_schemas,
		.attrs_count = 3,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_item_attr_node_schemas,
		.attrs_count = 2,
	},
};

/* root, attrs */
static const struct ctlv_node_schema mr_log_entry_appended_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = mr_log_entry_appended_reply_entry_optional_attr_node_schemas,
		.optional_attrs_count = 4,
	},
};

static const struct ctlv_node_schema mr_log_entry_appended_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_log_entry_appended_reply_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema evidence_accept_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_accept_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_accept_request_attr_node_schemas,
	.attrs_count = 2,
};

static const struct ctlv_node_schema evidence_accept_clear_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

static const struct ctlv_node_schema devsec_state_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, attrs */
static const struct ctlv_node_schema devsec_state_get_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema devsec_state_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 5,
	},
};

static const struct ctlv_node_schema devsec_state_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_state_get_reply_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = devsec_state_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 4,
};

/* root, attrs */
static const struct ctlv_node_schema devsec_run_request_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema devsec_run_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_run_request_attr_node_schemas,
	.attrs_count = 2,
};

/* root, attrs */
static const struct ctlv_node_schema devsec_run_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema devsec_run_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_run_reply_attr_node_schemas,
	.attrs_count = 1,
};

/* root, attrs */
static const struct ctlv_node_schema evidence_state_changed_reply_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema evidence_state_changed_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema evidence_state_changed_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_state_changed_reply_attr_node_schemas,
	.attrs_count = 1,
	.optional_attrs = evidence_state_changed_reply_optional_attr_node_schemas,
	.optional_attrs_count = 2,
};

/* mr.selection, attrs */
static const struct ctlv_node_schema mr_log_get_request_selection_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema mr_log_get_request_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_SELECTION,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.attrs = mr_log_get_request_selection_attr_node_schemas,
		.attrs_count = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_START_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_END_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

static const struct ctlv_node_schema mr_log_get_request_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = mr_log_get_request_optional_attr_node_schemas,
	.optional_attrs_count = 3,
};

/* mr.value, attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_value_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* log.mr, attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_mr_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_CAPS,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* log.mr, optional attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_mr_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* log.ima, attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_ima_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_VIOLATION,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* record.item, attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_item_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_FORMAT,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_DATA,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN_LENGTH | CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* log.entry, attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_ID,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_EVENT,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* log.entry, optional attrs */
static const struct ctlv_node_schema mr_log_get_reply_entry_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_VALUE,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_log_get_reply_entry_value_attr_node_schemas,
		.attrs_count = 2,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_MR,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_mr_attr_node_schemas,
		.attrs_count = 3,
		.optional_attrs = mr_log_get_reply_entry_mr_optional_attr_node_schemas,
		.optional_attrs_count = 1,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_IMA,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_ima_attr_node_schemas,
		.attrs_count = 3,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_RECORD_ITEM,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_item_attr_node_schemas,
		.attrs_count = 2,
	},
};

/* root, optional attrs */
static const struct ctlv_node_schema mr_log_get_reply_optional_attr_node_schemas[] = {
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_START_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_MR_END_GENERATION,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	{
		.attr_id = CTLV_ATTESTATION_ATTR_LOG_ENTRY,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = mr_log_get_reply_entry_attr_node_schemas,
		.attrs_count = 4,
		.optional_attrs = mr_log_get_reply_entry_optional_attr_node_schemas,
		.optional_attrs_count = 4,
	},
};

static const struct ctlv_node_schema mr_log_get_reply_node_schema = {
	.attr_id = CTLV_ATTR_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = mr_log_get_reply_optional_attr_node_schemas,
	.optional_attrs_count = 3,
};

/* Effective ops in declaration order. */
static const struct ctlv_op_spec ctlv_attestation_sim_op_specs[] = {
	{
		.op_id = CTLV_ATTESTATION_OP_DEVICE_GET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.action_reply_max_len = 13016,
		.request = &device_get_request_node_schema,
		.reply = &device_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_GET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 26800,
		.action_reply_max_len = 472,
		.request = &evidence_get_request_node_schema,
		.reply = &evidence_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_GET,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 40,
		.action_reply_max_len = 744,
		.request = &mr_get_request_node_schema,
		.reply = &mr_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_DUMP,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.query_reply_min_len = 184,
		.query_reply_suggested_len = 4096,
		.request = &mr_dump_request_node_schema,
		.reply = &mr_dump_reply_node_schema,
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
		.request = &mr_read_request_node_schema,
		.reply = &mr_read_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_EXTEND,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 6880,
		.action_reply_max_len = 64,
		.request = &mr_extend_request_node_schema,
		.reply = &mr_extend_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 64,
		.reply = &device_state_changed_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 73416,
		.reply = &mr_log_entry_appended_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 64,
		.request = &evidence_accept_request_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_CLEAR,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.request = &evidence_accept_clear_request_node_schema,
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
		.request = &devsec_state_get_request_node_schema,
		.reply = &devsec_state_get_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_DEVSEC_RUN,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 64,
		.action_reply_max_len = 40,
		.request = &devsec_run_request_node_schema,
		.reply = &devsec_run_reply_node_schema,
	},
	{
		.op_id = CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
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
		.request = &mr_log_get_request_node_schema,
		.reply = &mr_log_get_reply_node_schema,
	},
};

/* Family specification descriptor. */
const struct ctlv_family_spec ctlv_attestation_sim_spec = {
	.name = "ctlv-attestation-sim",
	.family_id = 5,
	.parent = &ctlv_attestation_spec,
	.ops = ctlv_attestation_sim_op_specs,
	.ops_count = 14,
};
