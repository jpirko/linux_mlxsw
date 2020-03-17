// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2020 Mellanox Technologies. All rights reserved */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/idr.h>

#include "spectrum.h"
#include "core.h"

static int mlxsw_sp1_counter_af_init(struct mlxsw_sp *mlxsw_sp)
{
	return 0;
}

static void mlxsw_sp1_counter_af_fini(struct mlxsw_sp *mlxsw_sp)
{
}

static int mlxsw_sp1_counter_af_alloc(struct mlxsw_sp *mlxsw_sp,
				      unsigned int *p_counter_index)
{
	return -EOPNOTSUPP;
}

const struct mlxsw_sp_counter_af_ops mlxsw_sp1_counter_af_ops = {
	.init			= mlxsw_sp1_counter_af_init,
	.fini			= mlxsw_sp1_counter_af_fini,
	.counter_alloc		= mlxsw_sp1_counter_af_alloc,
};

struct mlxsw_sp_counter_af_counter {
	atomic_long_t packets;
	atomic_long_t bytes;
};

struct mlxsw_sp_counter_af {
	u64 counter_index_start;
	u64 counter_index_end;
	atomic_t active_entries_count;
	struct idr counter_idr;
};

/* TODO: It is possible that under some circumstances when
 * there is a lot of counters with big trafffic, low-traffic
 * counter updates may starve. It is therefore needed to track
 * the last update time and periodically check-and-update them
 * manually using MAFRI register.
 */

static void mlxsw_sp_counter_af_inc(const struct mlxsw_reg_info *reg,
				    char *mafti_pl, void *priv)
{
	struct mlxsw_sp_counter_af_counter *counter;
	struct mlxsw_sp_counter_af *af = priv;
	u32 counter_index;
	u32 packets_inc;
	u32 bytes_inc;
	u8 num_rec;
	int i;

	num_rec = mlxsw_reg_mafti_num_rec_get(mafti_pl);
	for (i = 0; i < num_rec; i++) {
		mlxsw_reg_mafti_rec_unpack(mafti_pl, i, &counter_index,
					   &bytes_inc, &packets_inc);
		counter = idr_find(&af->counter_idr, counter_index);
		if (counter) {
			atomic_long_add(packets_inc, &counter->packets);
			atomic_long_add(bytes_inc, &counter->bytes);
		}
	}
}

static const struct mlxsw_listener mlxsw_sp_counter_af_listener =
	MLXSW_EVENTL(mlxsw_sp_counter_af_inc, ACCU_FLOW_INC, SP_ACCU_FLOW_INC);

static u64 mlxsw_sp_counter_af_occ_get(void *priv)
{
	const struct mlxsw_sp_counter_af *af = priv;

	return atomic_read(&af->active_entries_count);
}

static int mlxsw_sp2_counter_af_init(struct mlxsw_sp *mlxsw_sp)
{
	struct devlink *devlink = priv_to_devlink(mlxsw_sp->core);
	struct mlxsw_sp_counter_af *af;
	u64 size;
	int err;

	if (!MLXSW_CORE_RES_VALID(mlxsw_sp->core, START_ACCUFLOW))
		return -EIO;

	af = kzalloc(sizeof(*af), GFP_KERNEL);
	if (!af)
		return -ENOMEM;
	mlxsw_sp->counter_af = af;
	idr_init(&af->counter_idr);
	atomic_set(&af->active_entries_count, 0);
	af->counter_index_start = MLXSW_CORE_RES_GET(mlxsw_sp->core,
						     START_ACCUFLOW);

	err = devlink_resource_size_get(devlink,
					MLXSW_SP_RESOURCE_ACCU_FLOW_COUNTERS,
					&size);
	if (err)
		goto err_resource_size_get;
	af->counter_index_end = af->counter_index_start + size;

	devlink_resource_occ_get_register(devlink,
					  MLXSW_SP_RESOURCE_ACCU_FLOW_COUNTERS,
					  mlxsw_sp_counter_af_occ_get, af);

	err = mlxsw_core_trap_register(mlxsw_sp->core,
				       &mlxsw_sp_counter_af_listener, af);
	if (err)
		goto err_listener_register;

	return 0;

err_listener_register:
	devlink_resource_occ_get_unregister(devlink,
					    MLXSW_SP_RESOURCE_ACCU_FLOW_COUNTERS);
err_resource_size_get:
	kfree(af);
	return err;
}

static void mlxsw_sp2_counter_af_fini(struct mlxsw_sp *mlxsw_sp)
{
	struct devlink *devlink = priv_to_devlink(mlxsw_sp->core);
	struct mlxsw_sp_counter_af *af = mlxsw_sp->counter_af;

	mlxsw_core_trap_unregister(mlxsw_sp->core,
				   &mlxsw_sp_counter_af_listener, af);
	WARN_ON(atomic_read(&af->active_entries_count));
	devlink_resource_occ_get_unregister(devlink,
					    MLXSW_SP_RESOURCE_ACCU_FLOW_COUNTERS);
	WARN_ON(!idr_is_empty(&af->counter_idr));
	idr_destroy(&af->counter_idr);
	kfree(af);
}

static bool mlxsw_sp2_counter_af_index_check(struct mlxsw_sp *mlxsw_sp,
					     unsigned int counter_index)
{
	return counter_index >= mlxsw_sp->counter_af->counter_index_start;
}

static int mlxsw_sp2_counter_af_alloc(struct mlxsw_sp *mlxsw_sp,
				      unsigned int *p_counter_index)
{
	struct mlxsw_sp_counter_af *af = mlxsw_sp->counter_af;
	struct mlxsw_sp_counter_af_counter *counter;
	int err;
	int id;

	counter = kzalloc(sizeof(*counter), GFP_KERNEL);
	if (!counter)
		return -ENOMEM;
	atomic_long_set(&counter->packets, 0);
	atomic_long_set(&counter->bytes, 0);
	id = idr_alloc_cyclic(&af->counter_idr, counter,
			      af->counter_index_start,
			      af->counter_index_end, GFP_KERNEL);
	if (id < 0) {
		err = id;
		goto err_idr_alloc;
	}
	atomic_inc(&af->active_entries_count);
	*p_counter_index = id;
	return 0;

err_idr_alloc:
	kfree(counter);
	return err;
}

static void mlxsw_sp2_counter_af_free(struct mlxsw_sp *mlxsw_sp,
				      unsigned int counter_index)
{
	struct mlxsw_sp_counter_af *af = mlxsw_sp->counter_af;
	struct mlxsw_sp_counter_af_counter *counter;

	counter = idr_find(&af->counter_idr, counter_index);
	if (WARN_ON(!counter))
		return;
	atomic_dec(&af->active_entries_count);
	idr_remove(&af->counter_idr, counter_index);
	kfree(counter);
}

static void mlxsw_sp2_counter_af_get(struct mlxsw_sp *mlxsw_sp,
				     unsigned int counter_index,
				     u64 *p_packets, u64 *p_bytes)
{
	struct mlxsw_sp_counter_af *af = mlxsw_sp->counter_af;
	struct mlxsw_sp_counter_af_counter *counter;

	counter = idr_find(&af->counter_idr, counter_index);
	if (WARN_ON(!counter))
		return;
	*p_packets = atomic_long_read(&counter->packets);
	*p_bytes = atomic_long_read(&counter->bytes);
}

const struct mlxsw_sp_counter_af_ops mlxsw_sp2_counter_af_ops = {
	.init			= mlxsw_sp2_counter_af_init,
	.fini			= mlxsw_sp2_counter_af_fini,
	.counter_index_check	= mlxsw_sp2_counter_af_index_check,
	.counter_alloc		= mlxsw_sp2_counter_af_alloc,
	.counter_free		= mlxsw_sp2_counter_af_free,
	.counter_get		= mlxsw_sp2_counter_af_get,
};

int mlxsw_sp2_counter_af_resources_register(struct mlxsw_core *mlxsw_core)
{
	static struct devlink_resource_size_params size_params;
	struct devlink *devlink = priv_to_devlink(mlxsw_core);
	u64 size;

	if (!MLXSW_CORE_RES_VALID(mlxsw_core, MAX_ACCUFLOW))
		return -EIO;

	size = MLXSW_CORE_RES_GET(mlxsw_core, MAX_ACCUFLOW);

	devlink_resource_size_params_init(&size_params, size, size, 1,
					  DEVLINK_RESOURCE_UNIT_ENTRY);
	return devlink_resource_register(devlink,
					 MLXSW_SP_RESOURCE_NAME_ACCU_FLOW_COUNTERS,
					 size,
					 MLXSW_SP_RESOURCE_ACCU_FLOW_COUNTERS,
					 DEVLINK_RESOURCE_ID_PARENT_TOP,
					 &size_params);
}
