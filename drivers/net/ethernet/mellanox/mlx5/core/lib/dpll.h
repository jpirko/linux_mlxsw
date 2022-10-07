/* SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB */
/* Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef __MLX5_DPLL_H_
#define __MLX5_DPLL_H_

#include <linux/mlx5/driver.h>

struct mlx5_dpll_synce_port {
};

int mlx5_dpll_synce_port_init(struct mlx5e_priv *priv);
void mlx5_dpll_synce_port_cleanup(struct mlx5e_priv *priv);

int mlx5_dpll_init(struct mlx5_core_dev *mdev);
void mlx5_dpll_cleanup(struct mlx5_core_dev *mdev);

int mlx5_dpll_ht_init(void);
void mlx5_dpll_ht_cleanup(void);

#endif
