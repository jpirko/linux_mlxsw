/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV core internals.
 */
#ifndef _CTLV_INTERNAL_H
#define _CTLV_INTERNAL_H

#include <linux/ctlv_family.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

#include "message.h"

#define CTLV_ERROR_MESSAGE_LEN 256

/* An op that is always enabled has no per-device availability state. */
#define CTLV_OP_STATE_NONE U32_MAX

struct ctlv_op_entry {
	const struct ctlv_op_schema *schema;
	const struct ctlv_op *binding;
	void *priv;
	struct ctlv_op_sizes sizes;
	u32 state_index;
	bool absent;
};

struct ctlv_error {
	u32 reason;
	char message[CTLV_ERROR_MESSAGE_LEN];
	u32 bad_attr_offset;
	u32 nest_offset;
	u64 missing_attr_id;
	u64 expected_generation;
	u64 current_generation;
};

struct ctlv_query_fragment {
	u64 generation;
	u64 cursor;
	u64 reported;
	u64 next_cursor;
	bool progress;
	bool answered;
	int result;
};

struct ctlv_op_ctx {
	const struct ctlv_family *family;
	const struct ctlv_op_entry *op;

	struct ctlv_msg request;
	struct ctlv_cursor request_root;

	struct ctlv_msg output;
	struct ctlv_cursor output_root;
	struct ctlv_cursor *deepest;

	struct ctlv_query_fragment frag;
	struct ctlv_error error;
};

#endif /* _CTLV_INTERNAL_H */
