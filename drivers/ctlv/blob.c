// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Blob backing is acquired lazily in the submitting task and held until put.
 */
#include <linux/align.h>
#include <linux/atomic.h>
#include <linux/bits.h>
#include <linux/capability.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/highmem.h>
#include <linux/limits.h>
#include <linux/minmax.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/overflow.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/sched/signal.h>
#include <linux/slab.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "blob.h"
#include "blob_desc.h"
#include "internal.h"
#include "message.h"

struct ctlv_blob {
	u64 len;
	u8 direction;
	u8 type;
	struct ctlv_op_ctx *ctx;
	struct list_head entry;
	union {
		struct {
			struct mm_struct *mm;
			struct page **pages;
			unsigned long pages_count;
			u32 start;
		} va;
	};
};

static bool ctlv_blob_readable(const struct ctlv_blob *blob)
{
	return blob->direction != CTLV_BLOB_DIRECTION_OUTPUT;
}

static bool ctlv_blob_writable(const struct ctlv_blob *blob)
{
	return blob->direction != CTLV_BLOB_DIRECTION_INPUT;
}

static bool ctlv_blob_same_task(const struct ctlv_blob *blob)
{
	return !blob->ctx || !blob->ctx->submitter || blob->ctx->submitter == current;
}

static bool ctlv_blob_access_live(const struct ctlv_blob *blob)
{
	return ctlv_blob_same_task(blob) && (!blob->ctx || blob->ctx->blobs_live);
}

/*
 * Charged against RLIMIT_MEMLOCK of the address space the pages came from. A
 * pin lasts one op, which is why it is not FOLL_LONGTERM.
 */
static int ctlv_blob_pin_charge(struct mm_struct *mm, unsigned long pages_count)
{
	unsigned long limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

	if (atomic64_add_return(pages_count, &mm->pinned_vm) > limit && !capable(CAP_IPC_LOCK)) {
		atomic64_sub(pages_count, &mm->pinned_vm);
		return -ENOMEM;
	}
	return 0;
}

static int ctlv_blob_va_range(u64 address, u64 len, unsigned long *first_page,
			      unsigned long *pages_count)
{
	unsigned long first, last, span, count;
	size_t bytes;
	u64 end;

	if (!len)
		return -EINVAL;
	if (check_add_overflow(address, len, &end) || end > TASK_SIZE)
		return -EFAULT;

	/* Bounded by an unsigned long above, so nothing below loses a bit. */
	first = (unsigned long)address & PAGE_MASK;
	last = ((unsigned long)end - 1) & PAGE_MASK;
	if (check_sub_overflow(last, first, &span) ||
	    check_add_overflow(span >> PAGE_SHIFT, 1UL, &count) ||
	    check_mul_overflow(count, sizeof(struct page *), &bytes))
		return -EFAULT;

	*first_page = first;
	*pages_count = count;
	return 0;
}

static int ctlv_blob_va_acquire(struct ctlv_blob *blob, u64 address)
{
	unsigned long pinned = 0, first, pages_count;
	unsigned int gup_flags = 0;
	struct page **pages;
	int ret;

	/* Before charging or allocating, both of which this sizes. */
	ret = ctlv_blob_va_range(address, blob->len, &first, &pages_count);
	if (ret)
		return ret;

	pages = kvmalloc_array(pages_count, sizeof(*pages), GFP_KERNEL_ACCOUNT);
	if (!pages)
		return -ENOMEM;

	if (ctlv_blob_writable(blob))
		gup_flags |= FOLL_WRITE;
	ret = ctlv_blob_pin_charge(current->mm, pages_count);
	if (ret)
		goto err_free_pages;

	while (pinned < pages_count) {
		long got = pin_user_pages_fast(first + (pinned << PAGE_SHIFT),
					       min_t(unsigned long, pages_count - pinned, INT_MAX),
					       gup_flags, pages + pinned);

		/* A short pin is progress; nothing pinned is the caller's. */
		if (got <= 0) {
			ret = got ? (int)got : -EFAULT;
			goto err_unpin;
		}
		pinned += got;
	}

	/* Referenced, so the unpin is safe however far the submitter got. */
	mmgrab(current->mm);
	blob->va.start = offset_in_page(address);
	blob->va.mm = current->mm;
	blob->va.pages = pages;
	blob->va.pages_count = pages_count;
	return 0;

	/* The pins taken, and the whole charge, which covered every page. */
err_unpin:
	unpin_user_pages(pages, pinned);
	atomic64_sub(pages_count, &current->mm->pinned_vm);
err_free_pages:
	kvfree(pages);
	return ret;
}

static void ctlv_blob_va_release(struct ctlv_blob *blob)
{
	/* Dirtied, so that what the kernel wrote survives the unpin. */
	unpin_user_pages_dirty_lock(blob->va.pages, blob->va.pages_count, ctlv_blob_writable(blob));
	atomic64_sub(blob->va.pages_count, &blob->va.mm->pinned_vm);
	kvfree(blob->va.pages);
	mmdrop(blob->va.mm);
}

static void ctlv_blob_va_copy(struct ctlv_blob *blob, u64 offset, void *buf, size_t len, bool write)
{
	u64 at = blob->va.start + offset;
	unsigned long index = at >> PAGE_SHIFT;
	u32 in_page = offset_in_page(at);
	u8 *bytes = buf;

	while (len) {
		struct page *page = blob->va.pages[index++];
		size_t step = min_t(size_t, len, PAGE_SIZE - in_page);

		if (write)
			memcpy_to_page(page, in_page, bytes, step);
		else
			memcpy_from_page(bytes, page, in_page, step);
		bytes += step;
		len -= step;
		in_page = 0;
	}
}

u64 ctlv_blob_len(const struct ctlv_blob *blob)
{
	return blob->len;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_len, "CTLV");

/*
 * The length, the direction and addressing from zero are all the family's own
 * to know, so getting one wrong is a kernel bug rather than a caller's.
 */
static bool ctlv_blob_range_ok(struct ctlv_blob *blob, u64 offset, size_t len, bool write)
{
	u64 end;

	if (WARN_ON_ONCE(write ? !ctlv_blob_writable(blob) : !ctlv_blob_readable(blob)))
		return false;
	if (WARN_ON_ONCE(!ctlv_blob_access_live(blob)))
		return false;
	if (WARN_ON_ONCE(check_add_overflow(offset, (u64)len, &end) || end > blob->len))
		return false;
	return true;
}

static int ctlv_blob_copy(struct ctlv_blob *blob, u64 offset, void *buf, size_t len, bool write)
{
	if (!ctlv_blob_range_ok(blob, offset, len, write))
		return -EINVAL;
	if (!len)
		return 0;

	switch (blob->type) {
	case CTLV_BLOB_TYPE_VA:
		ctlv_blob_va_copy(blob, offset, buf, len, write);
		return 0;
	default:
		return -EINVAL;
	}
}

int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len)
{
	return ctlv_blob_copy(blob, offset, dst, len, false);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_read, "CTLV");

int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src, size_t len)
{
	return ctlv_blob_copy(blob, offset, (void *)src, len, true);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_write, "CTLV");

int ctlv_blob_put(struct ctlv_blob *blob)
{
	if (!blob)
		return 0;
	if (WARN_ON_ONCE(!ctlv_blob_same_task(blob)))
		return -EINVAL;

	switch (blob->type) {
	case CTLV_BLOB_TYPE_VA:
		ctlv_blob_va_release(blob);
		break;
	}
	if (blob->ctx) {
		list_del(&blob->entry);
		blob->ctx = NULL;
	}
	kfree(blob);
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_put, "CTLV");

void ctlv_blob_context_init(struct ctlv_op_ctx *ctx)
{
	INIT_LIST_HEAD(&ctx->blobs);
	ctx->blobs_live = true;
	ctx->blob_error = 0;
}

void ctlv_blob_context_track(struct ctlv_blob *blob, struct ctlv_op_ctx *ctx)
{
	blob->ctx = ctx;
	list_add_tail(&blob->entry, &ctx->blobs);
}

int ctlv_blob_context_finish(struct ctlv_op_ctx *ctx)
{
	ctx->blobs_live = false;
	if (WARN_ON_ONCE(!list_empty(&ctx->blobs)) && !ctx->blob_error)
		ctx->blob_error = -EINVAL;
	while (!list_empty(&ctx->blobs)) {
		struct ctlv_blob *blob = list_first_entry(&ctx->blobs, struct ctlv_blob, entry);

		ctlv_blob_put(blob);
	}
	return ctx->blob_error;
}

/*
 * Which of an op's blobs was the unusable one, in the shape validation would
 * have used. The errno the family returns says what went wrong.
 */
static void ctlv_blob_context_fail(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *cursor)
{
	if (ctx->error.reason)
		return;
	ctx->error = (struct ctlv_error){
		.reason = CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR,
		.bad_attr_offset = ctlv_message_offset(cursor->msg, cursor->attr),
		.nest_offset = ctlv_message_offset(cursor->msg, cursor->parent),
	};
}

static struct ctlv_blob *ctlv_blob_acquire(const struct ctlv_blob_desc *desc, u8 direction)
{
	u64 address = desc->member[CTLV_BLOB_MEMBER_ADDRESS];
	u64 len = desc->member[CTLV_BLOB_MEMBER_LENGTH];
	struct ctlv_blob *blob;
	int ret;

	blob = kzalloc_obj(*blob, GFP_KERNEL_ACCOUNT);
	if (!blob)
		return ERR_PTR(-ENOMEM);
	INIT_LIST_HEAD(&blob->entry);
	blob->len = len;
	blob->direction = direction;
	blob->type = desc->member[CTLV_BLOB_MEMBER_TYPE];

	ret = ctlv_blob_va_acquire(blob, address);
	if (ret) {
		kfree(blob);
		return ERR_PTR(ret);
	}
	return blob;
}

struct ctlv_blob *ctlv_blob_resolve(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *blob)
{
	const struct ctlv_attr *bad = NULL;
	const struct ctlv_node_schema *node;
	struct ctlv_blob_desc desc;
	struct ctlv_blob *acquired;

	/* An absent blob, or a cursor from some other execution entirely. */
	if (WARN_ON_ONCE(blob->msg != &ctx->request || !ctlv_attr_ok(blob)))
		return ERR_PTR(-EINVAL);
	node = &blob->tree->node_schemas[blob->node];
	if (WARN_ON_ONCE(node->kind != CTLV_SCHEMA_BLOB))
		return ERR_PTR(-EINVAL);
	if (WARN_ON_ONCE(!ctx->blobs_live || !ctx->submitter || ctx->submitter != current))
		return ERR_PTR(-EINVAL);
	/* Validated before the callback, so anything wrong with it is a bug. */
	if (WARN_ON_ONCE(ctlv_blob_desc_read(blob->attr, &desc, &bad) ||
			 ctlv_blob_desc_check(&desc, node)))
		return ERR_PTR(-EINVAL);

	acquired = ctlv_blob_acquire(&desc, node->blob.direction);
	if (IS_ERR(acquired))
		ctlv_blob_context_fail(ctx, blob);
	else
		ctlv_blob_context_track(acquired, ctx);
	return acquired;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_resolve, "CTLV");
