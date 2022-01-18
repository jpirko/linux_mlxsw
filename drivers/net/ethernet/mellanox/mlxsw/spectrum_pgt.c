// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2022 Mellanox Technologies. All rights reserved */

#include <linux/refcount.h>
#include <linux/idr.h>

#include "spectrum.h"
#include "reg.h"

struct mlxsw_sp_pgt {
	struct idr pgt_idr;
	u16 max_index;
	struct mutex lock;
};

struct mlxsw_sp_pgt_entry {
	struct list_head ports_list;
	u16 index;
	u16 smpe_index;
};

struct mlxsw_pgt_entry_port {
	struct list_head list;
	u16 local_port;
	refcount_t refcount;
};

#define MLXSW_SP_PGT_START_INDEX 0

int mlxsw_sp_pgt_init(struct mlxsw_sp *mlxsw_sp)
{
	struct mlxsw_sp_pgt *pgt;

	pgt = kzalloc(sizeof(*mlxsw_sp->pgt), GFP_KERNEL);
	if (!pgt)
		return -ENOMEM;

	idr_init(&pgt->pgt_idr);
	pgt->max_index = MLXSW_CORE_RES_GET(mlxsw_sp->core, PGT_SIZE);
	mutex_init(&pgt->lock);
	mlxsw_sp->pgt = pgt;
	return 0;
}

void mlxsw_sp_pgt_fini(struct mlxsw_sp *mlxsw_sp)
{
	mutex_destroy(&mlxsw_sp->pgt->lock);
	WARN_ON(!idr_is_empty(&mlxsw_sp->pgt->pgt_idr));
	idr_destroy(&mlxsw_sp->pgt->pgt_idr);
	kfree(mlxsw_sp->pgt);
}

static int mlxsw_sp_pgt_free_id(int id, void *ptr, void *data)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	struct idr *idr;

	if (ptr) {
		pgt_entry = (struct mlxsw_sp_pgt_entry *)ptr;
		kfree(pgt_entry);
	}

	idr = (struct idr *)data;
	idr_remove(idr, id);
	return 0;
}

int mlxsw_sp_pgt_alloc_range(struct mlxsw_sp *mlxsw_sp, u16 mid_base, u16 count)
{
	int i, index, err = 0;

	mutex_lock(&mlxsw_sp->pgt->lock);
	for (i = 0; i < count; i++) {
		index = idr_alloc_cyclic(&mlxsw_sp->pgt->pgt_idr, NULL,
					 mid_base, mlxsw_sp->pgt->max_index, 0);
		if (index < 0) {
			err = index;
			goto err_idr_alloc_cyclic;
		}
	}

	goto out;

err_idr_alloc_cyclic:
	idr_for_each(&mlxsw_sp->pgt->pgt_idr, &mlxsw_sp_pgt_free_id,
		     &mlxsw_sp->pgt->pgt_idr);
out:
	mutex_unlock(&mlxsw_sp->pgt->lock);
	return err;
}

int mlxsw_sp_pgt_alloc(struct mlxsw_sp *mlxsw_sp, u16 *p_mid)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	int index, err;

	pgt_entry = kzalloc(sizeof(*pgt_entry), GFP_KERNEL);
	if (!pgt_entry)
		return -ENOMEM;

	mutex_lock(&mlxsw_sp->pgt->lock);
	index = idr_alloc(&mlxsw_sp->pgt->pgt_idr, pgt_entry,
			  MLXSW_SP_PGT_START_INDEX, mlxsw_sp->pgt->max_index,
			  0);
	mutex_unlock(&mlxsw_sp->pgt->lock);

	if (index < 0) {
		err = index;
		goto err_idr_alloc;
	}

	INIT_LIST_HEAD(&pgt_entry->ports_list);
	pgt_entry->index = index;
	*p_mid = index;
	return 0;

err_idr_alloc:
	kfree(pgt_entry);
	return err;
}

void mlxsw_sp_pgt_free(struct mlxsw_sp *mlxsw_sp, u16 mid_base, u16 count)
{
	struct idr *pgt_idr = &mlxsw_sp->pgt->pgt_idr;
	struct mlxsw_sp_pgt_entry *pgt_entry;
	unsigned long id;
	int i;

	for (i = 0; i < count; i++) {
		id = mid_base + i;

		mutex_lock(&mlxsw_sp->pgt->lock);
		pgt_entry = (struct mlxsw_sp_pgt_entry *)idr_find(pgt_idr, id);
		idr_remove(pgt_idr, id);
		mutex_unlock(&mlxsw_sp->pgt->lock);

		if (pgt_entry) {
			WARN_ON_ONCE(!list_empty(&pgt_entry->ports_list));
			kfree(pgt_entry);
		}
	}
}
