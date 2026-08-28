/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-test-child CTLV family. */

#ifndef _LINUX_CTLV_TEST_CHILD_GEN_H
#define _LINUX_CTLV_TEST_CHILD_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_test_child_gen.h>

#include "ctlv_test_gen.h"

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_test_child_ctlv_op_schemas[];

extern const struct ctlv_family_schema ctlv_test_child_ctlv_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_test_child_extra_noop_action(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request,
				      struct ctlv_cursor *reply);

/* Family-wide hooks around every effective non-core op. */
int ctlv_test_child_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_test_child_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_test_child_priv_size(void);
const void *ctlv_test_child_pre_init(const void *init_info);
int ctlv_test_child_init(struct ctlv_device *ctlvdev, const void *init_info,
			 void *priv);
void ctlv_test_child_pre_fini(struct ctlv_device *ctlvdev, void *priv);
void ctlv_test_child_fini(struct ctlv_device *ctlvdev, void *priv);

/* extension-point of ctlv-test: fill and read callbacks. */
int ctlv_test_child_extension_point_fill(void *priv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *nest);
void ctlv_test_child_extension_point_read(void *priv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *nest);

extern const struct ctlv_family ctlv_test_child_ctlv_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */

#define ctlv_test_child_extension_point_extra_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_test_child_ctlv_schema.fills[0].members, \
		CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_test_child_extension_point_extra_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_child_extension_point_extra_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_test_child_extension_point_extra_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_test_child_extension_point_extra_ref());
	return ctlv_attr_u64(&attr);
}

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */

static inline bool ctlv_test_child_child_extra_put(struct ctlv_cursor *nest,
						   u64 value)
{
	return ctlv_put_u64(nest, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA,
			    CTLV_SCHEMA_UINT, value);
}
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_TEST_CHILD_GEN_H */
