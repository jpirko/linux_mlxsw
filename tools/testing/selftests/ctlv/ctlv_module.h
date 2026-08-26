/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Debugfs control helpers for the test families.
 */
#ifndef CTLV_MODULE_H
#define CTLV_MODULE_H

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/types.h>

#define CTLV_TEST_MODULE "/sys/module/ctlv_test"
#define CTLV_TEST_CHILD_MODULE "/sys/module/ctlv_test_child"

/*
 * A directory per module, separating the device family's family-wide hooks
 * from the declaring family's callback and op hooks.
 */
#define CTLV_DEBUGFS "/sys/kernel/debug"
#define CTLV_TEST_DEBUGFS CTLV_DEBUGFS "/ctlv-test"
#define CTLV_TEST_DEVICE_ADD CTLV_TEST_DEBUGFS "/device_add"
#define CTLV_TEST_DEVICE_REMOVE CTLV_TEST_DEBUGFS "/device_remove"
#define CTLV_TEST_HOOKS CTLV_TEST_DEBUGFS "/hooks"
#define CTLV_TEST_ECHO CTLV_TEST_DEBUGFS "/echo"
#define CTLV_TEST_FAIL_FAMILY_PRE CTLV_TEST_DEBUGFS "/fail_family_pre"
#define CTLV_TEST_FAIL_OP_PRE CTLV_TEST_DEBUGFS "/fail_op_pre"
#define CTLV_TEST_GENERATION CTLV_TEST_DEBUGFS "/series_generation"
#define CTLV_TEST_TRANSFER_HOLD CTLV_TEST_DEBUGFS "/transfer_hold"

#define CTLV_TEST_CHILD_DEBUGFS CTLV_DEBUGFS "/ctlv-test-child"
#define CTLV_TEST_CHILD_DEVICE_ADD CTLV_TEST_CHILD_DEBUGFS "/device_add"
#define CTLV_TEST_CHILD_DEVICE_REMOVE CTLV_TEST_CHILD_DEBUGFS "/device_remove"
#define CTLV_TEST_CHILD_HOOKS CTLV_TEST_CHILD_DEBUGFS "/hooks"
#define CTLV_TEST_CHILD_ECHO CTLV_TEST_CHILD_DEBUGFS "/echo"
#define CTLV_TEST_CHILD_FAIL_FAMILY_PRE \
	CTLV_TEST_CHILD_DEBUGFS "/fail_family_pre"

/* Enough for one file holding one number. */
#define CTLV_PARAM_LEN 64

#define CTLV_PATH_LEN 1024

/* Slot 7 is left to the standalone ctlvctl test. */
#define CTLV_INSTANCES_MAX 7

/* Whatever is in path, without its trailing newlines. Returns 0 or -errno. */
static int ctlv_sysfs_read(const char *path, char *buf, size_t len)
{
	ssize_t got;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -errno;
	got = read(fd, buf, len - 1);
	close(fd);
	if (got < 0)
		return -errno;
	while (got && buf[got - 1] == '\n')
		got--;
	buf[got] = '\0';
	return 0;
}

static int ctlv_sysfs_write(const char *path, const char *value)
{
	ssize_t wrote;
	int fd;

	fd = open(path, O_WRONLY);
	if (fd < 0)
		return -errno;
	wrote = write(fd, value, strlen(value));
	close(fd);
	return wrote < 0 ? -errno : 0;
}

static int ctlv_instance_add(const char *control, unsigned int which,
			     bool narrowed)
{
	char value[32];

	snprintf(value, sizeof(value), "%u %u", which, narrowed);
	return ctlv_sysfs_write(control, value);
}

static int ctlv_instance_remove(const char *control, unsigned int which)
{
	char value[16];

	snprintf(value, sizeof(value), "%u", which);
	return ctlv_sysfs_write(control, value);
}

static int ctlv_instances_family_set(const char *add, const char *remove,
				     unsigned int count, bool narrowed)
{
	unsigned int which;
	int ret;

	if (count > CTLV_INSTANCES_MAX)
		return -EINVAL;
	for (which = count; which < CTLV_INSTANCES_MAX; which++) {
		ret = ctlv_instance_remove(remove, which);
		if (ret)
			return ret;
	}
	for (which = 0; which < count; which++) {
		ret = ctlv_instance_add(add, which, narrowed);
		if (ret)
			return ret;
	}
	return 0;
}

static int ctlv_instances(unsigned int count, bool narrowed)
{
	int ret;

	ret = ctlv_instances_family_set(CTLV_TEST_DEVICE_ADD,
					CTLV_TEST_DEVICE_REMOVE, count, narrowed);
	if (ret)
		return ret;
	return ctlv_instances_family_set(CTLV_TEST_CHILD_DEVICE_ADD,
					 CTLV_TEST_CHILD_DEVICE_REMOVE,
					 count, narrowed);
}

/* However many devices of every test family exist, which may be none. */
static int ctlv_instances_set(unsigned int count)
{
	return ctlv_instances(count, false);
}

/*
 * The same, with every device this has to create withholding what it would
 * otherwise publish. The ones already there are left as they were made.
 */
static int ctlv_instances_set_narrowed(unsigned int count)
{
	return ctlv_instances(count, true);
}

static bool ctlv_instances_available(void)
{
	return !access(CTLV_TEST_DEVICE_ADD, W_OK) &&
	       !access(CTLV_TEST_DEVICE_REMOVE, W_OK) &&
	       !access(CTLV_TEST_CHILD_DEVICE_ADD, W_OK) &&
	       !access(CTLV_TEST_CHILD_DEVICE_REMOVE, W_OK);
}

/* Whether the inheriting family's module is there to have recorded anything. */
static bool ctlv_child_logs_available(void)
{
	return !access(CTLV_TEST_CHILD_HOOKS, W_OK);
}

/* Clear both per-family logs before each operation. */
static int ctlv_hooks_clear(void)
{
	int ret = ctlv_sysfs_write(CTLV_TEST_HOOKS, "clear");

	if (ret || !ctlv_child_logs_available())
		return ret;
	return ctlv_sysfs_write(CTLV_TEST_CHILD_HOOKS, "clear");
}

static int ctlv_hooks_read(char *buf, size_t len)
{
	return ctlv_sysfs_read(CTLV_TEST_HOOKS, buf, len);
}

static int ctlv_child_hooks_read(char *buf, size_t len)
{
	return ctlv_sysfs_read(CTLV_TEST_CHILD_HOOKS, buf, len);
}

static int ctlv_echo_clear(void)
{
	int ret = ctlv_sysfs_write(CTLV_TEST_ECHO, "clear");

	if (ret || !ctlv_child_logs_available())
		return ret;
	return ctlv_sysfs_write(CTLV_TEST_CHILD_ECHO, "clear");
}

static int ctlv_echo_read(char *buf, size_t len)
{
	return ctlv_sysfs_read(CTLV_TEST_ECHO, buf, len);
}

static int ctlv_child_echo_read(char *buf, size_t len)
{
	return ctlv_sysfs_read(CTLV_TEST_CHILD_ECHO, buf, len);
}

/* Moving this on stands in for a query's data changing under a reader. */
static __u64 ctlv_generation_read(void)
{
	char buf[CTLV_PARAM_LEN];

	if (ctlv_sysfs_read(CTLV_TEST_GENERATION, buf, sizeof(buf)))
		return 0;
	return strtoull(buf, NULL, 10);
}

static int ctlv_generation_bump(void)
{
	return ctlv_sysfs_write(CTLV_TEST_GENERATION, "1");
}

/* Transfers waiting with what they resolved still held, and letting them go. */
static unsigned int ctlv_hold_count(void)
{
	char buf[CTLV_PARAM_LEN];

	if (ctlv_sysfs_read(CTLV_TEST_TRANSFER_HOLD, buf, sizeof(buf)))
		return 0;
	return strtoul(buf, NULL, 10);
}

static int ctlv_hold_release(void)
{
	return ctlv_sysfs_write(CTLV_TEST_TRANSFER_HOLD, "1");
}

/* Failure is not decided here: a test that needs a device skips. */
static void ctlv_module_load(const char *name, const char *path)
{
	char command[64];

	if (!access(path, F_OK))
		return;
	snprintf(command, sizeof(command), "modprobe %s 2>/dev/null", name);
	if (system(command) < 0)
		perror(name);
}

/* A test image may not mount debugfs, nothing else on it wanting to. */
static void ctlv_mount(const char *type, const char *at, const char *proof)
{
	char command[128];

	if (!access(proof, F_OK))
		return;
	snprintf(command, sizeof(command), "mount -t %s none %s 2>/dev/null",
		 type, at);
	if (system(command) < 0)
		perror(type);
}

/* The child depends on the parent; fixtures create both devices. */
static void ctlv_test_module_load(void)
{
	ctlv_module_load("ctlv-test", CTLV_TEST_MODULE);
	ctlv_module_load("ctlv-test-child", CTLV_TEST_CHILD_MODULE);
	ctlv_mount("debugfs", CTLV_DEBUGFS, CTLV_TEST_DEBUGFS);
	ctlv_instances_set(1);
}

#endif /* CTLV_MODULE_H */
