/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-test CTLV family. */

#ifndef _LINUX_CTLV_TEST_GEN_H
#define _LINUX_CTLV_TEST_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/ctlv/ctlv_test_gen.h>

/* Extension points, as a descendant's fill names one. */
#define CTLV_TEST_EXT_EXTENSION_POINT 1

/* Enum extension points, numbered apart from the others. */
#define CTLV_TEST_ENUM_EXT_ECHO_VARIANT 1

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_test_op_schemas[];

extern const struct ctlv_family_schema ctlv_test_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_test_noop_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_probe_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_dynamic_noop_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_dynamic_toggle_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_echo_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_echo_reply_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_series_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_ping_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_transfer_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request, struct ctlv_cursor *reply);

/* Per-op hooks inside the device pair. */
int ctlv_test_probe_get_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
void ctlv_test_probe_get_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);

int ctlv_test_dynamic_noop_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
void ctlv_test_dynamic_noop_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);

int ctlv_test_series_get_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
void ctlv_test_series_get_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);

extern const struct ctlv_family ctlv_test_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Optional publication and dynamic availability helpers. */
CTLV_OP_PROVIDE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_ENABLE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_DISABLE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_PROVIDE(ctlv_test_dynamic_toggle, CTLV_TEST_OP_DYNAMIC_TOGGLE)

/* Reading a validated message. A blob has a resolver instead of a getter. */
/* What probe-get answers with as it was sent. */
CTLV_IN_REF(ctlv_test_token_info_token, CTLV_TEST_T_TOKEN_INFO_TOKEN, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_token_info_token)
CTLV_IN_U64(ctlv_test_token_info_token)
/* The one required field, an integer bounded on both sides. */
CTLV_IN_REF(ctlv_test_echo_values_u, CTLV_TEST_T_ECHO_VALUES_U, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_values_u)
CTLV_IN_U64(ctlv_test_echo_values_u)
/* A signed integer, bounded on both sides. */
CTLV_IN_REF(ctlv_test_echo_values_s, CTLV_TEST_T_ECHO_VALUES_S, CTLV_SCHEMA_SINT)
CTLV_IN_HAS(ctlv_test_echo_values_s)
CTLV_IN_S64(ctlv_test_echo_values_s)
/* A bool. */
CTLV_IN_REF(ctlv_test_echo_values_flag, CTLV_TEST_T_ECHO_VALUES_FLAG, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_echo_values_flag)
CTLV_IN_BOOL(ctlv_test_echo_values_flag)
/* An enum. */
CTLV_IN_REF(ctlv_test_echo_values_mode, CTLV_TEST_T_ECHO_VALUES_MODE, CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_test_echo_values_mode)
CTLV_IN_U64(ctlv_test_echo_values_mode)
/* A flags. */
CTLV_IN_REF(ctlv_test_echo_values_quirks, CTLV_TEST_T_ECHO_VALUES_QUIRKS, CTLV_SCHEMA_FLAGS)
CTLV_IN_HAS(ctlv_test_echo_values_quirks)
CTLV_IN_U64(ctlv_test_echo_values_quirks)
/* A string of 1 to 15 characters. */
CTLV_IN_REF(ctlv_test_echo_values_text, CTLV_TEST_T_ECHO_VALUES_TEXT, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_values_text)
CTLV_IN_STRING(ctlv_test_echo_values_text)
/* A binary of 4 to 32 bytes. */
CTLV_IN_REF(ctlv_test_echo_values_data, CTLV_TEST_T_ECHO_VALUES_DATA, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_test_echo_values_data)
CTLV_IN_BINARY(ctlv_test_echo_values_data)
/* An integer that may occur three times. */
CTLV_IN_REF(ctlv_test_echo_values_count, CTLV_TEST_T_ECHO_VALUES_COUNT, CTLV_SCHEMA_UINT)
#define ctlv_test_echo_values_count_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_values_count, _cursor, _record)
CTLV_IN_EACH_U64(ctlv_test_echo_values_count)
/* A record in place that may occur twice. */
CTLV_IN_REF(ctlv_test_echo_values_item, CTLV_TEST_T_ECHO_VALUES_ITEM, CTLV_SCHEMA_RECORD)
#define ctlv_test_echo_values_item_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_values_item, _cursor, _record)
/* A member the item requires. */
CTLV_IN_REF(ctlv_test_echo_values_item_index, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_values_item_index)
CTLV_IN_U64(ctlv_test_echo_values_item_index)
/* A member it may leave out. */
CTLV_IN_REF(ctlv_test_echo_values_item_label, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL,
	    CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_values_item_label)
CTLV_IN_STRING(ctlv_test_echo_values_item_label)
/* The third level, where a record inside a record ends. */
CTLV_IN_REF(ctlv_test_echo_values_item_leaf, CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF, CTLV_SCHEMA_RECORD)
CTLV_IN_HAS(ctlv_test_echo_values_item_leaf)
CTLV_IN_RECORD(ctlv_test_echo_values_item_leaf)
/* The leaf's one member. */
CTLV_IN_REF(ctlv_test_echo_values_leaf_index, CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_values_leaf_index)
CTLV_IN_U64(ctlv_test_echo_values_leaf_index)
/* Filled by ctlv-test-child, and empty on a ctlv-test device. */
CTLV_IN_REF(ctlv_test_echo_values_extension, CTLV_TEST_T_ECHO_VALUES_EXTENSION, CTLV_SCHEMA_RECORD)
CTLV_IN_HAS(ctlv_test_echo_values_extension)
CTLV_IN_RECORD(ctlv_test_echo_values_extension)
/* Valued by ctlv-test-child, and taking nothing on a ctlv-test device. */
CTLV_IN_REF(ctlv_test_echo_values_variant, CTLV_TEST_T_ECHO_VALUES_VARIANT, CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_test_echo_values_variant)
CTLV_IN_U64(ctlv_test_echo_values_variant)
/* Any backing at eight bytes. */
CTLV_IN_REF(ctlv_test_echo_message_region, CTLV_TEST_T_ECHO_MESSAGE_REGION, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_message_region)
CTLV_IN_BLOB(ctlv_test_echo_message_region)
/* A memfd at a page. */
CTLV_IN_REF(ctlv_test_echo_message_page, CTLV_TEST_T_ECHO_MESSAGE_PAGE, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_message_page)
CTLV_IN_BLOB(ctlv_test_echo_message_page)
/* How many chunks. */
CTLV_IN_REF(ctlv_test_series_get_chunks, CTLV_TEST_T_SERIES_GET_CHUNKS, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_get_chunks)
CTLV_IN_U64(ctlv_test_series_get_chunks)
/* How many items in each chunk. */
CTLV_IN_REF(ctlv_test_series_get_items, CTLV_TEST_T_SERIES_GET_ITEMS, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_get_items)
CTLV_IN_U64(ctlv_test_series_get_items)
/* How many values in each item. */
CTLV_IN_REF(ctlv_test_series_get_values, CTLV_TEST_T_SERIES_GET_VALUES, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_get_values)
CTLV_IN_U64(ctlv_test_series_get_values)
/* How many totals after the chunks. */
CTLV_IN_REF(ctlv_test_series_get_totals, CTLV_TEST_T_SERIES_GET_TOTALS, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_get_totals)
CTLV_IN_U64(ctlv_test_series_get_totals)
/* How many events, one when absent. */
CTLV_IN_REF(ctlv_test_ping_count, CTLV_TEST_T_PING_COUNT, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_count)
CTLV_IN_U64(ctlv_test_ping_count)
/* Held against each event of the burst. */
CTLV_IN_REF(ctlv_test_ping_serial, CTLV_TEST_T_PING_SERIAL, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_serial)
CTLV_IN_U64(ctlv_test_ping_serial)
/* How many bytes of filler each event carries. */
CTLV_IN_REF(ctlv_test_ping_payload, CTLV_TEST_T_PING_PAYLOAD, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_payload)
CTLV_IN_U64(ctlv_test_ping_payload)
/* Build every event of the burst and throw it away instead of emitting it. */
CTLV_IN_REF(ctlv_test_ping_abort, CTLV_TEST_T_PING_ABORT, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_ping_abort)
CTLV_IN_BOOL(ctlv_test_ping_abort)
/* Emit the event that carries nothing: a header and an empty root. */
CTLV_IN_REF(ctlv_test_ping_empty, CTLV_TEST_T_PING_EMPTY, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_ping_empty)
CTLV_IN_BOOL(ctlv_test_ping_empty)
/*
 * What is summed. It and the two blobs below take up to a quarter of a gibibyte each, more than an
 * inline message can hold.
 */
CTLV_IN_REF(ctlv_test_transfer_source, CTLV_TEST_T_TRANSFER_SOURCE, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_source)
CTLV_IN_BLOB(ctlv_test_transfer_source)
/* What the pattern is written over. */
CTLV_IN_REF(ctlv_test_transfer_target, CTLV_TEST_T_TRANSFER_TARGET, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_target)
CTLV_IN_BLOB(ctlv_test_transfer_target)
/* What is summed and then written over. */
CTLV_IN_REF(ctlv_test_transfer_scratch, CTLV_TEST_T_TRANSFER_SCRATCH, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_scratch)
CTLV_IN_BLOB(ctlv_test_transfer_scratch)
/* A blob a request may carry more than one of, each its own acquisition. */
CTLV_IN_REF(ctlv_test_transfer_piece, CTLV_TEST_T_TRANSFER_PIECE, CTLV_SCHEMA_BLOB)
#define ctlv_test_transfer_piece_for_each(_cursor, _record) \
	CTLV_IN_FOR_EACH(ctlv_test_transfer_piece, _cursor, _record)
CTLV_IN_EACH_BLOB(ctlv_test_transfer_piece)
/* What the pattern a write leaves behind is seeded from. */
CTLV_IN_REF(ctlv_test_transfer_fill, CTLV_TEST_T_TRANSFER_FILL, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_transfer_fill)
CTLV_IN_U64(ctlv_test_transfer_fill)
/* Resolve nothing at all, the descriptors being valid and unused. */
CTLV_IN_REF(ctlv_test_transfer_skip, CTLV_TEST_T_TRANSFER_SKIP, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_skip)
CTLV_IN_BOOL(ctlv_test_transfer_skip)
/* Resolve the source twice, two calls being two acquisitions. */
CTLV_IN_REF(ctlv_test_transfer_twice, CTLV_TEST_T_TRANSFER_TWICE, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_twice)
CTLV_IN_BOOL(ctlv_test_transfer_twice)
/*
 * Hold everything resolved until the module is told to let go, so a test can close, unmap or reuse
 * what a descriptor named while the kernel has it.
 */
CTLV_IN_REF(ctlv_test_transfer_hold, CTLV_TEST_T_TRANSFER_HOLD, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_hold)
CTLV_IN_BOOL(ctlv_test_transfer_hold)

/* Writing a reply. An append is atomic; a record ends with ctlv_record_end(). */
/* What probe-get answers with as it was sent. */
CTLV_PUT_U64(ctlv_test_token_info_token, CTLV_TEST_T_TOKEN_INFO_TOKEN, CTLV_SCHEMA_UINT)
/* Filled by ctlv-test-child, so one selftest run sees both states. */
CTLV_RECORD_START(ctlv_test_probe_get_extension, CTLV_TEST_T_PROBE_GET_EXTENSION,
		  CTLV_SCHEMA_RECORD)
/* The one required field, an integer bounded on both sides. */
CTLV_PUT_U64(ctlv_test_echo_values_u, CTLV_TEST_T_ECHO_VALUES_U, CTLV_SCHEMA_UINT)
/* A signed integer, bounded on both sides. */
CTLV_PUT_S64(ctlv_test_echo_values_s, CTLV_TEST_T_ECHO_VALUES_S, CTLV_SCHEMA_SINT)
/* A bool. */
CTLV_PUT_BOOL(ctlv_test_echo_values_flag, CTLV_TEST_T_ECHO_VALUES_FLAG, CTLV_SCHEMA_BOOL)
/* An enum. */
CTLV_PUT_U64(ctlv_test_echo_values_mode, CTLV_TEST_T_ECHO_VALUES_MODE, CTLV_SCHEMA_ENUM)
/* A flags. */
CTLV_PUT_U64(ctlv_test_echo_values_quirks, CTLV_TEST_T_ECHO_VALUES_QUIRKS, CTLV_SCHEMA_FLAGS)
/* A string of 1 to 15 characters. */
CTLV_PUT_STRING(ctlv_test_echo_values_text, CTLV_TEST_T_ECHO_VALUES_TEXT, CTLV_SCHEMA_STRING)
/* A binary of 4 to 32 bytes. */
CTLV_PUT_BINARY(ctlv_test_echo_values_data, CTLV_TEST_T_ECHO_VALUES_DATA, CTLV_SCHEMA_BINARY)
/* An integer that may occur three times. */
CTLV_PUT_U64(ctlv_test_echo_values_count, CTLV_TEST_T_ECHO_VALUES_COUNT, CTLV_SCHEMA_UINT)
/* A record in place that may occur twice. */
CTLV_RECORD_START(ctlv_test_echo_values_item, CTLV_TEST_T_ECHO_VALUES_ITEM, CTLV_SCHEMA_RECORD)
/* A member the item requires. */
CTLV_PUT_U64(ctlv_test_echo_values_item_index, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX, CTLV_SCHEMA_UINT)
/* A member it may leave out. */
CTLV_PUT_STRING(ctlv_test_echo_values_item_label, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL,
		CTLV_SCHEMA_STRING)
/* The third level, where a record inside a record ends. */
CTLV_RECORD_START(ctlv_test_echo_values_item_leaf, CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF,
		  CTLV_SCHEMA_RECORD)
/* The leaf's one member. */
CTLV_PUT_U64(ctlv_test_echo_values_leaf_index, CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX, CTLV_SCHEMA_UINT)
/* Filled by ctlv-test-child, and empty on a ctlv-test device. */
CTLV_RECORD_START(ctlv_test_echo_values_extension, CTLV_TEST_T_ECHO_VALUES_EXTENSION,
		  CTLV_SCHEMA_RECORD)
/* Valued by ctlv-test-child, and taking nothing on a ctlv-test device. */
CTLV_PUT_U64(ctlv_test_echo_values_variant, CTLV_TEST_T_ECHO_VALUES_VARIANT, CTLV_SCHEMA_ENUM)
/* One chunk, as many as chunks asks for. */
CTLV_RECORD_START(ctlv_test_series_get_chunk, CTLV_TEST_T_SERIES_GET_CHUNK, CTLV_SCHEMA_RECORD)
/* The chunk's number, from one. */
CTLV_PUT_U64(ctlv_test_series_get_chunk_index, CTLV_TEST_T_SERIES_GET_CHUNK_INDEX, CTLV_SCHEMA_UINT)
/* One item, as many as items asks for. */
CTLV_RECORD_START(ctlv_test_series_get_chunk_item, CTLV_TEST_T_SERIES_GET_CHUNK_ITEM,
		  CTLV_SCHEMA_RECORD)
/* The item's number in its chunk, from one. */
CTLV_PUT_U64(ctlv_test_series_get_item_index, CTLV_TEST_T_SERIES_GET_ITEM_INDEX, CTLV_SCHEMA_UINT)
/*
 * One value, as many as values asks for, each the chunk's number times 100, plus the item's times
 * 10, plus its own.
 */
CTLV_PUT_U64(ctlv_test_series_get_item_value, CTLV_TEST_T_SERIES_GET_ITEM_VALUE, CTLV_SCHEMA_UINT)
/* One total, as many as totals asks for, numbered from one. */
CTLV_PUT_U64(ctlv_test_series_get_total, CTLV_TEST_T_SERIES_GET_TOTAL, CTLV_SCHEMA_UINT)
/* The request's serial plus the event's place in its burst, from zero. */
CTLV_PUT_U64(ctlv_test_beeped_serial, CTLV_TEST_T_BEEPED_SERIAL, CTLV_SCHEMA_UINT)
/* As many bytes as the request's payload asks for, byte n being n % 256. */
CTLV_PUT_BINARY(ctlv_test_beeped_filler, CTLV_TEST_T_BEEPED_FILLER, CTLV_SCHEMA_BINARY)
/* What every readable byte added up to. */
CTLV_PUT_U64(ctlv_test_transfer_sum, CTLV_TEST_T_TRANSFER_SUM, CTLV_SCHEMA_UINT)
/* How many bytes were written. */
CTLV_PUT_U64(ctlv_test_transfer_written, CTLV_TEST_T_TRANSFER_WRITTEN, CTLV_SCHEMA_UINT)
/* Whether resolving one descriptor twice produced two objects. */
CTLV_PUT_BOOL(ctlv_test_transfer_distinct, CTLV_TEST_T_TRANSFER_DISTINCT, CTLV_SCHEMA_BOOL)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_test_pinged, CTLV_TEST_OP_PINGED)
CTLV_EVENT_CREATE(ctlv_test_beeped, CTLV_TEST_OP_BEEPED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_TEST_GEN_H */
