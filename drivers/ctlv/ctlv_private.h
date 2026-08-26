/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV core internals: the device instance, one open file description, and one
 * op execution. A family sees only <linux/ctlv.h>.
 */
#ifndef _CTLV_PRIVATE_H
#define _CTLV_PRIVATE_H

#include <linux/cdev.h>
#include <linux/ctlv.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

#include <uapi/linux/ctlv.h>

#include "message.h"
#include "schema.h"

struct task_struct;

/* An op that is always enabled has no per-device availability state. */
#define CTLV_OP_STATE_NONE U32_MAX

/**
 * struct ctlv_op_entry - One op of one device's effective schema, resolved.
 * @schema: immutable metadata of the op, owned by the family that declared it
 * @answers: descriptor whose binding and hook answer this op, NULL for an
 *	event
 * @binding: what answers it on this device, NULL for an event
 * @sizes: lengths of this op on this device, filled extension points included
 * @state_index: availability bit of this op, or CTLV_OP_STATE_NONE when it is
 *	always enabled
 * @removed: whether the family took this op away before publication
 * @event_index: subscription bit of this op, counted over this device's events
 *
 * Built once per device. It caches pointers and derived scalars, never a
 * second copy of the schema.
 */
struct ctlv_op_entry {
	const struct ctlv_op_schema *schema;
	const struct ctlv_family *answers;
	const struct ctlv_op *binding;
	struct ctlv_op_sizes sizes;
	u32 state_index;
	bool removed;
	u32 event_index;
};

/**
 * struct ctlv_device - One CTLV device instance.
 * @dev: class device, which is also this instance's reference count
 * @cdev: character device of @dev
 * @registration_lock: read around everything that reaches family code, taken
 *	for writing to publish that there is none left
 * @family: generated descriptor, NULL once unregistration has begun
 * @priv: family-private pointer, cleared with @family
 * @ops: resolved effective ops, ascending by op ID, immutable once published
 * @n_ops: number of entries in @ops
 * @transition_lock: orders availability writers; never held while family code
 *	runs and never taken by dispatch
 * @published: whether @ops has been frozen
 * @state_generation: advances once per actual availability change
 * @n_dynamic_ops: ops of @ops a device may disable, and bits in @disabled
 * @disabled: bit per dynamically available op, indexed by its state index, or
 *	NULL when this family has no such op
 * @events_lock: orders an event's fanout against the queues and against the
 *	descriptions arriving and leaving; taken inside @transition_lock
 * @files: every open file description of this device
 * @dead_announced: whether the removal record has been queued, the last one
 * @n_events: events this device may emit, and bits in a subscription set
 * @overflow_op: the overflow event, resolved before its module can unload
 * @removed_op: the removal event, resolved for the same reason
 * @queue_min: shortest queue an open of this device may ask for
 * @queue_reserved: bytes every open reserves for its two control records
 * @queue_default: what an open of this device starts with
 *
 * The device outlives its family: an open file description holds a reference,
 * so a dead instance stays allocated, answering -ENODEV, until the last close.
 */
struct ctlv_device {
	struct device dev;
	struct cdev cdev;

	struct rw_semaphore registration_lock;
	const struct ctlv_family *family;
	void *priv;

	struct ctlv_op_entry *ops;
	u32 n_ops;

	/* Availability writers, and nothing on the dispatch path. */
	spinlock_t transition_lock;
	bool published;
	u64 state_generation;
	u32 n_dynamic_ops;
	unsigned long *disabled;

	/* Everything about the queues, and nothing family code runs under. */
	spinlock_t events_lock;
	struct list_head files;
	bool dead_announced;
	u32 n_events;
	const struct ctlv_op_entry *overflow_op;
	const struct ctlv_op_entry *removed_op;
	u32 queue_min;
	u32 queue_reserved;
	u32 queue_default;
};

/**
 * struct ctlv_event_record - One event, as one read() delivers it.
 * @node: place in the queue of one open file description
 * @len: bytes of this record, @hdr and the padded root after it
 * @room: bytes that may be written from @hdr, which a reserved record needs
 * @reserved: whether this is a control record, returned to its open when read
 * @hdr: what the reader gets first
 * @root: the one root attr that follows it, padded to the alignment
 *
 * Copied out as it stands, so the padding after the root is always written.
 */
struct ctlv_event_record {
	struct list_head node;
	u32 len;
	u32 room;
	bool reserved;
	struct ctlv_event_hdr hdr;
	u8 root[] __aligned(8);
};

/**
 * struct ctlv_file - State of one open file description.
 * @ctlvdev: the device this was opened on, referenced for this object's life
 * @node: place in the device's list of open descriptions
 * @records: what this description has been sent and has not read yet
 * @queue_len: bytes it may hold, reserved storage included
 * @queue_used: bytes it holds, reserved storage included
 * @sequence: last sequence handed out, one per eligible enqueue
 * @subscribed: bit per event of the device, indexed by the op's event index
 * @overflow: the record that reports what this description was not told
 * @removed: the record that reports that there is nothing more to tell
 * @overflowing: whether the marker is pending, dropping and counting instead
 * @first_dropped: sequence of the first drop of this epoch
 * @last_dropped: sequence of the last one so far
 * @dropped: how many there have been
 * @reading_overflow: whether the marker is out being copied to userspace, the
 *	one state in which a new epoch cannot queue it
 * @first_reported: what the marker being copied out says, kept so a failed
 *	copy can merge its epoch into the one that followed
 * @last_reported: the last sequence that marker reports
 * @dropped_reported: the count it reports
 * @readers: what a blocking read and a poll wait on
 * @reading: held across one read, so two readers cannot consume one record
 *
 * Everything but @ctlvdev, @readers and @reading is under the device's
 * events_lock, and none of it is family memory: an open outlives the family.
 */
struct ctlv_file {
	struct ctlv_device *ctlvdev;
	struct list_head node;

	struct list_head records;
	u32 queue_len;
	u32 queue_used;
	u64 sequence;
	unsigned long *subscribed;
	struct ctlv_event_record *overflow;
	struct ctlv_event_record *removed;
	bool overflowing;
	u64 first_dropped;
	u64 last_dropped;
	u64 dropped;
	bool reading_overflow;
	u64 first_reported;
	u64 last_reported;
	u64 dropped_reported;

	wait_queue_head_t readers;
	/* One reader of this description at a time, however many hold it. */
	struct mutex reading;
};

/**
 * struct ctlv_error - What failed first while executing one op.
 * @reason: enum ctlv_validation_reason, zero while nothing has failed
 * @bad_attr_offset: offset of the offending attr within its message
 * @nest_offset: offset of the nest that contains it
 * @missing_attr_id: complete attr ID of a required attr that is absent
 * @expected_generation: query generation the caller resumed from
 * @current_generation: query generation the family answers at now
 *
 * Only the first failure is recorded. The two generations belong to the one
 * failure that is not about an attr, so they and the offsets above are never
 * both set.
 */
struct ctlv_error {
	u32 reason;
	u32 bad_attr_offset;
	u32 nest_offset;
	u64 missing_attr_id;
	u64 expected_generation;
	u64 current_generation;
};

/**
 * struct ctlv_frag - One fragment of a query reply, and where it stopped.
 * @generation: what the caller resumed from, zero when it started the query
 * @cursor: where the caller resumed from, zero when it started the query
 * @reported: generation the callback answered at, handed back to continue
 * @next_cursor: where a continuation resumes, zero once the reply is complete
 * @answered: whether the callback has ended its fragment, which it does once
 * @result: what ending it came to, so a callback that ignored the return of
 *	ctlv_op_query_more() cannot deliver a fragment the framework refused
 *
 * A query keeps nothing between invocations, so all of this is one execution's.
 */
struct ctlv_frag {
	u64 generation;
	u64 cursor;
	u64 reported;
	u64 next_cursor;
	bool answered;
	int result;
};

/**
 * struct ctlv_op_ctx - State of one op execution, or of one event.
 * @ctlvdev: the device being operated on, or emitting
 * @leaf: family the device answers as, resolving the extension points of every
 *	message this builds or reads
 * @file: the open file description this was submitted on, NULL for an event
 * @submitter: the task executing the ioctl, whose address space and fd table a
 *	blob descriptor names something in, NULL for an event
 * @gfp: how an event may allocate, unused by an execution
 * @op: the op being executed
 * @request: validated request, root NULL for an op with no request schema
 * @request_root: what the callback reads the request through
 * @output: the reply being built, root NULL for an op that answers with an ACK
 * @output_root: what the callback writes the reply through
 * @deepest: the innermost nest still open, @output_root when there is none. A
 *	family appends through this cursor and no other
 * @frag: continuation state of a query fragment, empty for every other op type
 * @error: first failure, for the error information a failed op may return
 *
 * A context lives for exactly one execution and is never handed to a family
 * beyond the return of its callback. @deepest may address a cursor on that
 * callback's stack, so nothing follows it once the callback has returned.
 */
struct ctlv_op_ctx {
	struct ctlv_device *ctlvdev;
	const struct ctlv_family_schema *leaf;
	struct ctlv_file *file;
	/* Compared against current and never followed: identity, not a hold. */
	const struct task_struct *submitter;
	gfp_t gfp;
	const struct ctlv_op_entry *op;

	struct ctlv_msg request;
	struct ctlv_cursor request_root;

	struct ctlv_msg output;
	struct ctlv_cursor output_root;
	struct ctlv_cursor *deepest;

	struct ctlv_frag frag;
	struct ctlv_error error;
};

int ctlv_ops_resolve(struct ctlv_device *ctlvdev,
		     const struct ctlv_family *family);
void ctlv_ops_publish(struct ctlv_device *ctlvdev);
int ctlv_uevent_check(const struct ctlv_family_schema *leaf);
struct ctlv_op_entry *ctlv_op_find(struct ctlv_device *ctlvdev, u64 op_id);

struct file;
struct poll_table_struct;

/* Events and the queue of one open file description, in event.c. */
int ctlv_events_sizes(struct ctlv_device *ctlvdev);
void ctlv_events_device_init(struct ctlv_device *ctlvdev);
void ctlv_events_device_dead(struct ctlv_device *ctlvdev);
int ctlv_events_file_init(struct ctlv_file *cfile);
void ctlv_events_file_exit(struct ctlv_file *cfile);
ssize_t ctlv_events_read(struct ctlv_file *cfile, char __user *buf,
			 size_t len, bool blocking);
__poll_t ctlv_events_poll(struct ctlv_file *cfile, struct file *filp,
			  struct poll_table_struct *wait);
void ctlv_events_op_changed(struct ctlv_device *ctlvdev,
			    const struct ctlv_op_entry *op, bool disabled,
			    u64 generation);

#endif /* _CTLV_PRIVATE_H */
