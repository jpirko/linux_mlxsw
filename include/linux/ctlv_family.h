/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family metadata and generated-code support.
 */
#ifndef _LINUX_CTLV_FAMILY_H
#define _LINUX_CTLV_FAMILY_H

#include <linux/ctlv.h>
#include <linux/kobject.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv.h>

/*
 * Implementation limits, not maxima of message fields: userspace learns the
 * lengths of each op from op-get, so raising one is compatible.
 */

/* Request roots, action replies and query reply fragments. */
#define CTLV_MAX_INLINE_MESSAGE_LEN	0x10000		/* 64 KiB */
/* Includes the error information root. */
#define CTLV_MAX_ERROR_LEN		0x4000		/* 16 KiB */
/* The root is at depth zero. Validation recurses once per level. */
#define CTLV_MAX_RECORD_DEPTH		8

/* op-schema-get needs one level to describe the declared schema. */
#define CTLV_MAX_DECLARED_RECORD_DEPTH (CTLV_MAX_RECORD_DEPTH - 1)

struct ctlv_node_schema {
	u64 type;
	u8 kind;
	u8 unit;
	s8 scale;
	bool reenters;
	/* An enum extension point, which enum_ext names in place of a range. */
	bool is_enum_ext;
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
		struct {
			u16 family_id;
			u32 ext_id;
		} enum_ext;
	};
};

#define CTLV_NODE_SCHEMA_UNLIMITED 0

/* Node 0 of every tree: the message root, holding the top-level fields. */
#define CTLV_NODE_SCHEMA_ROOT(_first, _count, _required)		\
	((struct ctlv_node_schema){					\
		.type = CTLV_TYPE_ROOT,					\
		.kind = CTLV_SCHEMA_RECORD,				\
		.max_occurrences = 1,					\
		.first_child = (_first),				\
		.children_count = (_count),				\
		.required_children_count = (_required),			\
	})

/* A record whose members are the _count nodes from _first, required first. */
#define CTLV_NODE_SCHEMA_RECORD(_type, _occurrences, _first, _count, _required) \
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_RECORD,				\
		.max_occurrences = (_occurrences),			\
		.first_child = (_first),				\
		.children_count = (_count),				\
		.required_children_count = (_required),			\
	})

/* A recursive record, sharing the members of the one above holding its set. */
#define CTLV_NODE_SCHEMA_RECORD_REENTERS(_type, _occurrences, _first, _count, _required) \
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_RECORD,				\
		.reenters = true,					\
		.max_occurrences = (_occurrences),			\
		.first_child = (_first),				\
		.children_count = (_count),				\
		.required_children_count = (_required),			\
	})

/* A u64 between _min and _max. */
#define CTLV_NODE_SCHEMA_UINT(_type, _occurrences, _min, _max)		\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_UINT,				\
		.max_occurrences = (_occurrences),			\
		.range = { .min = (_min), .max = (_max) },		\
	})

/* A u64 between _min and _max that carries a unit and a scale. */
#define CTLV_NODE_SCHEMA_UINT_UNIT(_type, _occurrences, _min, _max, _unit, _scale) \
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_UINT,				\
		.max_occurrences = (_occurrences),			\
		.range = { .min = (_min), .max = (_max) },		\
		.unit = (_unit),					\
		.scale = (_scale),					\
	})

/* An s64 between _min and _max, both stored as u64. */
#define CTLV_NODE_SCHEMA_SINT(_type, _occurrences, _min, _max)		\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_SINT,				\
		.max_occurrences = (_occurrences),			\
		.range = { .min = (_min), .max = (_max) },		\
	})

/* An s64 between _min and _max that carries a unit and a scale. */
#define CTLV_NODE_SCHEMA_SINT_UNIT(_type, _occurrences, _min, _max, _unit, _scale) \
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_SINT,				\
		.max_occurrences = (_occurrences),			\
		.range = { .min = (_min), .max = (_max) },		\
		.unit = (_unit),					\
		.scale = (_scale),					\
	})

/* An enum taking _min to _max, gaps between a header enum's values included. */
#define CTLV_NODE_SCHEMA_ENUM_RANGE(_type, _occurrences, _min, _max)	\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_ENUM,				\
		.max_occurrences = (_occurrences),			\
		.range = { .min = (_min), .max = (_max) },		\
	})

/* A family's own enum, whose values are numbered from 1 to _max. */
#define CTLV_NODE_SCHEMA_ENUM(_type, _occurrences, _max)		\
	CTLV_NODE_SCHEMA_ENUM_RANGE(_type, _occurrences, 1, _max)

/* Enum extension point _ext of _family, numbered as the device's family fills it. */
#define CTLV_NODE_SCHEMA_ENUM_EXT(_type, _occurrences, _family, _ext)	\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_ENUM,				\
		.is_enum_ext = true,					\
		.max_occurrences = (_occurrences),			\
		.enum_ext = { .family_id = (_family), .ext_id = (_ext) }, \
	})

/* A u64 of flags, none of them outside _mask. */
#define CTLV_NODE_SCHEMA_FLAGS(_type, _occurrences, _mask)		\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_FLAGS,				\
		.max_occurrences = (_occurrences),			\
		.flags_mask = (_mask),					\
	})

/* A u8 that is 0 or 1. */
#define CTLV_NODE_SCHEMA_BOOL(_type, _occurrences)			\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_BOOL,				\
		.max_occurrences = (_occurrences),			\
	})

/* A NUL-terminated string of _min to _max bytes, the NUL excluded. */
#define CTLV_NODE_SCHEMA_STRING(_type, _occurrences, _min, _max)	\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_STRING,				\
		.max_occurrences = (_occurrences),			\
		.length = { .min_len = (_min), .max_len = (_max) },	\
	})

/* A payload of _min to _max bytes. */
#define CTLV_NODE_SCHEMA_BINARY(_type, _occurrences, _min, _max)	\
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_BINARY,				\
		.max_occurrences = (_occurrences),			\
		.length = { .min_len = (_min), .max_len = (_max) },	\
	})

/* A blob descriptor, for data passed by reference instead of inline. */
#define CTLV_NODE_SCHEMA_BLOB(_type, _occurrences, _max, _alignment, _backings, _dir) \
	((struct ctlv_node_schema){					\
		.type = (_type),					\
		.kind = CTLV_SCHEMA_BLOB,				\
		.max_occurrences = (_occurrences),			\
		.blob = {						\
			.max_len = (_max),				\
			.alignment = (_alignment),			\
			.backings = (_backings),			\
			.direction = (_dir),				\
		},							\
	})

struct ctlv_ext_schema {
	u16 family_id;
	u32 ext_id;
	u32 node;
	bool optional;
};

/* An extension point at _node, which a descendant family fills. */
#define CTLV_EXT_SCHEMA(_family, _ext, _node)				\
	((struct ctlv_ext_schema){					\
		.family_id = (_family),					\
		.ext_id = (_ext),					\
		.node = (_node),					\
	})

/* An optional extension point: what fills it is optional too. */
#define CTLV_EXT_SCHEMA_OPTIONAL(_family, _ext, _node)			\
	((struct ctlv_ext_schema){					\
		.family_id = (_family),					\
		.ext_id = (_ext),					\
		.node = (_node),					\
		.optional = true,					\
	})

struct ctlv_schema_tree {
	const struct ctlv_node_schema *node_schemas;
	u32 node_schemas_count;
	const struct ctlv_ext_schema *ext_schemas;
	u32 ext_schemas_count;
};

/* The tree of one message, from _name##_node_schemas. */
#define CTLV_SCHEMA_TREE(_name)						\
	((struct ctlv_schema_tree){					\
		.node_schemas = _name##_node_schemas,			\
		.node_schemas_count = ARRAY_SIZE(_name##_node_schemas),	\
	})

/* The same, for a message with extension points in _name##_ext_schemas. */
#define CTLV_SCHEMA_TREE_EXTS(_name)					\
	((struct ctlv_schema_tree){					\
		.node_schemas = _name##_node_schemas,			\
		.node_schemas_count = ARRAY_SIZE(_name##_node_schemas),	\
		.ext_schemas = _name##_ext_schemas,			\
		.ext_schemas_count = ARRAY_SIZE(_name##_ext_schemas),	\
	})

/* The tree of a message an op does not have. */
#define CTLV_SCHEMA_TREE_NONE() ((struct ctlv_schema_tree){})

struct ctlv_ext_fill_schema {
	u16 family_id;
	u32 ext_id;
	struct ctlv_schema_tree members;
};

/* What a family puts into an ancestor's extension point. */
#define CTLV_EXT_FILL_SCHEMA(_family, _ext, _name)			\
	((struct ctlv_ext_fill_schema){					\
		.family_id = (_family),					\
		.ext_id = (_ext),					\
		.members = CTLV_SCHEMA_TREE(_name##_fill),		\
	})

struct ctlv_enum_fill_schema {
	u16 family_id;
	u32 ext_id;
	u32 max;
};

/* The values 1 to _max a family gives an ancestor's enum extension point. */
#define CTLV_ENUM_FILL_SCHEMA(_family, _ext, _max)			\
	((struct ctlv_enum_fill_schema){				\
		.family_id = (_family),					\
		.ext_id = (_ext),					\
		.max = (_max),						\
	})

struct ctlv_op_sizes {
	u32 request_max_len;
	u32 action_reply_max_len;
	u32 query_reply_min_len;
	u32 query_reply_suggested_len;
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

/* An op run once per call, its reply at most _reply_len bytes. */
#define CTLV_OP_SCHEMA_ACTION(_op, _access, _availability, _request_len, _reply_len, \
			      _request, _reply, ...)			\
	((struct ctlv_op_schema){					\
		.op_id = (_op),						\
		.type = CTLV_OP_TYPE_ACTION,				\
		.fd_access = (_access),					\
		.availability = (_availability),			\
		.request = _request,					\
		.reply = _reply,					\
		.sizes = {						\
			.request_max_len = (_request_len),		\
			.action_reply_max_len = (_reply_len),		\
		},							\
		__VA_ARGS__						\
	})

/* An op answered in fragments, each needing _reply_min bytes of room. */
#define CTLV_OP_SCHEMA_QUERY(_op, _access, _availability, _request_len, _reply_min, \
			     _reply_suggested, _request, _reply, ...)	\
	((struct ctlv_op_schema){					\
		.op_id = (_op),						\
		.type = CTLV_OP_TYPE_QUERY,				\
		.fd_access = (_access),					\
		.availability = (_availability),			\
		.request = _request,					\
		.reply = _reply,					\
		.sizes = {						\
			.request_max_len = (_request_len),		\
			.query_reply_min_len = (_reply_min),		\
			.query_reply_suggested_len = (_reply_suggested), \
		},							\
		__VA_ARGS__						\
	})

struct ctlv_family_schema {
	const char *name;
	u16 family_id;
	const struct ctlv_op_schema *ops;
	u32 ops_count;
	const struct ctlv_ext_fill_schema *ext_fill_schemas;
	u32 ext_fill_schemas_count;
	const struct ctlv_enum_fill_schema *enum_fill_schemas;
	u32 enum_fill_schemas_count;
};

/* Leave half of a uevent environment to the device model. */
#define CTLV_UEVENT_MAX_LEN (UEVENT_BUFFER_SIZE / 2)
#define CTLV_UEVENT_MAX_VARS (UEVENT_NUM_ENVP / 2)

/* A field a reader expects among the children of the record it reads. */
struct ctlv_input_ref {
	u64 type;
	u8 kind;
};

struct ctlv_op {
	union {
		int (*action)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply);
		int (*query)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			     const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply);
	};
	int (*pre)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
	void (*post)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx);
};

/* An action implemented by _name##_action. */
#define CTLV_OP_ACTION(_name) ((struct ctlv_op){ .action = _name##_action })

/* A query implemented by _name##_query. */
#define CTLV_OP_QUERY(_name) ((struct ctlv_op){ .query = _name##_query })

/* An action with its own hooks, _name##_pre and _name##_post. */
#define CTLV_OP_ACTION_PRE_POST(_name)					\
	((struct ctlv_op){						\
		.action = _name##_action,				\
		.pre = _name##_pre,					\
		.post = _name##_post,					\
	})

/* A query with its own hooks, _name##_pre and _name##_post. */
#define CTLV_OP_QUERY_PRE_POST(_name)					\
	((struct ctlv_op){						\
		.query = _name##_query,					\
		.pre = _name##_pre,					\
		.post = _name##_post,					\
	})

struct ctlv_ext_ops {
	u16 family_id;
	u32 ext_id;
	int (*fill)(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
		    const struct ctlv_cursor *request,
		    struct ctlv_cursor *record);
};

/* How a family fills an ancestor's extension point. */
#define CTLV_EXT_OPS(_family, _ext, _name)				\
	((struct ctlv_ext_ops){						\
		.family_id = (_family),					\
		.ext_id = (_ext),					\
		.fill = _name##_fill,					\
	})

struct ctlv_family {
	const struct ctlv_family_schema *schema;
	const struct ctlv_family *parent;
	const struct ctlv_op *ops;
	u32 ops_count;
	const struct ctlv_ext_ops *ext_ops;
	u32 ext_ops_count;
};

extern const struct ctlv_family ctlv_family;

u32 ctlv_enum_ext_max(const struct ctlv_family *family, u16 family_id, u32 ext_id);

#endif /* _LINUX_CTLV_FAMILY_H */
