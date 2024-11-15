/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/* Copyright (c) 2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef __MLX5_IB_DEVLINK_H__
#define __MLX5_IB_DEVLINK_H__

struct mlx5r_dev *mlx5r_create_devlink(struct device *dev,
				       struct mlx5_core_dev *mdev,
				       enum rdma_link_layer ll);
void mlx5r_destroy_devlink(struct mlx5r_dev *mlx5r_dev);
int mlx5r_devlink_port_register(struct mlx5r_dev *mlx5r_dev,
				struct mlx5_core_dev *mdev);
void mlx5r_devlink_port_unregister(struct mlx5r_dev *mlx5r_dev);
void mlx5r_devlink_port_ib_link(struct mlx5r_dev *mlx5r_dev);
void mlx5r_devlink_port_ib_unlink(struct mlx5r_dev *mlx5r_dev);

#endif /* __MLX5_IB_DEVLINK_H__ */
