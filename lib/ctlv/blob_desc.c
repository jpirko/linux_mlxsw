// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Blob descriptor parsing and validation.
 */
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/errno.h>
#include <linux/limits.h>
#include <linux/overflow.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "blob_desc.h"
#include "message.h"

static int ctlv_blob_member(u64 type)
{
	switch (type) {
	case CTLV_T_BLOB_TYPE:
		return CTLV_BLOB_MEMBER_TYPE;
	case CTLV_T_BLOB_ADDRESS:
		return CTLV_BLOB_MEMBER_ADDRESS;
	case CTLV_T_BLOB_FD:
		return CTLV_BLOB_MEMBER_FD;
	case CTLV_T_BLOB_OFFSET:
		return CTLV_BLOB_MEMBER_OFFSET;
	case CTLV_T_BLOB_LENGTH:
		return CTLV_BLOB_MEMBER_LENGTH;
	default:
		return -1;
	}
}

u32 ctlv_blob_desc_read(const struct ctlv_hdr *blob, struct ctlv_blob_desc *desc,
			const struct ctlv_hdr **bad)
{
	const struct ctlv_hdr *child = NULL;
	enum ctlv_message_step step;

	*desc = (struct ctlv_blob_desc){};
	while ((step = ctlv_message_next(blob, &child)) == CTLV_MESSAGE_STEP_TLV) {
		int member = ctlv_blob_member(child->type);

		*bad = child;
		if (child->flags & ~CTLV_HDR_F_RECORD)
			return CTLV_VALIDATION_REASON_UNKNOWN_FLAG;
		if (child->flags & CTLV_HDR_F_RECORD)
			return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		if (member < 0)
			return CTLV_VALIDATION_REASON_UNKNOWN_TYPE;
		if (ctlv_message_hdr_payload_len(child) != sizeof(u64))
			return CTLV_VALIDATION_REASON_BAD_LENGTH;
		if (desc->present & BIT(member))
			return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
		desc->present |= BIT(member);
		desc->member[member] = *(const u64 *)ctlv_message_hdr_payload(child);
	}
	*bad = NULL;
	if (step == CTLV_MESSAGE_STEP_BAD)
		return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
	return 0;
}

static bool ctlv_blob_variant_ok(const struct ctlv_blob_desc *desc)
{
	u32 required = BIT(CTLV_BLOB_MEMBER_TYPE) | BIT(CTLV_BLOB_MEMBER_LENGTH);
	u32 optional = 0;

	switch (desc->member[CTLV_BLOB_MEMBER_TYPE]) {
	case CTLV_BLOB_TYPE_VA:
		required |= BIT(CTLV_BLOB_MEMBER_ADDRESS);
		break;
	case CTLV_BLOB_TYPE_MEMFD:
		required |= BIT(CTLV_BLOB_MEMBER_FD);
		optional |= BIT(CTLV_BLOB_MEMBER_OFFSET);
		if (desc->member[CTLV_BLOB_MEMBER_FD] > INT_MAX)
			return false;
		break;
	default:
		return false;
	}
	return desc->present == required || desc->present == (required | optional);
}

/* Every failure is a bad descriptor; only a range that wraps is -EOVERFLOW. */
int ctlv_blob_desc_check(const struct ctlv_blob_desc *desc, const struct ctlv_node_schema *node)
{
	u64 start, length, end;

	if (!ctlv_blob_variant_ok(desc) ||
	    !(node->blob.backings & BIT(desc->member[CTLV_BLOB_MEMBER_TYPE])))
		return -EINVAL;

	length = desc->member[CTLV_BLOB_MEMBER_LENGTH];
	if (!length || length > node->blob.max_len)
		return -EINVAL;
	start = desc->member[desc->present & BIT(CTLV_BLOB_MEMBER_ADDRESS) ?
				     CTLV_BLOB_MEMBER_ADDRESS :
				     CTLV_BLOB_MEMBER_OFFSET];
	if (!IS_ALIGNED(start, node->blob.alignment))
		return -EINVAL;
	if (check_add_overflow(start, length, &end))
		return -EOVERFLOW;
	return 0;
}
