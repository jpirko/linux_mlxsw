// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * The test family on a misc device that answers read itself, as a subsystem
 * carries CTLV on a file of its own.
 */
#include <linux/ctlv.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/slab.h>

#include "ctlv_test.h"
#include "ctlv_test_gen.h"

static const char ctlv_test_misc_text[] = "ctlv-test-misc\n";

struct ctlv_test_misc {
	/* Counted but never added: open files pin it, and its release frees this. */
	struct device dev;
	struct miscdevice miscdev;
	struct ctlv_test_device test;
};

static struct ctlv_test_misc *ctlv_test_misc;

static int ctlv_test_misc_open(struct inode *inode, struct file *filp)
{
	struct ctlv_test_misc *misc =
		container_of(filp->private_data, struct ctlv_test_misc, miscdev);
	struct ctlv_file *cfile;

	cfile = ctlv_file_open(&misc->test.ctlv, filp);
	if (IS_ERR(cfile))
		return PTR_ERR(cfile);
	filp->private_data = cfile;
	return 0;
}

static int ctlv_test_misc_release(struct inode *inode, struct file *filp)
{
	ctlv_file_close(filp->private_data);
	return 0;
}

static ssize_t ctlv_test_misc_read(struct file *filp, char __user *buf,
				   size_t len, loff_t *pos)
{
	return simple_read_from_buffer(buf, len, pos, ctlv_test_misc_text,
				       sizeof(ctlv_test_misc_text) - 1);
}

static long ctlv_test_misc_ioctl(struct file *filp, unsigned int cmd,
				 unsigned long arg)
{
	long ret = ctlv_file_ioctl(filp->private_data, cmd, arg);

	return ret == -ENOIOCTLCMD ? -ENOTTY : ret;
}

/* Open files hold this module, so no op is in flight when it exits. */
static const struct file_operations ctlv_test_misc_fops = {
	.owner = THIS_MODULE,
	.open = ctlv_test_misc_open,
	.release = ctlv_test_misc_release,
	.read = ctlv_test_misc_read,
	.unlocked_ioctl = ctlv_test_misc_ioctl,
	.compat_ioctl = compat_ptr_ioctl,
	.llseek = default_llseek,
};

static void ctlv_test_misc_dev_release(struct device *dev)
{
	struct ctlv_test_misc *misc =
		container_of(dev, struct ctlv_test_misc, dev);

	ctlv_fini(&misc->test.ctlv);
	kfree(misc);
}

static int __init ctlv_test_misc_init(void)
{
	const struct ctlv_test_init_info info = {};
	struct ctlv_test_misc *misc;
	int ret;

	misc = kzalloc_obj(*misc, GFP_KERNEL);
	if (!misc)
		return -ENOMEM;
	misc->dev.release = ctlv_test_misc_dev_release;
	device_initialize(&misc->dev);
	ret = dev_set_name(&misc->dev, "ctlv-test-misc");
	if (ret)
		goto err_put;
	ret = ctlv_test_device_init(&misc->test, &misc->dev, &ctlv_test_family,
				    NULL, &info);
	if (ret)
		goto err_put;
	ctlv_publish(&misc->test.ctlv);
	misc->miscdev.minor = MISC_DYNAMIC_MINOR;
	misc->miscdev.name = "ctlv-test-misc";
	misc->miscdev.fops = &ctlv_test_misc_fops;
	misc->miscdev.mode = 0600;
	ret = misc_register(&misc->miscdev);
	if (ret)
		goto err_unpublish;
	ctlv_test_misc = misc;
	return 0;

err_unpublish:
	ctlv_unpublish(&misc->test.ctlv);
	ctlv_test_device_fini(&misc->test);
err_put:
	put_device(&misc->dev);
	return ret;
}

static void __exit ctlv_test_misc_exit(void)
{
	struct ctlv_test_misc *misc = ctlv_test_misc;

	misc_deregister(&misc->miscdev);
	ctlv_unpublish(&misc->test.ctlv);
	ctlv_test_device_fini(&misc->test);
	put_device(&misc->dev);
}

module_init(ctlv_test_misc_init);
module_exit(ctlv_test_misc_exit);
MODULE_DESCRIPTION("CTLV test device on a misc device");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("CTLV");
