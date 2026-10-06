/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-attestation CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_GEN_H
#define _LINUX_CTLV_ATTESTATION_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/ctlv/ctlv_attestation_gen.h>

/* Extension points, as a descendant's fill names one. */
#define CTLV_ATTESTATION_EXT_DEVICE_INFO 1

/* Enum extension points, numbered apart from the others. */
#define CTLV_ATTESTATION_ENUM_EXT_EVIDENCE_VENDOR_FORMAT 1

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_attestation_op_schemas[];

extern const struct ctlv_family_schema ctlv_attestation_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_attestation_device_get_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request,
				       struct ctlv_cursor *reply);
int ctlv_attestation_evidence_get_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *reply);
int ctlv_attestation_mr_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_read_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_extend_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_log_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply);

extern const struct ctlv_family ctlv_attestation_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Optional publication and dynamic availability helpers. */
CTLV_OP_PROVIDE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_ENABLE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_DISABLE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_PROVIDE(ctlv_attestation_mr_get, CTLV_ATTESTATION_OP_MR_GET)
CTLV_OP_PROVIDE(ctlv_attestation_mr_read, CTLV_ATTESTATION_OP_MR_READ)
CTLV_OP_PROVIDE(ctlv_attestation_mr_extend, CTLV_ATTESTATION_OP_MR_EXTEND)
CTLV_OP_PROVIDE(ctlv_attestation_mr_log_get, CTLV_ATTESTATION_OP_MR_LOG_GET)

/* Reading a validated message. A blob has a resolver instead of a getter. */
/* The objects to produce, each named once. */
CTLV_IN_REF(ctlv_attestation_evidence_get_objects, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS,
	    CTLV_SCHEMA_RECORD)
#define ctlv_attestation_evidence_get_objects_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_attestation_evidence_get_objects, _cursor, _record)
/* The object, by the ID device-get lists. */
CTLV_IN_REF(ctlv_attestation_evidence_get_objects_id, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_objects_id)
CTLV_IN_U64(ctlv_attestation_evidence_get_objects_id)
/*
 * Where the object is written. One too short for it fails the op with EMSGSIZE, and error-info's
 * required-length says how long it has to be.
 */
CTLV_IN_REF(ctlv_attestation_evidence_get_objects_data,
	    CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_attestation_evidence_get_objects_data)
CTLV_IN_BLOB(ctlv_attestation_evidence_get_objects_data)
/*
 * Bound into each signed object named: pass a verifier's nonce, or a hash binding one to your
 * context, never data a peer chose. Present exactly when a signed object is named, and of a length
 * each of them takes.
 */
CTLV_IN_REF(ctlv_attestation_evidence_get_challenge, CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE,
	    CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_evidence_get_challenge)
CTLV_IN_BINARY(ctlv_attestation_evidence_get_challenge)
/* Only this MR; every MR without it. */
CTLV_IN_REF(ctlv_attestation_mr_get_id, CTLV_ATTESTATION_T_MR_GET_ID, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_get_id)
CTLV_IN_U64(ctlv_attestation_mr_get_id)
/* The MR to read, by the ID mr-get lists. */
CTLV_IN_REF(ctlv_attestation_mr_read_id, CTLV_ATTESTATION_T_MR_READ_ID, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_read_id)
CTLV_IN_U64(ctlv_attestation_mr_read_id)
/* The MR to extend, by the ID mr-get lists. */
CTLV_IN_REF(ctlv_attestation_mr_extend_id, CTLV_ATTESTATION_T_MR_EXTEND_ID, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_extend_id)
CTLV_IN_U64(ctlv_attestation_mr_extend_id)
/* Values for banks of the MR, each bank once. */
CTLV_IN_REF(ctlv_attestation_mr_values, CTLV_ATTESTATION_T_MR_VALUES, CTLV_SCHEMA_RECORD)
#define ctlv_attestation_mr_values_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_attestation_mr_values, _cursor, _record)
/* The bank's algorithm. */
CTLV_IN_REF(ctlv_attestation_mr_value_hash_algo, CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
	    CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_attestation_mr_value_hash_algo)
CTLV_IN_U64(ctlv_attestation_mr_value_hash_algo)
/* The digest, as long as hash-algo's. */
CTLV_IN_REF(ctlv_attestation_mr_value_data, CTLV_ATTESTATION_T_MR_VALUE_DATA, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_mr_value_data)
CTLV_IN_BINARY(ctlv_attestation_mr_value_data)
/* Only these MRs' entries; every MR's without it. */
CTLV_IN_REF(ctlv_attestation_mr_log_get_selection, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION,
	    CTLV_SCHEMA_RECORD)
#define ctlv_attestation_mr_log_get_selection_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_attestation_mr_log_get_selection, _cursor, _record)
/* An MR's ID. */
CTLV_IN_REF(ctlv_attestation_mr_log_get_selection_id, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_log_get_selection_id)
CTLV_IN_U64(ctlv_attestation_mr_log_get_selection_id)
/* The first MR generation of the range mr-log-get selects and reports. */
CTLV_IN_REF(ctlv_attestation_mr_start_generation, CTLV_ATTESTATION_T_MR_START_GENERATION,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_start_generation)
CTLV_IN_U64(ctlv_attestation_mr_start_generation)
/* The last MR generation of that range. */
CTLV_IN_REF(ctlv_attestation_mr_end_generation, CTLV_ATTESTATION_T_MR_END_GENERATION,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_end_generation)
CTLV_IN_U64(ctlv_attestation_mr_end_generation)

/* Writing a reply. An append is atomic; a record ends with ctlv_record_end(). */
/* What the device supports, fixed for its life. */
CTLV_RECORD_START(ctlv_attestation_device_get_caps, CTLV_ATTESTATION_T_DEVICE_GET_CAPS,
		  CTLV_SCHEMA_RECORD)
/* Present when the device produces evidence. */
CTLV_RECORD_START(ctlv_attestation_device_get_caps_evidence,
		  CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE, CTLV_SCHEMA_RECORD)
/* One per object evidence-get can produce. */
CTLV_RECORD_START(ctlv_attestation_device_get_evidence_objects,
		  CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_OBJECTS, CTLV_SCHEMA_RECORD)
/* The device's handle for the object in requests. */
CTLV_PUT_U64(ctlv_attestation_device_get_objects_id, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID,
	     CTLV_SCHEMA_UINT)
/* What the object is. */
CTLV_PUT_U64(ctlv_attestation_device_get_objects_class, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS,
	     CTLV_SCHEMA_ENUM)
/* The longest the object gets; a blob this long always holds it. */
CTLV_PUT_U64(ctlv_attestation_device_get_objects_max_length,
	     CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH, CTLV_SCHEMA_UINT)
/*
 * The layouts a standard defines that the object may have. At least one of formats and
 * vendor-formats is present.
 */
CTLV_PUT_U64(ctlv_attestation_device_get_objects_formats,
	     CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_FORMATS, CTLV_SCHEMA_ENUM)
/* The layouts its vendor defines that the object may have. */
CTLV_PUT_U64(ctlv_attestation_device_get_objects_vendor_formats,
	     CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_VENDOR_FORMATS, CTLV_SCHEMA_ENUM)
/*
 * A signed object's challenge lengths: each entry is one length, or a range from min-length to
 * max-length.
 */
CTLV_RECORD_START(ctlv_attestation_device_get_objects_challenge_lengths,
		  CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CHALLENGE_LENGTHS, CTLV_SCHEMA_RECORD)
/* A single length. */
CTLV_PUT_U64(ctlv_attestation_device_get_challenge_lengths_length,
	     CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH, CTLV_SCHEMA_UINT)
/* The shortest of a range. */
CTLV_PUT_U64(ctlv_attestation_device_get_challenge_lengths_min_length,
	     CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH, CTLV_SCHEMA_UINT)
/* The longest of a range. */
CTLV_PUT_U64(ctlv_attestation_device_get_challenge_lengths_max_length,
	     CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH, CTLV_SCHEMA_UINT)
/* Present when the device has MRs. */
CTLV_RECORD_START(ctlv_attestation_device_get_caps_mr, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR,
		  CTLV_SCHEMA_RECORD)
/* The MR operations the device provides. */
CTLV_PUT_U64(ctlv_attestation_device_get_mr_flags, CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS,
	     CTLV_SCHEMA_FLAGS)
/* The device's family's fields. */
CTLV_RECORD_START(ctlv_attestation_device_get_info, CTLV_ATTESTATION_T_DEVICE_GET_INFO,
		  CTLV_SCHEMA_RECORD)
/* One per object, in request order. */
CTLV_RECORD_START(ctlv_attestation_evidence_get_results, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS,
		  CTLV_SCHEMA_RECORD)
/* The bytes written to its blob. */
CTLV_PUT_U64(ctlv_attestation_evidence_get_results_length,
	     CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH, CTLV_SCHEMA_UINT)
/*
 * The layout it came in, where a standard defines it. Exactly one of format and vendor-format is
 * present.
 */
CTLV_PUT_U64(ctlv_attestation_evidence_get_results_format,
	     CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT, CTLV_SCHEMA_ENUM)
/* The layout it came in, where its vendor defines it. */
CTLV_PUT_U64(ctlv_attestation_evidence_get_results_vendor_format,
	     CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT, CTLV_SCHEMA_ENUM)
/* The device's MR generation, which each extend bumps. */
CTLV_PUT_U64(ctlv_attestation_mr_generation, CTLV_ATTESTATION_T_MR_GENERATION, CTLV_SCHEMA_UINT)
/* One per MR, ordered by ID. */
CTLV_RECORD_START(ctlv_attestation_mr_get_items, CTLV_ATTESTATION_T_MR_GET_ITEMS,
		  CTLV_SCHEMA_RECORD)
/* The device's handle for the MR in requests. */
CTLV_PUT_U64(ctlv_attestation_mr_get_items_id, CTLV_ATTESTATION_T_MR_GET_ITEMS_ID, CTLV_SCHEMA_UINT)
/* The platform's name for the MR, such as rtmr0. */
CTLV_PUT_STRING(ctlv_attestation_mr_desc_name, CTLV_ATTESTATION_T_MR_DESC_NAME, CTLV_SCHEMA_STRING)
/* Its banks, each once, in the provider's order. */
CTLV_PUT_U64(ctlv_attestation_mr_desc_supported_hash_algorithms,
	     CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS, CTLV_SCHEMA_ENUM)
/* What it allows. */
CTLV_PUT_U64(ctlv_attestation_mr_desc_caps, CTLV_ATTESTATION_T_MR_DESC_CAPS, CTLV_SCHEMA_FLAGS)
/* The hardware's number for the MR, where it has one: 0 for rtmr0. */
CTLV_PUT_U64(ctlv_attestation_mr_desc_register_index, CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX,
	     CTLV_SCHEMA_UINT)
/* Values for banks of the MR, each bank once. */
CTLV_RECORD_START(ctlv_attestation_mr_values, CTLV_ATTESTATION_T_MR_VALUES, CTLV_SCHEMA_RECORD)
/* The bank's algorithm. */
CTLV_PUT_U64(ctlv_attestation_mr_value_hash_algo, CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO,
	     CTLV_SCHEMA_ENUM)
/* The digest, as long as hash-algo's. */
CTLV_PUT_BINARY(ctlv_attestation_mr_value_data, CTLV_ATTESTATION_T_MR_VALUE_DATA,
		CTLV_SCHEMA_BINARY)
/* The entry, as mr-log-get returns it. */
CTLV_RECORD_START(ctlv_attestation_mr_log_entry_appended_entry,
		  CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY, CTLV_SCHEMA_RECORD)
/* The ID of the MR the entry is about. */
CTLV_PUT_U64(ctlv_attestation_mr_log_entry_mr_id, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID,
	     CTLV_SCHEMA_UINT)
/* What the entry records. */
CTLV_PUT_U64(ctlv_attestation_mr_log_entry_event, CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT,
	     CTLV_SCHEMA_ENUM)
/* The MR an info entry describes. */
CTLV_RECORD_START(ctlv_attestation_mr_log_entry_mr, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR,
		  CTLV_SCHEMA_RECORD)
/* The platform's name for the MR. */
CTLV_PUT_STRING(ctlv_attestation_mr_log_entry_mr_name, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME,
		CTLV_SCHEMA_STRING)
/* Its banks, each once, in the provider's order. */
CTLV_PUT_U64(ctlv_attestation_mr_log_entry_mr_supported_hash_algorithms,
	     CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS, CTLV_SCHEMA_ENUM)
/* What it allows. */
CTLV_PUT_U64(ctlv_attestation_mr_log_entry_mr_caps, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS,
	     CTLV_SCHEMA_FLAGS)
/* The hardware's number for the MR, where it has one. */
CTLV_PUT_U64(ctlv_attestation_mr_log_entry_mr_register_index,
	     CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX, CTLV_SCHEMA_UINT)
/* IMA's measurement, on an extend entry IMA made. */
CTLV_RECORD_START(ctlv_attestation_mr_log_entry_ima, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA,
		  CTLV_SCHEMA_RECORD)
/* The IMA template, such as ima-ng. */
CTLV_PUT_STRING(ctlv_attestation_mr_log_entry_ima_template_name,
		CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME, CTLV_SCHEMA_STRING)
/* The template's fields, as IMA's binary measurement list has them. */
CTLV_PUT_BINARY(ctlv_attestation_mr_log_entry_ima_template_data,
		CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA, CTLV_SCHEMA_BINARY)
/* The entry records an IMA violation, not a measurement. */
CTLV_PUT_BOOL(ctlv_attestation_mr_log_entry_ima_violation,
	      CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION, CTLV_SCHEMA_BOOL)
/* The first MR generation of the range mr-log-get selects and reports. */
CTLV_PUT_U64(ctlv_attestation_mr_start_generation, CTLV_ATTESTATION_T_MR_START_GENERATION,
	     CTLV_SCHEMA_UINT)
/* The last MR generation of that range. */
CTLV_PUT_U64(ctlv_attestation_mr_end_generation, CTLV_ATTESTATION_T_MR_END_GENERATION,
	     CTLV_SCHEMA_UINT)
/* The entries, in journal order. */
CTLV_RECORD_START(ctlv_attestation_mr_log_get_entries, CTLV_ATTESTATION_T_MR_LOG_GET_ENTRIES,
		  CTLV_SCHEMA_RECORD)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_attestation_mr_log_entry_appended, CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_ATTESTATION_GEN_H */
