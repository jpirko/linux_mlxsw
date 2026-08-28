/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-test-child CTLV family. */

#ifndef _LINUX_CTLV_TEST_CHILD_GEN_H
#define _LINUX_CTLV_TEST_CHILD_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/ctlv/ctlv_test_child_gen.h>

#include "ctlv_test_gen.h"

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_test_child_op_schemas[];

extern const struct ctlv_family_schema ctlv_test_child_schema;

/* Implemented by this family, one per action and query it declares. */
int ctlv_test_child_extra_noop_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request, struct ctlv_cursor *reply);

/* extension-point of ctlv-test: fill and read callbacks. */
int ctlv_test_child_extension_point_fill(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *record);
void ctlv_test_child_extension_point_read(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *record);

extern const struct ctlv_family ctlv_test_child_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */
/* The one member the child adds, wherever the extension point is. */
CTLV_IN_REF(ctlv_test_child_extension_point_extra, CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
	    CTLV_SCHEMA_UINT)
CTLV_IN_HAS(ctlv_test_child_extension_point_extra)
CTLV_IN_U64(ctlv_test_child_extension_point_extra)

/* Writing a reply. An append is atomic; a record ends with ctlv_record_end(). */
/* The one member the child adds, wherever the extension point is. */
CTLV_PUT_U64(ctlv_test_child_extension_point_extra, CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA,
	     CTLV_SCHEMA_UINT)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_TEST_CHILD_GEN_H */
