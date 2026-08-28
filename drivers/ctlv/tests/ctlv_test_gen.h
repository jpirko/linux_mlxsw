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
extern const struct ctlv_op_schema ctlv_test_ctlv_op_schemas[];

extern const struct ctlv_family_schema ctlv_test_ctlv_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_test_noop_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_probe_get_query(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);
int ctlv_test_dynamic_noop_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request,
				  struct ctlv_cursor *reply);
int ctlv_test_dynamic_toggle_action(void *priv, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request,
				    struct ctlv_cursor *reply);
int ctlv_test_echo_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_echo_reply_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply);
int ctlv_test_series_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply);
int ctlv_test_ping_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply);
int ctlv_test_transfer_action(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);

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
int ctlv_test_init(struct ctlv_device *ctlvdev, const void *init_info,
		   void *priv);
int ctlv_test_post_init(struct ctlv_device *ctlvdev, void *priv);
void ctlv_test_pre_fini(struct ctlv_device *ctlvdev, void *priv);
void ctlv_test_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_test_ctlv_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Optional publication and dynamic availability helpers. */
static inline void ctlv_test_dynamic_noop_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000003ULL);
}

static inline void ctlv_test_dynamic_noop_enable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x0003000000000003ULL, false);
}

static inline void ctlv_test_dynamic_noop_disable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x0003000000000003ULL, true);
}

static inline void ctlv_test_dynamic_toggle_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000004ULL);
}

/* Reading a validated message. A blob has a resolver instead of a getter. */

#define ctlv_test_probe_get_probe_token_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[1].request, \
		CTLV_TEST_ATTR_PROBE_TOKEN, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_probe_get_probe_token_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_probe_get_probe_token_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_probe_get_probe_token_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_probe_get_probe_token_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_dynamic_toggle_probe_token_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[3].request, \
		CTLV_TEST_ATTR_PROBE_TOKEN, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_dynamic_toggle_probe_token_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_dynamic_toggle_probe_token_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_dynamic_toggle_probe_token_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_dynamic_toggle_probe_token_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_u_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_U, \
		0, 0, CTLV_SCHEMA_UINT })

static inline bool ctlv_test_echo_u_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_u_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_echo_u_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_u_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_s_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, CTLV_TEST_ATTR_ECHO_S, \
		0, 1, CTLV_SCHEMA_SINT })

static inline bool ctlv_test_echo_s_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_s_ref());
	return ctlv_attr_ok(&attr);
}

static inline s64 ctlv_test_echo_s_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_s_ref());
	return ctlv_attr_s64(&attr);
}

#define ctlv_test_echo_flag_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_FLAG, 0, 2, CTLV_SCHEMA_BOOL })

static inline bool ctlv_test_echo_flag_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_flag_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_echo_flag_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_flag_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_echo_mode_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_MODE, 0, 3, CTLV_SCHEMA_ENUM })

static inline bool ctlv_test_echo_mode_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_mode_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_echo_mode_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_mode_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_quirks_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_QUIRKS, 0, 4, CTLV_SCHEMA_FLAGS })

static inline bool ctlv_test_echo_quirks_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_quirks_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_echo_quirks_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_quirks_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_text_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_TEXT, 0, 5, CTLV_SCHEMA_STRING })

static inline bool ctlv_test_echo_text_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_text_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_test_echo_text_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_text_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_test_echo_data_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_DATA, 0, 6, CTLV_SCHEMA_BINARY })

static inline bool ctlv_test_echo_data_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_data_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_test_echo_data_get(const struct ctlv_cursor *nest, u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_data_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_test_echo_count_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_COUNT, 0, 7, CTLV_SCHEMA_UINT })

#define ctlv_test_echo_count_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, ctlv_test_echo_count_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline u64 ctlv_test_echo_count_get(const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr, ctlv_test_echo_count_ref()))
		return 0;
	return ctlv_attr_u64(attr);
}

#define ctlv_test_echo_item_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_ITEM, 0, 8, CTLV_SCHEMA_NEST })

#define ctlv_test_echo_item_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, ctlv_test_echo_item_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_test_echo_region_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_REGION, 0, 9, CTLV_SCHEMA_BLOB })

static inline bool ctlv_test_echo_region_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_region_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_echo_region_resolve(struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_region_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_echo_page_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ECHO_PAGE, 0, 10, CTLV_SCHEMA_BLOB })

static inline bool ctlv_test_echo_page_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_page_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_echo_page_resolve(struct ctlv_op_ctx *ctx,
			    const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_page_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_echo_probe_extension_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_PROBE_EXTENSION, 0, 11, CTLV_SCHEMA_NEST })

static inline bool
ctlv_test_echo_probe_extension_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_probe_extension_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_test_echo_probe_extension_get(struct ctlv_cursor *nested,
				   const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest, ctlv_test_echo_probe_extension_ref());
}

#define ctlv_test_echo_item_index_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ITEM_INDEX, 9, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_echo_item_index_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_index_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_echo_item_index_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_index_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_item_label_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ITEM_LABEL, 9, 1, CTLV_SCHEMA_STRING })

static inline bool
ctlv_test_echo_item_label_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_label_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_test_echo_item_label_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_label_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_test_echo_item_leaf_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ITEM_LEAF, 9, 2, CTLV_SCHEMA_NEST })

static inline bool
ctlv_test_echo_item_leaf_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_leaf_ref());
	return ctlv_attr_ok(&attr);
}

static inline void ctlv_test_echo_item_leaf_get(struct ctlv_cursor *nested,
						const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest, ctlv_test_echo_item_leaf_ref());
}

#define ctlv_test_echo_item_leaf_index_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[5].request, \
		CTLV_TEST_ATTR_ITEM_INDEX, 15, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_echo_item_leaf_index_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_leaf_index_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_item_leaf_index_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_item_leaf_index_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_reply_echo_u_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, CTLV_TEST_ATTR_ECHO_U, \
		0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_echo_reply_echo_u_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_u_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_reply_echo_u_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_u_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_reply_echo_s_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, CTLV_TEST_ATTR_ECHO_S, \
		0, 1, CTLV_SCHEMA_SINT })

static inline bool
ctlv_test_echo_reply_echo_s_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_s_ref());
	return ctlv_attr_ok(&attr);
}

static inline s64
ctlv_test_echo_reply_echo_s_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_s_ref());
	return ctlv_attr_s64(&attr);
}

#define ctlv_test_echo_reply_echo_flag_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_FLAG, 0, 2, CTLV_SCHEMA_BOOL })

static inline bool
ctlv_test_echo_reply_echo_flag_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_flag_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool
ctlv_test_echo_reply_echo_flag_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_flag_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_echo_reply_echo_mode_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_MODE, 0, 3, CTLV_SCHEMA_ENUM })

static inline bool
ctlv_test_echo_reply_echo_mode_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_mode_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_reply_echo_mode_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_mode_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_reply_echo_quirks_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_QUIRKS, 0, 4, CTLV_SCHEMA_FLAGS })

static inline bool
ctlv_test_echo_reply_echo_quirks_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_quirks_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_reply_echo_quirks_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_quirks_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_reply_echo_text_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_TEXT, 0, 5, CTLV_SCHEMA_STRING })

static inline bool
ctlv_test_echo_reply_echo_text_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_text_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_test_echo_reply_echo_text_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_text_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_test_echo_reply_echo_data_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_DATA, 0, 6, CTLV_SCHEMA_BINARY })

static inline bool
ctlv_test_echo_reply_echo_data_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_data_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_test_echo_reply_echo_data_get(const struct ctlv_cursor *nest, u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_data_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_test_echo_reply_echo_count_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_COUNT, 0, 7, CTLV_SCHEMA_UINT })

#define ctlv_test_echo_reply_echo_count_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
				ctlv_test_echo_reply_echo_count_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline u64
ctlv_test_echo_reply_echo_count_get(const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr, ctlv_test_echo_reply_echo_count_ref()))
		return 0;
	return ctlv_attr_u64(attr);
}

#define ctlv_test_echo_reply_echo_item_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_ITEM, 0, 8, CTLV_SCHEMA_NEST })

#define ctlv_test_echo_reply_echo_item_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
				ctlv_test_echo_reply_echo_item_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_test_echo_reply_echo_region_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_REGION, 0, 9, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_test_echo_reply_echo_region_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_region_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_echo_reply_echo_region_resolve(struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_region_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_echo_reply_echo_page_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ECHO_PAGE, 0, 10, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_test_echo_reply_echo_page_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_page_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_echo_reply_echo_page_resolve(struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_echo_page_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_echo_reply_probe_extension_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_PROBE_EXTENSION, 0, 11, CTLV_SCHEMA_NEST })

static inline bool
ctlv_test_echo_reply_probe_extension_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_echo_reply_probe_extension_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_test_echo_reply_probe_extension_get(struct ctlv_cursor *nested,
					 const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest,
			ctlv_test_echo_reply_probe_extension_ref());
}

#define ctlv_test_echo_reply_item_index_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ITEM_INDEX, 9, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_echo_reply_item_index_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_item_index_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_reply_item_index_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_item_index_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_echo_reply_item_label_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ITEM_LABEL, 9, 1, CTLV_SCHEMA_STRING })

static inline bool
ctlv_test_echo_reply_item_label_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_item_label_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_test_echo_reply_item_label_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_item_label_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_test_echo_reply_item_leaf_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ITEM_LEAF, 9, 2, CTLV_SCHEMA_NEST })

static inline bool
ctlv_test_echo_reply_item_leaf_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_echo_reply_item_leaf_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_test_echo_reply_item_leaf_get(struct ctlv_cursor *nested,
				   const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest, ctlv_test_echo_reply_item_leaf_ref());
}

#define ctlv_test_echo_reply_item_leaf_index_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[6].request, \
		CTLV_TEST_ATTR_ITEM_INDEX, 15, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_echo_reply_item_leaf_index_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_echo_reply_item_leaf_index_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_echo_reply_item_leaf_index_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_echo_reply_item_leaf_index_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_series_dump_series_chunks_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[7].request, \
		CTLV_TEST_ATTR_SERIES_CHUNKS, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_series_dump_series_chunks_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_chunks_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_series_dump_series_chunks_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_chunks_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_series_dump_series_items_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[7].request, \
		CTLV_TEST_ATTR_SERIES_ITEMS, 0, 1, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_series_dump_series_items_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_items_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_series_dump_series_items_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_items_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_series_dump_series_values_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[7].request, \
		CTLV_TEST_ATTR_SERIES_VALUES, 0, 2, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_series_dump_series_values_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_values_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_series_dump_series_values_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_values_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_series_dump_series_totals_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[7].request, \
		CTLV_TEST_ATTR_SERIES_TOTALS, 0, 3, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_series_dump_series_totals_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_totals_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_series_dump_series_totals_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_series_dump_series_totals_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_ping_count_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[8].request, \
		CTLV_TEST_ATTR_PING_COUNT, 0, 0, CTLV_SCHEMA_UINT })

static inline bool ctlv_test_ping_count_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_count_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_ping_count_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_count_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_ping_serial_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[8].request, \
		CTLV_TEST_ATTR_PING_SERIAL, 0, 1, CTLV_SCHEMA_UINT })

static inline bool ctlv_test_ping_serial_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_serial_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_ping_serial_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_serial_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_ping_payload_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[8].request, \
		CTLV_TEST_ATTR_PING_PAYLOAD, 0, 2, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_ping_payload_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_payload_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_ping_payload_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_payload_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_ping_abort_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[8].request, \
		CTLV_TEST_ATTR_PING_ABORT, 0, 3, CTLV_SCHEMA_BOOL })

static inline bool ctlv_test_ping_abort_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_abort_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_ping_abort_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_abort_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_ping_empty_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[8].request, \
		CTLV_TEST_ATTR_PING_EMPTY, 0, 4, CTLV_SCHEMA_BOOL })

static inline bool ctlv_test_ping_empty_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_empty_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_ping_empty_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_ping_empty_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_transfer_source_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_SOURCE, 0, 0, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_test_transfer_source_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_source_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_transfer_source_resolve(struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_source_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_transfer_target_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_TARGET, 0, 1, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_test_transfer_target_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_target_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_transfer_target_resolve(struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_target_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_transfer_scratch_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_SCRATCH, 0, 2, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_test_transfer_scratch_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_scratch_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_test_transfer_scratch_resolve(struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_scratch_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_test_transfer_piece_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_PIECE, 0, 3, CTLV_SCHEMA_BLOB })

#define ctlv_test_transfer_piece_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, ctlv_test_transfer_piece_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline struct ctlv_blob *
ctlv_test_transfer_piece_resolve(struct ctlv_op_ctx *ctx,
				 const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr, ctlv_test_transfer_piece_ref()))
		return ERR_PTR(-EINVAL);
	return ctlv_blob_resolve(ctx, attr);
}

#define ctlv_test_transfer_fill_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_FILL, 0, 4, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_transfer_fill_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_fill_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64 ctlv_test_transfer_fill_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_fill_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_test_transfer_skip_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_SKIP, 0, 5, CTLV_SCHEMA_BOOL })

static inline bool
ctlv_test_transfer_skip_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_skip_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_transfer_skip_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_skip_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_transfer_twice_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_TWICE, 0, 6, CTLV_SCHEMA_BOOL })

static inline bool
ctlv_test_transfer_twice_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_twice_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_transfer_twice_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_twice_ref());
	return ctlv_attr_bool(&attr);
}

#define ctlv_test_transfer_hold_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_ctlv_op_schemas[10].request, \
		CTLV_TEST_ATTR_TRANSFER_HOLD, 0, 7, CTLV_SCHEMA_BOOL })

static inline bool
ctlv_test_transfer_hold_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_hold_ref());
	return ctlv_attr_ok(&attr);
}

static inline bool ctlv_test_transfer_hold_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_test_transfer_hold_ref());
	return ctlv_attr_bool(&attr);
}

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */

static inline bool ctlv_test_probe_token_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_PROBE_TOKEN, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_probe_extension_start(struct ctlv_cursor *nest,
						   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_TEST_ATTR_PROBE_EXTENSION,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_test_echo_u_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_ECHO_U, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_echo_s_put(struct ctlv_cursor *nest, s64 value)
{
	return ctlv_put_s64(nest, CTLV_TEST_ATTR_ECHO_S, CTLV_SCHEMA_SINT,
			    value);
}

static inline bool ctlv_test_echo_flag_put(struct ctlv_cursor *nest, bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_ECHO_FLAG, CTLV_SCHEMA_BOOL,
			     value);
}

static inline bool ctlv_test_echo_mode_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_ECHO_MODE, CTLV_SCHEMA_ENUM,
			    value);
}

static inline bool ctlv_test_echo_quirks_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_ECHO_QUIRKS, CTLV_SCHEMA_FLAGS,
			    value);
}

static inline bool ctlv_test_echo_text_put(struct ctlv_cursor *nest,
					   const char *value)
{
	return ctlv_put_string(nest, CTLV_TEST_ATTR_ECHO_TEXT,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool ctlv_test_echo_data_put(struct ctlv_cursor *nest,
					   const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_TEST_ATTR_ECHO_DATA,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool ctlv_test_echo_count_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_ECHO_COUNT, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_echo_item_start(struct ctlv_cursor *nest,
					     struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_TEST_ATTR_ECHO_ITEM,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_test_item_index_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_ITEM_INDEX, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_item_label_put(struct ctlv_cursor *nest,
					    const char *value)
{
	return ctlv_put_string(nest, CTLV_TEST_ATTR_ITEM_LABEL,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool ctlv_test_item_leaf_start(struct ctlv_cursor *nest,
					     struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_TEST_ATTR_ITEM_LEAF,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_test_series_chunks_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_CHUNKS,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_test_series_items_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_ITEMS, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_series_values_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_VALUES,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_test_series_totals_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_TOTALS,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_test_series_chunk_start(struct ctlv_cursor *nest,
						struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_TEST_ATTR_SERIES_CHUNK,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_test_series_item_start(struct ctlv_cursor *nest,
					       struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_TEST_ATTR_SERIES_ITEM,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_test_series_value_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_VALUE, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_series_total_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_TOTAL, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_series_index_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_SERIES_INDEX, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_ping_count_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_PING_COUNT, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_ping_serial_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_PING_SERIAL, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_ping_payload_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_PING_PAYLOAD, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_ping_abort_put(struct ctlv_cursor *nest,
					    bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_PING_ABORT, CTLV_SCHEMA_BOOL,
			     value);
}

static inline bool ctlv_test_ping_empty_put(struct ctlv_cursor *nest,
					    bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_PING_EMPTY, CTLV_SCHEMA_BOOL,
			     value);
}

static inline bool ctlv_test_ping_filler_put(struct ctlv_cursor *nest,
					     const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_TEST_ATTR_PING_FILLER,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool ctlv_test_transfer_fill_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_TRANSFER_FILL,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_test_transfer_skip_put(struct ctlv_cursor *nest,
					       bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_TRANSFER_SKIP,
			     CTLV_SCHEMA_BOOL, value);
}

static inline bool ctlv_test_transfer_twice_put(struct ctlv_cursor *nest,
						bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_TRANSFER_TWICE,
			     CTLV_SCHEMA_BOOL, value);
}

static inline bool ctlv_test_transfer_hold_put(struct ctlv_cursor *nest,
					       bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_TRANSFER_HOLD,
			     CTLV_SCHEMA_BOOL, value);
}

static inline bool ctlv_test_transfer_sum_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_TRANSFER_SUM, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_test_transfer_written_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_ATTR_TRANSFER_WRITTEN,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_test_transfer_distinct_put(struct ctlv_cursor *nest,
						   bool value)
{
	return ctlv_put_bool(nest, CTLV_TEST_ATTR_TRANSFER_DISTINCT,
			     CTLV_SCHEMA_BOOL, value);
}

/* Building one event, passed to ctlv_event_emit() or ctlv_event_abort() once. */

static inline struct ctlv_op_ctx *
ctlv_test_pinged_create(struct ctlv_device *ctlvdev, struct ctlv_cursor **reply,
			gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x0003000000000005ULL, reply, gfp);
}

static inline struct ctlv_op_ctx *
ctlv_test_beeped_create(struct ctlv_device *ctlvdev, struct ctlv_cursor **reply,
			gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x000300000000000aULL, reply, gfp);
}
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_TEST_GEN_H */
