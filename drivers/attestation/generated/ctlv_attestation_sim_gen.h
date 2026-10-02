/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-attestation-sim CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_SIM_GEN_H
#define _LINUX_CTLV_ATTESTATION_SIM_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_gen.h"

extern const struct ctlv_family_schema ctlv_attestation_sim_schema;

/* Family-wide hooks around every effective non-core op. */
int ctlv_attestation_sim_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_attestation_sim_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_attestation_sim_priv_size(void);
const void *ctlv_attestation_sim_pre_init(const void *init_info);
int ctlv_attestation_sim_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
void ctlv_attestation_sim_fini(struct ctlv_device *ctlvdev, void *priv);

/* device-info of ctlv-attestation: fill and read callbacks. */
int ctlv_attestation_sim_device_info_fill(void *priv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *nest);
void ctlv_attestation_sim_device_info_read(void *priv, struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *nest);

extern const struct ctlv_family ctlv_attestation_sim_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */
CTLV_IN_REF(ctlv_attestation_sim_device_info_name,
	    &ctlv_attestation_sim_schema.stub_fill_schemas[0].members,
	    CTLV_ATTESTATION_SIM_ATTR_SIM_NAME, 0, 0, CTLV_SCHEMA_STRING)
CTLV_IN_HAS(ctlv_attestation_sim_device_info_name)
CTLV_IN_STRING(ctlv_attestation_sim_device_info_name)

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */
CTLV_PUT_STRING(ctlv_attestation_sim_sim_name, CTLV_ATTESTATION_SIM_ATTR_SIM_NAME,
		CTLV_SCHEMA_STRING)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_ATTESTATION_SIM_GEN_H */
