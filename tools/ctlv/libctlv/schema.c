// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Numeric schema parsing and comparison.
 */

#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "internal.h"

struct validate {
	const struct ctlv_node_schema *node[CTLV_DEPTH_MAX];
};

static const struct ctlv_node_schema *
schema_member(const struct ctlv_node_schema *parent, __u64 attr_id)
{
	unsigned int i;

	for (i = 0; i < parent->attrs_count; i++)
		if (parent->attrs[i].attr_id == attr_id)
			return &parent->attrs[i];
	for (i = 0; i < parent->optional_attrs_count; i++)
		if (parent->optional_attrs[i].attr_id == attr_id)
			return &parent->optional_attrs[i];
	return NULL;
}

static int schema_members(const struct ctlv_attr *nest,
			  const struct ctlv_node_schema *parent)
{
	const struct ctlv_node_schema *child;
	unsigned int count;
	unsigned int i;

	for (i = 0; i < parent->attrs_count; i++) {
		child = &parent->attrs[i];
		count = ctlv_attr_count(nest, child->attr_id);
		if (!count)
			return -EPROTO;
		if (child->max_occurrences &&
		    count > child->max_occurrences)
			return -EPROTO;
	}
	for (i = 0; i < parent->optional_attrs_count; i++) {
		child = &parent->optional_attrs[i];
		count = ctlv_attr_count(nest, child->attr_id);
		if (child->max_occurrences &&
		    count > child->max_occurrences)
			return -EPROTO;
	}
	return 0;
}

static bool schema_length_ok(const struct ctlv_node_schema *node, __u64 len)
{
	if ((node->present & CTLV_NODE_SCHEMA_HAS_MIN_LENGTH) &&
	    len < node->min_length)
		return false;
	if ((node->present & CTLV_NODE_SCHEMA_HAS_MAX_LENGTH) &&
	    len > node->max_length)
		return false;
	return true;
}

static int schema_scalar(const struct ctlv_attr *attr,
			 const struct ctlv_node_schema *node)
{
	const char *string;
	__s64 signed_value;
	__u64 value;
	__u32 len;
	bool boolean;
	int ret;

	switch (node->kind) {
	case CTLV_SCHEMA_UINT:
	case CTLV_SCHEMA_ENUM:
	case CTLV_SCHEMA_FLAGS:
		ret = ctlv_get_u64(attr, &value);
		if (ret)
			return -EPROTO;
		if (node->kind == CTLV_SCHEMA_FLAGS)
			return 0;
		if ((node->present & CTLV_NODE_SCHEMA_HAS_MIN) &&
		    value < node->min)
			return -EPROTO;
		/* New output enum values are appended above the known range. */
		if (node->kind == CTLV_SCHEMA_ENUM)
			return 0;
		if ((node->present & CTLV_NODE_SCHEMA_HAS_MAX) &&
		    value > node->max)
			return -EPROTO;
		return 0;
	case CTLV_SCHEMA_SINT:
		ret = ctlv_get_s64(attr, &signed_value);
		if (ret)
			return -EPROTO;
		if ((node->present & CTLV_NODE_SCHEMA_HAS_MIN) &&
		    signed_value < (__s64)node->min)
			return -EPROTO;
		if ((node->present & CTLV_NODE_SCHEMA_HAS_MAX) &&
		    signed_value > (__s64)node->max)
			return -EPROTO;
		return 0;
	case CTLV_SCHEMA_BOOL:
		return ctlv_get_bool(attr, &boolean) ? -EPROTO : 0;
	case CTLV_SCHEMA_STRING:
		ret = ctlv_get_str(attr, &string);
		if (ret)
			return -EPROTO;
		len = ctlv_attr_payload_len(attr);
		if (memchr(string, '\0', len - 1))
			return -EPROTO;
		return schema_length_ok(node, len - 1) ? 0 : -EPROTO;
	case CTLV_SCHEMA_BINARY:
		len = ctlv_attr_payload_len(attr);
		return schema_length_ok(node, len) ? 0 : -EPROTO;
	default:
		return -EPROTO;
	}
}

static int schema_nest(struct validate *validate,
		       const struct ctlv_attr *nest, unsigned int depth);

static int schema_attr(struct validate *validate,
		       const struct ctlv_attr *attr,
		       const struct ctlv_node_schema *node,
		       unsigned int depth)
{
	bool nested = node->kind == CTLV_SCHEMA_NEST ||
		      node->kind == CTLV_SCHEMA_BLOB;
	__u32 flags = nested ? CTLV_ATTR_F_NESTED : 0;

	if (nested && depth >= CTLV_DEPTH_MAX)
		return -ELOOP;
	if (attr->attr_id != node->attr_id || attr->flags != flags)
		return -EPROTO;
	if (!nested)
		return schema_scalar(attr, node);
	if (node->kind == CTLV_SCHEMA_BLOB)
		return -EPROTO;

	validate->node[depth] = node;
	return schema_nest(validate, attr, depth);
}

static int schema_nest(struct validate *validate,
		       const struct ctlv_attr *nest, unsigned int depth)
{
	const struct ctlv_node_schema *parent = validate->node[depth];
	const struct ctlv_node_schema *child_node;
	const struct ctlv_attr *child;
	unsigned int level = depth;
	int ret;

	while (parent->present & CTLV_NODE_SCHEMA_HAS_REENTERS) {
		if (!parent->reenters || parent->reenters > level)
			return -EPROTO;
		level -= parent->reenters;
		parent = validate->node[level];
	}

	ret = schema_members(nest, parent);
	if (ret)
		return ret;
	ctlv_for_each(child, nest) {
		child_node = schema_member(parent, child->attr_id);
		if (!child_node) {
			if (!child->attr_id ||
			    child->flags & ~CTLV_ATTR_F_NESTED)
				return -EPROTO;
			continue;
		}
		ret = schema_attr(validate, child, child_node, depth + 1);
		if (ret)
			return ret;
	}
	return 0;
}

int ctlv_bootstrap_validate(const void *buf, __u32 len, __u64 op_id)
{
	const struct ctlv_node_schema *schema;
	const struct ctlv_op_spec *op;
	const struct ctlv_attr *root = buf;
	struct validate validate = {};
	int ret;

	op = ctlv_spec_find(&ctlv_spec, op_id);
	if (!op || !op->reply)
		return -EINVAL;
	ret = ctlv_msg_check(buf, len);
	if (ret)
		return ret;
	if (root->len != len)
		return -EPROTO;

	schema = op->reply;
	if (schema->attr_id != CTLV_ATTR_ROOT ||
	    schema->kind != CTLV_SCHEMA_NEST)
		return -EINVAL;
	return schema_attr(&validate, root, schema, 0);
}

/* A node's members occupy one contiguous range in the pool. */
struct build {
	struct ctlv_node_schema *node_schemas;
	const struct ctlv_node_schema *path[CTLV_DEPTH_MAX];
	unsigned int node_schemas_count;
	unsigned int used;
};

static size_t text_append(char *buf, size_t len, size_t used,
			  const char *format, ...)
{
	va_list args;
	int written;

	if (!len)
		return 0;
	if (used >= len - 1) {
		buf[len - 1] = '\0';
		return len - 1;
	}

	va_start(args, format);
	written = vsnprintf(buf + used, len - used, format, args);
	va_end(args);
	if (written < 0) {
		buf[used] = '\0';
		return used;
	}
	if ((size_t)written >= len - used)
		return len - 1;
	return used + written;
}

static bool node_u64(const struct ctlv_attr *src, __u64 attr_id, __u64 *value)
{
	const struct ctlv_attr *attr = ctlv_attr_find(src, attr_id);

	return attr && !ctlv_get_u64(attr, value);
}

/* An optional member, and the bit that says it was there. */
static void node_opt(const struct ctlv_attr *src, __u64 attr_id, __u32 bit,
		     __u64 *value, __u32 *present)
{
	if (node_u64(src, attr_id, value))
		*present |= bit;
}

static bool node_has(const struct ctlv_attr *src, __u64 attr_id)
{
	return ctlv_attr_find(src, attr_id);
}

static int node_id_compare(const void *left, const void *right)
{
	const __u64 *a = left;
	const __u64 *b = right;

	return (*a > *b) - (*a < *b);
}

static int node_children_check(const struct ctlv_attr *src,
			       unsigned int count)
{
	const struct ctlv_attr *child;
	__u64 *ids;
	unsigned int i = 0;
	int ret = 0;

	if (!count)
		return 0;
	ids = calloc(count, sizeof(*ids));
	if (!ids)
		return -ENOMEM;
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_ATTRS) {
		if (!node_u64(child, CTLV_ATTR_SCHEMA_ATTR_ID, &ids[i++])) {
			ret = -EPROTO;
			goto out;
		}
	}
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS) {
		if (!node_u64(child, CTLV_ATTR_SCHEMA_ATTR_ID, &ids[i++])) {
			ret = -EPROTO;
			goto out;
		}
	}
	if (i != count) {
		ret = -EPROTO;
		goto out;
	}
	if (count > 1)
		qsort(ids, count, sizeof(*ids), node_id_compare);
	for (i = 1; i < count; i++) {
		if (ids[i] == ids[i - 1]) {
			ret = -EPROTO;
			break;
		}
	}
out:
	free(ids);
	return ret;
}

static int node_form_check(const struct ctlv_attr *src,
			   const struct ctlv_node_schema *node,
			   unsigned int depth)
{
	bool range = node->kind == CTLV_SCHEMA_UINT ||
		     node->kind == CTLV_SCHEMA_SINT ||
		     node->kind == CTLV_SCHEMA_ENUM;
	bool length = node->kind == CTLV_SCHEMA_STRING ||
		      node->kind == CTLV_SCHEMA_BINARY;
	bool blob = node->kind == CTLV_SCHEMA_BLOB;
	bool flags = node->kind == CTLV_SCHEMA_FLAGS;
	bool children = node->attrs_count || node->optional_attrs_count;
	bool reenters = node_has(src, CTLV_ATTR_SCHEMA_REENTERS);
	bool unit = node_has(src, CTLV_ATTR_SCHEMA_UNIT);
	bool scale = node_has(src, CTLV_ATTR_SCHEMA_SCALE);

	if ((!depth && (node->attr_id != CTLV_ATTR_ROOT ||
			node->kind != CTLV_SCHEMA_NEST ||
			node->max_occurrences != 1)) ||
	    (depth && node->attr_id == CTLV_ATTR_ROOT))
		return -EPROTO;

	/* A later library may understand an appended schema kind. */
	if (node->kind > CTLV_SCHEMA_BLOB)
		return 0;
	if (node_has(src, CTLV_ATTR_SCHEMA_MIN) != range ||
	    node_has(src, CTLV_ATTR_SCHEMA_MAX) != range ||
	    node_has(src, CTLV_ATTR_SCHEMA_MIN_LENGTH) != length ||
	    node_has(src, CTLV_ATTR_SCHEMA_MAX_LENGTH) != length ||
	    node_has(src, CTLV_ATTR_SCHEMA_FLAGS_MASK) != flags ||
	    node_has(src, CTLV_ATTR_SCHEMA_BLOB_DIRECTION) != blob ||
	    node_has(src, CTLV_ATTR_SCHEMA_BLOB_BACKINGS) != blob ||
	    node_has(src, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH) != blob ||
	    node_has(src, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT) != blob)
		return -EPROTO;
	if (unit != scale ||
	    (unit && !node->unit) ||
	    (unit && node->kind != CTLV_SCHEMA_UINT &&
	     node->kind != CTLV_SCHEMA_SINT))
		return -EPROTO;
	if ((children && node->kind != CTLV_SCHEMA_NEST) ||
	    (reenters && (node->kind != CTLV_SCHEMA_NEST || children)))
		return -EPROTO;

	if (range) {
		if (node->kind == CTLV_SCHEMA_SINT)
			return (__s64)node->min <= (__s64)node->max ?
			       0 : -EPROTO;
		if (node->kind == CTLV_SCHEMA_ENUM && !node->min)
			return -EPROTO;
		return node->min <= node->max ? 0 : -EPROTO;
	}
	if (length && node->min_length > node->max_length)
		return -EPROTO;
	if (flags && !node->flags_mask)
		return -EPROTO;
	if (blob && (!node->blob_backings || !node->blob_max_length ||
		     !node->blob_alignment ||
		     node->blob_alignment & (node->blob_alignment - 1)))
		return -EPROTO;
	return 0;
}

static bool node_is_a_node(const struct ctlv_attr *attr)
{
	return attr->attr_id == CTLV_ATTR_SCHEMA_ATTRS ||
	       attr->attr_id == CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS;
}

static int count_nodes(const struct ctlv_attr *src, unsigned int depth,
		       unsigned int *count)
{
	const struct ctlv_attr *child;
	int ret;

	if (depth > CTLV_MAX_NEST_DEPTH)
		return -ELOOP;
	if (*count == UINT_MAX)
		return -EOVERFLOW;
	(*count)++;
	ctlv_for_each(child, src) {
		if (!node_is_a_node(child))
			continue;
		if (!ctlv_attr_is_nest(child))
			return -EPROTO;
		ret = count_nodes(child, depth + 1, count);
		if (ret)
			return ret;
	}
	return 0;
}

static int fill(struct build *build, const struct ctlv_attr *src,
		struct ctlv_node_schema *dst, unsigned int depth)
{
	struct ctlv_node_schema *required, *optional;
	const struct ctlv_attr *child;
	unsigned int required_count, optional_count, i;
	__u64 value;
	int ret;

	if (depth > CTLV_MAX_NEST_DEPTH)
		return -ELOOP;
	build->path[depth] = dst;
	if (!node_u64(src, CTLV_ATTR_SCHEMA_ATTR_ID, &dst->attr_id))
		return -EPROTO;
	if (!node_u64(src, CTLV_ATTR_SCHEMA_KIND, &value))
		return -EPROTO;
	if (value > UINT32_MAX)
		return -EPROTO;
	dst->kind = value;
	if (!node_u64(src, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		      &dst->max_occurrences))
		return -EPROTO;

	node_opt(src, CTLV_ATTR_SCHEMA_MIN, CTLV_NODE_SCHEMA_HAS_MIN, &dst->min,
		 &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MAX, CTLV_NODE_SCHEMA_HAS_MAX, &dst->max,
		 &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MIN_LENGTH, CTLV_NODE_SCHEMA_HAS_MIN_LENGTH,
		 &dst->min_length, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MAX_LENGTH, CTLV_NODE_SCHEMA_HAS_MAX_LENGTH,
		 &dst->max_length, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_FLAGS_MASK, CTLV_NODE_SCHEMA_HAS_FLAGS_MASK,
		 &dst->flags_mask, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_REENTERS, CTLV_NODE_SCHEMA_HAS_REENTERS,
		 &dst->reenters, &dst->present);
	if (node_u64(src, CTLV_ATTR_SCHEMA_UNIT, &value)) {
		if (value > UINT32_MAX)
			return -EPROTO;
		dst->unit = value;
		dst->present |= CTLV_NODE_SCHEMA_HAS_UNIT;
	}
	/* A scale only ever accompanies a unit, and may be negative. */
	if (node_u64(src, CTLV_ATTR_SCHEMA_SCALE, &value)) {
		dst->scale = (__s64)value;
		dst->present |= CTLV_NODE_SCHEMA_HAS_SCALE;
	}
	if (node_u64(src, CTLV_ATTR_SCHEMA_BLOB_DIRECTION, &value)) {
		if (value > UINT32_MAX)
			return -EPROTO;
		dst->blob_direction = value;
		dst->present |= CTLV_NODE_SCHEMA_HAS_BLOB;
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
			 &dst->blob_backings);
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
			 &dst->blob_max_length);
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
			 &dst->blob_alignment);
	}

	required_count = ctlv_attr_count(src, CTLV_ATTR_SCHEMA_ATTRS);
	optional_count = ctlv_attr_count(src, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS);
	if (build->used > build->node_schemas_count ||
	    required_count > build->node_schemas_count - build->used ||
	    optional_count > build->node_schemas_count - build->used - required_count)
		return -EPROTO;
	ret = node_children_check(src, required_count + optional_count);
	if (ret)
		return ret;
	required = &build->node_schemas[build->used];
	build->used += required_count;
	optional = &build->node_schemas[build->used];
	build->used += optional_count;
	dst->attrs = required;
	dst->attrs_count = required_count;
	dst->optional_attrs = optional;
	dst->optional_attrs_count = optional_count;

	i = 0;
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_ATTRS) {
		ret = fill(build, child, &required[i++], depth + 1);
		if (ret)
			return ret;
	}
	i = 0;
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS) {
		ret = fill(build, child, &optional[i++], depth + 1);
		if (ret)
			return ret;
	}
	if ((dst->present & CTLV_NODE_SCHEMA_HAS_REENTERS) &&
	    (!dst->reenters || dst->reenters > depth ||
	     required_count || optional_count ||
	     build->path[depth - dst->reenters]->kind !=
			CTLV_SCHEMA_NEST))
		return -EPROTO;
	return node_form_check(src, dst, depth);
}

/*
 * The two trees of one op. Either may be absent, and an event reports the
 * record it carries as its reply.
 */
int ctlv_schema_parse(const struct ctlv_attr *root, __u32 len,
		      struct ctlv_schema **sp)
{
	const struct ctlv_attr *request, *reply;
	struct ctlv_schema *schema;
	struct build build = {};
	int ret;

	ret = ctlv_bootstrap_validate(root, len, CTLV_OP_OP_SCHEMA_GET);
	if (ret)
		return ret;
	request = ctlv_attr_find(root, CTLV_ATTR_SCHEMA_REQUEST);
	reply = ctlv_attr_find(root, CTLV_ATTR_SCHEMA_REPLY);

	schema = calloc(1, sizeof(*schema));
	if (!schema)
		return -ENOMEM;
	if (request) {
		ret = count_nodes(request, 0, &build.node_schemas_count);
		if (ret)
			goto err;
	}
	if (reply) {
		ret = count_nodes(reply, 0, &build.node_schemas_count);
		if (ret)
			goto err;
	}
	if (build.node_schemas_count) {
		build.node_schemas = calloc(build.node_schemas_count, sizeof(*build.node_schemas));
		if (!build.node_schemas) {
			free(schema);
			return -ENOMEM;
		}
	}
	schema->node_schemas = build.node_schemas;
	schema->node_schemas_count = build.node_schemas_count;

	if (request) {
		schema->request = &build.node_schemas[build.used++];
		ret = fill(&build, request,
			   (struct ctlv_node_schema *)schema->request, 0);
		if (ret)
			goto err;
	}
	if (reply) {
		schema->reply = &build.node_schemas[build.used++];
		ret = fill(&build, reply,
			   (struct ctlv_node_schema *)schema->reply, 0);
		if (ret)
			goto err;
	}
	if (build.used != build.node_schemas_count) {
		ret = -EPROTO;
		goto err;
	}
	*sp = schema;
	return 0;
err:
	ctlv_schema_free(schema);
	return ret;
}

void ctlv_schema_free(struct ctlv_schema *schema)
{
	if (!schema)
		return;
	free(schema->node_schemas);
	free(schema);
}

const struct ctlv_node_schema *ctlv_schema_request(const struct ctlv_schema *s)
{
	return s->request;
}

const struct ctlv_node_schema *ctlv_schema_reply(const struct ctlv_schema *s)
{
	return s->reply;
}

/*
 * Whether two descriptions of one op agree. The first difference is reported,
 * with the path to it, a drift in one place having many consequences.
 */

#define DIFFER(_member, _expected, _actual) do {			\
	if ((_expected) != (_actual)) {					\
		snprintf(diff, len, "%s: %s %llu != %llu", path,	\
			 _member, (unsigned long long)(_expected),	\
			 (unsigned long long)(_actual));		\
		return false;						\
	}								\
} while (0)

static bool node_equal(const struct ctlv_node_schema *expected,
		       const struct ctlv_node_schema *actual, char *path,
		       size_t path_len, size_t at, char *diff, size_t len)
{
	unsigned int i;

	DIFFER("attr-id", expected->attr_id, actual->attr_id);
	DIFFER("kind", expected->kind, actual->kind);
	DIFFER("max-occurrences", expected->max_occurrences,
	       actual->max_occurrences);
	DIFFER("present", expected->present, actual->present);
	DIFFER("min", expected->min, actual->min);
	DIFFER("max", expected->max, actual->max);
	DIFFER("min-length", expected->min_length, actual->min_length);
	DIFFER("max-length", expected->max_length, actual->max_length);
	DIFFER("flags-mask", expected->flags_mask, actual->flags_mask);
	DIFFER("unit", expected->unit, actual->unit);
	DIFFER("scale", expected->scale, actual->scale);
	DIFFER("blob-direction", expected->blob_direction,
	       actual->blob_direction);
	DIFFER("blob-backings", expected->blob_backings,
	       actual->blob_backings);
	DIFFER("blob-max-length", expected->blob_max_length,
	       actual->blob_max_length);
	DIFFER("blob-alignment", expected->blob_alignment,
	       actual->blob_alignment);
	DIFFER("reenters", expected->reenters, actual->reenters);
	DIFFER("attrs", expected->attrs_count, actual->attrs_count);
	DIFFER("optional-attrs", expected->optional_attrs_count,
	       actual->optional_attrs_count);

	for (i = 0; i < expected->attrs_count; i++) {
		size_t used = text_append(path, path_len, at, ".attrs[%u]", i);

		if (!node_equal(&expected->attrs[i], &actual->attrs[i], path,
				path_len, used, diff, len))
			return false;
	}
	for (i = 0; i < expected->optional_attrs_count; i++) {
		size_t used = text_append(path, path_len, at,
					  ".optional-attrs[%u]", i);

		if (!node_equal(&expected->optional_attrs[i],
				&actual->optional_attrs[i], path, path_len,
				used, diff, len))
			return false;
	}
	path[at] = '\0';
	return true;
}

bool ctlv_schema_equal(const struct ctlv_node_schema *expected,
		       const struct ctlv_node_schema *actual, char *diff,
		       size_t len)
{
	char path[512] = "root";

	if (!expected || !actual) {
		if (expected == actual)
			return true;
		snprintf(diff, len, "one tree is absent: %s expected, %s got",
			 expected ? "present" : "absent",
			 actual ? "present" : "absent");
		return false;
	}
	return node_equal(expected, actual, path, sizeof(path), strlen(path),
			  diff, len);
}

static const char *const kinds[] = {
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

/* One line for one placement, for a tool that has no names to print. */
void ctlv_schema_render(const struct ctlv_node_schema *node,
			unsigned int indent, char *buf, size_t len)
{
	const char *kind = "?";
	size_t used;

	if (node->kind < CTLV_ARRAY_SIZE(kinds) && kinds[node->kind])
		kind = kinds[node->kind];

	used = text_append(buf, len, 0, "%*s", indent * 2, "");
	if (node->attr_id == CTLV_ATTR_ROOT)
		used = text_append(buf, len, used, "root %s", kind);
	else
		used = text_append(buf, len, used, "attr %#llx %s",
				   (unsigned long long)node->attr_id, kind);

	if (node->max_occurrences == 1)
		;
	else if (!node->max_occurrences)
		used = text_append(buf, len, used, " xN");
	else
		used = text_append(buf, len, used, " x%llu",
				   (unsigned long long)node->max_occurrences);

	/* One pair of members serves signed and unsigned, read by the kind. */
	if (node->present & (CTLV_NODE_SCHEMA_HAS_MIN | CTLV_NODE_SCHEMA_HAS_MAX)) {
		if (node->kind == CTLV_SCHEMA_SINT)
			used = text_append(buf, len, used, " [%lld, %lld]",
					   (long long)node->min,
					   (long long)node->max);
		else
			used = text_append(buf, len, used, " [%llu, %llu]",
					   (unsigned long long)node->min,
					   (unsigned long long)node->max);
	}
	if (node->present & CTLV_NODE_SCHEMA_HAS_MAX_LENGTH)
		used = text_append(buf, len, used, " len [%llu, %llu]",
				   (unsigned long long)node->min_length,
				   (unsigned long long)node->max_length);
	if (node->present & CTLV_NODE_SCHEMA_HAS_FLAGS_MASK)
		used = text_append(buf, len, used, " mask %#llx",
				   (unsigned long long)node->flags_mask);
	if (node->present & CTLV_NODE_SCHEMA_HAS_UNIT)
		used = text_append(buf, len, used, " unit %u scale %lld",
				   node->unit, (long long)node->scale);
	if (node->present & CTLV_NODE_SCHEMA_HAS_BLOB)
		used = text_append(buf, len, used,
				   " blob dir %u backings %#llx max %llu align %llu",
				   node->blob_direction,
				   (unsigned long long)node->blob_backings,
				   (unsigned long long)node->blob_max_length,
				   (unsigned long long)node->blob_alignment);
	if (node->present & CTLV_NODE_SCHEMA_HAS_REENTERS)
		text_append(buf, len, used, " reenters %llu",
			    (unsigned long long)node->reenters);
}
