// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Character device nodes of CTLV instances.
 */
#include <linux/bug.h>
#include <linux/cdev.h>
#include <linux/cleanup.h>
#include <linux/container_of.h>
#include <linux/ctlv.h>
#include <linux/ctlv_device.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/rwsem.h>

#include "internal.h"

static int ctlv_device_open(struct inode *inode, struct file *filp)
{
	struct ctlv_device *ctlvdev = container_of(inode->i_cdev, struct ctlv_device, cdev);
	struct ctlv_file *cfile;

	cfile = ctlv_file_open(ctlvdev->ctlv, filp);
	if (IS_ERR(cfile))
		return PTR_ERR(cfile);
	filp->private_data = cfile;
	return 0;
}

static int ctlv_device_release(struct inode *inode, struct file *filp)
{
	ctlv_file_close(filp->private_data);
	return 0;
}

static long ctlv_device_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	return ctlv_file_ioctl(filp->private_data, cmd, arg);
}

static const struct file_operations ctlv_device_operations = {
	.owner = THIS_MODULE,
	.open = ctlv_device_open,
	.release = ctlv_device_release,
	.read = ctlv_fops_read,
	.poll = ctlv_fops_poll,
	.unlocked_ioctl = ctlv_device_ioctl,
	/* Every ioctl field is fixed width, so one entry point serves both. */
	.compat_ioctl = compat_ptr_ioctl,
};

static int ctlv_device_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct ctlv_device *ctlvdev = container_of_const(dev, struct ctlv_device, dev);

	return ctlv_uevent(ctlvdev->ctlv, env);
}

/**
 * __ctlv_device_class_register - Register a class of nodes.
 * @cls: class, its name, dev_release and max_devices set
 * @owner: module of the dev_release, held while a node is open
 *
 * Called as ctlv_device_class_register(), which passes the calling module.
 *
 * Return: 0, or a negative error.
 */
int __ctlv_device_class_register(struct ctlv_device_class *cls, struct module *owner)
{
	int ret;

	if (WARN_ON(!cls->max_devices))
		return -EINVAL;
	ret = alloc_chrdev_region(&cls->devt, 0, cls->max_devices, cls->class.name);
	if (ret)
		return ret;
	cls->owner = owner;
	cls->class.dev_uevent = ctlv_device_uevent;
	ida_init(&cls->ids);
	ret = class_register(&cls->class);
	if (ret)
		unregister_chrdev_region(cls->devt, cls->max_devices);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(__ctlv_device_class_register, "CTLV");

/**
 * ctlv_device_class_unregister - Unregister a class of nodes.
 * @cls: class whose nodes are all released
 */
void ctlv_device_class_unregister(struct ctlv_device_class *cls)
{
	class_unregister(&cls->class);
	unregister_chrdev_region(cls->devt, cls->max_devices);
	ida_destroy(&cls->ids);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_class_unregister, "CTLV");

/**
 * ctlv_device_init - Prepare the node of an instance.
 * @ctlvdev: zeroed node, in the structure the class's dev_release frees
 * @cls: registered class
 * @parent: parent device, or NULL
 * @ctlv: instance, for ctlv_init() with the node's device
 *
 * From here on, put_device() unwinds, whether or not this succeeds.
 *
 * Return: 0, or a negative error.
 */
int ctlv_device_init(struct ctlv_device *ctlvdev, struct ctlv_device_class *cls,
		     struct device *parent, struct ctlv *ctlv)
{
	int devnum;

	device_initialize(&ctlvdev->dev);
	ctlvdev->dev.class = &cls->class;
	ctlvdev->dev.parent = parent;
	ctlvdev->ctlv = ctlv;
	cdev_init(&ctlvdev->cdev, &ctlv_device_operations);
	ctlvdev->cdev.owner = cls->owner;

	devnum = ida_alloc_max(&cls->ids, cls->max_devices - 1, GFP_KERNEL);
	if (devnum < 0)
		return devnum;
	ctlvdev->cls = cls;
	ctlvdev->dev.devt = MKDEV(MAJOR(cls->devt), devnum);
	return dev_set_name(&ctlvdev->dev, "%s%d", cls->class.name, devnum);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_init, "CTLV");

/**
 * ctlv_device_fini - Free the node's number.
 * @ctlvdev: node
 *
 * For the class's dev_release, before it frees the node.
 */
void ctlv_device_fini(struct ctlv_device *ctlvdev)
{
	if (ctlvdev->cls)
		ida_free(&ctlvdev->cls->ids, MINOR(ctlvdev->dev.devt));
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_fini, "CTLV");

/**
 * ctlv_device_add - Add the node.
 * @ctlvdev: node, whose instance was initialized with its device and published
 *
 * The node may be opened as soon as it appears, so its instance is published
 * before and unpublished after ctlv_device_del().
 *
 * Return: 0, or a negative error.
 */
int ctlv_device_add(struct ctlv_device *ctlvdev)
{
	struct ctlv *ctlv = ctlvdev->ctlv;
	bool published;

	scoped_guard(rwsem_read, &ctlv->registration_lock)
		published = ctlv->registered;
	if (WARN_ON(ctlv->dev != &ctlvdev->dev || !published))
		return -EINVAL;
	return cdev_device_add(&ctlvdev->cdev, &ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_add, "CTLV");

/**
 * ctlv_device_del - Remove the node.
 * @ctlvdev: added node
 *
 * A file still open keeps the node's device until it is closed.
 */
void ctlv_device_del(struct ctlv_device *ctlvdev)
{
	cdev_device_del(&ctlvdev->cdev, &ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_del, "CTLV");
