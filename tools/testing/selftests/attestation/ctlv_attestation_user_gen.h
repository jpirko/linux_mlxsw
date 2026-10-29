/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_USER_GEN_H

#include <ctlv.h>

#include <ctlv/ctlv_attestation_gen.h>

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_spec;

/* Executing an op. A query is reassembled into the one reply it fills. */

#define CTLV_ATTESTATION_OP_DEVICE_GET_REQUEST_MAX_LEN 16

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

#define CTLV_ATTESTATION_OP_EVIDENCE_GET_REQUEST_MAX_LEN 23216

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

static inline int ctlv_attestation_mr_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_ATTESTATION_OP_MR_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_attestation_mr_get(struct ctlv_dev *dev, struct ctlv_msg *request,
					  struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_ATTESTATION_OP_MR_GET, request, reply, error);
}

#define CTLV_ATTESTATION_OP_MR_READ_REQUEST_MAX_LEN 80

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

#define CTLV_ATTESTATION_OP_MR_EXTEND_REQUEST_MAX_LEN 7232

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
						   struct ctlv_reply **reply,
						   struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT, request, reply, error);
}

#define CTLV_ATTESTATION_OP_DEVSEC_STATE_GET_REQUEST_MAX_LEN 16

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

#define CTLV_ATTESTATION_OP_MR_LOG_GET_REQUEST_MAX_LEN 10304

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

/* The platform's name for the MR, such as rtmr0. */
static inline int ctlv_attestation_mr_desc_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_MR_DESC_NAME, value);
}

/* Its banks, each once, in the provider's order. */
static inline int ctlv_attestation_mr_desc_supported_hash_algorithms_put(struct ctlv_msg *msg,
									 __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS, value);
}

/* What it allows, and whether the provider may remove it. */
static inline int ctlv_attestation_mr_desc_caps_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_DESC_CAPS, value);
}

/* The hardware's number for the MR, where it has one: 0 for rtmr0. */
static inline int ctlv_attestation_mr_desc_register_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX, value);
}

/* ima-active while IMA extends it. */
static inline int ctlv_attestation_mr_desc_state_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_DESC_STATE_FLAGS, value);
}

/* The bank's algorithm. */
static inline int ctlv_attestation_mr_value_hash_algo_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO, value);
}

/* The digest, as long as hash-algo's. */
static inline int ctlv_attestation_mr_value_data_put(struct ctlv_msg *msg, const void *value,
						     __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_MR_VALUE_DATA, value, len);
}

/* The device's MR generation, which each extend bumps. */
static inline int ctlv_attestation_mr_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_GENERATION, value);
}

/* Values for banks of the MR, each bank once. */
static inline int ctlv_attestation_mr_values_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_VALUES);
}

/* Advances with each change of the device's state. */
static inline int ctlv_attestation_device_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GENERATION, value);
}

/* The ID of the MR the entry is about. */
static inline int ctlv_attestation_mr_log_entry_mr_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID, value);
}

/* What the entry records. */
static inline int ctlv_attestation_mr_log_entry_event_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT, value);
}

/* The platform's name for the MR. */
static inline int ctlv_attestation_mr_log_entry_mr_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME, value);
}

/* Its banks, each once, in the provider's order. */
static inline int
ctlv_attestation_mr_log_entry_mr_supported_hash_algorithms_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS,
			    value);
}

/* What it allows, and whether the provider may remove it. */
static inline int ctlv_attestation_mr_log_entry_mr_caps_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS, value);
}

/* The hardware's number for the MR, where it has one. */
static inline int ctlv_attestation_mr_log_entry_mr_register_index_put(struct ctlv_msg *msg,
								      __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX, value);
}

/* The MR an info entry describes. */
static inline int ctlv_attestation_mr_log_entry_mr_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR);
}

/* The IMA template, such as ima-ng. */
static inline int ctlv_attestation_mr_log_entry_ima_template_name_put(struct ctlv_msg *msg,
								      const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME, value);
}

/* The template's fields, as IMA's binary measurement list has them. */
static inline int ctlv_attestation_mr_log_entry_ima_template_data_put(struct ctlv_msg *msg,
								      const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA, value, len);
}

/* The entry records an IMA violation, not a measurement. */
static inline int ctlv_attestation_mr_log_entry_ima_violation_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION, value);
}

/* IMA's measurement, on an extend entry IMA made. */
static inline int ctlv_attestation_mr_log_entry_ima_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA);
}

/* The layout of data, in printable ASCII without spaces. */
static inline int ctlv_attestation_mr_log_entry_record_format_put(struct ctlv_msg *msg,
								  const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT, value);
}

/* What was measured, laid out as format says. */
static inline int ctlv_attestation_mr_log_entry_record_data_put(struct ctlv_msg *msg,
								const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA, value, len);
}

/* What was measured, on an extend entry mr-extend made with a record. */
static inline int ctlv_attestation_mr_log_entry_record_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD);
}

/* The first MR generation of the range mr-log-get selects and reports. */
static inline int ctlv_attestation_mr_start_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_START_GENERATION, value);
}

/* The last MR generation of that range. */
static inline int ctlv_attestation_mr_end_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_END_GENERATION, value);
}

/* The device's handle for the object in requests. */
static inline int ctlv_attestation_device_get_objects_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID, value);
}

/* What the object is. */
static inline int ctlv_attestation_device_get_objects_class_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS, value);
}

/*
 * The layouts a standard defines that the object may have. At least one of formats and
 * vendor-formats is present.
 */
static inline int ctlv_attestation_device_get_objects_formats_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_FORMATS, value);
}

/* The layouts its vendor defines that the object may have. */
static inline int ctlv_attestation_device_get_objects_vendor_formats_put(struct ctlv_msg *msg,
									 __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_VENDOR_FORMATS, value);
}

/* The longest the object gets; a blob this long always holds it. */
static inline int ctlv_attestation_device_get_objects_max_length_put(struct ctlv_msg *msg,
								     __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH, value);
}

/* A single length. */
static inline int ctlv_attestation_device_get_challenge_lengths_length_put(struct ctlv_msg *msg,
									   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH, value);
}

/* The shortest of a range. */
static inline int ctlv_attestation_device_get_challenge_lengths_min_length_put(struct ctlv_msg *msg,
									       __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH, value);
}

/* The longest of a range. */
static inline int ctlv_attestation_device_get_challenge_lengths_max_length_put(struct ctlv_msg *msg,
									       __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH, value);
}

/*
 * A signed object's challenge lengths: each entry is one length, or a range from min-length to
 * max-length.
 */
static inline int ctlv_attestation_device_get_objects_challenge_lengths_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CHALLENGE_LENGTHS);
}

/* Tells apart the objects of one kind, such as an SPDM slot. */
static inline int ctlv_attestation_device_get_objects_instance_put(struct ctlv_msg *msg,
								   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_INSTANCE, value);
}

/* Where the TCB vouches for the object with a digest, its algorithm. */
static inline int ctlv_attestation_device_get_objects_tcb_digest_algo_put(struct ctlv_msg *msg,
									  __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_TCB_DIGEST_ALGO, value);
}

/* One per object evidence-get can produce. */
static inline int ctlv_attestation_device_get_evidence_objects_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_OBJECTS);
}

/* The optional evidence features the device provides. */
static inline int ctlv_attestation_device_get_evidence_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_FLAGS, value);
}

/* Present when the device produces evidence. */
static inline int ctlv_attestation_device_get_caps_evidence_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE);
}

/* The MR operations the device provides, and whether MRs come and go. */
static inline int ctlv_attestation_device_get_mr_flags_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS, value);
}

/* Present when the device has MRs. */
static inline int ctlv_attestation_device_get_caps_mr_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR);
}

/* What the device supports, fixed for its life. */
static inline int ctlv_attestation_device_get_caps_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_CAPS);
}

/* The device's family's fields. */
static inline int ctlv_attestation_device_get_info_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_DEVICE_GET_INFO);
}

/*
 * Bound into each signed object named: pass a verifier's nonce, or a hash binding one to your
 * context, never data a peer chose. Present exactly when a signed object is named, and of a length
 * each of them takes.
 */
static inline int ctlv_attestation_evidence_get_challenge_put(struct ctlv_msg *msg,
							      const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE, value, len);
}

/* The object, by the ID device-get lists. */
static inline int ctlv_attestation_evidence_get_objects_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID, value);
}

/*
 * Where the object is written. One too short for it fails the op with EMSGSIZE, and error-info's
 * required-length says how long it has to be.
 */
static inline int ctlv_attestation_evidence_get_objects_data_put_va(struct ctlv_msg *msg,
								    const void *address,
								    __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA, address, length);
}

static inline int ctlv_attestation_evidence_get_objects_data_put_memfd(struct ctlv_msg *msg, int fd,
								       __u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA, fd, offset,
				   length);
}

static inline int ctlv_attestation_evidence_get_objects_data_put_dma_buf(struct ctlv_msg *msg,
									 int fd, __u64 offset,
									 __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA, fd, offset,
				     length);
}

/* The objects to produce, each named once. */
static inline int ctlv_attestation_evidence_get_objects_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS);
}

/* The MR's ID. */
static inline int ctlv_attestation_evidence_get_selection_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_ID, value);
}

/* The bank's algorithm. */
static inline int ctlv_attestation_evidence_get_selection_hash_algo_put(struct ctlv_msg *msg,
									__u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_HASH_ALGO, value);
}

/* The MR banks the evidence reports, where the device takes mr-selection. */
static inline int ctlv_attestation_evidence_get_selection_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION);
}

/* The privilege level to report for, as configfs-tsm's privlevel. */
static inline int ctlv_attestation_evidence_get_privlevel_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_PRIVLEVEL, value);
}

/*
 * The service to attest, as configfs-tsm's service_provider; only with service-guid and
 * service-manifest-version.
 */
static inline int ctlv_attestation_evidence_get_service_provider_put(struct ctlv_msg *msg,
								     const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_PROVIDER, value);
}

/* The service's GUID, as configfs-tsm's service_guid. */
static inline int ctlv_attestation_evidence_get_service_guid_put(struct ctlv_msg *msg,
								 const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_GUID, value, len);
}

/* Its manifest version, as configfs-tsm's service_manifest_version. */
static inline int ctlv_attestation_evidence_get_service_manifest_version_put(struct ctlv_msg *msg,
									     __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_MANIFEST_VERSION, value);
}

/* The device's family's fields. */
static inline int ctlv_attestation_evidence_get_options_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_OPTIONS);
}

/*
 * The layout it came in, where a standard defines it. Exactly one of format and vendor-format is
 * present.
 */
static inline int ctlv_attestation_evidence_get_results_format_put(struct ctlv_msg *msg,
								   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT, value);
}

/* The layout it came in, where its vendor defines it. */
static inline int ctlv_attestation_evidence_get_results_vendor_format_put(struct ctlv_msg *msg,
									  __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT, value);
}

/* The bytes written to its blob. */
static inline int ctlv_attestation_evidence_get_results_length_put(struct ctlv_msg *msg,
								   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH, value);
}

/* The object's tcb-digest-algo. */
static inline int ctlv_attestation_evidence_get_tcb_digest_hash_algo_put(struct ctlv_msg *msg,
									 __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_HASH_ALGO, value);
}

/* The digest, as long as hash-algo's. */
static inline int ctlv_attestation_evidence_get_tcb_digest_data_put(struct ctlv_msg *msg,
								    const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_DATA, value, len);
}

/* Where the TCB vouches for the object, the digest it vouches with. */
static inline int ctlv_attestation_evidence_get_results_tcb_digest_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_TCB_DIGEST);
}

/* One per object, in request order. */
static inline int ctlv_attestation_evidence_get_results_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS);
}

/* On a device with evidence-accept, numbers this evidence for it. */
static inline int ctlv_attestation_evidence_get_evidence_generation_put(struct ctlv_msg *msg,
									__u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_GET_EVIDENCE_GENERATION, value);
}

/* Only this MR; every MR without it. */
static inline int ctlv_attestation_mr_get_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_GET_ID, value);
}

/* The device's handle for the MR in requests. */
static inline int ctlv_attestation_mr_get_items_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_GET_ITEMS_ID, value);
}

/* One per MR, ordered by ID. */
static inline int ctlv_attestation_mr_get_items_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_GET_ITEMS);
}

/* The MR to read, by the ID mr-get lists. */
static inline int ctlv_attestation_mr_read_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_READ_ID, value);
}

/* The device's family's fields. */
static inline int ctlv_attestation_mr_read_options_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_READ_OPTIONS);
}

/* The MR to extend, by the ID mr-get lists. */
static inline int ctlv_attestation_mr_extend_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_EXTEND_ID, value);
}

/* Data the device hashes for each bank, in place of the caller's digests. */
static inline int ctlv_attestation_mr_extend_raw_data_put_va(struct ctlv_msg *msg,
							     const void *address, __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_ATTESTATION_T_MR_EXTEND_RAW_DATA, address, length);
}

static inline int ctlv_attestation_mr_extend_raw_data_put_memfd(struct ctlv_msg *msg, int fd,
								__u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_ATTESTATION_T_MR_EXTEND_RAW_DATA, fd, offset, length);
}

static inline int ctlv_attestation_mr_extend_raw_data_put_dma_buf(struct ctlv_msg *msg, int fd,
								  __u64 offset, __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_ATTESTATION_T_MR_EXTEND_RAW_DATA, fd, offset,
				     length);
}

/* The device's family's fields. */
static inline int ctlv_attestation_mr_extend_options_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_EXTEND_OPTIONS);
}

/* The layout of data, in printable ASCII without spaces. */
static inline int ctlv_attestation_mr_extend_record_format_put(struct ctlv_msg *msg,
							       const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_FORMAT, value);
}

/* What was measured, laid out as format says. */
static inline int ctlv_attestation_mr_extend_record_data_put(struct ctlv_msg *msg,
							     const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_DATA, value, len);
}

/* What the data extended by is, recorded in the journal with it. */
static inline int ctlv_attestation_mr_extend_record_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_EXTEND_RECORD);
}

/* The entry, as mr-log-get returns it. */
static inline int ctlv_attestation_mr_log_entry_appended_entry_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY);
}

/* The evidence the verifier accepted, as evidence-get numbered it. */
static inline int ctlv_attestation_evidence_accept_evidence_generation_put(struct ctlv_msg *msg,
									   __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_ACCEPT_EVIDENCE_GENERATION, value);
}

/* The device interface's TDISP state, where the provider reports one. */
static inline int ctlv_attestation_devsec_state_get_tdisp_state_put(struct ctlv_msg *msg,
								    __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_DEVSEC_STATE_GET_TDISP_STATE, value);
}

/* The latest evidence, once evidence of this device state exists. */
static inline int
ctlv_attestation_evidence_state_changed_evidence_generation_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_EVIDENCE_STATE_CHANGED_EVIDENCE_GENERATION,
			    value);
}

/* An MR's ID. */
static inline int ctlv_attestation_mr_log_get_selection_id_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID, value);
}

/* Only these MRs' entries; every MR's without it. */
static inline int ctlv_attestation_mr_log_get_selection_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION);
}

/* The entries, in journal order. */
static inline int ctlv_attestation_mr_log_get_entries_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_ATTESTATION_T_MR_LOG_GET_ENTRIES);
}

/* Reading a reply or an event. A getter answers whether it could read. */

/* The platform's name for the MR, such as rtmr0. */
static inline bool ctlv_attestation_mr_desc_name_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_NAME);
}

static inline bool ctlv_attestation_mr_desc_name_get(const struct ctlv_hdr *record,
						     const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_NAME);
	return hdr && !ctlv_get_str(hdr, value);
}

/* Its banks, each once, in the provider's order. */
#define ctlv_attestation_mr_desc_supported_hash_algorithms_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS)

static inline bool
ctlv_attestation_mr_desc_supported_hash_algorithms_get(const struct ctlv_hdr *hdr, __u64 *value,
						       bool *known)
{
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* What it allows, and whether the provider may remove it. */
static inline bool ctlv_attestation_mr_desc_caps_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_CAPS);
}

static inline bool ctlv_attestation_mr_desc_caps_get(const struct ctlv_hdr *record, __u64 *value,
						     __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_CAPS);
	return hdr && !ctlv_get_flags(hdr, CTLV_ATTESTATION_MR_CAPS_MASK, value, unknown);
}

/* The hardware's number for the MR, where it has one: 0 for rtmr0. */
static inline bool ctlv_attestation_mr_desc_register_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX);
}

static inline bool ctlv_attestation_mr_desc_register_index_get(const struct ctlv_hdr *record,
							       __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* ima-active while IMA extends it. */
static inline bool ctlv_attestation_mr_desc_state_flags_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_STATE_FLAGS);
}

static inline bool ctlv_attestation_mr_desc_state_flags_get(const struct ctlv_hdr *record,
							    __u64 *value, __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_DESC_STATE_FLAGS);
	return hdr && !ctlv_get_flags(hdr, CTLV_ATTESTATION_MR_STATE_FLAGS_MASK, value, unknown);
}

/* The bank's algorithm. */
static inline bool ctlv_attestation_mr_value_hash_algo_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO);
}

static inline bool ctlv_attestation_mr_value_hash_algo_get(const struct ctlv_hdr *record,
							   __u64 *value, bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO);
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* The digest, as long as hash-algo's. */
static inline bool ctlv_attestation_mr_value_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_VALUE_DATA);
}

static inline bool ctlv_attestation_mr_value_data_get(const struct ctlv_hdr *record,
						      const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_VALUE_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* The device's MR generation, which each extend bumps. */
static inline bool ctlv_attestation_mr_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GENERATION);
}

static inline bool ctlv_attestation_mr_generation_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Values for banks of the MR, each bank once. */
#define ctlv_attestation_mr_values_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_MR_VALUES)

/* Advances with each change of the device's state. */
static inline bool ctlv_attestation_device_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GENERATION);
}

static inline bool ctlv_attestation_device_generation_get(const struct ctlv_hdr *record,
							  __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The ID of the MR the entry is about. */
static inline bool ctlv_attestation_mr_log_entry_mr_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID);
}

static inline bool ctlv_attestation_mr_log_entry_mr_id_get(const struct ctlv_hdr *record,
							   __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* What the entry records. */
static inline bool ctlv_attestation_mr_log_entry_event_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT);
}

static inline bool ctlv_attestation_mr_log_entry_event_get(const struct ctlv_hdr *record,
							   __u64 *value, bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT);
	return hdr && !ctlv_get_enum(hdr, 1, 2, value, known);
}

/* The platform's name for the MR. */
static inline bool ctlv_attestation_mr_log_entry_mr_name_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME);
}

static inline bool ctlv_attestation_mr_log_entry_mr_name_get(const struct ctlv_hdr *record,
							     const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME);
	return hdr && !ctlv_get_str(hdr, value);
}

/* Its banks, each once, in the provider's order. */
#define ctlv_attestation_mr_log_entry_mr_supported_hash_algorithms_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, \
			   CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS)

static inline bool
ctlv_attestation_mr_log_entry_mr_supported_hash_algorithms_get(const struct ctlv_hdr *hdr,
							       __u64 *value, bool *known)
{
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* What it allows, and whether the provider may remove it. */
static inline bool ctlv_attestation_mr_log_entry_mr_caps_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS);
}

static inline bool ctlv_attestation_mr_log_entry_mr_caps_get(const struct ctlv_hdr *record,
							     __u64 *value, __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS);
	return hdr && !ctlv_get_flags(hdr, CTLV_ATTESTATION_MR_CAPS_MASK, value, unknown);
}

/* The hardware's number for the MR, where it has one. */
static inline bool
ctlv_attestation_mr_log_entry_mr_register_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX);
}

static inline bool
ctlv_attestation_mr_log_entry_mr_register_index_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The MR an info entry describes. */
static inline bool ctlv_attestation_mr_log_entry_mr_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_log_entry_mr_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR);
}

/* The IMA template, such as ima-ng. */
static inline bool
ctlv_attestation_mr_log_entry_ima_template_name_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME);
}

static inline bool
ctlv_attestation_mr_log_entry_ima_template_name_get(const struct ctlv_hdr *record,
						    const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME);
	return hdr && !ctlv_get_str(hdr, value);
}

/* The template's fields, as IMA's binary measurement list has them. */
static inline bool
ctlv_attestation_mr_log_entry_ima_template_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA);
}

static inline bool
ctlv_attestation_mr_log_entry_ima_template_data_get(const struct ctlv_hdr *record,
						    const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* The entry records an IMA violation, not a measurement. */
static inline bool
ctlv_attestation_mr_log_entry_ima_violation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION);
}

static inline bool ctlv_attestation_mr_log_entry_ima_violation_get(const struct ctlv_hdr *record,
								   bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* IMA's measurement, on an extend entry IMA made. */
static inline bool ctlv_attestation_mr_log_entry_ima_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_log_entry_ima_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA);
}

/* The layout of data, in printable ASCII without spaces. */
static inline bool
ctlv_attestation_mr_log_entry_record_format_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT);
}

static inline bool ctlv_attestation_mr_log_entry_record_format_get(const struct ctlv_hdr *record,
								   const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT);
	return hdr && !ctlv_get_str(hdr, value);
}

/* What was measured, laid out as format says. */
static inline bool ctlv_attestation_mr_log_entry_record_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA);
}

static inline bool ctlv_attestation_mr_log_entry_record_data_get(const struct ctlv_hdr *record,
								 const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* What was measured, on an extend entry mr-extend made with a record. */
static inline bool ctlv_attestation_mr_log_entry_record_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_log_entry_record_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD);
}

/* The first MR generation of the range mr-log-get selects and reports. */
static inline bool ctlv_attestation_mr_start_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_START_GENERATION);
}

static inline bool ctlv_attestation_mr_start_generation_get(const struct ctlv_hdr *record,
							    __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_START_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The last MR generation of that range. */
static inline bool ctlv_attestation_mr_end_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_END_GENERATION);
}

static inline bool ctlv_attestation_mr_end_generation_get(const struct ctlv_hdr *record,
							  __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_END_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device's handle for the object in requests. */
static inline bool ctlv_attestation_device_get_objects_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID);
}

static inline bool ctlv_attestation_device_get_objects_id_get(const struct ctlv_hdr *record,
							      __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* What the object is. */
static inline bool ctlv_attestation_device_get_objects_class_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS);
}

static inline bool ctlv_attestation_device_get_objects_class_get(const struct ctlv_hdr *record,
								 __u64 *value, bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS);
	return hdr && !ctlv_get_enum(hdr, 1, 4, value, known);
}

/*
 * The layouts a standard defines that the object may have. At least one of formats and
 * vendor-formats is present.
 */
#define ctlv_attestation_device_get_objects_formats_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_FORMATS)

static inline bool ctlv_attestation_device_get_objects_formats_get(const struct ctlv_hdr *hdr,
								   __u64 *value, bool *known)
{
	return hdr && !ctlv_get_enum(hdr, 1, 8, value, known);
}

/* The layouts its vendor defines that the object may have. */
#define ctlv_attestation_device_get_objects_vendor_formats_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_VENDOR_FORMATS)

static inline bool
ctlv_attestation_device_get_objects_vendor_formats_get(const struct ctlv_hdr *hdr, __u64 *value)
{
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The longest the object gets; a blob this long always holds it. */
static inline bool
ctlv_attestation_device_get_objects_max_length_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH);
}

static inline bool ctlv_attestation_device_get_objects_max_length_get(const struct ctlv_hdr *record,
								      __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* A single length. */
static inline bool
ctlv_attestation_device_get_challenge_lengths_length_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH);
}

static inline bool
ctlv_attestation_device_get_challenge_lengths_length_get(const struct ctlv_hdr *record,
							 __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The shortest of a range. */
static inline bool
ctlv_attestation_device_get_challenge_lengths_min_length_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH);
}

static inline bool
ctlv_attestation_device_get_challenge_lengths_min_length_get(const struct ctlv_hdr *record,
							     __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The longest of a range. */
static inline bool
ctlv_attestation_device_get_challenge_lengths_max_length_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH);
}

static inline bool
ctlv_attestation_device_get_challenge_lengths_max_length_get(const struct ctlv_hdr *record,
							     __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH);
	return hdr && !ctlv_get_u64(hdr, value);
}

/*
 * A signed object's challenge lengths: each entry is one length, or a range from min-length to
 * max-length.
 */
#define ctlv_attestation_device_get_objects_challenge_lengths_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CHALLENGE_LENGTHS)

/* Tells apart the objects of one kind, such as an SPDM slot. */
static inline bool
ctlv_attestation_device_get_objects_instance_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_INSTANCE);
}

static inline bool ctlv_attestation_device_get_objects_instance_get(const struct ctlv_hdr *record,
								    __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_INSTANCE);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Where the TCB vouches for the object with a digest, its algorithm. */
static inline bool
ctlv_attestation_device_get_objects_tcb_digest_algo_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_TCB_DIGEST_ALGO);
}

static inline bool
ctlv_attestation_device_get_objects_tcb_digest_algo_get(const struct ctlv_hdr *record, __u64 *value,
							bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_TCB_DIGEST_ALGO);
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* One per object evidence-get can produce. */
#define ctlv_attestation_device_get_evidence_objects_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_OBJECTS)

/* The optional evidence features the device provides. */
static inline bool ctlv_attestation_device_get_evidence_flags_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_FLAGS);
}

static inline bool ctlv_attestation_device_get_evidence_flags_get(const struct ctlv_hdr *record,
								  __u64 *value, __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_FLAGS);
	return hdr && !ctlv_get_flags(hdr, CTLV_ATTESTATION_EVIDENCE_CAPS_MASK, value, unknown);
}

/* Present when the device produces evidence. */
static inline bool ctlv_attestation_device_get_caps_evidence_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE);
}

static inline const struct ctlv_hdr *
ctlv_attestation_device_get_caps_evidence_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE);
}

/* The MR operations the device provides, and whether MRs come and go. */
static inline bool ctlv_attestation_device_get_mr_flags_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS);
}

static inline bool ctlv_attestation_device_get_mr_flags_get(const struct ctlv_hdr *record,
							    __u64 *value, __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS);
	return hdr && !ctlv_get_flags(hdr, CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK, value, unknown);
}

/* Present when the device has MRs. */
static inline bool ctlv_attestation_device_get_caps_mr_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR);
}

static inline const struct ctlv_hdr *
ctlv_attestation_device_get_caps_mr_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR);
}

/* What the device supports, fixed for its life. */
static inline bool ctlv_attestation_device_get_caps_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS);
}

static inline const struct ctlv_hdr *
ctlv_attestation_device_get_caps_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_CAPS);
}

/* The device's family's fields. */
static inline bool ctlv_attestation_device_get_info_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_INFO);
}

static inline const struct ctlv_hdr *
ctlv_attestation_device_get_info_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVICE_GET_INFO);
}

/*
 * Bound into each signed object named: pass a verifier's nonce, or a hash binding one to your
 * context, never data a peer chose. Present exactly when a signed object is named, and of a length
 * each of them takes.
 */
static inline bool ctlv_attestation_evidence_get_challenge_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE);
}

static inline bool ctlv_attestation_evidence_get_challenge_get(const struct ctlv_hdr *record,
							       const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* The object, by the ID device-get lists. */
static inline bool ctlv_attestation_evidence_get_objects_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID);
}

static inline bool ctlv_attestation_evidence_get_objects_id_get(const struct ctlv_hdr *record,
								__u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The objects to produce, each named once. */
#define ctlv_attestation_evidence_get_objects_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS)

/* The MR's ID. */
static inline bool ctlv_attestation_evidence_get_selection_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_ID);
}

static inline bool ctlv_attestation_evidence_get_selection_id_get(const struct ctlv_hdr *record,
								  __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The bank's algorithm. */
static inline bool
ctlv_attestation_evidence_get_selection_hash_algo_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_HASH_ALGO);
}

static inline bool
ctlv_attestation_evidence_get_selection_hash_algo_get(const struct ctlv_hdr *record, __u64 *value,
						      bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_HASH_ALGO);
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* The MR banks the evidence reports, where the device takes mr-selection. */
#define ctlv_attestation_evidence_get_selection_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION)

/* The privilege level to report for, as configfs-tsm's privlevel. */
static inline bool ctlv_attestation_evidence_get_privlevel_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_PRIVLEVEL);
}

static inline bool ctlv_attestation_evidence_get_privlevel_get(const struct ctlv_hdr *record,
							       __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_PRIVLEVEL);
	return hdr && !ctlv_get_u64(hdr, value);
}

/*
 * The service to attest, as configfs-tsm's service_provider; only with service-guid and
 * service-manifest-version.
 */
static inline bool
ctlv_attestation_evidence_get_service_provider_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_PROVIDER);
}

static inline bool ctlv_attestation_evidence_get_service_provider_get(const struct ctlv_hdr *record,
								      const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_PROVIDER);
	return hdr && !ctlv_get_str(hdr, value);
}

/* The service's GUID, as configfs-tsm's service_guid. */
static inline bool ctlv_attestation_evidence_get_service_guid_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_GUID);
}

static inline bool ctlv_attestation_evidence_get_service_guid_get(const struct ctlv_hdr *record,
								  const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_GUID);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* Its manifest version, as configfs-tsm's service_manifest_version. */
static inline bool
ctlv_attestation_evidence_get_service_manifest_version_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_MANIFEST_VERSION);
}

static inline bool
ctlv_attestation_evidence_get_service_manifest_version_get(const struct ctlv_hdr *record,
							   __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_MANIFEST_VERSION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device's family's fields. */
static inline bool ctlv_attestation_evidence_get_options_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_OPTIONS);
}

static inline const struct ctlv_hdr *
ctlv_attestation_evidence_get_options_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_OPTIONS);
}

/*
 * The layout it came in, where a standard defines it. Exactly one of format and vendor-format is
 * present.
 */
static inline bool
ctlv_attestation_evidence_get_results_format_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT);
}

static inline bool ctlv_attestation_evidence_get_results_format_get(const struct ctlv_hdr *record,
								    __u64 *value, bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT);
	return hdr && !ctlv_get_enum(hdr, 1, 8, value, known);
}

/* The layout it came in, where its vendor defines it. */
static inline bool
ctlv_attestation_evidence_get_results_vendor_format_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT);
}

static inline bool
ctlv_attestation_evidence_get_results_vendor_format_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The bytes written to its blob. */
static inline bool
ctlv_attestation_evidence_get_results_length_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH);
}

static inline bool ctlv_attestation_evidence_get_results_length_get(const struct ctlv_hdr *record,
								    __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The object's tcb-digest-algo. */
static inline bool
ctlv_attestation_evidence_get_tcb_digest_hash_algo_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_HASH_ALGO);
}

static inline bool
ctlv_attestation_evidence_get_tcb_digest_hash_algo_get(const struct ctlv_hdr *record, __u64 *value,
						       bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_HASH_ALGO);
	return hdr && !ctlv_get_enum(hdr, HASH_ALGO_MD4, HASH_ALGO__LAST - 1, value, known);
}

/* The digest, as long as hash-algo's. */
static inline bool
ctlv_attestation_evidence_get_tcb_digest_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_DATA);
}

static inline bool ctlv_attestation_evidence_get_tcb_digest_data_get(const struct ctlv_hdr *record,
								     const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* Where the TCB vouches for the object, the digest it vouches with. */
static inline bool
ctlv_attestation_evidence_get_results_tcb_digest_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_TCB_DIGEST);
}

static inline const struct ctlv_hdr *
ctlv_attestation_evidence_get_results_tcb_digest_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_TCB_DIGEST);
}

/* One per object, in request order. */
#define ctlv_attestation_evidence_get_results_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS)

/* On a device with evidence-accept, numbers this evidence for it. */
static inline bool
ctlv_attestation_evidence_get_evidence_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_EVIDENCE_GENERATION);
}

static inline bool
ctlv_attestation_evidence_get_evidence_generation_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_GET_EVIDENCE_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Only this MR; every MR without it. */
static inline bool ctlv_attestation_mr_get_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GET_ID);
}

static inline bool ctlv_attestation_mr_get_id_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GET_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device's handle for the MR in requests. */
static inline bool ctlv_attestation_mr_get_items_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GET_ITEMS_ID);
}

static inline bool ctlv_attestation_mr_get_items_id_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_GET_ITEMS_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* One per MR, ordered by ID. */
#define ctlv_attestation_mr_get_items_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_MR_GET_ITEMS)

/* The MR to read, by the ID mr-get lists. */
static inline bool ctlv_attestation_mr_read_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_READ_ID);
}

static inline bool ctlv_attestation_mr_read_id_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_READ_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device's family's fields. */
static inline bool ctlv_attestation_mr_read_options_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_READ_OPTIONS);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_read_options_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_READ_OPTIONS);
}

/* The MR to extend, by the ID mr-get lists. */
static inline bool ctlv_attestation_mr_extend_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_ID);
}

static inline bool ctlv_attestation_mr_extend_id_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device's family's fields. */
static inline bool ctlv_attestation_mr_extend_options_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_OPTIONS);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_extend_options_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_OPTIONS);
}

/* The layout of data, in printable ASCII without spaces. */
static inline bool ctlv_attestation_mr_extend_record_format_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_FORMAT);
}

static inline bool ctlv_attestation_mr_extend_record_format_get(const struct ctlv_hdr *record,
								const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_FORMAT);
	return hdr && !ctlv_get_str(hdr, value);
}

/* What was measured, laid out as format says. */
static inline bool ctlv_attestation_mr_extend_record_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_DATA);
}

static inline bool ctlv_attestation_mr_extend_record_data_get(const struct ctlv_hdr *record,
							      const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* What the data extended by is, recorded in the journal with it. */
static inline bool ctlv_attestation_mr_extend_record_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_extend_record_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_EXTEND_RECORD);
}

/* The entry, as mr-log-get returns it. */
static inline bool
ctlv_attestation_mr_log_entry_appended_entry_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY);
}

static inline const struct ctlv_hdr *
ctlv_attestation_mr_log_entry_appended_entry_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY);
}

/* The evidence the verifier accepted, as evidence-get numbered it. */
static inline bool
ctlv_attestation_evidence_accept_evidence_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_ACCEPT_EVIDENCE_GENERATION);
}

static inline bool
ctlv_attestation_evidence_accept_evidence_generation_get(const struct ctlv_hdr *record,
							 __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_ACCEPT_EVIDENCE_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The device interface's TDISP state, where the provider reports one. */
static inline bool
ctlv_attestation_devsec_state_get_tdisp_state_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVSEC_STATE_GET_TDISP_STATE);
}

static inline bool ctlv_attestation_devsec_state_get_tdisp_state_get(const struct ctlv_hdr *record,
								     __u64 *value, bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_DEVSEC_STATE_GET_TDISP_STATE);
	return hdr && !ctlv_get_enum(hdr, 1, 4, value, known);
}

/* The latest evidence, once evidence of this device state exists. */
static inline bool
ctlv_attestation_evidence_state_changed_evidence_generation_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_STATE_CHANGED_EVIDENCE_GENERATION);
}

static inline bool
ctlv_attestation_evidence_state_changed_evidence_generation_get(const struct ctlv_hdr *record,
								__u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_EVIDENCE_STATE_CHANGED_EVIDENCE_GENERATION);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* An MR's ID. */
static inline bool ctlv_attestation_mr_log_get_selection_id_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID);
}

static inline bool ctlv_attestation_mr_log_get_selection_id_get(const struct ctlv_hdr *record,
								__u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Only these MRs' entries; every MR's without it. */
#define ctlv_attestation_mr_log_get_selection_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION)

/* The entries, in journal order. */
#define ctlv_attestation_mr_log_get_entries_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_ATTESTATION_T_MR_LOG_GET_ENTRIES)

/* Watching for events. A subscription is per open file description. */

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
