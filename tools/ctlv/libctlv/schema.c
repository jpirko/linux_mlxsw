// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Reading a schema as numbers, and saying whether two of them agree.
 * op-schema-get describes an op without naming anything, which is enough both
 * to validate a message against a device nothing was compiled for and to
 * compare a device against the specification its family was generated from.
 */

#include <stddef.h>
#include <stdio.h>

#include "internal.h"

/**
 * struct build - Where the nodes of one parse are handed out from.
 * @node: the pool, sized by a counting pass over the reply
 * @n: how many it holds
 * @used: how many have been handed out
 *
 * A node's members are contiguous, so a parent takes all their slots at once.
 */
struct build {
	struct ctlv_schema_node *node;
	unsigned int n;
	unsigned int used;
};

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

static bool node_is_a_node(const struct ctlv_attr *attr)
{
	return attr->attr_id == CTLV_ATTR_SCHEMA_ATTRS ||
	       attr->attr_id == CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS;
}

static unsigned int count_nodes(const struct ctlv_attr *src)
{
	const struct ctlv_attr *child;
	unsigned int n = 1;

	ctlv_for_each(child, src)
		if (node_is_a_node(child))
			n += count_nodes(child);
	return n;
}

static int fill(struct build *build, const struct ctlv_attr *src,
		struct ctlv_schema_node *dst)
{
	struct ctlv_schema_node *required, *optional;
	const struct ctlv_attr *child;
	unsigned int n_required, n_optional, i;
	__u64 value;
	int ret;

	if (!node_u64(src, CTLV_ATTR_SCHEMA_ATTR_ID, &dst->attr_id))
		return -EPROTO;
	if (!node_u64(src, CTLV_ATTR_SCHEMA_KIND, &value))
		return -EPROTO;
	dst->kind = value;
	if (!node_u64(src, CTLV_ATTR_SCHEMA_MAX_OCCURRENCES,
		      &dst->max_occurrences))
		return -EPROTO;

	node_opt(src, CTLV_ATTR_SCHEMA_MIN, CTLV_SCHEMA_HAS_MIN, &dst->min,
		 &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MAX, CTLV_SCHEMA_HAS_MAX, &dst->max,
		 &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MIN_LENGTH, CTLV_SCHEMA_HAS_MIN_LENGTH,
		 &dst->min_length, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_MAX_LENGTH, CTLV_SCHEMA_HAS_MAX_LENGTH,
		 &dst->max_length, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_FLAGS_MASK, CTLV_SCHEMA_HAS_FLAGS_MASK,
		 &dst->flags_mask, &dst->present);
	node_opt(src, CTLV_ATTR_SCHEMA_REENTERS, CTLV_SCHEMA_HAS_REENTERS,
		 &dst->reenters, &dst->present);
	if (node_u64(src, CTLV_ATTR_SCHEMA_UNIT, &value)) {
		dst->unit = value;
		dst->present |= CTLV_SCHEMA_HAS_UNIT;
	}
	/* A scale only ever accompanies a unit, and may be negative. */
	if (node_u64(src, CTLV_ATTR_SCHEMA_SCALE, &value)) {
		dst->scale = (__s64)value;
		dst->present |= CTLV_SCHEMA_HAS_SCALE;
	}
	if (node_u64(src, CTLV_ATTR_SCHEMA_BLOB_DIRECTION, &value)) {
		dst->blob_direction = value;
		dst->present |= CTLV_SCHEMA_HAS_BLOB;
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_BACKINGS,
			 &dst->blob_backings);
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH,
			 &dst->blob_max_length);
		node_u64(src, CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT,
			 &dst->blob_alignment);
	}

	n_required = ctlv_attr_count(src, CTLV_ATTR_SCHEMA_ATTRS);
	n_optional = ctlv_attr_count(src, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS);
	if (n_required + n_optional > build->n - build->used)
		return -EPROTO;
	required = &build->node[build->used];
	build->used += n_required;
	optional = &build->node[build->used];
	build->used += n_optional;
	dst->attrs = required;
	dst->n_attrs = n_required;
	dst->optional_attrs = optional;
	dst->n_optional_attrs = n_optional;

	i = 0;
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_ATTRS) {
		ret = fill(build, child, &required[i++]);
		if (ret)
			return ret;
	}
	i = 0;
	ctlv_for_each_id(child, src, CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS) {
		ret = fill(build, child, &optional[i++]);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * The two trees of one op. Either may be absent, and an event reports the
 * record it carries as its reply.
 */
int ctlv_schema_parse(const struct ctlv_attr *root, struct ctlv_schema **sp)
{
	const struct ctlv_attr *request, *reply;
	struct ctlv_schema *schema;
	struct build build = {};
	int ret;

	request = ctlv_attr_find(root, CTLV_ATTR_SCHEMA_REQUEST);
	reply = ctlv_attr_find(root, CTLV_ATTR_SCHEMA_REPLY);

	schema = calloc(1, sizeof(*schema));
	if (!schema)
		return -ENOMEM;
	if (request)
		build.n += count_nodes(request);
	if (reply)
		build.n += count_nodes(reply);
	if (build.n) {
		build.node = calloc(build.n, sizeof(*build.node));
		if (!build.node) {
			free(schema);
			return -ENOMEM;
		}
	}
	schema->node = build.node;
	schema->n_nodes = build.n;

	if (request) {
		schema->request = &build.node[build.used++];
		ret = fill(&build, request,
			   (struct ctlv_schema_node *)schema->request);
		if (ret)
			goto err;
	}
	if (reply) {
		schema->reply = &build.node[build.used++];
		ret = fill(&build, reply,
			   (struct ctlv_schema_node *)schema->reply);
		if (ret)
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
	free(schema->node);
	free(schema);
}

const struct ctlv_schema_node *ctlv_schema_request(const struct ctlv_schema *s)
{
	return s->request;
}

const struct ctlv_schema_node *ctlv_schema_reply(const struct ctlv_schema *s)
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

static bool node_equal(const struct ctlv_schema_node *expected,
		       const struct ctlv_schema_node *actual, char *path,
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
	DIFFER("attrs", expected->n_attrs, actual->n_attrs);
	DIFFER("optional-attrs", expected->n_optional_attrs,
	       actual->n_optional_attrs);

	for (i = 0; i < expected->n_attrs; i++) {
		size_t used = at + snprintf(path + at, path_len - at,
					    ".attrs[%u]", i);

		if (!node_equal(&expected->attrs[i], &actual->attrs[i], path,
				path_len, used, diff, len))
			return false;
	}
	for (i = 0; i < expected->n_optional_attrs; i++) {
		size_t used = at + snprintf(path + at, path_len - at,
					    ".optional-attrs[%u]", i);

		if (!node_equal(&expected->optional_attrs[i],
				&actual->optional_attrs[i], path, path_len,
				used, diff, len))
			return false;
	}
	path[at] = '\0';
	return true;
}

bool ctlv_schema_equal(const struct ctlv_schema_node *expected,
		       const struct ctlv_schema_node *actual, char *diff,
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
void ctlv_schema_render(const struct ctlv_schema_node *node,
			unsigned int indent, char *buf, size_t len)
{
	const char *kind = "?";
	size_t used;

	if (node->kind < CTLV_ARRAY_SIZE(kinds) && kinds[node->kind])
		kind = kinds[node->kind];

	used = snprintf(buf, len, "%*s", indent * 2, "");
	if (node->attr_id == CTLV_ATTR_ID_ROOT)
		used += snprintf(buf + used, len - used, "root %s", kind);
	else
		used += snprintf(buf + used, len - used, "attr %#llx %s",
				 (unsigned long long)node->attr_id, kind);

	if (node->max_occurrences == 1)
		;
	else if (!node->max_occurrences)
		used += snprintf(buf + used, len - used, " xN");
	else
		used += snprintf(buf + used, len - used, " x%llu",
				 (unsigned long long)node->max_occurrences);

	/* One pair of members serves signed and unsigned, read by the kind. */
	if (node->present & (CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX)) {
		if (node->kind == CTLV_SCHEMA_SINT)
			used += snprintf(buf + used, len - used,
					 " [%lld, %lld]",
					 (long long)node->min,
					 (long long)node->max);
		else
			used += snprintf(buf + used, len - used,
					 " [%llu, %llu]",
					 (unsigned long long)node->min,
					 (unsigned long long)node->max);
	}
	if (node->present & CTLV_SCHEMA_HAS_MAX_LENGTH)
		used += snprintf(buf + used, len - used, " len [%llu, %llu]",
				 (unsigned long long)node->min_length,
				 (unsigned long long)node->max_length);
	if (node->present & CTLV_SCHEMA_HAS_FLAGS_MASK)
		used += snprintf(buf + used, len - used, " mask %#llx",
				 (unsigned long long)node->flags_mask);
	if (node->present & CTLV_SCHEMA_HAS_UNIT)
		used += snprintf(buf + used, len - used, " unit %u scale %lld",
				 node->unit, (long long)node->scale);
	if (node->present & CTLV_SCHEMA_HAS_BLOB)
		used += snprintf(buf + used, len - used,
				 " blob dir %u backings %#llx max %llu align %llu",
				 node->blob_direction,
				 (unsigned long long)node->blob_backings,
				 (unsigned long long)node->blob_max_length,
				 (unsigned long long)node->blob_alignment);
	if (node->present & CTLV_SCHEMA_HAS_REENTERS)
		snprintf(buf + used, len - used, " reenters %llu",
			 (unsigned long long)node->reenters);
}
