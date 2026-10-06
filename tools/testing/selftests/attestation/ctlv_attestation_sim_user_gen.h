/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-sim.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation-sim CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H

#include <ctlv.h>

#include <ctlv/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_user_gen.h"

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_sim_spec;

/* Building a request. ctlv_msg_err() reports the first failure. */

/* The name of the fixture the device belongs to. */
static inline int ctlv_attestation_sim_device_info_name_put(struct ctlv_msg *msg, const char *value)
{
	return ctlv_put_str(msg, CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME, value);
}

/* Reading a reply or an event. A getter answers whether it could read. */

/* The name of the fixture the device belongs to. */
static inline bool ctlv_attestation_sim_device_info_name_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME);
}

static inline bool ctlv_attestation_sim_device_info_name_get(const struct ctlv_hdr *record,
							     const char **value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME);
	return hdr && !ctlv_get_str(hdr, value);
}

#endif /* _CTLV_CTLV_ATTESTATION_SIM_USER_GEN_H */
