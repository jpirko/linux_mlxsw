/* SPDX-License-Identifier: GPL-2.0 */
/*
 *  Copyright (c) 2021 Meta Platforms, Inc. and affiliates
 */

#ifndef __DPLL_CORE_H__
#define __DPLL_CORE_H__

#include <linux/dpll.h>

#include "dpll_netlink.h"

/**
 * struct dpll_device - structure for a DPLL device
 * @id:		unique id number for each edvice
 * @dev:	&struct device for this dpll device
 * @sources_count:	amount of input sources this dpll_device supports
 * @outputs_count:	amount of outputs this dpll_device supports
 * @ops:	operations this &dpll_device supports
 * @lock:	mutex to serialize operations
 * @priv:	pointer to private information of owner
 */
struct dpll_device {
	int id;
	struct device dev;
	int sources_count;
	int outputs_count;
	struct dpll_device_ops *ops;
	struct mutex lock;
	void *priv;
};

#define to_dpll_device(_dev) \
	container_of(_dev, struct dpll_device, dev)

int for_each_dpll_device(int id, int (*cb)(struct dpll_device *, void *),
			  void *data);
struct dpll_device *dpll_device_get_by_id(int id);
struct dpll_device *dpll_device_get_by_name(const char *name);
void dpll_device_unregister(struct dpll_device *dpll);
#endif
