/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-sim.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-attestation-sim CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_SIM_GEN_H
#define _LINUX_CTLV_ATTESTATION_SIM_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/ctlv/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_gen.h"

extern const struct ctlv_family_schema ctlv_attestation_sim_schema;

/* device-info of ctlv-attestation: fill and read callbacks. */
int ctlv_attestation_sim_device_info_fill(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *record);
void ctlv_attestation_sim_device_info_read(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *record);

extern const struct ctlv_family ctlv_attestation_sim_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Writing a reply. An append is atomic; a record ends with ctlv_record_end(). */
/* The name of the fixture the device belongs to. */
CTLV_PUT_STRING(ctlv_attestation_sim_device_info_name, CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME,
		CTLV_SCHEMA_STRING)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_ATTESTATION_SIM_GEN_H */
