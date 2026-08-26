/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-test CTLV family. */

#ifndef _CTLV_CTLV_TEST_USER_GEN_H
#define _CTLV_CTLV_TEST_USER_GEN_H

#include <ctlv.h>

#include <linux/ctlv_test_gen.h>

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_test_ctlv_spec;

/* Executing an op. A query is reassembled into the one reply it fills. */

#define CTLV_TEST_OP_NOOP_REQUEST_MAX_LEN 16

static inline int ctlv_test_noop_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_NOOP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_noop(struct ctlv_dev *dev, struct ctlv_msg *request,
				 struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_TEST_OP_NOOP, request, 0, NULL,
				 error);
}

#define CTLV_TEST_OP_PROBE_GET_REQUEST_MAX_LEN 40
#define CTLV_TEST_OP_PROBE_GET_QUERY_REPLY_MIN_LEN 40
#define CTLV_TEST_OP_PROBE_GET_QUERY_REPLY_SUGGESTED_LEN 65536

static inline int ctlv_test_probe_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_PROBE_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_probe_get(struct ctlv_dev *dev,
				      struct ctlv_msg *request,
				      struct ctlv_reply **reply,
				      struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_TEST_OP_PROBE_GET, request, reply, error);
}

#define CTLV_TEST_OP_DYNAMIC_NOOP_REQUEST_MAX_LEN 16

static inline int ctlv_test_dynamic_noop_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_DYNAMIC_NOOP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_dynamic_noop(struct ctlv_dev *dev,
					 struct ctlv_msg *request,
					 struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_TEST_OP_DYNAMIC_NOOP, request, 0,
				 NULL, error);
}

#define CTLV_TEST_OP_DYNAMIC_TOGGLE_REQUEST_MAX_LEN 40
#define CTLV_TEST_OP_DYNAMIC_TOGGLE_ACTION_REPLY_MAX_LEN 40

static inline int
ctlv_test_dynamic_toggle_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_DYNAMIC_TOGGLE_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_dynamic_toggle(struct ctlv_dev *dev,
					   struct ctlv_msg *request,
					   struct ctlv_reply **reply,
					   struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_DYNAMIC_TOGGLE, request, reply,
			   error);
}

#define CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN 736

static inline int ctlv_test_echo_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_echo(struct ctlv_dev *dev, struct ctlv_msg *request,
				 struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_TEST_OP_ECHO, request, 0, NULL,
				 error);
}

#define CTLV_TEST_OP_ECHO_REPLY_REQUEST_MAX_LEN 736
#define CTLV_TEST_OP_ECHO_REPLY_ACTION_REPLY_MAX_LEN 512

static inline int ctlv_test_echo_reply_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_ECHO_REPLY_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_echo_reply(struct ctlv_dev *dev,
				       struct ctlv_msg *request,
				       struct ctlv_reply **reply,
				       struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_ECHO_REPLY, request, reply, error);
}

#define CTLV_TEST_OP_SERIES_DUMP_REQUEST_MAX_LEN 112
#define CTLV_TEST_OP_SERIES_DUMP_QUERY_REPLY_MIN_LEN 72
#define CTLV_TEST_OP_SERIES_DUMP_QUERY_REPLY_SUGGESTED_LEN 4096

static inline int ctlv_test_series_dump_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_SERIES_DUMP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_series_dump(struct ctlv_dev *dev,
					struct ctlv_msg *request,
					struct ctlv_reply **reply,
					struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_TEST_OP_SERIES_DUMP, request, reply, error);
}

#define CTLV_TEST_OP_PING_REQUEST_MAX_LEN 136

static inline int ctlv_test_ping_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_PING_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_ping(struct ctlv_dev *dev, struct ctlv_msg *request,
				 struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_TEST_OP_PING, request, 0, NULL,
				 error);
}

#define CTLV_TEST_OP_TRANSFER_REQUEST_MAX_LEN 896
#define CTLV_TEST_OP_TRANSFER_ACTION_REPLY_MAX_LEN 88

static inline int ctlv_test_transfer_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_TRANSFER_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_transfer(struct ctlv_dev *dev,
				     struct ctlv_msg *request,
				     struct ctlv_reply **reply,
				     struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_TRANSFER, request, reply, error);
}

/* Building a request. ctlv_msg_err() reports the first failure. */

static inline int ctlv_test_probe_token_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_PROBE_TOKEN, value);
}

static inline int ctlv_test_probe_extension_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_TEST_ATTR_PROBE_EXTENSION);
}

static inline int ctlv_test_echo_u_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_ECHO_U, value);
}

static inline int ctlv_test_echo_s_put(struct ctlv_msg *msg, __s64 value)
{
	return ctlv_put_s64(msg, CTLV_TEST_ATTR_ECHO_S, value);
}

static inline int ctlv_test_echo_flag_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_ECHO_FLAG, value);
}

static inline int ctlv_test_echo_mode_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_ECHO_MODE, value);
}

static inline int ctlv_test_echo_quirks_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_ECHO_QUIRKS, value);
}

static inline int ctlv_test_echo_text_put(struct ctlv_msg *msg,
					  const char *value)
{
	return ctlv_put_str(msg, CTLV_TEST_ATTR_ECHO_TEXT, value);
}

static inline int ctlv_test_echo_data_put(struct ctlv_msg *msg,
					  const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_TEST_ATTR_ECHO_DATA, value, len);
}

static inline int ctlv_test_echo_count_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_ECHO_COUNT, value);
}

static inline int ctlv_test_echo_item_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_TEST_ATTR_ECHO_ITEM);
}

static inline int ctlv_test_echo_region_put_va(struct ctlv_msg *msg,
					       const void *address,
					       __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_ATTR_ECHO_REGION, address,
				length);
}

static inline int ctlv_test_echo_region_put_memfd(struct ctlv_msg *msg, int fd,
						  __u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_ECHO_REGION, fd, offset,
				   length);
}

static inline int ctlv_test_echo_region_put_dma_buf(struct ctlv_msg *msg,
						    int fd, __u64 offset,
						    __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_ATTR_ECHO_REGION, fd,
				     offset, length);
}

static inline int ctlv_test_echo_page_put_memfd(struct ctlv_msg *msg, int fd,
						__u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_ECHO_PAGE, fd, offset,
				   length);
}

static inline int ctlv_test_item_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_ITEM_INDEX, value);
}

static inline int ctlv_test_item_label_put(struct ctlv_msg *msg,
					   const char *value)
{
	return ctlv_put_str(msg, CTLV_TEST_ATTR_ITEM_LABEL, value);
}

static inline int ctlv_test_item_leaf_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_TEST_ATTR_ITEM_LEAF);
}

static inline int ctlv_test_series_chunks_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_CHUNKS, value);
}

static inline int ctlv_test_series_items_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_ITEMS, value);
}

static inline int ctlv_test_series_values_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_VALUES, value);
}

static inline int ctlv_test_series_totals_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_TOTALS, value);
}

static inline int ctlv_test_series_chunk_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_TEST_ATTR_SERIES_CHUNK);
}

static inline int ctlv_test_series_item_start(struct ctlv_msg *msg)
{
	return ctlv_nest_start(msg, CTLV_TEST_ATTR_SERIES_ITEM);
}

static inline int ctlv_test_series_value_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_VALUE, value);
}

static inline int ctlv_test_series_total_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_TOTAL, value);
}

static inline int ctlv_test_series_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_SERIES_INDEX, value);
}

static inline int ctlv_test_ping_count_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_PING_COUNT, value);
}

static inline int ctlv_test_ping_serial_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_PING_SERIAL, value);
}

static inline int ctlv_test_ping_payload_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_PING_PAYLOAD, value);
}

static inline int ctlv_test_ping_abort_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_PING_ABORT, value);
}

static inline int ctlv_test_ping_empty_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_PING_EMPTY, value);
}

static inline int ctlv_test_ping_filler_put(struct ctlv_msg *msg,
					    const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_TEST_ATTR_PING_FILLER, value, len);
}

static inline int ctlv_test_transfer_source_put_va(struct ctlv_msg *msg,
						   const void *address,
						   __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_ATTR_TRANSFER_SOURCE, address,
				length);
}

static inline int ctlv_test_transfer_source_put_memfd(struct ctlv_msg *msg,
						      int fd, __u64 offset,
						      __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_TRANSFER_SOURCE, fd,
				   offset, length);
}

static inline int ctlv_test_transfer_source_put_dma_buf(struct ctlv_msg *msg,
							int fd, __u64 offset,
							__u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_ATTR_TRANSFER_SOURCE, fd,
				     offset, length);
}

static inline int ctlv_test_transfer_target_put_va(struct ctlv_msg *msg,
						   const void *address,
						   __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_ATTR_TRANSFER_TARGET, address,
				length);
}

static inline int ctlv_test_transfer_target_put_memfd(struct ctlv_msg *msg,
						      int fd, __u64 offset,
						      __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_TRANSFER_TARGET, fd,
				   offset, length);
}

static inline int ctlv_test_transfer_target_put_dma_buf(struct ctlv_msg *msg,
							int fd, __u64 offset,
							__u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_ATTR_TRANSFER_TARGET, fd,
				     offset, length);
}

static inline int ctlv_test_transfer_scratch_put_va(struct ctlv_msg *msg,
						    const void *address,
						    __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_ATTR_TRANSFER_SCRATCH, address,
				length);
}

static inline int ctlv_test_transfer_scratch_put_memfd(struct ctlv_msg *msg,
						       int fd, __u64 offset,
						       __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_TRANSFER_SCRATCH, fd,
				   offset, length);
}

static inline int ctlv_test_transfer_scratch_put_dma_buf(struct ctlv_msg *msg,
							 int fd, __u64 offset,
							 __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_ATTR_TRANSFER_SCRATCH, fd,
				     offset, length);
}

static inline int ctlv_test_transfer_piece_put_va(struct ctlv_msg *msg,
						  const void *address,
						  __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_ATTR_TRANSFER_PIECE, address,
				length);
}

static inline int ctlv_test_transfer_piece_put_memfd(struct ctlv_msg *msg,
						     int fd, __u64 offset,
						     __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_ATTR_TRANSFER_PIECE, fd,
				   offset, length);
}

static inline int ctlv_test_transfer_piece_put_dma_buf(struct ctlv_msg *msg,
						       int fd, __u64 offset,
						       __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_ATTR_TRANSFER_PIECE, fd,
				     offset, length);
}

static inline int ctlv_test_transfer_fill_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_TRANSFER_FILL, value);
}

static inline int ctlv_test_transfer_skip_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_TRANSFER_SKIP, value);
}

static inline int ctlv_test_transfer_twice_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_TRANSFER_TWICE, value);
}

static inline int ctlv_test_transfer_hold_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_TRANSFER_HOLD, value);
}

static inline int ctlv_test_transfer_sum_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_TRANSFER_SUM, value);
}

static inline int ctlv_test_transfer_written_put(struct ctlv_msg *msg,
						 __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_ATTR_TRANSFER_WRITTEN, value);
}

static inline int ctlv_test_transfer_distinct_put(struct ctlv_msg *msg,
						  bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_ATTR_TRANSFER_DISTINCT, value);
}

/* Reading a reply or a record. A getter answers whether it could read. */

static inline bool ctlv_test_probe_token_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PROBE_TOKEN);
}

static inline bool ctlv_test_probe_token_get(const struct ctlv_attr *nest,
					     __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PROBE_TOKEN);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool
ctlv_test_probe_extension_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PROBE_EXTENSION);
}

static inline const struct ctlv_attr *
ctlv_test_probe_extension_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PROBE_EXTENSION);
}

static inline bool ctlv_test_echo_u_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_U);
}

static inline bool ctlv_test_echo_u_get(const struct ctlv_attr *nest,
					__u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_U);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_echo_s_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_S);
}

static inline bool ctlv_test_echo_s_get(const struct ctlv_attr *nest,
					__s64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_S);
	return attr && !ctlv_get_s64(attr, value);
}

static inline bool ctlv_test_echo_flag_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_FLAG);
}

static inline bool ctlv_test_echo_flag_get(const struct ctlv_attr *nest,
					   bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_FLAG);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_test_echo_mode_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_MODE);
}

static inline bool ctlv_test_echo_mode_get(const struct ctlv_attr *nest,
					   __u64 *value, bool *known)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_MODE);
	return attr && !ctlv_get_enum(attr, 3, value, known);
}

static inline bool ctlv_test_echo_quirks_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_QUIRKS);
}

static inline bool ctlv_test_echo_quirks_get(const struct ctlv_attr *nest,
					     __u64 *value, __u64 *unknown)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_QUIRKS);
	return attr && !ctlv_get_flags(attr, CTLV_TEST_ECHO_QUIRK_MASK, value,
				       unknown);
}

static inline bool ctlv_test_echo_text_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_TEXT);
}

static inline bool ctlv_test_echo_text_get(const struct ctlv_attr *nest,
					   const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_TEXT);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_test_echo_data_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_DATA);
}

static inline bool ctlv_test_echo_data_get(const struct ctlv_attr *nest,
					   const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ECHO_DATA);
	return attr && !ctlv_get_bin(attr, value, len);
}

#define ctlv_test_echo_for_each_count(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_ECHO_COUNT)

static inline bool ctlv_test_echo_count_get(const struct ctlv_attr *attr,
					    __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_test_echo_for_each_item(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_ECHO_ITEM)

static inline bool ctlv_test_item_index_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_INDEX);
}

static inline bool ctlv_test_item_index_get(const struct ctlv_attr *nest,
					    __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_INDEX);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_item_label_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_LABEL);
}

static inline bool ctlv_test_item_label_get(const struct ctlv_attr *nest,
					    const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_LABEL);
	return attr && !ctlv_get_str(attr, value);
}

static inline bool ctlv_test_item_leaf_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_LEAF);
}

static inline const struct ctlv_attr *
ctlv_test_item_leaf_get(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_ITEM_LEAF);
}

static inline bool ctlv_test_series_chunks_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_CHUNKS);
}

static inline bool ctlv_test_series_chunks_get(const struct ctlv_attr *nest,
					       __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_CHUNKS);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_series_items_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_ITEMS);
}

static inline bool ctlv_test_series_items_get(const struct ctlv_attr *nest,
					      __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_ITEMS);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_series_values_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_VALUES);
}

static inline bool ctlv_test_series_values_get(const struct ctlv_attr *nest,
					       __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_VALUES);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_series_totals_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_TOTALS);
}

static inline bool ctlv_test_series_totals_get(const struct ctlv_attr *nest,
					       __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_TOTALS);
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_test_series_for_each_chunk(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_SERIES_CHUNK)

#define ctlv_test_series_for_each_item(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_SERIES_ITEM)

#define ctlv_test_series_for_each_value(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_SERIES_VALUE)

static inline bool ctlv_test_series_value_get(const struct ctlv_attr *attr,
					      __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

#define ctlv_test_series_for_each_total(_attr, _nest) \
	ctlv_for_each_id(_attr, _nest, CTLV_TEST_ATTR_SERIES_TOTAL)

static inline bool ctlv_test_series_total_get(const struct ctlv_attr *attr,
					      __u64 *value)
{
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_series_index_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_INDEX);
}

static inline bool ctlv_test_series_index_get(const struct ctlv_attr *nest,
					      __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_SERIES_INDEX);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_ping_count_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_COUNT);
}

static inline bool ctlv_test_ping_count_get(const struct ctlv_attr *nest,
					    __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_COUNT);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_ping_serial_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_SERIAL);
}

static inline bool ctlv_test_ping_serial_get(const struct ctlv_attr *nest,
					     __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_SERIAL);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_ping_payload_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_PAYLOAD);
}

static inline bool ctlv_test_ping_payload_get(const struct ctlv_attr *nest,
					      __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_PAYLOAD);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_ping_abort_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_ABORT);
}

static inline bool ctlv_test_ping_abort_get(const struct ctlv_attr *nest,
					    bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_ABORT);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_test_ping_empty_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_EMPTY);
}

static inline bool ctlv_test_ping_empty_get(const struct ctlv_attr *nest,
					    bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_EMPTY);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_test_ping_filler_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_FILLER);
}

static inline bool ctlv_test_ping_filler_get(const struct ctlv_attr *nest,
					     const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_PING_FILLER);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool ctlv_test_transfer_fill_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_FILL);
}

static inline bool ctlv_test_transfer_fill_get(const struct ctlv_attr *nest,
					       __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_FILL);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool ctlv_test_transfer_skip_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_SKIP);
}

static inline bool ctlv_test_transfer_skip_get(const struct ctlv_attr *nest,
					       bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_SKIP);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool
ctlv_test_transfer_twice_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_TWICE);
}

static inline bool ctlv_test_transfer_twice_get(const struct ctlv_attr *nest,
						bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_TWICE);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_test_transfer_hold_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_HOLD);
}

static inline bool ctlv_test_transfer_hold_get(const struct ctlv_attr *nest,
					       bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_HOLD);
	return attr && !ctlv_get_bool(attr, value);
}

static inline bool ctlv_test_transfer_sum_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_SUM);
}

static inline bool ctlv_test_transfer_sum_get(const struct ctlv_attr *nest,
					      __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_SUM);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool
ctlv_test_transfer_written_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_WRITTEN);
}

static inline bool ctlv_test_transfer_written_get(const struct ctlv_attr *nest,
						  __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_WRITTEN);
	return attr && !ctlv_get_u64(attr, value);
}

static inline bool
ctlv_test_transfer_distinct_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_DISTINCT);
}

static inline bool ctlv_test_transfer_distinct_get(const struct ctlv_attr *nest,
						   bool *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_ATTR_TRANSFER_DISTINCT);
	return attr && !ctlv_get_bool(attr, value);
}

/* Watching for events. A subscription is per open file description. */

#define CTLV_TEST_OP_PINGED_EVENT_RECORD_MAX_LEN 40

static inline bool ctlv_test_pinged_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_TEST_OP_PINGED;
}

static inline int ctlv_test_pinged_subscribe(struct ctlv_dev *dev,
					     struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_PINGED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_test_pinged_unsubscribe(struct ctlv_dev *dev,
					       struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_PINGED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#define CTLV_TEST_OP_BEEPED_EVENT_RECORD_MAX_LEN 1104

static inline bool ctlv_test_beeped_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_TEST_OP_BEEPED;
}

static inline int ctlv_test_beeped_subscribe(struct ctlv_dev *dev,
					     struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_BEEPED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_test_beeped_unsubscribe(struct ctlv_dev *dev,
					       struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_BEEPED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#endif /* _CTLV_CTLV_TEST_USER_GEN_H */
