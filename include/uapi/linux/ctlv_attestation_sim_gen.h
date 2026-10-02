/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-sim CTLV family. Attr and op IDs are complete; the wire format
 * carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H
#define _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_attestation_gen.h>
#include <linux/ctlv_families_gen.h>

/* attr groups */
enum _ctlv_attestation_sim_group {
	CTLV_ATTESTATION_SIM_GROUP_SIM = 1,
};

#define _CTLV_ATTESTATION_SIM_ATTR(_group, _attr) \
	CTLV_ATTR(CTLV_FAMILY_ATTESTATION_SIM, (_group), (_attr))

/* attrs of group sim */
#define _CTLV_ATTESTATION_SIM_ATTR_SIM(_attr) \
	_CTLV_ATTESTATION_SIM_ATTR(CTLV_ATTESTATION_SIM_GROUP_SIM, \
				   _CTLV_ATTESTATION_SIM_ATTR_SIM_##_attr)

enum _ctlv_attestation_sim_attr_sim {
	_CTLV_ATTESTATION_SIM_ATTR_SIM_NAME = 1,
};

#define CTLV_ATTESTATION_SIM_ATTR_SIM_NAME _CTLV_ATTESTATION_SIM_ATTR_SIM(NAME)

#endif /* _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H */
