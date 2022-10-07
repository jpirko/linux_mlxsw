// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/dpll.h>
#include <linux/mlx5/driver.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/refcount.h>
#include <linux/rhashtable.h>
#include <linux/netdevice.h>
#include <linux/notifier.h>

// SHOULD NOT BE NEEDED
#include <uapi/linux/dpll.h>

#include "en.h"

#define MLX5_CLOCK_ID_SIZE MLX5_FLD_SZ_BYTES(msecq_reg, local_clock_identity)

/* This structure represents a DPLL instance, it may be shared among multiple
 * PCI devices (mdevs)
 */
struct mlx5_dpll {
	struct rhash_head ht_node;
	u8 clock_id[MLX5_CLOCK_ID_SIZE];
	refcount_t refcnt;
	struct dpll_device *dpll;
	struct list_head adev_list;
	struct mutex lock; /* Protects adev_list and makes sure that
			    * adev does not disappear while locked.
			    */
};

/* This structure represents a reference to DPLL, one is created
 * per mdev instance.
 */
struct mlx5_dpll_adev {
	struct list_head list;
	struct mlx5_dpll *dpll;
	struct mlx5_core_dev *mdev;
	struct notifier_block adev_nb;
};

// THIS HELPER IS NOT GOING TO BE NEEDED AFTER DPLL API CHANGE
static struct mlx5_dpll_adev *mlx5_dpll_adev_get(struct mlx5_dpll *mdpll, int index)
{
	struct mlx5_dpll_adev *mdpll_adev;
	int cnt = 0;

	list_for_each_entry(mdpll_adev, &mdpll->adev_list, list) {
		if (cnt++ == index)
			return mdpll_adev;
	}
	WARN_ON(1);
	return NULL;
}

static struct mlx5_core_dev *__mlx5_dpll_mdev_get(struct mlx5_dpll *mdpll)
{
	struct mlx5_dpll_adev *mdpll_adev;

	mdpll_adev = list_first_entry(&mdpll->adev_list, struct mlx5_dpll_adev, list);
	WARN_ON(!mdpll_adev); /* At least one adev has to be present */
	return mdpll_adev->mdev;
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
	struct mlx5_dpll_adev *mdpll_adev;

	if (val != DPLL_TYPE_SYNCE_ETH_PORT)
		return -EOPNOTSUPP;

	// CHANGED DPLL API SHOULD GIVE US mdlpp_adev POINTER HERE AS A PRIV
	mdpll_adev = mlx5_dpll_adev_get(mdpll, id);
	return mlx5_dpll_synce_status_set(mdpll_adev->mdev,
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

static int mlx5_dpll_synce_port_init(struct mlx5_dpll_adev *mdpll_adev,
				     struct net_device *netdev)
{
	int err;

	// WITH NEW DPLL API CHANGE, HERE WE REGISTER PIN OF TYPE SYNCE_PORT
	// PASSING THE NETDEV POINTER TO DPLL

	err = mlx5_dpll_synce_status_set(mdpll_adev->mdev,
					 MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
	if (err)
		return err;
	return 0;
}

static void mlx5_dpll_synce_port_cleanup(struct mlx5_dpll_adev *mdpll_adev)
{
	// WITH NEW DPLL API CHANGE, HERE WE UNREGISTER PIN OF TYPE SYNCE_PORT //
	mlx5_dpll_synce_status_set(mdpll_adev->mdev,
				   MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
}

static int mlx5_dpll_adev_notifier_event(struct notifier_block *nb,
					 unsigned long event, void *data)
{
	struct mlx5_dpll_adev *mdpll_adev =
			container_of(nb, struct mlx5_dpll_adev, adev_nb);
	struct net_device *netdev = data;

	switch (event) {
	case MLX5_ADEV_EVENT_NETDEV_ADDED:
		mlx5_dpll_synce_port_init(mdpll_adev, netdev);
		break;
	case MLX5_ADEV_EVENT_NETDEV_REMOVED:
		mlx5_dpll_synce_port_cleanup(mdpll_adev);
		break;
	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
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

static int mlx5_dpll_probe(struct auxiliary_device *adev,
			   const struct auxiliary_device_id *id)
{
	struct mlx5_adev *edev = container_of(adev, struct mlx5_adev, adev);
	struct mlx5_core_dev *mdev = edev->mdev;
	struct mlx5_dpll_adev *mdpll_adev;
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

	mdpll_adev = kzalloc(sizeof(*mdpll_adev), GFP_KERNEL);
	if (!mdpll_adev)
		return -ENOMEM;
	auxiliary_set_drvdata(adev, mdpll_adev);

	/* Multiple mdev instances might share one dpll clock.
	 * In case the previous instance initialized dpll, just take
	 * reference and quit.
	 */

	mutex_lock(&mlx5_dpll_ht_lock);
	mdpll = rhashtable_lookup_fast(&mlx5_dpll_ht, clock_id,
				       mlx5_dpll_ht_params);
	if (!mdpll) {
		mdpll = kzalloc(sizeof(*mdpll), GFP_KERNEL);
		if (!mdpll) {
			err = -ENOMEM;
			goto err_unlock;
		}
		memcpy(mdpll->clock_id, clock_id, sizeof(mdpll->clock_id));
		refcount_set(&mdpll->refcnt, 1);
		INIT_LIST_HEAD(&mdpll->adev_list);
		mutex_init(&mdpll->lock);

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
	} else {
		refcount_inc(&mdpll->refcnt);
	}

	mutex_unlock(&mlx5_dpll_ht_lock);
	mutex_lock(&mdpll->lock);
	list_add_tail(&mdpll_adev->list, &mdpll->adev_list);
	mutex_unlock(&mdpll->lock);
	mdpll_adev->dpll = mdpll;
	mdpll_adev->mdev = mdev;

	mdpll_adev->adev_nb.notifier_call = mlx5_dpll_adev_notifier_event;
	mlx5_adev_notifier_register(mdev, &mdpll_adev->adev_nb);

	return 0;

err_free_dpll_device:
	dpll_device_free(mdpll->dpll);
err_free_mdpll:
	kfree(mdpll);
err_unlock:
	mutex_unlock(&mlx5_dpll_ht_lock);
	kfree(mdpll_adev);
	return err;
}

static void mlx5_dpll_remove(struct auxiliary_device *adev)
{
	struct mlx5_dpll_adev *mdpll_adev = auxiliary_get_drvdata(adev);
	struct mlx5_core_dev *mdev = mdpll_adev->mdev;
	struct mlx5_dpll *mdpll = mdpll_adev->dpll;

	mlx5_adev_notifier_unregister(mdev, &mdpll_adev->adev_nb);

	mutex_lock(&mdpll->lock);
	list_del(&mdpll_adev->list);
	mutex_unlock(&mdpll->lock);
	kfree(mdpll_adev);

	mutex_lock(&mlx5_dpll_ht_lock);
	if (refcount_dec_and_test(&mdpll->refcnt)) {
		rhashtable_remove_fast(&mlx5_dpll_ht, &mdpll->ht_node,
				       mlx5_dpll_ht_params);
		dpll_device_unregister(mdpll->dpll);
		dpll_device_free(mdpll->dpll);
		mutex_destroy(&mdpll->lock);
		WARN_ON(!list_empty(&mdpll->adev_list));
		kfree(mdpll);
	}
	mutex_unlock(&mlx5_dpll_ht_lock);

	mlx5_dpll_synce_status_set(mdev,
				   MLX5_MSEES_ADMIN_STATUS_FREE_RUNNING);
}

static int mlx5_dpll_suspend(struct auxiliary_device *adev, pm_message_t state)
{
	return 0;
}

static int mlx5_dpll_resume(struct auxiliary_device *adev)
{
	return 0;
}

static const struct auxiliary_device_id mlx5_dpll_id_table[] = {
	{ .name = MLX5_ADEV_NAME ".dpll", },
	{},
};

MODULE_DEVICE_TABLE(auxiliary, mlx5_dpll_id_table);

static struct auxiliary_driver mlx5_dpll_driver = {
	.name = "dpll",
	.probe = mlx5_dpll_probe,
	.remove = mlx5_dpll_remove,
	.suspend = mlx5_dpll_suspend,
	.resume = mlx5_dpll_resume,
	.id_table = mlx5_dpll_id_table,
};

static int __init mlx5_dpll_init(void)
{
	int err;

	err = rhashtable_init(&mlx5_dpll_ht, &mlx5_dpll_ht_params);
	if (err)
		return err;

	err = auxiliary_driver_register(&mlx5_dpll_driver);
	if (err)
		goto err_rhashtable_destroy;
	return 0;

err_rhashtable_destroy:
	rhashtable_destroy(&mlx5_dpll_ht);
	return err;
}

static void __exit mlx5_dpll_exit(void)
{
	auxiliary_driver_unregister(&mlx5_dpll_driver);
	rhashtable_destroy(&mlx5_dpll_ht);
}

module_init(mlx5_dpll_init);
module_exit(mlx5_dpll_exit);

MODULE_AUTHOR("Jiri Pirko <jiri@nvidia.com>");
MODULE_DESCRIPTION("Mellanox 5th generation network adapters (ConnectX series) DPLL driver");
MODULE_LICENSE("Dual BSD/GPL");
