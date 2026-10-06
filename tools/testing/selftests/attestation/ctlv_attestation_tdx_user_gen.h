/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-tdx.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation-tdx CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H

#include <ctlv.h>

#include <ctlv/ctlv_attestation_tdx_gen.h>

#include "ctlv_attestation_user_gen.h"

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_tdx_spec;

/* Building a request. ctlv_msg_err() reports the first failure. */

/* MRCONFIGID of the TD, the ID of its non-owner configuration. */
static inline int ctlv_attestation_tdx_device_info_mrconfigid_put(struct ctlv_msg *msg,
								  const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MRCONFIGID, value, len);
}

/* MROWNER of the TD, the ID of its owner. */
static inline int ctlv_attestation_tdx_device_info_mrowner_put(struct ctlv_msg *msg,
							       const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNER, value, len);
}

/* MROWNERCONFIG of the TD, the ID of its owner's configuration. */
static inline int ctlv_attestation_tdx_device_info_mrownerconfig_put(struct ctlv_msg *msg,
								     const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNERCONFIG, value, len);
}

/* Reading a reply or an event. A getter answers whether it could read. */

/* MRCONFIGID of the TD, the ID of its non-owner configuration. */
static inline bool
ctlv_attestation_tdx_device_info_mrconfigid_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MRCONFIGID);
}

static inline bool ctlv_attestation_tdx_device_info_mrconfigid_get(const struct ctlv_hdr *record,
								   const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MRCONFIGID);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* MROWNER of the TD, the ID of its owner. */
static inline bool ctlv_attestation_tdx_device_info_mrowner_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNER);
}

static inline bool ctlv_attestation_tdx_device_info_mrowner_get(const struct ctlv_hdr *record,
								const void **value, __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNER);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

/* MROWNERCONFIG of the TD, the ID of its owner's configuration. */
static inline bool
ctlv_attestation_tdx_device_info_mrownerconfig_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNERCONFIG);
}

static inline bool ctlv_attestation_tdx_device_info_mrownerconfig_get(const struct ctlv_hdr *record,
								      const void **value,
								      __u32 *len)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_ATTESTATION_TDX_T_DEVICE_INFO_MROWNERCONFIG);
	return hdr && !ctlv_get_bin(hdr, value, len);
}

#endif /* _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H */
