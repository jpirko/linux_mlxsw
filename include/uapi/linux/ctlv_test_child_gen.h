/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-test-child CTLV family. Attr and op IDs are complete;
 * the wire format carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-test header is included here, so one include covers the
 * whole effective schema.
 */

#ifndef _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H
#define _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_test_gen.h>

#define CTLV_TEST_CHILD_FAMILY_ID 4

/* attr group child */
#define CTLV_TEST_CHILD_GROUP_CHILD 1

#define CTLV_TEST_CHILD_ATTR_CHILD_EXTRA \
	CTLV_ATTR_ID(CTLV_TEST_CHILD_FAMILY_ID, CTLV_TEST_CHILD_GROUP_CHILD, 1)

/* ops */
#define CTLV_TEST_CHILD_OP_EXTRA_NOOP CTLV_OP_ID(CTLV_TEST_CHILD_FAMILY_ID, 1)

#endif /* _UAPI_LINUX_CTLV_TEST_CHILD_GEN_H */
