/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-test-child CTLV family. Types and op IDs are complete; the message format
 * carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv-test header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_CTLV_CTLV_TEST_CHILD_GEN_H
#define _UAPI_CTLV_CTLV_TEST_CHILD_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>
#include <ctlv/ctlv_test_gen.h>

/* enum echo-variant, an extension point of ctlv-test */
enum _ctlv_test_child_echo_variant {
	/* The first value, which means nothing of its own. */
	CTLV_TEST_CHILD_ECHO_VARIANT_PLAIN = 1,
	/* The second and highest, likewise. */
	CTLV_TEST_CHILD_ECHO_VARIANT_FANCY = 2,
};

/* extension-point */
/* The one member the child adds, wherever the extension point is. */
#define CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA \
	CTLV_TYPE(CTLV_FAMILY_TEST_CHILD, 0x00010000, 1)

/* ops */
#define CTLV_TEST_CHILD_OP_EXTRA_NOOP CTLV_OP(CTLV_FAMILY_TEST_CHILD, 1)

#endif /* _UAPI_CTLV_CTLV_TEST_CHILD_GEN_H */
