/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation-tdx CTLV family. Attr and op IDs are complete; the wire format
 * carrying them is in <linux/ctlv.h>.
 *
 * The inherited ctlv-attestation header is included here, so one include covers the whole effective
 * schema.
 */

#ifndef _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H
#define _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H

#include <linux/ctlv.h>
#include <linux/ctlv_attestation_gen.h>
#include <linux/ctlv_families_gen.h>

/* attr groups */
enum _ctlv_attestation_tdx_group {
	CTLV_ATTESTATION_TDX_GROUP_INFO = 1,
};

#define _CTLV_ATTESTATION_TDX_ATTR(_group, _attr) \
	CTLV_ATTR(CTLV_FAMILY_ATTESTATION_TDX, (_group), (_attr))

/* attrs of group info */
#define _CTLV_ATTESTATION_TDX_ATTR_INFO(_attr) \
	_CTLV_ATTESTATION_TDX_ATTR(CTLV_ATTESTATION_TDX_GROUP_INFO, \
				   _CTLV_ATTESTATION_TDX_ATTR_INFO_##_attr)

enum _ctlv_attestation_tdx_attr_info {
	_CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID = 1,
	_CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER,
	_CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG,
};

#define CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID _CTLV_ATTESTATION_TDX_ATTR_INFO(MRCONFIGID)
#define CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER _CTLV_ATTESTATION_TDX_ATTR_INFO(MROWNER)
#define CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG _CTLV_ATTESTATION_TDX_ATTR_INFO(MROWNERCONFIG)

#endif /* _UAPI_LINUX_CTLV_ATTESTATION_TDX_GEN_H */
