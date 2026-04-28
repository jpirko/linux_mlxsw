/*
 * Copyright (c) 2013-2015, Mellanox Technologies. All rights reserved.
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

#include <linux/kref.h>
#include <linux/slab.h>
#include <linux/sched/mm.h>
#include <rdma/ib_umem.h>

#include "mlx5_ib.h"

struct mlx5_ib_user_db_page {
	struct list_head	list;
	struct ib_umem	       *umem;
	unsigned long		user_virt;
	int			refcnt;
	struct mm_struct	*mm;
};

int mlx5_ib_db_map_user(struct mlx5_ib_ucontext *context,
			struct ib_udata *udata, u16 attr_id,
			unsigned long virt, struct mlx5_db *db)
{
	struct mlx5_ib_user_db_page *page = NULL;
	unsigned long dma_offset;
	int err = 0;

	if (udata) {
		struct ib_umem *umem;

		umem = ib_umem_get_attr(context->ibucontext.device, udata,
					attr_id, sizeof(__be32) * 2, 0);
		if (IS_ERR(umem))
			return PTR_ERR(umem);
		if (umem) {
			/*
			 * The 8-byte DBR is programmed to the device as one
			 * DMA address, so it must stay within a single page.
			 * An 8-byte range that crosses a page boundary may
			 * be split across two non-contiguous DMA mappings.
			 */
			if (ib_umem_offset(umem) >
			    PAGE_SIZE - sizeof(__be32) * 2) {
				ib_umem_release(umem);
				return -EINVAL;
			}
			page = kzalloc_obj(*page);
			if (!page) {
				ib_umem_release(umem);
				return -ENOMEM;
			}
			page->umem = umem;
			dma_offset = ib_umem_offset(umem);
		}
	}

	mutex_lock(&context->db_page_mutex);

	if (page)
		goto add_page;

	dma_offset = virt & ~PAGE_MASK;

	list_for_each_entry(page, &context->db_page_list, list)
		if ((current->mm == page->mm) &&
		    (page->user_virt == (virt & PAGE_MASK)))
			goto found;

	page = kzalloc_obj(*page);
	if (!page) {
		err = -ENOMEM;
		goto out;
	}

	page->user_virt = (virt & PAGE_MASK);
	page->refcnt    = 0;
	page->umem = ib_umem_get_va(context->ibucontext.device,
				    virt & PAGE_MASK, PAGE_SIZE, 0);
	if (IS_ERR(page->umem)) {
		err = PTR_ERR(page->umem);
		kfree(page);
		goto out;
	}
	mmgrab(current->mm);
	page->mm = current->mm;

add_page:
	list_add(&page->list, &context->db_page_list);

found:
	db->dma = sg_dma_address(page->umem->sgt_append.sgt.sgl) + dma_offset;
	db->u.user_page = page;
	++page->refcnt;

out:
	mutex_unlock(&context->db_page_mutex);

	return err;
}

void mlx5_ib_db_unmap_user(struct mlx5_ib_ucontext *context, struct mlx5_db *db)
{
	mutex_lock(&context->db_page_mutex);

	if (!--db->u.user_page->refcnt) {
		list_del(&db->u.user_page->list);
		if (db->u.user_page->mm)
			mmdrop(db->u.user_page->mm);
		ib_umem_release(db->u.user_page->umem);
		kfree(db->u.user_page);
	}

	mutex_unlock(&context->db_page_mutex);
}
