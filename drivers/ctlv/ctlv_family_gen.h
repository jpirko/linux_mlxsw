/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN family header */

/* Registration descriptor of the ctlv CTLV family. */

#ifndef _LINUX_CTLV_FAMILY_GEN_H
#define _LINUX_CTLV_FAMILY_GEN_H

#include <linux/ctlv.h>

#include <uapi/linux/ctlv_gen.h>

#include <linux/ctlv_schema_gen.h>

/* Implemented by the framework, one per effective action and query. */
int ctlv_families_dump(struct ctlv_op_ctx *ctx,
		       const struct ctlv_cursor *request,
		       struct ctlv_cursor *reply);
int ctlv_ops_dump(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		  struct ctlv_cursor *reply);
int ctlv_op_schema_get(struct ctlv_op_ctx *ctx,
		       const struct ctlv_cursor *request,
		       struct ctlv_cursor *reply);
int ctlv_subscriptions_add(struct ctlv_op_ctx *ctx,
			   const struct ctlv_cursor *request,
			   struct ctlv_cursor *reply);
int ctlv_subscriptions_remove(struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);
int ctlv_event_queue_get(struct ctlv_op_ctx *ctx,
			 const struct ctlv_cursor *request,
			 struct ctlv_cursor *reply);
int ctlv_event_queue_set(struct ctlv_op_ctx *ctx,
			 const struct ctlv_cursor *request,
			 struct ctlv_cursor *reply);

/* Called around every op this family answers, inherited ones included. */
int ctlv_op_pre(struct ctlv_op_ctx *ctx);
void ctlv_op_post(struct ctlv_op_ctx *ctx);

extern const struct ctlv_family ctlv_ctlv_family;

/* What a device may do with one op. Removal must precede registration. */
static inline void ctlv_families_dump_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000001ULL);
}

static inline void ctlv_ops_dump_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000002ULL);
}

static inline void ctlv_op_schema_get_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000003ULL);
}

static inline void ctlv_subscriptions_add_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000004ULL);
}

static inline void ctlv_subscriptions_remove_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000005ULL);
}

static inline void ctlv_event_queue_get_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000006ULL);
}

static inline void ctlv_event_queue_set_remove(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_remove(ctlvdev, 0x0000000000000007ULL);
}

/* Reading a validated message. A blob has a resolver instead of a getter. */

static inline bool ctlv_family_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_FAMILY_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_family_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_FAMILY_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_family_name_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_FAMILY_NAME, CTLV_SCHEMA_STRING);
	return ctlv_attr_ok(&attr);
}

static inline const char *ctlv_family_name_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_FAMILY_NAME, CTLV_SCHEMA_STRING);
	return ctlv_attr_string(&attr);
}

#define ctlv_family_for_each_entry(_attr, _nest) \
	for (ctlv_attr_find(_attr, _nest, CTLV_ATTR_FAMILY_ENTRY, \
			    CTLV_SCHEMA_NEST); \
	     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))

static inline bool ctlv_blob_type_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_TYPE, CTLV_SCHEMA_ENUM);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_blob_type_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_TYPE, CTLV_SCHEMA_ENUM);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_blob_address_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_ADDRESS, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_blob_address_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_ADDRESS, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_blob_fd_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_FD, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_blob_fd_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_FD, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_blob_offset_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_OFFSET, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_blob_offset_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_OFFSET, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_blob_length_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_LENGTH, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_blob_length_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_BLOB_LENGTH, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_error_reason_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_error_reason_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_error_message_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_MESSAGE,
		       CTLV_SCHEMA_STRING);
	return ctlv_attr_ok(&attr);
}

static inline const char *ctlv_error_message_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_MESSAGE,
		       CTLV_SCHEMA_STRING);
	return ctlv_attr_string(&attr);
}

static inline bool
ctlv_error_bad_attr_offset_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_error_bad_attr_offset_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_error_nest_offset_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_NEST_OFFSET,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_error_nest_offset_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_NEST_OFFSET,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_error_missing_attr_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_MISSING_ATTR_ID,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_error_missing_attr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_MISSING_ATTR_ID,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_error_expected_generation_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_error_expected_generation_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_error_current_generation_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_CURRENT_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_error_current_generation_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_ERROR_CURRENT_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_op_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_op_type_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_TYPE, CTLV_SCHEMA_ENUM);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_type_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_TYPE, CTLV_SCHEMA_ENUM);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_op_availability_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_AVAILABILITY,
		       CTLV_SCHEMA_ENUM);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_availability_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_AVAILABILITY,
		       CTLV_SCHEMA_ENUM);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_op_disabled_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_DISABLED, CTLV_SCHEMA_BOOL);
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_op_disabled_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_DISABLED, CTLV_SCHEMA_BOOL);
	return ctlv_attr_bool(&attr);
}

static inline bool
ctlv_op_request_max_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_REQUEST_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_request_max_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_REQUEST_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_op_action_reply_max_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_op_action_reply_max_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_op_query_reply_min_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_op_query_reply_min_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_op_query_reply_suggested_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_op_query_reply_suggested_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_op_event_record_max_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_op_event_record_max_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_op_state_generation_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_STATE_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_op_state_generation_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_OP_STATE_GENERATION,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

#define ctlv_op_for_each_entry(_attr, _nest) \
	for (ctlv_attr_find(_attr, _nest, CTLV_ATTR_OP_ENTRY, \
			    CTLV_SCHEMA_NEST); \
	     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))

static inline bool ctlv_schema_attr_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_ATTR_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_attr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_ATTR_ID, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_kind_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_kind_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_KIND, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_max_occurrences_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_schema_max_occurrences_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_min_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MIN, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_min_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MIN, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_max_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_max_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_min_length_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MIN_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_min_length_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MIN_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_max_length_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_max_length_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_MAX_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_flags_mask_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_FLAGS_MASK,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_flags_mask_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_FLAGS_MASK,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_unit_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_UNIT, CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_unit_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_UNIT, CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_scale_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_SCALE, CTLV_SCHEMA_SINT);
	return ctlv_attr_ok(&attr);
}

static inline s64 ctlv_schema_scale_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_SCALE, CTLV_SCHEMA_SINT);
	return ctlv_attr_s64(&attr);
}

static inline bool
ctlv_schema_blob_direction_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
		       CTLV_SCHEMA_ENUM);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_blob_direction_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_DIRECTION,
		       CTLV_SCHEMA_ENUM);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_blob_backings_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_blob_backings_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_blob_max_length_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_schema_blob_max_length_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_schema_blob_alignment_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_blob_alignment_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_schema_reenters_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_REENTERS,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_schema_reenters_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_REENTERS,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

#define ctlv_schema_for_each_attrs(_attr, _nest) \
	for (ctlv_attr_find(_attr, _nest, CTLV_ATTR_SCHEMA_ATTRS, \
			    CTLV_SCHEMA_NEST); \
	     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))

#define ctlv_schema_for_each_optional_attrs(_attr, _nest) \
	for (ctlv_attr_find(_attr, _nest, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS, \
			    CTLV_SCHEMA_NEST); \
	     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))

static inline bool ctlv_schema_request_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_REQUEST, CTLV_SCHEMA_NEST);
	return ctlv_attr_ok(&attr);
}

static inline void ctlv_schema_request_get(struct ctlv_cursor *nested,
					   const struct ctlv_cursor *nest)
{
	ctlv_attr_find(nested, nest, CTLV_ATTR_SCHEMA_REQUEST,
		       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_schema_reply_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_SCHEMA_REPLY, CTLV_SCHEMA_NEST);
	return ctlv_attr_ok(&attr);
}

static inline void ctlv_schema_reply_get(struct ctlv_cursor *nested,
					 const struct ctlv_cursor *nest)
{
	ctlv_attr_find(nested, nest, CTLV_ATTR_SCHEMA_REPLY, CTLV_SCHEMA_NEST);
}

#define ctlv_event_for_each_op_id(_attr, _nest) \
	for (ctlv_attr_find(_attr, _nest, CTLV_ATTR_EVENT_OP_ID, \
			    CTLV_SCHEMA_UINT); \
	     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))

static inline u64 ctlv_event_op_id_get(const struct ctlv_cursor *attr)
{
	return ctlv_attr_u64(attr);
}

static inline bool ctlv_event_queue_len_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_QUEUE_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_event_queue_len_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_QUEUE_LEN,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool ctlv_event_queue_used_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_QUEUE_USED,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_event_queue_used_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_QUEUE_USED,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_event_first_sequence_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_FIRST_SEQUENCE,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_event_first_sequence_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_FIRST_SEQUENCE,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_event_last_sequence_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_LAST_SEQUENCE,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_event_last_sequence_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_LAST_SEQUENCE,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_u64(&attr);
}

static inline bool
ctlv_event_dropped_count_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_DROPPED_COUNT,
		       CTLV_SCHEMA_UINT);
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_event_dropped_count_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_attr_find(&attr, nest, CTLV_ATTR_EVENT_DROPPED_COUNT,
		       CTLV_SCHEMA_UINT);
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

#endif /* _LINUX_CTLV_FAMILY_GEN_H */
