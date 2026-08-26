/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv CTLV family. */

#ifndef _LINUX_CTLV_GEN_H
#define _LINUX_CTLV_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_gen.h>

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_op_schemas[];

extern const struct ctlv_family_schema ctlv_schema;

/* The error-info message, built by the core. */
#define CTLV_ERROR_INFO_MAX_LEN 432

extern const struct ctlv_schema_tree ctlv_error_info_schema;

/* Implemented by the CTLV core, one per action and query it declares. */
int ctlv_families_dump_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
int ctlv_ops_dump_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			struct ctlv_cursor *reply);
int ctlv_op_schema_get_query(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
int ctlv_subscriptions_add_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_subscriptions_remove_action(void *priv, struct ctlv_op_ctx *ctx,
				     const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_event_queue_get_query(void *priv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_event_queue_set_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply);

/* Root-family hooks around every core action or query. */
int ctlv_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_priv_size(void);
int ctlv_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
void ctlv_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */
CTLV_IN_REF(ctlv_op_schema_get_op_id, &ctlv_op_schemas[2].request, CTLV_ATTR_OP_ID, 0, 0,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_op_schema_get_op_id)
CTLV_IN_U64(ctlv_op_schema_get_op_id)
CTLV_IN_REF(ctlv_subscriptions_add_event_op_id, &ctlv_op_schemas[3].request, CTLV_ATTR_EVENT_OP_ID,
	    0, 0, CTLV_SCHEMA_UINT)
#define ctlv_subscriptions_add_event_op_id_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_subscriptions_add_event_op_id, _attr, _nest)
CTLV_IN_EACH_U64(ctlv_subscriptions_add_event_op_id)
CTLV_IN_REF(ctlv_subscriptions_remove_event_op_id, &ctlv_op_schemas[4].request,
	    CTLV_ATTR_EVENT_OP_ID, 0, 0, CTLV_SCHEMA_UINT)
#define ctlv_subscriptions_remove_event_op_id_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_subscriptions_remove_event_op_id, _attr, _nest)
CTLV_IN_EACH_U64(ctlv_subscriptions_remove_event_op_id)
CTLV_IN_REF(ctlv_event_queue_set_event_queue_len, &ctlv_op_schemas[6].request,
	    CTLV_ATTR_EVENT_QUEUE_LEN, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_event_queue_set_event_queue_len)
CTLV_IN_U64(ctlv_event_queue_set_event_queue_len)

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */
CTLV_PUT_U64(ctlv_family_id, CTLV_ATTR_FAMILY_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_STRING(ctlv_family_name, CTLV_ATTR_FAMILY_NAME, CTLV_SCHEMA_STRING)
CTLV_NEST_START(ctlv_family_entry, CTLV_ATTR_FAMILY_ENTRY, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_blob_type, CTLV_ATTR_BLOB_TYPE, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_blob_address, CTLV_ATTR_BLOB_ADDRESS, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_blob_fd, CTLV_ATTR_BLOB_FD, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_blob_offset, CTLV_ATTR_BLOB_OFFSET, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_blob_length, CTLV_ATTR_BLOB_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_error_reason, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM)
CTLV_PUT_STRING(ctlv_error_message, CTLV_ATTR_ERROR_MESSAGE, CTLV_SCHEMA_STRING)
CTLV_PUT_U64(ctlv_error_bad_attr_offset, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_error_nest_offset, CTLV_ATTR_ERROR_NEST_OFFSET, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_error_missing_attr_id, CTLV_ATTR_ERROR_MISSING_ATTR_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_error_expected_generation, CTLV_ATTR_ERROR_EXPECTED_GENERATION, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_error_current_generation, CTLV_ATTR_ERROR_CURRENT_GENERATION, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_id, CTLV_ATTR_OP_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_type, CTLV_ATTR_OP_TYPE, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_op_availability, CTLV_ATTR_OP_AVAILABILITY, CTLV_SCHEMA_ENUM)
CTLV_PUT_BOOL(ctlv_op_disabled, CTLV_ATTR_OP_DISABLED, CTLV_SCHEMA_BOOL)
CTLV_PUT_U64(ctlv_op_request_max_len, CTLV_ATTR_OP_REQUEST_MAX_LEN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_action_reply_max_len, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_query_reply_min_len, CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_query_reply_suggested_len, CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
	     CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_event_record_max_len, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_op_state_generation, CTLV_ATTR_OP_STATE_GENERATION, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_op_entry, CTLV_ATTR_OP_ENTRY, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_op_fd_access, CTLV_ATTR_OP_FD_ACCESS, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_schema_attr_id, CTLV_ATTR_SCHEMA_ATTR_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_kind, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_max_occurrences, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_min, CTLV_ATTR_SCHEMA_MIN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_max, CTLV_ATTR_SCHEMA_MAX, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_min_length, CTLV_ATTR_SCHEMA_MIN_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_max_length, CTLV_ATTR_SCHEMA_MAX_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_flags_mask, CTLV_ATTR_SCHEMA_FLAGS_MASK, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_unit, CTLV_ATTR_SCHEMA_UNIT, CTLV_SCHEMA_UINT)
CTLV_PUT_S64(ctlv_schema_scale, CTLV_ATTR_SCHEMA_SCALE, CTLV_SCHEMA_SINT)
CTLV_PUT_U64(ctlv_schema_blob_direction, CTLV_ATTR_SCHEMA_BLOB_DIRECTION, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_schema_blob_backings, CTLV_ATTR_SCHEMA_BLOB_BACKINGS, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_blob_max_length, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_blob_alignment, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_schema_reenters, CTLV_ATTR_SCHEMA_REENTERS, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_schema_attrs, CTLV_ATTR_SCHEMA_ATTRS, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_schema_optional_attrs, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_schema_request, CTLV_ATTR_SCHEMA_REQUEST, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_schema_reply, CTLV_ATTR_SCHEMA_REPLY, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_event_op_id, CTLV_ATTR_EVENT_OP_ID, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_event_queue_len, CTLV_ATTR_EVENT_QUEUE_LEN, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_event_queue_used, CTLV_ATTR_EVENT_QUEUE_USED, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_event_first_sequence, CTLV_ATTR_EVENT_FIRST_SEQUENCE, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_event_last_sequence, CTLV_ATTR_EVENT_LAST_SEQUENCE, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_event_dropped_count, CTLV_ATTR_EVENT_DROPPED_COUNT, CTLV_SCHEMA_UINT)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_op_changed, CTLV_OP_OP_CHANGED)
CTLV_EVENT_CREATE(ctlv_event_overflow, CTLV_OP_EVENT_OVERFLOW)
CTLV_EVENT_CREATE(ctlv_device_removed, CTLV_OP_DEVICE_REMOVED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_GEN_H */
