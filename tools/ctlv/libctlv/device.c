// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Finding a device, opening it, and learning what it is. A caller knows the
 * family it wants to speak, which is what the device announces in its uevent;
 * opening one then asks the device for its chain, so what was opened can be
 * compared against what the caller was compiled for.
 */

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "internal.h"

#define CLASS_DIR "/sys/class/ctlv"
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

/* The value of one uevent property of one device, or -ENOENT. */
static int uevent_read(const char *device, const char *property, char *value,
		       size_t len)
{
	char line[UEVENT_LINE_LEN];
	char path[PATH_LEN];
	size_t prefix;
	FILE *file;
	int ret = -ENOENT;

	snprintf(path, sizeof(path), "%s/%s/uevent", CLASS_DIR, device);
	file = fopen(path, "re");
	if (!file)
		return -errno;
	prefix = strlen(property);
	while (fgets(line, sizeof(line), file)) {
		size_t value_len;
		char *end;

		if (strncmp(line, property, prefix) || line[prefix] != '=')
			continue;
		end = strchr(line, '\n');
		if (end)
			*end = '\0';
		value_len = strlen(line + prefix + 1);
		/* A property this reader cannot hold is not one it matched. */
		ret = value_len < len ? 0 : -ENAMETOOLONG;
		if (!ret)
			memcpy(value, line + prefix + 1, value_len + 1);
		break;
	}
	fclose(file);
	return ret;
}

static int name_cmp(const void *a, const void *b)
{
	return strcmp(*(const char *const *)a, *(const char *const *)b);
}

int ctlv_enumerate(struct ctlv_dev_list *list)
{
	struct dirent *entry;
	char **name = NULL;
	unsigned int n = 0;
	DIR *dir;
	int ret = 0;

	memset(list, 0, sizeof(*list));
	dir = opendir(CLASS_DIR);
	if (!dir)
		return -errno;
	while ((entry = readdir(dir))) {
		char **grown;

		if (entry->d_name[0] == '.')
			continue;
		grown = realloc(name, (n + 1) * sizeof(*name));
		if (!grown) {
			ret = -ENOMEM;
			break;
		}
		name = grown;
		ret = name_dup(entry->d_name, &name[n]);
		if (ret)
			break;
		n++;
		ret = 0;
	}
	closedir(dir);
	if (ret) {
		list->n = n;
		list->name = name;
		ctlv_dev_list_free(list);
		return ret;
	}
	/* Ordered, so that a tool listing devices lists them the same twice. */
	if (name)
		qsort(name, n, sizeof(*name), name_cmp);
	list->n = n;
	list->name = name;
	return 0;
}

void ctlv_dev_list_free(struct ctlv_dev_list *list)
{
	unsigned int i;

	for (i = 0; i < list->n; i++)
		free(list->name[i]);
	free(list->name);
	memset(list, 0, sizeof(*list));
}

/* The chain the device reports, root family first and the driver's last. */
int ctlv_chain_load(struct ctlv_dev *dev)
{
	const struct ctlv_attr *root, *entry;
	struct ctlv_reply *reply = NULL;
	unsigned int n = 0;
	int ret;

	ret = ctlv_query_sized(dev, CTLV_OP_FAMILIES_DUMP, NULL, 0, &reply,
			       NULL);
	if (ret)
		return ret;

	root = ctlv_reply_root(reply);
	n = ctlv_attr_count(root, CTLV_ATTR_FAMILY_ENTRY);
	if (!n) {
		ret = -EPROTO;
		goto out;
	}
	dev->chain_id = calloc(n, sizeof(*dev->chain_id));
	dev->chain_name = calloc(n, sizeof(*dev->chain_name));
	if (!dev->chain_id || !dev->chain_name) {
		ret = -ENOMEM;
		goto out;
	}

	ctlv_for_each_id(entry, root, CTLV_ATTR_FAMILY_ENTRY) {
		const struct ctlv_attr *attr;
		const char *name;
		__u64 id;

		attr = ctlv_attr_find(entry, CTLV_ATTR_FAMILY_ID);
		if (!attr || ctlv_get_u64(attr, &id) || id > UINT16_MAX) {
			ret = -EPROTO;
			goto out;
		}
		attr = ctlv_attr_find(entry, CTLV_ATTR_FAMILY_NAME);
		if (!attr || ctlv_get_str(attr, &name)) {
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
	if (dev->fd >= 0)
		close(dev->fd);
	free(dev);
}

static int dev_open(const char *name, const char *path, int flags,
		    struct ctlv_dev **devp)
{
	struct ctlv_dev *dev;
	int ret;

	dev = calloc(1, sizeof(*dev));
	if (!dev)
		return -ENOMEM;
	dev->fd = -1;
	ret = name_dup(name, &dev->name);
	if (ret)
		goto err;

	dev->fd = open(path, flags | O_CLOEXEC);
	if (dev->fd < 0) {
		ret = -errno;
		goto err;
	}
	ret = ctlv_chain_load(dev);
	if (ret)
		goto err;

	*devp = dev;
	return 0;
err:
	dev_free(dev);
	return ret;
}

int ctlv_open_path(const char *path, int flags, struct ctlv_dev **devp)
{
	const char *name = strrchr(path, '/');

	return dev_open(name ? name + 1 : path, path, flags, devp);
}

/*
 * The @which'th device of @family, matched against the family the driver
 * registered rather than any it inherits.
 */
int ctlv_open(const char *family, unsigned int which, int flags,
	      struct ctlv_dev **devp)
{
	char value[NAME_LEN];
	char path[PATH_LEN];
	struct ctlv_dev_list list;
	unsigned int i, matches = 0;
	int ret;

	ret = ctlv_enumerate(&list);
	if (ret)
		return ret;

	ret = -ENODEV;
	for (i = 0; i < list.n; i++) {
		if (uevent_read(list.name[i], "CTLV_FAMILY", value,
				sizeof(value)))
			continue;
		if (strcmp(value, family) || matches++ != which)
			continue;
		snprintf(path, sizeof(path), "%s/%s", DEV_DIR, list.name[i]);
		ret = dev_open(list.name[i], path, flags, devp);
		break;
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
 * What one op answers with, for the two calls that were not told. The dump is
 * taken once: its lengths follow from the schema and cannot go out of date.
 */
static const struct ctlv_op_info *op_sizes(struct ctlv_dev *dev, __u64 op_id)
{
	if (!dev->sizes && ctlv_ops_dump(dev, &dev->sizes, NULL))
		return NULL;
	return ctlv_ops_find(dev->sizes, op_id);
}

int ctlv_action(struct ctlv_dev *dev, __u64 op_id, struct ctlv_msg *request,
		struct ctlv_reply **replyp, struct ctlv_error *error)
{
	const struct ctlv_op_info *info = op_sizes(dev, op_id);

	return ctlv_action_sized(dev, op_id, request,
				 info ? info->action_reply_max_len : 0, replyp,
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
