/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Private blob validation and resolution.
 */
#ifndef _CTLV_BLOB_H
#define _CTLV_BLOB_H

#include <linux/ctlv_family.h>
#include <linux/fs.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

void ctlv_blob_context_init(struct ctlv_op_ctx *ctx);
void ctlv_blob_context_track(struct ctlv_blob *blob, struct ctlv_op_ctx *ctx);
int ctlv_blob_context_finish(struct ctlv_op_ctx *ctx);
int ctlv_blob_memfd_access(u8 direction, fmode_t mode, int seals);

struct dma_buf;

/* The dma-buf half of an acquisition, reached without an fd by KUnit. */
struct ctlv_blob *ctlv_blob_dma_buf_acquire(struct dma_buf *dmabuf, u64 offset,
					    u64 len, u8 direction);

/* The pages an address range comes to, reached the same way and for KUnit. */
int ctlv_blob_va_range(u64 address, u64 len, unsigned long *first_page,
		       unsigned long *pages_count);

#endif /* _CTLV_BLOB_H */
