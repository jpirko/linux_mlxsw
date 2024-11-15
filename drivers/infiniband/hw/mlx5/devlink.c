// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
// Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

#include <linux/mlx5/driver.h>
#include <linux/mlx5/vport.h>
#include <net/devlink.h>
#include <rdma/ib_verbs.h>
#include "devlink.h"
#include "mlx5_ib.h"

static const struct devlink_ops mlx5r_devlink_ops = {
};

struct mlx5r_dev *mlx5r_create_devlink(struct device *dev,
				       struct mlx5_core_dev *mdev,
				       enum rdma_link_layer ll)
{
	struct mlx5r_dev *mlx5r_dev;
	struct devlink *devlink;
	int err;

	devlink = devlink_alloc_ns(&mlx5r_devlink_ops, sizeof(*mlx5r_dev),
				   devlink_net(priv_to_devlink(mdev)), dev);
	if (!devlink)
		return ERR_PTR(-ENOMEM);
	mlx5r_dev = devlink_priv(devlink);
	mlx5r_dev->ll = ll;

	err = devl_nested_devlink_set(priv_to_devlink(mdev), devlink);
	if (err) {
		devlink_free(devlink);
		return ERR_PTR(err);
	}

	devlink_register(devlink);
	return mlx5r_dev;
}

void mlx5r_destroy_devlink(struct mlx5r_dev *mlx5r_dev)
{
	struct devlink *devlink = priv_to_devlink(mlx5r_dev);

	devlink_unregister(devlink);
	devlink_free(devlink);
}

int mlx5r_devlink_port_register(struct mlx5r_dev *mlx5r_dev,
				struct mlx5_core_dev *mdev)
{
	struct devlink *devlink = priv_to_devlink(mlx5r_dev);
	struct devlink_port_attrs attrs = {};
	unsigned int dl_port_index;
	u16 vport_num;

	if (mlx5r_dev->ll != IB_LINK_LAYER_INFINIBAND)
		return 0;

	if (mlx5_core_is_pf(mdev)) {
		attrs.flavour = DEVLINK_PORT_FLAVOUR_PHYSICAL;
		attrs.phys.port_number = mlx5_get_dev_index(mdev);
		vport_num = MLX5_VPORT_UPLINK;
	} else {
		attrs.flavour = DEVLINK_PORT_FLAVOUR_VIRTUAL;
		vport_num = 0;
	}

	dl_port_index = mlx5_vport_to_devlink_port_index(mdev, vport_num);
	devlink_port_attrs_set(&mlx5r_dev->dl_port, &attrs);
	return devlink_port_register(devlink, &mlx5r_dev->dl_port,
				     dl_port_index);
}

void mlx5r_devlink_port_unregister(struct mlx5r_dev *mlx5r_dev)
{
	if (mlx5r_dev->ll != IB_LINK_LAYER_INFINIBAND)
		return;
	devlink_port_unregister(&mlx5r_dev->dl_port);
}

void mlx5r_devlink_port_ib_link(struct mlx5r_dev *mlx5r_dev)
{
	if (mlx5r_dev->ll != IB_LINK_LAYER_INFINIBAND)
		return;
	devlink_port_type_ib_set(&mlx5r_dev->dl_port, &mlx5r_dev->dev->ib_dev);
}

void mlx5r_devlink_port_ib_unlink(struct mlx5r_dev *mlx5r_dev)
{
	if (mlx5r_dev->ll != IB_LINK_LAYER_INFINIBAND)
		return;
	devlink_port_type_clear(&mlx5r_dev->dl_port);
}
