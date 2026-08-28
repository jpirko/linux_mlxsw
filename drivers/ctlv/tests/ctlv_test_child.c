// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Inheriting test family and extension-point implementation.
 */
#define pr_fmt(fmt) "ctlv_test_child: " fmt
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kstrtox.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>

#include "ctlv_test.h"
#include "ctlv_test_child_gen.h"

struct ctlv_test_child_device {
	struct ctlv_device *ctlvdev;
	u32 magic;
};

#define CTLV_TEST_CHILD_PRIV_MAGIC 0x6368696c

struct ctlv_test_child_init_info {
	struct ctlv_test_init_info parent;
};

size_t ctlv_test_child_priv_size(void)
{
	return sizeof(struct ctlv_test_child_device);
}

const void *ctlv_test_child_pre_init(const void *init_info)
{
	const struct ctlv_test_child_init_info *info = init_info;

	return &info->parent;
}

int ctlv_test_child_init(struct ctlv_device *ctlvdev,
			 const void *init_info, void *priv)
{
	struct ctlv_test_child_device *child = priv;

	child->ctlvdev = ctlvdev;
	child->magic = CTLV_TEST_CHILD_PRIV_MAGIC;
	return 0;
}

void ctlv_test_child_pre_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct ctlv_test_child_device *child = priv;

	WARN_ON_ONCE(child->magic != CTLV_TEST_CHILD_PRIV_MAGIC ||
		     ctlv_device_family(ctlvdev) !=
		     &ctlv_test_child_family);
}

void ctlv_test_child_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct ctlv_test_child_device *child = priv;

	child->magic = 0;
}

/* Separate logs identify child hooks and extension reads; full logs stop. */

#define CTLV_TEST_CHILD_LOG_LEN 1024
#define CTLV_TEST_CHILD_TOKEN_LEN 128

struct ctlv_test_child_log {
	/* Serializes the recording of one token against every other. */
	spinlock_t lock;
	size_t len;
	char text[CTLV_TEST_CHILD_LOG_LEN];
};

static struct ctlv_test_child_log ctlv_test_child_hooks_log = {
	.lock = __SPIN_LOCK_UNLOCKED(ctlv_test_child_hooks_log.lock),
};

static struct ctlv_test_child_log ctlv_test_child_echo_log = {
	.lock = __SPIN_LOCK_UNLOCKED(ctlv_test_child_echo_log.lock),
};

static bool ctlv_test_child_fail_family_pre;

/* One space-separated token, dropped whole if the log has no room for it. */
static __printf(2, 3) void
ctlv_test_child_log_add(struct ctlv_test_child_log *log, const char *fmt, ...)
{
	char token[CTLV_TEST_CHILD_TOKEN_LEN];
	va_list args;
	size_t len;

	va_start(args, fmt);
	len = vscnprintf(token, sizeof(token), fmt, args);
	va_end(args);

	guard(spinlock)(&log->lock);
	if (log->len + len + 2 > sizeof(log->text))
		return;
	log->len += scnprintf(log->text + log->len,
			      sizeof(log->text) - log->len, "%s%s",
			      log->len ? " " : "", token);
}

static void ctlv_test_child_device_log(const struct ctlv_test_child_device *child,
		     const char *what)
{
	ctlv_test_child_log_add(&ctlv_test_child_hooks_log, "%s",
				child &&
				child->magic == CTLV_TEST_CHILD_PRIV_MAGIC ?
				what : "wrong-device");
}

int ctlv_test_child_extra_noop_action(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request,
				      struct ctlv_cursor *reply)
{
	ctlv_test_child_device_log(priv, "call");
	return 0;
}

int ctlv_test_child_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_child_device_log(priv, "family-pre");
	if (ctlv_test_child_fail_family_pre)
		return ctlv_op_error(ctx, -EIO,
			"test child family pre hook refused the operation");
	return 0;
}

void ctlv_test_child_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_child_device_log(priv, "family-post");
}

/* Echo a request fill; a reply-only fill remains empty. */
int ctlv_test_child_extension_point_fill(void *priv,
					 struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *nest)
{
	struct ctlv_test_child_device *child = priv;

	if (WARN_ON_ONCE(child->magic != CTLV_TEST_CHILD_PRIV_MAGIC))
		return -EINVAL;
	if (!request ||
	    !ctlv_test_child_extension_point_extra_present(request))
		return 0;
	ctlv_test_child_child_extra_put(nest,
					ctlv_test_child_extension_point_extra_get(request));
	return 0;
}

/* And what arrived inside one is reported in this family's own echo log. */
void ctlv_test_child_extension_point_read(void *priv,
					  struct ctlv_op_ctx *ctx,
					  const struct ctlv_cursor *nest)
{
	struct ctlv_test_child_device *child = priv;

	if (WARN_ON_ONCE(child->magic != CTLV_TEST_CHILD_PRIV_MAGIC))
		return;
	if (!ctlv_test_child_extension_point_extra_present(nest))
		return;
	ctlv_test_child_log_add(&ctlv_test_child_echo_log, "child.extra=%llu",
				ctlv_test_child_extension_point_extra_get(nest));
}

/* What every device of this family is made under, there being no hardware. */
static struct device *ctlv_test_child_parent;

static void ctlv_test_child_device_remove(struct ctlv_device *ctlvdev)
{
	ctlv_destroy(ctlvdev);
}

static struct ctlv_device *
ctlv_test_child_device_add(bool withhold_ops)
{
	struct ctlv_test_child_init_info init_info = {
		.parent = {
			.withhold_ops = withhold_ops,
			.dynamic_disabled = false,
		},
	};

	return ctlv_create(ctlv_test_child_parent,
			   &ctlv_test_child_family, &init_info);
}

static struct ctlv_device *
ctlv_test_child_devices[CTLV_TEST_MAX_DEVICES];
/* Protects ctlv_test_child_devices. */
static DEFINE_MUTEX(ctlv_test_child_devices_lock);

static int
ctlv_test_child_device_add_parse(const char __user *buf, size_t len,
				 unsigned int *index, bool *withhold_ops)
{
	unsigned int withhold;
	char *withhold_text;
	char *index_text;
	char text[32];
	char *at;

	if (!len || len >= sizeof(text))
		return -EINVAL;
	if (copy_from_user(text, buf, len))
		return -EFAULT;
	text[len] = '\0';
	at = skip_spaces(text);
	index_text = strsep(&at, " \t\r\n");
	if (!at)
		return -EINVAL;
	at = skip_spaces(at);
	withhold_text = strsep(&at, " \t\r\n");
	if (!*index_text || !*withhold_text ||
	    (at && *skip_spaces(at)) ||
	    kstrtouint(index_text, 10, index) ||
	    kstrtouint(withhold_text, 10, &withhold) ||
	    *index >= ARRAY_SIZE(ctlv_test_child_devices) || withhold > 1)
		return -EINVAL;
	*withhold_ops = withhold;
	return 0;
}

static int
ctlv_test_child_device_remove_parse(const char __user *buf, size_t len,
				    unsigned int *index)
{
	char *index_text;
	char text[32];
	char *at;

	if (!len || len >= sizeof(text))
		return -EINVAL;
	if (copy_from_user(text, buf, len))
		return -EFAULT;
	text[len] = '\0';
	at = skip_spaces(text);
	index_text = strsep(&at, " \t\r\n");
	if (!*index_text || (at && *skip_spaces(at)) ||
	    kstrtouint(index_text, 10, index) ||
	    *index >= ARRAY_SIZE(ctlv_test_child_devices))
		return -EINVAL;
	return 0;
}

/* Write "<index> <withhold_ops>"; adding an existing index is a no-op. */
static ssize_t ctlv_test_child_device_add_write(struct file *file,
						const char __user *buf,
						size_t len, loff_t *pos)
{
	struct ctlv_device *ctlvdev;
	unsigned int index;
	bool withhold_ops;
	int ret;

	ret = ctlv_test_child_device_add_parse(buf, len, &index, &withhold_ops);
	if (ret)
		return ret;
	guard(mutex)(&ctlv_test_child_devices_lock);
	if (ctlv_test_child_devices[index])
		return len;
	ctlvdev = ctlv_test_child_device_add(withhold_ops);
	if (IS_ERR(ctlvdev))
		return PTR_ERR(ctlvdev);
	ctlv_test_child_devices[index] = ctlvdev;
	return len;
}

static const struct file_operations ctlv_test_child_device_add_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = ctlv_test_child_device_add_write,
	.llseek = noop_llseek,
};

static void ctlv_test_child_device_slot_remove(unsigned int index)
{
	lockdep_assert_held(&ctlv_test_child_devices_lock);
	if (!ctlv_test_child_devices[index])
		return;
	ctlv_test_child_device_remove(ctlv_test_child_devices[index]);
	ctlv_test_child_devices[index] = NULL;
}

static ssize_t ctlv_test_child_device_remove_write(struct file *file,
						   const char __user *buf,
						   size_t len, loff_t *pos)
{
	unsigned int index;
	int ret;

	ret = ctlv_test_child_device_remove_parse(buf, len, &index);
	if (ret)
		return ret;
	guard(mutex)(&ctlv_test_child_devices_lock);
	ctlv_test_child_device_slot_remove(index);
	return len;
}

static const struct file_operations ctlv_test_child_device_remove_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = ctlv_test_child_device_remove_write,
	.llseek = noop_llseek,
};

static int ctlv_test_child_devices_init(void)
{
	/* No hardware to be the child of, so a parent is invented. */
	ctlv_test_child_parent = root_device_register("ctlv_test_child");
	return PTR_ERR_OR_ZERO(ctlv_test_child_parent);
}

static void ctlv_test_child_devices_fini(void)
{
	unsigned int i;

	scoped_guard(mutex, &ctlv_test_child_devices_lock)
		for (i = 0; i < ARRAY_SIZE(ctlv_test_child_devices); i++)
			ctlv_test_child_device_slot_remove(i);
	root_device_unregister(ctlv_test_child_parent);
}

/* Reading a log says what it recorded, and writing anything forgets it. */
static ssize_t ctlv_test_child_log_read(struct file *file, char __user *buf,
					size_t len, loff_t *pos)
{
	struct ctlv_test_child_log *log = file->private_data;
	size_t at;

	/* A whole log is more than a 32-bit build wants on the stack. */
	char *text __free(kfree) =
		kmalloc(CTLV_TEST_CHILD_LOG_LEN + 2, GFP_KERNEL);

	if (!text)
		return -ENOMEM;
	scoped_guard(spinlock, &log->lock)
		at = scnprintf(text, CTLV_TEST_CHILD_LOG_LEN + 2, "%s\n",
			       log->text);
	return simple_read_from_buffer(buf, len, pos, text, at);
}

static ssize_t ctlv_test_child_log_write(struct file *file,
					 const char __user *buf, size_t len,
					 loff_t *pos)
{
	struct ctlv_test_child_log *log = file->private_data;

	guard(spinlock)(&log->lock);
	log->text[0] = '\0';
	log->len = 0;
	return len;
}

static const struct file_operations ctlv_test_child_log_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ctlv_test_child_log_read,
	.write = ctlv_test_child_log_write,
	.llseek = default_llseek,
};

static struct dentry *ctlv_test_child_debugfs;

static void ctlv_test_child_debugfs_add(void)
{
	struct dentry *dir = debugfs_create_dir("ctlv-test-child", NULL);

	ctlv_test_child_debugfs = dir;
	debugfs_create_file("device_add", 0200, dir, NULL,
			    &ctlv_test_child_device_add_fops);
	debugfs_create_file("device_remove", 0200, dir, NULL,
			    &ctlv_test_child_device_remove_fops);
	debugfs_create_file("hooks", 0600, dir, &ctlv_test_child_hooks_log,
			    &ctlv_test_child_log_fops);
	debugfs_create_file("echo", 0600, dir, &ctlv_test_child_echo_log,
			    &ctlv_test_child_log_fops);
	debugfs_create_bool("fail_family_pre", 0600, dir,
			    &ctlv_test_child_fail_family_pre);
}

static int __init ctlv_test_child_module_init(void)
{
	int ret;

	ret = ctlv_test_child_devices_init();
	if (ret)
		return ret;
	ctlv_test_child_debugfs_add();
	return 0;
}

static void __exit ctlv_test_child_module_exit(void)
{
	ctlv_test_transfers_drain_begin();
	debugfs_remove_recursive(ctlv_test_child_debugfs);
	ctlv_test_child_devices_fini();
	ctlv_test_transfers_drain_end();
}

module_init(ctlv_test_child_module_init);
module_exit(ctlv_test_child_module_exit);
MODULE_DESCRIPTION("CTLV test device of an inheriting family");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("CTLV");
