/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and family API of the ctlv-attestation-tdx CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_TDX_GEN_H
#define _LINUX_CTLV_ATTESTATION_TDX_GEN_H

#include <linux/ctlv_family.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_attestation_tdx_gen.h>

#include "ctlv_attestation_gen.h"

extern const struct ctlv_family_schema ctlv_attestation_tdx_schema;

/* Family-wide hooks around every effective non-core op. */
int ctlv_attestation_tdx_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_attestation_tdx_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* Core-allocated family priv lifecycle. */
size_t ctlv_attestation_tdx_priv_size(void);
const void *ctlv_attestation_tdx_pre_init(const void *init_info);
int ctlv_attestation_tdx_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv);
void ctlv_attestation_tdx_fini(struct ctlv_device *ctlvdev, void *priv);

/* device-info of ctlv-attestation: fill callback. */
int ctlv_attestation_tdx_device_info_fill(void *priv, struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *request,
					  struct ctlv_cursor *nest);

extern const struct ctlv_family ctlv_attestation_tdx_family;

#ifndef _CTLV_GENERATED_METADATA_ONLY

/* Reading a validated message. A blob has a resolver instead of a getter. */
CTLV_IN_REF(ctlv_attestation_tdx_device_info_info_mrconfigid,
	    &ctlv_attestation_tdx_schema.stub_fill_schemas[0].members,
	    CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID, 0, 0, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_tdx_device_info_info_mrconfigid)
CTLV_IN_BINARY(ctlv_attestation_tdx_device_info_info_mrconfigid)
CTLV_IN_REF(ctlv_attestation_tdx_device_info_info_mrowner,
	    &ctlv_attestation_tdx_schema.stub_fill_schemas[0].members,
	    CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER, 0, 1, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_tdx_device_info_info_mrowner)
CTLV_IN_BINARY(ctlv_attestation_tdx_device_info_info_mrowner)
CTLV_IN_REF(ctlv_attestation_tdx_device_info_info_mrownerconfig,
	    &ctlv_attestation_tdx_schema.stub_fill_schemas[0].members,
	    CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG, 0, 2, CTLV_SCHEMA_BINARY)
CTLV_IN_HAS(ctlv_attestation_tdx_device_info_info_mrownerconfig)
CTLV_IN_BINARY(ctlv_attestation_tdx_device_info_info_mrownerconfig)

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */
CTLV_PUT_BINARY(ctlv_attestation_tdx_info_mrconfigid, CTLV_ATTESTATION_TDX_ATTR_INFO_MRCONFIGID,
		CTLV_SCHEMA_BINARY)
CTLV_PUT_BINARY(ctlv_attestation_tdx_info_mrowner, CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNER,
		CTLV_SCHEMA_BINARY)
CTLV_PUT_BINARY(ctlv_attestation_tdx_info_mrownerconfig,
		CTLV_ATTESTATION_TDX_ATTR_INFO_MROWNERCONFIG, CTLV_SCHEMA_BINARY)
#endif /* _CTLV_GENERATED_METADATA_ONLY */

#endif /* _LINUX_CTLV_ATTESTATION_TDX_GEN_H */
