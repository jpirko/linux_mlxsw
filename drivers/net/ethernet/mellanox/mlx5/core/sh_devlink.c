// SPDX-License-Identifier: GPL-2.0 OR Linux-OpenIB
/* Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/device/faux.h>
#include <linux/mlx5/driver.h>
#include <linux/mlx5/vport.h>

#include "sh_devlink.h"

static LIST_HEAD(shd_list);
static DEFINE_MUTEX(shd_mutex); /* Protects shd_list and shd->list */

/* This structure represents a shared devlink instance,
 * there is one created for PF group of the same chip.
 */
struct mlx5_shd {
	/* Node in shd list */
	struct list_head list;
	/* Serial number of the chip */
	const char *sn;
	/* List of per-PF dev instances. */
	struct list_head dev_list;
	/* Related faux device */
	struct faux_device *faux_dev;
};

static const struct devlink_ops mlx5_shd_ops = {
};

static int mlx5_shd_faux_probe(struct faux_device *faux_dev)
{
	struct devlink *devlink;
	struct mlx5_shd *shd;

	devlink = devlink_alloc(&mlx5_shd_ops, sizeof(struct mlx5_shd), &faux_dev->dev);
	if (!devlink)
		return -ENOMEM;
	shd = devlink_priv(devlink);
	faux_device_set_drvdata(faux_dev, shd);

	devl_lock(devlink);
	devl_register(devlink);
	devl_unlock(devlink);
	return 0;
}

static void mlx5_shd_faux_remove(struct faux_device *faux_dev)
{
	struct mlx5_shd *shd = faux_device_get_drvdata(faux_dev);
	struct devlink *devlink = priv_to_devlink(shd);

	devl_lock(devlink);
	devl_unregister(devlink);
	devl_unlock(devlink);
	devlink_free(devlink);
}

static const struct faux_device_ops mlx5_shd_faux_ops = {
	.probe = mlx5_shd_faux_probe,
	.remove = mlx5_shd_faux_remove,
};

static struct mlx5_shd *mlx5_shd_create(const char *sn)
{
	struct faux_device *faux_dev;
	struct mlx5_shd *shd;

	faux_dev = faux_device_create(sn, NULL, &mlx5_shd_faux_ops);
	if (!faux_dev)
		return NULL;
	shd = faux_device_get_drvdata(faux_dev);
	if (!shd)
		return NULL;
	list_add_tail(&shd->list, &shd_list);
	shd->sn = sn;
	INIT_LIST_HEAD(&shd->dev_list);
	shd->faux_dev = faux_dev;
	return shd;
}

static void mlx5_shd_destroy(struct mlx5_shd *shd)
{
	list_del(&shd->list);
	kfree(shd->sn);
	faux_device_destroy(shd->faux_dev);
}

int mlx5_shd_init(struct mlx5_core_dev *dev)
{
	u8 *vpd_data __free(kfree) = NULL;
	struct pci_dev *pdev = dev->pdev;
	unsigned int vpd_size, kw_len;
	struct mlx5_shd *shd;
	const char *sn;
	char *end;
	int start;
	int err;

	if (!mlx5_core_is_pf(dev))
		return 0;

	vpd_data = pci_vpd_alloc(pdev, &vpd_size);
	if (IS_ERR(vpd_data)) {
		err = PTR_ERR(vpd_data);
		return err == -ENODEV ? 0 : err;
	}
	start = pci_vpd_find_ro_info_keyword(vpd_data, vpd_size, "V3", &kw_len);
	if (start < 0) {
		/* Fall-back to SN for older devices. */
		start = pci_vpd_find_ro_info_keyword(vpd_data, vpd_size,
						     PCI_VPD_RO_KEYWORD_SERIALNO, &kw_len);
		if (start < 0)
			return -ENOENT;
	}
	sn = kstrndup(vpd_data + start, kw_len, GFP_KERNEL);
	if (!sn)
		return -ENOMEM;
	end = strchrnul(sn, ' ');
	*end = '\0';

	guard(mutex)(&shd_mutex);
	list_for_each_entry(shd, &shd_list, list) {
		if (!strcmp(shd->sn, sn)) {
			kfree(sn);
			goto found;
		}
	}
	shd = mlx5_shd_create(sn);
	if (!shd) {
		kfree(sn);
		return -ENOMEM;
	}
found:
	list_add_tail(&dev->shd_list, &shd->dev_list);
	dev->shd = shd;
	return 0;
}

void mlx5_shd_uninit(struct mlx5_core_dev *dev)
{
	struct mlx5_shd *shd = dev->shd;

	if (!dev->shd)
		return;

	guard(mutex)(&shd_mutex);
	list_del(&dev->shd_list);
	if (list_empty(&shd->dev_list))
		mlx5_shd_destroy(shd);
}
