/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_BLOB_DESC_H
#define _CTLV_BLOB_DESC_H

#include <linux/ctlv_family.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>

/* The members of a descriptor, in the order the root family declares them. */
enum {
	CTLV_BLOB_MEMBER_TYPE = 0,
	CTLV_BLOB_MEMBER_ADDRESS = 1,
	CTLV_BLOB_MEMBER_FD = 2,
	CTLV_BLOB_MEMBER_OFFSET = 3,
	CTLV_BLOB_MEMBER_LENGTH = 4,
	CTLV_BLOB_MEMBERS = 5,
};

struct ctlv_blob_desc {
	u64 member[CTLV_BLOB_MEMBERS];
	u32 present;
};

u32 ctlv_blob_desc_read(const struct ctlv_hdr *blob, struct ctlv_blob_desc *desc,
			const struct ctlv_hdr **bad);
int ctlv_blob_desc_check(const struct ctlv_blob_desc *desc, const struct ctlv_node_schema *node);

#endif /* _CTLV_BLOB_DESC_H */
