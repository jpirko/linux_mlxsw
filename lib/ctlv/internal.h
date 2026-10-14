/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV core internals.
 */
#ifndef _CTLV_INTERNAL_H
#define _CTLV_INTERNAL_H

#include <linux/bug.h>
#include <linux/ctlv_family.h>
#include <linux/list.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>

#include "message.h"

/* Without CONFIG_CTLV_DEBUG @cond is not evaluated, so nothing may rely on it. */
#ifdef CONFIG_CTLV_DEBUG
#define CTLV_ASSERT(cond) ((void)WARN_ON_ONCE(!(cond)))
#else
#define CTLV_ASSERT(cond) BUILD_BUG_ON_INVALID(cond)
#endif

struct kobj_uevent_env;
struct task_struct;

#define CTLV_ERROR_MESSAGE_LEN 256

/* An op that is always enabled has no per-device availability state. */
#define CTLV_OP_STATE_NONE U32_MAX

struct ctlv_op_entry {
	const struct ctlv_op_schema *schema;
	const struct ctlv_op *binding;
	struct ctlv_op_sizes sizes;
	u32 state_index;
	bool absent;
};

struct ctlv_error {
	u32 reason;
	char message[CTLV_ERROR_MESSAGE_LEN];
	u32 bad_tlv_offset;
	u32 record_offset;
	u64 missing_type;
	u64 expected_generation;
	u64 current_generation;
	u64 required_length;
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
	struct ctlv *ctlv;
	const struct ctlv_family *family;
	/* Compared against current and never followed: identity, not a hold. */
	const struct task_struct *submitter;
	const struct ctlv_op_entry *op;

	struct ctlv_msg request;
	struct ctlv_cursor request_root;

	struct ctlv_msg output;
	struct ctlv_cursor output_root;
	struct ctlv_cursor *deepest;

	struct ctlv_query_fragment frag;
	struct list_head blobs;
	bool blobs_live;
	int blob_error;
	struct ctlv_error error;
};

struct ctlv_op_entry *ctlv_op_find(struct ctlv *ctlv, u64 op_id);
int ctlv_uevent(const struct ctlv *ctlv, struct kobj_uevent_env *env);

#endif /* _CTLV_INTERNAL_H */
