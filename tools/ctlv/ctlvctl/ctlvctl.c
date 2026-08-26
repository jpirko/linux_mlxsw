// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Family-independent CTLV command-line client.
 */

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_JSON_C
#include <json.h>
#endif

#include <ctlv.h>

#include "spec.h"

#define DEV_DIR "/dev"
#define PATH_LEN 1024
#define LINE_LEN 512
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

/* How deep a request built from the command line may nest. */
#define PATH_DEPTH (CTLV_MAX_NEST_DEPTH + 1)

/* Where a request buffer starts; it grows as attrs are added. */
#define REQUEST_HINT 512

static const char *prog = "ctlvctl";

static struct {
	const char *spec_dir;
	bool show_ids;
	bool output_json;
} options;

#ifdef HAVE_JSON_C
static int json_template_print(const struct ctlv_node_schema *root,
			       const struct ctlvctl_spec *spec);
static void json_add_id(struct json_object *object, const char *key, __u64 id);
static void json_add_op(struct json_object *object,
			const struct ctlvctl_spec *spec, __u64 op_id);
static int json_print(struct json_object *object);
static int json_schema_print(const struct ctlv_dev *dev,
			     const struct ctlvctl_spec *spec, __u64 op_id,
			     const struct ctlv_schema *schema);

static void json_add_string(struct json_object *object, const char *key,
			    const char *value)
{
	json_object_object_add(object, key, json_object_new_string(value));
}

static void json_add_u64(struct json_object *object, const char *key,
			 __u64 value)
{
	json_object_object_add(object, key, json_object_new_uint64(value));
}

static void json_add_s64(struct json_object *object, const char *key,
			 __s64 value)
{
	json_object_object_add(object, key, json_object_new_int64(value));
}

static void json_add_int(struct json_object *object, const char *key, int value)
{
	json_object_object_add(object, key, json_object_new_int(value));
}

static void json_add_bool(struct json_object *object, const char *key,
			  bool value)
{
	json_object_object_add(object, key, json_object_new_boolean(value));
}

static int json_array_add_string(struct json_object *array, const char *value)
{
	struct json_object *string = json_object_new_string(value);

	if (!string)
		return -ENOMEM;
	if (json_object_array_add(array, string)) {
		json_object_put(string);
		return -ENOMEM;
	}
	return 0;
}
#endif

static void help(FILE *out)
{
	fprintf(out,
		"usage: %s [options] <command> [arguments]\n"
		"\n"
		"  list                       every CTLV device and its family chain\n"
		"  ops <device>               every op the device publishes\n"
		"  schema [--template] <device> <op>\n"
		"                             the request and reply ABI of one op\n"
		"  exec <device> <op> [a=v]   run an action\n"
		"  query [-l len] <device> <op> [a=v]\n"
		"                             run a query, reassembling the answer\n"
		"  monitor [-q len] <device> [op ...]\n"
		"                             subscribe and print records as they arrive\n"
		"\n"
		"  --spec-dir DIR             use DIR/<family>.yaml for names\n"
		"  -n, --show-ids             show IDs beside specification names\n"
		"  --output-json              print structured JSON\n"
		"  --json TEXT|@FILE|-        build a request from JSON\n"
		"\n"
		"-l names the reply buffer, which is what decides how many fragments an\n"
		"answer arrives in; the device reports its smallest usable size in ops.\n"
		"-q resizes the event queue before subscribing, which is how a reader\n"
		"decides how far behind it may fall before records are dropped.\n"
		"\n"
		"With --spec-dir, operation names and top-level scalar names may be\n"
		"used. Otherwise operations and attributes use their complete IDs.\n"
		"A dotted numeric path enters a nest:\n"
		"\n"
		"  %s exec ctlv0 0x1000000002 0x1000100000001=7\n"
		"  %s query ctlv0 0x2 0x1000400000001.0x1000400000002=1\n",
		prog, prog, prog);
}

static int parse_u64(const char *text, __u64 *value)
{
	char *end;

	if (!*text || *text == '-' || isspace((unsigned char)*text))
		return -EINVAL;
	errno = 0;
	*value = strtoull(text, &end, 0);
	if (errno || end == text || *end)
		return -EINVAL;
	return 0;
}

static int parse_s64(const char *text, __s64 *value)
{
	char *end;

	errno = 0;
	*value = strtoll(text, &end, 0);
	if (errno || end == text || *end)
		return -EINVAL;
	return 0;
}

static int dev_open(const char *name, int flags, struct ctlv_dev **devp)
{
	char path[PATH_LEN];
	int ret;

	if (strchr(name, '/'))
		snprintf(path, sizeof(path), "%s", name);
	else
		snprintf(path, sizeof(path), "%s/%s", DEV_DIR, name);
	ret = ctlv_open_path(path, flags, devp);
	if (ret)
		fprintf(stderr, "%s: %s: %s\n", prog, path, strerror(-ret));
	return ret;
}

static int spec_load(const struct ctlv_dev *dev, struct ctlvctl_spec **spec)
{
	if (!options.spec_dir) {
		*spec = NULL;
		return 0;
	}
	return ctlvctl_spec_load(prog, options.spec_dir, dev, spec);
}

static int op_resolve(const struct ctlvctl_spec *spec, const char *text,
		      __u64 *op_id)
{
	const struct ctlvctl_op_name *op;

	if (!parse_u64(text, op_id))
		return 0;
	op = ctlvctl_spec_op_name(spec, text);
	if (op) {
		*op_id = op->id;
		return 0;
	}
	if (spec)
		fprintf(stderr, "%s: no operation \"%s\"\n", prog, text);
	else
		fprintf(stderr, "%s: %s: not an op ID\n", prog, text);
	return -ENOENT;
}

static void chain_print(const struct ctlv_dev *dev)
{
	unsigned int i;

	for (i = 0; i < ctlv_chain_len(dev); i++)
		printf("%s%s", i ? "/" : "", ctlv_chain_name(dev, i));
}

static int cmd_list(int argc, char **argv)
{
#ifdef HAVE_JSON_C
	struct json_object *json = NULL;
#endif
	struct ctlv_dev_list list;
	unsigned int i;
	int ret;

	if (argc) {
		help(stderr);
		return -EINVAL;
	}
	ret = ctlv_enumerate(&list);
	if (ret) {
		fprintf(stderr, "%s: no CTLV devices: %s\n", prog,
			strerror(-ret));
		return ret;
	}
#ifdef HAVE_JSON_C
	if (options.output_json) {
		json = json_object_new_array();
		if (!json) {
			ctlv_dev_list_free(&list);
			return -ENOMEM;
		}
	}
#endif
	for (i = 0; i < list.names_count; i++) {
		struct ctlv_dev *dev;

#ifdef HAVE_JSON_C
		if (json) {
			struct json_object *chain = json_object_new_array();
			struct json_object *item = json_object_new_object();
			unsigned int level;

			json_add_string(item, "device", list.names[i]);
			if (!dev_open(list.names[i], O_RDONLY, &dev)) {
				for (level = 0; level < ctlv_chain_len(dev);
				     level++) {
					struct json_object *family =
						json_object_new_object();

					json_add_string(family, "name",
							ctlv_chain_name(dev,
									level));
					if (options.show_ids)
						json_add_int(family, "id",
							     ctlv_chain_id(dev, level));
					json_object_array_add(chain, family);
				}
				ctlv_close(dev);
			}
			json_object_object_add(item, "chain", chain);
			json_object_array_add(json, item);
			continue;
		}
#endif
		printf("%s", list.names[i]);
		/* Listing should not need the right to change a device. */
		if (!dev_open(list.names[i], O_RDONLY, &dev)) {
			printf("\t");
			chain_print(dev);
			ctlv_close(dev);
		}
		printf("\n");
	}
	ctlv_dev_list_free(&list);
#ifdef HAVE_JSON_C
	if (json)
		return json_print(json);
#endif
	return 0;
}

static const char *type_name(__u32 type)
{
	switch (type) {
	case CTLV_OP_TYPE_ACTION:
		return "action";
	case CTLV_OP_TYPE_QUERY:
		return "query";
	case CTLV_OP_TYPE_EVENT:
		return "event";
	default:
		return "?";
	}
}

static const char *fd_access_name(__u32 fd_access)
{
	return fd_access == CTLV_OP_FD_ACCESS_READ ? "read" : "read-write";
}

static bool op_matches(const struct ctlvctl_op_name *name,
		       const struct ctlv_op_info *op)
{
	return name->type == op->type &&
	       name->fd_access == op->fd_access &&
	       name->availability == op->availability;
}

static int op_print(const struct ctlvctl_spec *spec,
		    const struct ctlv_op_info *op)
{
	const struct ctlvctl_op_name *name;

	name = ctlvctl_spec_op_id(spec, op->op_id);
	if (name && !op_matches(name, op)) {
		fprintf(stderr,
			"%s: specification disagrees with operation %#llx\n",
			prog, (unsigned long long)op->op_id);
		return -ENOMSG;
	}
	if (name) {
		printf("%-20s", name->name);
		if (options.show_ids)
			printf("\t%#018llx",
			       (unsigned long long)op->op_id);
	} else {
		printf("%#018llx", (unsigned long long)op->op_id);
	}
	printf("\t%-6s", type_name(op->type));
	if (op->fd_access)
		printf("\t%s", fd_access_name(op->fd_access));
	if (op->availability == CTLV_OP_AVAILABILITY_DYNAMIC)
		printf("\tdynamic %s", op->disabled ? "disabled" : "enabled");
	else if (op->availability == CTLV_OP_AVAILABILITY_FIXED)
		printf("\tfixed");
	if (op->request_max_len)
		printf("\trequest %llu",
		       (unsigned long long)op->request_max_len);
	if (op->action_reply_max_len)
		printf("\treply %llu",
		       (unsigned long long)op->action_reply_max_len);
	if (op->query_reply_min_len)
		printf("\treply %llu..%llu",
		       (unsigned long long)op->query_reply_min_len,
		       (unsigned long long)op->query_reply_suggested_len);
	if (op->event_record_max_len)
		printf("\trecord %llu",
		       (unsigned long long)op->event_record_max_len);
	printf("\n");
	return 0;
}

static int cmd_ops(int argc, char **argv)
{
#ifdef HAVE_JSON_C
	struct json_object *json = NULL;
	struct json_object *json_ops = NULL;
#endif
	struct ctlvctl_spec *spec = NULL;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	struct ctlv_ops *ops;
	char text[LINE_LEN];
	unsigned int i;
	int ret;

	if (argc != 1) {
		help(stderr);
		return -EINVAL;
	}
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;
	ret = spec_load(dev, &spec);
	if (ret)
		goto out;

	ret = ctlv_ops_dump(dev, &ops, &error);
	if (ret) {
		fprintf(stderr, "%s: ops-dump: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}
#ifdef HAVE_JSON_C
	if (options.output_json) {
		json = json_object_new_object();
		json_ops = json_object_new_array();
		if (!json || !json_ops) {
			ret = -ENOMEM;
			goto out_json;
		}
		json_add_string(json, "device", ctlv_dev_name(dev));
		json_add_u64(json, "generation", ops->generation);
		json_object_object_add(json, "ops", json_ops);
	}
#endif
	if (!options.output_json) {
		printf("# ");
		chain_print(dev);
		printf(", generation %llu\n",
		       (unsigned long long)ops->generation);
	}
	for (i = 0; i < ops->ops_count; i++) {
#ifdef HAVE_JSON_C
		if (json) {
			const struct ctlvctl_op_name *name;
			struct json_object *item;

			name = ctlvctl_spec_op_id(spec, ops->ops[i].op_id);
			if (name && !op_matches(name, &ops->ops[i])) {
				fprintf(stderr,
					"%s: specification disagrees with operation %#llx\n",
					prog, (unsigned long long)
					ops->ops[i].op_id);
				ret = -ENOMSG;
				break;
			}
			item = json_object_new_object();
			if (!item) {
				ret = -ENOMEM;
				break;
			}
			if (name)
				json_add_string(item, "name", name->name);
			if (!name || options.show_ids)
				json_add_id(item, "id", ops->ops[i].op_id);
			json_add_string(item, "type",
					type_name(ops->ops[i].type));
			if (ops->ops[i].fd_access) {
				const char *access =
					fd_access_name(ops->ops[i].fd_access);

				json_add_string(item, "fd-access",
						access);
			}
			if (ops->ops[i].availability) {
				const char *availability;

				availability = ops->ops[i].availability ==
					CTLV_OP_AVAILABILITY_DYNAMIC ?
					"dynamic" : "fixed";
				json_add_string(item, "availability",
						availability);
				if (ops->ops[i].availability ==
				    CTLV_OP_AVAILABILITY_DYNAMIC)
					json_add_bool(item, "disabled",
						      ops->ops[i].disabled);
			}
			json_object_array_add(json_ops, item);
			continue;
		}
#endif
		ret = op_print(spec, &ops->ops[i]);
		if (ret)
			break;
	}
#ifdef HAVE_JSON_C
	if (!ret && json)
		ret = json_print(json);
	else if (json)
		json_object_put(json);
	json = NULL;
	json_ops = NULL;
out_json:
	if (json)
		json_object_put(json);
	else if (json_ops)
		json_object_put(json_ops);
#endif
	ctlv_ops_free(ops);
out:
	ctlvctl_spec_free(spec);
	ctlv_close(dev);
	return ret;
}

static const struct ctlvctl_attr_name *
attr_name(const struct ctlvctl_spec *spec,
	  const struct ctlv_node_schema *node)
{
	const struct ctlvctl_attr_name *name;

	if (!node)
		return NULL;
	name = ctlvctl_spec_attr_id(spec, node->attr_id);
	if (name && name->kind != node->kind)
		return NULL;
	return name;
}

static int tree_names_check(const struct ctlvctl_spec *spec,
			    const struct ctlv_node_schema *node)
{
	const struct ctlvctl_attr_name *name;
	__u64 known = 0;
	unsigned int i;
	int ret;

	if (!node)
		return 0;
	name = ctlvctl_spec_attr_id(spec, node->attr_id);
	if (name && name->kind != node->kind) {
		fprintf(stderr,
			"%s: specification kind for %s disagrees with the device\n",
			prog, name->name);
		return -ENOMSG;
	}
	if (name && name->definition) {
		for (i = 0; i < name->definition->values_count; i++) {
			__u64 value = name->definition->values[i].value;

			known |= value;
			if (name->kind == CTLV_SCHEMA_ENUM &&
			    (!(node->present & CTLV_NODE_SCHEMA_HAS_MIN) ||
			     !(node->present & CTLV_NODE_SCHEMA_HAS_MAX) ||
			     value < node->min || value > node->max)) {
				fprintf(stderr,
					"%s: specification values for %s disagree with the device\n",
					prog, name->name);
				return -ENOMSG;
			}
		}
		if (name->kind == CTLV_SCHEMA_FLAGS &&
		    (!(node->present & CTLV_NODE_SCHEMA_HAS_FLAGS_MASK) ||
		     (known & ~node->flags_mask))) {
			fprintf(stderr,
				"%s: specification flags for %s disagree with the device\n",
				prog, name->name);
			return -ENOMSG;
		}
	}
	for (i = 0; i < node->attrs_count; i++) {
		ret = tree_names_check(spec, &node->attrs[i]);
		if (ret)
			return ret;
	}
	for (i = 0; i < node->optional_attrs_count; i++) {
		ret = tree_names_check(spec, &node->optional_attrs[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static void tree_print(const struct ctlvctl_spec *spec,
		       const struct ctlv_node_schema *node, unsigned int indent,
		       int required)
{
	char line[LINE_LEN];
	const struct ctlvctl_attr_name *name;
	char *detail;
	unsigned int i;

	name = attr_name(spec, node);
	ctlv_schema_render(node, 0, line, sizeof(line));
	if (!name || node->attr_id == CTLV_ATTR_ROOT) {
		printf("%*s%s\n", indent * 2, "", line);
	} else {
		detail = strchr(line + strlen("attr "), ' ');
		printf("%*s%s", indent * 2, "", name->name);
		if (options.show_ids)
			printf(" (%#018llx)",
			       (unsigned long long)node->attr_id);
		if (required >= 0)
			printf(" %s", required ? "required" : "optional");
		printf("%s", detail ? detail : "");
		if (name->definition) {
			unsigned int j;

			printf(" {");
			for (j = 0; j < name->definition->values_count; j++) {
				printf("%s%s", j ? ", " : "",
				       name->definition->values[j].name);
				if (options.show_ids)
					printf("=%#llx",
					       (unsigned long long)
					       name->definition->values[j].value);
			}
			printf("}");
		}
		printf("\n");
	}
	for (i = 0; i < node->attrs_count; i++)
		tree_print(spec, &node->attrs[i], indent + 1, true);
	for (i = 0; i < node->optional_attrs_count; i++)
		tree_print(spec, &node->optional_attrs[i], indent + 1, false);
}

static int cmd_schema(int argc, char **argv)
{
	const struct ctlvctl_op_name *op_name;
	const struct ctlv_op_info *op_info;
	const struct ctlv_node_schema *node;
	struct ctlvctl_spec *spec = NULL;
	struct ctlv_schema *schema;
	struct ctlv_ops *ops;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	char text[LINE_LEN];
	__u64 op_id;
	__u32 op_type = 0;
	bool template = false;
	int ret;

	if (argc && !strcmp(argv[0], "--template")) {
		template = true;
		argc--;
		argv++;
	}
	if (argc != 2) {
		help(stderr);
		return -EINVAL;
	}
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;
	ret = spec_load(dev, &spec);
	if (ret)
		goto out;
	ret = op_resolve(spec, argv[1], &op_id);
	if (ret)
		goto out;
	op_name = ctlvctl_spec_op_id(spec, op_id);
	if (spec) {
		ret = ctlv_ops_dump(dev, &ops, &error);
		if (ret) {
			fprintf(stderr, "%s: ops-dump: %s\n", prog,
				ctlv_error_str(&error, text, sizeof(text)));
			goto out;
		}
		op_info = ctlv_ops_find(ops, op_id);
		if (!op_info) {
			fprintf(stderr,
				"%s: operation is not available on %s\n",
				prog, ctlv_dev_name(dev));
			ret = -ENOENT;
		} else {
			op_type = op_info->type;
			if (op_name && !op_matches(op_name, op_info)) {
				fprintf(stderr,
					"%s: specification disagrees with operation %s\n",
					prog, op_name->name);
				ret = -ENOMSG;
			}
		}
		ctlv_ops_free(ops);
		if (ret)
			goto out;
	}

	ret = ctlv_op_schema_get(dev, op_id, &schema, &error);
	if (ret) {
		fprintf(stderr, "%s: op-schema-get: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}
	ret = tree_names_check(spec, ctlv_schema_request(schema));
	if (!ret)
		ret = tree_names_check(spec, ctlv_schema_reply(schema));
	if (ret)
		goto out_schema;
	if (template) {
		node = ctlv_schema_request(schema);
		if (!spec) {
			fprintf(stderr,
				"%s: --template requires --spec-dir\n", prog);
			ret = -EINVAL;
		} else if (!node) {
			printf("{}\n");
		} else {
#ifdef HAVE_JSON_C
			ret = json_template_print(node, spec);
#else
			ret = -EOPNOTSUPP;
#endif
		}
		goto out_schema;
	}
#ifdef HAVE_JSON_C
	if (options.output_json) {
		ret = json_schema_print(dev, spec, op_id, schema);
		goto out_schema;
	}
#endif
	if (op_name) {
		printf("%s (%s", op_name->name, type_name(op_name->type));
		if (options.show_ids)
			printf(", %#018llx", (unsigned long long)op_id);
		printf(")\n");
	} else if (spec) {
		printf("%#018llx (%s)\n", (unsigned long long)op_id,
		       type_name(op_type));
	}
	node = ctlv_schema_request(schema);
	if (node) {
		printf("request:\n");
		tree_print(spec, node, 1, -1);
	}
	node = ctlv_schema_reply(schema);
	if (node) {
		printf("reply:\n");
		tree_print(spec, node, 1, -1);
	}
out_schema:
	ctlv_schema_free(schema);
out:
	ctlvctl_spec_free(spec);
	ctlv_close(dev);
	return ret;
}

/*
 * An assignment names an attr by its complete ID and a dotted path enters a
 * nest. What a value means is decided by the kind the schema reports.
 */

/* The member of @parent for @attr_id, following any re-entry. */
static const struct ctlv_node_schema *
node_member(const struct ctlv_node_schema **stack, unsigned int depth,
	    __u64 attr_id)
{
	const struct ctlv_node_schema *parent = stack[depth];
	unsigned int level = depth;
	unsigned int i;

	if (!parent)
		return NULL;
	while (parent->reenters) {
		if (parent->reenters > level)
			return NULL;
		level -= parent->reenters;
		parent = stack[level];
		if (!parent)
			return NULL;
	}
	for (i = 0; i < parent->attrs_count; i++)
		if (parent->attrs[i].attr_id == attr_id)
			return &parent->attrs[i];
	for (i = 0; i < parent->optional_attrs_count; i++)
		if (parent->optional_attrs[i].attr_id == attr_id)
			return &parent->optional_attrs[i];
	return NULL;
}

static int hex_digit(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -EINVAL;
}

static int put_hex(struct ctlv_msg *msg, __u64 attr_id, const char *text)
{
	size_t text_len = strlen(text);
	size_t len = text_len / 2;
	unsigned char *bytes;
	size_t i;
	int ret;

	if (text_len & 1)
		return -EINVAL;
	bytes = malloc(len ? len : 1);
	if (!bytes)
		return -ENOMEM;
	for (i = 0; i < len; i++) {
		int high = hex_digit(text[i * 2]);
		int low = hex_digit(text[i * 2 + 1]);

		if (high < 0 || low < 0) {
			free(bytes);
			return -EINVAL;
		}
		bytes[i] = high << 4 | low;
	}
	ret = ctlv_put_bin(msg, attr_id, bytes, len);
	free(bytes);
	return ret;
}

static int flags_parse(const struct ctlvctl_attr_name *attr, const char *text,
		       __u64 *value)
{
	char *copy, *part, *walk;
	int ret = 0;

	if (!parse_u64(text, value))
		return 0;
	copy = strdup(text);
	if (!copy)
		return -ENOMEM;
	*value = 0;
	walk = copy;
	while ((part = strsep(&walk, ","))) {
		__u64 bit;

		if (!*part || ctlvctl_spec_value(attr, part, &bit)) {
			ret = -EINVAL;
			break;
		}
		*value |= bit;
	}
	free(copy);
	return ret;
}

static int put_value(struct ctlv_msg *msg, const struct ctlv_node_schema *node,
		     const struct ctlvctl_spec *spec, const char *text)
{
	const struct ctlvctl_attr_name *attr = attr_name(spec, node);
	__u64 value;
	__s64 signed_value;

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
		if (parse_u64(text, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	case CTLV_SCHEMA_ENUM:
		if (parse_u64(text, &value) &&
		    ctlvctl_spec_value(attr, text, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	case CTLV_SCHEMA_FLAGS:
		if (flags_parse(attr, text, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	case CTLV_SCHEMA_SINT:
		if (parse_s64(text, &signed_value))
			return -EINVAL;
		return ctlv_put_s64(msg, node->attr_id, signed_value);
	case CTLV_SCHEMA_BOOL:
		if (!strcmp(text, "true") || !strcmp(text, "1"))
			return ctlv_put_bool(msg, node->attr_id, true);
		if (!strcmp(text, "false") || !strcmp(text, "0"))
			return ctlv_put_bool(msg, node->attr_id, false);
		return -EINVAL;
	case CTLV_SCHEMA_STRING:
		return ctlv_put_str(msg, node->attr_id, text);
	case CTLV_SCHEMA_BINARY:
		if (attr) {
			if (strncmp(text, "hex:", 4))
				return -EINVAL;
			return put_hex(msg, node->attr_id, text + 4);
		}
		return ctlv_put_bin(msg, node->attr_id, text, strlen(text));
	default:
		/* A command line has no memory of the caller's to describe. */
		return -ENOTSUP;
	}
}

static const struct ctlv_node_schema *
node_named(const struct ctlv_node_schema *parent,
	   const struct ctlvctl_spec *spec, const char *name, bool *ambiguous)
{
	const struct ctlv_node_schema *match = NULL;
	unsigned int i;

	*ambiguous = false;
	for (i = 0; i < parent->attrs_count + parent->optional_attrs_count; i++) {
		const struct ctlv_node_schema *node;
		const struct ctlvctl_attr_name *attr;

		if (i < parent->attrs_count)
			node = &parent->attrs[i];
		else
			node = &parent->optional_attrs[i - parent->attrs_count];
		attr = attr_name(spec, node);
		if (!attr)
			continue;
		if (!strcmp(attr->name, name))
			return node;
		if (strcmp(attr->short_name, name))
			continue;
		if (match)
			*ambiguous = true;
		match = node;
	}
	return *ambiguous ? NULL : match;
}

/* One path element: an attr ID and which occurrence of it. */
static int path_element(char *name, __u64 *attr_id, __u64 *occurrence)
{
	char *at = strchr(name, '@');

	*occurrence = 0;
	if (at) {
		*at++ = '\0';
		if (parse_u64(at, occurrence))
			return -EINVAL;
	}
	return parse_u64(name, attr_id);
}

static int no_such_attr(__u64 attr_id)
{
	fprintf(stderr, "%s: %#llx: not an attr of this nest\n", prog,
		(unsigned long long)attr_id);
	return -ENOENT;
}

static int not_a_nest(__u64 attr_id)
{
	fprintf(stderr, "%s: %#llx: not a nest of this nest\n", prog,
		(unsigned long long)attr_id);
	return -ENOENT;
}

/**
 * request_build - Turns assignments into one request.
 * @msg: the message to append to
 * @root: the request tree the device reported
 * @argc: how many assignments
 * @argv: the assignments, each attr[@n][.attr[@n]...]=value
 *
 * Nests are opened as a path descends and closed as the next path leaves, so
 * assignments have to be given in the order the message holds them. Two
 * occurrences of one nest are told apart by the @n suffix.
 *
 * Return: 0, or a negative errno naming the assignment that failed.
 */
static int request_build(struct ctlv_msg *msg,
			 const struct ctlv_node_schema *root,
			 const struct ctlvctl_spec *spec, int argc, char **argv)
{
	const struct ctlv_node_schema *stack[PATH_DEPTH] = { root };
	__u64 path[PATH_DEPTH], occurrence[PATH_DEPTH];
	unsigned int depth = 0;
	int i, ret;

	for (i = 0; i < argc; i++) {
		char *text = strchr(argv[i], '=');
		unsigned int level = 0;
		char *name, *rest;

		if (!text) {
			fprintf(stderr, "%s: %s: not an assignment\n", prog,
				argv[i]);
			return -EINVAL;
		}
		*text++ = '\0';
		rest = argv[i];

		if (spec) {
			const struct ctlv_node_schema *node;
			bool ambiguous;

			node = node_named(root, spec, rest, &ambiguous);
			if (ambiguous) {
				fprintf(stderr, "%s: %s: ambiguous attribute\n",
					prog, rest);
				return -EINVAL;
			}
			if (node) {
				if (node->kind == CTLV_SCHEMA_BLOB) {
					fprintf(stderr,
						"%s: %s is a blob; blob input is not supported\n",
						prog, rest);
					return -ENOTSUP;
				}
				if (node->kind == CTLV_SCHEMA_NEST) {
					fprintf(stderr,
						"%s: %s: use --json for this attribute\n",
						prog, rest);
					return -ENOTSUP;
				}
				ret = put_value(msg, node, spec, text);
				if (ret) {
					fprintf(stderr, "%s: %s=%s: %s\n",
						prog, rest, text,
						strerror(-ret));
					return ret;
				}
				continue;
			}
		}

		while ((name = strsep(&rest, "."))) {
			const struct ctlv_node_schema *node;
			__u64 attr_id, which;

			if (level == PATH_DEPTH - 1)
				return -E2BIG;
			if (path_element(name, &attr_id, &which)) {
				fprintf(stderr, "%s: %s: not an attr ID\n",
					prog, name);
				return -EINVAL;
			}

			/*
			 * A different nest here, or the same one named again
			 * as a second occurrence, ends what came before.
			 */
			if (level < depth && (path[level] != attr_id ||
					      occurrence[level] != which))
				while (depth > level) {
					ctlv_nest_end(msg);
					depth--;
				}

			if (!rest) {
				/* The leaf, which carries the value. */
				while (depth > level) {
					ctlv_nest_end(msg);
					depth--;
				}
				node = node_member(stack, depth, attr_id);
				if (!node)
					return no_such_attr(attr_id);
				ret = put_value(msg, node, spec, text);
				if (ret) {
					fprintf(stderr,
						"%s: %#llx=%s: %s\n",
						prog,
						(unsigned long long)attr_id,
						text,
						strerror(-ret));
					return ret;
				}
				break;
			}

			if (level == depth) {
				node = node_member(stack, depth, attr_id);
				if (!node)
					return no_such_attr(attr_id);
				if (node->kind != CTLV_SCHEMA_NEST)
					return not_a_nest(attr_id);
				ret = ctlv_nest_start(msg, attr_id);
				if (ret)
					return ret;
				path[depth] = attr_id;
				occurrence[depth] = which;
				stack[depth + 1] = node;
				depth++;
			}
			level++;
		}
	}
	return ctlv_msg_err(msg);
}

#ifdef HAVE_JSON_C

static const struct ctlv_node_schema *
node_key(const struct ctlv_node_schema *parent,
	 const struct ctlvctl_spec *spec, const char *key, bool *ambiguous)
{
	__u64 id;

	if (!parse_u64(key, &id))
		return node_member((const struct ctlv_node_schema *[]){ parent },
				   0, id);
	return node_named(parent, spec, key, ambiguous);
}

static int json_node_value(struct json_object *object,
			   const struct ctlv_node_schema *parent,
			   const struct ctlv_node_schema *wanted,
			   const struct ctlvctl_spec *spec,
			   struct json_object **value, bool *present)
{
	struct json_object *found = NULL;

	json_object_object_foreach(object, key, item) {
		const struct ctlv_node_schema *node;
		bool ambiguous = false;

		node = node_key(parent, spec, key, &ambiguous);
		if (ambiguous) {
			fprintf(stderr, "%s: %s: ambiguous attribute\n",
				prog, key);
			return -EINVAL;
		}
		if (!node) {
			fprintf(stderr, "%s: %s: no such attribute here\n",
				prog, key);
			return -ENOENT;
		}
		if (node != wanted)
			continue;
		if (found) {
			fprintf(stderr, "%s: %s: attribute given twice\n",
				prog, key);
			return -EINVAL;
		}
		found = item;
	}
	*present = !!found;
	*value = found;
	return 0;
}

static int json_text_value(struct json_object *value, const char **text)
{
	if (!json_object_is_type(value, json_type_string))
		return -EINVAL;
	*text = json_object_get_string(value);
	if (strlen(*text) != (size_t)json_object_get_string_len(value))
		return -EINVAL;
	return 0;
}

static int json_u64_value(struct json_object *value, __u64 *number)
{
	const char *text;

	if (json_object_is_type(value, json_type_int)) {
		text = json_object_get_string(value);
		return parse_u64(text, number);
	}
	if (!json_text_value(value, &text))
		return parse_u64(text, number);
	return -EINVAL;
}

static int json_s64_value(struct json_object *value, __s64 *number)
{
	if (json_object_is_type(value, json_type_int))
		return parse_s64(json_object_get_string(value), number);
	{
		const char *text;

		if (json_text_value(value, &text))
			return -EINVAL;
		return parse_s64(text, number);
	}
}

static int json_flags_value(const struct ctlvctl_attr_name *attr,
			    struct json_object *json, __u64 *value)
{
	size_t i, n;

	if (!json_object_is_type(json, json_type_array))
		return json_u64_value(json, value);
	*value = 0;
	n = json_object_array_length(json);
	for (i = 0; i < n; i++) {
		struct json_object *item = json_object_array_get_idx(json, i);
		const char *name;
		__u64 bit;

		if (json_text_value(item, &name) ||
		    ctlvctl_spec_value(attr, name, &bit))
			return -EINVAL;
		*value |= bit;
	}
	return 0;
}

static int json_put_value(struct ctlv_msg *msg,
			  const struct ctlv_node_schema *node,
			  const struct ctlvctl_spec *spec,
			  struct json_object *json)
{
	const struct ctlvctl_attr_name *attr = attr_name(spec, node);
	const char *text;
	__u64 value;
	__s64 signed_value;

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
		if (json_u64_value(json, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	case CTLV_SCHEMA_SINT:
		if (json_s64_value(json, &signed_value))
			return -EINVAL;
		return ctlv_put_s64(msg, node->attr_id, signed_value);
	case CTLV_SCHEMA_BOOL:
		if (!json_object_is_type(json, json_type_boolean))
			return -EINVAL;
		return ctlv_put_bool(msg, node->attr_id,
				     json_object_get_boolean(json));
	case CTLV_SCHEMA_STRING:
		if (json_text_value(json, &text))
			return -EINVAL;
		return ctlv_put_str(msg, node->attr_id, text);
	case CTLV_SCHEMA_BINARY:
		if (json_text_value(json, &text))
			return -EINVAL;
		if (strncmp(text, "hex:", 4))
			return -EINVAL;
		return put_hex(msg, node->attr_id, text + 4);
	case CTLV_SCHEMA_ENUM:
		if (json_object_is_type(json, json_type_string)) {
			if (json_text_value(json, &text) ||
			    ctlvctl_spec_value(attr, text, &value))
				return -EINVAL;
			return ctlv_put_u64(msg, node->attr_id, value);
		}
		if (json_u64_value(json, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	case CTLV_SCHEMA_FLAGS:
		if (json_flags_value(attr, json, &value))
			return -EINVAL;
		return ctlv_put_u64(msg, node->attr_id, value);
	default:
		return -ENOTSUP;
	}
}

static const struct ctlv_node_schema *
node_members(const struct ctlv_node_schema **stack, unsigned int depth)
{
	const struct ctlv_node_schema *node = stack[depth];
	unsigned int level = depth;

	if (!node)
		return NULL;
	while (node->reenters) {
		if (node->reenters > level)
			return NULL;
		level -= node->reenters;
		node = stack[level];
		if (!node)
			return NULL;
	}
	return node;
}

static int json_build_object(struct ctlv_msg *msg,
			     const struct ctlv_node_schema **stack,
			     unsigned int depth, struct json_object *object,
			     const struct ctlvctl_spec *spec);

static int json_build_one(struct ctlv_msg *msg,
			  const struct ctlv_node_schema **stack,
			  unsigned int depth,
			  const struct ctlv_node_schema *node,
			  struct json_object *value,
			  const struct ctlvctl_spec *spec)
{
	int ret;

	if (node->kind == CTLV_SCHEMA_BLOB) {
		const struct ctlvctl_attr_name *name = attr_name(spec, node);

		fprintf(stderr, "%s: %s is a blob; blob input is not supported\n",
			prog, name ? name->name : "attribute");
		return -ENOTSUP;
	}
	if (node->kind != CTLV_SCHEMA_NEST)
		return json_put_value(msg, node, spec, value);
	if (!json_object_is_type(value, json_type_object))
		return -EINVAL;
	if (depth + 1 >= PATH_DEPTH)
		return -E2BIG;

	ret = ctlv_nest_start(msg, node->attr_id);
	if (ret)
		return ret;
	stack[depth + 1] = node;
	ret = json_build_object(msg, stack, depth + 1, value, spec);
	ctlv_nest_end(msg);
	return ret;
}

static int json_build_node(struct ctlv_msg *msg,
			   const struct ctlv_node_schema **stack,
			   unsigned int depth,
			   const struct ctlv_node_schema *node,
			   struct json_object *value,
			   const struct ctlvctl_spec *spec)
{
	size_t i, n;
	int ret;

	if (node->max_occurrences == 1)
		return json_build_one(msg, stack, depth, node, value, spec);
	if (!json_object_is_type(value, json_type_array))
		return -EINVAL;
	n = json_object_array_length(value);
	if (node->max_occurrences && n > node->max_occurrences)
		return -E2BIG;
	for (i = 0; i < n; i++) {
		ret = json_build_one(msg, stack, depth, node,
				     json_object_array_get_idx(value, i), spec);
		if (ret)
			return ret;
	}
	return 0;
}

static int json_build_members(struct ctlv_msg *msg,
			      const struct ctlv_node_schema **stack,
			      unsigned int depth,
			      const struct ctlv_node_schema *parent,
			      const struct ctlv_node_schema *node,
			      unsigned int n, bool required,
			      struct json_object *object,
			      const struct ctlvctl_spec *spec)
{
	unsigned int i;

	for (i = 0; i < n; i++) {
		const struct ctlvctl_attr_name *name = attr_name(spec, &node[i]);
		struct json_object *value;
		bool present;
		int ret;

		ret = json_node_value(object, parent, &node[i], spec,
				      &value, &present);
		if (ret)
			return ret;
		if (!present) {
			if (!required)
				continue;
			fprintf(stderr, "%s: %s: required attribute is missing\n",
				prog, name ? name->name : "attribute");
			return -EINVAL;
		}
		if (required && node[i].max_occurrences != 1 &&
		    json_object_is_type(value, json_type_array) &&
		    !json_object_array_length(value)) {
			fprintf(stderr, "%s: %s: at least one value is required\n",
				prog, name ? name->name : "attribute");
			return -EINVAL;
		}
		ret = json_build_node(msg, stack, depth, &node[i], value,
				      spec);
		if (ret) {
			if (ret != -ENOTSUP || node[i].kind != CTLV_SCHEMA_BLOB)
				fprintf(stderr, "%s: %s: %s\n", prog,
					name ? name->name : "attribute",
					strerror(-ret));
			return ret;
		}
	}
	return 0;
}

static int json_build_object(struct ctlv_msg *msg,
			     const struct ctlv_node_schema **stack,
			     unsigned int depth, struct json_object *object,
			     const struct ctlvctl_spec *spec)
{
	const struct ctlv_node_schema *parent;
	int ret;

	if (!json_object_is_type(object, json_type_object))
		return -EINVAL;
	parent = node_members(stack, depth);
	if (!parent)
		return -EINVAL;
	json_object_object_foreach(object, key, item) {
		bool ambiguous = false;

		if (!node_key(parent, spec, key, &ambiguous)) {
			fprintf(stderr, "%s: %s: %s attribute\n", prog, key,
				ambiguous ? "ambiguous" : "no such");
			return ambiguous ? -EINVAL : -ENOENT;
		}
	}

	ret = json_build_members(msg, stack, depth, parent, parent->attrs,
				 parent->attrs_count, true, object, spec);
	if (ret)
		return ret;
	return json_build_members(msg, stack, depth, parent,
				  parent->optional_attrs,
				  parent->optional_attrs_count, false, object,
				  spec);
}

static int stream_read(FILE *file, char **textp, size_t *lenp)
{
	size_t used = 0, size = 4096;
	char *text;

	text = malloc(size);
	if (!text)
		return -ENOMEM;
	for (;;) {
		size_t n;

		if (used == size - 1) {
			char *grown;

			size *= 2;
			grown = realloc(text, size);
			if (!grown) {
				free(text);
				return -ENOMEM;
			}
			text = grown;
		}
		n = fread(text + used, 1, size - used - 1, file);
		used += n;
		if (n)
			continue;
		if (feof(file))
			break;
		if (errno == EINTR) {
			clearerr(file);
			continue;
		}
		free(text);
		return -errno;
	}
	text[used] = '\0';
	*textp = text;
	*lenp = used;
	return 0;
}

static int json_source_read(const char *source, char **textp, size_t *lenp)
{
	FILE *file;
	int ret;

	if (source[0] != '@' && strcmp(source, "-")) {
		*textp = strdup(source);
		if (!*textp)
			return -ENOMEM;
		*lenp = strlen(*textp);
		return 0;
	}
	if (!strcmp(source, "-"))
		return stream_read(stdin, textp, lenp);

	file = fopen(source + 1, "re");
	if (!file)
		return -errno;
	ret = stream_read(file, textp, lenp);
	fclose(file);
	return ret;
}

static int json_request_build(struct ctlv_msg *msg,
			      const struct ctlv_node_schema *root,
			      const struct ctlvctl_spec *spec,
			      const char *source)
{
	const struct ctlv_node_schema *stack[PATH_DEPTH] = { root };
	struct json_tokener *tokener;
	struct json_object *object;
	enum json_tokener_error error;
	size_t text_len;
	char *text;
	int ret;

	ret = json_source_read(source, &text, &text_len);
	if (ret)
		return ret;
	if (memchr(text, '\0', text_len)) {
		free(text);
		return -EINVAL;
	}
	if (text_len >= INT_MAX) {
		free(text);
		return -E2BIG;
	}
	tokener = json_tokener_new_ex(PATH_DEPTH * 2 + 2);
	if (!tokener) {
		free(text);
		return -ENOMEM;
	}
	json_tokener_set_flags(tokener, JSON_TOKENER_STRICT |
			       JSON_TOKENER_VALIDATE_UTF8);
	object = json_tokener_parse_ex(tokener, text, text_len + 1);
	error = json_tokener_get_error(tokener);
	if (error != json_tokener_success || !object) {
		fprintf(stderr, "%s: invalid JSON request: %s\n", prog,
			json_tokener_error_desc(error));
		ret = -EINVAL;
	} else {
		ret = json_build_object(msg, stack, 0, object, spec);
		json_object_put(object);
	}
	json_tokener_free(tokener);
	free(text);
	return ret;
}

#else /* HAVE_JSON_C */

static int json_request_build(struct ctlv_msg *msg,
			      const struct ctlv_node_schema *root,
			      const struct ctlvctl_spec *spec,
			      const char *source)
{
	fprintf(stderr, "%s: JSON support was built without json-c\n", prog);
	return -EOPNOTSUPP;
}

#endif /* HAVE_JSON_C */

#ifdef HAVE_JSON_C

static char *hex_encode(const void *bytes, __u32 len, const char *prefix)
{
	const unsigned char *data = bytes;
	size_t prefix_len = strlen(prefix);
	char *text;
	__u32 i;

	text = malloc(prefix_len + (size_t)len * 2 + 1);
	if (!text)
		return NULL;
	memcpy(text, prefix, prefix_len);
	for (i = 0; i < len; i++)
		snprintf(text + prefix_len + i * 2, 3, "%02x", data[i]);
	return text;
}

static struct json_object *json_binary(const void *bytes, __u32 len)
{
	struct json_object *json;
	char *text;

	text = hex_encode(bytes, len, "hex:");
	if (!text)
		return NULL;
	json = json_object_new_string(text);
	free(text);
	return json;
}

static struct json_object *json_invalid_utf8(const void *bytes, __u32 len)
{
	struct json_object *json;
	char *text;

	json = json_object_new_object();
	text = hex_encode(bytes, len, "");
	if (!json || !text) {
		json_object_put(json);
		free(text);
		return NULL;
	}
	json_add_string(json, "encoding", "hex");
	json_add_string(json, "data", text);
	free(text);
	return json;
}

static bool utf8_valid(const unsigned char *text, size_t len)
{
	size_t i = 0;

	while (i < len) {
		unsigned char first = text[i++];
		unsigned int continuation;

		if (first < 0x80)
			continue;
		if (first >= 0xc2 && first <= 0xdf) {
			continuation = 1;
		} else if (first >= 0xe0 && first <= 0xef) {
			continuation = 2;
			if (i >= len ||
			    (first == 0xe0 && text[i] < 0xa0) ||
			    (first == 0xed && text[i] >= 0xa0))
				return false;
		} else if (first >= 0xf0 && first <= 0xf4) {
			continuation = 3;
			if (i >= len ||
			    (first == 0xf0 && text[i] < 0x90) ||
			    (first == 0xf4 && text[i] >= 0x90))
				return false;
		} else {
			return false;
		}
		if (continuation > len - i)
			return false;
		while (continuation--)
			if ((text[i++] & 0xc0) != 0x80)
				return false;
	}
	return true;
}

static struct json_object *
json_payload(const struct ctlv_attr *attr,
	     const struct ctlv_node_schema *node,
	     const struct ctlvctl_spec *spec)
{
	const struct ctlvctl_attr_name *name = attr_name(spec, node);
	const void *bytes;
	const char *text;
	struct json_object *json;
	__u64 value;
	__s64 signed_value;
	__u32 len;
	bool flag;
	unsigned int i;

	switch (node ? node->kind : CTLV_SCHEMA_BINARY) {
	case CTLV_SCHEMA_UINT:
		if (ctlv_get_u64(attr, &value))
			return NULL;
		return json_object_new_uint64(value);
	case CTLV_SCHEMA_SINT:
		if (ctlv_get_s64(attr, &signed_value))
			return NULL;
		return json_object_new_int64(signed_value);
	case CTLV_SCHEMA_BOOL:
		if (ctlv_get_bool(attr, &flag))
			return NULL;
		return json_object_new_boolean(flag);
	case CTLV_SCHEMA_STRING:
		if (ctlv_get_str(attr, &text))
			return NULL;
		len = ctlv_attr_payload_len(attr) - 1;
		if (!utf8_valid((const unsigned char *)text, len))
			return json_invalid_utf8(text, len);
		return json_object_new_string_len(text, len);
	case CTLV_SCHEMA_ENUM:
		if (ctlv_get_u64(attr, &value))
			return NULL;
		text = ctlvctl_spec_value_name(name, value);
		return text ? json_object_new_string(text) :
			      json_object_new_uint64(value);
	case CTLV_SCHEMA_FLAGS:
		if (ctlv_get_u64(attr, &value))
			return NULL;
		if (!name || !name->definition)
			return json_object_new_uint64(value);
		json = json_object_new_array();
		if (!json)
			return NULL;
		for (i = 0; i < name->definition->values_count; i++) {
			const struct ctlvctl_value_name *item =
				&name->definition->values[i];

			if (!(value & item->value))
				continue;
			if (json_array_add_string(json, item->name)) {
				json_object_put(json);
				return NULL;
			}
			value &= ~item->value;
		}
		if (value) {
			char unknown[32];

			snprintf(unknown, sizeof(unknown), "%#llx",
				 (unsigned long long)value);
			if (json_array_add_string(json, unknown)) {
				json_object_put(json);
				return NULL;
			}
		}
		return json;
	default:
		if (ctlv_get_bin(attr, &bytes, &len))
			return NULL;
		return json_binary(bytes, len);
	}
}

static int json_object_add_value(struct json_object *object, const char *key,
				 struct json_object *value, bool repeated)
{
	struct json_object *array, *existing;
	int ret;

	if (!json_object_object_get_ex(object, key, &existing) && !repeated) {
		ret = json_object_object_add(object, key, value);
		if (ret)
			json_object_put(value);
		return ret ? -ENOMEM : 0;
	}
	if (!json_object_object_get_ex(object, key, &existing)) {
		array = json_object_new_array();
		if (!array) {
			json_object_put(value);
			return -ENOMEM;
		}
		ret = json_object_object_add(object, key, array);
		if (ret) {
			json_object_put(array);
			json_object_put(value);
			return -ENOMEM;
		}
		ret = json_object_array_add(array, value);
		if (ret)
			json_object_put(value);
		return ret ? -ENOMEM : 0;
	}
	if (json_object_is_type(existing, json_type_array)) {
		ret = json_object_array_add(existing, value);
		if (ret)
			json_object_put(value);
		return ret ? -ENOMEM : 0;
	}

	array = json_object_new_array();
	if (!array) {
		json_object_put(value);
		return -ENOMEM;
	}
	json_object_get(existing);
	if (json_object_array_add(array, existing)) {
		json_object_put(existing);
		json_object_put(array);
		json_object_put(value);
		return -ENOMEM;
	}
	if (json_object_object_add(object, key, array)) {
		json_object_put(array);
		json_object_put(value);
		return -ENOMEM;
	}
	ret = json_object_array_add(array, value);
	if (ret)
		json_object_put(value);
	return ret ? -ENOMEM : 0;
}

static struct json_object *
json_reply_object(const struct ctlv_attr *nest,
		  const struct ctlv_node_schema **stack,
		  unsigned int depth, const struct ctlvctl_spec *spec)
{
	struct json_object *object;
	const struct ctlv_attr *child;

	object = json_object_new_object();
	if (!object)
		return NULL;
	ctlv_for_each(child, nest) {
		const struct ctlv_node_schema *node = NULL;
		const struct ctlvctl_attr_name *name;
		struct json_object *value;
		char numeric[32];
		const char *key;

		if (depth < PATH_DEPTH)
			node = node_member(stack, depth, child->attr_id);
		name = node ? attr_name(spec, node) : NULL;
		if (name) {
			key = name->name;
		} else {
			snprintf(numeric, sizeof(numeric), "%#018llx",
				 (unsigned long long)child->attr_id);
			key = numeric;
		}
		if (ctlv_attr_is_nest(child)) {
			if (depth + 1 >= PATH_DEPTH) {
				json_object_put(object);
				return NULL;
			}
			stack[depth + 1] = node;
			value = json_reply_object(child, stack, depth + 1,
						  spec);
		} else {
			value = json_payload(child, node, spec);
		}
		if (!value ||
		    json_object_add_value(object, key, value,
					  node &&
					  node->max_occurrences != 1)) {
			json_object_put(object);
			return NULL;
		}
	}
	return object;
}

static struct json_object *
json_reply(const struct ctlv_attr *root,
	   const struct ctlv_node_schema *tree,
	   const struct ctlvctl_spec *spec)
{
	const struct ctlv_node_schema *stack[PATH_DEPTH] = { tree };

	return json_reply_object(root, stack, 0, spec);
}

static void json_add_id(struct json_object *object, const char *key, __u64 id)
{
	char text[32];

	snprintf(text, sizeof(text), "%#018llx", (unsigned long long)id);
	json_object_object_add(object, key, json_object_new_string(text));
}

static void json_add_op(struct json_object *object,
			const struct ctlvctl_spec *spec, __u64 op_id)
{
	const struct ctlvctl_op_name *name = ctlvctl_spec_op_id(spec, op_id);

	if (name)
		json_add_string(object, "op", name->name);
	if (!name || options.show_ids)
		json_add_id(object, "id", op_id);
}

static const char *schema_kind_name(__u32 kind)
{
	static const char *const names[] = {
		[CTLV_SCHEMA_UINT] = "uint",
		[CTLV_SCHEMA_SINT] = "sint",
		[CTLV_SCHEMA_BOOL] = "bool",
		[CTLV_SCHEMA_STRING] = "string",
		[CTLV_SCHEMA_BINARY] = "binary",
		[CTLV_SCHEMA_ENUM] = "enum",
		[CTLV_SCHEMA_FLAGS] = "flags",
		[CTLV_SCHEMA_NEST] = "nest",
		[CTLV_SCHEMA_BLOB] = "blob",
	};

	if (kind >= ARRAY_SIZE(names) || !names[kind])
		return "unknown";
	return names[kind];
}

static struct json_object *
json_schema_node(const struct ctlv_node_schema *node,
		 const struct ctlvctl_spec *spec, bool required)
{
	const struct ctlvctl_attr_name *name = attr_name(spec, node);
	struct json_object *object, *array;
	unsigned int i;

	object = json_object_new_object();
	if (!object)
		return NULL;
	if (node->attr_id != CTLV_ATTR_ROOT) {
		if (name)
			json_add_string(object, "name", name->name);
		if (!name || options.show_ids)
			json_add_id(object, "id", node->attr_id);
		json_add_bool(object, "required", required);
	}
	json_add_string(object, "kind", schema_kind_name(node->kind));
	if (!node->max_occurrences)
		json_add_string(object, "max-occurrences", "unlimited");
	else
		json_add_u64(object, "max-occurrences",
			     node->max_occurrences);
	if (node->present & CTLV_NODE_SCHEMA_HAS_MIN) {
		if (node->kind == CTLV_SCHEMA_SINT)
			json_add_s64(object, "min", (__s64)node->min);
		else
			json_add_u64(object, "min", node->min);
	}
	if (node->present & CTLV_NODE_SCHEMA_HAS_MAX) {
		if (node->kind == CTLV_SCHEMA_SINT)
			json_add_s64(object, "max", (__s64)node->max);
		else
			json_add_u64(object, "max", node->max);
	}
	if (node->present & CTLV_NODE_SCHEMA_HAS_MIN_LENGTH)
		json_add_u64(object, "min-length", node->min_length);
	if (node->present & CTLV_NODE_SCHEMA_HAS_MAX_LENGTH)
		json_add_u64(object, "max-length", node->max_length);
	if (name && name->definition) {
		array = json_object_new_array();
		if (!array) {
			json_object_put(object);
			return NULL;
		}
		for (i = 0; i < name->definition->values_count; i++) {
			const char *value_name =
				name->definition->values[i].name;

			if (json_array_add_string(array, value_name)) {
				json_object_put(array);
				json_object_put(object);
				return NULL;
			}
		}
		json_object_object_add(object, "values", array);
	}
	if (node->reenters)
		json_add_u64(object, "reenters", node->reenters);

	array = json_object_new_array();
	if (!array) {
		json_object_put(object);
		return NULL;
	}
	for (i = 0; i < node->attrs_count; i++) {
		struct json_object *child =
			json_schema_node(&node->attrs[i], spec, true);

		if (!child) {
			json_object_put(array);
			json_object_put(object);
			return NULL;
		}
		json_object_array_add(array, child);
	}
	for (i = 0; i < node->optional_attrs_count; i++) {
		struct json_object *child =
			json_schema_node(&node->optional_attrs[i], spec, false);

		if (!child) {
			json_object_put(array);
			json_object_put(object);
			return NULL;
		}
		json_object_array_add(array, child);
	}
	if (json_object_array_length(array))
		json_object_object_add(object, "attrs", array);
	else
		json_object_put(array);
	return object;
}

static int json_schema_print(const struct ctlv_dev *dev,
			     const struct ctlvctl_spec *spec, __u64 op_id,
			     const struct ctlv_schema *schema)
{
	const struct ctlv_node_schema *node;
	struct json_object *object, *tree;

	object = json_object_new_object();
	if (!object)
		return -ENOMEM;
	json_add_string(object, "device", ctlv_dev_name(dev));
	json_add_op(object, spec, op_id);
	node = ctlv_schema_request(schema);
	if (node) {
		tree = json_schema_node(node, spec, true);
		if (!tree)
			goto err;
		json_object_object_add(object, "request", tree);
	}
	node = ctlv_schema_reply(schema);
	if (node) {
		tree = json_schema_node(node, spec, true);
		if (!tree)
			goto err;
		json_object_object_add(object, "reply", tree);
	}
	return json_print(object);
err:
	json_object_put(object);
	return -ENOMEM;
}

static int json_print(struct json_object *object)
{
	const char *text;
	size_t len;
	int ret = 0;

	if (!object)
		return -ENOMEM;
	text = json_object_to_json_string_ext(object,
					      JSON_C_TO_STRING_PLAIN);
	if (!text) {
		ret = -ENOMEM;
		goto out;
	}
	len = strlen(text);
	if (fwrite(text, 1, len, stdout) != len ||
	    fputc('\n', stdout) == EOF || fflush(stdout))
		ret = errno ? -errno : -EIO;
out:
	json_object_put(object);
	return ret;
}

static struct json_object *
json_template_object(const struct ctlv_node_schema **stack,
		     unsigned int depth, const struct ctlvctl_spec *spec);

static struct json_object *
json_template_value(const struct ctlv_node_schema **stack,
		    unsigned int depth, const struct ctlv_node_schema *node,
		    const struct ctlvctl_spec *spec)
{
	const struct ctlvctl_attr_name *name = attr_name(spec, node);
	const char *text;
	struct json_object *value;
	__u64 number;

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
		number = node->present & CTLV_NODE_SCHEMA_HAS_MIN ? node->min : 0;
		value = json_object_new_uint64(number);
		break;
	case CTLV_SCHEMA_SINT:
		number = node->present & CTLV_NODE_SCHEMA_HAS_MIN ? node->min : 0;
		value = json_object_new_int64((__s64)number);
		break;
	case CTLV_SCHEMA_BOOL:
		value = json_object_new_boolean(false);
		break;
	case CTLV_SCHEMA_STRING: {
		size_t len = node->present & CTLV_NODE_SCHEMA_HAS_MIN_LENGTH ?
			     node->min_length : 0;
		char *text = calloc(len + 1, 1);

		if (!text)
			return NULL;
		memset(text, 'x', len);
		value = json_object_new_string_len(text, len);
		free(text);
		break;
	}
	case CTLV_SCHEMA_BINARY: {
		size_t len = node->present & CTLV_NODE_SCHEMA_HAS_MIN_LENGTH ?
			     node->min_length : 0;
		unsigned char *bytes = calloc(len ? len : 1, 1);

		if (!bytes)
			return NULL;
		value = json_binary(bytes, len);
		free(bytes);
		break;
	}
	case CTLV_SCHEMA_ENUM:
		number = node->present & CTLV_NODE_SCHEMA_HAS_MIN ? node->min : 0;
		text = ctlvctl_spec_value_name(name, number);
		if (text)
			value = json_object_new_string(text);
		else
			value = json_object_new_uint64(number);
		break;
	case CTLV_SCHEMA_FLAGS:
		value = json_object_new_array();
		break;
	case CTLV_SCHEMA_NEST:
		if (depth + 1 >= PATH_DEPTH)
			return NULL;
		stack[depth + 1] = node;
		value = json_template_object(stack, depth + 1, spec);
		break;
	default:
		fprintf(stderr, "%s: cannot template a blob request\n", prog);
		return NULL;
	}
	if (!value)
		return NULL;
	if (node->max_occurrences == 1)
		return value;

	{
		struct json_object *array = json_object_new_array();

		if (!array) {
			json_object_put(value);
			return NULL;
		}
		json_object_array_add(array, value);
		return array;
	}
}

static struct json_object *
json_template_object(const struct ctlv_node_schema **stack,
		     unsigned int depth, const struct ctlvctl_spec *spec)
{
	const struct ctlv_node_schema *parent = node_members(stack, depth);
	struct json_object *object;
	unsigned int i;

	if (!parent)
		return NULL;
	object = json_object_new_object();
	if (!object)
		return NULL;
	for (i = 0; i < parent->attrs_count; i++) {
		const struct ctlv_node_schema *node = &parent->attrs[i];
		const struct ctlvctl_attr_name *name = attr_name(spec, node);
		struct json_object *value;
		char numeric[32];
		const char *key;

		value = json_template_value(stack, depth, node, spec);
		if (!value) {
			json_object_put(object);
			return NULL;
		}
		if (name) {
			key = name->name;
		} else {
			snprintf(numeric, sizeof(numeric), "%#018llx",
				 (unsigned long long)node->attr_id);
			key = numeric;
		}
		json_object_object_add(object, key, value);
	}
	return object;
}

static int json_template_print(const struct ctlv_node_schema *root,
			       const struct ctlvctl_spec *spec)
{
	const struct ctlv_node_schema *stack[PATH_DEPTH] = { root };

	return json_print(json_template_object(stack, 0, spec));
}

#endif /* HAVE_JSON_C */

static void payload_print(const struct ctlv_attr *attr,
			  const struct ctlv_node_schema *node,
			  const struct ctlvctl_spec *spec)
{
	const struct ctlvctl_attr_name *name = attr_name(spec, node);
	const void *bytes;
	const char *text;
	__u64 value;
	__s64 signed_value;
	bool flag;
	__u32 len, i;

	switch (node ? node->kind : CTLV_SCHEMA_BINARY) {
	case CTLV_SCHEMA_UINT:
		if (!ctlv_get_u64(attr, &value))
			printf("%llu", (unsigned long long)value);
		return;
	case CTLV_SCHEMA_ENUM:
		if (ctlv_get_u64(attr, &value))
			return;
		text = ctlvctl_spec_value_name(name, value);
		if (!text) {
			printf("%llu", (unsigned long long)value);
			return;
		}
		printf("%s", text);
		if (options.show_ids)
			printf(" (%llu)", (unsigned long long)value);
		return;
	case CTLV_SCHEMA_FLAGS:
		if (ctlv_get_u64(attr, &value))
			return;
		if (name && name->definition) {
			__u64 unknown = value;
			bool first = true;

			for (i = 0; i < name->definition->values_count; i++) {
				const struct ctlvctl_value_name *item =
					&name->definition->values[i];

				if (!(value & item->value))
					continue;
				printf("%s%s", first ? "" : ",", item->name);
				unknown &= ~item->value;
				first = false;
			}
			if (unknown || first)
				printf("%s%#llx", first ? "" : ",",
				       (unsigned long long)unknown);
			if (options.show_ids)
				printf(" (%#llx)", (unsigned long long)value);
		} else {
			printf("%#llx", (unsigned long long)value);
		}
		return;
	case CTLV_SCHEMA_SINT:
		if (!ctlv_get_s64(attr, &signed_value))
			printf("%lld", (long long)signed_value);
		return;
	case CTLV_SCHEMA_BOOL:
		if (!ctlv_get_bool(attr, &flag))
			printf("%s", flag ? "true" : "false");
		return;
	case CTLV_SCHEMA_STRING:
		if (!ctlv_get_str(attr, &text))
			printf("%s", text);
		return;
	default:
		if (ctlv_get_bin(attr, &bytes, &len))
			return;
		if (name)
			printf("hex:");
		for (i = 0; i < len; i++)
			printf("%02x", ((const __u8 *)bytes)[i]);
		return;
	}
}

/*
 * Printed as the schema says, and an attr the schema does not mention is
 * printed as bytes with a "?": a device newer than this tool is not an error.
 */
static void reply_print(const struct ctlv_attr *nest,
			const struct ctlv_node_schema **stack,
			unsigned int depth, const struct ctlvctl_spec *spec)
{
	const struct ctlv_attr *child;

	ctlv_for_each(child, nest) {
		const struct ctlv_node_schema *node = NULL;
		const struct ctlvctl_attr_name *name;

		if (depth < PATH_DEPTH)
			node = node_member(stack, depth, child->attr_id);
		name = node ? attr_name(spec, node) : NULL;
		printf("%*s", (depth + 1) * 2, "");
		if (name) {
			printf("%s", name->name);
			if (options.show_ids)
				printf(" (%#018llx)",
				       (unsigned long long)child->attr_id);
		} else {
			printf("%#018llx%s",
			       (unsigned long long)child->attr_id,
			       node ? "" : "?");
		}
		if (ctlv_attr_is_nest(child)) {
			printf("\n");
			if (depth + 1 < PATH_DEPTH) {
				stack[depth + 1] = node;
				reply_print(child, stack, depth + 1, spec);
			}
			continue;
		}
		printf(" ");
		payload_print(child, node, spec);
		printf("\n");
	}
}

static void reply_dump(const struct ctlv_attr *root,
		       const struct ctlv_node_schema *tree,
		       const struct ctlvctl_spec *spec)
{
	const struct ctlv_node_schema *stack[PATH_DEPTH] = { tree };

	reply_print(root, stack, 0, spec);
}

static int run_op_info(const struct ctlv_dev *dev,
		       const struct ctlvctl_spec *spec,
		       const struct ctlv_ops *ops, __u64 op_id,
		       const char *op_text, bool query,
		       const struct ctlv_op_info **infop)
{
	const struct ctlvctl_op_name *name = ctlvctl_spec_op_id(spec, op_id);
	const struct ctlv_op_info *info = ctlv_ops_find(ops, op_id);

	if (!info) {
		fprintf(stderr, "%s: operation is not available on %s\n",
			prog, ctlv_dev_name(dev));
		return -ENOENT;
	}
	if (name && !op_matches(name, info)) {
		fprintf(stderr, "%s: specification disagrees with operation %s\n",
			prog, name->name);
		return -ENOMSG;
	}
	if (info->type != (query ? CTLV_OP_TYPE_QUERY :
				  CTLV_OP_TYPE_ACTION)) {
		fprintf(stderr, "%s: %s is a %s; use \"%s\"\n",
			prog, name ? name->name : op_text,
			type_name(info->type),
			info->type == CTLV_OP_TYPE_QUERY ? "query" :
			info->type == CTLV_OP_TYPE_ACTION ? "exec" :
			"monitor");
		return -EINVAL;
	}
	if (info->fd_access != CTLV_OP_FD_ACCESS_READ &&
	    info->fd_access != CTLV_OP_FD_ACCESS_READ_WRITE)
		return -EPROTO;
	*infop = info;
	return 0;
}

static int cmd_run(int argc, char **argv, bool query)
{
	const struct ctlvctl_op_name *name;
	const struct ctlv_node_schema *request_tree;
	const struct ctlv_op_info *info;
	struct ctlvctl_spec *spec = NULL;
	struct ctlv_msg *request = NULL;
	struct ctlv_reply *reply = NULL;
	struct ctlv_schema *schema;
	struct ctlv_ops *ops = NULL;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	const char *json_source = NULL;
	char text[LINE_LEN];
	__u64 reply_len = 0;
	__u64 op_id;
	__u32 fd_access;
	int ret;

	if (query && argc > 1 && !strcmp(argv[0], "-l")) {
		ret = parse_u64(argv[1], &reply_len);
		if (ret || !reply_len ||
		    reply_len > CTLV_MAX_INLINE_MESSAGE_LEN) {
			fprintf(stderr, "%s: %s: not a reply length\n", prog,
				argv[1]);
			return ret ? ret : -EINVAL;
		}
		argc -= 2;
		argv += 2;
	}

	if (argc < 2) {
		help(stderr);
		return -EINVAL;
	}
	if (argc > 2 && !strcmp(argv[2], "--json")) {
		if (argc != 4) {
			fprintf(stderr,
				"%s: --json needs one source and no assignments\n",
				prog);
			return -EINVAL;
		}
		json_source = argv[3];
	}
	/* Discovery is always available through an observation fd. */
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;
	ret = spec_load(dev, &spec);
	if (ret)
		goto out_close;
	ret = op_resolve(spec, argv[1], &op_id);
	if (ret)
		goto out_close;

	name = ctlvctl_spec_op_id(spec, op_id);
	ret = ctlv_ops_dump(dev, &ops, &error);
	if (ret) {
		fprintf(stderr, "%s: ops-dump: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out_close;
	}
	ret = run_op_info(dev, spec, ops, op_id, argv[1], query, &info);
	if (ret)
		goto out_free_ops;
	fd_access = info->fd_access;
	ctlv_ops_free(ops);
	ops = NULL;

	if (fd_access == CTLV_OP_FD_ACCESS_READ_WRITE) {
		__u64 reopened_id;

		ctlvctl_spec_free(spec);
		spec = NULL;
		ctlv_close(dev);
		dev = NULL;
		ret = dev_open(argv[0], O_RDWR, &dev);
		if (ret)
			return ret;
		ret = spec_load(dev, &spec);
		if (ret)
			goto out_close;
		ret = op_resolve(spec, argv[1], &reopened_id);
		if (ret)
			goto out_close;
		if (reopened_id != op_id) {
			ret = -ENODEV;
			goto out_close;
		}
		name = ctlvctl_spec_op_id(spec, op_id);
		ret = ctlv_ops_dump(dev, &ops, &error);
		if (ret) {
			fprintf(stderr, "%s: ops-dump: %s\n", prog,
				ctlv_error_str(&error, text, sizeof(text)));
			goto out_close;
		}
		ret = run_op_info(dev, spec, ops, op_id, argv[1], query,
				  &info);
		if (ret)
			goto out_free_ops;
		if (info->fd_access != fd_access) {
			ret = -ENODEV;
			goto out_free_ops;
		}
		ctlv_ops_free(ops);
		ops = NULL;
	}

	/* The schema first: it says how to read values and print the reply. */
	ret = ctlv_op_schema_get(dev, op_id, &schema, &error);
	if (ret) {
		fprintf(stderr, "%s: op-schema-get: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out_close;
	}
	ret = tree_names_check(spec, ctlv_schema_request(schema));
	if (!ret)
		ret = tree_names_check(spec, ctlv_schema_reply(schema));
	if (ret)
		goto out_free_schema;

	request_tree = ctlv_schema_request(schema);
	if ((argc > 2 || json_source) && !request_tree) {
		fprintf(stderr, "%s: this op takes no request\n", prog);
		ret = -EINVAL;
		goto out_free_schema;
	}
	if (request_tree) {
		ret = ctlv_msg_new(REQUEST_HINT, &request);
		if (ret)
			goto out_free_schema;
		if (json_source)
			ret = json_request_build(request, request_tree, spec,
						 json_source);
		else
			ret = request_build(request, request_tree, spec,
					    argc - 2, argv + 2);
		if (ret)
			goto out_free_request;
	}

	if (query && reply_len)
		ret = ctlv_query_sized(dev, op_id, request, reply_len, &reply,
				       &error);
	else if (query)
		ret = ctlv_query(dev, op_id, request, &reply, &error);
	else
		ret = ctlv_action(dev, op_id, request, &reply, &error);
	if (ret) {
		fprintf(stderr, "%s: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out_free_request;
	}

	if (options.output_json) {
#ifdef HAVE_JSON_C
		struct json_object *output = json_object_new_object();

		if (!output) {
			ret = -ENOMEM;
			goto out_free_reply;
		}
		json_add_string(output, "device", ctlv_dev_name(dev));
		json_add_op(output, spec, op_id);
		if (!reply) {
			json_add_bool(output, "acknowledged", true);
		} else {
			struct json_object *body;

			json_add_u64(output, "generation",
				     ctlv_reply_generation(reply));
			json_add_int(output, "fragments",
				     ctlv_reply_fragments(reply));
			body = json_reply(ctlv_reply_root(reply),
					  ctlv_schema_reply(schema), spec);
			if (!body) {
				json_object_put(output);
				ret = -ENOMEM;
				goto out_free_reply;
			}
			json_object_object_add(output, "reply", body);
		}
		ret = json_print(output);
#endif
	} else if (!reply) {
		printf("# acknowledged\n");
	} else {
		printf("# ");
		if (name) {
			printf("%s", name->name);
			if (options.show_ids)
				printf(" (%#018llx)",
				       (unsigned long long)op_id);
			printf(", ");
		}
		printf("%u fragment%s, generation %llu\n",
		       ctlv_reply_fragments(reply),
		       ctlv_reply_fragments(reply) == 1 ? "" : "s",
		       (unsigned long long)ctlv_reply_generation(reply));
		reply_dump(ctlv_reply_root(reply), ctlv_schema_reply(schema),
			   spec);
	}
#ifdef HAVE_JSON_C
out_free_reply:
#endif
	ctlv_reply_free(reply);
out_free_request:
	ctlv_msg_free(request);
out_free_schema:
	ctlv_schema_free(schema);
out_close:
	ctlvctl_spec_free(spec);
	ctlv_close(dev);
	return ret;
out_free_ops:
	ctlv_ops_free(ops);
	goto out_close;
}

/*
 * A record names only the op it belongs to, so reading its payload means
 * asking the device what that op answers with. The answer is cached per op.
 */

struct schema_entry {
	__u64 op_id;
	/* NULL for an op whose schema could not be had, so as not to re-ask. */
	struct ctlv_schema *schema;
	int error;
};

struct schema_cache {
	struct schema_entry *entries;
	unsigned int entries_count;
};

static int cache_reply(struct schema_cache *cache, struct ctlv_dev *dev,
		       const struct ctlvctl_spec *spec, __u64 op_id,
		       const struct ctlv_node_schema **tree)
{
	struct schema_entry *entry;
	struct ctlv_error error;
	int ret;
	unsigned int i;

	for (i = 0; i < cache->entries_count; i++) {
		if (cache->entries[i].op_id != op_id)
			continue;
		*tree = cache->entries[i].schema ?
			ctlv_schema_reply(cache->entries[i].schema) : NULL;
		return cache->entries[i].error;
	}

	entry = realloc(cache->entries,
			(cache->entries_count + 1) * sizeof(*entry));
	if (!entry)
		return -ENOMEM;
	cache->entries = entry;
	entry += cache->entries_count++;
	entry->op_id = op_id;
	entry->schema = NULL;
	ret = ctlv_op_schema_get(dev, op_id, &entry->schema, &error);
	if (ret && !spec) {
		ctlv_schema_free(entry->schema);
		entry->schema = NULL;
		ret = 0;
	}
	if (!ret)
		ret = tree_names_check(spec, entry->schema ?
				       ctlv_schema_reply(entry->schema) : NULL);
	if (ret) {
		ctlv_schema_free(entry->schema);
		entry->schema = NULL;
	}
	entry->error = ret;
	*tree = entry->schema ? ctlv_schema_reply(entry->schema) : NULL;
	return ret;
}

static void cache_free(struct schema_cache *cache)
{
	unsigned int i;

	for (i = 0; i < cache->entries_count; i++)
		ctlv_schema_free(cache->entries[i].schema);
	free(cache->entries);
}

static int monitor_subscribe(struct ctlv_dev *dev,
			     const struct ctlvctl_spec *spec, int argc,
			     char **argv, struct ctlv_error *error)
{
	struct ctlv_ops *ops;
	__u64 *op_ids;
	unsigned int op_ids_count = 0, i;
	int ret;

	if (!spec && argc) {
		op_ids = calloc(argc, sizeof(*op_ids));
		if (!op_ids)
			return -ENOMEM;
		for (i = 0; i < (unsigned int)argc; i++) {
			if (parse_u64(argv[i], &op_ids[op_ids_count]))
				fprintf(stderr, "%s: %s: not an op ID\n",
					prog, argv[i]);
			else
				op_ids_count++;
		}
		ret = op_ids_count ?
			ctlv_subscribe(dev, op_ids, op_ids_count, error) :
			-ENOENT;
		free(op_ids);
		return ret;
	}

	ret = ctlv_ops_dump(dev, &ops, error);
	if (ret)
		return ret;
	for (i = 0; i < ops->ops_count; i++) {
		const struct ctlvctl_op_name *name;

		name = ctlvctl_spec_op_id(spec, ops->ops[i].op_id);
		if (name && !op_matches(name, &ops->ops[i])) {
			fprintf(stderr,
				"%s: specification disagrees with operation %s\n",
				prog, name->name);
			ret = -ENOMSG;
			goto out_ops;
		}
	}
	if (argc) {
		op_ids = calloc(argc, sizeof(*op_ids));
		if (!op_ids) {
			ret = -ENOMEM;
			goto out_ops;
		}
		for (i = 0; i < (unsigned int)argc; i++) {
			const struct ctlv_op_info *info;
			__u64 op_id;

			ret = op_resolve(spec, argv[i], &op_id);
			if (ret)
				goto out_ids;
			info = ctlv_ops_find(ops, op_id);
			if (!info || info->type != CTLV_OP_TYPE_EVENT) {
				fprintf(stderr, "%s: %s is not an event\n",
					prog, argv[i]);
				ret = -EINVAL;
				goto out_ids;
			}
			op_ids[op_ids_count++] = op_id;
		}
	} else {
		/* Every event the device reports, when none were named. */
		op_ids = calloc(ops->ops_count ? ops->ops_count : 1,
				sizeof(*op_ids));
		if (!op_ids) {
			ret = -ENOMEM;
			goto out_ops;
		}
		for (i = 0; i < ops->ops_count; i++)
			if (ops->ops[i].type == CTLV_OP_TYPE_EVENT)
				op_ids[op_ids_count++] = ops->ops[i].op_id;
	}

	if (!op_ids_count) {
		fprintf(stderr, "%s: device publishes no matching events\n",
			prog);
		ret = -ENOENT;
	} else {
		ret = ctlv_subscribe(dev, op_ids, op_ids_count, error);
	}
out_ids:
	free(op_ids);
out_ops:
	ctlv_ops_free(ops);
	return ret;
}

static int cmd_monitor(int argc, char **argv)
{
	struct schema_cache cache = { NULL, 0 };
	struct ctlvctl_spec *spec = NULL;
	__u64 queue_len = 0, len, used;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	char text[LINE_LEN];
	int ret;

	if (argc > 1 && !strcmp(argv[0], "-q")) {
		if (parse_u64(argv[1], &queue_len) || !queue_len) {
			fprintf(stderr, "%s: %s: not a queue length\n", prog,
				argv[1]);
			return -EINVAL;
		}
		argc -= 2;
		argv += 2;
	}

	if (argc < 1) {
		help(stderr);
		return -EINVAL;
	}
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;
	ret = spec_load(dev, &spec);
	if (ret)
		goto out;

	/* Resized before subscribing, when the queue holds nobody's records. */
	if (queue_len) {
		ret = ctlv_queue_set(dev, queue_len, &error);
		if (ret) {
			fprintf(stderr, "%s: event-queue-set: %s\n", prog,
				ctlv_error_str(&error, text, sizeof(text)));
			goto out;
		}
	}
	if (!ctlv_queue_get(dev, &len, &used, &error) &&
	    !options.output_json)
		printf("# queue %llu bytes, %llu used\n",
		       (unsigned long long)len, (unsigned long long)used);

	memset(&error, 0, sizeof(error));
	ret = monitor_subscribe(dev, spec, argc - 1, argv + 1, &error);
	if (ret) {
		if (error.err)
			fprintf(stderr, "%s: subscriptions-add: %s\n", prog,
				ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}

	/*
	 * Until the device goes away, which is -ENODEV once the records it
	 * queued before that have been read.
	 */
	for (;;) {
		const struct ctlv_node_schema *tree;
		struct ctlv_event *event;
		__u64 op_id;

		ret = ctlv_event_read(dev, &event);
		if (ret == -EAGAIN) {
			ret = ctlv_event_wait(dev, -1);
			if (!ret)
				continue;
		}
		if (ret) {
			if (ret != -ENODEV)
				fprintf(stderr, "%s: read: %s\n", prog,
					strerror(-ret));
			break;
		}
		op_id = ctlv_event_op_id(event);
		ret = cache_reply(&cache, dev, spec, op_id, &tree);
		if (ret) {
			fprintf(stderr, "%s: cannot read event schema: %s\n",
				prog, strerror(-ret));
			ctlv_event_free(event);
			break;
		}
		if (options.output_json) {
#ifdef HAVE_JSON_C
			struct json_object *output = json_object_new_object();
			struct json_object *record;

			if (!output) {
				ctlv_event_free(event);
				ret = -ENOMEM;
				break;
			}
			json_add_string(output, "device",
					ctlv_dev_name(dev));
			json_add_op(output, spec, op_id);
			json_add_u64(output, "sequence",
				     ctlv_event_sequence(event));
			record = json_reply(ctlv_event_root(event), tree, spec);
			if (!record) {
				json_object_put(output);
				ctlv_event_free(event);
				ret = -ENOMEM;
				break;
			}
			json_object_object_add(output, "record", record);
			ret = json_print(output);
			ctlv_event_free(event);
			if (ret)
				break;
			continue;
#endif
		}
		if (ctlvctl_spec_op_id(spec, op_id)) {
			printf("%s", ctlvctl_spec_op_id(spec, op_id)->name);
			if (options.show_ids)
				printf(" (%#018llx)",
				       (unsigned long long)op_id);
		} else {
			printf("%#018llx", (unsigned long long)op_id);
		}
		printf("\tsequence %llu\n", (unsigned long long)
		       ctlv_event_sequence(event));
		reply_dump(ctlv_event_root(event), tree, spec);
		ctlv_event_free(event);
	}
out:
	cache_free(&cache);
	ctlvctl_spec_free(spec);
	ctlv_close(dev);
	return ret == -ENODEV ? 0 : ret;
}

int main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "spec-dir", required_argument, NULL, 1 },
		{ "show-ids", no_argument, NULL, 'n' },
		{ "output-json", no_argument, NULL, 2 },
		{ "help", no_argument, NULL, 'h' },
		{},
	};
	const char *command;
	const char *spec_dir;
	int opt;

	/* Line buffered, monitoring ending by a kill with a pipe to flush. */
	setvbuf(stdout, NULL, _IOLBF, 0);

	if (argc > 0)
		prog = argv[0];
	while ((opt = getopt_long(argc, argv, "+nh", long_options, NULL)) != -1) {
		switch (opt) {
		case 1:
			options.spec_dir = optarg;
			break;
		case 2:
			options.output_json = true;
			break;
		case 'n':
			options.show_ids = true;
			break;
		case 'h':
			help(stdout);
			return EXIT_SUCCESS;
		default:
			help(stderr);
			return EXIT_FAILURE;
		}
	}
	if (!options.spec_dir) {
		spec_dir = getenv("CTLV_SPEC_DIR");
		if (spec_dir && *spec_dir)
			options.spec_dir = spec_dir;
	}
#ifndef HAVE_JSON_C
	if (options.spec_dir || options.output_json) {
		fprintf(stderr,
			"%s: specification and JSON support require json-c\n",
			prog);
		return EXIT_FAILURE;
	}
#endif
	if (optind == argc) {
		help(stderr);
		return EXIT_FAILURE;
	}
	command = argv[optind++];
	argc -= optind;
	argv += optind;

	if (!strcmp(command, "list"))
		return cmd_list(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (!strcmp(command, "ops"))
		return cmd_ops(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (!strcmp(command, "schema"))
		return cmd_schema(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (!strcmp(command, "exec"))
		return cmd_run(argc, argv, false) ? EXIT_FAILURE :
						    EXIT_SUCCESS;
	if (!strcmp(command, "query"))
		return cmd_run(argc, argv, true) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (!strcmp(command, "monitor"))
		return cmd_monitor(argc, argv) ? EXIT_FAILURE : EXIT_SUCCESS;
	if (!strcmp(command, "help") || !strcmp(command, "--help")) {
		help(stdout);
		return EXIT_SUCCESS;
	}
	fprintf(stderr, "%s: %s: no such command\n", prog, command);
	help(stderr);
	return EXIT_FAILURE;
}
