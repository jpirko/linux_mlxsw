// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * KUnit checks over what a blob is once it has been acquired.
 *
 * The dma-buf here is exported by this module, so the exporter can count what
 * the framework asked of it. Copying the way a blob does not allow warns, so
 * those cases suppress the warning. The page arithmetic is checked with
 * numbers no task could have, which need no address space to be tried in.
 */

#include <kunit/device.h>
#include <kunit/test.h>
#include <linux/array_size.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/dma-direction.h>
#include <linux/dma-fence.h>
#include <linux/dma-fence-array.h>
#include <linux/dma-resv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/gfp_types.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/ktime.h>
#include <linux/limits.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/workqueue.h>

#include <uapi/linux/ctlv_gen.h>

#include "../blob.h"
#include "../ctlv_private.h"

/* Long enough to hold a blob that begins somewhere other than at zero. */
#define BLOB_BUF_SIZE 8192
#define BLOB_BUF_HALF (BLOB_BUF_SIZE / 2)

/* What a fence is signaled after, so the least a wait for one takes. */
#define BLOB_FENCE_DELAY_MS 20

/**
 * struct blob_buffer - One exported buffer, and what was asked of it.
 * @dmabuf: the buffer, one reference of which is this fixture's
 * @access: CPU access sections currently open
 * @cpu_maps: CPU mappings the exporter has handed out
 * @attachments: device attachments currently held
 * @device_maps: device mappings currently held
 * @cpu_dir: direction of the last CPU access
 * @device_dir: direction of the last device mapping
 * @begin_mapped: whether begin CPU access saw the vmap
 * @end_mapped: whether end CPU access saw the vmap
 * @fail_attach: injected attach error
 * @fail_map: injected device-map error
 * @fail_begin: injected begin-CPU-access error
 * @fail_vmap: injected vmap error
 * @fail_end: injected end-CPU-access error
 * @fail_end_count: how many end calls fail
 * @sgt: whole-buffer device mapping
 * @bytes: what the buffer holds
 *
 * Freed by the exporter's release, not with the test: a file is closed after
 * whoever closed it has moved on.
 */
struct blob_buffer {
	struct dma_buf *dmabuf;
	unsigned int access;
	unsigned int cpu_maps;
	unsigned int attachments;
	unsigned int device_maps;
	enum dma_data_direction cpu_dir;
	enum dma_data_direction device_dir;
	bool begin_mapped;
	bool end_mapped;
	int fail_attach;
	int fail_map;
	int fail_begin;
	int fail_vmap;
	int fail_end;
	unsigned int fail_end_count;
	struct sg_table sgt;
	u8 bytes[BLOB_BUF_SIZE];
};

static int blob_attach(struct dma_buf *dmabuf,
		       struct dma_buf_attachment *attach)
{
	struct blob_buffer *buf = dmabuf->priv;

	if (buf->fail_attach)
		return buf->fail_attach;
	buf->attachments++;
	return 0;
}

static void blob_detach(struct dma_buf *dmabuf,
			struct dma_buf_attachment *attach)
{
	struct blob_buffer *buf = dmabuf->priv;

	buf->attachments--;
}

static struct sg_table *blob_map_dma_buf(struct dma_buf_attachment *attach,
					 enum dma_data_direction dir)
{
	struct blob_buffer *buf = attach->dmabuf->priv;

	if (buf->fail_map)
		return ERR_PTR(buf->fail_map);
	buf->device_maps++;
	buf->device_dir = dir;
	return &buf->sgt;
}

static void blob_unmap_dma_buf(struct dma_buf_attachment *attach,
			       struct sg_table *sgt,
			       enum dma_data_direction dir)
{
	struct blob_buffer *buf = attach->dmabuf->priv;

	buf->device_maps--;
}

static int blob_begin_cpu_access(struct dma_buf *dmabuf,
				 enum dma_data_direction dir)
{
	struct blob_buffer *buf = dmabuf->priv;

	if (buf->fail_begin)
		return buf->fail_begin;
	buf->begin_mapped = buf->cpu_maps;
	buf->access++;
	buf->cpu_dir = dir;
	return 0;
}

static int blob_end_cpu_access(struct dma_buf *dmabuf,
			       enum dma_data_direction dir)
{
	struct blob_buffer *buf = dmabuf->priv;
	int ret = 0;

	if (buf->fail_end_count) {
		buf->fail_end_count--;
		ret = buf->fail_end;
	}
	buf->end_mapped = buf->cpu_maps;
	buf->access--;
	return ret;
}

static int blob_vmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct blob_buffer *buf = dmabuf->priv;

	if (buf->fail_vmap)
		return buf->fail_vmap;
	iosys_map_set_vaddr(map, buf->bytes);
	buf->cpu_maps++;
	return 0;
}

static void blob_vunmap(struct dma_buf *dmabuf, struct iosys_map *map)
{
	struct blob_buffer *buf = dmabuf->priv;

	buf->cpu_maps--;
}

static void blob_release(struct dma_buf *dmabuf)
{
	struct blob_buffer *buf = dmabuf->priv;

	sg_free_table(&buf->sgt);
	kfree(buf);
}

static const struct dma_buf_ops blob_buf_ops = {
	.attach = blob_attach,
	.detach = blob_detach,
	.map_dma_buf = blob_map_dma_buf,
	.unmap_dma_buf = blob_unmap_dma_buf,
	.begin_cpu_access = blob_begin_cpu_access,
	.end_cpu_access = blob_end_cpu_access,
	.vmap = blob_vmap,
	.vunmap = blob_vunmap,
	.release = blob_release,
};

/* The fixture's own reference, which is the last one anything holds. */
static void blob_buffer_put(void *arg)
{
	struct blob_buffer *buf = arg;

	dma_buf_put(buf->dmabuf);
}

/* Filled with the position of each byte, so a read says which part it was. */
static struct blob_buffer *blob_buffer_new(struct kunit *test)
{
	DEFINE_DMA_BUF_EXPORT_INFO(exp_info);
	struct blob_buffer *buf;
	struct scatterlist *sg;
	u32 i;
	int ret;

	buf = kzalloc_obj(*buf, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, buf);
	for (i = 0; i < BLOB_BUF_SIZE; i++)
		buf->bytes[i] = i;
	ret = sg_alloc_table(&buf->sgt, 2, GFP_KERNEL);
	if (ret) {
		kfree(buf);
		KUNIT_FAIL_AND_ABORT(test, "allocating an sg table failed");
	}
	sg = buf->sgt.sgl;
	sg_set_buf(sg, buf->bytes, BLOB_BUF_HALF);
	sg_dma_address(sg) = PAGE_SIZE;
	sg_dma_len(sg) = BLOB_BUF_HALF;
	sg = sg_next(sg);
	sg_set_buf(sg, buf->bytes + BLOB_BUF_HALF, BLOB_BUF_HALF);
	sg_dma_address(sg) = 2 * PAGE_SIZE;
	sg_dma_len(sg) = BLOB_BUF_HALF;

	exp_info.exp_name = "ctlv_blob_test";
	exp_info.owner = THIS_MODULE;
	exp_info.ops = &blob_buf_ops;
	exp_info.size = BLOB_BUF_SIZE;
	exp_info.flags = O_RDWR;
	exp_info.priv = buf;

	buf->dmabuf = dma_buf_export(&exp_info);
	if (IS_ERR(buf->dmabuf)) {
		sg_free_table(&buf->sgt);
		kfree(buf);
		KUNIT_FAIL_AND_ABORT(test, "exporting a buffer failed");
	}
	kunit_add_action(test, blob_buffer_put, buf);
	return buf;
}

/*
 * One acquisition, which takes over a reference: the framework is handed what
 * dma_buf_get() would have produced from an fd.
 */
static struct ctlv_blob *blob_acquire(struct blob_buffer *buf, u64 offset,
				      u64 len, u8 direction)
{
	get_dma_buf(buf->dmabuf);
	return ctlv_blob_dma_buf_acquire(buf->dmabuf, offset, len, direction);
}

/*
 * A fence is signaled from a work and the wait measured from before the work
 * was queued, so an acquisition that returned early did not wait for it.
 */

struct blob_fence {
	struct dma_fence fence;
	/* Passed to dma_fence_init(), so it lives as long as the fence. */
	spinlock_t lock;
	struct delayed_work signal;
};

static const char *blob_fence_name(struct dma_fence *fence)
{
	return "ctlv_blob_test";
}

static const struct dma_fence_ops blob_fence_ops = {
	.get_driver_name = blob_fence_name,
	.get_timeline_name = blob_fence_name,
};

static void blob_fence_signal(struct blob_fence *f, int error)
{
	bool cookie = dma_fence_begin_signalling();

	if (error)
		dma_fence_set_error(&f->fence, error);
	dma_fence_signal(&f->fence);
	dma_fence_end_signalling(cookie);
}

static void blob_fence_work(struct work_struct *work)
{
	struct blob_fence *f = container_of(to_delayed_work(work),
					    struct blob_fence, signal);

	blob_fence_signal(f, 0);
}

/* Nothing may be left waiting for it, and nothing left to signal it. */
static void blob_fence_stop(void *arg)
{
	struct blob_fence *f = arg;

	cancel_delayed_work_sync(&f->signal);
	blob_fence_signal(f, 0);
	dma_fence_put(&f->fence);
}

static struct blob_fence *blob_fence_new(struct kunit *test)
{
	struct blob_fence *f;

	f = kzalloc_obj(*f, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, f);
	spin_lock_init(&f->lock);
	INIT_DELAYED_WORK(&f->signal, blob_fence_work);
	dma_fence_init(&f->fence, &blob_fence_ops, &f->lock,
		       dma_fence_context_alloc(1), 1);
	kunit_add_action(test, blob_fence_stop, f);
	return f;
}

static void blob_dma_fence_put(void *arg)
{
	dma_fence_put(arg);
}

static struct dma_fence *
blob_fence_array_new(struct kunit *test, struct blob_fence *component)
{
	struct dma_fence_array *array;
	struct dma_fence **fences;
	int ret;

	fences = kmalloc_array(1, sizeof(*fences), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, fences);
	fences[0] = dma_fence_get(&component->fence);
	array = dma_fence_array_create(1, fences,
				       dma_fence_context_alloc(1), 1);
	if (!array) {
		dma_fence_put(fences[0]);
		kfree(fences);
		KUNIT_FAIL_AND_ABORT(test, "allocating a fence array failed");
	}
	ret = kunit_add_action_or_reset(test, blob_dma_fence_put,
					&array->base);
	KUNIT_ASSERT_EQ(test, ret, 0);
	return &array->base;
}

static struct blob_fence *blob_fence_add(struct kunit *test,
					 struct blob_buffer *buf,
					 enum dma_resv_usage usage)
{
	struct dma_resv *resv = buf->dmabuf->resv;
	struct blob_fence *f = blob_fence_new(test);
	int ret;

	dma_resv_lock(resv, NULL);
	ret = dma_resv_reserve_fences(resv, 1);
	if (!ret)
		dma_resv_add_fence(resv, &f->fence, usage);
	dma_resv_unlock(resv);
	KUNIT_ASSERT_EQ(test, ret, 0);
	return f;
}

static bool blob_fences_signaled(struct blob_buffer *buf,
				 enum dma_resv_usage usage)
{
	return dma_resv_test_signaled(buf->dmabuf->resv, usage);
}

static void ctlv_blob_test_memfd_access(struct kunit *test)
{
	const fmode_t read = FMODE_READ | FMODE_CAN_READ;
	const fmode_t write = FMODE_WRITE | FMODE_CAN_WRITE;
	const int stable = F_SEAL_SHRINK | F_SEAL_SEAL;

	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INPUT,
					       read, F_SEAL_SHRINK), 0);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INPUT,
					       write, F_SEAL_SHRINK), -EBADF);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_OUTPUT,
					       write, stable), 0);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_OUTPUT,
					       read, stable), -EBADF);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INOUT,
					       read | write, stable), 0);

	/* Hugetlbfs has a read iterator but no write iterator. */
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INPUT,
					       read | FMODE_WRITE, stable), 0);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_OUTPUT,
					       read | FMODE_WRITE, stable),
			-EOPNOTSUPP);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INOUT,
					       read | FMODE_WRITE, stable),
			-EOPNOTSUPP);

	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INPUT,
					       read | write,
					       stable | F_SEAL_WRITE), 0);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_OUTPUT,
					       read | write,
					       stable | F_SEAL_WRITE), -EPERM);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_INOUT,
					       read | write,
					       stable | F_SEAL_FUTURE_WRITE),
			-EPERM);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_memfd_access(CTLV_BLOB_DIRECTION_OUTPUT,
					       read | write, F_SEAL_SHRINK),
			-EINVAL);
}

/* A CPU access section is open for as long as the blob is held. */
static void ctlv_blob_test_read_and_write(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_blob *blob;
	u8 *data;

	data = kunit_kzalloc(test, BLOB_BUF_SIZE, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, data);

	blob = blob_acquire(buf, 0, BLOB_BUF_SIZE,
			    CTLV_BLOB_DIRECTION_INOUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_len(blob), BLOB_BUF_SIZE);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);

	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, data, BLOB_BUF_SIZE), 0);
	KUNIT_EXPECT_EQ(test, buf->access, 1);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 1);
	KUNIT_EXPECT_EQ(test, buf->cpu_dir, DMA_BIDIRECTIONAL);
	KUNIT_EXPECT_TRUE(test, buf->begin_mapped);
	KUNIT_EXPECT_MEMEQ(test, data, buf->bytes, BLOB_BUF_SIZE);

	memset(data, 0xa5, BLOB_BUF_SIZE);
	KUNIT_EXPECT_EQ(test, ctlv_blob_write(blob, 0, data, BLOB_BUF_SIZE), 0);
	KUNIT_EXPECT_MEMEQ(test, buf->bytes, data, BLOB_BUF_SIZE);

	/* No bytes at all is nothing to copy, not a fault. */
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, BLOB_BUF_SIZE, data, 0), 0);

	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	KUNIT_EXPECT_TRUE(test, buf->end_mapped);
	/* The reference the release gave back was the blob's own. */
	KUNIT_EXPECT_EQ(test, file_count(buf->dmabuf->file), 1);
}

/* A family addresses a blob from zero, whatever offset it began at. */
static void ctlv_blob_test_where_a_blob_begins(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_blob *blob;
	u8 *data;
	u32 i;

	data = kunit_kzalloc(test, BLOB_BUF_HALF, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, data);

	blob = blob_acquire(buf, BLOB_BUF_HALF, BLOB_BUF_HALF,
			    CTLV_BLOB_DIRECTION_INOUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_len(blob), BLOB_BUF_HALF);

	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, data, BLOB_BUF_HALF), 0);
	KUNIT_EXPECT_MEMEQ(test, data, buf->bytes + BLOB_BUF_HALF,
			   BLOB_BUF_HALF);

	memset(data, 0x5a, BLOB_BUF_HALF);
	KUNIT_EXPECT_EQ(test, ctlv_blob_write(blob, 0, data, BLOB_BUF_HALF), 0);
	KUNIT_EXPECT_MEMEQ(test, buf->bytes + BLOB_BUF_HALF, data,
			   BLOB_BUF_HALF);
	for (i = 0; i < BLOB_BUF_HALF; i++)
		KUNIT_ASSERT_EQ_MSG(test, buf->bytes[i], (u8)i,
				    "byte %u before the blob was written", i);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

/* The declared direction reaches the exporter too. */
static void ctlv_blob_test_direction(struct kunit *test)
{
	struct blob_buffer *in = blob_buffer_new(test);
	struct blob_buffer *out = blob_buffer_new(test);
	struct ctlv_blob *blob;
	u8 data[8] = {};

	blob = blob_acquire(in, 0, sizeof(data), CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, data, sizeof(data)), 0);
	KUNIT_EXPECT_EQ(test, in->cpu_dir, DMA_FROM_DEVICE);
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test,
				ctlv_blob_write(blob, 0, data, sizeof(data)),
				-EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);

	blob = blob_acquire(out, 0, sizeof(data), CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_write(blob, 0, data, sizeof(data)), 0);
	KUNIT_EXPECT_EQ(test, out->cpu_dir, DMA_TO_DEVICE);
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test,
				ctlv_blob_read(blob, 0, data, sizeof(data)),
				-EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

static void ctlv_blob_test_a_copy_outside_the_blob(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_blob *blob;
	u8 data[8] = {};

	blob = blob_acquire(buf, 0, BLOB_BUF_HALF, CTLV_BLOB_DIRECTION_INOUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));

	kunit_warning_suppress(test) {
		/* Past the end of the blob, though inside the buffer. */
		KUNIT_EXPECT_EQ(test,
				ctlv_blob_read(blob, BLOB_BUF_HALF, data,
					       sizeof(data)), -EINVAL);
		/* Beginning inside it and ending past it. */
		KUNIT_EXPECT_EQ(test,
				ctlv_blob_write(blob, BLOB_BUF_HALF - 1, data,
						sizeof(data)), -EINVAL);
		/* A range that wraps. */
		KUNIT_EXPECT_EQ(test,
				ctlv_blob_read(blob, U64_MAX, data,
					       sizeof(data)), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 3);
	}
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

/* Releasing one of two acquisitions leaves the other as it was. */
static void ctlv_blob_test_two_acquisitions(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_blob *first, *second;
	u8 data[8] = {};

	first = blob_acquire(buf, 0, BLOB_BUF_HALF, CTLV_BLOB_DIRECTION_INOUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(first));
	second = blob_acquire(buf, 0, BLOB_BUF_HALF, CTLV_BLOB_DIRECTION_INOUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(second));
	KUNIT_EXPECT_PTR_NE(test, first, second);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(first, 0, data, sizeof(data)), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(second, 0, data, sizeof(data)), 0);
	KUNIT_EXPECT_EQ(test, buf->access, 2);

	KUNIT_EXPECT_EQ(test, ctlv_blob_put(first), 0);
	KUNIT_EXPECT_EQ(test, buf->access, 1);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 1);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(second, 0, data, sizeof(data)), 0);
	KUNIT_EXPECT_MEMEQ(test, data, buf->bytes, sizeof(data));

	KUNIT_EXPECT_EQ(test, ctlv_blob_put(second), 0);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	KUNIT_EXPECT_EQ(test, file_count(buf->dmabuf->file), 1);
}

/* A caller mistake, so an errno and not a warning, and the reference back. */
static void ctlv_blob_test_a_blob_outside_the_buffer(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_blob *blob;

	KUNIT_ASSERT_EQ(test, file_count(buf->dmabuf->file), 1);
	blob = blob_acquire(buf, BLOB_BUF_HALF, BLOB_BUF_SIZE,
			    CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_EXPECT_EQ(test, PTR_ERR(blob), -EINVAL);
	KUNIT_EXPECT_EQ(test, file_count(buf->dmabuf->file), 1);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
}

/* Releasing nothing is what a family that resolved nothing may do. */
static void ctlv_blob_test_releasing_nothing(struct kunit *test)
{
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(NULL), 0);
}

static void ctlv_blob_test_stale_cursor_is_rejected(struct kunit *test)
{
	struct ctlv_cursor cursor = {
		.msg = (void *)1,
		.attr = (void *)1,
	};
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob *blob;

	kunit_warning_suppress(test) {
		blob = ctlv_blob_resolve(&ctx, &cursor);
		KUNIT_EXPECT_TRUE(test, IS_ERR(blob));
		KUNIT_EXPECT_EQ(test, PTR_ERR(blob), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
}

static void ctlv_blob_test_a_read_waits_for_writers(struct kunit *test)
{
	struct blob_buffer *reading = blob_buffer_new(test);
	struct blob_buffer *written = blob_buffer_new(test);
	struct blob_fence *f;
	struct ctlv_blob *blob;
	ktime_t start;

	f = blob_fence_add(test, reading, DMA_RESV_USAGE_READ);
	blob = blob_acquire(reading, 0, BLOB_BUF_HALF,
			    CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &start, 1), 0);
	/* A read does not wait for another reader. */
	KUNIT_EXPECT_FALSE(test, dma_fence_is_signaled(&f->fence));
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);

	f = blob_fence_add(test, written, DMA_RESV_USAGE_WRITE);
	start = ktime_get();
	schedule_delayed_work(&f->signal,
			      msecs_to_jiffies(BLOB_FENCE_DELAY_MS));
	blob = blob_acquire(written, 0, BLOB_BUF_HALF,
			    CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &start, 1), 0);
	KUNIT_EXPECT_TRUE(test, dma_fence_is_signaled(&f->fence));
	KUNIT_EXPECT_GE(test, ktime_ms_delta(ktime_get(), start),
			BLOB_FENCE_DELAY_MS);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

/* A write waits for readers too, which would read what it overwrites. */
static void ctlv_blob_test_a_write_waits_for_everything(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *f;
	struct ctlv_blob *blob;
	ktime_t start;

	f = blob_fence_add(test, buf, DMA_RESV_USAGE_READ);
	start = ktime_get();
	schedule_delayed_work(&f->signal,
			      msecs_to_jiffies(BLOB_FENCE_DELAY_MS));
	blob = blob_acquire(buf, 0, BLOB_BUF_HALF,
			    CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_write(blob, 0, &start, 1), 0);
	KUNIT_EXPECT_TRUE(test, dma_fence_is_signaled(&f->fence));
	KUNIT_EXPECT_GE(test, ktime_ms_delta(ktime_get(), start),
			BLOB_FENCE_DELAY_MS);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

struct blob_dma_case {
	u8 direction;
	enum dma_data_direction dma_direction;
	bool writes;
};

static int blob_dma_complete(struct ctlv_blob_dma *dma,
			     struct blob_fence *fence)
{
	int ret;

	ret = ctlv_blob_dma_begin(dma, &fence->fence);
	if (!ret) {
		ctlv_blob_dma_submit_done(dma, 0);
		blob_fence_signal(fence, 0);
	}
	ctlv_blob_dma_unmap(dma, ret);
	return ret;
}

static void ctlv_blob_test_device_mapping_directions(struct kunit *test)
{
	static const struct blob_dma_case cases[] = {
		{ CTLV_BLOB_DIRECTION_INPUT, DMA_TO_DEVICE, false },
		{ CTLV_BLOB_DIRECTION_OUTPUT, DMA_FROM_DEVICE, true },
		{ CTLV_BLOB_DIRECTION_INOUT, DMA_BIDIRECTIONAL, true },
	};
	struct device *dev = kunit_device_register(test, "ctlv-blob");
	unsigned int i;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	for (i = 0; i < ARRAY_SIZE(cases); i++) {
		const struct blob_dma_case *c = &cases[i];
		struct blob_buffer *buf = blob_buffer_new(test);
		struct blob_fence *fence = blob_fence_new(test);
		struct ctlv_blob_dma *dma;
		struct ctlv_blob *blob;
		struct sg_table *sgt;
		bool read_signaled = true;
		bool write_signaled = true;
		u64 skip;
		int ret;

		blob = blob_acquire(buf, BLOB_BUF_HALF - 32, 64,
				    c->direction);
		KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
		dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
		KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
		KUNIT_EXPECT_NOT_NULL(test, sgt);
		KUNIT_EXPECT_EQ(test, skip, BLOB_BUF_HALF - 32);
		KUNIT_EXPECT_EQ(test, buf->attachments, 1);
		KUNIT_EXPECT_EQ(test, buf->device_maps, 1);
		KUNIT_EXPECT_EQ(test, buf->device_dir, c->dma_direction);

		ret = ctlv_blob_dma_begin(dma, &fence->fence);
		if (!ret) {
			ctlv_blob_dma_submit_done(dma, 0);
			read_signaled = blob_fences_signaled(buf, DMA_RESV_USAGE_READ);
			write_signaled = blob_fences_signaled(buf, DMA_RESV_USAGE_WRITE);
			blob_fence_signal(fence, 0);
		}
		ctlv_blob_dma_unmap(dma, ret);
		KUNIT_ASSERT_EQ(test, ret, 0);
		KUNIT_EXPECT_FALSE(test, read_signaled);
		KUNIT_EXPECT_EQ(test, write_signaled, !c->writes);
		KUNIT_EXPECT_TRUE(test,
				  blob_fences_signaled(buf,
						       DMA_RESV_USAGE_READ));
		KUNIT_EXPECT_EQ(test, buf->attachments, 0);
		KUNIT_EXPECT_EQ(test, buf->device_maps, 0);
		KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	}
}

static void ctlv_blob_test_device_mapping_dependencies(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-deps");
	struct blob_buffer *reading = blob_buffer_new(test);
	struct blob_buffer *written = blob_buffer_new(test);
	struct blob_buffer *overwritten = blob_buffer_new(test);
	struct blob_fence *read_blocker;
	struct blob_fence *write_blocker;
	struct blob_fence *completion;
	struct ctlv_blob_dma *dma;
	struct ctlv_blob *blob;
	struct sg_table *sgt;
	ktime_t start;
	u64 skip;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));

	read_blocker = blob_fence_add(test, reading, DMA_RESV_USAGE_READ);
	completion = blob_fence_new(test);
	blob = blob_acquire(reading, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	KUNIT_EXPECT_EQ(test, blob_dma_complete(dma, completion), 0);
	KUNIT_EXPECT_FALSE(test,
			   dma_fence_is_signaled(&read_blocker->fence));
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);

	write_blocker = blob_fence_add(test, written, DMA_RESV_USAGE_WRITE);
	completion = blob_fence_new(test);
	blob = blob_acquire(written, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	start = ktime_get();
	schedule_delayed_work(&write_blocker->signal,
			      msecs_to_jiffies(BLOB_FENCE_DELAY_MS));
	ret = blob_dma_complete(dma, completion);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_GE(test, ktime_ms_delta(ktime_get(), start),
			BLOB_FENCE_DELAY_MS);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);

	read_blocker = blob_fence_add(test, overwritten,
				      DMA_RESV_USAGE_READ);
	completion = blob_fence_new(test);
	blob = blob_acquire(overwritten, 0, 64,
			    CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	start = ktime_get();
	schedule_delayed_work(&read_blocker->signal,
			      msecs_to_jiffies(BLOB_FENCE_DELAY_MS));
	ret = blob_dma_complete(dma, completion);
	KUNIT_EXPECT_EQ(test, ret, 0);
	KUNIT_EXPECT_GE(test, ktime_ms_delta(ktime_get(), start),
			BLOB_FENCE_DELAY_MS);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

static void ctlv_blob_test_aliased_device_mappings(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-alias");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *first_fence = blob_fence_new(test);
	struct blob_fence *second_fence = blob_fence_new(test);
	struct ctlv_blob_dma *first_dma, *second_dma;
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob *first, *second;
	struct sg_table *sgt;
	u64 skip;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	ctlv_blobs_init(&ctx);
	first = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(first));
	ctlv_blob_track(first, &ctx);
	second = blob_acquire(buf, 64, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(second));
	ctlv_blob_track(second, &ctx);

	first_dma = ctlv_blob_dma_map(first, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(first_dma));
	second_dma = ctlv_blob_dma_map(second, dev, &sgt, &skip);
	KUNIT_EXPECT_EQ(test, PTR_ERR(second_dma), -EBUSY);
	KUNIT_EXPECT_EQ(test, buf->attachments, 1);
	KUNIT_EXPECT_EQ(test, buf->device_maps, 1);

	ret = ctlv_blob_dma_begin(first_dma, &first_fence->fence);
	if (!ret) {
		ctlv_blob_dma_submit_done(first_dma, 0);
		blob_fence_signal(first_fence, 0);
	}
	KUNIT_EXPECT_EQ(test, dma_fence_get_status(&first_fence->fence), 1);
	ctlv_blob_dma_unmap(first_dma, ret);
	KUNIT_EXPECT_EQ(test, ctx.blob_error, 0);
	KUNIT_ASSERT_EQ(test, ret, 0);
	second_dma = ctlv_blob_dma_map(second, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(second_dma));
	ret = ctlv_blob_dma_begin(second_dma, &second_fence->fence);
	if (!ret) {
		ctlv_blob_dma_submit_done(second_dma, 0);
		blob_fence_signal(second_fence, 0);
	}
	KUNIT_EXPECT_EQ(test, dma_fence_get_status(&second_fence->fence), 1);
	ctlv_blob_dma_unmap(second_dma, ret);
	KUNIT_EXPECT_EQ(test, ctx.blob_error, 0);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(first), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(second), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), 0);
}

static void ctlv_blob_test_cpu_and_device_access_are_serial(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-serial");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *fence = blob_fence_new(test);
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob_dma *dma;
	struct ctlv_blob *cpu, *device;
	struct sg_table *sgt;
	u64 skip;
	u8 byte;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	ctlv_blobs_init(&ctx);
	cpu = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(cpu));
	ctlv_blob_track(cpu, &ctx);
	device = blob_acquire(buf, 64, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(device));
	ctlv_blob_track(device, &ctx);

	KUNIT_EXPECT_EQ(test, ctlv_blob_read(cpu, 0, &byte, 1), 0);
	dma = ctlv_blob_dma_map(device, dev, &sgt, &skip);
	KUNIT_EXPECT_EQ(test, PTR_ERR(dma), -EBUSY);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(cpu), 0);

	dma = ctlv_blob_dma_map(device, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	cpu = blob_acquire(buf, 128, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(cpu));
	ctlv_blob_track(cpu, &ctx);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(cpu, 0, &byte, 1), -EBUSY);
	ret = ctlv_blob_dma_begin(dma, &fence->fence);
	if (!ret) {
		ctlv_blob_dma_submit_done(dma, 0);
		blob_fence_signal(fence, 0);
	}
	KUNIT_EXPECT_EQ(test, dma_fence_get_status(&fence->fence), 1);
	ctlv_blob_dma_unmap(dma, ret);
	KUNIT_EXPECT_EQ(test, ctx.blob_error, 0);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(cpu, 0, &byte, 1), 0);

	KUNIT_EXPECT_EQ(test, ctlv_blob_put(cpu), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(device), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), 0);
}

static void ctlv_blob_test_exporter_failures(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-fail");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *component;
	struct dma_fence *container;
	struct ctlv_blob_dma *dma;
	struct ctlv_blob *blob;
	struct sg_table *sgt;
	u64 skip;
	u8 byte;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));

	buf->fail_begin = -EIO;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &byte, 1), -EIO);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	buf->fail_begin = 0;

	buf->fail_vmap = -ENOMEM;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &byte, 1),
				-ENOMEM);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	buf->fail_vmap = 0;

	buf->fail_end = -EIO;
	buf->fail_end_count = 1;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &byte, 1), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), -EIO);
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	buf->fail_end = 0;

	buf->fail_attach = -ENODEV;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_EXPECT_EQ(test, PTR_ERR(dma), -ENODEV);
	KUNIT_EXPECT_EQ(test, buf->attachments, 0);
	KUNIT_EXPECT_EQ(test, buf->device_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	buf->fail_attach = 0;

	buf->fail_map = -EIO;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_EXPECT_EQ(test, PTR_ERR(dma), -EIO);
	KUNIT_EXPECT_EQ(test, buf->attachments, 0);
	KUNIT_EXPECT_EQ(test, buf->device_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	buf->fail_map = 0;

	sg_dma_len(buf->sgt.sgl) = 16;
	sg_dma_len(sg_next(buf->sgt.sgl)) = 16;
	blob = blob_acquire(buf, 16, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_EXPECT_EQ(test, PTR_ERR(dma), -EIO);
	KUNIT_EXPECT_EQ(test, buf->attachments, 0);
	KUNIT_EXPECT_EQ(test, buf->device_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	sg_dma_len(buf->sgt.sgl) = BLOB_BUF_HALF;
	sg_dma_len(sg_next(buf->sgt.sgl)) = BLOB_BUF_HALF;

	component = blob_fence_new(test);
	container = blob_fence_array_new(test, component);
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, ctlv_blob_dma_begin(dma, container),
				-EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	blob_fence_signal(component, 0);
	ctlv_blob_dma_unmap(dma, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

static void ctlv_blob_test_device_completion_error(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-error");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *fence = blob_fence_new(test);
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob_dma *dma;
	struct ctlv_blob *blob;
	struct sg_table *sgt;
	u64 skip;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	ctlv_blobs_init(&ctx);
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	ctlv_blob_track(blob, &ctx);
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	ret = ctlv_blob_dma_begin(dma, &fence->fence);
	if (!ret) {
		ctlv_blob_dma_submit_done(dma, 0);
		blob_fence_signal(fence, -EIO);
	}
	ctlv_blob_dma_unmap(dma, ret);
	KUNIT_ASSERT_EQ(test, ret, 0);

	KUNIT_EXPECT_EQ(test, dma_fence_get_status(&fence->fence), -EIO);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), -EIO);
}

static void ctlv_blob_test_submission_failure(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-submit-error");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *fence = blob_fence_new(test);
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob_dma *dma;
	struct ctlv_blob *blob;
	struct sg_table *sgt;
	bool signaled;
	u64 skip;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	ctlv_blobs_init(&ctx);
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	ctlv_blob_track(blob, &ctx);
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));
	ret = ctlv_blob_dma_begin(dma, &fence->fence);
	KUNIT_ASSERT_EQ(test, ret, 0);

	ctlv_blob_dma_submit_done(dma, -EIO);
	signaled = dma_fence_is_signaled(&fence->fence);
	KUNIT_EXPECT_TRUE(test, signaled);
	KUNIT_EXPECT_EQ(test, dma_fence_get_status(&fence->fence), -EIO);
	if (!signaled)
		blob_fence_signal(fence, 0);

	ctlv_blob_dma_unmap(dma, -EIO);
	KUNIT_EXPECT_EQ(test, buf->attachments, 0);
	KUNIT_EXPECT_EQ(test, buf->device_maps, 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), -EIO);
}

struct blob_waiter {
	struct dma_resv *resv;
	struct completion start;
	struct completion locked;
	struct completion done;
	long result;
};

static int blob_waiter_thread(void *arg)
{
	struct blob_waiter *waiter = arg;

	while (!wait_for_completion_timeout(&waiter->start,
					    msecs_to_jiffies(100)))
		if (kthread_should_stop())
			return 0;
	dma_resv_lock(waiter->resv, NULL);
	complete(&waiter->locked);
	waiter->result = dma_resv_wait_timeout(waiter->resv,
					       DMA_RESV_USAGE_READ, true,
					       MAX_SCHEDULE_TIMEOUT);
	dma_resv_unlock(waiter->resv);
	complete(&waiter->done);
	while (!kthread_should_stop())
		schedule_timeout_interruptible(MAX_SCHEDULE_TIMEOUT);
	return 0;
}

static void blob_waiter_stop(void *arg)
{
	kthread_stop(arg);
}

static void ctlv_blob_test_completion_does_not_need_resv(struct kunit *test)
{
	struct device *dev = kunit_device_register(test, "ctlv-blob-wait");
	struct blob_buffer *buf = blob_buffer_new(test);
	struct blob_fence *fence = blob_fence_new(test);
	struct blob_waiter waiter = { .resv = buf->dmabuf->resv };
	struct ctlv_blob_dma *dma;
	struct task_struct *task;
	struct ctlv_blob *blob;
	struct sg_table *sgt;
	unsigned long done;
	unsigned long locked;
	u64 skip;
	int status;
	int ret;

	KUNIT_ASSERT_FALSE(test, IS_ERR(dev));
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_OUTPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
	KUNIT_ASSERT_FALSE(test, IS_ERR(dma));

	init_completion(&waiter.start);
	init_completion(&waiter.locked);
	init_completion(&waiter.done);
	task = kthread_run(blob_waiter_thread, &waiter, "ctlv-blob-wait");
	KUNIT_ASSERT_FALSE(test, IS_ERR(task));
	ret = kunit_add_action_or_reset(test, blob_waiter_stop, task);
	KUNIT_ASSERT_EQ(test, ret, 0);

	ret = ctlv_blob_dma_begin(dma, &fence->fence);
	if (!ret)
		ctlv_blob_dma_submit_done(dma, 0);
	complete(&waiter.start);
	locked = wait_for_completion_timeout(&waiter.locked,
					     msecs_to_jiffies(5000));
	status = ret;
	if (!status && !locked)
		status = -ETIMEDOUT;
	blob_fence_signal(fence, status);
	ctlv_blob_dma_unmap(dma, status);
	done = wait_for_completion_timeout(&waiter.done,
					   msecs_to_jiffies(5000));
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_NE(test, locked, 0UL);
	KUNIT_ASSERT_NE(test, done, 0UL);
	KUNIT_EXPECT_GT(test, waiter.result, 0L);
	kunit_release_action(test, blob_waiter_stop, task);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), 0);
}

static void ctlv_blob_test_callback_completion(struct kunit *test)
{
	struct blob_buffer *buf = blob_buffer_new(test);
	struct ctlv_op_ctx ctx = {};
	struct ctlv_blob *blob;
	u8 byte;

	ctlv_blobs_init(&ctx);
	buf->fail_end = -EIO;
	buf->fail_end_count = 1;
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	ctlv_blob_track(blob, &ctx);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &byte, 1), 0);
	KUNIT_EXPECT_EQ(test, ctlv_blob_put(blob), -EIO);
	KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), -EIO);
	buf->fail_end = 0;

	ctlv_blobs_init(&ctx);
	blob = blob_acquire(buf, 0, 64, CTLV_BLOB_DIRECTION_INPUT);
	KUNIT_ASSERT_FALSE(test, IS_ERR(blob));
	ctlv_blob_track(blob, &ctx);
	KUNIT_EXPECT_EQ(test, ctlv_blob_read(blob, 0, &byte, 1), 0);
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, ctlv_blobs_finish(&ctx), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, buf->access, 0);
	KUNIT_EXPECT_EQ(test, buf->cpu_maps, 0);
	KUNIT_EXPECT_EQ(test, file_count(buf->dmabuf->file), 1);
}

/* One range, and the pages it should come to. */
struct blob_range {
	const char *name;
	u64 address;
	u64 len;
	unsigned long n_pages;
};

/* A page boundary is where an off-by-one in the count hides. */
static void ctlv_blob_test_the_pages_a_range_covers(struct kunit *test)
{
	static const struct blob_range ranges[] = {
		{ "one byte", PAGE_SIZE, 1, 1 },
		{ "a whole page", PAGE_SIZE, PAGE_SIZE, 1 },
		{ "a page and a byte", PAGE_SIZE, PAGE_SIZE + 1, 2 },
		{ "up to a boundary", PAGE_SIZE - 1, 1, 1 },
		{ "across a boundary", PAGE_SIZE - 1, 2, 2 },
		{ "a byte into the last of three", PAGE_SIZE - 1,
		  PAGE_SIZE + 2, 3 },
		{ "two whole pages", PAGE_SIZE, 2 * PAGE_SIZE, 2 },
		{ "two, one byte later", PAGE_SIZE + 1, 2 * PAGE_SIZE, 3 },
	};
	unsigned long first, n_pages;
	int i;

	for (i = 0; i < ARRAY_SIZE(ranges); i++) {
		const struct blob_range *r = &ranges[i];

		KUNIT_EXPECT_EQ_MSG(test,
				    ctlv_blob_va_range(r->address, r->len,
						       &first, &n_pages),
				    0, "%s", r->name);
		KUNIT_EXPECT_EQ_MSG(test, n_pages, r->n_pages, "%s", r->name);
		KUNIT_EXPECT_EQ_MSG(test, first, r->address & PAGE_MASK,
				    "%s", r->name);
	}

	/* A range of nothing is not a range at all. */
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(PAGE_SIZE, 0, &first, &n_pages),
			-EINVAL);
}

/*
 * Numbers that fit every field they arrived in and describe memory nobody has.
 * A short page count or a folded address would answer from memory never named.
 */
static void ctlv_blob_test_a_range_no_task_could_have(struct kunit *test)
{
	static const u64 lengths[] = {
		U64_MAX,
		/* The least length whose end, rounded up to a page, wraps. */
		U64_MAX - PAGE_SIZE + 2,
		/* And the greatest whose end does not. */
		U64_MAX - PAGE_SIZE + 1,
	};
	unsigned long first, n_pages;
	int i;

	for (i = 0; i < ARRAY_SIZE(lengths); i++)
		KUNIT_EXPECT_EQ_MSG(test,
				    ctlv_blob_va_range(0, lengths[i], &first,
						       &n_pages),
				    -EFAULT, "length %llu", lengths[i]);

	/* One byte more than the whole address space, from its first byte. */
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(0, (u64)TASK_SIZE + 1, &first,
					   &n_pages), -EFAULT);
	/* An address past the end of the address space, and the end itself. */
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(TASK_SIZE, PAGE_SIZE, &first,
					   &n_pages), -EFAULT);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(TASK_SIZE + PAGE_SIZE, PAGE_SIZE,
					   &first, &n_pages), -EFAULT);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(U64_MAX & PAGE_MASK, PAGE_SIZE,
					   &first, &n_pages), -EFAULT);
	/* Wherever the end lands, one before it is still inside the range. */
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(PAGE_SIZE, U64_MAX - PAGE_SIZE,
					   &first, &n_pages), -EFAULT);
}

/*
 * A count of pages is an unsigned long and a length is a u64, so which lengths
 * are refused depends on the word size. A count handed back is never short.
 */
static void ctlv_blob_test_a_count_of_pages_is_never_short(struct kunit *test)
{
	static const u64 lengths[] = {
		PAGE_SIZE,
		1ULL << 32,
		1ULL << 40,
		1ULL << 44,
		1ULL << 52,
		1ULL << 63,
	};
	unsigned long first, n_pages;
	int i;

	for (i = 0; i < ARRAY_SIZE(lengths); i++) {
		u64 len = lengths[i];
		int ret = ctlv_blob_va_range(0, len, &first, &n_pages);

		if (ret) {
			KUNIT_EXPECT_EQ_MSG(test, ret, -EFAULT, "%llu", len);
			continue;
		}
		/* Page-aligned from zero, so the count is exactly the pages. */
		KUNIT_EXPECT_EQ_MSG(test, (u64)n_pages, len >> PAGE_SHIFT,
				    "%llu", len);
	}
}

/*
 * An address is accepted whole or refused. One folded into a different address
 * would be pinned somewhere the caller does have, worse than an error.
 */
static void ctlv_blob_test_an_address_is_never_folded(struct kunit *test)
{
	static const u64 addresses[] = {
		PAGE_SIZE,
		PAGE_SIZE + 1,
		/* Bits an unsigned long has no room for, on 32 bits. */
		PAGE_SIZE | (1ULL << 32),
		PAGE_SIZE | (1ULL << 33),
		PAGE_SIZE | (1ULL << 47),
		PAGE_SIZE | (1ULL << 63),
	};
	unsigned long first, n_pages;
	int i;

	for (i = 0; i < ARRAY_SIZE(addresses); i++) {
		u64 address = addresses[i];
		int ret = ctlv_blob_va_range(address, PAGE_SIZE, &first,
					     &n_pages);

		if (ret) {
			KUNIT_EXPECT_EQ_MSG(test, ret, -EFAULT, "%llu",
					    address);
			continue;
		}
		KUNIT_EXPECT_EQ_MSG(test, (u64)first,
				    address & ~(u64)(PAGE_SIZE - 1),
				    "%llu", address);
		KUNIT_EXPECT_EQ_MSG(test, n_pages,
				    offset_in_page(address) ? 2 : 1, "%llu",
				    address);
	}
}

/*
 * A range ending on the last byte of the address space is the one range whose
 * end is not an address at all. It is accepted, and one byte more is not.
 */
static void ctlv_blob_test_a_range_that_ends_the_address_space(struct kunit *test)
{
	unsigned long first, n_pages;
	u64 last = TASK_SIZE - PAGE_SIZE;

	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(last, PAGE_SIZE, &first, &n_pages),
			0);
	KUNIT_EXPECT_EQ(test, first, last);
	KUNIT_EXPECT_EQ(test, n_pages, 1);

	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(last, PAGE_SIZE + 1, &first,
					   &n_pages), -EFAULT);
	KUNIT_EXPECT_EQ(test,
			ctlv_blob_va_range(last + 1, PAGE_SIZE, &first,
					   &n_pages), -EFAULT);
}

static struct kunit_case ctlv_blob_test_cases[] = {
	KUNIT_CASE(ctlv_blob_test_memfd_access),
	KUNIT_CASE(ctlv_blob_test_read_and_write),
	KUNIT_CASE(ctlv_blob_test_where_a_blob_begins),
	KUNIT_CASE(ctlv_blob_test_direction),
	KUNIT_CASE(ctlv_blob_test_a_copy_outside_the_blob),
	KUNIT_CASE(ctlv_blob_test_two_acquisitions),
	KUNIT_CASE(ctlv_blob_test_a_blob_outside_the_buffer),
	KUNIT_CASE(ctlv_blob_test_releasing_nothing),
	KUNIT_CASE(ctlv_blob_test_stale_cursor_is_rejected),
	KUNIT_CASE(ctlv_blob_test_a_read_waits_for_writers),
	KUNIT_CASE(ctlv_blob_test_a_write_waits_for_everything),
	KUNIT_CASE(ctlv_blob_test_device_mapping_directions),
	KUNIT_CASE(ctlv_blob_test_device_mapping_dependencies),
	KUNIT_CASE(ctlv_blob_test_aliased_device_mappings),
	KUNIT_CASE(ctlv_blob_test_cpu_and_device_access_are_serial),
	KUNIT_CASE(ctlv_blob_test_exporter_failures),
	KUNIT_CASE(ctlv_blob_test_device_completion_error),
	KUNIT_CASE(ctlv_blob_test_submission_failure),
	KUNIT_CASE(ctlv_blob_test_completion_does_not_need_resv),
	KUNIT_CASE(ctlv_blob_test_callback_completion),
	KUNIT_CASE(ctlv_blob_test_the_pages_a_range_covers),
	KUNIT_CASE(ctlv_blob_test_a_range_no_task_could_have),
	KUNIT_CASE(ctlv_blob_test_a_count_of_pages_is_never_short),
	KUNIT_CASE(ctlv_blob_test_an_address_is_never_folded),
	KUNIT_CASE(ctlv_blob_test_a_range_that_ends_the_address_space),
	{},
};

static struct kunit_suite ctlv_blob_test_suite = {
	.name = "ctlv_blob",
	.test_cases = ctlv_blob_test_cases,
};

kunit_test_suite(ctlv_blob_test_suite);
