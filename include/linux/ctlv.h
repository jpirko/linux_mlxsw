/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family API.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/device.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/spinlock_types.h>
#include <linux/types.h>
#include <linux/wait.h>

#include <uapi/ctlv/ctlv.h>

struct ctlv_family;
struct ctlv_file;
struct ctlv;
struct ctlv_msg;
struct ctlv_op_ctx;
struct ctlv_op_entry;
struct ctlv_schema_tree;
struct file;

/**
 * struct ctlv_cursor - One TLV position in one message.
 * @msg: the message this position is in, core-private bookkeeping
 * @hdr: the TLV itself, or NULL when a lookup found nothing
 * @parent: reading, the record containing @hdr, NULL at the root
 * @up: writing, cursor of the record containing @hdr, NULL at the root. The
 *	chain of them is the open-record stack, so a cursor must not outlive the
 *	cursor of its record
 * @start: writing, where @hdr begins in the message. An append reaches what
 *	it lengthens through the buffer rather than through @hdr, that buffer
 *	being the core's own
 * @tree: schema tree the node belongs to, which is a fill's own tree for a
 *	TLV that arrived through one
 * @node: index of the node within @tree
 * @depth: record levels above this position, the root being zero
 * @index_record_scope: schema scope used to read the members of @hdr, a record
 * @index_scope: core-private schema scope containing this TLV
 * @progress: writing, whether the fragment had progressed before this record
 *	opened
 *
 * Fixed-size, so family code can put a child cursor on the stack.
 * ctlv_record_start() opens at most CTLV_MAX_RECORD_DEPTH levels, which bounds
 * that stack. The members belong to the core: a family reaches a message
 * through what was generated for the op it answers. One direction at a time,
 * a helper of the other direction refusing the cursor.
 */
struct ctlv_cursor {
	struct ctlv_msg *msg;
	const struct ctlv_hdr *hdr;
	const struct ctlv_hdr *parent;
	struct ctlv_cursor *up;
	size_t start;
	const struct ctlv_schema_tree *tree;
	u32 node;
	u32 depth;
	u32 index_record_scope;
	u32 index_scope;
	bool progress;
};

/**
 * struct ctlv_hooks - What a device runs around the ops of its instance.
 * @pre: entered before every op the core does not answer, and may refuse it
 * @post: entered after every op whose @pre succeeded
 */
struct ctlv_hooks {
	int (*pre)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
	void (*post)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
};

/**
 * struct ctlv - The CTLV state of one device, embedded by its subsystem.
 * @dev: device whose release frees the structure embedding this instance;
 *	every open file pins it
 *
 * A family callback is handed the instance and reaches its own state with
 * container_of().
 */
struct ctlv {
	struct device *dev;
	/* private: */
	const struct ctlv_hooks *hooks;
	struct rw_semaphore registration_lock;
	bool registered;
	const struct ctlv_family *family;

	struct ctlv_op_entry *ops;
	u32 ops_count;

	/* Op publication and state writers, never the dispatch path. */
	spinlock_t transition_lock;
	bool published;
	bool destroying;
	u64 state_generation;
	u32 dynamic_ops_count;
	unsigned long *disabled;

	/* Everything about the queues, and nothing family code runs under. */
	spinlock_t events_lock;
	struct list_head files;
	bool dead_announced;
	wait_queue_head_t event_context_wait;
	u32 active_event_contexts;
	bool events_live;
	u32 events_count;
	const struct ctlv_op_entry *overflow_op;
	const struct ctlv_op_entry *removed_op;
	u32 queue_min;
	u32 queue_reserved;
	u32 queue_default;
};

int ctlv_init(struct ctlv *ctlv, struct device *dev,
	      const struct ctlv_family *family, const struct ctlv_hooks *hooks);
void ctlv_publish(struct ctlv *ctlv);
void ctlv_unpublish(struct ctlv *ctlv);
void ctlv_fini(struct ctlv *ctlv);
const struct ctlv_family *ctlv_family_of(const struct ctlv *ctlv);

bool ctlv_family_inherits(const struct ctlv_family *family,
			  const struct ctlv_family *ancestor);

struct ctlv_file *ctlv_file_open(struct ctlv *ctlv, struct file *filp);
long ctlv_file_ioctl(struct ctlv_file *cfile, unsigned int cmd,
		     unsigned long arg);
void ctlv_file_close(struct ctlv_file *cfile);

void ctlv_record_end(struct ctlv_cursor *child);
void ctlv_record_cancel(struct ctlv_cursor *child);

int ctlv_ext_fill(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		  struct ctlv_cursor *record);
void ctlv_ext_read(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *record);

int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message);
__printf(3, 4)
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...);

/*
 * The core retains no query state. A continuation repeats the request and
 * passes back the family's generation and nonzero cursor. It fails with
 * -ESTALE if the generation changed. Each fragment must complete a logical
 * leaf and return a cursor different from its input.
 *
 * ctlv_op_query_more() marks still-open records, whose cursors must stay live.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx);
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx);
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor);
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation);

struct ctlv_blob;
struct ctlv_blob_dma;
struct dma_fence;
struct sg_table;

int ctlv_blob_put(struct ctlv_blob *blob);
u64 ctlv_blob_len(const struct ctlv_blob *blob);
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len);
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len);
int ctlv_blob_too_short(struct ctlv_blob *blob, u64 len);

struct ctlv_blob_dma *ctlv_blob_dma_map(struct ctlv_blob *blob,
					struct device *dev,
					struct sg_table **sgt, u64 *dma_skip);
int ctlv_blob_dma_begin(struct ctlv_blob_dma *dma, struct dma_fence *fence);
void ctlv_blob_dma_submit_done(struct ctlv_blob_dma *dma, int status);
void ctlv_blob_dma_unmap(struct ctlv_blob_dma *dma, int status);

void ctlv_event_lost(struct ctlv *ctlv, u64 op_id);
int ctlv_event_emit(struct ctlv_op_ctx *ctx);
void ctlv_event_abort(struct ctlv_op_ctx *ctx);

#endif /* _LINUX_CTLV_H */
