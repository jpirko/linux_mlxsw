// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/mlx5/driver.h>
#include <linux/mlx5/vport.h>

#include "sh_devlink.h"

/* This structure represents a shared devlink instance,
 * there is one created for PF group of the same chip.
 */
struct mlx5_shd {
	/* List of per-PF mdev instances. */
	struct list_head mdev_list;
};

/*
static int mlx5_shd_mdev(struct devlink *devlink)
{
	struct mlx5_shd *shd = devlink_priv(devlink);

	return list_first_entry(&shd->mdev_list,
				struct mlx5_core_dev, shd_list);
}
*/

static int mlx5_shd_enable_sriov_get(struct devlink *devlink, u32 id,
				     struct devlink_param_gset_ctx *ctx)
{
	// struct mlx5_core_dev *mdev = mlx5_shd_mdev(devlink);

	ctx->val.vbool = true;
	return 0;
}

static int mlx5_shd_enable_srio_set(struct devlink *devlink, u32 id,
				    struct devlink_param_gset_ctx *ctx,
				    struct netlink_ext_ack *extack)
{
	// struct mlx5_core_dev *mdev = mlx5_shd_mdev(devlink);

	return 0;
}

static const struct devlink_param mlx5_shd_params[] = {
	DEVLINK_PARAM_GENERIC(ENABLE_SRIOV, BIT(DEVLINK_PARAM_CMODE_PERMANENT),
			      mlx5_shd_enable_sriov_get, mlx5_shd_enable_srio_set,
			      NULL),
};

static const struct devlink_ops mlx5_shd_ops = {
};


static int mlx5_shd_id_get(struct mlx5_core_dev *mdev, u64 *id)
{
	*id = 0xCAFE;
	return 0;
}

int mlx5_shd_init(struct mlx5_core_dev *mdev)
{
	struct devlink *devlink;
	struct mlx5_shd *shd;
	int err;
	u64 id;

	if (!mlx5_core_is_pf(mdev))
		return 0;

	err = mlx5_shd_id_get(mdev, &id);
	if (err)
		return err;
	/* Multiple mdev instances share one devlink instance. */
	devlink = devlink_alloc_shared(&mlx5_shd_ops, sizeof(struct mlx5_shd),
				       &init_net, id);
	if (!devlink)
		return -ENOMEM;

	err = devl_nested_devlink_set(devlink, priv_to_devlink(mdev));
	if (err)
		goto nested_devlink_set_err;

	shd = devlink_priv(devlink);
	mdev->shd = shd;

	devl_lock(devlink);
	if (devl_shared_inc(devlink)) {
		INIT_LIST_HEAD(&shd->mdev_list);
		err = devl_params_register(devlink, mlx5_shd_params,
					   ARRAY_SIZE(mlx5_shd_params));
		if (err)
			goto unlock;
		devl_register(devlink);
	}
	list_add_tail(&mdev->shd_list, &shd->mdev_list);
unlock:
	devl_unlock(devlink);

	return err;

nested_devlink_set_err:
	devlink_free(devlink);
	return err;
}

void mlx5_shd_uninit(struct mlx5_core_dev *mdev)
{
	struct devlink *devlink = priv_to_devlink(mdev->shd);

	if (!mlx5_core_is_pf(mdev))
		return;

	devl_lock(devlink);
	list_del(&mdev->shd_list);
	if (devl_shared_dec(devlink)) {
		devl_unregister(devlink);
		devl_params_unregister(devlink, mlx5_shd_params,
				       ARRAY_SIZE(mlx5_shd_params));
	}
	devl_unlock(devlink);

	devlink_free(devlink);
}
