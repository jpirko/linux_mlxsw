// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Device discovery, opening, and family-chain verification.
 */

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "internal.h"

/* Whichever subsystem a device is in announces its family, so all are here. */
#define CHAR_DIR "/sys/dev/char"
#define DEV_DIR "/dev"

/* Longest device or family name, and longest path naming one. */
#define NAME_LEN 256
#define PATH_LEN 1024

/* One line of a uevent file, PROPERTY=value. */
#define UEVENT_LINE_LEN 512

static int name_dup(const char *name, char **out)
{
	char *copy = strdup(name);

	if (!copy)
		return -ENOMEM;
	*out = copy;
	return 0;
}

/* The value @line gives @property, its newline dropped, or NULL. */
static const char *uevent_value(char *line, const char *property)
{
	size_t prefix = strlen(property);
	char *end;

	if (strncmp(line, property, prefix) || line[prefix] != '=')
		return NULL;
	end = strchr(line, '\n');
	if (end)
		*end = '\0';
	return line + prefix + 1;
}

/*
 * The family of one char device and its name under /dev, NAME_LEN each, or
 * -ENOENT for a device that is not a CTLV one.
 */
static int uevent_read(const char *devnum, char *family, char *devname)
{
	char line[UEVENT_LINE_LEN];
	char path[PATH_LEN];
	FILE *file;

	family[0] = '\0';
	devname[0] = '\0';
	snprintf(path, sizeof(path), "%s/%s/uevent", CHAR_DIR, devnum);
	file = fopen(path, "re");
	if (!file)
		return -errno;
	while (fgets(line, sizeof(line), file)) {
		const char *value = uevent_value(line, "CTLV_FAMILY");
		char *out = family;
		size_t len;

		if (!value) {
			value = uevent_value(line, "DEVNAME");
			out = devname;
		}
		if (!value)
			continue;
		len = strlen(value);
		/* A property this reader cannot hold is not one it matched. */
		if (len < NAME_LEN)
			memcpy(out, value, len + 1);
	}
	fclose(file);
	return *family && *devname ? 0 : -ENOENT;
}

static int name_cmp(const void *a, const void *b)
{
	return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* Every CTLV device, or only those registered as @family. */
static int dev_scan(struct ctlv_dev_list *list, const char *family)
{
	char found[NAME_LEN], devname[NAME_LEN];
	struct dirent *entry;
	char **name = NULL;
	unsigned int n = 0;
	DIR *dir;
	int ret = 0;

	memset(list, 0, sizeof(*list));
	dir = opendir(CHAR_DIR);
	if (!dir)
		return -errno;
	while ((entry = readdir(dir))) {
		char **grown;

		if (entry->d_name[0] == '.' ||
		    uevent_read(entry->d_name, found, devname) ||
		    (family && strcmp(found, family)))
			continue;
		grown = realloc(name, (n + 1) * sizeof(*name));
		if (!grown) {
			ret = -ENOMEM;
			break;
		}
		name = grown;
		ret = name_dup(devname, &name[n]);
		if (ret)
			break;
		n++;
	}
	closedir(dir);
	list->names_count = n;
	list->names = name;
	if (ret) {
		ctlv_dev_list_free(list);
		return ret;
	}
	/* Ordered, so that a tool listing devices lists them the same twice. */
	if (name)
		qsort(name, n, sizeof(*name), name_cmp);
	return 0;
}

int ctlv_enumerate(struct ctlv_dev_list *list)
{
	return dev_scan(list, NULL);
}

void ctlv_dev_list_free(struct ctlv_dev_list *list)
{
	unsigned int i;

	for (i = 0; i < list->names_count; i++)
		free(list->names[i]);
	free(list->names);
	memset(list, 0, sizeof(*list));
}

/* The chain the device reports, root family first and device family last. */
int ctlv_chain_load(struct ctlv_dev *dev)
{
	const struct ctlv_hdr *root, *entry;
	struct ctlv_reply *reply = NULL;
	unsigned int n = 0;
	int ret;

	ret = ctlv_query_sized(dev, CTLV_OP_FAMILY_GET, NULL, 0, &reply,
			       NULL);
	if (ret)
		return ret;

	root = ctlv_reply_root(reply);
	ret = ctlv_bootstrap_validate(root, reply->msg->len,
				      CTLV_OP_FAMILY_GET);
	if (ret)
		goto out;
	n = ctlv_hdr_count(root, CTLV_T_FAMILY_GET_FAMILIES);
	dev->chain_id = calloc(n, sizeof(*dev->chain_id));
	dev->chain_name = calloc(n, sizeof(*dev->chain_name));
	if (!dev->chain_id || !dev->chain_name) {
		ret = -ENOMEM;
		goto out;
	}

	ctlv_for_each_type(entry, root, CTLV_T_FAMILY_GET_FAMILIES) {
		const struct ctlv_hdr *hdr;
		const char *name;
		__u64 id;

		hdr = ctlv_hdr_find(entry, CTLV_T_FAMILY_GET_FAMILIES_ID);
		if (!hdr || ctlv_get_u64(hdr, &id) || id > UINT16_MAX) {
			ret = -EPROTO;
			goto out;
		}
		hdr = ctlv_hdr_find(entry, CTLV_T_FAMILY_GET_FAMILIES_NAME);
		if (!hdr || ctlv_get_str(hdr, &name)) {
			ret = -EPROTO;
			goto out;
		}
		dev->chain_id[dev->chain_len] = id;
		ret = name_dup(name, &dev->chain_name[dev->chain_len]);
		if (ret)
			goto out;
		dev->chain_len++;
	}
	ret = 0;
out:
	ctlv_reply_free(reply);
	return ret;
}

static void dev_free(struct ctlv_dev *dev)
{
	unsigned int i;

	for (i = 0; i < dev->chain_len; i++)
		free(dev->chain_name[i]);
	free(dev->chain_name);
	free(dev->chain_id);
	free(dev->name);
	ctlv_ops_free(dev->sizes);
	if (dev->fd >= 0 && dev->owns_fd)
		close(dev->fd);
	free(dev);
}

/* @fd's device, whose family chain it is asked for over that same fd. */
static int dev_attach(const char *name, int fd, bool owns_fd,
		      struct ctlv_dev **devp)
{
	struct ctlv_dev *dev;
	int ret;

	dev = calloc(1, sizeof(*dev));
	if (!dev) {
		if (owns_fd)
			close(fd);
		return -ENOMEM;
	}
	dev->fd = fd;
	dev->owns_fd = owns_fd;
	ret = name_dup(name, &dev->name);
	if (!ret)
		ret = ctlv_chain_load(dev);
	if (ret) {
		dev_free(dev);
		return ret;
	}
	*devp = dev;
	return 0;
}

static int dev_open(const char *name, const char *path, int flags,
		    struct ctlv_dev **devp)
{
	int fd = open(path, flags | O_CLOEXEC);

	if (fd < 0)
		return -errno;
	return dev_attach(name, fd, true, devp);
}

int ctlv_open_path(const char *path, int flags, struct ctlv_dev **devp)
{
	const char *name = strrchr(path, '/');

	return dev_open(name ? name + 1 : path, path, flags, devp);
}

int ctlv_open_fd(int fd, struct ctlv_dev **devp)
{
	char link[PATH_LEN], target[PATH_LEN];
	const char *name = target;
	ssize_t len;

	if (fd < 0)
		return -EBADF;
	/* Named for messages only, after the file if /proc can say which. */
	snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
	len = readlink(link, target, sizeof(target) - 1);
	if (len < 0) {
		snprintf(target, sizeof(target), "fd%d", fd);
	} else {
		target[len] = '\0';
		if (strrchr(target, '/'))
			name = strrchr(target, '/') + 1;
	}
	return dev_attach(name, fd, false, devp);
}

/*
 * The @which'th device of @family, matched against its registered family
 * rather than any it inherits.
 */
int ctlv_open(const char *family, unsigned int which, int flags,
	      struct ctlv_dev **devp)
{
	char path[PATH_LEN];
	struct ctlv_dev_list list;
	int ret;

	ret = dev_scan(&list, family);
	if (ret)
		return ret;
	ret = -ENODEV;
	if (which < list.names_count) {
		snprintf(path, sizeof(path), "%s/%s", DEV_DIR,
			 list.names[which]);
		ret = dev_open(list.names[which], path, flags, devp);
	}
	ctlv_dev_list_free(&list);
	return ret;
}

void ctlv_close(struct ctlv_dev *dev)
{
	if (dev)
		dev_free(dev);
}

int ctlv_dev_fd(const struct ctlv_dev *dev)
{
	return dev->fd;
}

const char *ctlv_dev_name(const struct ctlv_dev *dev)
{
	return dev->name;
}

unsigned int ctlv_chain_len(const struct ctlv_dev *dev)
{
	return dev->chain_len;
}

__u16 ctlv_chain_id(const struct ctlv_dev *dev, unsigned int level)
{
	return level < dev->chain_len ? dev->chain_id[level] : 0;
}

const char *ctlv_chain_name(const struct ctlv_dev *dev, unsigned int level)
{
	return level < dev->chain_len ? dev->chain_name[level] : NULL;
}

const char *ctlv_dev_family(const struct ctlv_dev *dev)
{
	return ctlv_chain_name(dev, dev->chain_len - 1);
}

bool ctlv_dev_inherits(const struct ctlv_dev *dev, const char *family)
{
	unsigned int i;

	for (i = 0; i < dev->chain_len; i++)
		if (!strcmp(dev->chain_name[i], family))
			return true;
	return false;
}

/*
 * The dump is taken once: its lengths follow from the schema and cannot go out
 * of date.
 */
const struct ctlv_ops *ctlv_dev_sizes(struct ctlv_dev *dev)
{
	if (!dev->sizes && ctlv_ops_get(dev, &dev->sizes, NULL))
		return NULL;
	return dev->sizes;
}

/* What one op answers with, for the two calls that were not told. */
static const struct ctlv_op_info *op_sizes(struct ctlv_dev *dev, __u64 op_id)
{
	const struct ctlv_ops *sizes = ctlv_dev_sizes(dev);

	return sizes ? ctlv_ops_find(sizes, op_id) : NULL;
}

/*
 * Room for a refusal's error information as well: an action cannot be retried
 * into a larger buffer the way a query can, it has happened.
 */
int ctlv_action(struct ctlv_dev *dev, __u64 op_id, struct ctlv_msg *request,
		struct ctlv_reply **replyp, struct ctlv_error *error)
{
	const struct ctlv_op_info *info = op_sizes(dev, op_id);
	__u64 reply_len = 0;

	if (info) {
		reply_len = info->action_reply_max_len;
		if (reply_len < info->error_info_max_len)
			reply_len = info->error_info_max_len;
	}
	return ctlv_action_sized(dev, op_id, request, reply_len, replyp,
				 error);
}

/*
 * A dump that went stale is started again, the fragments of two answers not
 * being an answer. A caller wanting the -ESTALE instead asks for a size.
 */
int ctlv_query(struct ctlv_dev *dev, __u64 op_id, struct ctlv_msg *request,
	       struct ctlv_reply **replyp, struct ctlv_error *error)
{
	const struct ctlv_op_info *info = op_sizes(dev, op_id);
	__u32 buf_len = 0;
	unsigned int try;
	int ret;

	if (info)
		buf_len = info->query_reply_suggested_len;
	for (try = 0; try < QUERY_RESTARTS; try++) {
		ret = ctlv_query_sized(dev, op_id, request, buf_len, replyp,
				       error);
		if (ret != -ESTALE)
			return ret;
	}
	return ret;
}
