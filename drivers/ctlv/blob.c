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
#include <linux/dma-fence.h>
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
#include <linux/scatterlist.h>
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
 * A descriptor is the framework's own object and not part of a family's
 * schema, so nothing in one may be skipped as unknown: a member this kernel
 * does not know could be the one that bounds the transfer.
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

struct ctlv_blob_dma;

/**
 * struct ctlv_blob - What one resolver acquired.
 * @len: bytes of it, as the descriptor declared
 * @direction: enum ctlv_blob_direction, which copies the attr allows
 * @type: enum ctlv_blob_type, selecting which member below is live
 * @va: pages pinned out of the submitter's address space, @start bytes into
 *	the first of them, and the mm they were charged to
 * @memfd: the memory file, referenced, and where in it the blob begins
 * @ctx: execution which owns this blob, NULL for an internal test
 * @entry: membership in that execution's resolved blobs
 * @dma_buf: the buffer and whichever CPU or device access is active
 *
 * A family addresses a blob from zero; the offset into the backing is ours.
 */
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
			struct ctlv_blob_dma *dma;
			u64 offset;
			enum dma_data_direction cpu_dir;
			bool cpu_active;
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

static bool ctlv_blob_same_task(const struct ctlv_blob *blob)
{
	return !blob->ctx || !blob->ctx->submitter ||
	       blob->ctx->submitter == current;
}

static bool ctlv_blob_access_live(const struct ctlv_blob *blob)
{
	return ctlv_blob_same_task(blob) &&
	       (!blob->ctx || blob->ctx->blobs_live);
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

int ctlv_blob_memfd_access(u8 direction, fmode_t mode, int seals)
{
	fmode_t access = 0;
	fmode_t operations = 0;
	bool readable;
	bool writable;

	if (direction < CTLV_BLOB_DIRECTION_INPUT ||
	    direction > CTLV_BLOB_DIRECTION_INOUT)
		return -EINVAL;
	readable = direction != CTLV_BLOB_DIRECTION_OUTPUT;
	writable = direction != CTLV_BLOB_DIRECTION_INPUT;
	if (readable) {
		access |= FMODE_READ;
		operations |= FMODE_CAN_READ;
	}
	if (writable) {
		access |= FMODE_WRITE;
		operations |= FMODE_CAN_WRITE;
	}
	if ((mode & access) != access)
		return -EBADF;
	if ((mode & operations) != operations)
		return -EOPNOTSUPP;
	if (writable &&
	    seals & (F_SEAL_WRITE | F_SEAL_FUTURE_WRITE))
		return -EPERM;
	if (writable && !(seals & F_SEAL_SEAL))
		return -EINVAL;
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blob_memfd_access);

static int ctlv_blob_memfd_acquire(struct ctlv_blob *blob, int fd, u64 offset)
{
	struct file *source = fget(fd);
	struct file *file;
	u64 end;
	int seals;
	int ret;

	if (!source)
		return -EBADF;
	seals = memfd_get_seals(source);
	if (seals < 0) {
		ret = -EINVAL;
		goto err_put_source;
	}
	file = file_clone_open(source);
	fput(source);
	if (IS_ERR(file))
		return PTR_ERR(file);
	if (ctlv_blob_writable(blob) &&
	    (IS_APPEND(file_inode(file)) || IS_IMMUTABLE(file_inode(file)))) {
		ret = -EPERM;
		goto err_put;
	}
	/* The descriptor's explicit offset, never the caller's append state. */
	file->f_flags &= ~O_APPEND;
	file->f_iocb_flags = iocb_flags(file);
	/*
	 * Having seals at all is what says memfd; F_SEAL_SHRINK says the range
	 * cannot be truncated away underneath us. Existing write seals are
	 * incompatible with a blob this operation fills.
	 */
	seals = memfd_get_seals(file);
	if (seals < 0 || !(seals & F_SEAL_SHRINK) ||
	    check_add_overflow(offset, blob->len, &end) ||
	    end > i_size_read(file_inode(file))) {
		ret = -EINVAL;
		goto err_put;
	}
	ret = ctlv_blob_memfd_access(blob->direction, file->f_mode, seals);
	if (ret)
		goto err_put;
	blob->memfd.file = file;
	blob->memfd.offset = offset;
	return 0;

err_put:
	fput(file);
	return ret;

err_put_source:
	fput(source);
	return ret;
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

struct ctlv_blob_dma {
	struct ctlv_blob *blob;
	struct dma_buf_attachment *attach;
	struct sg_table *sgt;
	struct dma_fence *fence;
	enum dma_data_direction direction;
	bool signalling;
	bool submitting;
};

static void ctlv_blob_record_error(struct ctlv_blob *blob, int error)
{
	if (error && blob->ctx && !blob->ctx->blob_error)
		blob->ctx->blob_error = error;
}

/* Returns with the reservation locked and one fence slot reserved. */
static int ctlv_blob_resv_prepare(struct ctlv_blob *blob)
{
	struct dma_resv *resv = blob->dma_buf.dmabuf->resv;
	enum dma_resv_usage usage;
	signed long remaining;
	int ret;

	usage = dma_resv_usage_rw(ctlv_blob_writable(blob));
	ret = dma_resv_lock_interruptible(resv, NULL);
	if (ret)
		return ret;
	remaining = dma_resv_wait_timeout(resv, usage, true,
					  MAX_SCHEDULE_TIMEOUT);
	if (remaining <= 0) {
		ret = remaining ? (int)remaining : -EBUSY;
		goto err_unlock;
	}
	ret = dma_resv_reserve_fences(resv, 1);
	if (ret)
		goto err_unlock;
	return 0;

err_unlock:
	dma_resv_unlock(resv);
	return ret;
}

static void ctlv_blob_fence_publish(struct ctlv_blob *blob,
				    struct dma_fence *fence)
{
	dma_resv_add_fence(blob->dma_buf.dmabuf->resv, fence,
			   ctlv_blob_writable(blob) ? DMA_RESV_USAGE_WRITE :
						      DMA_RESV_USAGE_READ);
}

/**
 * ctlv_blob_dma_buf_acquire - Take one buffer for the length of one op.
 * @dmabuf: the buffer, whose reference this takes over
 * @offset: where in it the blob begins
 * @len: bytes of it the blob is
 * @direction: enum ctlv_blob_direction the attr declared
 *
 * Mapping is lazy: CPU copies and device DMA use different dma-buf interfaces,
 * and an exporter need only support the one the family asks for.
 *
 * Return: the blob, or an ERR_PTR.
 */
struct ctlv_blob *ctlv_blob_dma_buf_acquire(struct dma_buf *dmabuf, u64 offset,
					    u64 len, u8 direction)
{
	struct ctlv_blob *blob;
	u64 end;

	blob = kzalloc_obj(*blob, GFP_KERNEL_ACCOUNT);
	if (!blob) {
		dma_buf_put(dmabuf);
		return ERR_PTR(-ENOMEM);
	}
	INIT_LIST_HEAD(&blob->entry);
	blob->len = len;
	blob->direction = direction;
	blob->type = CTLV_BLOB_TYPE_DMA_BUF;
	blob->dma_buf.dmabuf = dmabuf;
	blob->dma_buf.offset = offset;
	if (!ctlv_blob_writable(blob))
		blob->dma_buf.cpu_dir = DMA_FROM_DEVICE;
	else if (!ctlv_blob_readable(blob))
		blob->dma_buf.cpu_dir = DMA_TO_DEVICE;
	else
		blob->dma_buf.cpu_dir = DMA_BIDIRECTIONAL;

	if (!len || direction < CTLV_BLOB_DIRECTION_INPUT ||
	    direction > CTLV_BLOB_DIRECTION_INOUT ||
	    check_add_overflow(offset, len, &end) || end > dmabuf->size) {
		kfree(blob);
		dma_buf_put(dmabuf);
		return ERR_PTR(-EINVAL);
	}
	return blob;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blob_dma_buf_acquire);

static int ctlv_blob_dma_buf_cpu_begin(struct ctlv_blob *blob)
{
	struct dma_buf *dmabuf = blob->dma_buf.dmabuf;
	int ret;

	if (blob->dma_buf.cpu_active)
		return 0;
	if (WARN_ON_ONCE(blob->dma_buf.dma))
		return -EBUSY;
	if (blob->ctx && blob->ctx->dma_maps)
		return -EBUSY;

	ret = dma_buf_vmap_unlocked(dmabuf, &blob->dma_buf.map);
	if (ret)
		return ret;
	ret = dma_buf_begin_cpu_access(dmabuf, blob->dma_buf.cpu_dir);
	if (ret) {
		dma_buf_vunmap_unlocked(dmabuf, &blob->dma_buf.map);
		return ret;
	}
	blob->dma_buf.cpu_active = true;
	if (blob->ctx)
		blob->ctx->cpu_blobs++;
	return 0;
}

static int ctlv_blob_dma_buf_cpu_end(struct ctlv_blob *blob)
{
	struct dma_buf *dmabuf = blob->dma_buf.dmabuf;
	int ret;

	if (!blob->dma_buf.cpu_active)
		return 0;
	ret = dma_buf_end_cpu_access(dmabuf, blob->dma_buf.cpu_dir);
	dma_buf_vunmap_unlocked(dmabuf, &blob->dma_buf.map);
	blob->dma_buf.cpu_active = false;
	if (blob->ctx)
		blob->ctx->cpu_blobs--;
	ctlv_blob_record_error(blob, ret);
	return ret;
}

static enum dma_data_direction
ctlv_blob_device_direction(const struct ctlv_blob *blob)
{
	if (!ctlv_blob_writable(blob))
		return DMA_TO_DEVICE;
	if (!ctlv_blob_readable(blob))
		return DMA_FROM_DEVICE;
	return DMA_BIDIRECTIONAL;
}

static bool ctlv_blob_sgt_covers(const struct ctlv_blob *blob,
				 const struct sg_table *sgt)
{
	struct scatterlist *sg;
	u64 mapped = 0;
	u64 needed;
	int i;

	if (check_add_overflow(blob->dma_buf.offset, blob->len, &needed))
		return false;
	for_each_sgtable_dma_sg(sgt, sg, i) {
		if (check_add_overflow(mapped, (u64)sg_dma_len(sg), &mapped))
			return false;
		if (mapped >= needed)
			return true;
	}
	return false;
}

/**
 * ctlv_blob_dma_map - Map one resolved blob for a family device.
 * @blob: the resolved blob
 * @dev: device which will access it
 * @sgt: receives the exporter's whole-buffer DMA mapping
 * @dma_skip: receives bytes to skip in @sgt before this blob begins
 *
 * The direction comes from the blob declaration. This prepares no access;
 * ctlv_blob_dma_begin() publishes it immediately before device submission.
 *
 * Return: an opaque mapping token, or an ERR_PTR.
 */
struct ctlv_blob_dma *ctlv_blob_dma_map(struct ctlv_blob *blob,
					struct device *dev,
					struct sg_table **sgt, u64 *dma_skip)
{
	struct ctlv_blob_dma *dma;
	int ret;

	if (WARN_ON_ONCE(!blob || !dev || !sgt || !dma_skip))
		return ERR_PTR(-EINVAL);
	if (WARN_ON_ONCE(!ctlv_blob_access_live(blob)))
		return ERR_PTR(-EINVAL);
	if (blob->type != CTLV_BLOB_TYPE_DMA_BUF)
		return ERR_PTR(-EOPNOTSUPP);
	if (WARN_ON_ONCE(blob->dma_buf.cpu_active || blob->dma_buf.dma))
		return ERR_PTR(-EBUSY);
	if (blob->ctx && (blob->ctx->cpu_blobs || blob->ctx->dma_maps))
		return ERR_PTR(-EBUSY);

	dma = kzalloc_obj(*dma, GFP_KERNEL_ACCOUNT);
	if (!dma)
		return ERR_PTR(-ENOMEM);
	dma->blob = blob;
	dma->direction = ctlv_blob_device_direction(blob);
	dma->attach = dma_buf_attach(blob->dma_buf.dmabuf, dev);
	if (IS_ERR(dma->attach)) {
		ret = PTR_ERR(dma->attach);
		goto err_free_dma;
	}
	dma->sgt = dma_buf_map_attachment_unlocked(dma->attach,
						   dma->direction);
	if (IS_ERR(dma->sgt)) {
		ret = PTR_ERR(dma->sgt);
		goto err_detach;
	}
	if (!ctlv_blob_sgt_covers(blob, dma->sgt)) {
		ret = -EIO;
		goto err_unmap;
	}
	blob->dma_buf.dma = dma;
	if (blob->ctx)
		blob->ctx->dma_maps++;

	*sgt = dma->sgt;
	*dma_skip = blob->dma_buf.offset;
	return dma;

err_unmap:
	dma_buf_unmap_attachment_unlocked(dma->attach, dma->sgt,
					  dma->direction);
err_detach:
	dma_buf_detach(blob->dma_buf.dmabuf, dma->attach);
err_free_dma:
	kfree(dma);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_dma_map, "CTLV");

/**
 * ctlv_blob_dma_begin - Publish a device's prepared completion fence.
 * @dma: token returned by ctlv_blob_dma_map()
 * @fence: unsignaled, non-container fence for the device completion
 *
 * The family prepares the fence and all submission resources first. This opens
 * the signaling section before publishing; the family submits the work and
 * closes that section with ctlv_blob_dma_submit_done().
 *
 * Return: 0, or a negative errno before any fence was published.
 */
int ctlv_blob_dma_begin(struct ctlv_blob_dma *dma, struct dma_fence *fence)
{
	struct ctlv_blob *blob;
	int ret;

	if (WARN_ON_ONCE(!dma || !fence))
		return -EINVAL;
	blob = dma->blob;
	if (WARN_ON_ONCE(!ctlv_blob_access_live(blob) ||
			 blob->dma_buf.dma != dma))
		return -EINVAL;
	if (WARN_ON_ONCE(dma->fence))
		return -EBUSY;
	if (WARN_ON_ONCE(dma_fence_is_signaled(fence) ||
			 dma_fence_is_container(fence)))
		return -EINVAL;
	ret = ctlv_blob_resv_prepare(blob);
	if (ret)
		return ret;
	dma->fence = dma_fence_get(fence);
	dma->signalling = dma_fence_begin_signalling();
	dma->submitting = true;
	ctlv_blob_fence_publish(blob, fence);
	dma_resv_unlock(blob->dma_buf.dmabuf->resv);
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_dma_begin, "CTLV");

/**
 * ctlv_blob_dma_submit_done - Finish one device submission attempt.
 * @dma: token whose fence ctlv_blob_dma_begin() published
 * @status: zero if queued, or a negative submission errno
 *
 * Call exactly once after attempting to queue the prepared work. On failure no
 * device completion will arrive, so this sets the fence error and signals it.
 */
void ctlv_blob_dma_submit_done(struct ctlv_blob_dma *dma, int status)
{
	if (WARN_ON_ONCE(!dma || !dma->submitting))
		return;
	if (WARN_ON_ONCE(!ctlv_blob_same_task(dma->blob)))
		return;
	if (WARN_ON_ONCE(status > 0 || status < -MAX_ERRNO))
		status = -EINVAL;
	if (status) {
		dma_fence_set_error(dma->fence, status);
		dma_fence_signal(dma->fence);
	}
	dma->submitting = false;
	dma_fence_end_signalling(dma->signalling);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_dma_submit_done, "CTLV");

/**
 * ctlv_blob_dma_unmap - Complete and release one family-device mapping.
 * @dma: token returned by ctlv_blob_dma_map()
 * @status: zero, or a negative errno from the completed device access
 *
 * The family must signal its fence after stopping device access. This waits
 * uninterruptibly if needed before tearing down the mapping and attachment.
 */
void ctlv_blob_dma_unmap(struct ctlv_blob_dma *dma, int status)
{
	struct ctlv_blob *blob;
	signed long wait_ret;
	int fence_status;

	if (WARN_ON_ONCE(!dma))
		return;
	blob = dma->blob;
	if (WARN_ON_ONCE(!ctlv_blob_same_task(blob)))
		return;
	if (WARN_ON_ONCE(blob->dma_buf.dma != dma))
		return;
	if (WARN_ON_ONCE(status > 0 || status < -MAX_ERRNO))
		status = -EINVAL;
	if (WARN_ON_ONCE(dma->submitting))
		ctlv_blob_dma_submit_done(dma, status);

	if (dma->fence) {
		do {
			wait_ret = dma_fence_wait(dma->fence, false);
			if (wait_ret < 0 && !status)
				status = wait_ret;
			if (wait_ret < 0)
				cond_resched();
		} while (!dma_fence_is_signaled(dma->fence));
		fence_status = dma_fence_get_status(dma->fence);
		if (!status && fence_status < 0)
			status = fence_status;
	}
	blob->dma_buf.dma = NULL;
	ctlv_blob_record_error(blob, status);
	dma_buf_unmap_attachment_unlocked(dma->attach, dma->sgt,
					  dma->direction);
	dma_buf_detach(blob->dma_buf.dmabuf, dma->attach);
	if (blob->ctx)
		blob->ctx->dma_maps--;
	dma_fence_put(dma->fence);
	kfree(dma);
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_dma_unmap, "CTLV");

static int ctlv_blob_dma_buf_release(struct ctlv_blob *blob)
{
	int ret = 0;

	if (WARN_ON_ONCE(blob->dma_buf.dma)) {
		ret = -ECANCELED;
		ctlv_blob_dma_unmap(blob->dma_buf.dma, ret);
	}
	if (!ret)
		ret = ctlv_blob_dma_buf_cpu_end(blob);
	else
		ctlv_blob_dma_buf_cpu_end(blob);
	dma_buf_put(blob->dma_buf.dmabuf);
	return ret;
}

static int ctlv_blob_dma_buf_copy(struct ctlv_blob *blob, u64 offset,
				  void *buf, size_t len, bool write)
{
	u64 at = blob->dma_buf.offset + offset;
	int ret;

	ret = ctlv_blob_dma_buf_cpu_begin(blob);
	if (ret)
		return ret;
	if (write)
		iosys_map_memcpy_to(&blob->dma_buf.map, at, buf, len);
	else
		iosys_map_memcpy_from(buf, &blob->dma_buf.map, at, len);
	return 0;
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
	if (WARN_ON_ONCE(!ctlv_blob_access_live(blob)))
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
		return ctlv_blob_dma_buf_copy(blob, offset, buf, len, write);
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
 * Return: 0, or a negative errno from memfd I/O or dma-buf CPU mapping.
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
 * asked for it and before it returns. A fallible dma-buf completion is also
 * retained by the op context, so ignoring this return cannot report success.
 *
 * Return: 0, or a negative dma-buf completion error.
 */
int ctlv_blob_put(struct ctlv_blob *blob)
{
	int ret = 0;

	if (!blob)
		return 0;
	if (WARN_ON_ONCE(!ctlv_blob_same_task(blob)))
		return -EINVAL;

	switch (blob->type) {
	case CTLV_BLOB_TYPE_VA:
		ctlv_blob_va_release(blob);
		break;
	case CTLV_BLOB_TYPE_MEMFD:
		fput(blob->memfd.file);
		break;
	case CTLV_BLOB_TYPE_DMA_BUF:
		ret = ctlv_blob_dma_buf_release(blob);
		break;
	}
	ctlv_blob_record_error(blob, ret);
	if (blob->ctx) {
		list_del(&blob->entry);
		blob->ctx = NULL;
	}
	kfree(blob);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_put, "CTLV");

void ctlv_blobs_init(struct ctlv_op_ctx *ctx)
{
	INIT_LIST_HEAD(&ctx->blobs);
	ctx->blobs_live = true;
	ctx->blob_error = 0;
	ctx->cpu_blobs = 0;
	ctx->dma_maps = 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blobs_init);

void ctlv_blob_track(struct ctlv_blob *blob, struct ctlv_op_ctx *ctx)
{
	blob->ctx = ctx;
	list_add_tail(&blob->entry, &ctx->blobs);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blob_track);

int ctlv_blobs_finish(struct ctlv_op_ctx *ctx)
{
	ctx->blobs_live = false;
	if (WARN_ON_ONCE(!list_empty(&ctx->blobs)) && !ctx->blob_error)
		ctx->blob_error = -EINVAL;
	while (!list_empty(&ctx->blobs)) {
		struct ctlv_blob *blob =
			list_first_entry(&ctx->blobs, struct ctlv_blob, entry);

		ctlv_blob_put(blob);
	}
	WARN_ON_ONCE(ctx->cpu_blobs);
	WARN_ON_ONCE(ctx->dma_maps);
	return ctx->blob_error;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_blobs_finish);

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
	INIT_LIST_HEAD(&blob->entry);
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
	if (WARN_ON_ONCE(!ctx || !blob || blob->msg != &ctx->request ||
			 !ctlv_attr_ok(blob)))
		return ERR_PTR(-EINVAL);
	node = &blob->tree->nodes[blob->node];
	if (WARN_ON_ONCE(node->kind != CTLV_SCHEMA_BLOB))
		return ERR_PTR(-EINVAL);
	if (WARN_ON_ONCE(!ctx->blobs_live || !ctx->submitter ||
			 ctx->submitter != current))
		return ERR_PTR(-EINVAL);
	/* Validated before the callback, so anything wrong with it is a bug. */
	if (WARN_ON_ONCE(ctlv_blob_desc_read(blob->attr, &desc, &bad) ||
			 ctlv_blob_desc_check(&desc, node)))
		return ERR_PTR(-EINVAL);

	acquired = ctlv_blob_acquire(&desc, node->blob.direction);
	if (IS_ERR(acquired))
		ctlv_blob_failed(ctx, blob);
	else
		ctlv_blob_track(acquired, ctx);
	return acquired;
}
EXPORT_SYMBOL_NS_GPL(ctlv_blob_resolve, "CTLV");
