/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * One blob descriptor, read off the wire. A descriptor is a nest of
 * framework-owned attrs and the one shape the schema tree does not describe,
 * so validation and resolution both read it through here.
 */
#ifndef _CTLV_BLOB_H
#define _CTLV_BLOB_H

#include <linux/ctlv.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

/* The members of a descriptor, in the order the root family declares them. */
enum {
	CTLV_BLOB_MEMBER_TYPE,
	CTLV_BLOB_MEMBER_ADDRESS,
	CTLV_BLOB_MEMBER_FD,
	CTLV_BLOB_MEMBER_OFFSET,
	CTLV_BLOB_MEMBER_LENGTH,
	CTLV_BLOB_MEMBERS,
};

/**
 * struct ctlv_blob_desc - What one descriptor says.
 * @member: value of each member, zero for one the descriptor omits
 * @present: bit per member the descriptor carries
 */
struct ctlv_blob_desc {
	u64 member[CTLV_BLOB_MEMBERS];
	u32 present;
};

u32 ctlv_blob_desc_read(const struct ctlv_attr *blob,
			struct ctlv_blob_desc *desc,
			const struct ctlv_attr **bad);
u32 ctlv_blob_desc_check(const struct ctlv_blob_desc *desc,
			 const struct ctlv_schema_node *node);

struct dma_buf;

/* The dma-buf half of an acquisition, reached without an fd by KUnit. */
struct ctlv_blob *ctlv_blob_dma_buf_acquire(struct dma_buf *dmabuf, u64 offset,
					    u64 len, u8 direction);

/* The pages an address range comes to, reached the same way and for KUnit. */
int ctlv_blob_va_range(u64 address, u64 len, unsigned long *first_page,
		       unsigned long *n_pages);

#endif /* _CTLV_BLOB_H */
