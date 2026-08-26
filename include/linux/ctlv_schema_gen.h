/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN kernel header */

/* Schema metadata of the ctlv CTLV family. */

#ifndef _LINUX_CTLV_SCHEMA_GEN_H
#define _LINUX_CTLV_SCHEMA_GEN_H

#include <linux/ctlv.h>

/* Ops this family declares, in declaration order. */
#define CTLV_N_OPS 10

extern const struct ctlv_op_schema ctlv_ctlv_op_schemas[CTLV_N_OPS];

extern const struct ctlv_family_schema ctlv_ctlv_schema;

/* The error-info message, built by the core. */
#define CTLV_ERROR_INFO_MAX_LEN 432

extern const struct ctlv_schema_tree ctlv_error_info_schema;

#endif /* _LINUX_CTLV_SCHEMA_GEN_H */
