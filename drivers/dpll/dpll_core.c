// SPDX-License-Identifier: GPL-2.0
/*
 *  dpll_core.c - Generic DPLL Management class support.
 *
 *  Copyright (c) 2021 Meta Platforms, Inc. and affiliates
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/string.h>

#include "dpll_core.h"

static DEFINE_MUTEX(dpll_device_xa_lock);
static DEFINE_XARRAY_FLAGS(dpll_device_xa, XA_FLAGS_ALLOC);
#define DPLL_REGISTERED XA_MARK_1

#define ASSERT_DPLL_REGISTERED(d)                                           \
	WARN_ON_ONCE(!xa_get_mark(&dpll_device_xa, (d)->id, DPLL_REGISTERED))
#define ASSERT_DPLL_NOT_REGISTERED(d)                                      \
	WARN_ON_ONCE(xa_get_mark(&dpll_device_xa, (d)->id, DPLL_REGISTERED))


int for_each_dpll_device(int id, int (*cb)(struct dpll_device *, void *),
			 void *data)
{
	struct dpll_device *dpll;
	unsigned long index;
	int ret = 0;

	mutex_lock(&dpll_device_xa_lock);
	xa_for_each_start(&dpll_device_xa, index, dpll, id) {
		if (!xa_get_mark(&dpll_device_xa, index, DPLL_REGISTERED))
			continue;
		ret = cb(dpll, data);
		if (ret)
			break;
	}
	mutex_unlock(&dpll_device_xa_lock);

	return ret;
}

struct dpll_device *dpll_device_get_by_id(int id)
{
	struct dpll_device *dpll = NULL;

	if (xa_get_mark(&dpll_device_xa, id, DPLL_REGISTERED))
		dpll = xa_load(&dpll_device_xa, id);
	return dpll;
}

struct dpll_device *dpll_device_get_by_name(const char *name)
{
	struct dpll_device *dpll, *ret = NULL;
	unsigned long index;

	mutex_lock(&dpll_device_xa_lock);
	xa_for_each_marked(&dpll_device_xa, index, dpll, DPLL_REGISTERED) {
		if (!strcmp(dev_name(&dpll->dev), name)) {
			ret = dpll;
			break;
		}
	}
	mutex_unlock(&dpll_device_xa_lock);

	return ret;
}

void *dpll_priv(struct dpll_device *dpll)
{
	return dpll->priv;
}
EXPORT_SYMBOL_GPL(dpll_priv);

static void dpll_device_release(struct device *dev)
{
	struct dpll_device *dpll;

	dpll = to_dpll_device(dev);

	dpll_device_unregister(dpll);
	dpll_device_free(dpll);
}

static struct class dpll_class = {
	.name = "dpll",
	.dev_release = dpll_device_release,
};

struct dpll_device *dpll_device_alloc(struct dpll_device_ops *ops, const char *name,
				      int sources_count, int outputs_count, void *priv)
{
	struct dpll_device *dpll;
	int ret;

	dpll = kzalloc(sizeof(*dpll), GFP_KERNEL);
	if (!dpll)
		return ERR_PTR(-ENOMEM);

	mutex_init(&dpll->lock);
	dpll->ops = ops;
	dpll->dev.class = &dpll_class;
	dpll->sources_count = sources_count;
	dpll->outputs_count = outputs_count;

	mutex_lock(&dpll_device_xa_lock);
	ret = xa_alloc(&dpll_device_xa, &dpll->id, dpll, xa_limit_16b, GFP_KERNEL);
	if (ret)
		goto error;
	dev_set_name(&dpll->dev, "%s%d", name ? name : "dpll", dpll->id);
	mutex_unlock(&dpll_device_xa_lock);
	dpll->priv = priv;

	dpll_notify_device_create(dpll->id, dev_name(&dpll->dev));

	return dpll;

error:
	mutex_unlock(&dpll_device_xa_lock);
	kfree(dpll);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_GPL(dpll_device_alloc);

void dpll_device_free(struct dpll_device *dpll)
{
	if (!dpll)
		return;

	mutex_destroy(&dpll->lock);
	kfree(dpll);
}
EXPORT_SYMBOL_GPL(dpll_device_free);

void dpll_device_register(struct dpll_device *dpll)
{
	ASSERT_DPLL_NOT_REGISTERED(dpll);

	mutex_lock(&dpll_device_xa_lock);
	xa_set_mark(&dpll_device_xa, dpll->id, DPLL_REGISTERED);
	mutex_unlock(&dpll_device_xa_lock);
}
EXPORT_SYMBOL_GPL(dpll_device_register);

void dpll_device_unregister(struct dpll_device *dpll)
{
	ASSERT_DPLL_REGISTERED(dpll);

	mutex_lock(&dpll_device_xa_lock);
	xa_erase(&dpll_device_xa, dpll->id);
	dpll_notify_device_delete(dpll->id);
	mutex_unlock(&dpll_device_xa_lock);
}
EXPORT_SYMBOL_GPL(dpll_device_unregister);

static int __init dpll_init(void)
{
	int ret;

	ret = dpll_netlink_init();
	if (ret)
		goto error;

	ret = class_register(&dpll_class);
	if (ret)
		goto unregister_netlink;

	return 0;

unregister_netlink:
	dpll_netlink_finish();
error:
	mutex_destroy(&dpll_device_xa_lock);
	return ret;
}
subsys_initcall(dpll_init);
