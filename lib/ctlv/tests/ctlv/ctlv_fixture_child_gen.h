/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generated from tools/ctlv/tests/specs/ctlv-fixture-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-fixture-child CTLV family. Types and op IDs are complete; the message
 * format carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv-fixture header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_CTLV_CTLV_FIXTURE_CHILD_GEN_H
#define _UAPI_CTLV_CTLV_FIXTURE_CHILD_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>
#include <ctlv/ctlv_fixture_gen.h>

/* enum variant, an extension point of ctlv-fixture */
enum _ctlv_fixture_child_variant {
	/* The first value. */
	CTLV_FIXTURE_CHILD_VARIANT_PLAIN = 1,
	/* The second and highest. */
	CTLV_FIXTURE_CHILD_VARIANT_FANCY = 2,
};

/* serial */
/* Placed in the fill and in a request, with one type in both. */
#define CTLV_FIXTURE_CHILD_T_SERIAL \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE_CHILD, 0x00010000, 1)

/* extension-point */
/* A string the fill adds. */
#define CTLV_FIXTURE_CHILD_T_EXTENSION_POINT_TAG \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE_CHILD, 0x00020000, 1)

/* child-dump reply */
/* A binary of up to a page. */
#define CTLV_FIXTURE_CHILD_T_CHILD_DUMP_DUMP \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE_CHILD, 0xc0020000, 1)

/* ops */
#define CTLV_FIXTURE_CHILD_OP_CHILD_INFO_GET CTLV_OP(CTLV_FAMILY_FIXTURE_CHILD, 1)
#define CTLV_FIXTURE_CHILD_OP_CHILD_DUMP CTLV_OP(CTLV_FAMILY_FIXTURE_CHILD, 2)

#endif /* _UAPI_CTLV_CTLV_FIXTURE_CHILD_GEN_H */
