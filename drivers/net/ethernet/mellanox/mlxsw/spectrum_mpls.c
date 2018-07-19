/*
 * drivers/net/ethernet/mellanox/mlxsw/spectrum_mpls.c
 * Copyright (c) 2017 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2017 Arkadi Sharshevsky <arkadis@mellanox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <linux/rhashtable.h>
#include <net/arp.h>
#include <net/ndisc.h>

#include "spectrum_mpls.h"

struct mlxsw_sp_fib_mpls_entry {
	struct mlxsw_sp_fib_entry common;
	struct rhash_head ht_node;
	u32 label;
};

static const struct rhashtable_params mlxsw_sp_mpls_route_ht_params = {
	.key_len = sizeof(u32),
	.key_offset = offsetof(struct mlxsw_sp_fib_mpls_entry, label),
	.head_offset = offsetof(struct mlxsw_sp_fib_mpls_entry, ht_node),
	.automatic_shrinking = true,
};

static int mlxsw_sp_fib_mpls_insert(struct mlxsw_sp *mlxsw_sp,
				    struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry)
{
	return rhashtable_insert_fast(&mlxsw_sp->router->mpls_ht,
				      &fib_mpls_entry->ht_node,
				      mlxsw_sp_mpls_route_ht_params);
}

static void mlxsw_sp_fib_mpls_remove(struct mlxsw_sp *mlxsw_sp,
				     struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry)
{
	rhashtable_remove_fast(&mlxsw_sp->router->mpls_ht,
			       &fib_mpls_entry->ht_node,
			       mlxsw_sp_mpls_route_ht_params);
}

static struct mlxsw_sp_fib_mpls_entry *
mlxsw_sp_fib_mpls_lookup(struct mlxsw_sp *mlxsw_sp, u32 label)
{
	return rhashtable_lookup_fast(&mlxsw_sp->router->mpls_ht, &label,
				      mlxsw_sp_mpls_route_ht_params);
}

struct mpls_nh *mlxsw_sp_nexthop_mpls_get_by_index(struct mpls_route *rt, int i)
{
	struct mpls_nh *mpls_nh = NULL;

	if (i >= rt->rt_nhn)
		return NULL;

	for_nexthops(rt) {
		mpls_nh = nh;
		if (nhsel < i)
			continue;
		break;
	} endfor_nexthops(rt);

	return mpls_nh;
}

static void
mlxsw_sp_fib_mpls_entry_offload_set(struct mlxsw_sp_fib_entry *fib_entry)
{
	struct mlxsw_sp_nexthop_group *nh_grp = fib_entry->nh_group;
	struct mpls_route *rt = nh_grp->priv;
	struct mlxsw_sp_nexthop *nh;
	struct mpls_nh *mpls_nh;
	int i;

	for (i = 0; i < nh_grp->count; i++) {
		mpls_nh = mlxsw_sp_nexthop_mpls_get_by_index(rt, i);
		nh = &nh_grp->nexthops[i];

		if (nh->offloaded)
			mpls_nh->nh_flags |= RTNH_F_OFFLOAD;
		else
			mpls_nh->nh_flags &= ~RTNH_F_OFFLOAD;
	}
}

static void
mlxsw_sp_fib_mpls_entry_offload_unset(struct mlxsw_sp_fib_entry *fib_entry)
{
	struct mlxsw_sp_nexthop_group *nh_grp = fib_entry->nh_group;
	struct mpls_route *rt = nh_grp->priv;
	struct mlxsw_sp_nexthop *nh;
	struct mpls_nh *mpls_nh;
	int i;

	for (i = 0; i < nh_grp->count; i++) {
		mpls_nh = mlxsw_sp_nexthop_mpls_get_by_index(rt, i);
		nh = &nh_grp->nexthops[i];

		mpls_nh->nh_flags &= ~RTNH_F_OFFLOAD;
	}
}

static void
mlxsw_sp_fib_mpls_entry_offload_refresh(struct mlxsw_sp_fib_entry *fib_entry,
					enum mlxsw_reg_mpilm_op op, int err)
{
	switch (op) {
	case MLXSW_REG_MPILM_OP_DELETE:
		return mlxsw_sp_fib_mpls_entry_offload_unset(fib_entry);
	case MLXSW_REG_MPILM_OP_READ_WRITE:
		if (err)
			return;
		if (fib_entry->nh_group->adj_index_valid)
			mlxsw_sp_fib_mpls_entry_offload_set(fib_entry);
		else
			mlxsw_sp_fib_mpls_entry_offload_unset(fib_entry);
		return;
	default:
		return;
	}
}

static int
__mlxsw_sp_fib_mpls_entry_op(struct mlxsw_sp *mlxsw_sp,
			     struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry,
			     enum mlxsw_reg_mpilm_op op)
{
	char mpilm_pl[MLXSW_REG_MPILM_LEN];
	struct mlxsw_sp_fib_entry *fib_entry = &fib_mpls_entry->common;
	enum mlxsw_reg_mpilm_trap_action trap_action;
	u32 adjacency_index = 0;
	u16 ecmp_size = 1;
	u16 trap_id = 0;

	/* In case the nexthop group adjacency index is valid, use it
	 * with provided ECMP size. Otherwise, setup trap and pass
	 * traffic to kernel.
	 */
	if (fib_entry->nh_group->adj_index_valid) {
		trap_action = MLXSW_REG_MPILM_TRAP_ACTION_NOP;
		adjacency_index = fib_entry->nh_group->adj_index;
		ecmp_size = fib_entry->nh_group->ecmp_size;
	} else {
		trap_action = MLXSW_REG_MPILM_TRAP_ACTION_TRAP;
		trap_id = MLXSW_TRAP_ID_MPLS_ILM0;
	}

	mlxsw_reg_mpilm_pack(mpilm_pl, op, fib_mpls_entry->label,
			     trap_action, trap_id, adjacency_index,
			     ecmp_size, 1);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);
}

static int
mlxsw_sp_fib_mpls_entry_op(struct mlxsw_sp *mlxsw_sp,
			   struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry,
			   enum mlxsw_reg_mpilm_op op)
{
	int err = __mlxsw_sp_fib_mpls_entry_op(mlxsw_sp, fib_mpls_entry, op);

	mlxsw_sp_fib_mpls_entry_offload_refresh(&fib_mpls_entry->common,
						op, err);

	return err;
}

static int
mlxsw_sp_fib_mpls_entry_update(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry)
{
	return mlxsw_sp_fib_mpls_entry_op(mlxsw_sp, fib_mpls_entry,
					  MLXSW_REG_MPILM_OP_READ_WRITE);
}

static int
mlxsw_sp_fib_mpls_entry_del(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry)
{
	return mlxsw_sp_fib_mpls_entry_op(mlxsw_sp, fib_mpls_entry,
					  MLXSW_REG_MPILM_OP_DELETE);
}

static int
mlxsw_sp_nexthop_mpls_type_init(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_nexthop *nh,
				struct mpls_nh *mpls_nh)
{
	struct net_device *dev = mpls_nh->nh_dev;
	struct mlxsw_sp_rif *rif;
	int err;

	nh->type = MLXSW_SP_NEXTHOP_TYPE_ETH;
	rif = mlxsw_sp_rif_find_by_dev(mlxsw_sp, dev);
	if (!rif)
		return 0;

	mlxsw_sp_nexthop_rif_init(nh, rif);
	err = mlxsw_sp_nexthop_neigh_init(mlxsw_sp, nh);
	if (err)
		goto err_neigh_init;

	return 0;

err_neigh_init:
	mlxsw_sp_nexthop_rif_fini(nh);
	return err;
}

static void
mlxsw_sp_nexthop_mpls_type_fini(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_nexthop *nh)
{
	mlxsw_sp_nexthop_type_fini(mlxsw_sp, nh);
}

static struct neigh_table *
mlxsw_sp_nexthop_mpls_neigh_tbl(struct mpls_nh *mpls_nh)
{
	switch (mpls_nh->nh_via_table) {
	case NEIGH_ARP_TABLE:
		return &arp_tbl;
	case NEIGH_ND_TABLE:
#if IS_ENABLED(CONFIG_IPV6)
		return &nd_tbl;
#endif
	default:
		return NULL;
	}
}

static int
mlxsw_sp_nexthop_mpls_init(struct mlxsw_sp *mlxsw_sp,
			   struct mlxsw_sp_nexthop_group *nh_grp,
			   struct mlxsw_sp_nexthop *nh,
			   struct mpls_nh *mpls_nh,
			   struct mpls_route *rt)
{
	struct net_device *dev = mpls_nh->nh_dev;
	int err;

	nh->nh_grp = nh_grp;
	nh->nh_weight = 1;
	nh->neigh_tbl = mlxsw_sp_nexthop_mpls_neigh_tbl(mpls_nh);
	if (!nh->neigh_tbl)
		return -EINVAL;

	memcpy(&nh->gw_addr, __mpls_nh_via(rt, mpls_nh), mpls_nh->nh_via_alen);
	list_add_tail(&nh->router_list_node, &mlxsw_sp->router->nexthop_list);
	if (!dev)
		return 0;

	err = mlxsw_sp_nexthop_mpls_type_init(mlxsw_sp, nh, mpls_nh);
	if (err)
		goto err_nexthop_neigh_init;

	return 0;

err_nexthop_neigh_init:
	mlxsw_sp_nexthop_remove(mlxsw_sp, nh);
	return err;
}

static void
mlxsw_sp_nexthop_mpls_fini(struct mlxsw_sp *mlxsw_sp,
			   struct mlxsw_sp_nexthop *nh)
{
	mlxsw_sp_nexthop_mpls_type_fini(mlxsw_sp, nh);
	list_del(&nh->router_list_node);
}

static int
mlxsw_sp_nexthop_mpls_update(struct mlxsw_sp *mlxsw_sp, u32 adj_index,
			     struct mlxsw_sp_nexthop *nh,
			     struct mpls_nh *mpls_nh)
{
	struct mlxsw_sp_neigh_entry *neigh_entry = nh->neigh_entry;
	char mpnhlfe_pl[MLXSW_REG_MPNHLFE_LEN];
	int nh_count = nh->nh_grp->ecmp_size;
	int i;
	int err;

	/* For netxhops with multiple labels multiple entries should be added */
	for (i = mpls_nh->nh_labels - 1; i > 0; i--) {
		mlxsw_reg_mpnhlfe_next_pack(mpnhlfe_pl, mpls_nh->nh_label[i],
					    adj_index, adj_index + nh_count, 1);
		err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpnhlfe),
				      mpnhlfe_pl);
		if (err)
			return err;
		adj_index += nh_count;
	}

	mlxsw_reg_mpnhlfe_pack(mpnhlfe_pl, mpls_nh->nh_label[0],
			       neigh_entry->ha, adj_index, neigh_entry->rif);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpnhlfe), mpnhlfe_pl);
}

static int
mlxsw_sp_nexthop_group_mpls_group_update(struct mlxsw_sp *mlxsw_sp,
					 struct mlxsw_sp_nexthop_group *nh_grp,
					 bool reallocate)
{
	u32 adj_index = nh_grp->adj_index; /* base */
	struct mpls_route *rt = nh_grp->priv;
	struct mlxsw_sp_nexthop *nh;
	struct mpls_nh *mpls_nh;
	int i;
	int err;

	for (i = 0; i < nh_grp->count; i++) {
		mpls_nh = mlxsw_sp_nexthop_mpls_get_by_index(rt, i);
		nh = &nh_grp->nexthops[i];

		if (!nh->should_offload) {
			nh->offloaded = 0;
			continue;
		}

		if (nh->update || reallocate) {
			err = mlxsw_sp_nexthop_mpls_update(mlxsw_sp, adj_index,
							   nh, mpls_nh);
			if (err)
				return err;

			nh->update = 0;
			nh->offloaded = 1;
		}
		adj_index += nh->num_adj_entries;
	}
	return 0;
}

static int
mlxsw_sp_nexthop_group_mpls_fib_update(struct mlxsw_sp *mlxsw_sp,
				       struct mlxsw_sp_nexthop_group *nh_grp)
{
	struct mlxsw_sp_fib_entry *fib_entry;
	int err;

	list_for_each_entry(fib_entry, &nh_grp->fib_list, nexthop_group_node) {
		err = mlxsw_sp_fib_mpls_entry_update(mlxsw_sp,
						     (struct mlxsw_sp_fib_mpls_entry *)fib_entry);
		if (err)
			return err;
	}
	return 0;
}

static int
mlxsw_sp_mpls_adj_index_mass_update(struct mlxsw_sp *mlxsw_sp,
				    u32 adj_index, u16 ecmp_size,
				    u32 new_adj_index,
				    u16 new_ecmp_size)
{
	char mpibe_pl[MLXSW_REG_MPIBE_LEN];

	mlxsw_reg_mpibe_pack(mpibe_pl, adj_index, ecmp_size, new_adj_index,
			     new_ecmp_size);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpibe), mpibe_pl);
}

static int
mlxsw_sp_nexthop_group_mpls_adj_index_mass_update(struct mlxsw_sp *mlxsw_sp,
						  struct mlxsw_sp_nexthop_group *nh_grp,
						  u32 old_adj_index, u16 old_ecmp_size)
{
	struct mlxsw_sp_fib_entry *fib_entry;
	int err;

	list_for_each_entry(fib_entry, &nh_grp->fib_list, nexthop_group_node) {
		err = mlxsw_sp_mpls_adj_index_mass_update(mlxsw_sp,
							  old_adj_index,
							  old_ecmp_size,
							  nh_grp->adj_index,
							  nh_grp->ecmp_size);
		if (err)
			return err;
	}
	return 0;
}

static int
mlxsw_sp_nexthop_group_mpls_fib_refresh(struct mlxsw_sp_nexthop_group *nh_grp)
{
	enum mlxsw_reg_mpilm_op op = MLXSW_REG_MPILM_OP_READ_WRITE;
	struct mlxsw_sp_fib_entry *fib_entry;

	list_for_each_entry(fib_entry, &nh_grp->fib_list, nexthop_group_node)
		mlxsw_sp_fib_mpls_entry_offload_refresh(fib_entry, op, 0);

	return 0;
}

static void
mlxsw_sp_nexthop_group_mpls_adj_group_size_update(struct mlxsw_sp_nexthop_group *nh_grp)
{
	struct mpls_route *rt = nh_grp->priv;
	int max_labels = 0;
	int i;

	for (i = 0; i < nh_grp->count; i++) {
		struct mpls_nh *mpls_nh;

		mpls_nh = mlxsw_sp_nexthop_mpls_get_by_index(rt, i);
		if (mpls_nh->nh_labels > max_labels)
			max_labels = mpls_nh->nh_labels;
	}

	nh_grp->adj_group_size = max_labels * rt->rt_nhn;
}

static void
mlxsw_sp_nexthop_group_mpls_ecmp_size_update(struct mlxsw_sp_nexthop_group *nh_grp)
{
	struct mpls_route *rt = nh_grp->priv;

	nh_grp->ecmp_size = rt->rt_nhn;
}

static const struct mlxsw_sp_nexthop_group_ops mlxsw_sp_nexthop_group_mpls_ops = {
	.group_update		= mlxsw_sp_nexthop_group_mpls_group_update,
	.fib_update		= mlxsw_sp_nexthop_group_mpls_fib_update,
	.adj_index_mass_update  = mlxsw_sp_nexthop_group_mpls_adj_index_mass_update,
	.fib_refresh            = mlxsw_sp_nexthop_group_mpls_fib_refresh,
	.adj_group_size_update	= mlxsw_sp_nexthop_group_mpls_adj_group_size_update,
	.ecmp_size_update	= mlxsw_sp_nexthop_group_mpls_ecmp_size_update,
};

static struct mlxsw_sp_nexthop_group *
mlxsw_sp_nexthop_mpls_group_create(struct mlxsw_sp *mlxsw_sp,
				   struct mpls_route *rt)
{
	struct mlxsw_sp_nexthop_group *nh_grp;
	struct mlxsw_sp_nexthop *nh;
	size_t alloc_size;
	int i;
	int err;

	alloc_size = sizeof(*nh_grp) +
		     rt->rt_nhn * sizeof(struct mlxsw_sp_nexthop);
	nh_grp = kzalloc(alloc_size, GFP_KERNEL);
	if (!nh_grp)
		return ERR_PTR(-ENOMEM);
	nh_grp->priv = rt;
	INIT_LIST_HEAD(&nh_grp->fib_list);
	nh_grp->group_type = MLXSW_SP_NEXTHOP_GROUP_TYPE_MPLS;
	nh_grp->gateway = true;
	nh_grp->count = rt->rt_nhn;
	nh_grp->ops = &mlxsw_sp_nexthop_group_mpls_ops;
	mpls_rt_hold(rt);
	for (i = 0; i < nh_grp->count; i++) {
		struct mpls_nh *mpls_nh;

		nh = &nh_grp->nexthops[i];
		mpls_nh = mlxsw_sp_nexthop_mpls_get_by_index(rt, i);
		err = mlxsw_sp_nexthop_mpls_init(mlxsw_sp, nh_grp, nh,
						 mpls_nh, rt);
		if (err)
			goto err_nexthop_mpls_init;
	}
	mlxsw_sp_nexthop_group_refresh(mlxsw_sp, nh_grp);
	return nh_grp;

err_nexthop_mpls_init:
	for (i--; i >= 0; i--) {
		nh = &nh_grp->nexthops[i];
		mlxsw_sp_nexthop_mpls_fini(mlxsw_sp, nh);
	}
	mpls_rt_put(rt);
	kfree(nh_grp);
	return ERR_PTR(err);
}

static void
mlxsw_sp_nexthop_mpls_group_destroy(struct mlxsw_sp *mlxsw_sp,
				    struct mlxsw_sp_nexthop_group *nh_grp)
{
	struct mpls_route *rt = nh_grp->priv;
	struct mlxsw_sp_nexthop *nh;
	int i;

	for (i = 0; i < nh_grp->count; i++) {
		nh = &nh_grp->nexthops[i];
		mlxsw_sp_nexthop_mpls_fini(mlxsw_sp, nh);
	}
	mlxsw_sp_nexthop_group_refresh(mlxsw_sp, nh_grp);
	WARN_ON_ONCE(nh_grp->adj_index_valid);
	mpls_rt_put(rt);
	kfree(nh_grp);
}

static int
mlxsw_sp_nexthop_mpls_group_get(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_fib_entry *fib_entry,
				struct mpls_route *rt)
{
	struct mlxsw_sp_nexthop_group *nh_grp;

	nh_grp = mlxsw_sp_nexthop_mpls_group_create(mlxsw_sp, rt);
	if (IS_ERR(nh_grp))
		return PTR_ERR(nh_grp);

	list_add_tail(&fib_entry->nexthop_group_node, &nh_grp->fib_list);
	fib_entry->nh_group = nh_grp;
	return 0;
}

static void
mlxsw_sp_nexthop_mpls_group_put(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_fib_entry *fib_entry)
{
	struct mlxsw_sp_nexthop_group *nh_grp = fib_entry->nh_group;

	list_del(&fib_entry->nexthop_group_node);
	if (!list_empty(&nh_grp->fib_list))
		return;
	mlxsw_sp_nexthop_mpls_group_destroy(mlxsw_sp, nh_grp);
}

static void
mlxsw_sp_fib_mpls_entry_replace(struct mlxsw_sp *mlxsw_sp, u32 label,
				bool replace)
{
	struct mlxsw_sp_fib_mpls_entry *replaced;

	if (!replace)
		return;

	replaced = mlxsw_sp_fib_mpls_lookup(mlxsw_sp, label);
	if (WARN_ON_ONCE(!replaced))
		return;

	mlxsw_sp_fib_mpls_remove(mlxsw_sp, replaced);
	mlxsw_sp_nexthop_mpls_group_put(mlxsw_sp, &replaced->common);
	kfree(replaced);
}

int mlxsw_sp_fib_mpls_add(struct mlxsw_sp *mlxsw_sp, struct mpls_route *rt,
			  u32 label, bool replace)
{
	struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry;
	struct mlxsw_sp_fib_entry *fib_entry;
	int err;

	if (mlxsw_sp->router->aborted)
		return 0;

	fib_mpls_entry = kzalloc(sizeof(*fib_mpls_entry), GFP_KERNEL);
	if (!fib_mpls_entry)
		return -ENOMEM;

	fib_entry = &fib_mpls_entry->common;
	fib_entry->type = MLXSW_SP_FIB_ENTRY_TYPE_REMOTE;

	err = mlxsw_sp_nexthop_mpls_group_get(mlxsw_sp, fib_entry, rt);
	if (err)
		goto err_nexthop_group_create;

	fib_mpls_entry->label = label;
	err = mlxsw_sp_fib_mpls_entry_update(mlxsw_sp, fib_mpls_entry);
	if (err)
		goto err_mpls_entry_update;

	mlxsw_sp_fib_mpls_entry_replace(mlxsw_sp, label, replace);
	err = mlxsw_sp_fib_mpls_insert(mlxsw_sp, fib_mpls_entry);
	if (err)
		goto err_mpls_insert;

	return 0;

err_mpls_insert:
	mlxsw_sp_fib_mpls_entry_del(mlxsw_sp, fib_mpls_entry);
err_mpls_entry_update:
	mlxsw_sp_nexthop_mpls_group_put(mlxsw_sp, fib_entry);
err_nexthop_group_create:
	kfree(fib_mpls_entry);
	return err;
}

void mlxsw_sp_fib_mpls_del(struct mlxsw_sp *mlxsw_sp, u32 label)
{
	struct mlxsw_sp_fib_mpls_entry *fib_mpls_entry;

	fib_mpls_entry = mlxsw_sp_fib_mpls_lookup(mlxsw_sp, label);
	if (WARN_ON_ONCE(!fib_mpls_entry))
		return;

	mlxsw_sp_fib_mpls_remove(mlxsw_sp, fib_mpls_entry);
	mlxsw_sp_fib_mpls_entry_del(mlxsw_sp, fib_mpls_entry);
	mlxsw_sp_nexthop_mpls_group_put(mlxsw_sp, &fib_mpls_entry->common);
	kfree(fib_mpls_entry);
}

static int mlxsw_sp_mpls_config_traps(struct mlxsw_sp *mlxsw_sp)
{
	char mpilm_pl[MLXSW_REG_MPILM_LEN];
	int err;

	mlxsw_reg_mpilm_pack(mpilm_pl, MLXSW_REG_MPILM_OP_READ_WRITE,
			     MPLS_LABEL_IPV4NULL, MLXSW_REG_MPILM_TRAP_ACTION_TRAP,
			     MLXSW_TRAP_ID_MPLS_ILM0, 0, 1, 0);

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);
	if (err)
		return err;

	mlxsw_reg_mpilm_pack(mpilm_pl, MLXSW_REG_MPILM_OP_READ_WRITE,
			     MPLS_LABEL_IPV6NULL, MLXSW_REG_MPILM_TRAP_ACTION_TRAP,
			     MLXSW_TRAP_ID_MPLS_ILM0, 0, 1, 0);

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);
	if (err)
		goto err_trap_set;

	return err;

err_trap_set:
	mlxsw_reg_mpilm_pack(mpilm_pl, MLXSW_REG_MPILM_OP_DELETE,
			     MPLS_LABEL_IPV4NULL, MLXSW_REG_MPILM_TRAP_ACTION_TRAP,
			     MLXSW_TRAP_ID_MPLS_ILM0, 0, 1, 0);
	mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);
	return err;
}

static void mlxsw_sp_mpls_deconfig_traps(struct mlxsw_sp *mlxsw_sp)
{
	char mpilm_pl[MLXSW_REG_MPILM_LEN];

	mlxsw_reg_mpilm_pack(mpilm_pl, MLXSW_REG_MPILM_OP_DELETE,
			     MPLS_LABEL_IPV6NULL, MLXSW_REG_MPILM_TRAP_ACTION_TRAP,
			     MLXSW_TRAP_ID_MPLS_ILM0, 0, 1, 0);
	mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);

	mlxsw_reg_mpilm_pack(mpilm_pl, MLXSW_REG_MPILM_OP_DELETE,
			     MPLS_LABEL_IPV4NULL, MLXSW_REG_MPILM_TRAP_ACTION_TRAP,
			     MLXSW_TRAP_ID_MPLS_ILM0, 0, 1, 0);
	mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpilm), mpilm_pl);
}

int mlxsw_sp_mpls_general_config(struct mlxsw_sp *mlxsw_sp)
{
	char mpgcr_pl[MLXSW_REG_MPGCR_LEN];
	int label_limit = (1 << 20) - 1;

	mlxsw_reg_mpgcr_pack(mpgcr_pl, MPLS_LABEL_FIRST_UNRESERVED,
			     label_limit);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(mpgcr), mpgcr_pl);
}

int mlxsw_sp_mpls_init(struct mlxsw_sp *mlxsw_sp)
{
	int err;

	err = mlxsw_sp_mpls_general_config(mlxsw_sp);
	if (err)
		return err;
	err = mlxsw_sp_mpls_config_traps(mlxsw_sp);
	if (err)
		return err;

	err = rhashtable_init(&mlxsw_sp->router->mpls_ht,
			      &mlxsw_sp_mpls_route_ht_params);
	if (err)
		goto err_rhashtable_init;

	return err;

err_rhashtable_init:
	mlxsw_sp_mpls_deconfig_traps(mlxsw_sp);
	return err;
}

void mlxsw_sp_mpls_fini(struct mlxsw_sp *mlxsw_sp)
{
	rhashtable_destroy(&mlxsw_sp->router->mpls_ht);
	mlxsw_sp_mpls_deconfig_traps(mlxsw_sp);
}
