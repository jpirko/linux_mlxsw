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

#include <uapi/ctlv/ctlv.h>

/*
 * Implementation limits, not maxima of message fields: userspace learns the
 * lengths of each op from op-get, so raising one is compatible.
 */

/* Request roots, action replies, query reply fragments and event roots. */
#define CTLV_MAX_INLINE_MESSAGE_LEN	0x20000		/* 128 KiB */
/* Includes the error information root. */
#define CTLV_MAX_ERROR_LEN		0x4000		/* 16 KiB */
/* The root is at depth zero. Validation recurses once per level. */
#define CTLV_MAX_RECORD_DEPTH		8
#define CTLV_MAX_EVENT_QUEUE_LEN	0x1000000	/* 16 MiB */

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
	u32 event_max_len;
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

/* An op never executed: the device queues events of up to _event_len. */
#define CTLV_OP_SCHEMA_EVENT(_op, _event_len, _reply, ...)		\
	((struct ctlv_op_schema){					\
		.op_id = (_op),						\
		.type = CTLV_OP_TYPE_EVENT,				\
		.availability = CTLV_OP_AVAILABILITY_FIXED,		\
		.reply = _reply,					\
		.sizes = {						\
			.event_max_len = (_event_len),			\
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

void ctlv_op_provide(struct ctlv *ctlv, u64 op_id);
void ctlv_op_state_set(struct ctlv *ctlv, u64 op_id, bool disabled);

struct ctlv_cursor *
ctlv_input_field(struct ctlv_cursor *cursor, const struct ctlv_cursor *record,
		 struct ctlv_input_ref ref);
bool ctlv_input_cursor(const struct ctlv_cursor *cursor,
		       struct ctlv_input_ref ref);
void ctlv_cursor_next(struct ctlv_cursor *cursor);
bool ctlv_cursor_ok(const struct ctlv_cursor *cursor);
u64 ctlv_cursor_u64(const struct ctlv_cursor *cursor);
s64 ctlv_cursor_s64(const struct ctlv_cursor *cursor);
bool ctlv_cursor_bool(const struct ctlv_cursor *cursor);
const char *ctlv_cursor_string(const struct ctlv_cursor *cursor);
const void *ctlv_cursor_binary(const struct ctlv_cursor *cursor, u32 *len);

bool ctlv_put_u64(struct ctlv_cursor *record, u64 type, u8 kind, u64 value);
bool ctlv_put_s64(struct ctlv_cursor *record, u64 type, u8 kind, s64 value);
bool ctlv_put_bool(struct ctlv_cursor *record, u64 type, u8 kind, bool value);
bool ctlv_put_string(struct ctlv_cursor *record, u64 type, u8 kind,
		     const char *value);
bool ctlv_put_binary(struct ctlv_cursor *record, u64 type, u8 kind,
		     const void *value, u32 len);
bool ctlv_record_start(struct ctlv_cursor *record, struct ctlv_cursor *child,
		       u64 type, u8 kind);

struct ctlv_blob *ctlv_blob_resolve(struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *blob);
struct ctlv_op_ctx *ctlv_event_create(struct ctlv *ctlv, u64 op_id,
				      struct ctlv_cursor **payload, gfp_t gfp);

#define CTLV_OP_PROVIDE(_name, _op_id)				\
static inline void _name##_provide(struct ctlv *ctlv)	\
{								\
	ctlv_op_provide(ctlv, (_op_id));		\
}

#define CTLV_OP_ENABLE(_name, _op_id)				\
static inline void _name##_enable(struct ctlv *ctlv)	\
{								\
	ctlv_op_state_set(ctlv, (_op_id), false);	\
}

#define CTLV_OP_DISABLE(_name, _op_id)				\
static inline void _name##_disable(struct ctlv *ctlv)	\
{								\
	ctlv_op_state_set(ctlv, (_op_id), true);	\
}

#define CTLV_EVENT_CREATE(_name, _op_id)			\
static inline struct ctlv_op_ctx *				\
_name##_create(struct ctlv *ctlv,			\
	       struct ctlv_cursor **payload, gfp_t gfp)		\
{								\
	return ctlv_event_create(ctlv, (_op_id), payload, gfp);	\
}

#define __CTLV_DEFINE_PUT(_name, _ctype, _put_fn, _type, _kind)	\
static inline bool _name##_put(struct ctlv_cursor *record,	\
			       _ctype value)			\
{								\
	return _put_fn(record, (_type), (_kind), value);	\
}

#define CTLV_PUT_U64(_name, _type, _kind)			\
	__CTLV_DEFINE_PUT(_name, u64, ctlv_put_u64, _type, _kind)
#define CTLV_PUT_S64(_name, _type, _kind)			\
	__CTLV_DEFINE_PUT(_name, s64, ctlv_put_s64, _type, _kind)
#define CTLV_PUT_BOOL(_name, _type, _kind)			\
	__CTLV_DEFINE_PUT(_name, bool, ctlv_put_bool, _type, _kind)
#define CTLV_PUT_STRING(_name, _type, _kind)			\
	__CTLV_DEFINE_PUT(_name, const char *, ctlv_put_string,	\
			  _type, _kind)

#define CTLV_PUT_BINARY(_name, _type, _kind)			\
static inline bool _name##_put(struct ctlv_cursor *record,	\
			       const void *value, u32 len)	\
{								\
	return ctlv_put_binary(record, (_type), (_kind), value, len); \
}

#define CTLV_RECORD_START(_name, _type, _kind)			\
static inline bool _name##_start(struct ctlv_cursor *record,	\
				 struct ctlv_cursor *inner)	\
{								\
	return ctlv_record_start(record, inner, (_type), (_kind)); \
}

#define CTLV_IN_REF(_name, _type, _kind)			\
static inline struct ctlv_input_ref _name##_ref(void)		\
{								\
	return (struct ctlv_input_ref){				\
		.type = (_type),				\
		.kind = (_kind),				\
	};							\
}

#define CTLV_IN_FOR_EACH(_name, _cursor, _record)		\
	for (struct ctlv_cursor *__ctlv_cursor =		\
		ctlv_input_field(_cursor, _record, _name##_ref()); \
	     ctlv_cursor_ok(__ctlv_cursor);			\
	     ctlv_cursor_next(__ctlv_cursor))

#define CTLV_IN_HAS(_name)					\
static inline bool _name##_present(const struct ctlv_cursor *record) \
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_ok(&cursor);				\
}

#define CTLV_IN_U64(_name)					\
static inline u64 _name##_get(const struct ctlv_cursor *record)	\
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_u64(&cursor);			\
}

#define CTLV_IN_S64(_name)					\
static inline s64 _name##_get(const struct ctlv_cursor *record)	\
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_s64(&cursor);			\
}

#define CTLV_IN_BOOL(_name)					\
static inline bool _name##_get(const struct ctlv_cursor *record) \
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_bool(&cursor);			\
}

#define CTLV_IN_STRING(_name)					\
static inline const char *					\
_name##_get(const struct ctlv_cursor *record)			\
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_string(&cursor);			\
}

#define CTLV_IN_BINARY(_name)					\
static inline const void *					\
_name##_get(const struct ctlv_cursor *record, u32 *len)		\
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_cursor_binary(&cursor, len);		\
}

#define CTLV_IN_RECORD(_name)					\
static inline void _name##_get(struct ctlv_cursor *inner,	\
			       const struct ctlv_cursor *record) \
{								\
	ctlv_input_field(inner, record, _name##_ref());		\
}

#define CTLV_IN_BLOB(_name)					\
static inline struct ctlv_blob *				\
_name##_resolve(struct ctlv_op_ctx *ctx,			\
		const struct ctlv_cursor *record)		\
{								\
	struct ctlv_cursor cursor;				\
								\
	ctlv_input_field(&cursor, record, _name##_ref());	\
	return ctlv_blob_resolve(ctx, &cursor);			\
}

#define CTLV_IN_EACH_U64(_name)					\
static inline u64 _name##_get(const struct ctlv_cursor *cursor)	\
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref()))		\
		return 0;					\
	return ctlv_cursor_u64(cursor);				\
}

#define CTLV_IN_EACH_S64(_name)					\
static inline s64 _name##_get(const struct ctlv_cursor *cursor)	\
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref()))		\
		return 0;					\
	return ctlv_cursor_s64(cursor);				\
}

#define CTLV_IN_EACH_BOOL(_name)				\
static inline bool _name##_get(const struct ctlv_cursor *cursor) \
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref()))		\
		return false;					\
	return ctlv_cursor_bool(cursor);			\
}

#define CTLV_IN_EACH_STRING(_name)				\
static inline const char *					\
_name##_get(const struct ctlv_cursor *cursor)			\
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref()))		\
		return NULL;					\
	return ctlv_cursor_string(cursor);			\
}

#define CTLV_IN_EACH_BINARY(_name)				\
static inline const void *					\
_name##_get(const struct ctlv_cursor *cursor, u32 *len)		\
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref())) {	\
		*len = 0;					\
		return NULL;					\
	}							\
	return ctlv_cursor_binary(cursor, len);			\
}

#define CTLV_IN_EACH_BLOB(_name)				\
static inline struct ctlv_blob *				\
_name##_resolve(struct ctlv_op_ctx *ctx,			\
		const struct ctlv_cursor *cursor)		\
{								\
	if (!ctlv_input_cursor(cursor, _name##_ref()))		\
		return ERR_PTR(-EINVAL);			\
	return ctlv_blob_resolve(ctx, cursor);			\
}

#endif /* _LINUX_CTLV_FAMILY_H */
