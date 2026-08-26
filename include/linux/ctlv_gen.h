/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and driver API of the ctlv CTLV family. */

#ifndef _LINUX_CTLV_GEN_H
#define _LINUX_CTLV_GEN_H

#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_gen.h>

/* Ops this family declares, in declaration order. */
#define CTLV_N_OPS 10

extern const struct ctlv_op_schema ctlv_ctlv_op_schemas[CTLV_N_OPS];

extern const struct ctlv_family_schema ctlv_ctlv_schema;

/* The error-info message, built by the core. */
#define CTLV_ERROR_INFO_MAX_LEN 432

extern const struct ctlv_schema_tree ctlv_error_info_schema;

/* Implemented by the framework, one per action and query it declares. */
int ctlv_families_dump_query(void *priv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
int ctlv_ops_dump_query(void *priv, struct ctlv_op_ctx *ctx,
			const struct ctlv_cursor *request,
			struct ctlv_cursor *reply);
int ctlv_op_schema_get_query(void *priv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
int ctlv_subscriptions_add_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request,
				  struct ctlv_cursor *reply);
int ctlv_subscriptions_remove_action(void *priv, struct ctlv_op_ctx *ctx,
				     const struct ctlv_cursor *request,
				     struct ctlv_cursor *reply);
int ctlv_event_queue_get_query(void *priv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request,
			       struct ctlv_cursor *reply);
int ctlv_event_queue_set_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply);

/*
 * Called around every core action or query,
 * on every device.
 */
int ctlv_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* The lifecycle of this family's core-allocated priv. */
size_t ctlv_priv_size(void);
int ctlv_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
void ctlv_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_ctlv_family;

/* Reading a validated message. A blob has a resolver instead of a getter. */

#define ctlv_op_schema_get_op_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_ctlv_op_schemas[2].request, CTLV_ATTR_OP_ID, 0, 0, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_op_schema_get_op_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_op_schema_get_op_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_schema_get_op_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_op_schema_get_op_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_subscriptions_add_event_op_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_ctlv_op_schemas[3].request, CTLV_ATTR_EVENT_OP_ID, 0, 0, \
		CTLV_SCHEMA_UINT })

#define ctlv_subscriptions_add_event_op_id_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
				ctlv_subscriptions_add_event_op_id_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline u64
ctlv_subscriptions_add_event_op_id_get(const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr, ctlv_subscriptions_add_event_op_id_ref()))
		return 0;
	return ctlv_attr_u64(attr);
}

#define ctlv_subscriptions_remove_event_op_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_ctlv_op_schemas[4].request, CTLV_ATTR_EVENT_OP_ID, 0, 0, \
		CTLV_SCHEMA_UINT })

#define ctlv_subscriptions_remove_event_op_id_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
				ctlv_subscriptions_remove_event_op_id_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline u64
ctlv_subscriptions_remove_event_op_id_get(const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr,
			       ctlv_subscriptions_remove_event_op_id_ref()))
		return 0;
	return ctlv_attr_u64(attr);
}

#define ctlv_event_queue_set_event_queue_len_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_ctlv_op_schemas[6].request, CTLV_ATTR_EVENT_QUEUE_LEN, \
		0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_event_queue_set_event_queue_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_event_queue_set_event_queue_len_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_event_queue_set_event_queue_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_event_queue_set_event_queue_len_ref());
	return ctlv_attr_u64(&attr);
}

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */

static inline bool ctlv_family_id_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_FAMILY_ID, CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_family_name_put(struct ctlv_cursor *nest,
					const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTR_FAMILY_NAME, CTLV_SCHEMA_STRING,
			       value);
}

static inline bool ctlv_family_entry_start(struct ctlv_cursor *nest,
					   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_FAMILY_ENTRY,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_blob_type_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_BLOB_TYPE, CTLV_SCHEMA_ENUM, value);
}

static inline bool ctlv_blob_address_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_BLOB_ADDRESS, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_blob_fd_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_BLOB_FD, CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_blob_offset_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_BLOB_OFFSET, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_blob_length_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_BLOB_LENGTH, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_error_reason_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM,
			    value);
}

static inline bool ctlv_error_message_put(struct ctlv_cursor *nest,
					  const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTR_ERROR_MESSAGE,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool ctlv_error_bad_attr_offset_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_error_nest_offset_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_NEST_OFFSET, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_error_missing_attr_id_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_MISSING_ATTR_ID,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_error_expected_generation_put(struct ctlv_cursor *nest,
						      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_error_current_generation_put(struct ctlv_cursor *nest,
						     u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_ERROR_CURRENT_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_id_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_ID, CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_type_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_TYPE, CTLV_SCHEMA_ENUM, value);
}

static inline bool ctlv_op_availability_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_AVAILABILITY, CTLV_SCHEMA_ENUM,
			    value);
}

static inline bool ctlv_op_disabled_put(struct ctlv_cursor *nest, bool value)
{
	return ctlv_put_bool(nest, CTLV_ATTR_OP_DISABLED, CTLV_SCHEMA_BOOL,
			     value);
}

static inline bool ctlv_op_request_max_len_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_REQUEST_MAX_LEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_action_reply_max_len_put(struct ctlv_cursor *nest,
						    u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_query_reply_min_len_put(struct ctlv_cursor *nest,
						   u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_op_query_reply_suggested_len_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_event_record_max_len_put(struct ctlv_cursor *nest,
						    u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_state_generation_put(struct ctlv_cursor *nest,
						u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_STATE_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_op_entry_start(struct ctlv_cursor *nest,
				       struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_OP_ENTRY,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_op_fd_access_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_OP_FD_ACCESS, CTLV_SCHEMA_ENUM,
			    value);
}

static inline bool ctlv_schema_attr_id_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_ATTR_ID, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_kind_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_max_occurrences_put(struct ctlv_cursor *nest,
						   u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_schema_min_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_MIN, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_max_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_MAX, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_min_length_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_MIN_LENGTH, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_max_length_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_MAX_LENGTH, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_flags_mask_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_FLAGS_MASK, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_unit_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_UNIT, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_scale_put(struct ctlv_cursor *nest, s64 value)
{
	return ctlv_put_s64(nest, CTLV_ATTR_SCHEMA_SCALE, CTLV_SCHEMA_SINT,
			    value);
}

static inline bool ctlv_schema_blob_direction_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool ctlv_schema_blob_backings_put(struct ctlv_cursor *nest,
						 u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_schema_blob_max_length_put(struct ctlv_cursor *nest,
						   u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_schema_blob_alignment_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_schema_reenters_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_SCHEMA_REENTERS, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_schema_attrs_start(struct ctlv_cursor *nest,
					   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_SCHEMA_ATTRS,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_schema_optional_attrs_start(struct ctlv_cursor *nest,
						    struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_schema_request_start(struct ctlv_cursor *nest,
					     struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_SCHEMA_REQUEST,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_schema_reply_start(struct ctlv_cursor *nest,
					   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTR_SCHEMA_REPLY,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_event_op_id_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_OP_ID, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_event_queue_len_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_QUEUE_LEN, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_event_queue_used_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_QUEUE_USED, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_event_first_sequence_put(struct ctlv_cursor *nest,
						 u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_FIRST_SEQUENCE,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_event_last_sequence_put(struct ctlv_cursor *nest,
						u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_LAST_SEQUENCE,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_event_dropped_count_put(struct ctlv_cursor *nest,
						u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTR_EVENT_DROPPED_COUNT,
			    CTLV_SCHEMA_UINT, value);
}

/* Building one event, passed to ctlv_op_emit() or ctlv_op_abort() once. */

static inline struct ctlv_op_ctx *
ctlv_op_changed_create(struct ctlv_device *ctlvdev, struct ctlv_cursor **reply,
		       gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x0000000000000008ULL, reply, gfp);
}

static inline struct ctlv_op_ctx *
ctlv_event_overflow_create(struct ctlv_device *ctlvdev,
			   struct ctlv_cursor **reply, gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x0000000000000009ULL, reply, gfp);
}

static inline struct ctlv_op_ctx *
ctlv_device_removed_create(struct ctlv_device *ctlvdev,
			   struct ctlv_cursor **reply, gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x000000000000000aULL, reply, gfp);
}

#endif /* _LINUX_CTLV_GEN_H */
