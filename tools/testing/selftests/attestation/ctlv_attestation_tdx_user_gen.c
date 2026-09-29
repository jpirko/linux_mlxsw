// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

#include "ctlv_attestation_tdx_user_gen.h"

/* What each op's messages may hold, composed as the core composes them. */

/* device-get request */
static const struct ctlv_schema_node device_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* capability.object, attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_evidence_object_attrs[] = {
	/* capability.blob-class */
	{
		.attr_id = 0x000300020000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 9,
	},
	/* capability.format */
	{
		.attr_id = 0x000300020000000dULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* capability.representation */
	{
		.attr_id = 0x000300020000000eULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 2,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	/* capability.max-length */
	{
		.attr_id = 0x0003000200000010ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* capability.object, optional attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_evidence_object_optional_attrs[] = {
	/* capability.blob-instance */
	{
		.attr_id = 0x000300020000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* capability.hash-algo */
	{
		.attr_id = 0x000300020000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* capability.evidence, attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_evidence_attrs[] = {
	/* capability.min-challenge-length */
	{
		.attr_id = 0x0003000200000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* capability.max-challenge-length */
	{
		.attr_id = 0x0003000200000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* capability.max-total-output-length */
	{
		.attr_id = 0x0003000200000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* capability.evidence-flags */
	{
		.attr_id = 0x0003000200000007ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 15,
	},
	/* capability.object */
	{
		.attr_id = 0x000300020000000aULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.attrs = device_get_reply_capabilities_evidence_object_attrs,
		.n_attrs = 4,
		.optional_attrs =
	device_get_reply_capabilities_evidence_object_optional_attrs,
		.n_optional_attrs = 2,
	},
};

/* capability.mr, attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_mr_attrs[] = {
	/* capability.mr-flags */
	{
		.attr_id = 0x0003000200000008ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* capability.devsec, attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_devsec_attrs[] = {
	/* capability.devsec-flags */
	{
		.attr_id = 0x0003000200000009ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* device.capabilities, optional attrs */
static const struct ctlv_schema_node
	device_get_reply_capabilities_optional_attrs[] = {
	/* capability.evidence */
	{
		.attr_id = 0x0003000200000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_capabilities_evidence_attrs,
		.n_attrs = 5,
	},
	/* capability.mr */
	{
		.attr_id = 0x0003000200000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_capabilities_mr_attrs,
		.n_attrs = 1,
	},
	/* capability.devsec */
	{
		.attr_id = 0x0003000200000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_capabilities_devsec_attrs,
		.n_attrs = 1,
	},
};

/* device.info, attrs */
static const struct ctlv_schema_node device_get_reply_info_attrs[] = {
	/* tdx.mrconfigid */
	{
		.attr_id = 0x0005000100000001ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
	/* tdx.mrowner */
	{
		.attr_id = 0x0005000100000002ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
	/* tdx.mrownerconfig */
	{
		.attr_id = 0x0005000100000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 48,
		.max_length = 48,
	},
};

/* root, attrs */
static const struct ctlv_schema_node device_get_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* device.capabilities */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.optional_attrs = device_get_reply_capabilities_optional_attrs,
		.n_optional_attrs = 3,
	},
	/* device.info */
	{
		.attr_id = 0x0003000100000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = device_get_reply_info_attrs,
		.n_attrs = 3,
	},
};

/* device-get reply */
static const struct ctlv_schema_node device_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = device_get_reply_attrs,
	.n_attrs = 3,
};

/* evidence.object, attrs */
static const struct ctlv_schema_node evidence_get_request_object_attrs[] = {
	/* evidence.blob-class */
	{
		.attr_id = 0x0003000300000005ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 9,
	},
	/* evidence.format */
	{
		.attr_id = 0x0003000300000007ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* evidence.representation */
	{
		.attr_id = 0x0003000300000008ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
	/* evidence.data */
	{
		.attr_id = 0x000300030000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_BLOB,
		.blob_direction = CTLV_BLOB_DIRECTION_OUTPUT,
		.blob_backings = 14,
		.blob_max_length = 0xffffffffULL,
		.blob_alignment = 1,
	},
};

/* evidence.object, optional attrs */
static const struct ctlv_schema_node
	evidence_get_request_object_optional_attrs[] = {
	/* evidence.blob-instance */
	{
		.attr_id = 0x0003000300000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.hash-algo */
	{
		.attr_id = 0x0003000300000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* evidence.selection, attrs */
static const struct ctlv_schema_node evidence_get_request_selection_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.hash-algo */
	{
		.attr_id = 0x0003000300000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
};

/* root, attrs */
static const struct ctlv_schema_node evidence_get_request_attrs[] = {
	/* evidence.challenge */
	{
		.attr_id = 0x0003000300000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
	/* evidence.object */
	{
		.attr_id = 0x0003000300000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.attrs = evidence_get_request_object_attrs,
		.n_attrs = 4,
		.optional_attrs = evidence_get_request_object_optional_attrs,
		.n_optional_attrs = 2,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node evidence_get_request_optional_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.selection */
	{
		.attr_id = 0x000300030000000bULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.attrs = evidence_get_request_selection_attrs,
		.n_attrs = 2,
	},
	/* evidence.privlevel */
	{
		.attr_id = 0x000300030000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.service-provider */
	{
		.attr_id = 0x000300030000000dULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* evidence.service-guid */
	{
		.attr_id = 0x000300030000000eULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 16,
	},
	/* evidence.service-manifest-version */
	{
		.attr_id = 0x000300030000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.options */
	{
		.attr_id = 0x0003000300000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* evidence-get request */
static const struct ctlv_schema_node evidence_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_get_request_attrs,
	.n_attrs = 2,
	.optional_attrs = evidence_get_request_optional_attrs,
	.n_optional_attrs = 7,
};

/* root, attrs */
static const struct ctlv_schema_node evidence_get_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.object-length */
	{
		.attr_id = 0x0003000300000011ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 16,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node evidence_get_reply_optional_attrs[] = {
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* evidence-get reply */
static const struct ctlv_schema_node evidence_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_get_reply_attrs,
	.n_attrs = 2,
	.optional_attrs = evidence_get_reply_optional_attrs,
	.n_optional_attrs = 2,
};

/* root, attrs */
static const struct ctlv_schema_node evidence_accept_request_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* evidence-accept request */
static const struct ctlv_schema_node evidence_accept_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_accept_request_attrs,
	.n_attrs = 2,
};

/* evidence-accept-clear request */
static const struct ctlv_schema_node evidence_accept_clear_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, attrs */
static const struct ctlv_schema_node mr_get_request_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-get request */
static const struct ctlv_schema_node mr_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_get_request_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node mr_get_reply_attrs[] = {
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
	/* mr.state-flags */
	{
		.attr_id = 0x0003000400000006ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node mr_get_reply_optional_attrs[] = {
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-get reply */
static const struct ctlv_schema_node mr_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_get_reply_attrs,
	.n_attrs = 4,
	.optional_attrs = mr_get_reply_optional_attrs,
	.n_optional_attrs = 1,
};

/* mr-dump request */
static const struct ctlv_schema_node mr_dump_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* mr.item, attrs */
static const struct ctlv_schema_node mr_dump_reply_item_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
	/* mr.state-flags */
	{
		.attr_id = 0x0003000400000006ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 1,
	},
};

/* mr.item, optional attrs */
static const struct ctlv_schema_node mr_dump_reply_item_optional_attrs[] = {
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, attrs */
static const struct ctlv_schema_node mr_dump_reply_attrs[] = {
	/* mr.item */
	{
		.attr_id = 0x0003000400000007ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = mr_dump_reply_item_attrs,
		.n_attrs = 5,
		.optional_attrs = mr_dump_reply_item_optional_attrs,
		.n_optional_attrs = 1,
	},
};

/* mr-dump reply */
static const struct ctlv_schema_node mr_dump_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_dump_reply_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node mr_read_request_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node mr_read_request_optional_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.options */
	{
		.attr_id = 0x0003000400000008ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* mr-read request */
static const struct ctlv_schema_node mr_read_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_read_request_attrs,
	.n_attrs = 1,
	.optional_attrs = mr_read_request_optional_attrs,
	.n_optional_attrs = 2,
};

/* mr.value, attrs */
static const struct ctlv_schema_node mr_read_reply_value_attrs[] = {
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* root, attrs */
static const struct ctlv_schema_node mr_read_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_read_reply_value_attrs,
		.n_attrs = 2,
	},
};

/* mr-read reply */
static const struct ctlv_schema_node mr_read_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_read_reply_attrs,
	.n_attrs = 3,
};

/* mr.value, attrs */
static const struct ctlv_schema_node mr_extend_request_value_attrs[] = {
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* record.item, attrs */
static const struct ctlv_schema_node mr_extend_request_item_attrs[] = {
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* root, attrs */
static const struct ctlv_schema_node mr_extend_request_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_extend_request_value_attrs,
		.n_attrs = 2,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node mr_extend_request_optional_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.options */
	{
		.attr_id = 0x0003000400000008ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_extend_request_item_attrs,
		.n_attrs = 2,
	},
};

/* mr-extend request */
static const struct ctlv_schema_node mr_extend_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_extend_request_attrs,
	.n_attrs = 2,
	.optional_attrs = mr_extend_request_optional_attrs,
	.n_optional_attrs = 3,
};

/* root, attrs */
static const struct ctlv_schema_node mr_extend_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-extend reply */
static const struct ctlv_schema_node mr_extend_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_extend_reply_attrs,
	.n_attrs = 2,
};

/* mr.selection, attrs */
static const struct ctlv_schema_node mr_log_get_request_selection_attrs[] = {
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node mr_log_get_request_optional_attrs[] = {
	/* mr.selection */
	{
		.attr_id = 0x000300040000000cULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.attrs = mr_log_get_request_selection_attrs,
		.n_attrs = 1,
	},
	/* mr.start-generation */
	{
		.attr_id = 0x000300040000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.end-generation */
	{
		.attr_id = 0x000300040000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* mr-log-get request */
static const struct ctlv_schema_node mr_log_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = mr_log_get_request_optional_attrs,
	.n_optional_attrs = 3,
};

/* mr.value, attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_value_attrs[] = {
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* record.item, attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_item_attrs[] = {
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* log.mr, attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_mr_attrs[] = {
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* log.mr, optional attrs */
static const struct ctlv_schema_node
	mr_log_get_reply_entry_mr_optional_attrs[] = {
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* log.ima, attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_ima_attrs[] = {
	/* log.template-name */
	{
		.attr_id = 0x0003000600000005ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* log.template-data */
	{
		.attr_id = 0x0003000600000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	/* log.violation */
	{
		.attr_id = 0x0003000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* log.entry, attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_attrs[] = {
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* log.event */
	{
		.attr_id = 0x0003000600000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* log.entry, optional attrs */
static const struct ctlv_schema_node mr_log_get_reply_entry_optional_attrs[] = {
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_log_get_reply_entry_value_attrs,
		.n_attrs = 2,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_item_attrs,
		.n_attrs = 2,
	},
	/* log.mr */
	{
		.attr_id = 0x0003000600000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_mr_attrs,
		.n_attrs = 3,
		.optional_attrs = mr_log_get_reply_entry_mr_optional_attrs,
		.n_optional_attrs = 1,
	},
	/* log.ima */
	{
		.attr_id = 0x0003000600000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_get_reply_entry_ima_attrs,
		.n_attrs = 3,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node mr_log_get_reply_optional_attrs[] = {
	/* mr.start-generation */
	{
		.attr_id = 0x000300040000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.end-generation */
	{
		.attr_id = 0x000300040000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* log.entry */
	{
		.attr_id = 0x0003000600000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = mr_log_get_reply_entry_attrs,
		.n_attrs = 4,
		.optional_attrs = mr_log_get_reply_entry_optional_attrs,
		.n_optional_attrs = 4,
	},
};

/* mr-log-get reply */
static const struct ctlv_schema_node mr_log_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = mr_log_get_reply_optional_attrs,
	.n_optional_attrs = 3,
};

/* devsec-state-get request */
static const struct ctlv_schema_node devsec_state_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, attrs */
static const struct ctlv_schema_node devsec_state_get_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node devsec_state_get_reply_optional_attrs[] = {
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.accepted-generation */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* devsec.tdisp-state */
	{
		.attr_id = 0x0003000700000001ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 4,
	},
	/* devsec.device-trust */
	{
		.attr_id = 0x0003000700000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 5,
	},
};

/* devsec-state-get reply */
static const struct ctlv_schema_node devsec_state_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_state_get_reply_attrs,
	.n_attrs = 1,
	.optional_attrs = devsec_state_get_reply_optional_attrs,
	.n_optional_attrs = 4,
};

/* root, attrs */
static const struct ctlv_schema_node devsec_run_request_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* devsec-run request */
static const struct ctlv_schema_node devsec_run_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_run_request_attrs,
	.n_attrs = 2,
};

/* root, attrs */
static const struct ctlv_schema_node devsec_run_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* devsec-run reply */
static const struct ctlv_schema_node devsec_run_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = devsec_run_reply_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node device_state_changed_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* device-state-changed reply */
static const struct ctlv_schema_node device_state_changed_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = device_state_changed_reply_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node evidence_state_changed_reply_attrs[] = {
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node
	evidence_state_changed_reply_optional_attrs[] = {
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* evidence.accepted-generation */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* evidence-state-changed reply */
static const struct ctlv_schema_node evidence_state_changed_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = evidence_state_changed_reply_attrs,
	.n_attrs = 1,
	.optional_attrs = evidence_state_changed_reply_optional_attrs,
	.n_optional_attrs = 2,
};

/* mr.value, attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_value_attrs[] = {
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 16,
		.max_length = 64,
	},
};

/* record.item, attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_item_attrs[] = {
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 4096,
	},
};

/* log.mr, attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_mr_attrs[] = {
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 20,
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_FLAGS_MASK,
		.flags_mask = 7,
	},
};

/* log.mr, optional attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_mr_optional_attrs[] = {
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
};

/* log.ima, attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_ima_attrs[] = {
	/* log.template-name */
	{
		.attr_id = 0x0003000600000005ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 1,
		.max_length = 128,
	},
	/* log.template-data */
	{
		.attr_id = 0x0003000600000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 0x10000ULL,
	},
	/* log.violation */
	{
		.attr_id = 0x0003000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* notification.entry, attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_attrs[] = {
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = 0xffffffffULL,
	},
	/* log.event */
	{
		.attr_id = 0x0003000600000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
	},
};

/* notification.entry, optional attrs */
static const struct ctlv_schema_node
	mr_log_entry_appended_reply_entry_optional_attrs[] = {
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.attrs = mr_log_entry_appended_reply_entry_value_attrs,
		.n_attrs = 2,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_item_attrs,
		.n_attrs = 2,
	},
	/* log.mr */
	{
		.attr_id = 0x0003000600000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_mr_attrs,
		.n_attrs = 3,
		.optional_attrs =
			mr_log_entry_appended_reply_entry_mr_optional_attrs,
		.n_optional_attrs = 1,
	},
	/* log.ima */
	{
		.attr_id = 0x0003000600000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_ima_attrs,
		.n_attrs = 3,
	},
};

/* root, attrs */
static const struct ctlv_schema_node mr_log_entry_appended_reply_attrs[] = {
	/* notification.entry */
	{
		.attr_id = 0x0003000800000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = mr_log_entry_appended_reply_entry_attrs,
		.n_attrs = 4,
		.optional_attrs =
			mr_log_entry_appended_reply_entry_optional_attrs,
		.n_optional_attrs = 4,
	},
};

/* mr-log-entry-appended reply */
static const struct ctlv_schema_node mr_log_entry_appended_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = mr_log_entry_appended_reply_attrs,
	.n_attrs = 1,
};

/* Every op a device of this family answers, in chain declaration order. */
static const struct ctlv_op_spec ctlv_attestation_tdx_ctlv_op_specs[] = {
	/* device-get */
	{
		.op_id = 0x0003000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.action_reply_max_len = 13128,
		.request = &device_get_request,
		.reply = &device_get_reply,
	},
	/* evidence-get */
	{
		.op_id = 0x0003000000000002ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 26800,
		.action_reply_max_len = 472,
		.request = &evidence_get_request,
		.reply = &evidence_get_reply,
	},
	/* evidence-accept */
	{
		.op_id = 0x0003000000000003ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 64,
		.request = &evidence_accept_request,
	},
	/* evidence-accept-clear */
	{
		.op_id = 0x0003000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.request = &evidence_accept_clear_request,
	},
	/* mr-get */
	{
		.op_id = 0x0003000000000005ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 40,
		.action_reply_max_len = 744,
		.request = &mr_get_request,
		.reply = &mr_get_reply,
	},
	/* mr-dump */
	{
		.op_id = 0x0003000000000006ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.query_reply_min_len = 184,
		.query_reply_suggested_len = 4096,
		.request = &mr_dump_request,
		.reply = &mr_dump_reply,
	},
	/* mr-read */
	{
		.op_id = 0x0003000000000007ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 80,
		.query_reply_min_len = 112,
		.query_reply_suggested_len = 4096,
		.request = &mr_read_request,
		.reply = &mr_read_reply,
	},
	/* mr-extend */
	{
		.op_id = 0x0003000000000008ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16960,
		.action_reply_max_len = 64,
		.request = &mr_extend_request,
		.reply = &mr_extend_reply,
	},
	/* mr-log-get */
	{
		.op_id = 0x0003000000000009ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 10304,
		.query_reply_min_len = 65600,
		.query_reply_suggested_len = 131072,
		.request = &mr_log_get_request,
		.reply = &mr_log_get_reply,
	},
	/* devsec-state-get */
	{
		.op_id = 0x000300000000000aULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request_max_len = 16,
		.query_reply_min_len = 40,
		.query_reply_suggested_len = 1024,
		.request = &devsec_state_get_request,
		.reply = &devsec_state_get_reply,
	},
	/* devsec-run */
	{
		.op_id = 0x000300000000000bULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request_max_len = 64,
		.action_reply_max_len = 40,
		.request = &devsec_run_request,
		.reply = &devsec_run_reply,
	},
	/* device-state-changed */
	{
		.op_id = 0x000300000000000cULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 64,
		.reply = &device_state_changed_reply,
	},
	/* evidence-state-changed */
	{
		.op_id = 0x000300000000000dULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
		.reply = &evidence_state_changed_reply,
	},
	/* mr-log-entry-appended */
	{
		.op_id = 0x000300000000000eULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 83496,
		.reply = &mr_log_entry_appended_reply,
	},
};

/* What a device of this family is, for a caller holding one against it. */
const struct ctlv_family_spec ctlv_attestation_tdx_ctlv_spec = {
	.name = "ctlv-attestation-tdx",
	.family_id = 5,
	.parent = &ctlv_attestation_ctlv_spec,
	.op = ctlv_attestation_tdx_ctlv_op_specs,
	.n_op = 14,
};
