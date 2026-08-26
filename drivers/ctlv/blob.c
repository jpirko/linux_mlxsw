// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Blobs: memory a request describes instead of carrying, named by an address
 * or by an fd and an offset.
 *
 * Two rules hold throughout. Acquisition happens in the submitting task, the
 * only one those numbers mean anything in, and only when a callback asks. What
 * was acquired stays acquired until the family releases it, so closing the fd
 * or unmapping the address cannot move the ground under a running op.
 */
#include <kunit/visibility.h>
#include <linux/align.h>
#include <linux/atomic.h>
#include <linux/bits.h>
#include <linux/capability.h>
#include <linux/ctlv.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/dma-resv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/highmem.h>
#include <linux/iosys-map.h>
#include <linux/limits.h>
#include <linux/memfd.h>
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
#include <uapi/linux/fcntl.h>

#include "blob.h"
#include "ctlv_private.h"
#include "message.h"

/* Which descriptor member an attr ID names, or -1 for none. */
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

/**
 * ctlv_blob_desc_read - Read one descriptor out of the message carrying it.
 * @blob: the descriptor nest
 * @desc: filled in with what it says
 * @bad: set to the offending child, where there is one
 *
 * Rules that depend on the declared type are ctlv_blob_desc_check()'s.
 *
 * Return: zero, or the enum ctlv_validation_reason to refuse it with.
 */
u32 ctlv_blob_desc_read(const struct ctlv_attr *blob,
			struct ctlv_blob_desc *desc,
			const struct ctlv_attr **bad)
{
	const struct ctlv_attr *child = NULL;
	enum ctlv_step step;

	*desc = (struct ctlv_blob_desc){};
	while ((step = ctlv_msg_step(blob, &child)) == CTLV_STEP_ATTR) {
		int member = ctlv_blob_member(child->attr_id);

		*bad = child;
		if (child->flags & ~CTLV_ATTR_F_NESTED)
			return CTLV_VALIDATION_REASON_UNKNOWN_FLAG;
		if (child->flags & CTLV_ATTR_F_NESTED)
			return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		if (member < 0)
			return CTLV_VALIDATION_REASON_UNKNOWN_ATTR;
		if (ctlv_attr_payload_len(child) != sizeof(u64))
			return CTLV_VALIDATION_REASON_BAD_LENGTH;
		/* Each member occurs once. */
		if (desc->present & BIT(member))
			return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
		desc->present |= BIT(member);
		/* Aligned by the wire format: the root began an allocation. */
		desc->member[member] = *(const u64 *)ctlv_attr_payload(child);
	}
	*bad = NULL;
	if (step == CTLV_STEP_BAD)
		return CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
	return 0;
}

/* The members of one variant: what it must carry, and what it may. */
static u32 ctlv_blob_variant(const struct ctlv_blob_desc *desc)
{
	u32 required = BIT(CTLV_BLOB_MEMBER_TYPE) |
		       BIT(CTLV_BLOB_MEMBER_LENGTH);
	u32 optional = 0;

	switch (desc->member[CTLV_BLOB_MEMBER_TYPE]) {
	case CTLV_BLOB_TYPE_VA:
		required |= BIT(CTLV_BLOB_MEMBER_ADDRESS);
		break;
	case CTLV_BLOB_TYPE_MEMFD:
	case CTLV_BLOB_TYPE_DMA_BUF:
		required |= BIT(CTLV_BLOB_MEMBER_FD);
		optional |= BIT(CTLV_BLOB_MEMBER_OFFSET);
		if (desc->member[CTLV_BLOB_MEMBER_FD] > INT_MAX)
			return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
		break;
	default:
		/* Including the type reserved for a pre-imported blob. */
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	}
	if (desc->present != required &&
	    desc->present != (required | optional))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	return 0;
}

/**
 * ctlv_blob_desc_check - Whether one attr allows what one descriptor says.
 * @desc: what the descriptor says
 * @node: schema node of the blob attr that carries it
 *
 * The descriptor against the declaration, before any family code runs. Nothing
 * is acquired and nothing is asked to exist.
 *
 * Return: zero, or the enum ctlv_validation_reason it is refused with.
 */
u32 ctlv_blob_desc_check(const struct ctlv_blob_desc *desc,
			 const struct ctlv_schema_node *node)
{
	u64 start, length, end;
	u32 reason;

	reason = ctlv_blob_variant(desc);
	if (reason)
		return reason;

	if (!(node->blob.backings &
	      BIT(desc->member[CTLV_BLOB_MEMBER_TYPE])))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;

	length = desc->member[CTLV_BLOB_MEMBER_LENGTH];
	if (!length || length > node->blob.max_len)
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	/* Which member begins the range is what the variant settled. */
	start = desc->member[desc->present & BIT(CTLV_BLOB_MEMBER_ADDRESS) ?
			     CTLV_BLOB_MEMBER_ADDRESS :
			     CTLV_BLOB_MEMBER_OFFSET];
	if (!IS_ALIGNED(start, node->blob.alignment) ||
	    check_add_overflow(start, length, &end))
		return CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR;
	return 0;
}

/**
 * struct ctlv_blob - What one resolver acquired.
 * @len: bytes of it, as the descriptor declared
 * @direction: enum ctlv_blob_direction, which copies the attr allows
 * @type: enum ctlv_blob_type, selecting which member below is live
 * @va: pages pinned out of the submitter's address space, @start bytes into
 *	the first of them, and the mm they were charged to
 * @memfd: the memory file, referenced, and where in it the blob begins
 * @dma_buf: the buffer, referenced and mapped for CPU access in @dir
 *
 * A family addresses a blob from zero; the offset into the backing is ours.
 */
struct ctlv_blob {
	u64 len;
	u8 direction;
	u8 type;
	union {
		struct {
			struct mm_struct *mm;
			struct page **pages;
			unsigned long n_pages;
			u32 start;
		} va;
		struct {
			struct file *file;
			loff_t offset;
		} memfd;
		struct {
			struct dma_buf *dmabuf;
			struct iosys_map map;
			u64 offset;
			enum dma_data_direction dir;
		} dma_buf;
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

/*
 * Charged against RLIMIT_MEMLOCK of the address space the pages came from. A
 * pin lasts one op, which is why it is not FOLL_LONGTERM.
 */
static int ctlv_blob_pin_charge(struct mm_struct *mm, unsigned long n_pages)
{
	unsigned long limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

	if (atomic64_add_return(n_pages, &mm->pinned_vm) > limit &&
	    !capable(CAP_IPC_LOCK)) {
		atomic64_sub(n_pages, &mm->pinned_vm);
		return -ENOMEM;
	}
	return 0;
}

/**
 * ctlv_blob_va_range - The pages one address range covers.
 * @address: first byte of it, as the descriptor said
 * @len: bytes of it, as the descriptor said
 * @first_page: filled in with where the first page begins
 * @n_pages: filled in with the page count, at least one
 *
 * An address is a u64 on the wire whatever the caller's word size, so refuse
 * an impossible range here, before PAGE_MASK drops its top half or an end
 * rounded up to a page wraps to a count of zero. TASK_SIZE is current's
 * because a descriptor is resolved in the task that sent it.
 *
 * Return: zero, -EFAULT for a range no task could have, -EINVAL for no length.
 */
int ctlv_blob_va_range(u64 address, u64 len, unsigned long *first_page,
		       unsigned long *n_pages)
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
	*n_pages = count;
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blob_va_range);

static int ctlv_blob_va_acquire(struct ctlv_blob *blob, u64 address)
{
	unsigned long pinned = 0, first, n_pages;
	unsigned int gup_flags = 0;
	struct page **pages;
	int ret;

	/* Before charging or allocating, both of which this sizes. */
	ret = ctlv_blob_va_range(address, blob->len, &first, &n_pages);
	if (ret)
		return ret;

	pages = kvmalloc_array(n_pages, sizeof(*pages), GFP_KERNEL_ACCOUNT);
	if (!pages)
		return -ENOMEM;

	if (ctlv_blob_writable(blob))
		gup_flags |= FOLL_WRITE;
	ret = ctlv_blob_pin_charge(current->mm, n_pages);
	if (ret)
		goto err_free_pages;

	while (pinned < n_pages) {
		long got = pin_user_pages_fast(first + (pinned << PAGE_SHIFT),
					       min_t(unsigned long,
						     n_pages - pinned, INT_MAX),
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
	blob->va.n_pages = n_pages;
	return 0;

	/* The pins taken, and the whole charge, which covered every page. */
err_unpin:
	unpin_user_pages(pages, pinned);
	atomic64_sub(n_pages, &current->mm->pinned_vm);
err_free_pages:
	kvfree(pages);
	return ret;
}

static void ctlv_blob_va_release(struct ctlv_blob *blob)
{
	/* Dirtied, so that what the kernel wrote survives the unpin. */
	unpin_user_pages_dirty_lock(blob->va.pages, blob->va.n_pages,
				    ctlv_blob_writable(blob));
	atomic64_sub(blob->va.n_pages, &blob->va.mm->pinned_vm);
	kvfree(blob->va.pages);
	mmdrop(blob->va.mm);
}

static void ctlv_blob_va_copy(struct ctlv_blob *blob, u64 offset, void *buf,
			      size_t len, bool write)
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

static int ctlv_blob_memfd_acquire(struct ctlv_blob *blob, int fd, u64 offset)
{
	struct file *file = fget(fd);
	int seals;

	if (!file)
		return -EBADF;
	/*
	 * Having seals at all is what says memfd; F_SEAL_SHRINK says the range
	 * cannot be truncated away underneath us. Seals are permanent, so
	 * reading them needs nothing held. Writing is left unsealed either way.
	 */
	seals = memfd_get_seals(file);
	if (seals < 0 || !(seals & F_SEAL_SHRINK) ||
	    offset + blob->len > i_size_read(file_inode(file))) {
		fput(file);
		return -EINVAL;
	}
	blob->memfd.file = file;
	blob->memfd.offset = offset;
	return 0;
}

static int ctlv_blob_memfd_copy(struct ctlv_blob *blob, u64 offset, void *buf,
				size_t len, bool write)
{
	loff_t pos = blob->memfd.offset + offset;
	u8 *bytes = buf;

	while (len) {
		ssize_t done;

		if (write)
			done = kernel_write(blob->memfd.file, bytes, len, &pos);
		else
			done = kernel_read(blob->memfd.file, bytes, len, &pos);
		if (done <= 0)
			return done ? done : -EIO;
		bytes += done;
		len -= done;
	}
	return 0;
}

/**
 * ctlv_blob_dma_buf_acquire - Take one buffer for the length of one op.
 * @dmabuf: the buffer, whose reference this takes over
 * @offset: where in it the blob begins
 * @len: bytes of it the blob is
 * @direction: enum ctlv_blob_direction the attr declared
 *
 * The importer is the CPU, so the buffer is mapped rather than attached to a
 * device: a blob only read waits for writers, one written waits for everyone.
 * The access ends before the resolver's caller returns, leaving nothing
 * asynchronous for a fence of ours to stand for.
 *
 * Return: the blob, or an ERR_PTR.
 */
struct ctlv_blob *ctlv_blob_dma_buf_acquire(struct dma_buf *dmabuf, u64 offset,
					    u64 len, u8 direction)
{
	struct ctlv_blob *blob;
	long remaining;
	int ret;

	blob = kzalloc_obj(*blob, GFP_KERNEL_ACCOUNT);
	if (!blob) {
		dma_buf_put(dmabuf);
		return ERR_PTR(-ENOMEM);
	}
	blob->len = len;
	blob->direction = direction;
	blob->type = CTLV_BLOB_TYPE_DMA_BUF;
	blob->dma_buf.dmabuf = dmabuf;
	blob->dma_buf.offset = offset;
	if (!ctlv_blob_writable(blob))
		blob->dma_buf.dir = DMA_FROM_DEVICE;
	else if (!ctlv_blob_readable(blob))
		blob->dma_buf.dir = DMA_TO_DEVICE;
	else
		blob->dma_buf.dir = DMA_BIDIRECTIONAL;

	ret = -EINVAL;
	if (offset + len > dmabuf->size)
		goto err_free_blob;

	remaining = dma_resv_wait_timeout(dmabuf->resv,
					  ctlv_blob_writable(blob) ?
					  DMA_RESV_USAGE_READ :
					  DMA_RESV_USAGE_WRITE,
					  true, MAX_SCHEDULE_TIMEOUT);
	if (remaining <= 0) {
		ret = remaining ? (int)remaining : -EBUSY;
		goto err_free_blob;
	}

	ret = dma_buf_begin_cpu_access(dmabuf, blob->dma_buf.dir);
	if (ret)
		goto err_free_blob;
	ret = dma_buf_vmap_unlocked(dmabuf, &blob->dma_buf.map);
	if (ret)
		goto err_end_access;
	return blob;

err_end_access:
	dma_buf_end_cpu_access(dmabuf, blob->dma_buf.dir);
err_free_blob:
	kfree(blob);
	dma_buf_put(dmabuf);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blob_dma_buf_acquire);

static void ctlv_blob_dma_buf_release(struct ctlv_blob *blob)
{
	dma_buf_vunmap_unlocked(blob->dma_buf.dmabuf, &blob->dma_buf.map);
	dma_buf_end_cpu_access(blob->dma_buf.dmabuf, blob->dma_buf.dir);
	dma_buf_put(blob->dma_buf.dmabuf);
}

static void ctlv_blob_dma_buf_copy(struct ctlv_blob *blob, u64 offset,
				   void *buf, size_t len, bool write)
{
	u64 at = blob->dma_buf.offset + offset;

	if (write)
		iosys_map_memcpy_to(&blob->dma_buf.map, at, buf, len);
	else
		iosys_map_memcpy_from(buf, &blob->dma_buf.map, at, len);
}

/**
 * ctlv_blob_len - How long the memory one blob stands for is.
 * @blob: the resolved blob
 *
 * Return: the declared length, the whole of what a copy may address.
 */
u64 ctlv_blob_len(const struct ctlv_blob *blob)
{
	return blob->len;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_len, "CTLV");

/*
 * The length, the direction and addressing from zero are all the family's own
 * to know, so getting one wrong is a kernel bug rather than a caller's.
 */
static bool ctlv_blob_range_ok(struct ctlv_blob *blob, u64 offset, size_t len,
			       bool write)
{
	u64 end;

	if (WARN_ON_ONCE(write ? !ctlv_blob_writable(blob) :
				 !ctlv_blob_readable(blob)))
		return false;
	if (WARN_ON_ONCE(check_add_overflow(offset, (u64)len, &end) ||
			 end > blob->len))
		return false;
	return true;
}

static int ctlv_blob_copy(struct ctlv_blob *blob, u64 offset, void *buf,
			  size_t len, bool write)
{
	if (!ctlv_blob_range_ok(blob, offset, len, write))
		return -EINVAL;
	if (!len)
		return 0;

	switch (blob->type) {
	case CTLV_BLOB_TYPE_VA:
		ctlv_blob_va_copy(blob, offset, buf, len, write);
		return 0;
	case CTLV_BLOB_TYPE_MEMFD:
		return ctlv_blob_memfd_copy(blob, offset, buf, len, write);
	case CTLV_BLOB_TYPE_DMA_BUF:
		ctlv_blob_dma_buf_copy(blob, offset, buf, len, write);
		return 0;
	default:
		return -EINVAL;
	}
}

/**
 * ctlv_blob_read - Copy out of a blob the kernel is given to read.
 * @blob: the resolved blob
 * @offset: where in it to begin, zero being its first byte
 * @dst: kernel memory to copy into
 * @len: bytes to copy, which have to be inside the blob
 *
 * Only a blob declared input or inout may be read, and only within its own
 * length; anything else warns.
 *
 * Return: 0, or a negative errno. Only a memfd can fail here.
 */
int ctlv_blob_read(struct ctlv_blob *blob, u64 offset, void *dst, size_t len)
{
	return ctlv_blob_copy(blob, offset, dst, len, false);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_read, "CTLV");

/**
 * ctlv_blob_write - Copy into a blob the kernel is given to fill.
 * @blob: the resolved blob
 * @offset: where in it to begin, zero being its first byte
 * @src: kernel memory to copy from
 * @len: bytes to copy, which have to be inside the blob
 *
 * The mirror of ctlv_blob_read(), for a blob declared output or inout.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_blob_write(struct ctlv_blob *blob, u64 offset, const void *src,
		    size_t len)
{
	return ctlv_blob_copy(blob, offset, (void *)src, len, true);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_write, "CTLV");

/**
 * ctlv_blob_put - Release one resolved blob.
 * @blob: what a resolver returned, or NULL
 *
 * Every successful resolve is released exactly once, by the callback that
 * asked for it and before it returns. Blobs share nothing, so releasing one
 * says nothing about any other.
 */
void ctlv_blob_put(struct ctlv_blob *blob)
{
	if (!blob)
		return;

	switch (blob->type) {
	case CTLV_BLOB_TYPE_VA:
		ctlv_blob_va_release(blob);
		break;
	case CTLV_BLOB_TYPE_MEMFD:
		fput(blob->memfd.file);
		break;
	case CTLV_BLOB_TYPE_DMA_BUF:
		ctlv_blob_dma_buf_release(blob);
		break;
	}
	kfree(blob);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_put, "CTLV");

/*
 * Which of an op's blobs was the unusable one, in the shape validation would
 * have used. The errno the family returns says what went wrong.
 */
static void ctlv_blob_failed(struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *cursor)
{
	if (ctx->error.reason)
		return;
	ctx->error = (struct ctlv_error){
		.reason = CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR,
		.bad_attr_offset = ctlv_msg_offset(cursor->msg, cursor->attr),
		.nest_offset = ctlv_msg_offset(cursor->msg, cursor->parent),
	};
}

static struct ctlv_blob *ctlv_blob_acquire(const struct ctlv_blob_desc *desc,
					   u8 direction)
{
	u64 address = desc->member[CTLV_BLOB_MEMBER_ADDRESS];
	u64 offset = desc->member[CTLV_BLOB_MEMBER_OFFSET];
	u64 len = desc->member[CTLV_BLOB_MEMBER_LENGTH];
	int fd = desc->member[CTLV_BLOB_MEMBER_FD];
	struct ctlv_blob *blob;
	struct dma_buf *dmabuf;
	int ret;

	if (desc->member[CTLV_BLOB_MEMBER_TYPE] == CTLV_BLOB_TYPE_DMA_BUF) {
		dmabuf = dma_buf_get(fd);
		if (IS_ERR(dmabuf))
			return ERR_CAST(dmabuf);
		return ctlv_blob_dma_buf_acquire(dmabuf, offset, len,
						 direction);
	}

	blob = kzalloc_obj(*blob, GFP_KERNEL_ACCOUNT);
	if (!blob)
		return ERR_PTR(-ENOMEM);
	blob->len = len;
	blob->direction = direction;
	blob->type = desc->member[CTLV_BLOB_MEMBER_TYPE];

	if (blob->type == CTLV_BLOB_TYPE_VA)
		ret = ctlv_blob_va_acquire(blob, address);
	else
		ret = ctlv_blob_memfd_acquire(blob, fd, offset);
	if (ret) {
		kfree(blob);
		return ERR_PTR(ret);
	}
	return blob;
}

/**
 * ctlv_blob_resolve - Acquire what one blob attr of one request describes.
 * @ctx: context of the op being executed
 * @blob: cursor of the descriptor, which a generated resolver located
 *
 * Reached through the resolver generated for the attr, which knows the attr's
 * declaration. Nothing is remembered between calls, so resolving one
 * descriptor twice acquires twice.
 *
 * Return: the blob, or an ERR_PTR: an errno to answer the caller with, or
 * -EINVAL where the mistake is this kernel's and has warned.
 */
struct ctlv_blob *ctlv_blob_resolve(struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *blob)
{
	const struct ctlv_attr *bad = NULL;
	const struct ctlv_schema_node *node;
	struct ctlv_blob_desc desc;
	struct ctlv_blob *acquired;

	/* An absent blob, or a cursor from some other execution entirely. */
	if (WARN_ON_ONCE(!ctlv_attr_ok(blob) || blob->msg != &ctx->request))
		return ERR_PTR(-EINVAL);
	node = &blob->tree->nodes[blob->node];
	if (WARN_ON_ONCE(node->kind != CTLV_SCHEMA_BLOB))
		return ERR_PTR(-EINVAL);
	if (WARN_ON_ONCE(!ctx->submitter || ctx->submitter != current))
		return ERR_PTR(-EINVAL);
	/* Validated before the callback, so anything wrong with it is a bug. */
	if (WARN_ON_ONCE(ctlv_blob_desc_read(blob->attr, &desc, &bad) ||
			 ctlv_blob_desc_check(&desc, node)))
		return ERR_PTR(-EINVAL);

	acquired = ctlv_blob_acquire(&desc, node->blob.direction);
	if (IS_ERR(acquired))
		ctlv_blob_failed(ctx, blob);
	return acquired;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_resolve, "CTLV");
