/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Family-independent userspace API; generated bindings build on it.
 * int functions return zero or a negative errno; builders retain first error.
 */
#ifndef _CTLV_H
#define _CTLV_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_gen.h>
#include <linux/types.h>
#include <stdbool.h>
#include <stddef.h>

/* Room for the device's message in struct ctlv_error; a longer one is cut. */
#define CTLV_ERROR_MESSAGE_LEN 256

/* Which members of struct ctlv_error the failure came with. */
#define CTLV_ERROR_HAS_REASON			(1U << 0)
#define CTLV_ERROR_HAS_MESSAGE			(1U << 1)
#define CTLV_ERROR_HAS_BAD_TLV_OFFSET		(1U << 2)
#define CTLV_ERROR_HAS_RECORD_OFFSET		(1U << 3)
#define CTLV_ERROR_HAS_MISSING_TYPE		(1U << 4)
#define CTLV_ERROR_HAS_EXPECTED_GENERATION	(1U << 5)
#define CTLV_ERROR_HAS_CURRENT_GENERATION	(1U << 6)
#define CTLV_ERROR_HAS_REQUIRED_LENGTH		(1U << 7)

/**
 * struct ctlv_error - Why one op failed, as the device explained it.
 * @err: the negative errno the op failed with, zero when it did not
 * @present: CTLV_ERROR_HAS_* bits saying which members below were reported
 * @reason: enum ctlv_validation_reason
 * @message: what the device said, NUL terminated
 * @bad_tlv_offset: offset in the request of the TLV that was refused
 * @record_offset: offset in the request of the record holding it
 * @missing_type: type of a required field that was absent
 * @expected_generation: the generation a stale continuation carried
 * @current_generation: the generation the device is on now
 * @required_length: how long the output blob at @bad_tlv_offset has to be
 *
 * Everything beyond @err is optional, and a caller that wants only the errno
 * may pass NULL wherever this is asked for.
 */
struct ctlv_error {
	int err;
	__u32 present;
	__u32 reason;
	char message[CTLV_ERROR_MESSAGE_LEN];
	__u64 bad_tlv_offset;
	__u64 record_offset;
	__u64 missing_type;
	__u64 expected_generation;
	__u64 current_generation;
	__u64 required_length;
};

/* Renders @error into @buf and returns it, for a diagnostic of one line. */
const char *ctlv_error_str(const struct ctlv_error *error, char *buf,
			   size_t len);

/* An open device, its family chain resolved. */
struct ctlv_dev;
/* A message being built. */
struct ctlv_msg;
/* A complete reply, however many fragments it arrived in. */
struct ctlv_reply;
/* One event. */
struct ctlv_event;
/* One op's request or reply tree, as numbers. */
struct ctlv_schema;

int ctlv_open(const char *family, unsigned int which, int flags,
	      struct ctlv_dev **devp);
int ctlv_open_path(const char *path, int flags, struct ctlv_dev **devp);
/* A file its own subsystem opened, which ctlv_close() leaves open. */
int ctlv_open_fd(int fd, struct ctlv_dev **devp);
void ctlv_close(struct ctlv_dev *dev);

int ctlv_dev_fd(const struct ctlv_dev *dev);
const char *ctlv_dev_name(const struct ctlv_dev *dev);

/* The chain of the opened device, root first and the device family last. */
unsigned int ctlv_chain_len(const struct ctlv_dev *dev);
__u16 ctlv_chain_id(const struct ctlv_dev *dev, unsigned int level);
const char *ctlv_chain_name(const struct ctlv_dev *dev, unsigned int level);
/* The family the device answers as. */
const char *ctlv_dev_family(const struct ctlv_dev *dev);
/* Whether @family is anywhere in the chain, so an inherited op applies. */
bool ctlv_dev_inherits(const struct ctlv_dev *dev, const char *family);

/**
 * struct ctlv_dev_list - Every CTLV device this system has.
 * @names: device names as they appear in /dev
 * @names_count: number of names
 */
struct ctlv_dev_list {
	char **names;
	unsigned int names_count;
};

int ctlv_enumerate(struct ctlv_dev_list *list);
void ctlv_dev_list_free(struct ctlv_dev_list *list);

int ctlv_msg_new(__u32 capacity, struct ctlv_msg **msgp);
/* The same with a ceiling, so an over-long request fails as it is built. */
int ctlv_msg_new_max(__u32 capacity, __u32 max, struct ctlv_msg **msgp);
void ctlv_msg_free(struct ctlv_msg *msg);
void ctlv_msg_reset(struct ctlv_msg *msg);

/* The first failure since the last reset, or 0. */
int ctlv_msg_err(const struct ctlv_msg *msg);
/* Closes whatever is still open and returns the message, or the failure. */
int ctlv_msg_done(struct ctlv_msg *msg);
const struct ctlv_hdr *ctlv_msg_root(const struct ctlv_msg *msg);
__u32 ctlv_msg_len(const struct ctlv_msg *msg);

int ctlv_put_u64(struct ctlv_msg *msg, __u64 type, __u64 value);
int ctlv_put_s64(struct ctlv_msg *msg, __u64 type, __s64 value);
int ctlv_put_bool(struct ctlv_msg *msg, __u64 type, bool value);
int ctlv_put_str(struct ctlv_msg *msg, __u64 type, const char *value);
int ctlv_put_bin(struct ctlv_msg *msg, __u64 type, const void *value,
		 __u32 len);
int ctlv_record_start(struct ctlv_msg *msg, __u64 type);
int ctlv_record_end(struct ctlv_msg *msg);

/*
 * Says of the TLV written last, a record just ended included, that a device
 * whose schema does not declare its type should skip it rather than refuse
 * the request. A type the device does declare is held to its schema as ever,
 * so this covers a field a newer version of a family added and nothing more.
 */
int ctlv_msg_ignore_if_unknown(struct ctlv_msg *msg);

/* A blob TLV carries a record describing memory; these write all of it. */
int ctlv_put_blob_va(struct ctlv_msg *msg, __u64 type, const void *address,
		     __u64 length);

/* Fails when framing, lengths or records are not self-consistent. */
int ctlv_msg_check(const void *buf, __u32 len);

bool ctlv_hdr_is_record(const struct ctlv_hdr *hdr);
const void *ctlv_hdr_payload(const struct ctlv_hdr *hdr, __u32 *len);
__u32 ctlv_hdr_payload_len(const struct ctlv_hdr *hdr);

const struct ctlv_hdr *ctlv_hdr_first(const struct ctlv_hdr *record);
const struct ctlv_hdr *ctlv_hdr_next(const struct ctlv_hdr *record,
				     const struct ctlv_hdr *child);
const struct ctlv_hdr *ctlv_hdr_find(const struct ctlv_hdr *record,
				     __u64 type);
const struct ctlv_hdr *ctlv_hdr_find_next(const struct ctlv_hdr *record,
					  const struct ctlv_hdr *hdr,
					  __u64 type);
unsigned int ctlv_hdr_count(const struct ctlv_hdr *record, __u64 type);

/* Every child of @record, in the order the device wrote them. */
#define ctlv_for_each(_child, _record)					\
	for ((_child) = ctlv_hdr_first(_record); (_child);		\
	     (_child) = ctlv_hdr_next((_record), (_child)))

/* Every occurrence of a field that may repeat. */
#define ctlv_for_each_type(_child, _record, _type)			\
	for ((_child) = ctlv_hdr_find((_record), (_type)); (_child);	\
	     (_child) = ctlv_hdr_find_next((_record), (_child),		\
					   (_type)))

int ctlv_get_u64(const struct ctlv_hdr *hdr, __u64 *value);
int ctlv_get_s64(const struct ctlv_hdr *hdr, __s64 *value);
int ctlv_get_bool(const struct ctlv_hdr *hdr, bool *value);
int ctlv_get_str(const struct ctlv_hdr *hdr, const char **value);
int ctlv_get_bin(const struct ctlv_hdr *hdr, const void **value, __u32 *len);

/*
 * Neither refuses a value a device newer than its caller sent: @known says
 * whether the value is from @min to @max, @unknown collects the bits outside
 * @mask.
 */
int ctlv_get_enum(const struct ctlv_hdr *hdr, __u64 min, __u64 max,
		  __u64 *value, bool *known);
int ctlv_get_flags(const struct ctlv_hdr *hdr, __u64 mask, __u64 *value,
		   __u64 *unknown);

/*
 * An action is one ioctl. A query is as many as its answer takes, reassembled
 * here into the one reply the device would have sent given room for all of it.
 */
int ctlv_action(struct ctlv_dev *dev, __u64 op_id, struct ctlv_msg *request,
		struct ctlv_reply **replyp, struct ctlv_error *error);
int ctlv_query(struct ctlv_dev *dev, __u64 op_id, struct ctlv_msg *request,
	       struct ctlv_reply **replyp, struct ctlv_error *error);

/*
 * The same two with the reply buffer sized, sparing the fallback dump. A sized
 * action keeps error information only if @reply_len has room for it, and a
 * sized query does not restart a stale dump, so a caller learns that it went
 * stale.
 */
int ctlv_action_sized(struct ctlv_dev *dev, __u64 op_id,
		      struct ctlv_msg *request, __u32 reply_len,
		      struct ctlv_reply **replyp, struct ctlv_error *error);
int ctlv_query_sized(struct ctlv_dev *dev, __u64 op_id,
		     struct ctlv_msg *request, __u32 buf_len,
		     struct ctlv_reply **replyp, struct ctlv_error *error);

const struct ctlv_hdr *ctlv_reply_root(const struct ctlv_reply *reply);
/* The generation the answer belongs to, which orders it against events. */
__u64 ctlv_reply_generation(const struct ctlv_reply *reply);
/* How many ioctls it took, a property of the buffer, not the reply. */
unsigned int ctlv_reply_fragments(const struct ctlv_reply *reply);
void ctlv_reply_free(struct ctlv_reply *reply);

/*
 * A subscription belongs to the open file description, one read() returns one
 * event, and an overflowed queue says so with an event of its own.
 */
int ctlv_subscribe(struct ctlv_dev *dev, const __u64 *op_ids, unsigned int n,
		   struct ctlv_error *error);
int ctlv_unsubscribe(struct ctlv_dev *dev, const __u64 *op_ids, unsigned int n,
		     struct ctlv_error *error);

/**
 * struct ctlv_queue - The event queue of one open file description.
 * @len: bytes it may hold
 * @used: bytes its events and reserved room take now
 * @min_len: smallest @len the device takes, zero when it does not say
 * @max_len: largest @len the device takes, zero when it does not say
 */
struct ctlv_queue {
	__u64 len;
	__u64 used;
	__u64 min_len;
	__u64 max_len;
};

int ctlv_queue_get(struct ctlv_dev *dev, struct ctlv_queue *queue,
		   struct ctlv_error *error);
int ctlv_queue_set(struct ctlv_dev *dev, __u64 len, struct ctlv_error *error);

/* -EAGAIN when nothing is queued and the device was opened non-blocking. */
int ctlv_event_read(struct ctlv_dev *dev, struct ctlv_event **eventp);
/* 0 when an event is waiting, -ETIMEDOUT when @timeout_ms passed first. */
int ctlv_event_wait(struct ctlv_dev *dev, int timeout_ms);

__u64 ctlv_event_op_id(const struct ctlv_event *event);
__u64 ctlv_event_sequence(const struct ctlv_event *event);
const struct ctlv_hdr *ctlv_event_root(const struct ctlv_event *event);
void ctlv_event_free(struct ctlv_event *event);

/**
 * struct ctlv_op_change - What one op-changed event reports.
 * @op_id: the op whose state changed
 * @disabled: whether the device refuses it now
 * @generation: the op-state generation the change made current
 *
 * A change no newer than an ops dump's generation is one the dump reported.
 */
struct ctlv_op_change {
	__u64 op_id;
	bool disabled;
	__u64 generation;
};

/**
 * struct ctlv_overflow - What one event-overflow event reports.
 * @first_sequence: sequence the first lost event would have had
 * @last_sequence: sequence of the last one
 * @dropped_count: how many were lost, the two above included
 *
 * Everything between the two sequences was missed and nothing outside them.
 */
struct ctlv_overflow {
	__u64 first_sequence;
	__u64 last_sequence;
	__u64 dropped_count;
};

/*
 * The root family's two events as structures, no generated binding needed.
 * -EINVAL says the event is of another op, -EPROTO that it is malformed.
 */
int ctlv_event_op_change(const struct ctlv_event *event,
			 struct ctlv_op_change *change);
int ctlv_event_overflow(const struct ctlv_event *event,
			struct ctlv_overflow *overflow);

/*
 * What op-schema-get reports, in the shape the generator computes the same
 * numbers into, so a device and its specification compare member by member.
 */

/* Which members of struct ctlv_node_schema the node declares. */
#define CTLV_NODE_SCHEMA_HAS_MIN		(1U << 0)
#define CTLV_NODE_SCHEMA_HAS_MAX		(1U << 1)
#define CTLV_NODE_SCHEMA_HAS_MIN_LENGTH		(1U << 2)
#define CTLV_NODE_SCHEMA_HAS_MAX_LENGTH		(1U << 3)
#define CTLV_NODE_SCHEMA_HAS_FLAGS_MASK		(1U << 4)
#define CTLV_NODE_SCHEMA_HAS_UNIT		(1U << 5)
#define CTLV_NODE_SCHEMA_HAS_SCALE		(1U << 6)
#define CTLV_NODE_SCHEMA_HAS_BLOB		(1U << 7)
#define CTLV_NODE_SCHEMA_HAS_REENTERS		(1U << 8)

/**
 * struct ctlv_node_schema - One node of an op's request or reply tree.
 * @type: the field's type, or CTLV_TYPE_ROOT for the message root
 * @kind: enum ctlv_schema_kind, the one schema kind this payload takes
 * @max_occurrences: how often it may appear, zero meaning as often as fits
 * @present: CTLV_NODE_SCHEMA_HAS_* bits saying which members below apply
 * @min: inclusive lower bound of an integer or enum payload
 * @max: inclusive upper bound of the same
 * @min_length: shortest string or binary payload, a terminator excluded
 * @max_length: longest of the same
 * @flags_mask: every bit a flags payload may carry
 * @unit: enum ctlv_unit of an integer payload
 * @scale: decimal scale that accompanies @unit
 * @blob_direction: enum ctlv_blob_direction of a blob
 * @blob_backings: mask of BIT(enum ctlv_blob_type) this blob takes
 * @blob_max_length: longest external payload the blob may describe
 * @blob_alignment: alignment the blob's address or offset must have
 * @reenters: levels to climb to the node whose members are this one's too
 * @fields: the required nodes one level in
 * @fields_count: how many
 * @optional_fields: the optional nodes one level in
 * @optional_fields_count: how many
 *
 * The same field in two places is two nodes, each required or not.
 */
struct ctlv_node_schema {
	__u64 type;
	__u32 kind;
	__u64 max_occurrences;
	__u32 present;
	__u64 min;
	__u64 max;
	__u64 min_length;
	__u64 max_length;
	__u64 flags_mask;
	__u32 unit;
	__s64 scale;
	__u32 blob_direction;
	__u64 blob_backings;
	__u64 blob_max_length;
	__u64 blob_alignment;
	__u64 reenters;
	const struct ctlv_node_schema *fields;
	unsigned int fields_count;
	const struct ctlv_node_schema *optional_fields;
	unsigned int optional_fields_count;
};

/*
 * Either tree is absent when the op has no such message; an event's payload
 * is reported as the reply.
 */
int ctlv_op_schema_get(struct ctlv_dev *dev, __u64 op_id,
		       struct ctlv_schema **schemap, struct ctlv_error *error);
const struct ctlv_node_schema *ctlv_schema_request(const struct ctlv_schema *s);
const struct ctlv_node_schema *ctlv_schema_reply(const struct ctlv_schema *s);
void ctlv_schema_free(struct ctlv_schema *schema);

/*
 * Whether two trees agree, and where they first differ. @expected is what a
 * specification generated, @actual what a device reported.
 */
bool ctlv_schema_equal(const struct ctlv_node_schema *expected,
		       const struct ctlv_node_schema *actual, char *diff,
		       size_t len);

/* One line describing a node; @indent is how deep it sits. */
void ctlv_schema_render(const struct ctlv_node_schema *node,
			unsigned int indent, char *buf, size_t len);

/* What every device answers, whatever family it belongs to. */

/**
 * struct ctlv_op_info - One entry of an ops dump.
 * @op_id: complete op ID
 * @type: enum ctlv_op_type
 * @fd_access: enum ctlv_op_fd_access, zero for an event
 * @availability: enum ctlv_op_availability, zero for an event
 * @disabled: whether this device refuses the op right now
 * @request_max_len: longest request made from known fields, zero for an event
 * @action_reply_max_len: longest action reply, zero when it is not an action
 * @query_reply_min_len: smallest buffer a query fragment needs
 * @query_reply_suggested_len: buffer size the device would rather have
 * @event_max_len: longest event, zero when it is not an event
 * @error_info_max_len: longest error information, zero for an event
 */
struct ctlv_op_info {
	__u64 op_id;
	__u32 type;
	__u32 fd_access;
	__u32 availability;
	bool disabled;
	__u64 request_max_len;
	__u64 action_reply_max_len;
	__u64 query_reply_min_len;
	__u64 query_reply_suggested_len;
	__u64 event_max_len;
	__u64 error_info_max_len;
};

/**
 * struct ctlv_ops - Every op one device publishes.
 * @ops: entries in the order the device reported them
 * @ops_count: number of entries
 * @generation: the op-state generation the whole dump belongs to
 *
 * Monitoring starts without a race by subscribing, dumping, then taking only
 * the op-changed events newer than @generation.
 */
struct ctlv_ops {
	struct ctlv_op_info *ops;
	unsigned int ops_count;
	__u64 generation;
};

int ctlv_ops_get(struct ctlv_dev *dev, struct ctlv_ops **opsp,
		 struct ctlv_error *error);
void ctlv_ops_free(struct ctlv_ops *ops);
const struct ctlv_op_info *ctlv_ops_find(const struct ctlv_ops *ops,
					 __u64 op_id);

/*
 * What a family's specification says, as its generated binding carries it.
 * Comparing it against what a device reports says whether the two agree.
 */

/**
 * struct ctlv_op_spec - What a specification says about one op.
 * @op_id: complete op ID
 * @type: enum ctlv_op_type
 * @fd_access: enum ctlv_op_fd_access, zero for an event
 * @availability: enum ctlv_op_availability, zero for an event
 * @optional: whether a device may omit this op
 * @request_max_len: longest request made from known fields, zero for an event
 * @action_reply_max_len: longest action reply, zero when it is not an action
 * @query_reply_min_len: smallest buffer a query fragment needs
 * @query_reply_suggested_len: buffer size worth offering a query
 * @event_max_len: longest event, zero when it is not an event
 * @error_info_max_len: longest error information, zero for an event
 * @request: the request tree, NULL when the op takes no request
 * @reply: the reply tree, NULL when it answers none
 *
 * An ops dump says whether a particular device offers an optional op.
 */
struct ctlv_op_spec {
	__u64 op_id;
	__u32 type;
	__u32 fd_access;
	__u32 availability;
	bool optional;
	__u64 request_max_len;
	__u64 action_reply_max_len;
	__u64 query_reply_min_len;
	__u64 query_reply_suggested_len;
	__u64 event_max_len;
	__u64 error_info_max_len;
	const struct ctlv_node_schema *request;
	const struct ctlv_node_schema *reply;
};

/**
 * struct ctlv_family_spec - What a specification says about one family.
 * @name: the family name, as a device's uevent carries it
 * @family_id: its position in the central registry
 * @parent: the family it inherits, NULL for the root
 * @ops: every op a device of the family answers below the root, in the order
 *	its chain declared them
 * @ops_count: number of entries
 *
 * A chain is these linked upwards, one per family. An inherited op is
 * described again in each, because every length that touches it differs.
 */
struct ctlv_family_spec {
	const char *name;
	__u16 family_id;
	const struct ctlv_family_spec *parent;
	const struct ctlv_op_spec *ops;
	unsigned int ops_count;
};

/* The root family, which every chain starts from and every device answers. */
extern const struct ctlv_family_spec ctlv_spec;

/* The op of @spec's chain with @op_id, or NULL for one it does not describe. */
const struct ctlv_op_spec *ctlv_spec_find(const struct ctlv_family_spec *spec,
					  __u64 op_id);

/*
 * Whether @dev agrees with @spec about its published ops: 0 when they do,
 * -ENOMSG when they do not with @diff saying where, -EINVAL when @spec is not
 * the family @dev registered as, another errno when the device could not be
 * asked. Every mandatory op must be published; an optional one may be absent.
 */
int ctlv_spec_check(struct ctlv_dev *dev, const struct ctlv_family_spec *spec,
		    char *diff, size_t len, struct ctlv_error *error);

#endif /* _CTLV_H */
