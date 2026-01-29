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
	struct ib_umem_list     *umem_list;
	unsigned int		dbr_index;
	unsigned long		user_virt;
	int			refcnt;
	struct mm_struct	*mm;
};

int mlx5_ib_db_map_user(struct mlx5_ib_ucontext *context, unsigned long virt,
			struct ib_umem_list *umem_list, unsigned int dbr_index,
			size_t dbr_size, struct mlx5_db *db)
{
	unsigned long dma_offset;
	struct mlx5_ib_user_db_page *page;
	struct ib_umem *umem;
	int err = 0;

	mutex_lock(&context->db_page_mutex);

	umem = ib_umem_list_load(umem_list, dbr_index, dbr_size);
	if (IS_ERR(umem)) {
		err = PTR_ERR(umem);
		goto out;
	} else if (umem) {
		/* External umem path - no page sharing */
		page = kzalloc_obj(*page);
		if (!page) {
			err = -ENOMEM;
			goto out;
		}

		page->umem = umem;
		page->umem_list = umem_list;
		page->dbr_index = dbr_index;
		dma_offset = ib_umem_offset(umem);
		goto add_page;
	}

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
	page->umem = ib_umem_get(context->ibucontext.device, virt & PAGE_MASK,
				 PAGE_SIZE, 0);
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
		ib_umem_release_non_listed(db->u.user_page->umem_list,
					  db->u.user_page->dbr_index,
					  db->u.user_page->umem);
		kfree(db->u.user_page);
	}

	mutex_unlock(&context->db_page_mutex);
}
