// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Tests of the character device nodes.
 */

#include <kunit/device.h>
#include <kunit/test.h>
#include <linux/atomic.h>
#include <linux/ctlv.h>
#include <linux/ctlv_device.h>
#include <linux/ctlv_gen.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>

/* The node after the instance, so that its offset is not zero. */
struct ctlv_device_test_node {
	struct ctlv ctlv;
	struct ctlv_device ctlvdev;
};

/* The file the test opens the node with. */
struct ctlv_device_test_file {
	struct inode inode;
	struct file filp;
};

static atomic_t ctlv_device_test_released = ATOMIC_INIT(0);

static void ctlv_device_test_release(struct device *dev)
{
	struct ctlv_device_test_node *node =
		container_of(dev, struct ctlv_device_test_node, ctlvdev.dev);

	ctlv_fini(&node->ctlv);
	ctlv_device_fini(&node->ctlvdev);
	kfree(node);
	atomic_inc(&ctlv_device_test_released);
}

/* Two nodes, so that a test can fill the class. */
static struct ctlv_device_class ctlv_device_test_class = {
	.class = {
		.name = "ctlv-kunit",
		.dev_release = ctlv_device_test_release,
	},
	.max_devices = 2,
};

static void ctlv_device_test_put(void *node)
{
	put_device(&((struct ctlv_device_test_node *)node)->ctlvdev.dev);
}

/* A node of the test class, put when the test ends, whether or not @ret is 0. */
static struct ctlv_device_test_node *
ctlv_device_test_alloc(struct kunit *test, struct device *parent, int *ret)
{
	struct ctlv_device_test_node *node;

	node = kzalloc_obj(*node, GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, node);
	*ret = ctlv_device_init(&node->ctlvdev, &ctlv_device_test_class, parent, &node->ctlv);
	KUNIT_ASSERT_EQ(test, kunit_add_action_or_reset(test, ctlv_device_test_put, node), 0);
	return node;
}

static unsigned int ctlv_device_test_minor(struct ctlv_device_test_node *node)
{
	return MINOR(node->ctlvdev.dev.devt);
}

/* A node is numbered and named in its class, and its file pins the class's module. */
static void ctlv_device_test_init(struct kunit *test)
{
	struct ctlv_device_test_node *node;
	struct device *parent;
	char name[32];
	int ret;

	parent = kunit_device_register(test, "ctlv-device-init");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	node = ctlv_device_test_alloc(test, parent, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);

	snprintf(name, sizeof(name), "ctlv-kunit%u", ctlv_device_test_minor(node));
	KUNIT_EXPECT_STREQ(test, dev_name(&node->ctlvdev.dev), name);
	KUNIT_EXPECT_EQ(test, MAJOR(node->ctlvdev.dev.devt), MAJOR(ctlv_device_test_class.devt));
	KUNIT_EXPECT_PTR_EQ(test, node->ctlvdev.dev.class, &ctlv_device_test_class.class);
	KUNIT_EXPECT_PTR_EQ(test, node->ctlvdev.dev.parent, parent);
	KUNIT_EXPECT_PTR_EQ(test, node->ctlvdev.cdev.owner, THIS_MODULE);
	KUNIT_EXPECT_PTR_EQ(test, node->ctlvdev.ctlv, &node->ctlv);
}

/* A full class refuses a node, and a node's number is the next one's once it goes. */
static void ctlv_device_test_numbers(struct kunit *test)
{
	struct ctlv_device_test_node *first, *second, *node;
	unsigned int minor;
	int released, ret;

	first = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	second = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_EXPECT_NE(test, ctlv_device_test_minor(first), ctlv_device_test_minor(second));

	released = atomic_read(&ctlv_device_test_released);
	node = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_EXPECT_EQ(test, ret, -ENOSPC);
	kunit_release_action(test, ctlv_device_test_put, node);
	KUNIT_EXPECT_EQ(test, atomic_read(&ctlv_device_test_released), released + 1);
	KUNIT_EXPECT_TRUE(test, ida_exists(&ctlv_device_test_class.ids,
					   ctlv_device_test_minor(first)));
	KUNIT_EXPECT_TRUE(test, ida_exists(&ctlv_device_test_class.ids,
					   ctlv_device_test_minor(second)));

	minor = ctlv_device_test_minor(first);
	kunit_release_action(test, ctlv_device_test_put, first);
	node = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, ctlv_device_test_minor(node), minor);
}

/* put_device() alone unwinds a node whose instance failed to initialize. */
static void ctlv_device_test_unwind(struct kunit *test)
{
	struct ctlv_device_test_node *node;
	unsigned int minor;
	int released, ret;

	node = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_EXPECT_EQ(test, ctlv_init(&node->ctlv, &node->ctlvdev.dev, NULL, NULL), -EINVAL);

	minor = ctlv_device_test_minor(node);
	released = atomic_read(&ctlv_device_test_released);
	kunit_release_action(test, ctlv_device_test_put, node);
	KUNIT_EXPECT_EQ(test, atomic_read(&ctlv_device_test_released), released + 1);
	KUNIT_EXPECT_FALSE(test, ida_exists(&ctlv_device_test_class.ids, minor));
}

/* Only a published instance of the node's own device has the node added. */
static void ctlv_device_test_add_refused(struct kunit *test)
{
	struct ctlv_device_test_node *node, *other;
	struct device *parent;
	int ret;

	parent = kunit_device_register(test, "ctlv-device-add");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, parent);
	node = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_EQ(test, ctlv_init(&node->ctlv, &node->ctlvdev.dev, &ctlv_family, NULL), 0);
	other = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_EQ(test, ctlv_init(&other->ctlv, parent, &ctlv_family, NULL), 0);
	ctlv_publish(&other->ctlv);

	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, ctlv_device_add(&node->ctlvdev), -EINVAL);
		KUNIT_EXPECT_EQ(test, ctlv_device_add(&other->ctlvdev), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 2);
	}
	ctlv_unpublish(&other->ctlv);
	KUNIT_EXPECT_FALSE(test, device_is_registered(&node->ctlvdev.dev));
	KUNIT_EXPECT_FALSE(test, device_is_registered(&other->ctlvdev.dev));
}

/* The node announces its family, and a file open on it sees the device go. */
static void ctlv_device_test_add_del(struct kunit *test)
{
	const struct file_operations *fops;
	struct ctlv_device_test_node *node;
	struct ctlv_device_test_file *file;
	struct kobj_uevent_env *env;
	int released, ret;
	__poll_t mask;

	file = kunit_kzalloc(test, sizeof(*file), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, file);
	env = kunit_kzalloc(test, sizeof(*env), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, env);
	node = ctlv_device_test_alloc(test, NULL, &ret);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_EQ(test, ctlv_init(&node->ctlv, &node->ctlvdev.dev, &ctlv_family, NULL), 0);
	ctlv_publish(&node->ctlv);
	KUNIT_ASSERT_EQ(test, ctlv_device_add(&node->ctlvdev), 0);
	KUNIT_EXPECT_TRUE(test, device_is_registered(&node->ctlvdev.dev));

	KUNIT_ASSERT_EQ(test, node->ctlvdev.dev.class->dev_uevent(&node->ctlvdev.dev, env), 0);
	KUNIT_ASSERT_GT(test, env->envp_idx, 0);
	KUNIT_EXPECT_STREQ(test, env->envp[0], "CTLV_FAMILY=ctlv");

	fops = node->ctlvdev.cdev.ops;
	file->inode.i_cdev = &node->ctlvdev.cdev;
	file->filp.f_mode = FMODE_READ;
	KUNIT_ASSERT_EQ(test, fops->open(&file->inode, &file->filp), 0);

	ctlv_device_del(&node->ctlvdev);
	ctlv_unpublish(&node->ctlv);
	KUNIT_EXPECT_FALSE(test, device_is_registered(&node->ctlvdev.dev));
	mask = fops->poll(&file->filp, NULL);
	KUNIT_EXPECT_EQ(test, mask & (EPOLLIN | EPOLLHUP), EPOLLIN | EPOLLHUP);

	released = atomic_read(&ctlv_device_test_released);
	kunit_release_action(test, ctlv_device_test_put, node);
	KUNIT_EXPECT_EQ(test, atomic_read(&ctlv_device_test_released), released);
	fops->release(&file->inode, &file->filp);
	KUNIT_EXPECT_EQ(test, atomic_read(&ctlv_device_test_released), released + 1);
}

/* The numbers are freed by the releases, which this option delays. */
static int ctlv_device_test_case_init(struct kunit *test)
{
	if (IS_ENABLED(CONFIG_DEBUG_KOBJECT_RELEASE))
		kunit_skip(test, "device releases are delayed");
	return 0;
}

static int ctlv_device_test_suite_init(struct kunit_suite *suite)
{
	return ctlv_device_class_register(&ctlv_device_test_class);
}

static void ctlv_device_test_suite_exit(struct kunit_suite *suite)
{
	ctlv_device_class_unregister(&ctlv_device_test_class);
}

static struct kunit_case ctlv_device_test_cases[] = {
	KUNIT_CASE(ctlv_device_test_init),
	KUNIT_CASE(ctlv_device_test_numbers),
	KUNIT_CASE(ctlv_device_test_unwind),
	KUNIT_CASE(ctlv_device_test_add_refused),
	KUNIT_CASE(ctlv_device_test_add_del),
	{},
};

static struct kunit_suite ctlv_device_test_suite = {
	.name = "ctlv_device",
	.init = ctlv_device_test_case_init,
	.suite_init = ctlv_device_test_suite_init,
	.suite_exit = ctlv_device_test_suite_exit,
	.test_cases = ctlv_device_test_cases,
};

kunit_test_suite(ctlv_device_test_suite);
