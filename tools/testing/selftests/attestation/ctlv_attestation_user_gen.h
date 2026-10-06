/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation.yaml */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_USER_GEN_H

#include <ctlv.h>

#include <linux/ctlv_attestation_gen.h>

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_spec;

/* Executing an op. A query is reassembled into the one reply it fills. */

#define CTLV_ATTESTATION_OP_DEVICE_GET_REQUEST_MAX_LEN 16
#define CTLV_ATTESTATION_OP_DEVICE_GET_ACTION_REPLY_MAX_LEN 12936

static inline int ctlv_attestation_device_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_DEVICE_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_device_get(struct ctlv_dev *dev, struct ctlv_msg *request,
					      struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_DEVICE_GET, request, reply, error);
}

#define CTLV_ATTESTATION_OP_EVIDENCE_GET_REQUEST_MAX_LEN 26800
#define CTLV_ATTESTATION_OP_EVIDENCE_GET_ACTION_REPLY_MAX_LEN 472

static inline int ctlv_attestation_evidence_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_EVIDENCE_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_evidence_get(struct ctlv_dev *dev, struct ctlv_msg *request,
						struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_EVIDENCE_GET, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_GET_REQUEST_MAX_LEN 40
#define CTLV_ATTESTATION_OP_MR_GET_ACTION_REPLY_MAX_LEN 744

static inline int ctlv_attestation_mr_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_get(struct ctlv_dev *dev, struct ctlv_msg *request,
					  struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_MR_GET, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_DUMP_REQUEST_MAX_LEN 16
#define CTLV_ATTESTATION_OP_MR_DUMP_QUERY_REPLY_MIN_LEN 184
#define CTLV_ATTESTATION_OP_MR_DUMP_QUERY_REPLY_SUGGESTED_LEN 4096

static inline int ctlv_attestation_mr_dump_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_DUMP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_dump(struct ctlv_dev *dev, struct ctlv_msg *request,
					   struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_ATTESTATION_OP_MR_DUMP, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_READ_REQUEST_MAX_LEN 80
#define CTLV_ATTESTATION_OP_MR_READ_QUERY_REPLY_MIN_LEN 112
#define CTLV_ATTESTATION_OP_MR_READ_QUERY_REPLY_SUGGESTED_LEN 4096

static inline int ctlv_attestation_mr_read_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_READ_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_read(struct ctlv_dev *dev, struct ctlv_msg *request,
					   struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_ATTESTATION_OP_MR_READ, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_EXTEND_REQUEST_MAX_LEN 6880
#define CTLV_ATTESTATION_OP_MR_EXTEND_ACTION_REPLY_MAX_LEN 64

static inline int ctlv_attestation_mr_extend_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_EXTEND_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_extend(struct ctlv_dev *dev, struct ctlv_msg *request,
					     struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_MR_EXTEND, request, reply, error);
}

#define CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_REQUEST_MAX_LEN 64

static inline int ctlv_attestation_evidence_accept_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_evidence_accept(struct ctlv_dev *dev, struct ctlv_msg *request,
						   struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT, request, 0, NULL, error);
}

#define CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_CLEAR_REQUEST_MAX_LEN 16

static inline int ctlv_attestation_evidence_accept_clear_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_CLEAR_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_evidence_accept_clear(struct ctlv_dev *dev,
							 struct ctlv_msg *request,
							 struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT_CLEAR, request, 0, NULL,
				 error);
}

#define CTLV_ATTESTATION_OP_DEVSEC_STATE_GET_REQUEST_MAX_LEN 16
#define CTLV_ATTESTATION_OP_DEVSEC_STATE_GET_QUERY_REPLY_MIN_LEN 40
#define CTLV_ATTESTATION_OP_DEVSEC_STATE_GET_QUERY_REPLY_SUGGESTED_LEN 1024

static inline int ctlv_attestation_devsec_state_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_DEVSEC_STATE_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_devsec_state_get(struct ctlv_dev *dev, struct ctlv_msg *request,
						    struct ctlv_reply **reply,
						    struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_ATTESTATION_OP_DEVSEC_STATE_GET, request, reply, error);
}

#define CTLV_ATTESTATION_OP_DEVSEC_RUN_REQUEST_MAX_LEN 64
#define CTLV_ATTESTATION_OP_DEVSEC_RUN_ACTION_REPLY_MAX_LEN 40

static inline int ctlv_attestation_devsec_run_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_DEVSEC_RUN_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_devsec_run(struct ctlv_dev *dev, struct ctlv_msg *request,
					      struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_DEVSEC_RUN, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_LOG_GET_REQUEST_MAX_LEN 10304
#define CTLV_ATTESTATION_OP_MR_LOG_GET_QUERY_REPLY_MIN_LEN 65600
#define CTLV_ATTESTATION_OP_MR_LOG_GET_QUERY_REPLY_SUGGESTED_LEN 131072

static inline int ctlv_attestation_mr_log_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_LOG_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_log_get(struct ctlv_dev *dev, struct ctlv_msg *request,
					      struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_ATTESTATION_OP_MR_LOG_GET, request, reply, error);
}

/* Building a request. ctlv_msg_err() reports the first failure. */

static inline int ctlv_attestation_device_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, value);
}

static inline int ctlv_attestation_device_caps_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_DEVICE_CAPS);
}

static inline int ctlv_attestation_device_info_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_DEVICE_INFO);
}

static inline int ctlv_attestation_cap_evidence_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE);
}

static inline int ctlv_attestation_cap_mr_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_CAP_MR);
}

static inline int ctlv_attestation_cap_min_challenge_length_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_MIN_CHALLENGE_LENGTH, value);
}

static inline int ctlv_attestation_cap_max_challenge_length_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_MAX_CHALLENGE_LENGTH, value);
}

static inline int ctlv_attestation_cap_max_total_output_length_put(struct ctlv_msg *msg,
								   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_MAX_TOTAL_OUTPUT_LENGTH, value);
}

static inline int ctlv_attestation_cap_mr_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_MR_FLAGS, value);
}

static inline int ctlv_attestation_cap_object_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_CAP_OBJECT);
}

static inline int ctlv_attestation_cap_blob_class_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_BLOB_CLASS, value);
}

static inline int ctlv_attestation_cap_format_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_CAP_FORMAT, value);
}

static inline int ctlv_attestation_cap_representation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_REPRESENTATION, value);
}

static inline int ctlv_attestation_cap_max_length_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_MAX_LENGTH, value);
}

static inline int ctlv_attestation_cap_evidence_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE_FLAGS, value);
}

static inline int ctlv_attestation_cap_devsec_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_CAP_DEVSEC);
}

static inline int ctlv_attestation_cap_devsec_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_DEVSEC_FLAGS, value);
}

static inline int ctlv_attestation_cap_blob_instance_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_BLOB_INSTANCE, value);
}

static inline int ctlv_attestation_cap_hash_algo_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_CAP_HASH_ALGO, value);
}

static inline int ctlv_attestation_evidence_challenge_put(struct ctlv_msg *msg, const void *value,
							  __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE, value, len);
}

static inline int ctlv_attestation_evidence_object_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT);
}

static inline int ctlv_attestation_evidence_blob_class_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS, value);
}

static inline int ctlv_attestation_evidence_format_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT, value);
}

static inline int ctlv_attestation_evidence_representation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION, value);
}

static inline int ctlv_attestation_evidence_data_put_va(struct ctlv_msg *msg, const void *address,
							__u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_DATA, address, length);
}

static inline int ctlv_attestation_evidence_data_put_memfd(struct ctlv_msg *msg, int fd,
							   __u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_DATA, fd, offset, length);
}

static inline int ctlv_attestation_evidence_data_put_dma_buf(struct ctlv_msg *msg, int fd,
							     __u64 offset, __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_DATA, fd, offset, length);
}

static inline int ctlv_attestation_evidence_object_length_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT_LENGTH, value);
}

static inline int ctlv_attestation_evidence_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION, value);
}

static inline int ctlv_attestation_evidence_accepted_generation_put(struct ctlv_msg *msg,
								    __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION, value);
}

static inline int ctlv_attestation_evidence_blob_instance_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE, value);
}

static inline int ctlv_attestation_evidence_hash_algo_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO, value);
}

static inline int ctlv_attestation_evidence_selection_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION);
}

static inline int ctlv_attestation_evidence_privlevel_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL, value);
}

static inline int ctlv_attestation_evidence_service_provider_put(struct ctlv_msg *msg,
								 const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER, value);
}

static inline int ctlv_attestation_evidence_service_guid_put(struct ctlv_msg *msg,
							     const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID, value, len);
}

static inline int ctlv_attestation_evidence_service_manifest_version_put(struct ctlv_msg *msg,
									 __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION, value);
}

static inline int ctlv_attestation_evidence_options_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS);
}

static inline int ctlv_attestation_mr_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_ID, value);
}

static inline int ctlv_attestation_mr_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_MR_NAME, value);
}

static inline int ctlv_attestation_mr_hash_algo_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_HASH_ALGO, value);
}

static inline int ctlv_attestation_mr_supported_hash_algo_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO, value);
}

static inline int ctlv_attestation_mr_register_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX, value);
}

static inline int ctlv_attestation_mr_caps_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_CAPS, value);
}

static inline int ctlv_attestation_mr_item_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_MR_ITEM);
}

static inline int ctlv_attestation_mr_value_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_MR_VALUE);
}

static inline int ctlv_attestation_mr_data_put(struct ctlv_msg *msg, const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_ATTR_MR_DATA, value, len);
}

static inline int ctlv_attestation_mr_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_GENERATION, value);
}

static inline int ctlv_attestation_mr_state_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS, value);
}

static inline int ctlv_attestation_mr_options_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_MR_OPTIONS);
}

static inline int ctlv_attestation_mr_selection_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_MR_SELECTION);
}

static inline int ctlv_attestation_mr_start_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_START_GENERATION, value);
}

static inline int ctlv_attestation_mr_end_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_MR_END_GENERATION, value);
}

static inline int ctlv_attestation_log_entry_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_LOG_ENTRY);
}

static inline int ctlv_attestation_log_event_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_LOG_EVENT, value);
}

static inline int ctlv_attestation_log_mr_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_LOG_MR);
}

static inline int ctlv_attestation_log_ima_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_LOG_IMA);
}

static inline int ctlv_attestation_log_template_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME, value);
}

static inline int ctlv_attestation_log_template_data_put(struct ctlv_msg *msg, const void *value,
							 __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA, value, len);
}

static inline int ctlv_attestation_log_violation_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_ATTESTATION_ATTR_LOG_VIOLATION, value);
}

static inline int ctlv_attestation_notification_entry_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY);
}

static inline int ctlv_attestation_devsec_tdisp_state_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE, value);
}

static inline int ctlv_attestation_devsec_device_trust_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST, value);
}

static inline int ctlv_attestation_record_item_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_ATTESTATION_ATTR_RECORD_ITEM);
}

static inline int ctlv_attestation_record_format_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_ATTR_RECORD_FORMAT, value);
}

static inline int ctlv_attestation_record_data_put(struct ctlv_msg *msg, const void *value,
						   __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_ATTR_RECORD_DATA, value, len);
}

/* Reading a reply or a record. A getter answers whether it could read. */

static inline bool ctlv_attestation_device_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION);
}

static inline bool ctlv_attestation_device_generation_get(const struct ctlv_attr *nest,
							  __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_device_caps_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_CAPS);
}

static inline const struct ctlv_attr *ctlv_attestation_device_caps_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_CAPS);
}

static inline bool ctlv_attestation_device_info_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_INFO);
}

static inline const struct ctlv_attr *ctlv_attestation_device_info_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVICE_INFO);
}

static inline bool ctlv_attestation_cap_evidence_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE);
}

static inline const struct ctlv_attr *
ctlv_attestation_cap_evidence_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE);
}

static inline bool ctlv_attestation_cap_mr_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MR);
}

static inline const struct ctlv_attr *ctlv_attestation_cap_mr_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MR);
}

static inline bool ctlv_attestation_cap_min_challenge_length_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MIN_CHALLENGE_LENGTH);
}

static inline bool ctlv_attestation_cap_min_challenge_length_get(const struct ctlv_attr *nest,
								 __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MIN_CHALLENGE_LENGTH);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_cap_max_challenge_length_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_CHALLENGE_LENGTH);
}

static inline bool ctlv_attestation_cap_max_challenge_length_get(const struct ctlv_attr *nest,
								 __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_CHALLENGE_LENGTH);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool
ctlv_attestation_cap_max_total_output_length_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_TOTAL_OUTPUT_LENGTH);
}

static inline bool ctlv_attestation_cap_max_total_output_length_get(const struct ctlv_attr *nest,
								    __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_TOTAL_OUTPUT_LENGTH);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_cap_mr_flags_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MR_FLAGS);
}

static inline bool ctlv_attestation_cap_mr_flags_get(const struct ctlv_attr *nest, __u64 *value,
						     __u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MR_FLAGS);
	return attr && !ctlv_get_flags(attr, CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK, value, unknown);
}

#define ctlv_attestation_cap_for_each_object(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_CAP_OBJECT)

static inline bool ctlv_attestation_cap_blob_class_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_BLOB_CLASS);
}

static inline bool ctlv_attestation_cap_blob_class_get(const struct ctlv_attr *nest, __u64 *value,
						       bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_BLOB_CLASS);
	return attr && !ctlv_get_enum(attr, 9, value, known);
}

static inline bool ctlv_attestation_cap_format_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_FORMAT);
}

static inline bool ctlv_attestation_cap_format_get(const struct ctlv_attr *nest, const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_FORMAT);
	return attr && !ctlv_get_str(attr, value);
}

#define ctlv_attestation_cap_for_each_representation(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_CAP_REPRESENTATION)

static inline bool ctlv_attestation_cap_representation_get(const struct ctlv_attr *attr,
							   __u64 *value, bool *known)
{
	return attr && !ctlv_get_enum(attr, 2, value, known);
}

static inline bool ctlv_attestation_cap_max_length_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_LENGTH);
}

static inline bool ctlv_attestation_cap_max_length_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_MAX_LENGTH);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_cap_evidence_flags_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE_FLAGS);
}

static inline bool ctlv_attestation_cap_evidence_flags_get(const struct ctlv_attr *nest,
							   __u64 *value, __u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_EVIDENCE_FLAGS);
	return attr && !ctlv_get_flags(attr, CTLV_ATTESTATION_EVIDENCE_CAPS_MASK, value, unknown);
}

static inline bool ctlv_attestation_cap_devsec_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_DEVSEC);
}

static inline const struct ctlv_attr *ctlv_attestation_cap_devsec_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_DEVSEC);
}

static inline bool ctlv_attestation_cap_devsec_flags_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_DEVSEC_FLAGS);
}

static inline bool ctlv_attestation_cap_devsec_flags_get(const struct ctlv_attr *nest, __u64 *value,
							 __u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_DEVSEC_FLAGS);
	return attr && !ctlv_get_flags(attr, CTLV_ATTESTATION_DEVSEC_CAPS_MASK, value, unknown);
}

static inline bool ctlv_attestation_cap_blob_instance_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_BLOB_INSTANCE);
}

static inline bool ctlv_attestation_cap_blob_instance_get(const struct ctlv_attr *nest,
							  __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_CAP_BLOB_INSTANCE);
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_attestation_cap_for_each_hash_algo(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_CAP_HASH_ALGO)

static inline bool ctlv_attestation_cap_hash_algo_get(const struct ctlv_attr *attr, __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_challenge_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE);
}

static inline bool ctlv_attestation_evidence_challenge_get(const struct ctlv_attr *nest,
							   const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE);
	return attr && !ctlv_get_bin(attr, value, len);
}

#define ctlv_attestation_evidence_for_each_object(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT)

static inline bool ctlv_attestation_evidence_blob_class_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS);
}

static inline bool ctlv_attestation_evidence_blob_class_get(const struct ctlv_attr *nest,
							    __u64 *value, bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS);
	return attr && !ctlv_get_enum(attr, 9, value, known);
}

static inline bool ctlv_attestation_evidence_format_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT);
}

static inline bool ctlv_attestation_evidence_format_get(const struct ctlv_attr *nest,
							const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_attestation_evidence_representation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION);
}

static inline bool ctlv_attestation_evidence_representation_get(const struct ctlv_attr *nest,
								__u64 *value, bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION);
	return attr && !ctlv_get_enum(attr, 2, value, known);
}

#define ctlv_attestation_evidence_for_each_object_length(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT_LENGTH)

static inline bool ctlv_attestation_evidence_object_length_get(const struct ctlv_attr *attr,
							       __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION);
}

static inline bool ctlv_attestation_evidence_generation_get(const struct ctlv_attr *nest,
							    __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool
ctlv_attestation_evidence_accepted_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION);
}

static inline bool ctlv_attestation_evidence_accepted_generation_get(const struct ctlv_attr *nest,
								     __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_blob_instance_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE);
}

static inline bool ctlv_attestation_evidence_blob_instance_get(const struct ctlv_attr *nest,
							       __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_hash_algo_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO);
}

static inline bool ctlv_attestation_evidence_hash_algo_get(const struct ctlv_attr *nest,
							   __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO);
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_attestation_evidence_for_each_selection(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION)

static inline bool ctlv_attestation_evidence_privlevel_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL);
}

static inline bool ctlv_attestation_evidence_privlevel_get(const struct ctlv_attr *nest,
							   __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_service_provider_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER);
}

static inline bool ctlv_attestation_evidence_service_provider_get(const struct ctlv_attr *nest,
								  const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_attestation_evidence_service_guid_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID);
}

static inline bool ctlv_attestation_evidence_service_guid_get(const struct ctlv_attr *nest,
							      const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool
ctlv_attestation_evidence_service_manifest_version_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION);
}

static inline bool
ctlv_attestation_evidence_service_manifest_version_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_evidence_options_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS);
}

static inline const struct ctlv_attr *
ctlv_attestation_evidence_options_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS);
}

static inline bool ctlv_attestation_mr_id_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_ID);
}

static inline bool ctlv_attestation_mr_id_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_ID);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_mr_name_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_NAME);
}

static inline bool ctlv_attestation_mr_name_get(const struct ctlv_attr *nest, const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_NAME);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_attestation_mr_hash_algo_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_HASH_ALGO);
}

static inline bool ctlv_attestation_mr_hash_algo_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_HASH_ALGO);
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_attestation_mr_for_each_supported_hash_algo(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_MR_SUPPORTED_HASH_ALGO)

static inline bool ctlv_attestation_mr_supported_hash_algo_get(const struct ctlv_attr *attr,
							       __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_mr_register_index_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX);
}

static inline bool ctlv_attestation_mr_register_index_get(const struct ctlv_attr *nest,
							  __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_mr_caps_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_CAPS);
}

static inline bool ctlv_attestation_mr_caps_get(const struct ctlv_attr *nest, __u64 *value,
						__u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_CAPS);
	return attr && !ctlv_get_flags(attr, CTLV_ATTESTATION_MR_CAPS_MASK, value, unknown);
}

#define ctlv_attestation_mr_for_each_item(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_MR_ITEM)

#define ctlv_attestation_mr_for_each_value(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_MR_VALUE)

static inline bool ctlv_attestation_mr_data_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_DATA);
}

static inline bool ctlv_attestation_mr_data_get(const struct ctlv_attr *nest, const void **value,
						__u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_DATA);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool ctlv_attestation_mr_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_GENERATION);
}

static inline bool ctlv_attestation_mr_generation_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_mr_state_flags_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS);
}

static inline bool ctlv_attestation_mr_state_flags_get(const struct ctlv_attr *nest, __u64 *value,
						       __u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS);
	return attr && !ctlv_get_flags(attr, CTLV_ATTESTATION_MR_STATE_FLAGS_MASK, value, unknown);
}

static inline bool ctlv_attestation_mr_options_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_OPTIONS);
}

static inline const struct ctlv_attr *ctlv_attestation_mr_options_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_OPTIONS);
}

#define ctlv_attestation_mr_for_each_selection(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_MR_SELECTION)

static inline bool ctlv_attestation_mr_start_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_START_GENERATION);
}

static inline bool ctlv_attestation_mr_start_generation_get(const struct ctlv_attr *nest,
							    __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_START_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_attestation_mr_end_generation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_END_GENERATION);
}

static inline bool ctlv_attestation_mr_end_generation_get(const struct ctlv_attr *nest,
							  __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_MR_END_GENERATION);
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_attestation_log_for_each_entry(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_ATTESTATION_ATTR_LOG_ENTRY)

static inline bool ctlv_attestation_log_event_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_EVENT);
}

static inline bool ctlv_attestation_log_event_get(const struct ctlv_attr *nest, __u64 *value,
						  bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_EVENT);
	return attr && !ctlv_get_enum(attr, 2, value, known);
}

static inline bool ctlv_attestation_log_mr_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_MR);
}

static inline const struct ctlv_attr *ctlv_attestation_log_mr_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_MR);
}

static inline bool ctlv_attestation_log_ima_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_IMA);
}

static inline const struct ctlv_attr *ctlv_attestation_log_ima_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_IMA);
}

static inline bool ctlv_attestation_log_template_name_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME);
}

static inline bool ctlv_attestation_log_template_name_get(const struct ctlv_attr *nest,
							  const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_attestation_log_template_data_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA);
}

static inline bool ctlv_attestation_log_template_data_get(const struct ctlv_attr *nest,
							  const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool ctlv_attestation_log_violation_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_VIOLATION);
}

static inline bool ctlv_attestation_log_violation_get(const struct ctlv_attr *nest, bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_LOG_VIOLATION);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_attestation_notification_entry_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY);
}

static inline const struct ctlv_attr *
ctlv_attestation_notification_entry_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY);
}

static inline bool ctlv_attestation_devsec_tdisp_state_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE);
}

static inline bool ctlv_attestation_devsec_tdisp_state_get(const struct ctlv_attr *nest,
							   __u64 *value, bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE);
	return attr && !ctlv_get_enum(attr, 4, value, known);
}

static inline bool ctlv_attestation_devsec_device_trust_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST);
}

static inline bool ctlv_attestation_devsec_device_trust_get(const struct ctlv_attr *nest,
							    __u64 *value, bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST);
	return attr && !ctlv_get_enum(attr, 5, value, known);
}

static inline bool ctlv_attestation_record_item_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_ITEM);
}

static inline const struct ctlv_attr *ctlv_attestation_record_item_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_ITEM);
}

static inline bool ctlv_attestation_record_format_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_FORMAT);
}

static inline bool ctlv_attestation_record_format_get(const struct ctlv_attr *nest,
						      const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_FORMAT);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_attestation_record_data_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_DATA);
}

static inline bool ctlv_attestation_record_data_get(const struct ctlv_attr *nest,
						    const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_ATTR_RECORD_DATA);
	return attr && !ctlv_get_bin(attr, value, len);
}

/* Watching for events. A subscription is per open file description. */

#define CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED_EVENT_RECORD_MAX_LEN 64

static inline bool ctlv_attestation_device_state_changed_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED;
}

static inline int ctlv_attestation_device_state_changed_subscribe(struct ctlv_dev *dev,
								  struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_attestation_device_state_changed_unsubscribe(struct ctlv_dev *dev,
								    struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#define CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED_EVENT_RECORD_MAX_LEN 73416

static inline bool ctlv_attestation_mr_log_entry_appended_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED;
}

static inline int ctlv_attestation_mr_log_entry_appended_subscribe(struct ctlv_dev *dev,
								   struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_attestation_mr_log_entry_appended_unsubscribe(struct ctlv_dev *dev,
								     struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#define CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED_EVENT_RECORD_MAX_LEN 112

static inline bool ctlv_attestation_evidence_state_changed_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED;
}

static inline int ctlv_attestation_evidence_state_changed_subscribe(struct ctlv_dev *dev,
								    struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_attestation_evidence_state_changed_unsubscribe(struct ctlv_dev *dev,
								      struct ctlv_error *error)
{
	const __u64 op_id = CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#endif /* _CTLV_CTLV_ATTESTATION_USER_GEN_H */
