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

/**
 * struct tdx_guest_ops - Native TDX guest operations
 * @info_get: Return immutable TD construction fields
 * @report_get: Get a TDREPORT bound to report data
 * @quote_get: Get a Quote bound to report data
 * @mr_read: Read MRTD or an RTMR
 * @rtmr_extend: Extend an RTMR
 */
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

/**
 * struct tdx_guest_client - Native TDX service consumer
 * @add: Attach to the service and return client private data or an ERR_PTR
 * @remove: Detach the client private data returned by @add
 */
struct tdx_guest_client {
	void *(*add)(struct device *parent_dev,
		     const struct tdx_guest_ops *ops);
	void (*remove)(void *client_priv);
};

/**
 * tdx_guest_client_register - Register one native TDX service consumer
 * @client: Immutable add/remove callback pair
 *
 * Return: 0, or a negative errno.
 */
int tdx_guest_client_register(const struct tdx_guest_client *client);

/**
 * tdx_guest_client_unregister - Unregister a native TDX service consumer
 * @client: Descriptor passed to tdx_guest_client_register()
 */
void tdx_guest_client_unregister(const struct tdx_guest_client *client);

#endif /* _LINUX_TDX_GUEST_H */
