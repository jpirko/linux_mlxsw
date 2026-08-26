// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Events, and the queue of one open file description. One record per
 * subscribed description, queued whole or not at all, one per read().
 *
 * Queued bytes are charged against a per-description limit, so a description
 * that does not read cannot spend another one's memory: an event that does not
 * fit is dropped and the drop is reported. Two records are reserved at open for
 * the indications that may not be lost to memory, the drop and the removal.
 */

#include <kunit/visibility.h>
#include <linux/bitmap.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/minmax.h>
#include <linux/overflow.h>
#include <linux/poll.h>
#include <linux/sizes.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "ctlv_family_gen.h"
#include "ctlv_private.h"
#include "message.h"
#include "schema.h"

/*
 * A record is copied out as one range, so the root follows the header with
 * nothing between them, at the alignment every attr of this transport has.
 */
static_assert(offsetof(struct ctlv_event_record, root) ==
	      offsetof(struct ctlv_event_record, hdr) +
	      sizeof(struct ctlv_event_hdr));
static_assert(sizeof(struct ctlv_event_hdr) % CTLV_ALIGNTO == 0);

static u32 ctlv_event_len(const struct ctlv_op_entry *op)
{
	return op->sizes.event_record_max_len;
}

/**
 * ctlv_events_sizes - Work out what one device's queues hold.
 * @ctlvdev: device being allocated, whose op index is resolved
 *
 * The shortest queue allowed is one largest event and the two reserved
 * records. Those are resolved here because they outlive the family module.
 *
 * Return: 0, or a negative errno when this device's events do not fit a queue.
 */
int ctlv_events_sizes(struct ctlv_device *ctlvdev)
{
	u32 largest = 0;
	u32 index;

	for (index = 0; index < ctlvdev->n_ops; index++) {
		const struct ctlv_op_entry *op = &ctlvdev->ops[index];

		if (op->schema->type != CTLV_OP_TYPE_EVENT)
			continue;
		largest = max(largest, ctlv_event_len(op));
		if (op->schema->op_id == CTLV_OP_EVENT_OVERFLOW)
			ctlvdev->overflow_op = op;
		else if (op->schema->op_id == CTLV_OP_DEVICE_REMOVED)
			ctlvdev->removed_op = op;
	}
	/* Every chain inherits the root, so every device has both of them. */
	if (WARN_ON_ONCE(!ctlvdev->overflow_op || !ctlvdev->removed_op))
		return -EINVAL;

	ctlvdev->queue_reserved = ctlv_event_len(ctlvdev->overflow_op) +
				  ctlv_event_len(ctlvdev->removed_op);
	ctlvdev->queue_min = largest + ctlvdev->queue_reserved;
	if (ctlvdev->queue_min > CTLV_MAX_EVENT_QUEUE_LEN)
		return -EMSGSIZE;
	ctlvdev->queue_default = clamp(max(SZ_256K, 4 * largest),
				       ctlvdev->queue_min,
				       CTLV_MAX_EVENT_QUEUE_LEN);
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_events_sizes);

void ctlv_events_device_init(struct ctlv_device *ctlvdev)
{
	spin_lock_init(&ctlvdev->events_lock);
	INIT_LIST_HEAD(&ctlvdev->files);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_events_device_init);

/*
 * One record's worth of storage. @room is what may be written from the header
 * on, which for a reserved record is the most its event can ever come to.
 */
static struct ctlv_event_record *ctlv_record_alloc(u32 room, gfp_t gfp)
{
	struct ctlv_event_record *rec;

	rec = kmalloc(struct_size(rec, root, room - sizeof(rec->hdr)), gfp);
	if (!rec)
		return NULL;
	INIT_LIST_HEAD(&rec->node);
	rec->len = 0;
	rec->room = room;
	rec->reserved = false;
	return rec;
}

/*
 * The leaf is the framework's own, not the device's family: neither event
 * built this way has an extension point, and one outlives the family.
 */
static void ctlv_record_build(struct ctlv_op_ctx *ctx,
			      struct ctlv_event_record *rec,
			      const struct ctlv_op_entry *op)
{
	ctx->leaf = &ctlv_ctlv_schema;
	ctx->op = op;
	ctlv_msg_output(&ctx->output, ctx, rec->root,
			rec->room - sizeof(rec->hdr), &op->schema->reply);
	ctlv_msg_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
}

/* The padding is written rather than left as the allocation found it. */
static void ctlv_record_done(struct ctlv_op_ctx *ctx,
			     struct ctlv_event_record *rec)
{
	u32 root_len = ctx->output.root->len;

	rec->len = sizeof(rec->hdr) + CTLV_ALIGN(root_len);
	rec->hdr.length = rec->len;
	memset(rec->root + root_len, 0,
	       rec->len - sizeof(rec->hdr) - root_len);
}

static struct ctlv_event_record *
ctlv_record_reserve(struct ctlv_device *ctlvdev,
		    const struct ctlv_op_entry *op)
{
	struct ctlv_event_record *rec;

	rec = ctlv_record_alloc(ctlv_event_len(op), GFP_KERNEL_ACCOUNT);
	if (!rec)
		return NULL;
	rec->hdr.flags = 0;
	rec->hdr.op_id = op->schema->op_id;
	rec->reserved = true;
	return rec;
}

/**
 * ctlv_events_file_init - Give one open description its queue.
 * @cfile: description being opened, with its device already set
 *
 * Both control records are allocated and charged against the limit here: an
 * open that cannot reserve them cannot promise to report a drop.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_events_file_init(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_op_ctx ctx = { .ctlvdev = ctlvdev };

	INIT_LIST_HEAD(&cfile->node);
	INIT_LIST_HEAD(&cfile->records);
	init_waitqueue_head(&cfile->readers);
	mutex_init(&cfile->reading);
	cfile->queue_len = ctlvdev->queue_default;
	cfile->queue_used = ctlvdev->queue_reserved;

	cfile->subscribed = bitmap_zalloc(ctlvdev->n_events,
					  GFP_KERNEL_ACCOUNT);
	cfile->overflow = ctlv_record_reserve(ctlvdev, ctlvdev->overflow_op);
	cfile->removed = ctlv_record_reserve(ctlvdev, ctlvdev->removed_op);
	if (!cfile->subscribed || !cfile->overflow || !cfile->removed) {
		ctlv_events_file_exit(cfile);
		return -ENOMEM;
	}
	ctlv_record_build(&ctx, cfile->removed, ctlvdev->removed_op);
	ctlv_record_done(&ctx, cfile->removed);

	guard(spinlock)(&ctlvdev->events_lock);
	list_add_tail(&cfile->node, &ctlvdev->files);
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_events_file_init);

/**
 * ctlv_events_file_exit - Release the queue of one open description.
 * @cfile: description being released
 */
void ctlv_events_file_exit(struct ctlv_file *cfile)
{
	struct ctlv_event_record *rec, *next;

	scoped_guard(spinlock, &cfile->ctlvdev->events_lock)
		list_del_init(&cfile->node);

	/*
	 * Unreachable by now, so what is left is ours to free. A reserved
	 * record is freed by name whether it was queued or not.
	 */
	list_for_each_entry_safe(rec, next, &cfile->records, node)
		if (!rec->reserved)
			kfree(rec);
	kfree(cfile->overflow);
	kfree(cfile->removed);
	bitmap_free(cfile->subscribed);
	mutex_destroy(&cfile->reading);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_events_file_exit);

static void ctlv_queue_add(struct ctlv_file *cfile,
			   struct ctlv_event_record *rec)
{
	list_add_tail(&rec->node, &cfile->records);
	wake_up_interruptible(&cfile->readers);
}

static void ctlv_queue_link(struct ctlv_file *cfile,
			    struct ctlv_event_record *rec)
{
	rec->hdr.sequence = ++cfile->sequence;
	ctlv_queue_add(cfile, rec);
}

/*
 * The first drop of an epoch queues the reserved marker where the record would
 * have gone, unless a reader is already copying it out; every drop after that
 * is counted into it. The marker takes no sequence number of its own.
 */
static void ctlv_queue_dropped(struct ctlv_file *cfile, u64 sequence)
{
	if (!cfile->overflowing) {
		cfile->overflowing = true;
		cfile->first_dropped = sequence;
		cfile->dropped = 0;
		if (!cfile->reading_overflow)
			ctlv_queue_add(cfile, cfile->overflow);
	}
	cfile->last_dropped = sequence;
	cfile->dropped++;
}

/*
 * One subscribed event into one queue, or one drop. A sequence number is taken
 * either way, so a gap a reader sees is an event that was meant for it. @rec is
 * the emitter's next spare, moved out of its list when taken.
 */
static void ctlv_queue_event(struct ctlv_file *cfile,
			     const struct ctlv_op_entry *op,
			     struct ctlv_event_record *rec)
{
	u64 sequence;

	lockdep_assert_held(&cfile->ctlvdev->events_lock);
	if (!test_bit(op->event_index, cfile->subscribed))
		return;
	sequence = ++cfile->sequence;
	if (cfile->overflowing || !rec ||
	    cfile->queue_used + rec->len > cfile->queue_len) {
		ctlv_queue_dropped(cfile, sequence);
		return;
	}
	cfile->queue_used += rec->len;
	rec->hdr.sequence = sequence;
	list_move_tail(&rec->node, &cfile->records);
	wake_up_interruptible(&cfile->readers);
}

/**
 * ctlv_event_create - Begin one event of one device.
 * @ctlvdev: device the event is about
 * @op_id: complete ID of the event op, which the generated creator states
 * @reply: where to store the cursor the event is written through
 * @gfp: how this event may allocate, here and when it is queued
 *
 * The returned context is the caller's until it passes it to ctlv_op_emit() or
 * ctlv_op_abort(), exactly once.
 *
 * Return: the context, or an ERR_PTR.
 */
struct ctlv_op_ctx *ctlv_event_create(struct ctlv_device *ctlvdev, u64 op_id,
				      struct ctlv_cursor **reply, gfp_t gfp)
{
	const struct ctlv_op_entry *op = ctlv_op_find(ctlvdev, op_id);
	const struct ctlv_family *family;
	struct ctlv_op_ctx *ctx;
	void *out;
	u32 room;

	if (WARN_ON_ONCE(!op) ||
	    WARN_ON_ONCE(op->schema->type != CTLV_OP_TYPE_EVENT))
		return ERR_PTR(-EINVAL);
	/*
	 * Read without the registration lock: a family emitting from one of
	 * its own ops is holding that lock already.
	 */
	family = READ_ONCE(ctlvdev->family);
	if (!family)
		return ERR_PTR(-ENODEV);

	ctx = kzalloc_obj(*ctx, gfp);
	if (!ctx)
		return ERR_PTR(-ENOMEM);
	room = ctlv_event_len(op) - sizeof(struct ctlv_event_hdr);
	out = kmalloc(room, gfp);
	if (!out) {
		kfree(ctx);
		return ERR_PTR(-ENOMEM);
	}

	ctx->ctlvdev = ctlvdev;
	ctx->leaf = family->schema;
	ctx->gfp = gfp;
	ctx->op = op;
	ctlv_msg_output(&ctx->output, ctx, out, room, &op->schema->reply);
	ctlv_msg_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
	*reply = &ctx->output_root;
	return ctx;
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_create, "CTLV");

static void ctlv_event_free(struct ctlv_op_ctx *ctx)
{
	kfree(ctx->output.root);
	kfree(ctx);
}

/*
 * Closed, complete, and a message of its own schema. A failure here is the
 * emitting family's bug and nothing is queued for anyone.
 */
static int ctlv_event_finish(struct ctlv_op_ctx *ctx)
{
	struct ctlv_error refused = {};

	/* Whatever poisoned it has warned about it already. */
	if (ctx->output.poison)
		return ctx->output.poison;
	if (WARN_ON_ONCE(ctx->deepest != &ctx->output_root)) {
		/* A cursor of a frame that has returned is not followed. */
		ctx->deepest = &ctx->output_root;
		return -EINVAL;
	}
	if (WARN_ON_ONCE(ctlv_schema_validate(&ctx->output, &refused)))
		return -EINVAL;
	return 0;
}

/*
 * One record per description, allocated before any queue is locked, since a
 * queue is held under a spinlock and the emitter's gfp is its own to say.
 */
static void ctlv_event_copies(struct ctlv_op_ctx *ctx, u32 len,
			      struct list_head *spare, u32 wanted)
{
	const struct ctlv_attr *root = ctx->output.root;
	struct ctlv_event_record *rec;

	while (wanted--) {
		rec = ctlv_record_alloc(len, ctx->gfp);
		if (!rec)
			return;
		rec->len = len;
		rec->hdr.length = len;
		rec->hdr.flags = 0;
		rec->hdr.op_id = ctx->op->schema->op_id;
		/* Padded, since what is copied out is the whole record. */
		memcpy_and_pad(rec->root, len - sizeof(rec->hdr), root,
			       root->len, 0);
		list_add_tail(&rec->node, spare);
	}
}

static void ctlv_event_deliver(struct ctlv_op_ctx *ctx,
			       struct list_head *spare)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_event_record *rec;
	struct ctlv_file *cfile;

	guard(spinlock)(&ctlvdev->events_lock);
	if (ctlvdev->dead_announced)
		return;
	list_for_each_entry(cfile, &ctlvdev->files, node) {
		rec = list_first_entry_or_null(spare,
					       struct ctlv_event_record, node);
		ctlv_queue_event(cfile, ctx->op, rec);
	}
}

/**
 * ctlv_op_emit - Deliver one built event and release its context.
 * @ctx: context from a generated event creator
 *
 * Copied into the queue of every description that subscribed. A full queue
 * loses the record and says so with its marker, which is not a failure here.
 *
 * Consumes @ctx whatever it returns.
 *
 * Return: 0, or the negative errno that made the event undeliverable, always a
 * bug in the family that built it.
 */
int ctlv_op_emit(struct ctlv_op_ctx *ctx)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_event_record *rec, *next;
	struct ctlv_file *cfile;
	u32 len, wanted = 0;
	LIST_HEAD(spare);
	int ret;

	if (WARN_ON_ONCE(ctx->file))
		return -EINVAL;
	ret = ctlv_event_finish(ctx);
	if (ret)
		goto out_free;

	len = sizeof(struct ctlv_event_hdr) + CTLV_ALIGN(ctx->output.root->len);
	scoped_guard(spinlock, &ctlvdev->events_lock) {
		if (ctlvdev->dead_announced)
			goto out_free;
		list_for_each_entry(cfile, &ctlvdev->files, node)
			if (test_bit(ctx->op->event_index, cfile->subscribed))
				wanted++;
	}
	if (wanted) {
		ctlv_event_copies(ctx, len, &spare, wanted);
		ctlv_event_deliver(ctx, &spare);
		list_for_each_entry_safe(rec, next, &spare, node)
			kfree(rec);
	}

out_free:
	ctlv_event_free(ctx);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_emit, "CTLV");

/**
 * ctlv_op_abort - Throw one built event away.
 * @ctx: context from a generated event creator
 *
 * Nothing is queued and nothing is validated: an event no reader will see does
 * not have to be a message.
 */
void ctlv_op_abort(struct ctlv_op_ctx *ctx)
{
	if (WARN_ON_ONCE(ctx->file))
		return;
	ctlv_event_free(ctx);
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_abort, "CTLV");

/**
 * ctlv_events_op_changed - Report that one op of one device changed state.
 * @ctlvdev: device whose op it is
 * @op: the op, as this device resolved it
 * @disabled: the state it is in now
 * @generation: op state generation after the change
 *
 * Called with the transition serializer held, which orders the records of two
 * changes in every queue. GFP_NOWAIT: a committed change may not be held up.
 */
void ctlv_events_op_changed(struct ctlv_device *ctlvdev,
			    const struct ctlv_op_entry *op, bool disabled,
			    u64 generation)
{
	struct ctlv_cursor *reply;
	struct ctlv_op_ctx *ctx;

	ctx = ctlv_op_changed_create(ctlvdev, &reply, GFP_NOWAIT);
	if (IS_ERR(ctx))
		return;
	ctlv_op_id_put(reply, op->schema->op_id);
	ctlv_op_disabled_put(reply, disabled);
	ctlv_op_state_generation_put(reply, generation);
	ctlv_op_emit(ctx);
}

/**
 * ctlv_events_device_dead - Tell every open description that there is no more.
 * @ctlvdev: device being unregistered
 *
 * Out of storage each reserved at open, so this cannot fail and cannot be
 * dropped. A reader drains what it was sent, reads this, then gets -ENODEV.
 */
void ctlv_events_device_dead(struct ctlv_device *ctlvdev)
{
	struct ctlv_file *cfile;

	guard(spinlock)(&ctlvdev->events_lock);
	if (WARN_ON_ONCE(ctlvdev->dead_announced))
		return;
	list_for_each_entry(cfile, &ctlvdev->files, node)
		ctlv_queue_link(cfile, cfile->removed);
	/*
	 * After the records, so nothing can be queued behind the one that says
	 * there is nothing more.
	 */
	ctlvdev->dead_announced = true;
	list_for_each_entry(cfile, &ctlvdev->files, node)
		wake_up_interruptible(&cfile->readers);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_events_device_dead);

/*
 * Written when a reader takes the marker, not when it was queued, because
 * until then the epoch is still being counted up. Taking it closes the epoch.
 */
static void ctlv_overflow_fill(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_op_ctx ctx = { .ctlvdev = ctlvdev };
	struct ctlv_cursor *root;

	ctlv_record_build(&ctx, cfile->overflow, ctlvdev->overflow_op);
	cfile->overflow->hdr.sequence = cfile->first_dropped;
	root = &ctx.output_root;
	ctlv_event_first_sequence_put(root, cfile->first_dropped);
	ctlv_event_last_sequence_put(root, cfile->last_dropped);
	ctlv_event_dropped_count_put(root, cfile->dropped);
	/* Reserved storage is the most this event can be, so it all fits. */
	WARN_ON_ONCE(ctx.output.poison);
	ctlv_record_done(&ctx, cfile->overflow);

	cfile->reading_overflow = true;
	cfile->overflowing = false;
	cfile->first_reported = cfile->first_dropped;
	cfile->last_reported = cfile->last_dropped;
	cfile->dropped_reported = cfile->dropped;
	cfile->dropped = 0;
}

static struct ctlv_event_record *ctlv_queue_take(struct ctlv_file *cfile)
{
	struct ctlv_event_record *rec;

	guard(spinlock)(&cfile->ctlvdev->events_lock);
	rec = list_first_entry_or_null(&cfile->records,
				       struct ctlv_event_record, node);
	if (!rec)
		return NULL;
	list_del_init(&rec->node);
	if (rec == cfile->overflow)
		ctlv_overflow_fill(cfile);
	else if (!rec->reserved)
		cfile->queue_used -= rec->len;
	return rec;
}

/*
 * A record that could not be delivered goes back where it was. The marker
 * takes its epoch back, merged with whatever was dropped while it was out.
 */
static void ctlv_queue_untake(struct ctlv_file *cfile,
			      struct ctlv_event_record *rec)
{
	guard(spinlock)(&cfile->ctlvdev->events_lock);
	if (rec == cfile->overflow) {
		cfile->reading_overflow = false;
		cfile->first_dropped = cfile->first_reported;
		if (!cfile->overflowing)
			cfile->last_dropped = cfile->last_reported;
		cfile->dropped += cfile->dropped_reported;
		cfile->overflowing = true;
	} else if (!rec->reserved) {
		cfile->queue_used += rec->len;
	}
	list_add(&rec->node, &cfile->records);
}

/*
 * The marker is the description's storage, not one record's, so an epoch that
 * opened while it was out puts it back in line, at the end this time.
 */
static void ctlv_queue_consumed(struct ctlv_file *cfile,
				struct ctlv_event_record *rec)
{
	if (rec == cfile->overflow) {
		guard(spinlock)(&cfile->ctlvdev->events_lock);
		cfile->reading_overflow = false;
		if (cfile->overflowing)
			ctlv_queue_add(cfile, rec);
		return;
	}
	if (!rec->reserved)
		kfree(rec);
}

static bool ctlv_events_waiting(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;

	guard(spinlock)(&ctlvdev->events_lock);
	return !list_empty(&cfile->records) || ctlvdev->dead_announced;
}

/**
 * ctlv_events_read - Deliver one queued event to one reader.
 * @cfile: description being read
 * @buf: where the record goes
 * @len: bytes that may be written to @buf
 * @blocking: whether to wait for a record rather than answer -EAGAIN
 *
 * One read is one record, whole. -EMSGSIZE for a buffer too short and -EFAULT
 * for one that cannot be written, both with the record still queued.
 *
 * Return: bytes delivered, or a negative errno.
 */
ssize_t ctlv_events_read(struct ctlv_file *cfile, char __user *buf,
			 size_t len, bool blocking)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_event_record *rec;
	ssize_t ret;

	/*
	 * One reader at a time, which is what makes the record being copied
	 * out its own: a duplicated or passed descriptor shares this queue.
	 */
	if (mutex_lock_interruptible(&cfile->reading))
		return -ERESTARTSYS;

	while (!(rec = ctlv_queue_take(cfile))) {
		/* Drained, and the device is gone. */
		if (READ_ONCE(ctlvdev->dead_announced)) {
			ret = -ENODEV;
			goto out_unlock;
		}
		if (!blocking) {
			ret = -EAGAIN;
			goto out_unlock;
		}
		ret = wait_event_interruptible(cfile->readers,
					       ctlv_events_waiting(cfile));
		if (ret) {
			ret = -ERESTARTSYS;
			goto out_unlock;
		}
	}

	if (rec->len > len) {
		ctlv_queue_untake(cfile, rec);
		ret = -EMSGSIZE;
		goto out_unlock;
	}
	if (copy_to_user(buf, &rec->hdr, rec->len)) {
		ctlv_queue_untake(cfile, rec);
		ret = -EFAULT;
		goto out_unlock;
	}
	ret = rec->len;
	ctlv_queue_consumed(cfile, rec);

out_unlock:
	mutex_unlock(&cfile->reading);
	return ret;
}

/**
 * ctlv_events_poll - Whether one description has a record waiting.
 * @cfile: description being polled
 * @filp: file it was polled through
 * @wait: what to register on
 *
 * Return: the poll mask.
 */
__poll_t ctlv_events_poll(struct ctlv_file *cfile, struct file *filp,
			  struct poll_table_struct *wait)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	__poll_t mask = 0;

	poll_wait(filp, &cfile->readers, wait);

	guard(spinlock)(&ctlvdev->events_lock);
	if (!list_empty(&cfile->records))
		mask |= EPOLLIN | EPOLLRDNORM;
	/* Which a reader sees beside what it has not drained yet. */
	if (ctlvdev->dead_announced)
		mask |= EPOLLHUP;
	return mask;
}

/*
 * All of them or none: a request naming an op this device does not emit
 * changes nothing, which is what the first pass is for.
 */
static int ctlv_subscriptions_set(struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request,
				  bool subscribe)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_cursor attr;

	ctlv_event_for_each_op_id(&attr, request) {
		const struct ctlv_op_entry *op =
			ctlv_op_find(ctlvdev, ctlv_event_op_id_get(&attr));

		if (!op || op->schema->type != CTLV_OP_TYPE_EVENT)
			return -EOPNOTSUPP;
	}

	guard(spinlock)(&ctlvdev->events_lock);
	ctlv_event_for_each_op_id(&attr, request) {
		const struct ctlv_op_entry *op =
			ctlv_op_find(ctlvdev, ctlv_event_op_id_get(&attr));

		/*
		 * Including the two that arrive whether asked for or not:
		 * what a caller is owed does not depend on its subscription.
		 */
		if (subscribe)
			set_bit(op->event_index, ctx->file->subscribed);
		else
			clear_bit(op->event_index, ctx->file->subscribed);
	}
	return 0;
}

int ctlv_subscriptions_add(struct ctlv_op_ctx *ctx,
			   const struct ctlv_cursor *request,
			   struct ctlv_cursor *reply)
{
	return ctlv_subscriptions_set(ctx, request, true);
}

int ctlv_subscriptions_remove(struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply)
{
	return ctlv_subscriptions_set(ctx, request, false);
}

int ctlv_event_queue_get(struct ctlv_op_ctx *ctx,
			 const struct ctlv_cursor *request,
			 struct ctlv_cursor *reply)
{
	struct ctlv_file *cfile = ctx->file;

	scoped_guard(spinlock, &ctx->ctlvdev->events_lock) {
		ctlv_event_queue_len_put(reply, cfile->queue_len);
		ctlv_event_queue_used_put(reply, cfile->queue_used);
	}
	/* One state, whole, so there is never a second fragment of it. */
	return ctlv_op_query_done(ctx, 0);
}

/*
 * Nothing is discarded to make a queue fit: a limit below what the queue holds
 * already is refused, and so is one too short for one record.
 */
int ctlv_event_queue_set(struct ctlv_op_ctx *ctx,
			 const struct ctlv_cursor *request,
			 struct ctlv_cursor *reply)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	u64 len = ctlv_event_queue_len_get(request);

	if (len < ctlvdev->queue_min || len > CTLV_MAX_EVENT_QUEUE_LEN)
		return -EINVAL;

	guard(spinlock)(&ctlvdev->events_lock);
	if (len < ctx->file->queue_used)
		return -EBUSY;
	ctx->file->queue_len = len;
	return 0;
}
