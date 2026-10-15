/* SPDX-License-Identifier: GPL-2.0 */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	tools/ctlv/tests/specs/ctlv-fixture-child.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-fixture-child CTLV family. Attr and op IDs are complete; the wire format
 * carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-fixture header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H
#define _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_families_gen.h>
#include <linux/ctlv_fixture_gen.h>

/* attr groups */
enum _ctlv_fixture_child_group {
	CTLV_FIXTURE_CHILD_GROUP_CHILD = 1,
};

#define _CTLV_FIXTURE_CHILD_ATTR(_group, _attr) \
	CTLV_ATTR(CTLV_FAMILY_FIXTURE_CHILD, (_group), (_attr))

/* attrs of group child */
#define _CTLV_FIXTURE_CHILD_ATTR_CHILD(_attr) \
	_CTLV_FIXTURE_CHILD_ATTR(CTLV_FIXTURE_CHILD_GROUP_CHILD, \
				 _CTLV_FIXTURE_CHILD_ATTR_CHILD_##_attr)

enum _ctlv_fixture_child_attr_child {
	_CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG = 1,
	_CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL,
	_CTLV_FIXTURE_CHILD_ATTR_CHILD_DUMP,
};

#define CTLV_FIXTURE_CHILD_ATTR_CHILD_TAG _CTLV_FIXTURE_CHILD_ATTR_CHILD(TAG)
#define CTLV_FIXTURE_CHILD_ATTR_CHILD_SERIAL _CTLV_FIXTURE_CHILD_ATTR_CHILD(SERIAL)
#define CTLV_FIXTURE_CHILD_ATTR_CHILD_DUMP _CTLV_FIXTURE_CHILD_ATTR_CHILD(DUMP)

/* attr sets */
enum _ctlv_fixture_child_set {
	CTLV_FIXTURE_CHILD_SET_CHILD_KEY = 1,
};

/* ops */
enum _ctlv_fixture_child_op {
	_CTLV_FIXTURE_CHILD_OP_CHILD_INFO_GET = 1,
	_CTLV_FIXTURE_CHILD_OP_CHILD_DUMP,
};

#define _CTLV_FIXTURE_CHILD_OP(_op) CTLV_OP(CTLV_FAMILY_FIXTURE_CHILD, _CTLV_FIXTURE_CHILD_OP_##_op)
#define CTLV_FIXTURE_CHILD_OP_CHILD_INFO_GET _CTLV_FIXTURE_CHILD_OP(CHILD_INFO_GET)
#define CTLV_FIXTURE_CHILD_OP_CHILD_DUMP _CTLV_FIXTURE_CHILD_OP(CHILD_DUMP)

#endif /* _UAPI_LINUX_CTLV_FIXTURE_CHILD_GEN_H */
