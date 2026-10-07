// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Blob descriptor parsing and validation.
 */
#include <linux/align.h>
#include <linux/bits.h>
#include <linux/limits.h>
#include <linux/overflow.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "blob_desc.h"
#include "message.h"

static int ctlv_blob_member(u64 attr_id)
{
	switch (attr_id) {
	case CTLV_ATTR_BLOB_TYPE:
		return CTLV_BLOB_MEMBER_TYPE;
	case CTLV_ATTR_BLOB_ADDRESS:
		return CTLV_BLOB_MEMBER_ADDRESS;
	case CTLV_ATTR_BLOB_FD:
		return CTLV_BLOB_MEMBER_FD;
	case CTLV_ATTR_BLOB_OFFSET:
		return CTLV_BLOB_MEMBER_OFFSET;
	case CTLV_ATTR_BLOB_LENGTH:
		return CTLV_BLOB_MEMBER_LENGTH;
	default:
		return -1;
	}
}

u32 ctlv_blob_desc_read(const struct ctlv_attr *blob, struct ctlv_blob_desc *desc,
			const struct ctlv_attr **bad)
{
	const struct ctlv_attr *child = NULL;
	enum ctlv_message_step step;

	*desc = (struct ctlv_blob_desc){};
	while ((step = ctlv_message_next(blob, &child)) == CTLV_MESSAGE_STEP_ATTR) {
		int member = ctlv_blob_member(child->attr_id);

		*bad = child;
		if (child->flags & ~CTLV_ATTR_F_NESTED)
			return CTLV_VALIDATION_REASON_UNKNOWN_FLAG;
		if (child->flags & CTLV_ATTR_F_NESTED)
			return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		if (member < 0)
			return CTLV_VALIDATION_REASON_UNKNOWN_ATTR;
		if (ctlv_message_attr_payload_len(child) != sizeof(u64))
			return CTLV_VALIDATION_REASON_BAD_LENGTH;
		if (desc->present & BIT(member))
			return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
		desc->present |= BIT(member);
		desc->member[member] = *(const u64 *)ctlv_message_attr_payload(child);
	}
	*bad = NULL;
	if (step == CTLV_MESSAGE_STEP_BAD)
		return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
	return 0;
}

static u32 ctlv_blob_variant(const struct ctlv_blob_desc *desc)
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
			return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
		break;
	default:
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	}
	if (desc->present != required && desc->present != (required | optional))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	return 0;
}

u32 ctlv_blob_desc_check(const struct ctlv_blob_desc *desc, const struct ctlv_node_schema *node)
{
	u64 start, length, end;
	u32 reason;

	reason = ctlv_blob_variant(desc);
	if (reason)
		return reason;
	if (!(node->blob.backings & BIT(desc->member[CTLV_BLOB_MEMBER_TYPE])))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;

	length = desc->member[CTLV_BLOB_MEMBER_LENGTH];
	if (!length || length > node->blob.max_len)
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	start = desc->member[desc->present & BIT(CTLV_BLOB_MEMBER_ADDRESS) ?
				     CTLV_BLOB_MEMBER_ADDRESS :
				     CTLV_BLOB_MEMBER_OFFSET];
	if (!IS_ALIGNED(start, node->blob.alignment) || check_add_overflow(start, length, &end))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	return 0;
}

