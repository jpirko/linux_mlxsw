/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-sim.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-sim CTLV family. Attr and op IDs are
 * complete; the wire format carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers
 * the whole effective schema.
 */

#ifndef _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H
#define _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_attestation_gen.h>

#define CTLV_ATTESTATION_SIM_FAMILY_ID 4

/* attr group sim */
#define CTLV_ATTESTATION_SIM_GROUP_SIM 1

#define CTLV_ATTESTATION_SIM_ATTR_SIM_NAME \
	CTLV_ATTR_ID(CTLV_ATTESTATION_SIM_FAMILY_ID, \
		     CTLV_ATTESTATION_SIM_GROUP_SIM, 1)

#endif /* _UAPI_LINUX_CTLV_ATTESTATION_SIM_GEN_H */
