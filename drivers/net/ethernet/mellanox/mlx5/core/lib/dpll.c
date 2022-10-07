// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/dpll.h>
#include <linux/mlx5/driver.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/refcount.h>
#include <linux/rhashtable.h>

// SHOULD NOT BE NEEDED
#include <uapi/linux/dpll.h>

#include "en.h"

#define MLX5_CLOCK_ID_SIZE MLX5_FLD_SZ_BYTES(msecq_reg, local_clock_identity)

struct mlx5_dpll {
	struct rhash_head ht_node;
	u8 clock_id[MLX5_CLOCK_ID_SIZE];
	refcount_t refcnt;
	struct dpll_device *dpll;
	struct list_head mdev_list; /* List of mdevs. */
	struct mutex lock; /* Protects mdev_list and makes sure that
			    * mdev does not disappear while locked.
			    */
	struct mlx5e_priv *synce_ports[2]; // THIS IS GOING TO BE DONE DIFFERENTLY AFTER DPLL API CHANGE
	int synce_port_cnt; // THIS IS GOING TO BE DONE DIFFERENTLY AFTER DPLL API CHANGE
};

static struct mlx5_core_dev *__mlx5_dpll_mdev_get(struct mlx5_dpll *mdpll)
{
	struct mlx5_core_dev *mdev;

	mdev = list_first_entry(&mdpll->mdev_list, struct mlx5_core_dev, dpll_list);
	WARN_ON(!mdev);
	return mdev;
}

static int mlx5_dpll_clock_id_get(struct mlx5_core_dev *mdev, u8 *clock_id)
{
	u32 out[MLX5_ST_SZ_DW(msecq_reg)] = {};
	u32 in[MLX5_ST_SZ_DW(msecq_reg)] = {};
	int err;

	err = mlx5_core_access_reg(mdev, in, sizeof(in), out, sizeof(out),
				   MLX5_REG_MSECQ, 0, 0);
	if (err)
		return err;
	memcpy(clock_id, MLX5_ADDR_OF(msecq_reg, out, local_clock_identity),
	       MLX5_CLOCK_ID_SIZE);
	return 0;
}

static int mlx5_dpll_synce_status_get(struct mlx5_core_dev *mdev,
				      enum mlx5_msees_oper_status *oper_status)
{
	u32 out[MLX5_ST_SZ_DW(msees_reg)] = {};
	u32 in[MLX5_ST_SZ_DW(msees_reg)] = {};
	int err;

	MLX5_SET(msees_reg, in, local_port, 1);
	err = mlx5_core_access_reg(mdev, in, sizeof(in), out, sizeof(out),
				   MLX5_REG_MSEES, 0, 0);
	if (err)
		return err;
	*oper_status = MLX5_GET(msees_reg, out, oper_status);
	return 0;
}

static int mlx5_dpll_synce_status_set(struct mlx5_core_dev *mdev,
				      enum mlx5_msees_admin_status admin_status)
{
	u32 out[MLX5_ST_SZ_DW(msees_reg)] = {};
	u32 in[MLX5_ST_SZ_DW(msees_reg)] = {};

	MLX5_SET(msees_reg, in, local_port, 1);
	MLX5_SET(msees_reg, in, field_select,
		 MLX5_MSEES_FIELD_SELECT_ENABLE |
		 MLX5_MSEES_FIELD_SELECT_ADMIN_STATUS);
	MLX5_SET(msees_reg, in, admin_status, admin_status);
	MLX5_SET(msees_reg, in, admin_freq_measure,
		 admin_status == MLX5_MSEES_ADMIN_STATUS_TRACK);
	return mlx5_core_access_reg(mdev, in, sizeof(in), out, sizeof(out),
				    MLX5_REG_MSEES, 0, 0);
}

static int mlx5_dpll_get_status(struct dpll_device *dpll)
{
	return DPLL_STATUS_NONE;
};

static int mlx5_dpll_get_lock_status(struct dpll_device *dpll)
{
	struct mlx5_dpll *mdpll = dpll_priv(dpll);
	enum mlx5_msees_oper_status oper_status;
	int err;

	mutex_lock(&mdpll->lock);
	err = mlx5_dpll_synce_status_get(__mlx5_dpll_mdev_get(mdpll),
					 &oper_status);
	mutex_unlock(&mdpll->lock);
	if (err)
		return err;

	switch (oper_status) {
	case MLX5_MSEES_OPER_STATUS_SELF_TRACK: /* fall-through */
	case MLX5_MSEES_OPER_STATUS_OTHER_TRACK:
		return DPLL_LOCK_STATUS_SYNCE;
	default:
		return DPLL_LOCK_STATUS_INT_OSCILLATOR;
	}
}

static int mlx5_dpll_get_source_type(struct dpll_device *dpll, int id)
{
	return DPLL_TYPE_SYNCE_ETH_PORT;
}

static int mlx5_dpll_get_source_supported(struct dpll_device *dpll, int id, int type)
{
	return type == DPLL_TYPE_SYNCE_ETH_PORT;
}

static int mlx5_dpll_get_output_type(struct dpll_device *dpll, int id)
{
	return DPLL_TYPE_SYNCE_ETH_PORT;
}

static int mlx5_dpll_get_output_supported(struct dpll_device *dpll, int id, int type)
{
	return type == DPLL_TYPE_SYNCE_ETH_PORT;
}

static int mlx5_dpll_set_source_type(struct dpll_device *dpll, int id, int val)
{
	struct mlx5_dpll *mdpll = dpll_priv(dpll);
	struct mlx5e_priv *priv;

	if (val != DPLL_TYPE_SYNCE_ETH_PORT)
		return -EOPNOTSUPP;

	priv = mdpll->synce_ports[id];
	return mlx5_dpll_synce_status_set(priv->mdev,
					  MLX5_MSEES_ADMIN_STATUS_TRACK);
}

static int mlx5_dpll_set_output_type(struct dpll_device *dpll, int id, int val)
{
	return 0;
}

static struct dpll_device_ops mlx5_dpll_ops = {
	.get_status		= mlx5_dpll_get_status,
	.get_lock_status	= mlx5_dpll_get_lock_status,
	.get_source_type	= mlx5_dpll_get_source_type,
	.get_source_supported	= mlx5_dpll_get_source_supported,
	.get_output_type	= mlx5_dpll_get_output_type,
	.get_output_supported	= mlx5_dpll_get_output_supported,
	.set_source_type	= mlx5_dpll_set_source_type,
	.set_output_type	= mlx5_dpll_set_output_type,
};

// POSSIBLE API EXTENSIONS:
// 1) Expose clock quality: MSECQ->local_ssm_code, MSECQ->local_enhanced_ssm_code
//    Proposed enum:
//      QL_DNU,
//      QL_EEC1,
//      QL_eEEC,
//      QL_SSU_B,
//      QL_SSU_A,
//      QL_PRC,
//      QL_ePRC,
//      QL_PRTC,
//      QL_ePRTC,
// 2) Expose possibility to do holdover: MSEES->ho_acq
// 3) DPLL Implementation hw-speficic values (debug?): MSEES->oper_freq_measure

int mlx5_dpll_synce_port_init(struct mlx5e_priv *priv)
{
	struct mlx5_dpll *mdpll = priv->mdev->dpll;
	int err;

	err = mlx5_dpll_synce_status_set(priv->mdev,
					 MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
	if (err)
		return err;
	mdpll->synce_ports[mdpll->synce_port_cnt++] = priv;

	return 0;
}

void mlx5_dpll_synce_port_cleanup(struct mlx5e_priv *priv)
{
	struct mlx5_dpll *mdpll = priv->mdev->dpll;

	// CLEANUP IS BROKEN, DOES NOT TOUCH synce_ports and synce_port_cnt
	// THIS IS GOING TO BE DONE DIFFERENTLY AFTER DPLL API CHANGE
	mdpll->synce_port_cnt--;
	mlx5_dpll_synce_status_set(priv->mdev,
				   MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
}

static DEFINE_MUTEX(mlx5_dpll_ht_lock); /* Protects the dpll hash table
					 * and reference counting.
					 */
static struct rhashtable mlx5_dpll_ht;

static const struct rhashtable_params mlx5_dpll_ht_params = {
	.key_len = sizeof_field(struct mlx5_dpll, clock_id),
	.key_offset = offsetof(struct mlx5_dpll, clock_id),
	.head_offset = offsetof(struct mlx5_dpll, ht_node),
	.automatic_shrinking = true,
};

int mlx5_dpll_init(struct mlx5_core_dev *mdev)
{
	u8 clock_id[MLX5_CLOCK_ID_SIZE];
	struct mlx5_dpll *mdpll;
	int err;

	err = mlx5_dpll_synce_status_set(mdev,
					 MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
	if (err)
		return err;

	err = mlx5_dpll_clock_id_get(mdev, clock_id);
	if (err)
		return err;

	/* Multiple mdev instances might share one dpll clock.
	 * In case the previous instance initialized dpll, just take
	 * reference and quit.
	 */

	mutex_lock(&mlx5_dpll_ht_lock);
	mdpll = rhashtable_lookup_fast(&mlx5_dpll_ht, clock_id,
				       mlx5_dpll_ht_params);
	if (mdpll) {
		refcount_inc(&mdpll->refcnt);
		mutex_unlock(&mlx5_dpll_ht_lock);
		mutex_lock(&mdpll->lock);
		list_add_tail(&mdev->dpll_list, &mdpll->mdev_list);
		mutex_unlock(&mdpll->lock);
		mdev->dpll = mdpll;
		return 0;
	}
	mdpll = kzalloc(sizeof(*mdpll), GFP_KERNEL);
	if (!mdpll) {
		err = -ENOMEM;
		goto err_unlock;
	}
	refcount_set(&mdpll->refcnt, 1);
	memcpy(mdpll->clock_id, clock_id, sizeof(mdpll->clock_id));
	INIT_LIST_HEAD(&mdpll->mdev_list);
	mutex_init(&mdpll->lock);
	mdpll->synce_port_cnt = 0; // THIS IS GOING TO BE DONE DIFFERENTLY AFTER DPLL API CHANGE

	// 2,2 is temporary, this is going to be done differently, by dynamically
	// register/unregister source/output pins during dpll lifetime.
	mdpll->dpll = dpll_device_alloc(&mlx5_dpll_ops, "mlx5", 2, 2, mdpll);
	if (IS_ERR(mdpll->dpll)) {
		err = PTR_ERR(mdpll->dpll);
		goto err_free_mdpll;
	}
	dpll_device_register(mdpll->dpll);

	err = rhashtable_insert_fast(&mlx5_dpll_ht, &mdpll->ht_node,
				     mlx5_dpll_ht_params);
	if (err)
		goto err_free_dpll_device;

	mutex_unlock(&mlx5_dpll_ht_lock);

	mdev->dpll = mdpll;
	return 0;

err_free_dpll_device:
	dpll_device_free(mdpll->dpll);
err_free_mdpll:
	kfree(mdpll);
err_unlock:
	mutex_unlock(&mlx5_dpll_ht_lock);
	return err;
}

void mlx5_dpll_cleanup(struct mlx5_core_dev *mdev)
{
	struct mlx5_dpll *mdpll = mdev->dpll;

	mutex_lock(&mlx5_dpll_ht_lock);
	if (!refcount_dec_and_test(&mdpll->refcnt)) {
		mutex_unlock(&mlx5_dpll_ht_lock);
		mutex_lock(&mdpll->lock);
		list_del(&mdev->dpll_list);
		mutex_unlock(&mdpll->lock);
		return;
	}
	dpll_device_unregister(mdpll->dpll);
	dpll_device_free(mdpll->dpll);
	mutex_destroy(&mdpll->lock);
	WARN_ON(!list_empty(&mdpll->mdev_list));
	rhashtable_remove_fast(&mlx5_dpll_ht, &mdpll->ht_node,
			       mlx5_dpll_ht_params);
	mutex_unlock(&mlx5_dpll_ht_lock);
	kfree(mdpll);
	mlx5_dpll_synce_status_set(mdev,
				   MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
}

int mlx5_dpll_ht_init(void)
{
	return rhashtable_init(&mlx5_dpll_ht, &mlx5_dpll_ht_params);
}

void mlx5_dpll_ht_cleanup(void)
{
	rhashtable_destroy(&mlx5_dpll_ht);
}
