// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <ctlv.h>

#include "spec.h"

#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

struct ctlvctl_spec {
	struct ctlvctl_definition *definitions;
	unsigned int definitions_count;
	struct ctlvctl_attr_name *attrs;
	unsigned int attrs_count;
	struct ctlvctl_op_name *ops;
	unsigned int ops_count;
};

#ifdef HAVE_JSON_C

#include <json.h>

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

static int json_member(struct json_object *parent, const char *name,
		       enum json_type type, struct json_object **value)
{
	if (!json_object_object_get_ex(parent, name, value) ||
	    !json_object_is_type(*value, type))
		return -EINVAL;
	return 0;
}

static int json_string_dup(struct json_object *parent, const char *name,
			   char **value)
{
	struct json_object *item;

	if (json_member(parent, name, json_type_string, &item))
		return -EINVAL;
	*value = strdup(json_object_get_string(item));
	return *value ? 0 : -ENOMEM;
}

static int json_u64(struct json_object *parent, const char *name, __u64 *value)
{
	struct json_object *item;

	if (!json_object_object_get_ex(parent, name, &item))
		return -EINVAL;
	if (json_object_is_type(item, json_type_string))
		return parse_u64(json_object_get_string(item), value);
	if (!json_object_is_type(item, json_type_int))
		return -EINVAL;
	*value = json_object_get_uint64(item);
	return 0;
}

static int kind_parse(const char *name, __u32 *kind)
{
	static const char *const names[] = {
		[CTLV_SCHEMA_UINT] = "uint",
		[CTLV_SCHEMA_SINT] = "sint",
		[CTLV_SCHEMA_BOOL] = "bool",
		[CTLV_SCHEMA_STRING] = "string",
		[CTLV_SCHEMA_BINARY] = "binary",
		[CTLV_SCHEMA_ENUM] = "enum",
		[CTLV_SCHEMA_FLAGS] = "flags",
		[CTLV_SCHEMA_NEST] = "nested",
		[CTLV_SCHEMA_BLOB] = "blob",
	};
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(names); i++)
		if (names[i] && !strcmp(names[i], name)) {
			*kind = i;
			return 0;
		}
	return -EINVAL;
}

static int op_type_parse(const char *name, __u32 *type)
{
	if (!strcmp(name, "action"))
		*type = CTLV_OP_TYPE_ACTION;
	else if (!strcmp(name, "query"))
		*type = CTLV_OP_TYPE_QUERY;
	else if (!strcmp(name, "event"))
		*type = CTLV_OP_TYPE_EVENT;
	else
		return -EINVAL;
	return 0;
}

static int availability_parse(const char *name, __u32 *availability)
{
	if (!strcmp(name, "fixed"))
		*availability = CTLV_OP_AVAILABILITY_FIXED;
	else if (!strcmp(name, "dynamic"))
		*availability = CTLV_OP_AVAILABILITY_DYNAMIC;
	else
		return -EINVAL;
	return 0;
}

static int fd_access_parse(const char *name, __u32 *fd_access)
{
	if (!strcmp(name, "read"))
		*fd_access = CTLV_OP_FD_ACCESS_READ;
	else if (!strcmp(name, "read-write"))
		*fd_access = CTLV_OP_FD_ACCESS_READ_WRITE;
	else
		return -EINVAL;
	return 0;
}

static struct ctlvctl_definition *
definition_find(struct ctlvctl_spec *spec, const char *name)
{
	unsigned int i;

	for (i = 0; i < spec->definitions_count; i++)
		if (!strcmp(spec->definitions[i].name, name))
			return &spec->definitions[i];
	return NULL;
}

static int definition_parse(struct json_object *json,
			    struct ctlvctl_definition *definition)
{
	struct json_object *kind, *values;
	size_t i, n;
	int ret;

	ret = json_string_dup(json, "name", &definition->name);
	if (ret)
		return ret;
	if (json_member(json, "kind", json_type_string, &kind) ||
	    kind_parse(json_object_get_string(kind), &definition->kind))
		return -EINVAL;
	if (definition->kind != CTLV_SCHEMA_ENUM &&
	    definition->kind != CTLV_SCHEMA_FLAGS)
		return -EINVAL;
	if (json_member(json, "values", json_type_array, &values))
		return -EINVAL;

	n = json_object_array_length(values);
	definition->values = calloc(n, sizeof(*definition->values));
	if (n && !definition->values)
		return -ENOMEM;
	definition->values_count = n;
	for (i = 0; i < n; i++) {
		struct json_object *item = json_object_array_get_idx(values, i);

		if (!item ||
		    json_string_dup(item, "name",
				    &definition->values[i].name) ||
		    json_u64(item, "value", &definition->values[i].value))
			return -EINVAL;
	}
	return 0;
}

static int definitions_parse(struct json_object *root,
			     struct ctlvctl_spec *spec)
{
	struct json_object *items;
	size_t i, n;
	int ret;

	if (json_member(root, "definitions", json_type_array, &items))
		return -EINVAL;
	n = json_object_array_length(items);
	spec->definitions = calloc(n, sizeof(*spec->definitions));
	if (n && !spec->definitions)
		return -ENOMEM;
	spec->definitions_count = n;
	for (i = 0; i < n; i++) {
		ret = definition_parse(json_object_array_get_idx(items, i),
				       &spec->definitions[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static int attr_parse(struct json_object *json, struct ctlvctl_spec *spec,
		      struct ctlvctl_attr_name *attr)
{
	struct json_object *item;
	const char *definition;
	int ret;

	ret = json_string_dup(json, "name", &attr->name);
	if (ret)
		return ret;
	ret = json_string_dup(json, "short", &attr->short_name);
	if (ret)
		return ret;
	ret = json_u64(json, "id", &attr->id);
	if (ret)
		return ret;
	if (json_member(json, "kind", json_type_string, &item) ||
	    kind_parse(json_object_get_string(item), &attr->kind))
		return -EINVAL;

	if (!json_object_object_get_ex(json, "definition", &item))
		return 0;
	if (!json_object_is_type(item, json_type_string))
		return -EINVAL;
	definition = json_object_get_string(item);
	attr->definition = definition_find(spec, definition);
	return attr->definition ? 0 : -EINVAL;
}

static int attrs_parse(struct json_object *root, struct ctlvctl_spec *spec)
{
	struct json_object *items;
	size_t i, n;
	int ret;

	if (json_member(root, "attrs", json_type_array, &items))
		return -EINVAL;
	n = json_object_array_length(items);
	spec->attrs = calloc(n, sizeof(*spec->attrs));
	if (n && !spec->attrs)
		return -ENOMEM;
	spec->attrs_count = n;
	for (i = 0; i < n; i++) {
		ret = attr_parse(json_object_array_get_idx(items, i),
				 spec, &spec->attrs[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static int op_parse(struct json_object *json, struct ctlvctl_op_name *op)
{
	struct json_object *item;
	int ret;

	ret = json_string_dup(json, "name", &op->name);
	if (ret)
		return ret;
	ret = json_u64(json, "id", &op->id);
	if (ret)
		return ret;
	if (json_member(json, "type", json_type_string, &item) ||
	    op_type_parse(json_object_get_string(item), &op->type))
		return -EINVAL;
	if (op->type != CTLV_OP_TYPE_EVENT) {
		if (json_member(json, "fd-access", json_type_string, &item) ||
		    fd_access_parse(json_object_get_string(item),
				    &op->fd_access) ||
		    json_member(json, "availability", json_type_string, &item) ||
		    availability_parse(json_object_get_string(item),
				       &op->availability))
			return -EINVAL;
	}
	if (json_u64(json, "request-max-len", &op->request_max_len) ||
	    json_u64(json, "action-reply-max-len",
		     &op->action_reply_max_len) ||
	    json_u64(json, "query-reply-min-len",
		     &op->query_reply_min_len) ||
	    json_u64(json, "query-reply-suggested-len",
		     &op->query_reply_suggested_len) ||
	    json_u64(json, "event-record-max-len",
		     &op->event_record_max_len))
		return -EINVAL;
	return 0;
}

static int ops_parse(struct json_object *root, struct ctlvctl_spec *spec)
{
	struct json_object *items;
	size_t i, n;
	int ret;

	if (json_member(root, "ops", json_type_array, &items))
		return -EINVAL;
	n = json_object_array_length(items);
	spec->ops = calloc(n, sizeof(*spec->ops));
	if (n && !spec->ops)
		return -ENOMEM;
	spec->ops_count = n;
	for (i = 0; i < n; i++) {
		ret = op_parse(json_object_array_get_idx(items, i), &spec->ops[i]);
		if (ret)
			return ret;
	}
	return 0;
}

static int chain_check(struct json_object *root, const struct ctlv_dev *dev)
{
	struct json_object *family, *chain;
	size_t i, n;

	if (json_member(root, "family", json_type_string, &family) ||
	    strcmp(json_object_get_string(family), ctlv_dev_family(dev)))
		return -ENOMSG;
	if (json_member(root, "chain", json_type_array, &chain))
		return -EINVAL;
	n = json_object_array_length(chain);
	if (n != ctlv_chain_len(dev))
		return -ENOMSG;

	for (i = 0; i < n; i++) {
		struct json_object *item, *name;
		__u64 id;

		item = json_object_array_get_idx(chain, i);
		if (!item ||
		    json_member(item, "name", json_type_string, &name) ||
		    json_u64(item, "id", &id))
			return -EINVAL;
		if (id != ctlv_chain_id(dev, i) ||
		    strcmp(json_object_get_string(name),
			   ctlv_chain_name(dev, i)))
			return -ENOMSG;
	}
	return 0;
}

static int manifest_parse(const char *prog, const char *text,
			  const struct ctlv_dev *dev,
			  struct ctlvctl_spec **specp)
{
	struct json_tokener *tokener;
	struct json_object *root;
	struct ctlvctl_spec *spec;
	enum json_tokener_error error;
	size_t text_len;
	__u64 version;
	int ret;

	if (!text)
		return -EINVAL;
	text_len = strlen(text);
	if (text_len >= INT_MAX)
		return -E2BIG;
	tokener = json_tokener_new();
	if (!tokener)
		return -ENOMEM;
	json_tokener_set_flags(tokener, JSON_TOKENER_STRICT |
			       JSON_TOKENER_VALIDATE_UTF8);
	root = json_tokener_parse_ex(tokener, text, text_len + 1);
	error = json_tokener_get_error(tokener);
	json_tokener_free(tokener);
	if (error != json_tokener_success || !root) {
		fprintf(stderr, "%s: invalid ctlvgen manifest: %s\n", prog,
			json_tokener_error_desc(error));
		return -EINVAL;
	}
	if (!json_object_is_type(root, json_type_object) ||
	    json_u64(root, "version", &version) || version != 1) {
		fprintf(stderr, "%s: unsupported ctlvgen manifest\n", prog);
		ret = -EINVAL;
		goto out_root;
	}

	spec = calloc(1, sizeof(*spec));
	if (!spec) {
		ret = -ENOMEM;
		goto out_root;
	}
	ret = chain_check(root, dev);
	if (!ret)
		ret = definitions_parse(root, spec);
	if (!ret)
		ret = attrs_parse(root, spec);
	if (!ret)
		ret = ops_parse(root, spec);
	if (ret) {
		if (ret == -ENOMSG)
			fprintf(stderr,
				"%s: specifications do not match device %s\n",
				prog, ctlv_dev_name(dev));
		else
			fprintf(stderr, "%s: malformed ctlvgen manifest\n",
				prog);
		ctlvctl_spec_free(spec);
		goto out_root;
	}
	*specp = spec;
out_root:
	json_object_put(root);
	return ret;
}

static int read_all(int fd, char **textp)
{
	size_t used = 0, size = 4096;
	char *text;

	text = malloc(size);
	if (!text)
		return -ENOMEM;
	for (;;) {
		ssize_t n;

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
		n = read(fd, text + used, size - used - 1);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			free(text);
			return -errno;
		}
		if (!n)
			break;
		used += n;
	}
	text[used] = '\0';
	*textp = text;
	return 0;
}

static void generator_exec(const char *prog, const char *spec_path)
{
	const char *override = getenv("CTLVGEN");
	char executable[PATH_MAX];
	char candidate[PATH_MAX];
	char *slash;
	ssize_t n;

	if (override)
		execl(override, override, "--spec", spec_path,
		      "--runtime-json", NULL);

	n = readlink("/proc/self/exe", executable, sizeof(executable) - 1);
	if (n > 0) {
		static const char suffix[] = "/../ctlvgen/cli.py";
		size_t len;

		executable[n] = '\0';
		slash = strrchr(executable, '/');
		if (slash) {
			*slash = '\0';
			len = strlen(executable);
			if (len + sizeof(suffix) <= sizeof(candidate)) {
				memcpy(candidate, executable, len);
				memcpy(candidate + len, suffix, sizeof(suffix));
				execl(candidate, candidate, "--spec",
				      spec_path, "--runtime-json", NULL);
			}
		}
	}
	execlp("ctlvgen", "ctlvgen", "--spec", spec_path,
	       "--runtime-json", NULL);
	dprintf(STDERR_FILENO, "%s: cannot execute ctlvgen: %s\n", prog,
		strerror(errno));
	_exit(127);
}

static int manifest_read(const char *prog, const char *spec_path, char **textp)
{
	int pipefd[2];
	int status;
	pid_t pid;
	int ret;

	if (pipe2(pipefd, O_CLOEXEC))
		return -errno;
	pid = fork();
	if (pid < 0) {
		ret = -errno;
		goto out_pipe;
	}
	if (!pid) {
		close(pipefd[0]);
		if (pipefd[1] == STDOUT_FILENO) {
			int flags = fcntl(STDOUT_FILENO, F_GETFD);

			if (flags < 0 ||
			    fcntl(STDOUT_FILENO, F_SETFD,
				  flags & ~FD_CLOEXEC) < 0)
				_exit(127);
		} else {
			if (dup2(pipefd[1], STDOUT_FILENO) < 0)
				_exit(127);
			close(pipefd[1]);
		}
		generator_exec(prog, spec_path);
	}
	close(pipefd[1]);
	ret = read_all(pipefd[0], textp);
	close(pipefd[0]);
	while (waitpid(pid, &status, 0) < 0) {
		if (errno != EINTR) {
			ret = -errno;
			goto out_text;
		}
	}
	if (!ret && (!WIFEXITED(status) || WEXITSTATUS(status)))
		ret = -EINVAL;
out_text:
	if (ret) {
		free(*textp);
		*textp = NULL;
	}
	return ret;
out_pipe:
	close(pipefd[0]);
	close(pipefd[1]);
	return ret;
}

int ctlvctl_spec_load(const char *prog, const char *dir,
		      const struct ctlv_dev *dev, struct ctlvctl_spec **specp)
{
	char spec_path[PATH_MAX];
	char *text = NULL;
	int ret;

	if (snprintf(spec_path, sizeof(spec_path), "%s/%s.yaml", dir,
		     ctlv_dev_family(dev)) >= (int)sizeof(spec_path))
		return -ENAMETOOLONG;
	ret = manifest_read(prog, spec_path, &text);
	if (ret)
		return ret;
	ret = manifest_parse(prog, text, dev, specp);
	free(text);
	return ret;
}

#else /* HAVE_JSON_C */

int ctlvctl_spec_load(const char *prog, const char *dir,
		      const struct ctlv_dev *dev, struct ctlvctl_spec **specp)
{
	fprintf(stderr,
		"%s: specification support was built without json-c\n", prog);
	return -EOPNOTSUPP;
}

#endif /* HAVE_JSON_C */

void ctlvctl_spec_free(struct ctlvctl_spec *spec)
{
	unsigned int i, j;

	if (!spec)
		return;
	for (i = 0; i < spec->definitions_count; i++) {
		for (j = 0; j < spec->definitions[i].values_count; j++)
			free(spec->definitions[i].values[j].name);
		free(spec->definitions[i].values);
		free(spec->definitions[i].name);
	}
	for (i = 0; i < spec->attrs_count; i++) {
		free(spec->attrs[i].name);
		free(spec->attrs[i].short_name);
	}
	for (i = 0; i < spec->ops_count; i++)
		free(spec->ops[i].name);
	free(spec->definitions);
	free(spec->attrs);
	free(spec->ops);
	free(spec);
}

const struct ctlvctl_op_name *
ctlvctl_spec_op_id(const struct ctlvctl_spec *spec, __u64 id)
{
	unsigned int i;

	if (!spec)
		return NULL;
	for (i = 0; i < spec->ops_count; i++)
		if (spec->ops[i].id == id)
			return &spec->ops[i];
	return NULL;
}

const struct ctlvctl_op_name *
ctlvctl_spec_op_name(const struct ctlvctl_spec *spec, const char *name)
{
	unsigned int i;

	if (!spec)
		return NULL;
	for (i = 0; i < spec->ops_count; i++)
		if (!strcmp(spec->ops[i].name, name))
			return &spec->ops[i];
	return NULL;
}

const struct ctlvctl_attr_name *
ctlvctl_spec_attr_id(const struct ctlvctl_spec *spec, __u64 id)
{
	unsigned int i;

	if (!spec)
		return NULL;
	for (i = 0; i < spec->attrs_count; i++)
		if (spec->attrs[i].id == id)
			return &spec->attrs[i];
	return NULL;
}

const char *ctlvctl_spec_value_name(const struct ctlvctl_attr_name *attr,
				    __u64 value)
{
	unsigned int i;

	if (!attr || !attr->definition)
		return NULL;
	for (i = 0; i < attr->definition->values_count; i++)
		if (attr->definition->values[i].value == value)
			return attr->definition->values[i].name;
	return NULL;
}

int ctlvctl_spec_value(const struct ctlvctl_attr_name *attr, const char *name,
		       __u64 *value)
{
	unsigned int i;

	if (!attr || !attr->definition)
		return -ENOENT;
	for (i = 0; i < attr->definition->values_count; i++)
		if (!strcmp(attr->definition->values[i].name, name)) {
			*value = attr->definition->values[i].value;
			return 0;
		}
	return -ENOENT;
}
