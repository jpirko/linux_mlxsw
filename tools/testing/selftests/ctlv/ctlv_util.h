/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Raw-ioctl helpers selecting devices by family name.
 */
#ifndef CTLV_UTIL_H
#define CTLV_UTIL_H

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctlv/ctlv.h>
#include <linux/netlink.h>

#include "ctlv_module.h"

/* A device is in its subsystem's class, so every char device is looked at. */
#define CTLV_CHAR_DIR "/sys/dev/char"

#define CTLV_NAME_LEN 256

/* As much as one uevent environment can hold. */
#define CTLV_UEVENT_LEN 2048

/* Long enough for a device to be registered on a loaded machine. */
#define CTLV_MONITOR_WAIT_MS 5000

/* Long enough for one echo report and for the longest self-description. */
#define CTLV_LOG_LEN 8192

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))
#endif

/* A userspace pointer as the ioctl carries it. */
#define ctlv_ptr(_p) ((__u64)(uintptr_t)(_p))

/*
 * The kernel's own assertions, made again from this side of the boundary: a
 * 32-bit build of any test here fails to compile rather than talking past a
 * 64-bit kernel.
 */
static_assert(sizeof(struct ctlv_hdr) == 16, "header moved");
static_assert(__alignof__(struct ctlv_hdr) == CTLV_ALIGNTO, "header alignment");

static_assert(sizeof(struct ctlv_op_exec) == 56, "exec grew");
static_assert(__alignof__(struct ctlv_op_exec) == CTLV_ALIGNTO,
	      "exec alignment");

static_assert(sizeof(struct ctlv_event_hdr) == 24, "event header grew");
static_assert(__alignof__(struct ctlv_event_hdr) == CTLV_ALIGNTO,
	      "event header alignment");

/* What uevent, a uevent file's text, gives property. Returns 0 or -ENOENT. */
static int ctlv_uevent_value(const char *uevent, const char *property,
			     char *buf, size_t len)
{
	size_t prefix = strlen(property);
	const char *line = uevent;

	while (strncmp(line, property, prefix) || line[prefix] != '=') {
		line = strchr(line, '\n');
		if (!line)
			return -ENOENT;
		line++;
	}
	line += prefix + 1;
	snprintf(buf, len, "%.*s", (int)strcspn(line, "\n"), line);
	return 0;
}

/*
 * The CTLV_CHAR_DIR entry of the which'th device whose uevent gives property
 * value, in directory order, read out of the device because a test image has
 * no udev. Returns 0 or -errno, with -ENODEV when there is no such device.
 */
static int ctlv_char_entry(const char *property, const char *value,
			   unsigned int which, char *entry, size_t len)
{
	char found[CTLV_NAME_LEN] = "";
	char uevent[CTLV_UEVENT_LEN];
	char path[CTLV_PATH_LEN];
	unsigned int matches = 0;
	struct dirent **entries;
	char buf[CTLV_NAME_LEN];
	int n, i;

	n = scandir(CTLV_CHAR_DIR, &entries, NULL, alphasort);
	if (n < 0)
		return -errno;
	for (i = 0; i < n; i++) {
		const char *name = entries[i]->d_name;

		if (name[0] == '.' || *found)
			continue;
		snprintf(path, sizeof(path), "%s/%s/uevent", CTLV_CHAR_DIR,
			 name);
		if (ctlv_sysfs_read(path, uevent, sizeof(uevent)) ||
		    ctlv_uevent_value(uevent, property, buf, sizeof(buf)) ||
		    strcmp(buf, value) || matches++ != which)
			continue;
		snprintf(found, sizeof(found), "%s", name);
	}
	for (i = 0; i < n; i++)
		free(entries[i]);
	free(entries);
	if (!*found)
		return -ENODEV;
	snprintf(entry, len, "%s", found);
	return 0;
}

/* Where file of the device named device under /dev is in sysfs. */
static int ctlv_device_sysfs_path(const char *device, const char *file,
				  char *path, size_t len)
{
	char entry[CTLV_NAME_LEN];
	int ret;

	ret = ctlv_char_entry("DEVNAME", device, 0, entry, sizeof(entry));
	if (ret)
		return ret;
	snprintf(path, len, "%s/%s/%s", CTLV_CHAR_DIR, entry, file);
	return 0;
}

static int ctlv_device_sysfs_read(const char *device, const char *file,
				  char *buf, size_t len)
{
	char path[CTLV_PATH_LEN];
	int ret;

	ret = ctlv_device_sysfs_path(device, file, path, sizeof(path));
	if (ret)
		return ret;
	return ctlv_sysfs_read(path, buf, len);
}

/*
 * What a device announces under property. Returns 0 or -errno, -ENOENT for no
 * such property.
 */
static int ctlv_uevent_read(const char *device, const char *property,
			    char *buf, size_t len)
{
	char uevent[CTLV_UEVENT_LEN];
	int ret;

	ret = ctlv_device_sysfs_read(device, "uevent", uevent, sizeof(uevent));
	if (ret)
		return ret;
	return ctlv_uevent_value(uevent, property, buf, len);
}

/*
 * Watching for devices as they appear. Bound before enumerating, so a device
 * registered mid-scan is not missed by both halves of discovery.
 */
struct ctlv_monitor {
	int fd;
	/* One uevent, as the NUL-separated KEY=value strings it arrives as. */
	char buf[CTLV_UEVENT_LEN];
	size_t len;
};

static int ctlv_monitor_open(struct ctlv_monitor *mon)
{
	struct sockaddr_nl addr = {
		.nl_family = AF_NETLINK,
		/* What the kernel broadcasts, not what udev repeats. */
		.nl_groups = 1,
	};
	int size = 1 << 20;
	int ret;

	memset(mon, 0, sizeof(*mon));
	mon->fd = socket(AF_NETLINK, SOCK_DGRAM | SOCK_CLOEXEC,
			 NETLINK_KOBJECT_UEVENT);
	if (mon->fd < 0)
		return -errno;
	/* Enough that a burst of additions cannot be dropped under us. */
	setsockopt(mon->fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (bind(mon->fd, (struct sockaddr *)&addr, sizeof(addr))) {
		ret = -errno;
		close(mon->fd);
		mon->fd = -1;
		return ret;
	}
	return 0;
}

static void ctlv_monitor_close(struct ctlv_monitor *mon)
{
	if (mon->fd >= 0)
		close(mon->fd);
	mon->fd = -1;
}

/* What the held event announces under property. Returns 0 or -errno. */
static int ctlv_monitor_read(const struct ctlv_monitor *mon,
			     const char *property, char *buf, size_t len)
{
	size_t prefix = strlen(property);
	size_t at = 0;

	while (at < mon->len) {
		const char *entry = mon->buf + at;

		at += strlen(entry) + 1;
		if (strncmp(entry, property, prefix) || entry[prefix] != '=')
			continue;
		snprintf(buf, len, "%s", entry + prefix + 1);
		return 0;
	}
	return -ENOENT;
}

/* The next CTLV event of action, held for the reader. Returns 0 or -errno. */
static int ctlv_monitor_wait(struct ctlv_monitor *mon, const char *action)
{
	struct pollfd pfd = { .fd = mon->fd, .events = POLLIN };
	char value[CTLV_NAME_LEN];

	for (;;) {
		struct sockaddr_nl from;
		socklen_t from_len = sizeof(from);
		ssize_t got;
		int ready;

		ready = poll(&pfd, 1, CTLV_MONITOR_WAIT_MS);
		if (ready < 0)
			return -errno;
		if (!ready)
			return -ETIMEDOUT;
		got = recvfrom(mon->fd, mon->buf, sizeof(mon->buf) - 1, 0,
			       (struct sockaddr *)&from, &from_len);
		if (got < 0)
			return -errno;
		/* Only what the kernel itself sent, and only whole. */
		if (from.nl_pid)
			continue;
		mon->len = got;
		mon->buf[got] = '\0';
		if (ctlv_monitor_read(mon, "CTLV_FAMILY", value, sizeof(value)))
			continue;
		if (ctlv_monitor_read(mon, "ACTION", value, sizeof(value)) ||
		    strcmp(value, action))
			continue;
		return 0;
	}
}

/*
 * The name under /dev of the which'th device answering family, in
 * CTLV_CHAR_DIR order. Returns 0 or -errno, with -ENODEV when there is no such
 * device.
 */
static int ctlv_device_name(const char *family, unsigned int which, char *name,
			    size_t len)
{
	char uevent[CTLV_UEVENT_LEN];
	char path[CTLV_PATH_LEN];
	char entry[CTLV_NAME_LEN];
	int ret;

	ret = ctlv_char_entry("CTLV_FAMILY", family, which, entry,
			      sizeof(entry));
	if (ret)
		return ret;
	snprintf(path, sizeof(path), "%s/%s/uevent", CTLV_CHAR_DIR, entry);
	ret = ctlv_sysfs_read(path, uevent, sizeof(uevent));
	if (ret)
		return ret;
	return ctlv_uevent_value(uevent, "DEVNAME", name, len);
}

/* The which'th device of family, opened with flags. Returns an fd or -errno. */
static int ctlv_open(const char *family, unsigned int which, int flags)
{
	char name[CTLV_NAME_LEN];
	char path[CTLV_PATH_LEN];
	int fd, ret;

	ret = ctlv_device_name(family, which, name, sizeof(name));
	if (ret)
		return ret;
	snprintf(path, sizeof(path), "/dev/%s", name);
	fd = open(path, flags);
	return fd < 0 ? -errno : fd;
}

/* The ioctl result as 0 or -errno, rather than as -1 and errno. */
static int ctlv_exec(int fd, struct ctlv_op_exec *exec)
{
	if (ioctl(fd, CTLV_IOC_OP_EXEC, exec))
		return -errno;
	return 0;
}

static const struct ctlv_hdr ctlv_empty_request = {
	.len = sizeof(struct ctlv_hdr),
	.flags = CTLV_HDR_F_RECORD,
	.type = CTLV_TYPE_ROOT,
};

static void ctlv_exec_set_empty_request(struct ctlv_op_exec *exec)
{
	exec->request = (__u64)(uintptr_t)&ctlv_empty_request;
	exec->request_len = sizeof(ctlv_empty_request);
}

/* One op with an empty request and no reply buffer. */
static int ctlv_exec_bare(int fd, __u64 op_id)
{
	struct ctlv_op_exec exec = { .size = sizeof(exec), .op_id = op_id };

	ctlv_exec_set_empty_request(&exec);
	return ctlv_exec(fd, &exec);
}

static int ctlv_exec_reply(int fd, __u64 op_id, void *reply, __u32 reply_len)
{
	struct ctlv_op_exec exec = {
		.size = sizeof(exec),
		.op_id = op_id,
		.reply = (__u64)(uintptr_t)reply,
		.reply_len = reply_len,
	};

	ctlv_exec_set_empty_request(&exec);
	return ctlv_exec(fd, &exec);
}

/* The readable length is the caller's to state, right or wrong. */
static int ctlv_exec_request(int fd, __u64 op_id, const void *request,
			     __u32 request_len)
{
	struct ctlv_op_exec exec = {
		.size = sizeof(exec),
		.op_id = op_id,
		.request = (__u64)(uintptr_t)request,
		.request_len = request_len,
	};

	return ctlv_exec(fd, &exec);
}

static int ctlv_exec_full(int fd, __u64 op_id, const void *request,
			  __u32 request_len, void *reply, __u32 reply_len)
{
	struct ctlv_op_exec exec = {
		.size = sizeof(exec),
		.op_id = op_id,
		.request = (__u64)(uintptr_t)request,
		.request_len = request_len,
		.reply = (__u64)(uintptr_t)reply,
		.reply_len = reply_len,
	};

	return ctlv_exec(fd, &exec);
}

/* A root with a declared length, which need not be its real one. */
static void ctlv_root_init(void *buf, __u32 len)
{
	struct ctlv_hdr *root = buf;

	root->len = len;
	root->flags = CTLV_HDR_F_RECORD;
	root->type = CTLV_TYPE_ROOT;
}

#endif /* CTLV_UTIL_H */
