/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-tdx.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-tdx CTLV family. Types and op IDs are complete; the message
 * format carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_CTLV_CTLV_ATTESTATION_TDX_GEN_H
#define _UAPI_CTLV_CTLV_ATTESTATION_TDX_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_attestation_gen.h>
#include <ctlv/ctlv_families_gen.h>

/* enum evidence-vendor-format, an extension point of ctlv-attestation */
enum _ctlv_attestation_tdx_evidence_vendor_format {
	/* An Intel TDX DCAP Quote, from the host's quoting enclave. */
	CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE = 1,
	/* A DICE-based TDX Quote, an EAT in a CBOR Web Token. */
	CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE_DICE = 2,
};

/* device-info */
/* MRCONFIGID of the TD, the ID of its non-owner configuration. */
#define CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MRCONFIGID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION_TDX, 0x00010000, 1)
/* MROWNER of the TD, the ID of its owner. */
#define CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNER \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION_TDX, 0x00010000, 2)
/* MROWNERCONFIG of the TD, the ID of its owner's configuration. */
#define CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNERCONFIG \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION_TDX, 0x00010000, 3)

#endif /* _UAPI_CTLV_CTLV_ATTESTATION_TDX_GEN_H */
