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
	bool smpe_index_valid;
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

static void mlxsw_sp1_pgt_smpe_index_valid_set(struct mlxsw_sp_pgt *pgt)
{
	pgt->smpe_index_valid = true;
}

static void mlxsw_sp2_pgt_smpe_index_valid_set(struct mlxsw_sp_pgt *pgt)
{
	pgt->smpe_index_valid = false;
}

struct mlxsw_sp_pgt_ops {
	void (*smpe_index_valid_set)(struct mlxsw_sp_pgt *pgt);
};

const struct mlxsw_sp_pgt_ops mlxsw_sp1_pgt_ops = {
	.smpe_index_valid_set = mlxsw_sp1_pgt_smpe_index_valid_set,
};

const struct mlxsw_sp_pgt_ops mlxsw_sp2_pgt_ops = {
	.smpe_index_valid_set = mlxsw_sp2_pgt_smpe_index_valid_set,
};

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
	mlxsw_sp->pgt_ops->smpe_index_valid_set(mlxsw_sp->pgt);
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

static struct mlxsw_pgt_entry_port *
mlxsw_sp_pgt_entry_port_lookup(struct mlxsw_sp_pgt_entry *pgt_entry,
			       u16 local_port)
{
	struct mlxsw_pgt_entry_port *pgt_entry_port;

	list_for_each_entry(pgt_entry_port, &pgt_entry->ports_list, list) {
		if (pgt_entry_port->local_port == local_port)
			return pgt_entry_port;
	}

	return NULL;
}

static int mlxsw_sp_pgt_entry_port_add(struct mlxsw_sp_pgt_entry *pgt_entry,
				       u16 local_port,
				       bool *p_require_hw_update)
{
	struct mlxsw_pgt_entry_port *pgt_entry_port;

	pgt_entry_port = mlxsw_sp_pgt_entry_port_lookup(pgt_entry, local_port);
	if (pgt_entry_port) {
		refcount_inc(&pgt_entry_port->refcount);
		*p_require_hw_update = false;
		return 0;
	}

	pgt_entry_port = kzalloc(sizeof(*pgt_entry_port), GFP_KERNEL);
	if (!pgt_entry_port)
		return -ENOMEM;

	pgt_entry_port->local_port = local_port;
	refcount_set(&pgt_entry_port->refcount, 1);
	list_add(&pgt_entry_port->list, &pgt_entry->ports_list);
	*p_require_hw_update = true;

	return 0;
}

static void mlxsw_sp_pgt_entry_port_del(struct mlxsw_sp_pgt_entry *pgt_entry,
					u16 local_port,
					bool *p_require_hw_update)
{
	struct mlxsw_pgt_entry_port *pgt_entry_port;
	*p_require_hw_update = false;

	pgt_entry_port = mlxsw_sp_pgt_entry_port_lookup(pgt_entry, local_port);
	if (!pgt_entry_port)
		/* The function can be called for pgt_entry_port that does not
		 * exist, trying to set local_port as no member.
		 * For such case, just return.
		 */
		return;

	if (refcount_dec_and_test(&pgt_entry_port->refcount)) {
		list_del(&pgt_entry_port->list);
		kfree(pgt_entry_port);
		*p_require_hw_update = true;
		return;
	}
}

static struct mlxsw_sp_pgt_entry *
mlxsw_sp_pgt_entry_create(struct mlxsw_sp_pgt *pgt, u16 mid)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	void *ret;

	pgt_entry = kzalloc(sizeof(*pgt_entry), GFP_KERNEL);
	if (!pgt_entry)
		return ERR_PTR(-ENOMEM);

	ret = idr_replace(&pgt->pgt_idr, pgt_entry, mid);

	if (WARN_ON(PTR_ERR(ret) == -ENOENT))
		/* There is no such id, warn and return an error. */
		goto err_idr_replace;

	if (PTR_ERR(ret) == -EINVAL)
		/* pgt_entry is invalid. */
		goto err_idr_replace;

	INIT_LIST_HEAD(&pgt_entry->ports_list);
	pgt_entry->index = mid;
	return pgt_entry;

err_idr_replace:
	kfree(pgt_entry);
	return ret;
}

static void mlxsw_sp_pgt_entry_remove(struct mlxsw_sp_pgt *pgt, u16 mid)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;

	pgt_entry = idr_replace(&pgt->pgt_idr, NULL, mid);
	if (pgt_entry)
		kfree(pgt_entry);
}

static struct mlxsw_sp_pgt_entry *
mlxsw_sp_pgt_entry_get(struct mlxsw_sp_pgt *pgt, u16 mid, bool create,
		       bool *p_new_entry)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	*p_new_entry = false;

	pgt_entry = (struct mlxsw_sp_pgt_entry *)idr_find(&pgt->pgt_idr, mid);
	if (pgt_entry)
		return pgt_entry;

	if (!create)
		return NULL;

	*p_new_entry = true;
	return mlxsw_sp_pgt_entry_create(pgt, mid);
}

static int
__mlxsw_sp_pgt_entry_port_add_del(struct mlxsw_sp_pgt_entry *pgt_entry,
				  u16 local_port, bool member,
				  bool *p_require_hw_update)
{
	if (member)
		return mlxsw_sp_pgt_entry_port_add(pgt_entry, local_port,
						   p_require_hw_update);

	mlxsw_sp_pgt_entry_port_del(pgt_entry, local_port, p_require_hw_update);
	return 0;
}

int mlxsw_sp_pgt_entry_port_set(struct mlxsw_sp *mlxsw_sp, u16 mid, u16 smpe,
				u16 local_port, bool member)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	bool new_entry, require_hw_update;
	char *smid2_pl;
	int err = 0;

	mutex_lock(&mlxsw_sp->pgt->lock);

	pgt_entry = mlxsw_sp_pgt_entry_get(mlxsw_sp->pgt, mid, member,
					   &new_entry);
	if (IS_ERR_OR_NULL(pgt_entry)) {
		err = PTR_ERR(pgt_entry);
		goto out;
	}

	err = __mlxsw_sp_pgt_entry_port_add_del(pgt_entry, local_port, member,
						&require_hw_update);
	if (err)
		goto err_port_add_del;

	if (!require_hw_update)
		goto out;

	smid2_pl = kmalloc(MLXSW_REG_SMID2_LEN, GFP_KERNEL);
	if (!smid2_pl) {
		err = -ENOMEM;
		goto err_smid2_pl_alloc;
	}

	if (mlxsw_sp->ubridge)
		mlxsw_reg_smid2_smpe_pack(smid2_pl, mid, local_port, member,
					  mlxsw_sp->pgt->smpe_index_valid,
					  smpe);
	else
		/* SMPE fields are reserved using ubridge0. */
		mlxsw_reg_smid2_pack(smid2_pl, mid, local_port, member);

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(smid2), smid2_pl);
	kfree(smid2_pl);
	goto out;

err_smid2_pl_alloc:
	__mlxsw_sp_pgt_entry_port_add_del(pgt_entry, local_port, !member,
					  &require_hw_update);
err_port_add_del:
	if (new_entry)
		mlxsw_sp_pgt_entry_remove(mlxsw_sp->pgt, mid);
out:
	mutex_unlock(&mlxsw_sp->pgt->lock);
	return err;
}

int
mlxsw_sp_pgt_entry_port_range_set(struct mlxsw_sp *mlxsw_sp, u16 mid, u16 smpe,
				  long *ports_bitmap, bool member)
{
	struct mlxsw_sp_pgt_entry *pgt_entry;
	bool new_entry, require_hw_update;
	unsigned int max_ports;
	char *smid2_pl;
	int i, err = 0;

	mutex_lock(&mlxsw_sp->pgt->lock);
	pgt_entry = mlxsw_sp_pgt_entry_get(mlxsw_sp->pgt, mid, member,
					   &new_entry);
	if (IS_ERR_OR_NULL(pgt_entry)) {
		err = PTR_ERR(pgt_entry);
		goto err_pgt_entry_get;
	}

	smid2_pl = kzalloc(MLXSW_REG_SMID2_LEN, GFP_KERNEL);
	if (!smid2_pl) {
		err = -ENOMEM;
		goto err_smid2_pl_alloc;
	}

	if (mlxsw_sp->ubridge)
		mlxsw_reg_smid2_smpe_pack(smid2_pl, mid, 0, 0,
					  mlxsw_sp->pgt->smpe_index_valid,
					  smpe);
	else
		/* SMPE fields are reserved using ubridge0. */
		mlxsw_reg_smid2_pack(smid2_pl, mid, 0, 0);

	max_ports = mlxsw_core_max_ports(mlxsw_sp->core);

	for_each_set_bit(i, ports_bitmap, max_ports) {
		err = __mlxsw_sp_pgt_entry_port_add_del(pgt_entry, i, member,
							&require_hw_update);
		if (err) {
			max_ports = i;
			goto err_pgt_entry_port_add_del;
		}

		if (!require_hw_update)
			continue;

		mlxsw_reg_smid2_port_set(smid2_pl, i, member);
		mlxsw_reg_smid2_port_mask_set(smid2_pl, i, 1);
	}

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(smid2), smid2_pl);
	if (err)
		goto err_reg_write;

	goto out;

err_reg_write:
err_pgt_entry_port_add_del:
	for_each_set_bit(i, ports_bitmap, max_ports)
		__mlxsw_sp_pgt_entry_port_add_del(pgt_entry, i, !member,
						  &require_hw_update);
	if (new_entry)
		mlxsw_sp_pgt_entry_remove(mlxsw_sp->pgt, mid);
out:
	kfree(smid2_pl);
err_smid2_pl_alloc:
err_pgt_entry_get:
	mutex_unlock(&mlxsw_sp->pgt->lock);
	return err;
}

bool mlxsw_sp_pgt_is_ports_list_empty(struct mlxsw_sp *mlxsw_sp, u16 mid)
{
	struct idr *pgt_idr = &mlxsw_sp->pgt->pgt_idr;
	struct mlxsw_sp_pgt_entry *pgt_entry;

	pgt_entry = (struct mlxsw_sp_pgt_entry *)idr_find(pgt_idr, mid);
	if (WARN_ON(!pgt_entry))
		return true;

	return list_empty(&pgt_entry->ports_list);
}

bool mlxsw_sp_pgt_is_port_in_mid_entry(struct mlxsw_sp *mlxsw_sp, u16 mid,
				       u16 local_port)
{
	struct idr *pgt_idr = &mlxsw_sp->pgt->pgt_idr;
	struct mlxsw_pgt_entry_port *pgt_entry_port;
	struct mlxsw_sp_pgt_entry *pgt_entry;

	pgt_entry = (struct mlxsw_sp_pgt_entry *)idr_find(pgt_idr, mid);
	if (WARN_ON(!pgt_entry))
		return false;

	list_for_each_entry(pgt_entry_port, &pgt_entry->ports_list, list) {
		if (pgt_entry_port->local_port == local_port)
			return true;
	}

	return false;
}
