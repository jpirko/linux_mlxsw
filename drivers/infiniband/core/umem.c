/*
 * Copyright (c) 2005 Topspin Communications.  All rights reserved.
 * Copyright (c) 2005 Cisco Systems.  All rights reserved.
 * Copyright (c) 2005 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2020 Intel Corporation. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/slab.h>
#include <linux/pagemap.h>
#include <linux/count_zeros.h>
#include <rdma/ib_umem_odp.h>

#include "uverbs.h"

#define RESCHED_LOOP_CNT_THRESHOLD 0x1000

static void __ib_umem_release(struct ib_device *dev, struct ib_umem *umem, int dirty)
{
	bool make_dirty = umem->writable && dirty;
	struct scatterlist *sg;
	unsigned int i;

	if (dirty)
		ib_dma_unmap_sgtable_attrs(dev, &umem->sgt_append.sgt,
					   DMA_BIDIRECTIONAL, umem->dma_attrs);

	for_each_sgtable_sg(&umem->sgt_append.sgt, sg, i) {
		unpin_user_page_range_dirty_lock(sg_page(sg),
			DIV_ROUND_UP(sg->length, PAGE_SIZE), make_dirty);

		if (i && !(i % RESCHED_LOOP_CNT_THRESHOLD))
			cond_resched();
	}

	sg_free_append_table(&umem->sgt_append);
}

/**
 * ib_umem_find_best_pgsz - Find best HW page size to use for this MR
 *
 * @umem: umem struct
 * @pgsz_bitmap: bitmap of HW supported page sizes
 * @virt: IOVA
 *
 * This helper is intended for HW that support multiple page
 * sizes but can do only a single page size in an MR.
 *
 * Returns 0 if the umem requires page sizes not supported by
 * the driver to be mapped. Drivers always supporting PAGE_SIZE
 * or smaller will never see a 0 result.
 */
unsigned long ib_umem_find_best_pgsz(struct ib_umem *umem,
				     unsigned long pgsz_bitmap,
				     unsigned long virt)
{
	unsigned long curr_len = 0;
	dma_addr_t curr_base = ~0;
	unsigned long va, pgoff;
	struct scatterlist *sg;
	dma_addr_t mask;
	dma_addr_t end;
	int i;

	umem->iova = va = virt;

	if (umem->is_odp) {
		unsigned int page_size = BIT(to_ib_umem_odp(umem)->page_shift);

		/* ODP must always be self consistent. */
		if (!(pgsz_bitmap & page_size))
			return 0;
		return page_size;
	}

	/* The best result is the smallest page size that results in the minimum
	 * number of required pages. Compute the largest page size that could
	 * work based on VA address bits that don't change.
	 */
	mask = pgsz_bitmap &
	       GENMASK(BITS_PER_LONG - 1,
		       bits_per((umem->length - 1 + virt) ^ virt));
	/* offset into first SGL */
	pgoff = umem->address & ~PAGE_MASK;

	for_each_sgtable_dma_sg(&umem->sgt_append.sgt, sg, i) {
		/* If the current entry is physically contiguous with the previous
		 * one, no need to take its start addresses into consideration.
		 */
		if (check_add_overflow(curr_base, curr_len, &end) ||
		    end != sg_dma_address(sg)) {

			curr_base = sg_dma_address(sg);
			curr_len = 0;

			/* Reduce max page size if VA/PA bits differ */
			mask |= (curr_base + pgoff) ^ va;

			/* The alignment of any VA matching a discontinuity point
			* in the physical memory sets the maximum possible page
			* size as this must be a starting point of a new page that
			* needs to be aligned.
			*/
			if (i != 0)
				mask |= va;
		}

		curr_len += sg_dma_len(sg);
		va += sg_dma_len(sg) - pgoff;

		pgoff = 0;
	}

	/* The mask accumulates 1's in each position where the VA and physical
	 * address differ, thus the length of trailing 0 is the largest page
	 * size that can pass the VA through to the physical.
	 */
	if (mask)
		pgsz_bitmap &= GENMASK(count_trailing_zeros(mask), 0);
	return pgsz_bitmap ? rounddown_pow_of_two(pgsz_bitmap) : 0;
}
EXPORT_SYMBOL(ib_umem_find_best_pgsz);

/**
 * ib_umem_get - Pin and DMA map userspace memory.
 *
 * @device: IB device to connect UMEM
 * @addr: userspace virtual address to start at
 * @size: length of region to pin
 * @access: IB_ACCESS_xxx flags for memory being pinned
 */
struct ib_umem *ib_umem_get(struct ib_device *device, unsigned long addr,
			    size_t size, int access)
{
	struct ib_umem *umem;
	struct page **page_list;
	unsigned long lock_limit;
	unsigned long new_pinned;
	unsigned long cur_base;
	struct mm_struct *mm;
	unsigned long npages;
	int pinned, ret;
	unsigned int gup_flags = FOLL_LONGTERM;

	if (device->cc_dma_bounce)
		return ERR_PTR(-EOPNOTSUPP);

	/*
	 * If the combination of the addr and size requested for this memory
	 * region causes an integer overflow, return error.
	 */
	if (((addr + size) < addr) ||
	    PAGE_ALIGN(addr + size) < (addr + size))
		return ERR_PTR(-EINVAL);

	if (!can_do_mlock())
		return ERR_PTR(-EPERM);

	if (access & IB_ACCESS_ON_DEMAND)
		return ERR_PTR(-EOPNOTSUPP);

	umem = kzalloc_obj(*umem);
	if (!umem)
		return ERR_PTR(-ENOMEM);
	umem->ibdev      = device;
	umem->length     = size;
	umem->address    = addr;
	/*
	 * Drivers should call ib_umem_find_best_pgsz() to set the iova
	 * correctly.
	 */
	umem->iova = addr;
	umem->writable   = ib_access_writable(access);
	umem->owning_mm = mm = current->mm;
	umem->dma_attrs = DMA_ATTR_REQUIRE_COHERENT;
	if (access & IB_ACCESS_RELAXED_ORDERING)
		umem->dma_attrs |= DMA_ATTR_WEAK_ORDERING;

	mmgrab(mm);

	page_list = (struct page **) __get_free_page(GFP_KERNEL);
	if (!page_list) {
		ret = -ENOMEM;
		goto umem_kfree;
	}

	npages = ib_umem_num_pages(umem);
	if (npages == 0 || npages > UINT_MAX) {
		ret = -EINVAL;
		goto out;
	}

	lock_limit = rlimit(RLIMIT_MEMLOCK) >> PAGE_SHIFT;

	new_pinned = atomic64_add_return(npages, &mm->pinned_vm);
	if (new_pinned > lock_limit && !capable(CAP_IPC_LOCK)) {
		atomic64_sub(npages, &mm->pinned_vm);
		ret = -ENOMEM;
		goto out;
	}

	cur_base = addr & PAGE_MASK;

	if (umem->writable)
		gup_flags |= FOLL_WRITE;

	while (npages) {
		cond_resched();
		pinned = pin_user_pages_fast(cur_base,
					  min_t(unsigned long, npages,
						PAGE_SIZE /
						sizeof(struct page *)),
					  gup_flags, page_list);
		if (pinned < 0) {
			ret = pinned;
			goto umem_release;
		}

		cur_base += pinned * PAGE_SIZE;
		npages -= pinned;
		ret = sg_alloc_append_table_from_pages(
			&umem->sgt_append, page_list, pinned, 0,
			pinned << PAGE_SHIFT, ib_dma_max_seg_size(device),
			npages, GFP_KERNEL);
		if (ret) {
			unpin_user_pages_dirty_lock(page_list, pinned, 0);
			goto umem_release;
		}
	}

	ret = ib_dma_map_sgtable_attrs(device, &umem->sgt_append.sgt,
				       DMA_BIDIRECTIONAL, umem->dma_attrs);
	if (ret)
		goto umem_release;
	goto out;

umem_release:
	__ib_umem_release(device, umem, 0);
	atomic64_sub(ib_umem_num_pages(umem), &mm->pinned_vm);
out:
	free_page((unsigned long) page_list);
umem_kfree:
	if (ret) {
		mmdrop(umem->owning_mm);
		kfree(umem);
	}
	return ret ? ERR_PTR(ret) : umem;
}
EXPORT_SYMBOL(ib_umem_get);

/**
 * ib_umem_release - release memory pinned with ib_umem_get
 * @umem: umem struct to release
 */
void ib_umem_release(struct ib_umem *umem)
{
	if (IS_ERR_OR_NULL(umem))
		return;
	if (umem->is_dmabuf)
		return ib_umem_dmabuf_release(to_ib_umem_dmabuf(umem));
	if (umem->is_odp)
		return ib_umem_odp_release(to_ib_umem_odp(umem));

	__ib_umem_release(umem->ibdev, umem, 1);

	atomic64_sub(ib_umem_num_pages(umem), &umem->owning_mm->pinned_vm);
	mmdrop(umem->owning_mm);
	kfree(umem);
}
EXPORT_SYMBOL(ib_umem_release);

/*
 * Copy from the given ib_umem's pages to the given buffer.
 *
 * umem - the umem to copy from
 * offset - offset to start copying from
 * dst - destination buffer
 * length - buffer length
 *
 * Returns 0 on success, or an error code.
 */
int ib_umem_copy_from(void *dst, struct ib_umem *umem, size_t offset,
		      size_t length)
{
	size_t end = offset + length;
	int ret;

	if (offset > umem->length || length > umem->length - offset) {
		pr_err("%s not in range. offset: %zd umem length: %zd end: %zd\n",
		       __func__, offset, umem->length, end);
		return -EINVAL;
	}

	ret = sg_pcopy_to_buffer(umem->sgt_append.sgt.sgl,
				 umem->sgt_append.sgt.orig_nents, dst, length,
				 offset + ib_umem_offset(umem));

	if (ret < 0)
		return ret;
	else if (ret != length)
		return -EINVAL;
	else
		return 0;
}
EXPORT_SYMBOL(ib_umem_copy_from);

struct ib_umem_list {
	unsigned int count; /* Total slots in the list. */
	unsigned long provided; /* Bitmask of slots provided by the user. */
	unsigned long loaded; /* Bitmask of slots loaded by the driver. */
	struct ib_umem *umems[] __counted_by(count);
};

/**
 * ib_umem_from_buffer_desc - Create a umem from a buffer descriptor
 * @device: IB device
 * @desc: buffer descriptor describing a dma-buf or user VA region
 *
 * Return: umem pointer, or ERR_PTR on failure.
 */
struct ib_umem *ib_umem_from_buffer_desc(struct ib_device *device,
					 const struct ib_uverbs_buffer_desc *desc)
{
	struct ib_umem_dmabuf *umem_dmabuf;

	if (desc->reserved)
		return ERR_PTR(-EINVAL);

	switch (desc->type) {
	case IB_UVERBS_BUFFER_TYPE_DMABUF:
		umem_dmabuf = ib_umem_dmabuf_get_pinned(device, desc->addr,
							desc->length, desc->fd,
							IB_ACCESS_LOCAL_WRITE);
		if (IS_ERR(umem_dmabuf))
			return ERR_CAST(umem_dmabuf);
		return &umem_dmabuf->umem;
	case IB_UVERBS_BUFFER_TYPE_VA:
		return ib_umem_get(device, desc->addr, desc->length,
				   IB_ACCESS_LOCAL_WRITE);
	default:
		return ERR_PTR(-EINVAL);
	}
}
EXPORT_SYMBOL(ib_umem_from_buffer_desc);

/**
 * ib_umem_list_create - Create a umem list from a buffers attribute
 * @device: IB device
 * @attrs: uverbs attribute bundle
 * @attr_id: per-command attribute ID carrying the buffer descriptors
 * @slot_max: highest buffer slot index (count = slot_max + 1)
 *
 * Return: umem list, or ERR_PTR on failure.
 */
struct ib_umem_list *ib_umem_list_create(struct ib_device *device,
					 const struct uverbs_attr_bundle *attrs,
					 u16 attr_id,
					 unsigned int slot_max)
{
	const struct ib_uverbs_buffer_desc *descs;
	struct ib_umem_list *list;
	struct ib_umem *umem;
	unsigned int count;
	int num_descs;
	int err;
	int i;

	if (WARN_ON_ONCE(slot_max >= BITS_PER_LONG))
		return ERR_PTR(-EINVAL);
	count = slot_max + 1;

	num_descs = uverbs_attr_ptr_get_array_size(attrs, attr_id,
						   sizeof(*descs));
	if (num_descs == -ENOENT) {
		num_descs = 0;
		descs = NULL;
	} else if (num_descs < 0) {
		return ERR_PTR(num_descs);
	} else if (num_descs > count) {
		return ERR_PTR(-EINVAL);
	} else {
		descs = uverbs_attr_get_alloced_ptr(attrs, attr_id);
		if (IS_ERR(descs))
			return ERR_CAST(descs);
	}

	list = kzalloc(struct_size(list, umems, count), GFP_KERNEL);
	if (!list)
		return ERR_PTR(-ENOMEM);
	list->count = count;

	for (i = 0; i < num_descs; i++) {
		unsigned int idx = descs[i].index;

		if (idx >= count || (list->provided & BIT(idx))) {
			err = -EINVAL;
			goto err_release;
		}

		umem = ib_umem_from_buffer_desc(device, &descs[i]);
		if (IS_ERR(umem)) {
			err = PTR_ERR(umem);
			goto err_release;
		}
		list->umems[idx] = umem;
		list->provided |= BIT(idx);
	}

	return list;

err_release:
	ib_umem_list_release(list);
	return ERR_PTR(err);
}
EXPORT_SYMBOL(ib_umem_list_create);

/**
 * ib_umem_list_release - Release all umems in the list and free it
 * @list: umem list
 */
void ib_umem_list_release(struct ib_umem_list *list)
{
	int i;

	if (!list)
		return;
	for (i = 0; i < list->count; i++)
		ib_umem_release(list->umems[i]);
	kfree(list);
}
EXPORT_SYMBOL(ib_umem_list_release);

/**
 * ib_umem_list_check_consumed - Verify all provided umems were loaded
 * @list: umem list
 *
 * Return: 0 if all provided slots were loaded, -EINVAL otherwise.
 */
int ib_umem_list_check_consumed(const struct ib_umem_list *list)
{
	return (list->provided & ~list->loaded) == 0 ? 0 : -EINVAL;
}
EXPORT_SYMBOL(ib_umem_list_check_consumed);

/**
 * ib_umem_list_insert - Insert a umem into the list at a given index
 * @list: umem list
 * @index: per-command buffer slot index
 * @umem: umem pointer to store
 *
 * Stores @umem at @index (replacing any existing). For use from create_cq
 * when the buffer comes from legacy ATTRs rather than the buffer list.
 */
void ib_umem_list_insert(struct ib_umem_list *list, unsigned int index,
			 struct ib_umem *umem)
{
	ib_umem_list_replace(list, index, umem);
	if (umem)
		list->provided |= BIT(index);
}
EXPORT_SYMBOL(ib_umem_list_insert);

/**
 * ib_umem_list_load - Load a umem from the list by index
 * @list: umem list (may be NULL)
 * @index: per-command buffer slot index
 * @size: minimum required umem length
 *
 * Return: umem pointer, or NULL if the slot is empty or
 * the slot is out of bounds, or ERR_PTR(-EINVAL) if the umem is too small.
 */
struct ib_umem *ib_umem_list_load(struct ib_umem_list *list,
				 unsigned int index, size_t size)
{
	struct ib_umem *umem;

	if (!list || index >= list->count)
		return NULL;
	umem = list->umems[index];
	if (!umem)
		return NULL;
	if (umem->length < size)
		return ERR_PTR(-EINVAL);
	list->loaded |= BIT(index);
	return umem;
}
EXPORT_SYMBOL(ib_umem_list_load);

/**
 * ib_umem_list_load_or_get - Umem from list or pin user memory
 * @list: umem list (may be NULL)
 * @index: per-command buffer slot index
 * @device: IB device for ib_umem_get when the list slot is empty
 * @addr: user virtual address for ib_umem_get
 * @size: length for ib_umem_get
 * @access: access flags for ib_umem_get
 *
 * If @list has a umem at @index, returns it like ib_umem_list_load() (and
 * marks the slot loaded). Otherwise calls ib_umem_get() with the given
 * @access flags and on success stores the result at @index when
 * @list is non-NULL.
 *
 * Return: valid umem pointer, or ERR_PTR.
 */
struct ib_umem *ib_umem_list_load_or_get(struct ib_umem_list *list,
					 unsigned int index,
					 struct ib_device *device,
					 unsigned long addr, size_t size,
					 int access)
{
	struct ib_umem *umem;

	umem = ib_umem_list_load(list, index, size);
	if (IS_ERR(umem) || umem)
		return umem;
	umem = ib_umem_get(device, addr, size, access);
	if (IS_ERR(umem))
		return umem;
	if (list && index < list->count)
		list->umems[index] = umem;
	return umem;
}
EXPORT_SYMBOL(ib_umem_list_load_or_get);

/**
 * ib_umem_list_replace - Replace umem at index, releasing the previous one
 * @list: umem list (may be NULL)
 * @index: per-command buffer slot index
 * @umem: new umem pointer (may be NULL to clear the slot)
 *
 * Stores @umem at @index. If a different umem was already stored there, it is
 * released. Used for CQ resize and similar.
 */
void ib_umem_list_replace(struct ib_umem_list *list, unsigned int index,
			  struct ib_umem *umem)
{
	struct ib_umem *old;

	if (!list || index >= list->count)
		return;
	old = list->umems[index];
	list->umems[index] = umem;
	if (old && old != umem)
		ib_umem_release(old);
}
EXPORT_SYMBOL(ib_umem_list_replace);

/**
 * ib_umem_release_non_listed - Release a umem that is not stored in the list
 * @list: umem list
 * @index: per-command buffer slot index
 * @umem: umem pointer to release
 *
 * Releases @umem if it is not stored in @list.
 */
void ib_umem_release_non_listed(struct ib_umem_list *list, unsigned int index,
				struct ib_umem *umem)
{
	if (!list || index >= list->count || list->umems[index] != umem)
		ib_umem_release(umem);
}
EXPORT_SYMBOL(ib_umem_release_non_listed);
