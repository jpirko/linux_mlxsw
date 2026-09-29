/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _TDX_GUEST_INTERNAL_H
#define _TDX_GUEST_INTERNAL_H

#include <crypto/sha2.h>
#include <linux/types.h>

struct device;

#define TDX_GUEST_QUOTE_MAX_LEN ((128 * 1024) - 24)

struct tdx_guest_info {
	u8 mrconfigid[SHA384_DIGEST_SIZE];
	u8 mrowner[SHA384_DIGEST_SIZE];
	u8 mrownerconfig[SHA384_DIGEST_SIZE];
};

enum tdx_guest_mr {
	TDX_GUEST_MR_MRTD = 0,
	TDX_GUEST_MR_RTMR0 = 1,
	TDX_GUEST_MR_RTMR1 = 2,
	TDX_GUEST_MR_RTMR2 = 3,
	TDX_GUEST_MR_RTMR3 = 4,
};

/* @reportdata is TDX_REPORTDATA_LEN bytes, and @out holds TDX_GUEST_QUOTE_MAX_LEN. */
int tdx_guest_quote_get(const u8 *reportdata, void *out, size_t *out_length);
int tdx_guest_mr_read(enum tdx_guest_mr mr, u8 value[SHA384_DIGEST_SIZE]);
int tdx_guest_rtmr_extend(unsigned int rtmr,
			  const u8 value[SHA384_DIGEST_SIZE]);

#if IS_ENABLED(CONFIG_ATTESTATION)
int tdx_attestation_register(struct device *parent,
			     const struct tdx_guest_info *info);
void tdx_attestation_unregister(void);
#else
static inline int tdx_attestation_register(struct device *parent,
					   const struct tdx_guest_info *info)
{
	return 0;
}

static inline void tdx_attestation_unregister(void)
{
}
#endif

#endif /* _TDX_GUEST_INTERNAL_H */
