/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation-sim CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H

#include <ctlv.h>

#include <linux/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_user_gen.h"

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_sim_spec;

/* Building a request. ctlv_msg_err() reports the first failure. */

static inline int ctlv_attestation_sim_sim_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_SIM_ATTR_SIM_NAME, value);
}

/* Reading a reply or a record. A getter answers whether it could read. */

static inline bool ctlv_attestation_sim_sim_name_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_SIM_ATTR_SIM_NAME);
}

static inline bool ctlv_attestation_sim_sim_name_get(const struct ctlv_attr *nest,
						     const char **value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_SIM_ATTR_SIM_NAME);
	return attr && !ctlv_get_str(attr, value);
}

#endif /* _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H */
