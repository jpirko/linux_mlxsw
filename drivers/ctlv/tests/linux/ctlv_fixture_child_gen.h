/* SPDX-License-Identifier: GPL-2.0 */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture-child.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-fixture-child CTLV family. Attr and op IDs are
 * complete; the wire format carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-fixture header is included here, so one include covers the
 * whole effective schema.
 */

#ifndef _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H
#define _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_fixture_gen.h>

#define CTLV_FIXTURE_CHILD_FAMILY_ID 2

/* attr group child */
#define CTLV_FIXTURE_CHILD_GROUP_CHILD 1

#define CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG \
	CTLV_ATTR_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, \
		     CTLV_FIXTURE_CHILD_GROUP_CHILD, 1)
#define CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL \
	CTLV_ATTR_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, \
		     CTLV_FIXTURE_CHILD_GROUP_CHILD, 2)
#define CTLV_FIXTURE_CHILD_ATTR_CHILD_DUMP \
	CTLV_ATTR_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, \
		     CTLV_FIXTURE_CHILD_GROUP_CHILD, 3)

/* attr sets */
#define CTLV_FIXTURE_CHILD_SET_CHILD_KEY 1

/* ops */
#define CTLV_FIXTURE_CHILD_OP_CHILD_INFO_GET \
	CTLV_OP_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, 1)
#define CTLV_FIXTURE_CHILD_OP_CHILD_DUMP \
	CTLV_OP_ID(CTLV_FIXTURE_CHILD_FAMILY_ID, 2)

#endif /* _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H */
