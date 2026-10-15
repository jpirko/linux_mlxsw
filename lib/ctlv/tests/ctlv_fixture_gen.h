/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generated from tools/ctlv/tests/specs/ctlv-fixture.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Schema metadata of the ctlv-fixture CTLV family. */

#ifndef _LINUX_CTLV_FIXTURE_GEN_H
#define _LINUX_CTLV_FIXTURE_GEN_H

#include <linux/ctlv_family.h>

/* Extension points, as a descendant's fill names one. */
#define CTLV_FIXTURE_EXT_EXTENSION_POINT 1

/* Enum extension points, numbered apart from the others. */
#define CTLV_FIXTURE_ENUM_EXT_VARIANT 1

/* Ops this family declares, in declaration order. */
extern const struct ctlv_op_schema ctlv_fixture_op_schemas[];

extern const struct ctlv_family_schema ctlv_fixture_schema;

#endif /* _LINUX_CTLV_FIXTURE_GEN_H */
