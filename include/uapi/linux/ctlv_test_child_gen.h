/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-test-child CTLV family. Attr and op IDs are complete; the wire format
 * carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-test header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H
#define _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_families_gen.h>
#include <linux/ctlv_test_gen.h>

/* attr groups */
enum _ctlv_test_child_group {
	CTLV_TEST_CHILD_GROUP_CHILD = 1,
};

#define _CTLV_TEST_CHILD_ATTR(_group, _attr) CTLV_ATTR(CTLV_FAMILY_TEST_CHILD, (_group), (_attr))

/* attrs of group child */
#define _CTLV_TEST_CHILD_ATTR_CHILD(_attr) \
	_CTLV_TEST_CHILD_ATTR(CTLV_TEST_CHILD_GROUP_CHILD, _CTLV_TEST_CHILD_ATTR_CHILD_##_attr)

enum _ctlv_test_child_attr_child {
	_CTLV_TEST_CHILD_ATTR_CHILD_EXTRA = 1,
};

#define CTLV_TEST_CHILD_ATTR_CHILD_EXTRA _CTLV_TEST_CHILD_ATTR_CHILD(EXTRA)

/* ops */
enum _ctlv_test_child_op {
	_CTLV_TEST_CHILD_OP_EXTRA_NOOP = 1,
};

#define _CTLV_TEST_CHILD_OP(_op) CTLV_OP(CTLV_FAMILY_TEST_CHILD, _CTLV_TEST_CHILD_OP_##_op)
#define CTLV_TEST_CHILD_OP_EXTRA_NOOP _CTLV_TEST_CHILD_OP(EXTRA_NOOP)

#endif /* _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H */
