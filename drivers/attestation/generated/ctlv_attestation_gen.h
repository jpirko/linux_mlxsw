/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-attestation CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_GEN_H
#define _LINUX_CTLV_ATTESTATION_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_attestation_gen.h>

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_attestation_op_schemas[];

extern const struct ctlv_family_schema ctlv_attestation_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_attestation_device_get_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request,
				       struct ctlv_cursor *reply);
int ctlv_attestation_evidence_get_action(void *priv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *reply);
int ctlv_attestation_mr_get_action(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_read_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_mr_extend_action(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_attestation_evidence_accept_action(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply);
int ctlv_attestation_evidence_accept_clear_action(void *priv, struct ctlv_op_ctx *ctx,
						  const struct ctlv_cursor *request,
						  struct ctlv_cursor *reply);
int ctlv_attestation_devsec_state_get_query(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply);
int ctlv_attestation_devsec_run_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request,
				       struct ctlv_cursor *reply);
int ctlv_attestation_mr_log_get_query(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply);

/* Family-wide hooks around every effective non-core op. */
int ctlv_attestation_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_attestation_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_attestation_priv_size(void);
int ctlv_attestation_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
int ctlv_attestation_post_init(struct ctlv_device *ctlvdev, void *priv);
void ctlv_attestation_pre_fini(struct ctlv_device *ctlvdev, void *priv);
void ctlv_attestation_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_attestation_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Optional publication and dynamic availability helpers. */
CTLV_OP_PROVIDE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_ENABLE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_DISABLE(ctlv_attestation_evidence_get, CTLV_ATTESTATION_OP_EVIDENCE_GET)
CTLV_OP_PROVIDE(ctlv_attestation_mr_get, CTLV_ATTESTATION_OP_MR_GET)
CTLV_OP_PROVIDE(ctlv_attestation_mr_dump, CTLV_ATTESTATION_OP_MR_DUMP)
CTLV_OP_PROVIDE(ctlv_attestation_mr_read, CTLV_ATTESTATION_OP_MR_READ)
CTLV_OP_PROVIDE(ctlv_attestation_mr_extend, CTLV_ATTESTATION_OP_MR_EXTEND)
CTLV_OP_PROVIDE(ctlv_attestation_evidence_accept, CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT)
CTLV_OP_PROVIDE(ctlv_attestation_evidence_accept_clear, CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_CLEAR)
CTLV_OP_PROVIDE(ctlv_attestation_devsec_state_get, CTLV_ATTESTATION_OP_DEVSEC_STATE_GET)
CTLV_OP_PROVIDE(ctlv_attestation_devsec_run, CTLV_ATTESTATION_OP_DEVSEC_RUN)
CTLV_OP_ENABLE(ctlv_attestation_devsec_run, CTLV_ATTESTATION_OP_DEVSEC_RUN)
CTLV_OP_DISABLE(ctlv_attestation_devsec_run, CTLV_ATTESTATION_OP_DEVSEC_RUN)
CTLV_OP_PROVIDE(ctlv_attestation_mr_log_get, CTLV_ATTESTATION_OP_MR_LOG_GET)

/* Reading a validated message. A blob has a resolver instead of a getter. */
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_challenge,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE, 0, 0,
	    CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_challenge)
CTLV_IN_BINARY(ctlv_attestation_evidence_get_evidence_challenge)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_object, &ctlv_attestation_op_schemas[1].request,
	    CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT, 0, 1, CTLV_SCHEMA_NEST)
#define ctlv_attestation_evidence_get_evidence_object_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_attestation_evidence_get_evidence_object, _attr, _nest)
CTLV_IN_REF(ctlv_attestation_evidence_get_device_generation,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 2,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_device_generation)
CTLV_IN_U64(ctlv_attestation_evidence_get_device_generation)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_selection,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION, 0, 3,
	    CTLV_SCHEMA_NEST)
#define ctlv_attestation_evidence_get_evidence_selection_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_attestation_evidence_get_evidence_selection, _attr, _nest)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_privlevel,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL, 0, 4,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_privlevel)
CTLV_IN_U64(ctlv_attestation_evidence_get_evidence_privlevel)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_service_provider,
	    &ctlv_attestation_op_schemas[1].request,
	    CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER, 0, 5, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_service_provider)
CTLV_IN_STRING(ctlv_attestation_evidence_get_evidence_service_provider)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_service_guid,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID, 0,
	    6, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_service_guid)
CTLV_IN_BINARY(ctlv_attestation_evidence_get_evidence_service_guid)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_service_manifest_version,
	    &ctlv_attestation_op_schemas[1].request,
	    CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION, 0, 7, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_service_manifest_version)
CTLV_IN_U64(ctlv_attestation_evidence_get_evidence_service_manifest_version)
CTLV_IN_REF(ctlv_attestation_evidence_get_evidence_options, &ctlv_attestation_op_schemas[1].request,
	    CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS, 0, 8, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_attestation_evidence_get_evidence_options)
CTLV_IN_NEST(ctlv_attestation_evidence_get_evidence_options)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_blob_class,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS, 2,
	    0, CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_blob_class)
CTLV_IN_U64(ctlv_attestation_evidence_get_object_evidence_blob_class)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_format,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT, 2, 1,
	    CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_format)
CTLV_IN_STRING(ctlv_attestation_evidence_get_object_evidence_format)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_representation,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION,
	    2, 2, CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_representation)
CTLV_IN_U64(ctlv_attestation_evidence_get_object_evidence_representation)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_data,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_DATA, 2, 3,
	    CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_data)
CTLV_IN_BLOB(ctlv_attestation_evidence_get_object_evidence_data)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_blob_instance,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE,
	    2, 4, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_blob_instance)
CTLV_IN_U64(ctlv_attestation_evidence_get_object_evidence_blob_instance)
CTLV_IN_REF(ctlv_attestation_evidence_get_object_evidence_hash_algo,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO, 2, 5,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_object_evidence_hash_algo)
CTLV_IN_U64(ctlv_attestation_evidence_get_object_evidence_hash_algo)
CTLV_IN_REF(ctlv_attestation_evidence_get_selection_mr_id, &ctlv_attestation_op_schemas[1].request,
	    CTLV_ATTESTATION_ATTR_MR_ID, 4, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_selection_mr_id)
CTLV_IN_U64(ctlv_attestation_evidence_get_selection_mr_id)
CTLV_IN_REF(ctlv_attestation_evidence_get_selection_evidence_hash_algo,
	    &ctlv_attestation_op_schemas[1].request, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO, 4, 1,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_get_selection_evidence_hash_algo)
CTLV_IN_U64(ctlv_attestation_evidence_get_selection_evidence_hash_algo)
CTLV_IN_REF(ctlv_attestation_mr_get_mr_id, &ctlv_attestation_op_schemas[2].request,
	    CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_get_mr_id)
CTLV_IN_U64(ctlv_attestation_mr_get_mr_id)
CTLV_IN_REF(ctlv_attestation_mr_read_mr_id, &ctlv_attestation_op_schemas[4].request,
	    CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_read_mr_id)
CTLV_IN_U64(ctlv_attestation_mr_read_mr_id)
CTLV_IN_REF(ctlv_attestation_mr_read_device_generation, &ctlv_attestation_op_schemas[4].request,
	    CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_read_device_generation)
CTLV_IN_U64(ctlv_attestation_mr_read_device_generation)
CTLV_IN_REF(ctlv_attestation_mr_read_mr_options, &ctlv_attestation_op_schemas[4].request,
	    CTLV_ATTESTATION_ATTR_MR_OPTIONS, 0, 2, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_attestation_mr_read_mr_options)
CTLV_IN_NEST(ctlv_attestation_mr_read_mr_options)
CTLV_IN_REF(ctlv_attestation_mr_extend_mr_id, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_extend_mr_id)
CTLV_IN_U64(ctlv_attestation_mr_extend_mr_id)
CTLV_IN_REF(ctlv_attestation_mr_extend_mr_value, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_MR_VALUE, 0, 1, CTLV_SCHEMA_NEST)
#define ctlv_attestation_mr_extend_mr_value_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_attestation_mr_extend_mr_value, _attr, _nest)
CTLV_IN_REF(ctlv_attestation_mr_extend_device_generation, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 2, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_extend_device_generation)
CTLV_IN_U64(ctlv_attestation_mr_extend_device_generation)
CTLV_IN_REF(ctlv_attestation_mr_extend_mr_options, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_MR_OPTIONS, 0, 3, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_attestation_mr_extend_mr_options)
CTLV_IN_NEST(ctlv_attestation_mr_extend_mr_options)
CTLV_IN_REF(ctlv_attestation_mr_extend_record_item, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_RECORD_ITEM, 0, 4, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_attestation_mr_extend_record_item)
CTLV_IN_NEST(ctlv_attestation_mr_extend_record_item)
CTLV_IN_REF(ctlv_attestation_mr_extend_value_mr_hash_algo, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_MR_HASH_ALGO, 2, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_extend_value_mr_hash_algo)
CTLV_IN_U64(ctlv_attestation_mr_extend_value_mr_hash_algo)
CTLV_IN_REF(ctlv_attestation_mr_extend_value_mr_data, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_MR_DATA, 2, 1, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_mr_extend_value_mr_data)
CTLV_IN_BINARY(ctlv_attestation_mr_extend_value_mr_data)
CTLV_IN_REF(ctlv_attestation_mr_extend_item_record_format, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_RECORD_FORMAT, 5, 0, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_attestation_mr_extend_item_record_format)
CTLV_IN_STRING(ctlv_attestation_mr_extend_item_record_format)
CTLV_IN_REF(ctlv_attestation_mr_extend_item_record_data, &ctlv_attestation_op_schemas[5].request,
	    CTLV_ATTESTATION_ATTR_RECORD_DATA, 5, 1, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_mr_extend_item_record_data)
CTLV_IN_BINARY(ctlv_attestation_mr_extend_item_record_data)
CTLV_IN_REF(ctlv_attestation_evidence_accept_device_generation,
	    &ctlv_attestation_op_schemas[8].request, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 0,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_accept_device_generation)
CTLV_IN_U64(ctlv_attestation_evidence_accept_device_generation)
CTLV_IN_REF(ctlv_attestation_evidence_accept_evidence_generation,
	    &ctlv_attestation_op_schemas[8].request, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION, 0,
	    1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_evidence_accept_evidence_generation)
CTLV_IN_U64(ctlv_attestation_evidence_accept_evidence_generation)
CTLV_IN_REF(ctlv_attestation_devsec_run_device_generation, &ctlv_attestation_op_schemas[11].request,
	    CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_devsec_run_device_generation)
CTLV_IN_U64(ctlv_attestation_devsec_run_device_generation)
CTLV_IN_REF(ctlv_attestation_devsec_run_evidence_generation,
	    &ctlv_attestation_op_schemas[11].request, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION, 0,
	    1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_devsec_run_evidence_generation)
CTLV_IN_U64(ctlv_attestation_devsec_run_evidence_generation)
CTLV_IN_REF(ctlv_attestation_mr_log_get_mr_selection, &ctlv_attestation_op_schemas[13].request,
	    CTLV_ATTESTATION_ATTR_MR_SELECTION, 0, 0, CTLV_SCHEMA_NEST)
#define ctlv_attestation_mr_log_get_mr_selection_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_attestation_mr_log_get_mr_selection, _attr, _nest)
CTLV_IN_REF(ctlv_attestation_mr_log_get_mr_start_generation,
	    &ctlv_attestation_op_schemas[13].request, CTLV_ATTESTATION_ATTR_MR_START_GENERATION, 0,
	    1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_log_get_mr_start_generation)
CTLV_IN_U64(ctlv_attestation_mr_log_get_mr_start_generation)
CTLV_IN_REF(ctlv_attestation_mr_log_get_mr_end_generation, &ctlv_attestation_op_schemas[13].request,
	    CTLV_ATTESTATION_ATTR_MR_END_GENERATION, 0, 2, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_log_get_mr_end_generation)
CTLV_IN_U64(ctlv_attestation_mr_log_get_mr_end_generation)
CTLV_IN_REF(ctlv_attestation_mr_log_get_selection_mr_id, &ctlv_attestation_op_schemas[13].request,
	    CTLV_ATTESTATION_ATTR_MR_ID, 1, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_attestation_mr_log_get_selection_mr_id)
CTLV_IN_U64(ctlv_attestation_mr_log_get_selection_mr_id)

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */
CTLV_PUT_U64(ctlv_attestation_device_generation, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
	     CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_attestation_device_caps, CTLV_ATTESTATION_ATTR_DEVICE_CAPS, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_device_info, CTLV_ATTESTATION_ATTR_DEVICE_INFO, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_cap_evidence, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_cap_mr, CTLV_ATTESTATION_ATTR_CAP_MR, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_cap_min_challenge_length,
	     CTLV_ATTESTATION_ATTR_CAP_MIN_CHALLENGE_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_cap_max_challenge_length,
	     CTLV_ATTESTATION_ATTR_CAP_MAX_CHALLENGE_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_cap_max_total_output_length,
	     CTLV_ATTESTATION_ATTR_CAP_MAX_TOTAL_OUTPUT_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_cap_mr_flags, CTLV_ATTESTATION_ATTR_CAP_MR_FLAGS, CTLV_SCHEMA_FLAGS)
CTLV_NEST_START(ctlv_attestation_cap_object, CTLV_ATTESTATION_ATTR_CAP_OBJECT, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_cap_blob_class, CTLV_ATTESTATION_ATTR_CAP_BLOB_CLASS,
	     CTLV_SCHEMA_ENUM)
CTLV_PUT_STRING(ctlv_attestation_cap_format, CTLV_ATTESTATION_ATTR_CAP_FORMAT, CTLV_SCHEMA_STRING)
CTLV_PUT_U64(ctlv_attestation_cap_representation, CTLV_ATTESTATION_ATTR_CAP_REPRESENTATION,
	     CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_attestation_cap_max_length, CTLV_ATTESTATION_ATTR_CAP_MAX_LENGTH,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_cap_evidence_flags, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE_FLAGS,
	     CTLV_SCHEMA_FLAGS)
CTLV_NEST_START(ctlv_attestation_cap_devsec, CTLV_ATTESTATION_ATTR_CAP_DEVSEC, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_cap_devsec_flags, CTLV_ATTESTATION_ATTR_CAP_DEVSEC_FLAGS,
	     CTLV_SCHEMA_FLAGS)
CTLV_PUT_U64(ctlv_attestation_cap_blob_instance, CTLV_ATTESTATION_ATTR_CAP_BLOB_INSTANCE,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_cap_hash_algo, CTLV_ATTESTATION_ATTR_CAP_HASH_ALGO, CTLV_SCHEMA_UINT)
CTLV_PUT_BINARY(ctlv_attestation_evidence_challenge, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE,
		CTLV_SCHEMA_BINARY)
CTLV_NEST_START(ctlv_attestation_evidence_object, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT,
		CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_evidence_blob_class, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS,
	     CTLV_SCHEMA_ENUM)
CTLV_PUT_STRING(ctlv_attestation_evidence_format, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT,
		CTLV_SCHEMA_STRING)
CTLV_PUT_U64(ctlv_attestation_evidence_representation,
	     CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_attestation_evidence_object_length, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT_LENGTH,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_evidence_generation, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_evidence_accepted_generation,
	     CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_evidence_blob_instance, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_evidence_hash_algo, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO,
	     CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_attestation_evidence_selection, CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION,
		CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_evidence_privlevel, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_STRING(ctlv_attestation_evidence_service_provider,
		CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER, CTLV_SCHEMA_STRING)
CTLV_PUT_BINARY(ctlv_attestation_evidence_service_guid, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID,
		CTLV_SCHEMA_BINARY)
CTLV_PUT_U64(ctlv_attestation_evidence_service_manifest_version,
	     CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_attestation_evidence_options, CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS,
		CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_mr_id, CTLV_ATTESTATION_ATTR_MR_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_STRING(ctlv_attestation_mr_name, CTLV_ATTESTATION_ATTR_MR_NAME, CTLV_SCHEMA_STRING)
CTLV_PUT_U64(ctlv_attestation_mr_hash_algo, CTLV_ATTESTATION_ATTR_MR_HASH_ALGO, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_mr_supported_hash_algo, CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_mr_register_index, CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_mr_caps, CTLV_ATTESTATION_ATTR_MR_CAPS, CTLV_SCHEMA_FLAGS)
CTLV_NEST_START(ctlv_attestation_mr_item, CTLV_ATTESTATION_ATTR_MR_ITEM, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_mr_value, CTLV_ATTESTATION_ATTR_MR_VALUE, CTLV_SCHEMA_NEST)
CTLV_PUT_BINARY(ctlv_attestation_mr_data, CTLV_ATTESTATION_ATTR_MR_DATA, CTLV_SCHEMA_BINARY)
CTLV_PUT_U64(ctlv_attestation_mr_generation, CTLV_ATTESTATION_ATTR_MR_GENERATION, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_mr_state_flags, CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS,
	     CTLV_SCHEMA_FLAGS)
CTLV_NEST_START(ctlv_attestation_mr_options, CTLV_ATTESTATION_ATTR_MR_OPTIONS, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_mr_selection, CTLV_ATTESTATION_ATTR_MR_SELECTION, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_mr_start_generation, CTLV_ATTESTATION_ATTR_MR_START_GENERATION,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_attestation_mr_end_generation, CTLV_ATTESTATION_ATTR_MR_END_GENERATION,
	     CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_attestation_log_entry, CTLV_ATTESTATION_ATTR_LOG_ENTRY, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_log_event, CTLV_ATTESTATION_ATTR_LOG_EVENT, CTLV_SCHEMA_ENUM)
CTLV_NEST_START(ctlv_attestation_log_mr, CTLV_ATTESTATION_ATTR_LOG_MR, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_attestation_log_ima, CTLV_ATTESTATION_ATTR_LOG_IMA, CTLV_SCHEMA_NEST)
CTLV_PUT_STRING(ctlv_attestation_log_template_name, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME,
		CTLV_SCHEMA_STRING)
CTLV_PUT_BINARY(ctlv_attestation_log_template_data, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA,
		CTLV_SCHEMA_BINARY)
CTLV_PUT_BOOL(ctlv_attestation_log_violation, CTLV_ATTESTATION_ATTR_LOG_VIOLATION, CTLV_SCHEMA_BOOL)
CTLV_NEST_START(ctlv_attestation_notification_entry, CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY,
		CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_attestation_devsec_tdisp_state, CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE,
	     CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_attestation_devsec_device_trust, CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST,
	     CTLV_SCHEMA_ENUM)
CTLV_NEST_START(ctlv_attestation_record_item, CTLV_ATTESTATION_ATTR_RECORD_ITEM, CTLV_SCHEMA_NEST)
CTLV_PUT_STRING(ctlv_attestation_record_format, CTLV_ATTESTATION_ATTR_RECORD_FORMAT,
		CTLV_SCHEMA_STRING)
CTLV_PUT_BINARY(ctlv_attestation_record_data, CTLV_ATTESTATION_ATTR_RECORD_DATA, CTLV_SCHEMA_BINARY)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_attestation_device_state_changed, CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED)
CTLV_EVENT_CREATE(ctlv_attestation_mr_log_entry_appended, CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED)
CTLV_EVENT_CREATE(ctlv_attestation_evidence_state_changed,
		  CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_ATTESTATION_GEN_H */
