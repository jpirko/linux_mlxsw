/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family API.
 */
#ifndef _LINUX_CTLV_H
#define _LINUX_CTLV_H

#include <linux/types.h>

#include <uapi/linux/ctlv.h>

struct ctlv_device;
struct ctlv_family;
struct ctlv_msg;
struct ctlv_op_ctx;
struct ctlv_schema_tree;

/**
 * struct ctlv_cursor - One attr position in one message.
 * @msg: the message this position is in, core-private bookkeeping
 * @attr: the attr itself, or NULL when a lookup found nothing
 * @parent: reading, the nest containing @attr, NULL at the root
 * @up: writing, cursor of the nest containing @attr, NULL at the root. The
 *	chain of them is the open-nest stack
 * @start: writing, where @attr begins in the message. An append reaches what
 *	it lengthens through the buffer rather than through @attr, that buffer
 *	being the core's own
 * @tree: schema tree the node belongs to, which is a fill's own tree for an
 *	attr that arrived through one
 * @node: index of the node within @tree
 * @depth: nesting levels above this position, the root being zero
 * @index_nest_scope: schema scope used to read members of the physical nest
 * @index_scope: core-private schema scope containing this attr
 *
 * Fixed-size, so family code can put a child cursor on the stack. The members
 * belong to the core: a family reaches a message through what was generated
 * for the op it answers. One direction at a time, a helper of the other
 * direction refusing the cursor.
 */
struct ctlv_cursor {
	struct ctlv_msg *msg;
	const struct ctlv_attr *attr;
	const struct ctlv_attr *parent;
	struct ctlv_cursor *up;
	size_t start;
	const struct ctlv_schema_tree *tree;
	u32 node;
	u32 depth;
	u32 index_nest_scope;
	u32 index_scope;
};

/**
 * ctlv_priv_parent - Get the direct parent family private data.
 * @priv: core-allocated private data passed to a family callback
 *
 * Return: parent private data, or NULL for the root family.
 */
void *ctlv_priv_parent(const void *priv);

/**
 * ctlv_priv_child - Get the direct child family private data.
 * @priv: core-allocated private data passed to a family callback
 *
 * Return: child private data, or NULL for the device family.
 */
void *ctlv_priv_child(const void *priv);

struct device;

/**
 * ctlv_device_family - Get the device family.
 * @ctlvdev: CTLV device
 *
 * Return: family used to create the device.
 */
const struct ctlv_family *
ctlv_device_family(const struct ctlv_device *ctlvdev);

/**
 * ctlv_device_parent - Get the backing device.
 * @ctlvdev: CTLV device
 *
 * Return: backing device passed to ctlv_create().
 */
struct device *ctlv_device_parent(const struct ctlv_device *ctlvdev);

/**
 * ctlv_create - Create and register a CTLV device.
 * @parent_dev: backing device
 * @family: device family
 * @init_info: family initialization data
 *
 * Return: CTLV device, or an error pointer.
 */
struct ctlv_device *ctlv_create(struct device *parent_dev,
				const struct ctlv_family *family,
				const void *init_info);

/**
 * ctlv_destroy - Unregister and destroy a CTLV device.
 * @ctlvdev: CTLV device
 */
void ctlv_destroy(struct ctlv_device *ctlvdev);

/**
 * ctlv_nest_end - Finish an output nest.
 * @child: nest cursor returned by a generated start helper
 */
void ctlv_nest_end(struct ctlv_cursor *child);

/**
 * ctlv_stub_fill - Delegate an extension-point output nest.
 * @ctx: operation context
 * @request: validated request root
 * @nest: output nest selected by the generated helper
 *
 * Return: filling callback result, or 0 for an unfilled point.
 */
int ctlv_stub_fill(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		   struct ctlv_cursor *nest);

/**
 * ctlv_stub_read - Pass an extension-point input nest to its filling family.
 * @ctx: operation context
 * @nest: validated extension-point nest
 */
void ctlv_stub_read(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *nest);

/**
 * ctlv_op_error - Record an operation error message.
 * @ctx: operation context
 * @error: negative error to return
 * @message: message copied into the operation result
 *
 * Return: @error, or -EINVAL if @error is invalid.
 */
int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message);

/**
 * ctlv_op_errorf - Format an operation error message.
 * @ctx: operation context
 * @error: negative error to return
 * @fmt: printf format and arguments
 *
 * Return: @error, or -EINVAL if @error is invalid.
 */
__printf(3, 4)
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...);

/*
 * The core retains no query state. A continuation repeats the request and
 * passes back the family's generation and nonzero cursor. It fails with
 * -ESTALE if the generation changed. Each fragment must complete a logical
 * leaf and return a cursor different from its input.
 *
 * ctlv_op_query_more() marks still-open nests, whose cursors must remain live.
 */
/**
 * ctlv_op_query_generation - Get a query continuation generation.
 * @ctx: query operation context
 *
 * Return: generation supplied by the previous fragment, or zero initially.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx);

/**
 * ctlv_op_query_cursor - Get a query continuation cursor.
 * @ctx: query operation context
 *
 * Return: cursor supplied by the previous fragment, or zero initially.
 */
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx);

/**
 * ctlv_op_query_more - Complete a query fragment with more data pending.
 * @ctx: query operation context
 * @generation: current data generation
 * @next_cursor: nonzero cursor for the next fragment
 *
 * Return: 0, or a negative error.
 */
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor);

/**
 * ctlv_op_query_done - Complete the last query fragment.
 * @ctx: query operation context
 * @generation: current data generation
 *
 * Return: 0, or a negative error.
 */
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation);

struct ctlv_blob;
struct ctlv_blob_dma;
struct dma_fence;
struct sg_table;

/**
 * ctlv_blob_put - Release a resolved blob.
 * @blob: blob returned by a generated resolve helper
 *
 * Return: 0, or a negative error.
 */
int ctlv_blob_put(struct ctlv_blob *blob);

/**
 * ctlv_blob_len - Get a resolved blob's length.
 * @blob: resolved blob
 *
 * Return: blob length in bytes.
 */
u64 ctlv_blob_len(const struct ctlv_blob *blob);

/**
 * ctlv_blob_read - Copy bytes from a resolved input blob.
 * @blob: resolved blob
 * @offset: byte offset
 * @dst: destination buffer
 * @len: number of bytes
 *
 * Return: 0, or a negative error.
 */
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len);

/**
 * ctlv_blob_write - Copy bytes to a resolved output blob.
 * @blob: resolved blob
 * @offset: byte offset
 * @src: source buffer
 * @len: number of bytes
 *
 * Return: 0, or a negative error.
 */
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len);

/**
 * ctlv_blob_dma_map - Map a resolved dma-buf for a device.
 * @blob: resolved dma-buf blob
 * @dev: DMA device
 * @sgt: returned scatterlist
 * @dma_skip: returned offset into @sgt
 *
 * Return: DMA mapping, or an error pointer.
 */
struct ctlv_blob_dma *ctlv_blob_dma_map(struct ctlv_blob *blob,
					struct device *dev,
					struct sg_table **sgt, u64 *dma_skip);

/**
 * ctlv_blob_dma_begin - Attach a completion fence to a DMA mapping.
 * @dma: DMA mapping
 * @fence: completion fence
 *
 * Return: 0, or a negative error.
 */
int ctlv_blob_dma_begin(struct ctlv_blob_dma *dma, struct dma_fence *fence);

/**
 * ctlv_blob_dma_submit_done - Report DMA submission completion.
 * @dma: DMA mapping
 * @status: submission status
 */
void ctlv_blob_dma_submit_done(struct ctlv_blob_dma *dma, int status);

/**
 * ctlv_blob_dma_unmap - Finish and release a DMA mapping.
 * @dma: DMA mapping
 * @status: operation status
 */
void ctlv_blob_dma_unmap(struct ctlv_blob_dma *dma, int status);

/**
 * ctlv_event_lost - Report an event that could not be created.
 * @ctlvdev: CTLV device
 * @op_id: event operation ID
 *
 * This does not allocate or sleep. It is not for interrupt context.
 */
void ctlv_event_lost(struct ctlv_device *ctlvdev, u64 op_id);

/**
 * ctlv_event_emit - Validate and publish a completed event.
 * @ctx: context returned by a generated event create helper
 *
 * This consumes @ctx. It is not for interrupt context.
 *
 * Return: 0, or a negative error.
 */
int ctlv_event_emit(struct ctlv_op_ctx *ctx);

/**
 * ctlv_event_abort - Discard an event under construction.
 * @ctx: context returned by a generated event create helper
 *
 * This consumes @ctx. It is not for interrupt context.
 */
void ctlv_event_abort(struct ctlv_op_ctx *ctx);

#endif /* _LINUX_CTLV_H */
