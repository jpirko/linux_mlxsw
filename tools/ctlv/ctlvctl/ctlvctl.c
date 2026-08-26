// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * ctlvctl: talk to any CTLV device, including one this program has never heard
 * of. There is no family-specific code here: a device says which families it
 * answers, which ops those publish and what each op's messages look like, and
 * everything below is written against those three answers. What it cannot do
 * is print a name for an attr, names not being on the wire.
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ctlv.h>

#define DEV_DIR "/dev"
#define PATH_LEN 1024
#define LINE_LEN 512

/* How deep a request built from the command line may nest. */
#define PATH_DEPTH 16

/* Where a request buffer starts; it grows as attrs are added. */
#define REQUEST_HINT 512

static const char *prog = "ctlvctl";

static void help(FILE *out)
{
	fprintf(out,
		"usage: %s <command> [arguments]\n"
		"\n"
		"  list                       every CTLV device and its family chain\n"
		"  ops <device>               every op the device publishes\n"
		"  schema <device> <op>       the request and reply ABI of one op\n"
		"  exec <device> <op> [a=v]   run an action\n"
		"  query [-l len] <device> <op> [a=v]\n"
		"                             run a query, reassembling the answer\n"
		"  monitor [-q len] <device> [op ...]\n"
		"                             subscribe and print records as they arrive\n"
		"\n"
		"-l names the reply buffer, which is what decides how many fragments an\n"
		"answer arrives in; the device reports its smallest usable size in ops.\n"
		"-q resizes the event queue before subscribing, which is how a reader\n"
		"decides how far behind it may fall before records are dropped.\n"
		"\n"
		"An op is a complete op ID, and an assignment names an attr by its\n"
		"complete attr ID. A dotted path enters a nest, and assignments are\n"
		"given in the order the message holds them:\n"
		"\n"
		"  %s exec ctlv0 0x1000000002 0x1000100000001=7\n"
		"  %s query ctlv0 0x2 0x1000400000001.0x1000400000002=1\n",
		prog, prog, prog);
}

static int parse_u64(const char *text, __u64 *value)
{
	char *end;

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

static void chain_print(const struct ctlv_dev *dev)
{
	unsigned int i;

	for (i = 0; i < ctlv_chain_len(dev); i++)
		printf("%s%s", i ? "/" : "", ctlv_chain_name(dev, i));
}

static int cmd_list(int argc, char **argv)
{
	struct ctlv_dev_list list;
	unsigned int i;
	int ret;

	ret = ctlv_enumerate(&list);
	if (ret) {
		fprintf(stderr, "%s: no CTLV devices: %s\n", prog,
			strerror(-ret));
		return ret;
	}
	for (i = 0; i < list.n; i++) {
		struct ctlv_dev *dev;

		printf("%s", list.name[i]);
		/* Listing should not need the right to change a device. */
		if (!dev_open(list.name[i], O_RDONLY, &dev)) {
			printf("\t");
			chain_print(dev);
			ctlv_close(dev);
		}
		printf("\n");
	}
	ctlv_dev_list_free(&list);
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

static void op_print(const struct ctlv_op_info *op)
{
	printf("%#018" PRIx64 "\t%-6s", (uint64_t)op->op_id,
	       type_name(op->type));
	if (op->availability == CTLV_OP_AVAILABILITY_DYNAMIC)
		printf("\tdynamic %s", op->disabled ? "disabled" : "enabled");
	else if (op->availability == CTLV_OP_AVAILABILITY_FIXED)
		printf("\tfixed");
	if (op->request_max_len)
		printf("\trequest %" PRIu64, (uint64_t)op->request_max_len);
	if (op->action_reply_max_len)
		printf("\treply %" PRIu64,
		       (uint64_t)op->action_reply_max_len);
	if (op->query_reply_min_len)
		printf("\treply %" PRIu64 "..%" PRIu64,
		       (uint64_t)op->query_reply_min_len,
		       (uint64_t)op->query_reply_suggested_len);
	if (op->event_record_max_len)
		printf("\trecord %" PRIu64,
		       (uint64_t)op->event_record_max_len);
	printf("\n");
}

static int cmd_ops(int argc, char **argv)
{
	struct ctlv_error error;
	struct ctlv_dev *dev;
	struct ctlv_ops *ops;
	char text[LINE_LEN];
	unsigned int i;
	int ret;

	if (argc < 1) {
		help(stderr);
		return -EINVAL;
	}
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;

	ret = ctlv_ops_dump(dev, &ops, &error);
	if (ret) {
		fprintf(stderr, "%s: ops-dump: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}
	printf("# ");
	chain_print(dev);
	printf(", generation %" PRIu64 "\n", (uint64_t)ops->generation);
	for (i = 0; i < ops->n; i++)
		op_print(&ops->op[i]);
	ctlv_ops_free(ops);
out:
	ctlv_close(dev);
	return ret;
}

static void tree_print(const struct ctlv_schema_node *node,
		       unsigned int indent)
{
	char line[LINE_LEN];
	unsigned int i;

	ctlv_schema_render(node, indent, line, sizeof(line));
	printf("%s\n", line);
	for (i = 0; i < node->n_attrs; i++)
		tree_print(&node->attrs[i], indent + 1);
	for (i = 0; i < node->n_optional_attrs; i++)
		tree_print(&node->optional_attrs[i], indent + 1);
}

static int cmd_schema(int argc, char **argv)
{
	const struct ctlv_schema_node *node;
	struct ctlv_schema *schema;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	char text[LINE_LEN];
	__u64 op_id;
	int ret;

	if (argc < 2) {
		help(stderr);
		return -EINVAL;
	}
	ret = parse_u64(argv[1], &op_id);
	if (ret) {
		fprintf(stderr, "%s: %s: not an op ID\n", prog, argv[1]);
		return ret;
	}
	ret = dev_open(argv[0], O_RDONLY, &dev);
	if (ret)
		return ret;

	ret = ctlv_op_schema_get(dev, op_id, &schema, &error);
	if (ret) {
		fprintf(stderr, "%s: op-schema-get: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}
	node = ctlv_schema_request(schema);
	if (node) {
		printf("request:\n");
		tree_print(node, 1);
	}
	node = ctlv_schema_reply(schema);
	if (node) {
		printf("reply:\n");
		tree_print(node, 1);
	}
	ctlv_schema_free(schema);
out:
	ctlv_close(dev);
	return ret;
}

/*
 * An assignment names an attr by its complete ID and a dotted path enters a
 * nest. What a value means is decided by the kind the schema reports.
 */

/* The member of @parent for @attr_id, following any re-entry. */
static const struct ctlv_schema_node *
node_member(const struct ctlv_schema_node **stack, unsigned int depth,
	    __u64 attr_id)
{
	const struct ctlv_schema_node *parent = stack[depth];
	unsigned int i;

	if (!parent)
		return NULL;
	while (parent->reenters) {
		if (parent->reenters > depth)
			return NULL;
		parent = stack[depth - parent->reenters];
	}
	for (i = 0; i < parent->n_attrs; i++)
		if (parent->attrs[i].attr_id == attr_id)
			return &parent->attrs[i];
	for (i = 0; i < parent->n_optional_attrs; i++)
		if (parent->optional_attrs[i].attr_id == attr_id)
			return &parent->optional_attrs[i];
	return NULL;
}

static int put_value(struct ctlv_msg *msg, const struct ctlv_schema_node *node,
		     const char *text)
{
	__u64 value;
	__s64 signed_value;

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_ENUM:
	case CTLV_SCHEMA_FLAGS:
		if (parse_u64(text, &value))
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
		return ctlv_put_bin(msg, node->attr_id, text, strlen(text));
	default:
		/* A command line has no memory of the caller's to describe. */
		return -ENOTSUP;
	}
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
	fprintf(stderr, "%s: %#" PRIx64 ": not an attr of this nest\n", prog,
		(uint64_t)attr_id);
	return -ENOENT;
}

static int not_a_nest(__u64 attr_id)
{
	fprintf(stderr, "%s: %#" PRIx64 ": not a nest of this nest\n", prog,
		(uint64_t)attr_id);
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
			 const struct ctlv_schema_node *root, int argc,
			 char **argv)
{
	const struct ctlv_schema_node *stack[PATH_DEPTH] = { root };
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

		while ((name = strsep(&rest, "."))) {
			const struct ctlv_schema_node *node;
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
				ret = put_value(msg, node, text);
				if (ret) {
					fprintf(stderr,
						"%s: %#" PRIx64 "=%s: %s\n",
						prog, (uint64_t)attr_id, text,
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

static void payload_print(const struct ctlv_attr *attr,
			  const struct ctlv_schema_node *node)
{
	const void *bytes;
	const char *text;
	__u64 value;
	__s64 signed_value;
	bool flag;
	__u32 len, i;

	switch (node ? node->kind : CTLV_SCHEMA_BINARY) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_ENUM:
		if (!ctlv_get_u64(attr, &value))
			printf("%" PRIu64, (uint64_t)value);
		return;
	case CTLV_SCHEMA_FLAGS:
		if (!ctlv_get_u64(attr, &value))
			printf("%#" PRIx64, (uint64_t)value);
		return;
	case CTLV_SCHEMA_SINT:
		if (!ctlv_get_s64(attr, &signed_value))
			printf("%" PRId64, (int64_t)signed_value);
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
			const struct ctlv_schema_node **stack,
			unsigned int depth)
{
	const struct ctlv_attr *child;

	ctlv_for_each(child, nest) {
		const struct ctlv_schema_node *node = NULL;

		if (depth < PATH_DEPTH)
			node = node_member(stack, depth, child->attr_id);
		printf("%*s%#018" PRIx64 "%s", (depth + 1) * 2, "",
		       (uint64_t)child->attr_id, node ? "" : "?");
		if (ctlv_attr_is_nest(child)) {
			printf("\n");
			if (depth + 1 < PATH_DEPTH) {
				stack[depth + 1] = node;
				reply_print(child, stack, depth + 1);
			}
			continue;
		}
		printf(" ");
		payload_print(child, node);
		printf("\n");
	}
}

static void reply_dump(const struct ctlv_attr *root,
		       const struct ctlv_schema_node *tree)
{
	const struct ctlv_schema_node *stack[PATH_DEPTH] = { tree };

	reply_print(root, stack, 0);
}

static int cmd_run(int argc, char **argv, bool query)
{
	const struct ctlv_schema_node *request_tree;
	struct ctlv_msg *request = NULL;
	struct ctlv_reply *reply = NULL;
	struct ctlv_schema *schema;
	struct ctlv_error error;
	struct ctlv_dev *dev;
	char text[LINE_LEN];
	__u64 reply_len = 0;
	__u64 op_id;
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
	ret = parse_u64(argv[1], &op_id);
	if (ret) {
		fprintf(stderr, "%s: %s: not an op ID\n", prog, argv[1]);
		return ret;
	}
	ret = dev_open(argv[0], query ? O_RDONLY : O_RDWR, &dev);
	if (ret)
		return ret;

	/* The schema first: it says how to read values and print the reply. */
	ret = ctlv_op_schema_get(dev, op_id, &schema, &error);
	if (ret) {
		fprintf(stderr, "%s: op-schema-get: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out_close;
	}

	request_tree = ctlv_schema_request(schema);
	if (argc > 2 && !request_tree) {
		fprintf(stderr, "%s: this op takes no request\n", prog);
		ret = -EINVAL;
		goto out_free_schema;
	}
	if (request_tree) {
		ret = ctlv_msg_new(REQUEST_HINT, &request);
		if (ret)
			goto out_free_schema;
		ret = request_build(request, request_tree, argc - 2, argv + 2);
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

	if (!reply) {
		printf("# acknowledged\n");
	} else {
		printf("# %u fragment%s, generation %" PRIu64 "\n",
		       ctlv_reply_fragments(reply),
		       ctlv_reply_fragments(reply) == 1 ? "" : "s",
		       (uint64_t)ctlv_reply_generation(reply));
		reply_dump(ctlv_reply_root(reply), ctlv_schema_reply(schema));
		ctlv_reply_free(reply);
	}

out_free_request:
	ctlv_msg_free(request);
out_free_schema:
	ctlv_schema_free(schema);
out_close:
	ctlv_close(dev);
	return ret;
}

/*
 * A record names only the op it belongs to, so reading its payload means
 * asking the device what that op answers with. The answer is cached per op.
 */

struct schema_entry {
	__u64 op_id;
	/* NULL for an op whose schema could not be had, so as not to re-ask. */
	struct ctlv_schema *schema;
};

struct schema_cache {
	struct schema_entry *entry;
	unsigned int n;
};

static const struct ctlv_schema_node *
cache_reply(struct schema_cache *cache, struct ctlv_dev *dev, __u64 op_id)
{
	struct schema_entry *entry;
	struct ctlv_error error;
	unsigned int i;

	for (i = 0; i < cache->n; i++)
		if (cache->entry[i].op_id == op_id)
			return cache->entry[i].schema ?
			       ctlv_schema_reply(cache->entry[i].schema) : NULL;

	entry = realloc(cache->entry, (cache->n + 1) * sizeof(*entry));
	if (!entry)
		return NULL;
	cache->entry = entry;
	entry += cache->n++;
	entry->op_id = op_id;
	if (ctlv_op_schema_get(dev, op_id, &entry->schema, &error))
		entry->schema = NULL;
	return entry->schema ? ctlv_schema_reply(entry->schema) : NULL;
}

static void cache_free(struct schema_cache *cache)
{
	unsigned int i;

	for (i = 0; i < cache->n; i++)
		ctlv_schema_free(cache->entry[i].schema);
	free(cache->entry);
}

static int monitor_subscribe(struct ctlv_dev *dev, int argc, char **argv,
			     struct ctlv_error *error)
{
	struct ctlv_ops *ops;
	__u64 *op_ids;
	unsigned int n = 0, i;
	int ret;

	if (argc) {
		op_ids = calloc(argc, sizeof(*op_ids));
		if (!op_ids)
			return -ENOMEM;
		for (i = 0; i < (unsigned int)argc; i++)
			if (parse_u64(argv[i], &op_ids[n]))
				fprintf(stderr, "%s: %s: not an op ID\n", prog,
					argv[i]);
			else
				n++;
	} else {
		/* Every event the device reports, when none were named. */
		ret = ctlv_ops_dump(dev, &ops, error);
		if (ret)
			return ret;
		op_ids = calloc(ops->n ? ops->n : 1, sizeof(*op_ids));
		if (!op_ids) {
			ctlv_ops_free(ops);
			return -ENOMEM;
		}
		for (i = 0; i < ops->n; i++)
			if (ops->op[i].type == CTLV_OP_TYPE_EVENT)
				op_ids[n++] = ops->op[i].op_id;
		ctlv_ops_free(ops);
	}

	ret = n ? ctlv_subscribe(dev, op_ids, n, error) : -ENOENT;
	free(op_ids);
	return ret;
}

static int cmd_monitor(int argc, char **argv)
{
	struct schema_cache cache = { NULL, 0 };
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
	ret = dev_open(argv[0], O_RDWR, &dev);
	if (ret)
		return ret;

	/* Resized before subscribing, when the queue holds nobody's records. */
	if (queue_len) {
		ret = ctlv_queue_set(dev, queue_len, &error);
		if (ret) {
			fprintf(stderr, "%s: event-queue-set: %s\n", prog,
				ctlv_error_str(&error, text, sizeof(text)));
			goto out;
		}
	}
	if (!ctlv_queue_get(dev, &len, &used, &error))
		printf("# queue %" PRIu64 " bytes, %" PRIu64 " used\n",
		       (uint64_t)len, (uint64_t)used);

	ret = monitor_subscribe(dev, argc - 1, argv + 1, &error);
	if (ret) {
		fprintf(stderr, "%s: subscriptions-add: %s\n", prog,
			ctlv_error_str(&error, text, sizeof(text)));
		goto out;
	}

	/*
	 * Until the device goes away, which is -ENODEV once the records it
	 * queued before that have been read.
	 */
	for (;;) {
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
		printf("%#018" PRIx64 "\tsequence %" PRIu64 "\n",
		       (uint64_t)op_id,
		       (uint64_t)ctlv_event_sequence(event));
		reply_dump(ctlv_event_root(event),
			   cache_reply(&cache, dev, op_id));
		ctlv_event_free(event);
	}
out:
	cache_free(&cache);
	ctlv_close(dev);
	return ret == -ENODEV ? 0 : ret;
}

int main(int argc, char **argv)
{
	const char *command;

	/* Line buffered, monitoring ending by a kill with a pipe to flush. */
	setvbuf(stdout, NULL, _IOLBF, 0);

	if (argc > 0)
		prog = argv[0];
	if (argc < 2) {
		help(stderr);
		return EXIT_FAILURE;
	}
	command = argv[1];
	argc -= 2;
	argv += 2;

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
