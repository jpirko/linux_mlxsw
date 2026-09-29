/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and driver API of the ctlv-attestation-sim CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_SIM_GEN_H
#define _LINUX_CTLV_ATTESTATION_SIM_GEN_H

#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_gen.h"

/* Ops this family declares, in declaration order. */
#define CTLV_ATTESTATION_SIM_N_OPS 0

extern const struct ctlv_family_schema ctlv_attestation_sim_ctlv_schema;

/*
 * Called around every effective non-core action or query,
 * on a device created with this family.
 */
int ctlv_attestation_sim_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_attestation_sim_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* The lifecycle of this family's core-allocated priv. */
size_t ctlv_attestation_sim_priv_size(void);
const void *ctlv_attestation_sim_pre_init(const void *init_info);
int ctlv_attestation_sim_init(struct ctlv_device *ctlvdev,
			      const void *init_info, void *priv);
void ctlv_attestation_sim_fini(struct ctlv_device *ctlvdev, void *priv);

/* device-info of ctlv-attestation: what this family puts into it, and reads. */
int ctlv_attestation_sim_device_info_fill(void *priv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *nest);
void ctlv_attestation_sim_device_info_read(void *priv, struct ctlv_op_ctx *ctx,
					   const struct ctlv_cursor *nest);

extern const struct ctlv_family ctlv_attestation_sim_ctlv_family;

/* Reading a validated message. A blob has a resolver instead of a getter. */

#define ctlv_attestation_sim_device_info_name_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_sim_ctlv_schema.fills[0].members, \
		CTLV_ATTESTATION_SIM_ATTR_SIM_NAME, 0, 0, \
		CTLV_SCHEMA_STRING })

static inline bool
ctlv_attestation_sim_device_info_name_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_sim_device_info_name_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_attestation_sim_device_info_name_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_sim_device_info_name_ref());
	return ctlv_attr_string(&attr);
}

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */

static inline bool ctlv_attestation_sim_sim_name_put(struct ctlv_cursor *nest,
						     const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_SIM_ATTR_SIM_NAME,
			       CTLV_SCHEMA_STRING, value);
}

#endif /* _LINUX_CTLV_ATTESTATION_SIM_GEN_H */
