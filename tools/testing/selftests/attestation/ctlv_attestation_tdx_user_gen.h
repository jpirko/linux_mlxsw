/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-attestation-tdx CTLV family. */

#ifndef _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H
#define _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H

#include <ctlv.h>

#include <linux/ctlv_attestation_tdx_gen.h>

#include "ctlv_attestation_user_gen.h"

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_attestation_tdx_spec;

/* Building a request. ctlv_msg_err() reports the first failure. */

static inline int ctlv_attestation_tdx_info_mrconfigid_put(struct ctlv_msg *msg, const void *value,
							   __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID, value, len);
}

static inline int ctlv_attestation_tdx_info_mrowner_put(struct ctlv_msg *msg, const void *value,
							__u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER, value, len);
}

static inline int ctlv_attestation_tdx_info_mrownerconfig_put(struct ctlv_msg *msg,
							      const void *value, __u32 len)
{
	return ctlv_put_bin(msg, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG, value, len);
}

/* Reading a reply or a record. A getter answers whether it could read. */

static inline bool ctlv_attestation_tdx_info_mrconfigid_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID);
}

static inline bool ctlv_attestation_tdx_info_mrconfigid_get(const struct ctlv_attr *nest,
							    const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool ctlv_attestation_tdx_info_mrowner_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER);
}

static inline bool ctlv_attestation_tdx_info_mrowner_get(const struct ctlv_attr *nest,
							 const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER);
	return attr && !ctlv_get_bin(attr, value, len);
}

static inline bool ctlv_attestation_tdx_info_mrownerconfig_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG);
}

static inline bool ctlv_attestation_tdx_info_mrownerconfig_get(const struct ctlv_attr *nest,
							       const void **value, __u32 *len)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG);
	return attr && !ctlv_get_bin(attr, value, len);
}

#endif /* _CTLV_CTLV_ATTESTATION_TDX_USER_GEN_H */
