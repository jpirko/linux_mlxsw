// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/mlx5/driver.h>
#include <linux/mlx5/vport.h>

#include "sh_devlink.h"

/* This structure represents a shared devlink instance,
 * there is one created for PF group of the same chip.
 */
struct mlx5_shd {
	/* List of per-PF dev instances. */
	struct list_head dev_list;
};

/*
static int mlx5_shd_dev(struct devlink *devlink)
{
	struct mlx5_shd *shd = devlink_priv(devlink);

	return list_first_entry(&shd->dev_list,
				struct mlx5_core_dev, shd_list);
}
*/

static int mlx5_shd_enable_sriov_get(struct devlink *devlink, u32 id,
				     struct devlink_param_gset_ctx *ctx)
{
	// struct mlx5_core_dev *dev = mlx5_shd_dev(devlink);

	ctx->val.vbool = true;
	return 0;
}

static int mlx5_shd_enable_srio_set(struct devlink *devlink, u32 id,
				    struct devlink_param_gset_ctx *ctx,
				    struct netlink_ext_ack *extack)
{
	// struct mlx5_core_dev *dev = mlx5_shd_dev(devlink);

	return 0;
}

static const struct devlink_param mlx5_shd_params[] = {
	DEVLINK_PARAM_GENERIC(ENABLE_SRIOV, BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			      mlx5_shd_enable_sriov_get, mlx5_shd_enable_srio_set,
			      NULL),
};

static const struct devlink_ops mlx5_shd_ops = {
};


static int mlx5_shd_id_get(struct mlx5_core_dev *dev, u64 *id)
{
	*id = 0xCAFE;
	return 0;
}

int mlx5_shd_init(struct mlx5_core_dev *dev)
{
	struct devlink *devlink;
	struct mlx5_shd *shd;
	int err;
	u64 id;

	if (!mlx5_core_is_pf(dev))
		return 0;

	err = mlx5_shd_id_get(dev, &id);
	if (err)
		return err;
	/* Multiple dev instances share one devlink instance. */
	devlink = devlink_shared_alloc(&mlx5_shd_ops, sizeof(struct mlx5_shd),
				       &init_net, id, dev, dev->shd_inst);
	if (!devlink)
		return -ENOMEM;

	err = devl_nested_devlink_set(devlink, priv_to_devlink(dev));
	if (err)
		goto nested_devlink_set_err;

	shd = devlink_priv(devlink);
	dev->shd = shd;

	devl_lock(devlink);
	if (devl_shared_should_init(devlink, dev->shd_inst)) {
		err = devl_params_register(devlink, mlx5_shd_params,
					   ARRAY_SIZE(mlx5_shd_params));
		if (err)
			goto unlock;
		devl_register(devlink);
	}
unlock:
	devl_unlock(devlink);

	return err;

nested_devlink_set_err:
	devlink_free_shared(devlink, dev->shd_inst);
	return err;
}

void mlx5_shd_uninit(struct mlx5_core_dev *dev)
{
	struct devlink *devlink = priv_to_devlink(dev->shd);

	if (!mlx5_core_is_pf(dev))
		return;

	devl_lock(devlink);
	if (devl_shared_should_fini(devlink)) {
		devl_unregister(devlink);
		devl_params_unregister(devlink, mlx5_shd_params,
				       ARRAY_SIZE(mlx5_shd_params));
	}
	devl_unlock(devlink);

	devlink_free_shared(devlink);
}
