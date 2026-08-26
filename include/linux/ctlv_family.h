/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family metadata and generated-code support.
 */
#ifndef _LINUX_CTLV_FAMILY_H
#define _LINUX_CTLV_FAMILY_H

#include <linux/ctlv.h>
#include <linux/gfp_types.h>
#include <linux/kobject.h>
#include <linux/types.h>

#include <uapi/linux/ctlv.h>

/* op-schema-get needs one level to describe the declared schema. */
#define CTLV_MAX_DECLARED_NEST_DEPTH (CTLV_MAX_NEST_DEPTH - 1)

struct ctlv_node_schema {
	u64 attr_id;
	u8 kind;
	u8 unit;
	s8 scale;
	bool reenters;
	u32 max_occurrences;
	u32 first_child;
	u32 children_count;
	u32 required_children_count;
	union {
		struct {
			u64 min;
			u64 max;
		} range;
		u64 flags_mask;
		struct {
			u32 min_len;
			u32 max_len;
		} length;
		struct {
			u64 max_len;
			u32 alignment;
			u16 backings;
			u8 direction;
		} blob;
	};
};

#define CTLV_NODE_SCHEMA_UNLIMITED 0

#define __CTLV_NODE_SCHEMA(_attr, _kind, _occurrences, ...)	\
	{ .attr_id = (_attr), .kind = (_kind),			\
	  .max_occurrences = (_occurrences), __VA_ARGS__ }

#define __CTLV_NODE_SCHEMA_CHILDREN(_first, _count, _required)	\
	.first_child = (_first), .children_count = (_count),	\
	.required_children_count = (_required)

#define CTLV_NODE_SCHEMA_ROOT(_first, _count, _required)	\
	__CTLV_NODE_SCHEMA(CTLV_ATTR_ROOT, CTLV_SCHEMA_NEST, 1,	\
			   __CTLV_NODE_SCHEMA_CHILDREN(_first, _count, _required))

#define CTLV_NODE_SCHEMA_NEST(_attr, _occurrences, _first, _count, _required) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_NEST, _occurrences,	\
			   __CTLV_NODE_SCHEMA_CHILDREN(_first, _count, _required))

#define CTLV_NODE_SCHEMA_NEST_REENTERS(_attr, _occurrences, _first, _count, _required) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_NEST, _occurrences,	\
			   .reenters = true,				\
			   __CTLV_NODE_SCHEMA_CHILDREN(_first, _count, _required))

#define CTLV_NODE_SCHEMA_UINT(_attr, _occurrences, _min, _max)	\
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_UINT, _occurrences,	\
			   .range = { .min = (_min), .max = (_max) })

#define CTLV_NODE_SCHEMA_UINT_UNIT(_attr, _occurrences, _min, _max, _unit, _scale) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_UINT, _occurrences,	\
			   .range = { .min = (_min), .max = (_max) },	\
			   .unit = (_unit), .scale = (_scale))

#define CTLV_NODE_SCHEMA_SINT(_attr, _occurrences, _min, _max)	\
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_SINT, _occurrences,	\
			   .range = { .min = (_min), .max = (_max) })

#define CTLV_NODE_SCHEMA_SINT_UNIT(_attr, _occurrences, _min, _max, _unit, _scale) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_SINT, _occurrences,	\
			   .range = { .min = (_min), .max = (_max) },	\
			   .unit = (_unit), .scale = (_scale))

#define CTLV_NODE_SCHEMA_ENUM(_attr, _occurrences, _max)	\
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_ENUM, _occurrences,	\
			   .range = { .min = 1, .max = (_max) })

#define CTLV_NODE_SCHEMA_FLAGS(_attr, _occurrences, _mask)	\
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_FLAGS, _occurrences,	\
			   .flags_mask = (_mask))

#define CTLV_NODE_SCHEMA_BOOL(_attr, _occurrences)		\
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_BOOL, _occurrences)

#define CTLV_NODE_SCHEMA_STRING(_attr, _occurrences, _min, _max) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_STRING, _occurrences,	\
			   .length = { .min_len = (_min), .max_len = (_max) })

#define CTLV_NODE_SCHEMA_BINARY(_attr, _occurrences, _min, _max) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_BINARY, _occurrences,	\
			   .length = { .min_len = (_min), .max_len = (_max) })

#define CTLV_NODE_SCHEMA_BLOB(_attr, _occurrences, _max, _alignment, _backings, _dir) \
	__CTLV_NODE_SCHEMA(_attr, CTLV_SCHEMA_BLOB, _occurrences,	\
			   .blob = { .max_len = (_max),			\
				     .alignment = (_alignment),		\
				     .backings = (_backings),		\
				     .direction = (_dir) })

struct ctlv_stub_schema {
	u16 family_id;
	u32 set_id;
	u32 node;
	bool optional;
};

#define __CTLV_STUB_SCHEMA(_family, _set, _node, _optional)	\
	{ .family_id = (_family), .set_id = (_set),		\
	  .node = (_node), .optional = (_optional) }

#define CTLV_STUB_SCHEMA(_family, _set, _node)			\
	__CTLV_STUB_SCHEMA(_family, _set, _node, false)

#define CTLV_STUB_SCHEMA_OPTIONAL(_family, _set, _node)		\
	__CTLV_STUB_SCHEMA(_family, _set, _node, true)

struct ctlv_schema_tree {
	const struct ctlv_node_schema *node_schemas;
	u32 node_schemas_count;
	const struct ctlv_stub_schema *stub_schemas;
	u32 stub_schemas_count;
};

#define CTLV_SCHEMA_TREE(_name) {				\
	.node_schemas = _name##_node_schemas,			\
	.node_schemas_count = ARRAY_SIZE(_name##_node_schemas),	\
}

#define CTLV_SCHEMA_TREE_STUBS(_name) {				\
	.node_schemas = _name##_node_schemas,			\
	.node_schemas_count = ARRAY_SIZE(_name##_node_schemas),	\
	.stub_schemas = _name##_stub_schemas,			\
	.stub_schemas_count = ARRAY_SIZE(_name##_stub_schemas),	\
}

#define CTLV_SCHEMA_TREE_NONE() { }

struct ctlv_stub_fill_schema {
	u16 family_id;
	u32 set_id;
	struct ctlv_schema_tree members;
};

#define CTLV_STUB_FILL_SCHEMA(_family, _set, _name) {		\
	.family_id = (_family),					\
	.set_id = (_set),					\
	.members = CTLV_SCHEMA_TREE(_name##_fill),		\
}

struct ctlv_op_sizes {
	u32 request_max_len;
	u32 action_reply_max_len;
	u32 query_reply_min_len;
	u32 query_reply_suggested_len;
	u32 event_record_max_len;
};

struct ctlv_op_schema {
	u64 op_id;
	u8 type;
	u8 fd_access;
	u8 availability;
	bool optional;
	bool pre_post;
	struct ctlv_schema_tree request;
	struct ctlv_schema_tree reply;
	struct ctlv_op_sizes sizes;
};

#define __CTLV_OP_SCHEMA(_op, _type, _availability, ...)	\
	{ .op_id = (_op), .type = (_type),			\
	  .availability = (_availability), __VA_ARGS__ }

#define CTLV_OP_SCHEMA_ACTION(_op, _access, _availability, _request_len, _reply_len, \
			      _request, _reply, ...)			\
	__CTLV_OP_SCHEMA(_op, CTLV_OP_TYPE_ACTION, _availability,	\
			 .fd_access = (_access),			\
			 .request = _request, .reply = _reply,		\
			 .sizes = {					\
				.request_max_len = (_request_len),	\
				.action_reply_max_len = (_reply_len),	\
			 },						\
			 __VA_ARGS__)

#define CTLV_OP_SCHEMA_QUERY(_op, _access, _availability, _request_len, _reply_min, \
			     _reply_suggested, _request, _reply, ...)	\
	__CTLV_OP_SCHEMA(_op, CTLV_OP_TYPE_QUERY, _availability,	\
			 .fd_access = (_access),			\
			 .request = _request, .reply = _reply,		\
			 .sizes = {					\
				.request_max_len = (_request_len),	\
				.query_reply_min_len = (_reply_min),	\
				.query_reply_suggested_len =		\
					(_reply_suggested),		\
			 },						\
			 __VA_ARGS__)

#define CTLV_OP_SCHEMA_EVENT(_op, _record_len, _reply, ...)	\
	__CTLV_OP_SCHEMA(_op, CTLV_OP_TYPE_EVENT,		\
			 CTLV_OP_AVAILABILITY_FIXED,			\
			 .reply = _reply,				\
			 .sizes = {					\
				.event_record_max_len = (_record_len),	\
			 },						\
			 __VA_ARGS__)

struct ctlv_family_schema {
	const char *name;
	u16 family_id;
	const struct ctlv_op_schema *ops;
	u32 ops_count;
	const struct ctlv_stub_fill_schema *stub_fill_schemas;
	u32 stub_fill_schemas_count;
};

/* Leave half of a uevent environment to the device model. */
#define CTLV_UEVENT_MAX_LEN (UEVENT_BUFFER_SIZE / 2)
#define CTLV_UEVENT_MAX_VARS (UEVENT_NUM_ENVP / 2)

struct ctlv_input_ref {
	const struct ctlv_schema_tree *tree;
	u64 attr_id;
	u32 node;
	u32 slot;
	u8 kind;
};

typedef int (*ctlv_action_fn)(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);
typedef int (*ctlv_query_fn)(void *priv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
typedef int (*ctlv_op_pre_fn)(void *priv, struct ctlv_op_ctx *ctx);
typedef void (*ctlv_op_post_fn)(void *priv, struct ctlv_op_ctx *ctx);

struct ctlv_op {
	union {
		ctlv_action_fn action;
		ctlv_query_fn query;
	};
	ctlv_op_pre_fn pre;
	ctlv_op_post_fn post;
};

#define CTLV_OP_ACTION(_name) { .action = _name##_action }
#define CTLV_OP_QUERY(_name) { .query = _name##_query }
#define CTLV_OP_ACTION_PRE_POST(_name)				\
	{ .action = _name##_action, .pre = _name##_pre,		\
	  .post = _name##_post }
#define CTLV_OP_QUERY_PRE_POST(_name)				\
	{ .query = _name##_query, .pre = _name##_pre,		\
	  .post = _name##_post }

typedef size_t (*ctlv_family_priv_size_fn)(void);
typedef const void *(*ctlv_family_pre_init_fn)(const void *init_info);
typedef int (*ctlv_family_init_fn)(struct ctlv_device *ctlvdev,
				   const void *init_info, void *priv);
typedef int (*ctlv_family_post_init_fn)(struct ctlv_device *ctlvdev,
					void *priv);
typedef void (*ctlv_family_pre_fini_fn)(struct ctlv_device *ctlvdev,
					void *priv);
typedef void (*ctlv_family_fini_fn)(struct ctlv_device *ctlvdev, void *priv);

typedef int (*ctlv_stub_fill_fn)(void *priv, struct ctlv_op_ctx *ctx,
				 const struct ctlv_cursor *request,
				 struct ctlv_cursor *nest);

struct ctlv_stub_ops {
	u16 family_id;
	u32 set_id;
	ctlv_stub_fill_fn fill;
};

#define CTLV_STUB_OPS(_family, _set, _name) {			\
	.family_id = (_family),					\
	.set_id = (_set),					\
	.fill = _name##_fill,					\
}

struct ctlv_family {
	const struct ctlv_family_schema *schema;
	const struct ctlv_family *parent;
	const struct ctlv_op *ops;
	u32 ops_count;
	ctlv_op_pre_fn pre;
	ctlv_op_post_fn post;
	ctlv_family_priv_size_fn priv_size;
	ctlv_family_pre_init_fn pre_init;
	ctlv_family_init_fn init;
	ctlv_family_post_init_fn post_init;
	ctlv_family_pre_fini_fn pre_fini;
	ctlv_family_fini_fn fini;
	const struct ctlv_stub_ops *stub_ops;
	u32 stub_ops_count;
};

extern const struct ctlv_family ctlv_family;

#endif /* _LINUX_CTLV_FAMILY_H */
