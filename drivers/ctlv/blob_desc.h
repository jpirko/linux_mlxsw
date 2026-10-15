/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_BLOB_DESC_H
#define _CTLV_BLOB_DESC_H

#include <linux/ctlv_family.h>
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

struct ctlv_blob_desc {
	u64 member[CTLV_BLOB_MEMBERS];
	u32 present;
};

u32 ctlv_blob_desc_read(const struct ctlv_attr *blob, struct ctlv_blob_desc *desc,
			const struct ctlv_attr **bad);
u32 ctlv_blob_desc_check(const struct ctlv_blob_desc *desc, const struct ctlv_node_schema *node);

#endif /* _CTLV_BLOB_DESC_H */
