/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv CTLV family. */

#ifndef _LINUX_CTLV_GEN_H
#define _LINUX_CTLV_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/ctlv/ctlv_gen.h>

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_op_schemas[];

extern const struct ctlv_family_schema ctlv_schema;

/* The error-info message, built by the core. */
#define CTLV_ERROR_INFO_MAX_LEN 456

extern const struct ctlv_schema_tree ctlv_error_info_schema;

/* Implemented by the CTLV core, one per action and query it declares. */
int ctlv_family_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_op_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		      struct ctlv_cursor *reply);
int ctlv_op_schema_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_subscriptions_add_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_subscriptions_remove_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				     const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_event_queue_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_event_queue_set_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply);

extern const struct ctlv_family ctlv_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */
/* The ID of the op to describe. */
CTLV_IN_REF(ctlv_op_schema_get_id, CTLV_T_OP_SCHEMA_GET_ID, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_op_schema_get_id)
CTLV_IN_U64(ctlv_op_schema_get_id)
/* The op ID of an event the device emits. */
CTLV_IN_REF(ctlv_event_subscription_op_ids, CTLV_T_EVENT_SUBSCRIPTION_OP_IDS, CTLV_SCHEMA_UINT)
#define ctlv_event_subscription_op_ids_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_event_subscription_op_ids, _cursor, _record)
CTLV_IN_EACH_U64(ctlv_event_subscription_op_ids)
/* The new length, within what event-queue-get reports. */
CTLV_IN_REF(ctlv_event_queue_set_queue_len, CTLV_T_EVENT_QUEUE_SET_QUEUE_LEN, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_event_queue_set_queue_len)
CTLV_IN_U64(ctlv_event_queue_set_queue_len)

/* Writing a reply. An append is atomic; a record ends with ctlv_record_end(). */
/* One entry per family of the chain. */
CTLV_RECORD_START(ctlv_family_get_families, CTLV_T_FAMILY_GET_FAMILIES, CTLV_SCHEMA_RECORD)
/* The family ID, which bits 63..48 of the family's types carry. */
CTLV_PUT_U64(ctlv_family_get_families_id, CTLV_T_FAMILY_GET_FAMILIES_ID, CTLV_SCHEMA_UINT)
/*
 * The registry name, also what a uevent carries. Bounded here, this being the only place a message
 * carries a family name.
 */
CTLV_PUT_STRING(ctlv_family_get_families_name, CTLV_T_FAMILY_GET_FAMILIES_NAME, CTLV_SCHEMA_STRING)
/* One entry per op the device publishes. */
CTLV_RECORD_START(ctlv_op_get_ops, CTLV_T_OP_GET_OPS, CTLV_SCHEMA_RECORD)
/* The op ID, which CTLV_IOC_OP_EXEC and an event header carry. */
CTLV_PUT_U64(ctlv_op_get_ops_id, CTLV_T_OP_GET_OPS_ID, CTLV_SCHEMA_UINT)
/* Action, query or event. */
CTLV_PUT_U64(ctlv_op_get_ops_type, CTLV_T_OP_GET_OPS_TYPE, CTLV_SCHEMA_ENUM)
/* Whether the device may disable and enable the op. */
CTLV_PUT_U64(ctlv_op_get_ops_availability, CTLV_T_OP_GET_OPS_AVAILABILITY, CTLV_SCHEMA_ENUM)
/* Whether the device has the op disabled right now. */
CTLV_PUT_BOOL(ctlv_op_get_ops_disabled, CTLV_T_OP_GET_OPS_DISABLED, CTLV_SCHEMA_BOOL)
/* The longest request composed from the schema. */
CTLV_PUT_U64(ctlv_op_get_ops_request_max_len, CTLV_T_OP_GET_OPS_REQUEST_MAX_LEN, CTLV_SCHEMA_UINT)
/* The reply buffer an action needs, zero for one that answers nothing. */
CTLV_PUT_U64(ctlv_op_get_ops_action_reply_max_len, CTLV_T_OP_GET_OPS_ACTION_REPLY_MAX_LEN,
	     CTLV_SCHEMA_UINT)
/* The shortest reply buffer a query takes. */
CTLV_PUT_U64(ctlv_op_get_ops_query_reply_min_len, CTLV_T_OP_GET_OPS_QUERY_REPLY_MIN_LEN,
	     CTLV_SCHEMA_UINT)
/* The reply buffer length the family suggests for a query. */
CTLV_PUT_U64(ctlv_op_get_ops_query_reply_suggested_len, CTLV_T_OP_GET_OPS_QUERY_REPLY_SUGGESTED_LEN,
	     CTLV_SCHEMA_UINT)
/* The access a file description needs to execute the op. */
CTLV_PUT_U64(ctlv_op_get_ops_fd_access, CTLV_T_OP_GET_OPS_FD_ACCESS, CTLV_SCHEMA_ENUM)
/* The longest error information the op can return. */
CTLV_PUT_U64(ctlv_op_get_ops_error_info_max_len, CTLV_T_OP_GET_OPS_ERROR_INFO_MAX_LEN,
	     CTLV_SCHEMA_UINT)
/* The longest message an event of the op carries. */
CTLV_PUT_U64(ctlv_op_get_ops_event_max_len, CTLV_T_OP_GET_OPS_EVENT_MAX_LEN, CTLV_SCHEMA_UINT)
/* The root of the op's request tree, absent when it has no request. */
CTLV_RECORD_START(ctlv_op_schema_get_request, CTLV_T_OP_SCHEMA_GET_REQUEST, CTLV_SCHEMA_RECORD)
/* Zero in a message root node, as in the root TLV. */
CTLV_PUT_U64(ctlv_schema_node_type, CTLV_T_SCHEMA_NODE_TYPE, CTLV_SCHEMA_UINT)
/* enum ctlv_schema_kind. */
CTLV_PUT_U64(ctlv_schema_node_kind, CTLV_T_SCHEMA_NODE_KIND, CTLV_SCHEMA_UINT)
/* Zero where the node may repeat as often as the message has room. */
CTLV_PUT_U64(ctlv_schema_node_max_occurrences, CTLV_T_SCHEMA_NODE_MAX_OCCURRENCES, CTLV_SCHEMA_UINT)
/*
 * The inclusive lower bound of an integer or enum payload, a raw 64-bit value read according to
 * kind. An enum's values are numbered without gaps.
 */
CTLV_PUT_U64(ctlv_schema_node_min, CTLV_T_SCHEMA_NODE_MIN, CTLV_SCHEMA_UINT)
/* The inclusive upper bound, read the same way. */
CTLV_PUT_U64(ctlv_schema_node_max, CTLV_T_SCHEMA_NODE_MAX, CTLV_SCHEMA_UINT)
/* The shortest payload of a string or binary field, a string's terminator excluded. */
CTLV_PUT_U64(ctlv_schema_node_min_length, CTLV_T_SCHEMA_NODE_MIN_LENGTH, CTLV_SCHEMA_UINT)
/* The longest payload, likewise. */
CTLV_PUT_U64(ctlv_schema_node_max_length, CTLV_T_SCHEMA_NODE_MAX_LENGTH, CTLV_SCHEMA_UINT)
/* The bits a flags payload may carry. */
CTLV_PUT_U64(ctlv_schema_node_flags_mask, CTLV_T_SCHEMA_NODE_FLAGS_MASK, CTLV_SCHEMA_UINT)
/* enum ctlv_unit, absent when the payload is dimensionless. */
CTLV_PUT_U64(ctlv_schema_node_unit, CTLV_T_SCHEMA_NODE_UNIT, CTLV_SCHEMA_UINT)
/*
 * The decimal exponent of the unit, a payload being value * 10^scale of it, present only with a
 * unit.
 */
CTLV_PUT_S64(ctlv_schema_node_scale, CTLV_T_SCHEMA_NODE_SCALE, CTLV_SCHEMA_SINT)
/* Which way a blob field's bytes go. */
CTLV_PUT_U64(ctlv_schema_node_blob_direction, CTLV_T_SCHEMA_NODE_BLOB_DIRECTION, CTLV_SCHEMA_ENUM)
/* Mask of BIT(enum ctlv_blob_type): the backings this node takes. */
CTLV_PUT_U64(ctlv_schema_node_blob_backings, CTLV_T_SCHEMA_NODE_BLOB_BACKINGS, CTLV_SCHEMA_UINT)
/* Maximum external payload length, unrelated to the inline message limit. */
CTLV_PUT_U64(ctlv_schema_node_blob_max_length, CTLV_T_SCHEMA_NODE_BLOB_MAX_LENGTH, CTLV_SCHEMA_UINT)
/* The power of two a blob's address or offset is a multiple of. */
CTLV_PUT_U64(ctlv_schema_node_blob_alignment, CTLV_T_SCHEMA_NODE_BLOB_ALIGNMENT, CTLV_SCHEMA_UINT)
/*
 * Levels to climb to the node whose members are this one's as well, one meaning the node this one
 * is a member of. A node that carries it lists no members itself.
 */
CTLV_PUT_U64(ctlv_schema_node_reenters, CTLV_T_SCHEMA_NODE_REENTERS, CTLV_SCHEMA_UINT)
/* The required nodes one level in. */
CTLV_RECORD_START(ctlv_schema_node_fields, CTLV_T_SCHEMA_NODE_FIELDS, CTLV_SCHEMA_RECORD)
/* The optional nodes one level in. */
CTLV_RECORD_START(ctlv_schema_node_optional_fields, CTLV_T_SCHEMA_NODE_OPTIONAL_FIELDS,
		  CTLV_SCHEMA_RECORD)
/*
 * The root of its reply tree, absent when it has no reply. An event's one tree, its payload's, is
 * reported here.
 */
CTLV_RECORD_START(ctlv_op_schema_get_reply, CTLV_T_OP_SCHEMA_GET_REPLY, CTLV_SCHEMA_RECORD)
/* How long the queue is. */
CTLV_PUT_U64(ctlv_event_queue_get_queue_len, CTLV_T_EVENT_QUEUE_GET_QUEUE_LEN, CTLV_SCHEMA_UINT)
/* How much of it the queued events take. */
CTLV_PUT_U64(ctlv_event_queue_get_queue_used, CTLV_T_EVENT_QUEUE_GET_QUEUE_USED, CTLV_SCHEMA_UINT)
/* The shortest queue-len this device takes. */
CTLV_PUT_U64(ctlv_event_queue_get_queue_min_len, CTLV_T_EVENT_QUEUE_GET_QUEUE_MIN_LEN,
	     CTLV_SCHEMA_UINT)
/* The longest queue-len this device takes. */
CTLV_PUT_U64(ctlv_event_queue_get_queue_max_len, CTLV_T_EVENT_QUEUE_GET_QUEUE_MAX_LEN,
	     CTLV_SCHEMA_UINT)
/* The op that changed. */
CTLV_PUT_U64(ctlv_op_changed_id, CTLV_T_OP_CHANGED_ID, CTLV_SCHEMA_UINT)
/* Whether the op is disabled now. */
CTLV_PUT_BOOL(ctlv_op_changed_disabled, CTLV_T_OP_CHANGED_DISABLED, CTLV_SCHEMA_BOOL)
/* Advances once per op state change on one device. */
CTLV_PUT_U64(ctlv_op_changed_state_generation, CTLV_T_OP_CHANGED_STATE_GENERATION, CTLV_SCHEMA_UINT)
/* The sequence number of the first event lost. */
CTLV_PUT_U64(ctlv_event_overflow_first_sequence, CTLV_T_EVENT_OVERFLOW_FIRST_SEQUENCE,
	     CTLV_SCHEMA_UINT)
/* The sequence number of the last event lost. */
CTLV_PUT_U64(ctlv_event_overflow_last_sequence, CTLV_T_EVENT_OVERFLOW_LAST_SEQUENCE,
	     CTLV_SCHEMA_UINT)
/* How many events were lost. */
CTLV_PUT_U64(ctlv_event_overflow_dropped_count, CTLV_T_EVENT_OVERFLOW_DROPPED_COUNT,
	     CTLV_SCHEMA_UINT)
/* The rule the message broke. */
CTLV_PUT_U64(ctlv_error_info_reason, CTLV_T_ERROR_INFO_REASON, CTLV_SCHEMA_ENUM)
/* What went wrong, for a person to read. */
CTLV_PUT_STRING(ctlv_error_info_message, CTLV_T_ERROR_INFO_MESSAGE, CTLV_SCHEMA_STRING)
/*
 * The offset of the offending TLV from the start of the message, or of its record when what is
 * wrong is something the record lacks.
 */
CTLV_PUT_U64(ctlv_error_info_bad_tlv_offset, CTLV_T_ERROR_INFO_BAD_TLV_OFFSET, CTLV_SCHEMA_UINT)
/* The offset of the record holding it, from the start of the message. */
CTLV_PUT_U64(ctlv_error_info_record_offset, CTLV_T_ERROR_INFO_RECORD_OFFSET, CTLV_SCHEMA_UINT)
/* The type of the required field that is absent. */
CTLV_PUT_U64(ctlv_error_info_missing_type, CTLV_T_ERROR_INFO_MISSING_TYPE, CTLV_SCHEMA_UINT)
/* The generation the query continuation carried. */
CTLV_PUT_U64(ctlv_error_info_expected_generation, CTLV_T_ERROR_INFO_EXPECTED_GENERATION,
	     CTLV_SCHEMA_UINT)
/* The generation the query has now. */
CTLV_PUT_U64(ctlv_error_info_current_generation, CTLV_T_ERROR_INFO_CURRENT_GENERATION,
	     CTLV_SCHEMA_UINT)
/* How long the output blob at bad-tlv-offset has to be for the op. */
CTLV_PUT_U64(ctlv_error_info_required_length, CTLV_T_ERROR_INFO_REQUIRED_LENGTH, CTLV_SCHEMA_UINT)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_op_changed, CTLV_OP_OP_CHANGED)
CTLV_EVENT_CREATE(ctlv_event_overflow, CTLV_OP_EVENT_OVERFLOW)
CTLV_EVENT_CREATE(ctlv_device_removed, CTLV_OP_DEVICE_REMOVED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_GEN_H */
