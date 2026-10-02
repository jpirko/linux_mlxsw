/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-sim.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-sim CTLV family. Types and op IDs are complete; the message
 * format carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_CTLV_CTLV_ATTESTATION_SIM_GEN_H
#define _UAPI_CTLV_CTLV_ATTESTATION_SIM_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_attestation_gen.h>
#include <ctlv/ctlv_families_gen.h>

/* enum evidence-vendor-format, an extension point of ctlv-attestation */
enum _ctlv_attestation_sim_evidence_vendor_format {
	/* The simulator's report of its MRs. */
	CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_REPORT = 1,
	/* The simulator's token, which carries the challenge. */
	CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_TOKEN = 2,
	/* The simulator's manifest, which names its fixture. */
	CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_MANIFEST = 3,
};

/* device-info */
/* The name of the fixture the device belongs to. */
#define CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION_SIM, 0x00010000, 1)

#endif /* _UAPI_CTLV_CTLV_ATTESTATION_SIM_GEN_H */
