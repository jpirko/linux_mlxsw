/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_TDX_GUEST_H
#define _LINUX_TDX_GUEST_H

#include <crypto/sha2.h>
#include <linux/types.h>

struct device;

#define TDX_GUEST_REPORTDATA_LEN 64
#define TDX_GUEST_REPORT_LEN 1024
#define TDX_GUEST_QUOTE_MAX_LEN ((128 * 1024) - 24)

struct tdx_guest_info {
	u8 mrconfigid[SHA384_DIGEST_SIZE];
	u8 mrowner[SHA384_DIGEST_SIZE];
	u8 mrownerconfig[SHA384_DIGEST_SIZE];
};

enum tdx_guest_mr {
	TDX_GUEST_MR_MRTD,
	TDX_GUEST_MR_RTMR0,
	TDX_GUEST_MR_RTMR1,
	TDX_GUEST_MR_RTMR2,
	TDX_GUEST_MR_RTMR3,
};

struct tdx_guest_ops {
	const struct tdx_guest_info *(*info_get)(struct device *dev);
	int (*report_get)(struct device *dev, const u8 *reportdata,
			  size_t length, void *out, size_t capacity,
			  size_t *out_length);
	int (*quote_get)(struct device *dev, const u8 *reportdata,
			 size_t length, void *out, size_t capacity,
			 size_t *out_length);
	int (*mr_read)(struct device *dev, enum tdx_guest_mr mr,
		       u8 value[SHA384_DIGEST_SIZE]);
	int (*rtmr_extend)(struct device *dev, unsigned int rtmr,
			   const u8 value[SHA384_DIGEST_SIZE]);
};

struct tdx_guest_client {
	void *(*add)(struct device *parent_dev,
		     const struct tdx_guest_ops *ops);
	void (*remove)(void *client_priv);
};

int tdx_guest_client_register(const struct tdx_guest_client *client);
void tdx_guest_client_unregister(const struct tdx_guest_client *client);

#endif /* _LINUX_TDX_GUEST_H */
