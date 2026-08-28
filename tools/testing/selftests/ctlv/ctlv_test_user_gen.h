/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-test CTLV family. */

#ifndef _CTLV_CTLV_TEST_USER_GEN_H
#define _CTLV_CTLV_TEST_USER_GEN_H

#include <ctlv.h>

#include <ctlv/ctlv_test_gen.h>

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_test_spec;

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
	return ctlv_action(dev, CTLV_TEST_OP_NOOP, request, NULL, error);
}

#define CTLV_TEST_OP_PROBE_GET_REQUEST_MAX_LEN 40

static inline int ctlv_test_probe_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_PROBE_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_probe_get(struct ctlv_dev *dev, struct ctlv_msg *request,
				      struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_TEST_OP_PROBE_GET, request, reply, error);
}

#define CTLV_TEST_OP_DYNAMIC_NOOP_REQUEST_MAX_LEN 16

static inline int ctlv_test_dynamic_noop_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_DYNAMIC_NOOP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_dynamic_noop(struct ctlv_dev *dev, struct ctlv_msg *request,
					 struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_DYNAMIC_NOOP, request, NULL, error);
}

#define CTLV_TEST_OP_DYNAMIC_TOGGLE_REQUEST_MAX_LEN 40

static inline int ctlv_test_dynamic_toggle_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_DYNAMIC_TOGGLE_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_dynamic_toggle(struct ctlv_dev *dev, struct ctlv_msg *request,
					   struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_DYNAMIC_TOGGLE, request, reply, error);
}

#define CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN 760

static inline int ctlv_test_echo_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_ECHO_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_echo(struct ctlv_dev *dev, struct ctlv_msg *request,
				 struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_ECHO, request, NULL, error);
}

#define CTLV_TEST_OP_ECHO_REPLY_REQUEST_MAX_LEN 760

static inline int ctlv_test_echo_reply_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_ECHO_REPLY_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_echo_reply(struct ctlv_dev *dev, struct ctlv_msg *request,
				       struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_ECHO_REPLY, request, reply, error);
}

#define CTLV_TEST_OP_SERIES_GET_REQUEST_MAX_LEN 112

static inline int ctlv_test_series_get_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_SERIES_GET_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_series_get(struct ctlv_dev *dev, struct ctlv_msg *request,
				       struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_query(dev, CTLV_TEST_OP_SERIES_GET, request, reply, error);
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
	return ctlv_action(dev, CTLV_TEST_OP_PING, request, NULL, error);
}

#define CTLV_TEST_OP_TRANSFER_REQUEST_MAX_LEN 896

static inline int ctlv_test_transfer_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_OP_TRANSFER_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_transfer(struct ctlv_dev *dev, struct ctlv_msg *request,
				     struct ctlv_reply **reply, struct ctlv_error *error)
{
	return ctlv_action(dev, CTLV_TEST_OP_TRANSFER, request, reply, error);
}

/* Building a request. ctlv_msg_err() reports the first failure. */

/* The one required field, an integer bounded on both sides. */
static inline int ctlv_test_echo_values_u_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_U, value);
}

/* A signed integer, bounded on both sides. */
static inline int ctlv_test_echo_values_s_put(struct ctlv_msg *msg, __s64 value)
{
	return ctlv_put_s64(msg, CTLV_TEST_T_ECHO_VALUES_S, value);
}

/* A bool. */
static inline int ctlv_test_echo_values_flag_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_ECHO_VALUES_FLAG, value);
}

/* An enum. */
static inline int ctlv_test_echo_values_mode_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_MODE, value);
}

/* A flags. */
static inline int ctlv_test_echo_values_quirks_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_QUIRKS, value);
}

/* A string of 1 to 15 characters. */
static inline int ctlv_test_echo_values_text_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_TEST_T_ECHO_VALUES_TEXT, value);
}

/* A binary of 4 to 32 bytes. */
static inline int ctlv_test_echo_values_data_put(struct ctlv_msg *msg, const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_TEST_T_ECHO_VALUES_DATA, value, len);
}

/* An integer that may occur three times. */
static inline int ctlv_test_echo_values_count_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_COUNT, value);
}

/* A member the item requires. */
static inline int ctlv_test_echo_values_item_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX, value);
}

/* A member it may leave out. */
static inline int ctlv_test_echo_values_item_label_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL, value);
}

/* The leaf's one member. */
static inline int ctlv_test_echo_values_leaf_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX, value);
}

/* The third level, where a record inside a record ends. */
static inline int ctlv_test_echo_values_item_leaf_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF);
}

/* A record in place that may occur twice. */
static inline int ctlv_test_echo_values_item_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_ECHO_VALUES_ITEM);
}

/* Filled by ctlv-test-child, and empty on a ctlv-test device. */
static inline int ctlv_test_echo_values_extension_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_ECHO_VALUES_EXTENSION);
}

/* Valued by ctlv-test-child, and taking nothing on a ctlv-test device. */
static inline int ctlv_test_echo_values_variant_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_ECHO_VALUES_VARIANT, value);
}

/* Any backing at eight bytes. */
static inline int ctlv_test_echo_message_region_put_va(struct ctlv_msg *msg, const void *address,
						       __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_T_ECHO_MESSAGE_REGION, address, length);
}

static inline int ctlv_test_echo_message_region_put_memfd(struct ctlv_msg *msg, int fd,
							  __u64 offset, __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_ECHO_MESSAGE_REGION, fd, offset, length);
}

static inline int ctlv_test_echo_message_region_put_dma_buf(struct ctlv_msg *msg, int fd,
							    __u64 offset, __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_T_ECHO_MESSAGE_REGION, fd, offset, length);
}

/* A memfd at a page. */
static inline int ctlv_test_echo_message_page_put_memfd(struct ctlv_msg *msg, int fd, __u64 offset,
							__u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_ECHO_MESSAGE_PAGE, fd, offset, length);
}

/* What probe-get answers with as it was sent. */
static inline int ctlv_test_token_info_token_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_TOKEN_INFO_TOKEN, value);
}

/* Filled by ctlv-test-child, so one selftest run sees both states. */
static inline int ctlv_test_probe_get_extension_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_PROBE_GET_EXTENSION);
}

/* How many chunks. */
static inline int ctlv_test_series_get_chunks_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_CHUNKS, value);
}

/* How many items in each chunk. */
static inline int ctlv_test_series_get_items_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_ITEMS, value);
}

/* How many values in each item. */
static inline int ctlv_test_series_get_values_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_VALUES, value);
}

/* How many totals after the chunks. */
static inline int ctlv_test_series_get_totals_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_TOTALS, value);
}

/* The chunk's number, from one. */
static inline int ctlv_test_series_get_chunk_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_CHUNK_INDEX, value);
}

/* The item's number in its chunk, from one. */
static inline int ctlv_test_series_get_item_index_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_ITEM_INDEX, value);
}

/*
 * One value, as many as values asks for, each the chunk's number times 100, plus the item's times
 * 10, plus its own.
 */
static inline int ctlv_test_series_get_item_value_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_ITEM_VALUE, value);
}

/* One item, as many as items asks for. */
static inline int ctlv_test_series_get_chunk_item_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_SERIES_GET_CHUNK_ITEM);
}

/* One chunk, as many as chunks asks for. */
static inline int ctlv_test_series_get_chunk_start(struct ctlv_msg *msg)
{
	return ctlv_record_start(msg, CTLV_TEST_T_SERIES_GET_CHUNK);
}

/* One total, as many as totals asks for, numbered from one. */
static inline int ctlv_test_series_get_total_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_SERIES_GET_TOTAL, value);
}

/* How many events, one when absent. */
static inline int ctlv_test_ping_count_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_PING_COUNT, value);
}

/* Held against each event of the burst. */
static inline int ctlv_test_ping_serial_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_PING_SERIAL, value);
}

/* How many bytes of filler each event carries. */
static inline int ctlv_test_ping_payload_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_PING_PAYLOAD, value);
}

/* Build every event of the burst and throw it away instead of emitting it. */
static inline int ctlv_test_ping_abort_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_PING_ABORT, value);
}

/* Emit the event that carries nothing: a header and an empty root. */
static inline int ctlv_test_ping_empty_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_PING_EMPTY, value);
}

/* The request's serial plus the event's place in its burst, from zero. */
static inline int ctlv_test_beeped_serial_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_BEEPED_SERIAL, value);
}

/* As many bytes as the request's payload asks for, byte n being n % 256. */
static inline int ctlv_test_beeped_filler_put(struct ctlv_msg *msg, const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_TEST_T_BEEPED_FILLER, value, len);
}

/*
 * What is summed. It and the two blobs below take up to a quarter of a gibibyte each, more than an
 * inline message can hold.
 */
static inline int ctlv_test_transfer_source_put_va(struct ctlv_msg *msg, const void *address,
						   __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_T_TRANSFER_SOURCE, address, length);
}

static inline int ctlv_test_transfer_source_put_memfd(struct ctlv_msg *msg, int fd, __u64 offset,
						      __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_TRANSFER_SOURCE, fd, offset, length);
}

static inline int ctlv_test_transfer_source_put_dma_buf(struct ctlv_msg *msg, int fd, __u64 offset,
							__u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_T_TRANSFER_SOURCE, fd, offset, length);
}

/* What the pattern is written over. */
static inline int ctlv_test_transfer_target_put_va(struct ctlv_msg *msg, const void *address,
						   __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_T_TRANSFER_TARGET, address, length);
}

static inline int ctlv_test_transfer_target_put_memfd(struct ctlv_msg *msg, int fd, __u64 offset,
						      __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_TRANSFER_TARGET, fd, offset, length);
}

static inline int ctlv_test_transfer_target_put_dma_buf(struct ctlv_msg *msg, int fd, __u64 offset,
							__u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_T_TRANSFER_TARGET, fd, offset, length);
}

/* What is summed and then written over. */
static inline int ctlv_test_transfer_scratch_put_va(struct ctlv_msg *msg, const void *address,
						    __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_T_TRANSFER_SCRATCH, address, length);
}

static inline int ctlv_test_transfer_scratch_put_memfd(struct ctlv_msg *msg, int fd, __u64 offset,
						       __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_TRANSFER_SCRATCH, fd, offset, length);
}

static inline int ctlv_test_transfer_scratch_put_dma_buf(struct ctlv_msg *msg, int fd, __u64 offset,
							 __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_T_TRANSFER_SCRATCH, fd, offset, length);
}

/* A blob a request may carry more than one of, each its own acquisition. */
static inline int ctlv_test_transfer_piece_put_va(struct ctlv_msg *msg, const void *address,
						  __u64 length)
{
	return ctlv_put_blob_va(msg, CTLV_TEST_T_TRANSFER_PIECE, address, length);
}

static inline int ctlv_test_transfer_piece_put_memfd(struct ctlv_msg *msg, int fd, __u64 offset,
						     __u64 length)
{
	return ctlv_put_blob_memfd(msg, CTLV_TEST_T_TRANSFER_PIECE, fd, offset, length);
}

static inline int ctlv_test_transfer_piece_put_dma_buf(struct ctlv_msg *msg, int fd, __u64 offset,
						       __u64 length)
{
	return ctlv_put_blob_dma_buf(msg, CTLV_TEST_T_TRANSFER_PIECE, fd, offset, length);
}

/* What the pattern a write leaves behind is seeded from. */
static inline int ctlv_test_transfer_fill_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_TRANSFER_FILL, value);
}

/* Resolve nothing at all, the descriptors being valid and unused. */
static inline int ctlv_test_transfer_skip_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_TRANSFER_SKIP, value);
}

/* Resolve the source twice, two calls being two acquisitions. */
static inline int ctlv_test_transfer_twice_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_TRANSFER_TWICE, value);
}

/*
 * Hold everything resolved until the module is told to let go, so a test can close, unmap or reuse
 * what a descriptor named while the kernel has it.
 */
static inline int ctlv_test_transfer_hold_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_TRANSFER_HOLD, value);
}

/* What every readable byte added up to. */
static inline int ctlv_test_transfer_sum_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_TRANSFER_SUM, value);
}

/* How many bytes were written. */
static inline int ctlv_test_transfer_written_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_T_TRANSFER_WRITTEN, value);
}

/* Whether resolving one descriptor twice produced two objects. */
static inline int ctlv_test_transfer_distinct_put(struct ctlv_msg *msg, bool value)
{
	return ctlv_put_bool(msg, CTLV_TEST_T_TRANSFER_DISTINCT, value);
}

/* Reading a reply or an event. A getter answers whether it could read. */

/* The one required field, an integer bounded on both sides. */
static inline bool ctlv_test_echo_values_u_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_U);
}

static inline bool ctlv_test_echo_values_u_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_U);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* A signed integer, bounded on both sides. */
static inline bool ctlv_test_echo_values_s_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_S);
}

static inline bool ctlv_test_echo_values_s_get(const struct ctlv_hdr *record, __s64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_S);
	return hdr && !ctlv_get_s64(hdr, value);
}

/* A bool. */
static inline bool ctlv_test_echo_values_flag_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_FLAG);
}

static inline bool ctlv_test_echo_values_flag_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_FLAG);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* An enum. */
static inline bool ctlv_test_echo_values_mode_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_MODE);
}

static inline bool ctlv_test_echo_values_mode_get(const struct ctlv_hdr *record, __u64 *value,
						  bool *known)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_MODE);
	return hdr && !ctlv_get_enum(hdr, 1, 3, value, known);
}

/* A flags. */
static inline bool ctlv_test_echo_values_quirks_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_QUIRKS);
}

static inline bool ctlv_test_echo_values_quirks_get(const struct ctlv_hdr *record, __u64 *value,
						    __u64 *unknown)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_QUIRKS);
	return hdr && !ctlv_get_flags(hdr, CTLV_TEST_ECHO_QUIRK_MASK, value, unknown);
}

/* A string of 1 to 15 characters. */
static inline bool ctlv_test_echo_values_text_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_TEXT);
}

static inline bool ctlv_test_echo_values_text_get(const struct ctlv_hdr *record, const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_TEXT);
	return hdr && !ctlv_get_str(hdr, value);
}

/* A binary of 4 to 32 bytes. */
static inline bool ctlv_test_echo_values_data_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_DATA);
}

static inline bool ctlv_test_echo_values_data_get(const struct ctlv_hdr *record, const void **value,
						  __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_DATA);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* An integer that may occur three times. */
#define ctlv_test_echo_values_count_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_ECHO_VALUES_COUNT)

static inline bool ctlv_test_echo_values_count_get(const struct ctlv_hdr *hdr, __u64 *value)
{
	return hdr && !ctlv_get_u64(hdr, value);
}

/* A member the item requires. */
static inline bool ctlv_test_echo_values_item_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX);
}

static inline bool ctlv_test_echo_values_item_index_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* A member it may leave out. */
static inline bool ctlv_test_echo_values_item_label_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL);
}

static inline bool ctlv_test_echo_values_item_label_get(const struct ctlv_hdr *record,
							const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL);
	return hdr && !ctlv_get_str(hdr, value);
}

/* The leaf's one member. */
static inline bool ctlv_test_echo_values_leaf_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX);
}

static inline bool ctlv_test_echo_values_leaf_index_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The third level, where a record inside a record ends. */
static inline bool ctlv_test_echo_values_item_leaf_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF);
}

static inline const struct ctlv_hdr *
ctlv_test_echo_values_item_leaf_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF);
}

/* A record in place that may occur twice. */
#define ctlv_test_echo_values_item_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_ECHO_VALUES_ITEM)

/* Filled by ctlv-test-child, and empty on a ctlv-test device. */
static inline bool ctlv_test_echo_values_extension_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_EXTENSION);
}

static inline const struct ctlv_hdr *
ctlv_test_echo_values_extension_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_EXTENSION);
}

/* Valued by ctlv-test-child, and taking nothing on a ctlv-test device. */
static inline bool ctlv_test_echo_values_variant_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_VARIANT);
}

static inline bool ctlv_test_echo_values_variant_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_ECHO_VALUES_VARIANT);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* What probe-get answers with as it was sent. */
static inline bool ctlv_test_token_info_token_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TOKEN_INFO_TOKEN);
}

static inline bool ctlv_test_token_info_token_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TOKEN_INFO_TOKEN);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Filled by ctlv-test-child, so one selftest run sees both states. */
static inline bool ctlv_test_probe_get_extension_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PROBE_GET_EXTENSION);
}

static inline const struct ctlv_hdr *
ctlv_test_probe_get_extension_get(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PROBE_GET_EXTENSION);
}

/* How many chunks. */
static inline bool ctlv_test_series_get_chunks_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_CHUNKS);
}

static inline bool ctlv_test_series_get_chunks_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_CHUNKS);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many items in each chunk. */
static inline bool ctlv_test_series_get_items_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_ITEMS);
}

static inline bool ctlv_test_series_get_items_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_ITEMS);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many values in each item. */
static inline bool ctlv_test_series_get_values_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_VALUES);
}

static inline bool ctlv_test_series_get_values_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_VALUES);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many totals after the chunks. */
static inline bool ctlv_test_series_get_totals_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_TOTALS);
}

static inline bool ctlv_test_series_get_totals_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_TOTALS);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The chunk's number, from one. */
static inline bool ctlv_test_series_get_chunk_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_CHUNK_INDEX);
}

static inline bool ctlv_test_series_get_chunk_index_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_CHUNK_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* The item's number in its chunk, from one. */
static inline bool ctlv_test_series_get_item_index_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_ITEM_INDEX);
}

static inline bool ctlv_test_series_get_item_index_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_SERIES_GET_ITEM_INDEX);
	return hdr && !ctlv_get_u64(hdr, value);
}

/*
 * One value, as many as values asks for, each the chunk's number times 100, plus the item's times
 * 10, plus its own.
 */
#define ctlv_test_series_get_item_value_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_SERIES_GET_ITEM_VALUE)

static inline bool ctlv_test_series_get_item_value_get(const struct ctlv_hdr *hdr, __u64 *value)
{
	return hdr && !ctlv_get_u64(hdr, value);
}

/* One item, as many as items asks for. */
#define ctlv_test_series_get_chunk_item_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_SERIES_GET_CHUNK_ITEM)

/* One chunk, as many as chunks asks for. */
#define ctlv_test_series_get_chunk_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_SERIES_GET_CHUNK)

/* One total, as many as totals asks for, numbered from one. */
#define ctlv_test_series_get_total_for_each(_hdr, _record) \
	ctlv_for_each_type(_hdr, _record, CTLV_TEST_T_SERIES_GET_TOTAL)

static inline bool ctlv_test_series_get_total_get(const struct ctlv_hdr *hdr, __u64 *value)
{
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many events, one when absent. */
static inline bool ctlv_test_ping_count_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PING_COUNT);
}

static inline bool ctlv_test_ping_count_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_PING_COUNT);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Held against each event of the burst. */
static inline bool ctlv_test_ping_serial_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PING_SERIAL);
}

static inline bool ctlv_test_ping_serial_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_PING_SERIAL);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many bytes of filler each event carries. */
static inline bool ctlv_test_ping_payload_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PING_PAYLOAD);
}

static inline bool ctlv_test_ping_payload_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_PING_PAYLOAD);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Build every event of the burst and throw it away instead of emitting it. */
static inline bool ctlv_test_ping_abort_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PING_ABORT);
}

static inline bool ctlv_test_ping_abort_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_PING_ABORT);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* Emit the event that carries nothing: a header and an empty root. */
static inline bool ctlv_test_ping_empty_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_PING_EMPTY);
}

static inline bool ctlv_test_ping_empty_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_PING_EMPTY);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* The request's serial plus the event's place in its burst, from zero. */
static inline bool ctlv_test_beeped_serial_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_BEEPED_SERIAL);
}

static inline bool ctlv_test_beeped_serial_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_BEEPED_SERIAL);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* As many bytes as the request's payload asks for, byte n being n % 256. */
static inline bool ctlv_test_beeped_filler_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_BEEPED_FILLER);
}

static inline bool ctlv_test_beeped_filler_get(const struct ctlv_hdr *record, const void **value,
					       __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_BEEPED_FILLER);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* What the pattern a write leaves behind is seeded from. */
static inline bool ctlv_test_transfer_fill_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_FILL);
}

static inline bool ctlv_test_transfer_fill_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_FILL);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Resolve nothing at all, the descriptors being valid and unused. */
static inline bool ctlv_test_transfer_skip_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_SKIP);
}

static inline bool ctlv_test_transfer_skip_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_SKIP);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* Resolve the source twice, two calls being two acquisitions. */
static inline bool ctlv_test_transfer_twice_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_TWICE);
}

static inline bool ctlv_test_transfer_twice_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_TWICE);
	return hdr && !ctlv_get_bool(hdr, value);
}

/*
 * Hold everything resolved until the module is told to let go, so a test can close, unmap or reuse
 * what a descriptor named while the kernel has it.
 */
static inline bool ctlv_test_transfer_hold_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_HOLD);
}

static inline bool ctlv_test_transfer_hold_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_HOLD);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* What every readable byte added up to. */
static inline bool ctlv_test_transfer_sum_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_SUM);
}

static inline bool ctlv_test_transfer_sum_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_SUM);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* How many bytes were written. */
static inline bool ctlv_test_transfer_written_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_WRITTEN);
}

static inline bool ctlv_test_transfer_written_get(const struct ctlv_hdr *record, __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_WRITTEN);
	return hdr && !ctlv_get_u64(hdr, value);
}

/* Whether resolving one descriptor twice produced two objects. */
static inline bool ctlv_test_transfer_distinct_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_DISTINCT);
}

static inline bool ctlv_test_transfer_distinct_get(const struct ctlv_hdr *record, bool *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_T_TRANSFER_DISTINCT);
	return hdr && !ctlv_get_bool(hdr, value);
}

/* Watching for events. A subscription is per open file description. */

static inline bool ctlv_test_pinged_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_TEST_OP_PINGED;
}

static inline int ctlv_test_pinged_subscribe(struct ctlv_dev *dev, struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_PINGED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_test_pinged_unsubscribe(struct ctlv_dev *dev, struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_PINGED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

static inline bool ctlv_test_beeped_is(const struct ctlv_event *event)
{
	return ctlv_event_op_id(event) == CTLV_TEST_OP_BEEPED;
}

static inline int ctlv_test_beeped_subscribe(struct ctlv_dev *dev, struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_BEEPED;

	return ctlv_subscribe(dev, &op_id, 1, error);
}

static inline int ctlv_test_beeped_unsubscribe(struct ctlv_dev *dev, struct ctlv_error *error)
{
	const __u64 op_id = CTLV_TEST_OP_BEEPED;

	return ctlv_unsubscribe(dev, &op_id, 1, error);
}

#endif /* _CTLV_CTLV_TEST_USER_GEN_H */
