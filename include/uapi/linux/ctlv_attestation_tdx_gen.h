/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-tdx CTLV family. Attr and op IDs are
 * complete; the wire format carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers
 * the whole effective schema.
 */

#ifndef _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H
#define _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_attestation_gen.h>

#define CTLV_ATTESTATION_TDX_FAMILY_ID 5

/* attr group tdx */
#define CTLV_ATTESTATION_TDX_GROUP_TDX 1

#define CTLV_ATTESTATION_TDX_ATTR_TDX_MRCONFIGID \
	CTLV_ATTR_ID(CTLV_ATTESTATION_TDX_FAMILY_ID, \
		     CTLV_ATTESTATION_TDX_GROUP_TDX, 1)
#define CTLV_ATTESTATION_TDX_ATTR_TDX_MROWNER \
	CTLV_ATTR_ID(CTLV_ATTESTATION_TDX_FAMILY_ID, \
		     CTLV_ATTESTATION_TDX_GROUP_TDX, 2)
#define CTLV_ATTESTATION_TDX_ATTR_TDX_MROWNERCONFIG \
	CTLV_ATTR_ID(CTLV_ATTESTATION_TDX_FAMILY_ID, \
		     CTLV_ATTESTATION_TDX_GROUP_TDX, 3)

#endif /* _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H */
