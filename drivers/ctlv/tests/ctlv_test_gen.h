/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-test CTLV family. */

#ifndef _LINUX_CTLV_TEST_GEN_H
#define _LINUX_CTLV_TEST_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_test_gen.h>

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_test_op_schemas[];

extern const struct ctlv_family_schema ctlv_test_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_test_noop_action(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_probe_get_query(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_dynamic_noop_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_dynamic_toggle_action(void *priv, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_echo_action(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_echo_reply_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_series_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply);
int ctlv_test_ping_action(void *priv, struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_transfer_action(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request, struct ctlv_cursor *reply);

/* Family-wide hooks around every effective non-core op. */
int ctlv_test_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_test_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Per-op hooks nested inside the family-wide pair. */
int ctlv_test_probe_get_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_test_probe_get_post(void *priv, struct ctlv_op_ctx *ctx);

int ctlv_test_dynamic_noop_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_test_dynamic_noop_post(void *priv, struct ctlv_op_ctx *ctx);

int ctlv_test_series_dump_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_test_series_dump_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_test_priv_size(void);
int ctlv_test_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
int ctlv_test_post_init(struct ctlv_device *ctlvdev, void *priv);
void ctlv_test_pre_fini(struct ctlv_device *ctlvdev, void *priv);
void ctlv_test_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_test_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Optional publication and dynamic availability helpers. */
CTLV_OP_PROVIDE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_ENABLE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_DISABLE(ctlv_test_dynamic_noop, CTLV_TEST_OP_DYNAMIC_NOOP)
CTLV_OP_PROVIDE(ctlv_test_dynamic_toggle, CTLV_TEST_OP_DYNAMIC_TOGGLE)

/* Reading a validated message. A blob has a resolver instead of a getter. */
CTLV_IN_REF(ctlv_test_probe_get_probe_token, &ctlv_test_op_schemas[1].request,
	    CTLV_TEST_ATTR_PROBE_TOKEN, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_probe_get_probe_token)
CTLV_IN_U64(ctlv_test_probe_get_probe_token)
CTLV_IN_REF(ctlv_test_dynamic_toggle_probe_token, &ctlv_test_op_schemas[3].request,
	    CTLV_TEST_ATTR_PROBE_TOKEN, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_dynamic_toggle_probe_token)
CTLV_IN_U64(ctlv_test_dynamic_toggle_probe_token)
CTLV_IN_REF(ctlv_test_echo_u, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_U, 0, 0,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_u)
CTLV_IN_U64(ctlv_test_echo_u)
CTLV_IN_REF(ctlv_test_echo_s, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_S, 0, 1,
	    CTLV_SCHEMA_SINT)
CTLV_IN_HAS(ctlv_test_echo_s)
CTLV_IN_S64(ctlv_test_echo_s)
CTLV_IN_REF(ctlv_test_echo_flag, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_FLAG, 0, 2,
	    CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_echo_flag)
CTLV_IN_BOOL(ctlv_test_echo_flag)
CTLV_IN_REF(ctlv_test_echo_mode, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_MODE, 0, 3,
	    CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_test_echo_mode)
CTLV_IN_U64(ctlv_test_echo_mode)
CTLV_IN_REF(ctlv_test_echo_quirks, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_QUIRKS, 0,
	    4, CTLV_SCHEMA_FLAGS)
CTLV_IN_HAS(ctlv_test_echo_quirks)
CTLV_IN_U64(ctlv_test_echo_quirks)
CTLV_IN_REF(ctlv_test_echo_text, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_TEXT, 0, 5,
	    CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_text)
CTLV_IN_STRING(ctlv_test_echo_text)
CTLV_IN_REF(ctlv_test_echo_data, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_DATA, 0, 6,
	    CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_test_echo_data)
CTLV_IN_BINARY(ctlv_test_echo_data)
CTLV_IN_REF(ctlv_test_echo_count, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_COUNT, 0, 7,
	    CTLV_SCHEMA_UINT)
#define ctlv_test_echo_count_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_count, _attr, _nest)
CTLV_IN_EACH_U64(ctlv_test_echo_count)
CTLV_IN_REF(ctlv_test_echo_item, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_ITEM, 0, 8,
	    CTLV_SCHEMA_NEST)
#define ctlv_test_echo_item_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_item, _attr, _nest)
CTLV_IN_REF(ctlv_test_echo_region, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_REGION, 0,
	    9, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_region)
CTLV_IN_BLOB(ctlv_test_echo_region)
CTLV_IN_REF(ctlv_test_echo_page, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_PAGE, 0, 10,
	    CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_page)
CTLV_IN_BLOB(ctlv_test_echo_page)
CTLV_IN_REF(ctlv_test_echo_probe_extension, &ctlv_test_op_schemas[5].request,
	    CTLV_TEST_ATTR_PROBE_EXTENSION, 0, 11, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_test_echo_probe_extension)
CTLV_IN_NEST(ctlv_test_echo_probe_extension)
CTLV_IN_REF(ctlv_test_echo_item_index, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ITEM_INDEX,
	    9, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_item_index)
CTLV_IN_U64(ctlv_test_echo_item_index)
CTLV_IN_REF(ctlv_test_echo_item_label, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ITEM_LABEL,
	    9, 1, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_item_label)
CTLV_IN_STRING(ctlv_test_echo_item_label)
CTLV_IN_REF(ctlv_test_echo_item_leaf, &ctlv_test_op_schemas[5].request, CTLV_TEST_ATTR_ITEM_LEAF, 9,
	    2, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_test_echo_item_leaf)
CTLV_IN_NEST(ctlv_test_echo_item_leaf)
CTLV_IN_REF(ctlv_test_echo_item_leaf_index, &ctlv_test_op_schemas[5].request,
	    CTLV_TEST_ATTR_ITEM_INDEX, 15, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_item_leaf_index)
CTLV_IN_U64(ctlv_test_echo_item_leaf_index)
CTLV_IN_REF(ctlv_test_echo_reply_echo_u, &ctlv_test_op_schemas[6].request, CTLV_TEST_ATTR_ECHO_U, 0,
	    0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_u)
CTLV_IN_U64(ctlv_test_echo_reply_echo_u)
CTLV_IN_REF(ctlv_test_echo_reply_echo_s, &ctlv_test_op_schemas[6].request, CTLV_TEST_ATTR_ECHO_S, 0,
	    1, CTLV_SCHEMA_SINT)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_s)
CTLV_IN_S64(ctlv_test_echo_reply_echo_s)
CTLV_IN_REF(ctlv_test_echo_reply_echo_flag, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_FLAG, 0, 2, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_flag)
CTLV_IN_BOOL(ctlv_test_echo_reply_echo_flag)
CTLV_IN_REF(ctlv_test_echo_reply_echo_mode, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_MODE, 0, 3, CTLV_SCHEMA_ENUM)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_mode)
CTLV_IN_U64(ctlv_test_echo_reply_echo_mode)
CTLV_IN_REF(ctlv_test_echo_reply_echo_quirks, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_QUIRKS, 0, 4, CTLV_SCHEMA_FLAGS)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_quirks)
CTLV_IN_U64(ctlv_test_echo_reply_echo_quirks)
CTLV_IN_REF(ctlv_test_echo_reply_echo_text, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_TEXT, 0, 5, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_text)
CTLV_IN_STRING(ctlv_test_echo_reply_echo_text)
CTLV_IN_REF(ctlv_test_echo_reply_echo_data, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_DATA, 0, 6, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_data)
CTLV_IN_BINARY(ctlv_test_echo_reply_echo_data)
CTLV_IN_REF(ctlv_test_echo_reply_echo_count, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_COUNT, 0, 7, CTLV_SCHEMA_UINT)
#define ctlv_test_echo_reply_echo_count_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_reply_echo_count, _attr, _nest)
CTLV_IN_EACH_U64(ctlv_test_echo_reply_echo_count)
CTLV_IN_REF(ctlv_test_echo_reply_echo_item, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_ITEM, 0, 8, CTLV_SCHEMA_NEST)
#define ctlv_test_echo_reply_echo_item_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_test_echo_reply_echo_item, _attr, _nest)
CTLV_IN_REF(ctlv_test_echo_reply_echo_region, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_REGION, 0, 9, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_region)
CTLV_IN_BLOB(ctlv_test_echo_reply_echo_region)
CTLV_IN_REF(ctlv_test_echo_reply_echo_page, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ECHO_PAGE, 0, 10, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_echo_reply_echo_page)
CTLV_IN_BLOB(ctlv_test_echo_reply_echo_page)
CTLV_IN_REF(ctlv_test_echo_reply_probe_extension, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_PROBE_EXTENSION, 0, 11, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_test_echo_reply_probe_extension)
CTLV_IN_NEST(ctlv_test_echo_reply_probe_extension)
CTLV_IN_REF(ctlv_test_echo_reply_item_index, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ITEM_INDEX, 9, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_reply_item_index)
CTLV_IN_U64(ctlv_test_echo_reply_item_index)
CTLV_IN_REF(ctlv_test_echo_reply_item_label, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ITEM_LABEL, 9, 1, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_test_echo_reply_item_label)
CTLV_IN_STRING(ctlv_test_echo_reply_item_label)
CTLV_IN_REF(ctlv_test_echo_reply_item_leaf, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ITEM_LEAF, 9, 2, CTLV_SCHEMA_NEST)
CTLV_IN_HAS(ctlv_test_echo_reply_item_leaf)
CTLV_IN_NEST(ctlv_test_echo_reply_item_leaf)
CTLV_IN_REF(ctlv_test_echo_reply_item_leaf_index, &ctlv_test_op_schemas[6].request,
	    CTLV_TEST_ATTR_ITEM_INDEX, 15, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_echo_reply_item_leaf_index)
CTLV_IN_U64(ctlv_test_echo_reply_item_leaf_index)
CTLV_IN_REF(ctlv_test_series_dump_series_chunks, &ctlv_test_op_schemas[7].request,
	    CTLV_TEST_ATTR_SERIES_CHUNKS, 0, 0, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_dump_series_chunks)
CTLV_IN_U64(ctlv_test_series_dump_series_chunks)
CTLV_IN_REF(ctlv_test_series_dump_series_items, &ctlv_test_op_schemas[7].request,
	    CTLV_TEST_ATTR_SERIES_ITEMS, 0, 1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_dump_series_items)
CTLV_IN_U64(ctlv_test_series_dump_series_items)
CTLV_IN_REF(ctlv_test_series_dump_series_values, &ctlv_test_op_schemas[7].request,
	    CTLV_TEST_ATTR_SERIES_VALUES, 0, 2, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_dump_series_values)
CTLV_IN_U64(ctlv_test_series_dump_series_values)
CTLV_IN_REF(ctlv_test_series_dump_series_totals, &ctlv_test_op_schemas[7].request,
	    CTLV_TEST_ATTR_SERIES_TOTALS, 0, 3, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_series_dump_series_totals)
CTLV_IN_U64(ctlv_test_series_dump_series_totals)
CTLV_IN_REF(ctlv_test_ping_count, &ctlv_test_op_schemas[8].request, CTLV_TEST_ATTR_PING_COUNT, 0, 0,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_count)
CTLV_IN_U64(ctlv_test_ping_count)
CTLV_IN_REF(ctlv_test_ping_serial, &ctlv_test_op_schemas[8].request, CTLV_TEST_ATTR_PING_SERIAL, 0,
	    1, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_serial)
CTLV_IN_U64(ctlv_test_ping_serial)
CTLV_IN_REF(ctlv_test_ping_payload, &ctlv_test_op_schemas[8].request, CTLV_TEST_ATTR_PING_PAYLOAD,
	    0, 2, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_ping_payload)
CTLV_IN_U64(ctlv_test_ping_payload)
CTLV_IN_REF(ctlv_test_ping_abort, &ctlv_test_op_schemas[8].request, CTLV_TEST_ATTR_PING_ABORT, 0, 3,
	    CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_ping_abort)
CTLV_IN_BOOL(ctlv_test_ping_abort)
CTLV_IN_REF(ctlv_test_ping_empty, &ctlv_test_op_schemas[8].request, CTLV_TEST_ATTR_PING_EMPTY, 0, 4,
	    CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_ping_empty)
CTLV_IN_BOOL(ctlv_test_ping_empty)
CTLV_IN_REF(ctlv_test_transfer_source, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_SOURCE, 0, 0, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_source)
CTLV_IN_BLOB(ctlv_test_transfer_source)
CTLV_IN_REF(ctlv_test_transfer_target, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_TARGET, 0, 1, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_target)
CTLV_IN_BLOB(ctlv_test_transfer_target)
CTLV_IN_REF(ctlv_test_transfer_scratch, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_SCRATCH, 0, 2, CTLV_SCHEMA_BLOB)
CTLV_IN_HAS(ctlv_test_transfer_scratch)
CTLV_IN_BLOB(ctlv_test_transfer_scratch)
CTLV_IN_REF(ctlv_test_transfer_piece, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_PIECE, 0, 3, CTLV_SCHEMA_BLOB)
#define ctlv_test_transfer_piece_for_each(_attr, _nest) \
	CTLV_IN_FOR_EACH(ctlv_test_transfer_piece, _attr, _nest)
CTLV_IN_EACH_BLOB(ctlv_test_transfer_piece)
CTLV_IN_REF(ctlv_test_transfer_fill, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_FILL, 0, 4, CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_transfer_fill)
CTLV_IN_U64(ctlv_test_transfer_fill)
CTLV_IN_REF(ctlv_test_transfer_skip, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_SKIP, 0, 5, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_skip)
CTLV_IN_BOOL(ctlv_test_transfer_skip)
CTLV_IN_REF(ctlv_test_transfer_twice, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_TWICE, 0, 6, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_twice)
CTLV_IN_BOOL(ctlv_test_transfer_twice)
CTLV_IN_REF(ctlv_test_transfer_hold, &ctlv_test_op_schemas[10].request,
	    CTLV_TEST_ATTR_TRANSFER_HOLD, 0, 7, CTLV_SCHEMA_BOOL)
CTLV_IN_HAS(ctlv_test_transfer_hold)
CTLV_IN_BOOL(ctlv_test_transfer_hold)

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */
CTLV_PUT_U64(ctlv_test_probe_token, CTLV_TEST_ATTR_PROBE_TOKEN, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_test_probe_extension, CTLV_TEST_ATTR_PROBE_EXTENSION, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_test_echo_u, CTLV_TEST_ATTR_ECHO_U, CTLV_SCHEMA_UINT)
CTLV_PUT_S64(ctlv_test_echo_s, CTLV_TEST_ATTR_ECHO_S, CTLV_SCHEMA_SINT)
CTLV_PUT_BOOL(ctlv_test_echo_flag, CTLV_TEST_ATTR_ECHO_FLAG, CTLV_SCHEMA_BOOL)
CTLV_PUT_U64(ctlv_test_echo_mode, CTLV_TEST_ATTR_ECHO_MODE, CTLV_SCHEMA_ENUM)
CTLV_PUT_U64(ctlv_test_echo_quirks, CTLV_TEST_ATTR_ECHO_QUIRKS, CTLV_SCHEMA_FLAGS)
CTLV_PUT_STRING(ctlv_test_echo_text, CTLV_TEST_ATTR_ECHO_TEXT, CTLV_SCHEMA_STRING)
CTLV_PUT_BINARY(ctlv_test_echo_data, CTLV_TEST_ATTR_ECHO_DATA, CTLV_SCHEMA_BINARY)
CTLV_PUT_U64(ctlv_test_echo_count, CTLV_TEST_ATTR_ECHO_COUNT, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_test_echo_item, CTLV_TEST_ATTR_ECHO_ITEM, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_test_item_index, CTLV_TEST_ATTR_ITEM_INDEX, CTLV_SCHEMA_UINT)
CTLV_PUT_STRING(ctlv_test_item_label, CTLV_TEST_ATTR_ITEM_LABEL, CTLV_SCHEMA_STRING)
CTLV_NEST_START(ctlv_test_item_leaf, CTLV_TEST_ATTR_ITEM_LEAF, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_test_series_chunks, CTLV_TEST_ATTR_SERIES_CHUNKS, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_series_items, CTLV_TEST_ATTR_SERIES_ITEMS, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_series_values, CTLV_TEST_ATTR_SERIES_VALUES, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_series_totals, CTLV_TEST_ATTR_SERIES_TOTALS, CTLV_SCHEMA_UINT)
CTLV_NEST_START(ctlv_test_series_chunk, CTLV_TEST_ATTR_SERIES_CHUNK, CTLV_SCHEMA_NEST)
CTLV_NEST_START(ctlv_test_series_item, CTLV_TEST_ATTR_SERIES_ITEM, CTLV_SCHEMA_NEST)
CTLV_PUT_U64(ctlv_test_series_value, CTLV_TEST_ATTR_SERIES_VALUE, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_series_total, CTLV_TEST_ATTR_SERIES_TOTAL, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_series_index, CTLV_TEST_ATTR_SERIES_INDEX, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_ping_count, CTLV_TEST_ATTR_PING_COUNT, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_ping_serial, CTLV_TEST_ATTR_PING_SERIAL, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_ping_payload, CTLV_TEST_ATTR_PING_PAYLOAD, CTLV_SCHEMA_UINT)
CTLV_PUT_BOOL(ctlv_test_ping_abort, CTLV_TEST_ATTR_PING_ABORT, CTLV_SCHEMA_BOOL)
CTLV_PUT_BOOL(ctlv_test_ping_empty, CTLV_TEST_ATTR_PING_EMPTY, CTLV_SCHEMA_BOOL)
CTLV_PUT_BINARY(ctlv_test_ping_filler, CTLV_TEST_ATTR_PING_FILLER, CTLV_SCHEMA_BINARY)
CTLV_PUT_U64(ctlv_test_transfer_fill, CTLV_TEST_ATTR_TRANSFER_FILL, CTLV_SCHEMA_UINT)
CTLV_PUT_BOOL(ctlv_test_transfer_skip, CTLV_TEST_ATTR_TRANSFER_SKIP, CTLV_SCHEMA_BOOL)
CTLV_PUT_BOOL(ctlv_test_transfer_twice, CTLV_TEST_ATTR_TRANSFER_TWICE, CTLV_SCHEMA_BOOL)
CTLV_PUT_BOOL(ctlv_test_transfer_hold, CTLV_TEST_ATTR_TRANSFER_HOLD, CTLV_SCHEMA_BOOL)
CTLV_PUT_U64(ctlv_test_transfer_sum, CTLV_TEST_ATTR_TRANSFER_SUM, CTLV_SCHEMA_UINT)
CTLV_PUT_U64(ctlv_test_transfer_written, CTLV_TEST_ATTR_TRANSFER_WRITTEN, CTLV_SCHEMA_UINT)
CTLV_PUT_BOOL(ctlv_test_transfer_distinct, CTLV_TEST_ATTR_TRANSFER_DISTINCT, CTLV_SCHEMA_BOOL)

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */
CTLV_EVENT_CREATE(ctlv_test_pinged, CTLV_TEST_OP_PINGED)
CTLV_EVENT_CREATE(ctlv_test_beeped, CTLV_TEST_OP_BEEPED)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_TEST_GEN_H */
