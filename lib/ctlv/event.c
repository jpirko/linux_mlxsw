// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Per-open event queues and loss reporting.
 */

#include <linux/bitmap.h>
#include <linux/build_bug.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
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

#include "builder.h"
#include "event.h"
#include "internal.h"
#include "message.h"
#include "schema.h"

/* Header and root form one naturally aligned copy range. */
static_assert(offsetof(struct ctlv_event, root) ==
	      offsetof(struct ctlv_event, hdr) + sizeof(struct ctlv_event_hdr));
static_assert(sizeof(struct ctlv_event_hdr) % CTLV_ALIGNTO == 0);

static u32 ctlv_event_len(const struct ctlv_op_entry *op)
{
	return op->sizes.event_max_len;
}

int ctlv_event_sizes(struct ctlv *ctlv)
{
	u32 largest = 0;
	u32 index;

	for (index = 0; index < ctlv->ops_count; index++) {
		const struct ctlv_op_entry *op = &ctlv->ops[index];

		if (op->schema->type != CTLV_OP_TYPE_EVENT)
			continue;
		largest = max(largest, ctlv_event_len(op));
		if (op->schema->op_id == CTLV_OP_EVENT_OVERFLOW)
			ctlv->overflow_op = op;
		else if (op->schema->op_id == CTLV_OP_DEVICE_REMOVED)
			ctlv->removed_op = op;
	}
	/* Every chain inherits the root, so every device has both of them. */
	if (WARN_ON_ONCE(!ctlv->overflow_op || !ctlv->removed_op))
		return -EINVAL;

	ctlv->queue_reserved =
		ctlv_event_len(ctlv->overflow_op) + ctlv_event_len(ctlv->removed_op);
	ctlv->queue_min = largest + ctlv->queue_reserved;
	if (ctlv->queue_min > CTLV_MAX_EVENT_QUEUE_LEN)
		return -EMSGSIZE;
	ctlv->queue_default =
		clamp(max(SZ_256K, 4 * largest), ctlv->queue_min, CTLV_MAX_EVENT_QUEUE_LEN);
	return 0;
}

void ctlv_event_device_init(struct ctlv *ctlv)
{
	spin_lock_init(&ctlv->events_lock);
	INIT_LIST_HEAD(&ctlv->files);
	init_waitqueue_head(&ctlv->event_context_wait);
}

void ctlv_event_contexts_start(struct ctlv *ctlv)
{
	guard(spinlock)(&ctlv->events_lock);
	if (WARN_ON_ONCE(ctlv->events_live || ctlv->active_event_contexts))
		return;
	ctlv->events_live = true;
}

void ctlv_event_contexts_stop(struct ctlv *ctlv)
{
	guard(spinlock)(&ctlv->events_lock);
	ctlv->events_live = false;
}

static bool ctlv_event_contexts_idle(struct ctlv *ctlv)
{
	guard(spinlock)(&ctlv->events_lock);
	return !ctlv->active_event_contexts;
}

void ctlv_event_contexts_wait(struct ctlv *ctlv)
{
	wait_event(ctlv->event_context_wait, ctlv_event_contexts_idle(ctlv));
}

/* @room bounds storage from the header through the root. */
static struct ctlv_event *ctlv_event_alloc(u32 room, gfp_t gfp)
{
	struct ctlv_event *event;

	event = kmalloc(struct_size(event, root, room - sizeof(event->hdr)), gfp);
	if (!event)
		return NULL;
	INIT_LIST_HEAD(&event->node);
	event->len = 0;
	event->room = room;
	event->reserved = false;
	return event;
}

/* Core events use root schemas and may outlive the device family. */
static void ctlv_event_build(struct ctlv_op_ctx *ctx, struct ctlv_event *event,
			     const struct ctlv_op_entry *op)
{
	ctx->family = &ctlv_family;
	ctx->op = op;
	ctlv_message_output_init(&ctx->output, ctx, event->root, event->room - sizeof(event->hdr),
				 &op->schema->reply);
	ctlv_message_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
}

/* The padding is written rather than left as the allocation found it. */
static void ctlv_event_done(struct ctlv_op_ctx *ctx, struct ctlv_event *event)
{
	u32 root_len = ctx->output.root->len;

	event->len = sizeof(event->hdr) + CTLV_ALIGN(root_len);
	event->hdr.length = event->len;
	memset(event->root + root_len, 0, event->len - sizeof(event->hdr) - root_len);
}

static struct ctlv_event *ctlv_event_reserve(struct ctlv *ctlv,
					     const struct ctlv_op_entry *op)
{
	struct ctlv_event *event;

	event = ctlv_event_alloc(ctlv_event_len(op), GFP_KERNEL_ACCOUNT);
	if (!event)
		return NULL;
	event->hdr.flags = 0;
	event->hdr.op_id = op->schema->op_id;
	event->reserved = true;
	return event;
}

int ctlv_event_file_init(struct ctlv_file *cfile)
{
	struct ctlv *ctlv = cfile->ctlv;
	struct ctlv_op_ctx ctx = { .ctlv = ctlv };

	INIT_LIST_HEAD(&cfile->node);
	INIT_LIST_HEAD(&cfile->events);
	init_waitqueue_head(&cfile->readers);
	mutex_init(&cfile->reading);
	cfile->queue_len = ctlv->queue_default;
	cfile->queue_used = ctlv->queue_reserved;

	cfile->subscribed = bitmap_zalloc(ctlv->events_count, GFP_KERNEL_ACCOUNT);
	cfile->overflow = ctlv_event_reserve(ctlv, ctlv->overflow_op);
	cfile->removed = ctlv_event_reserve(ctlv, ctlv->removed_op);
	if (!cfile->subscribed || !cfile->overflow || !cfile->removed) {
		ctlv_event_file_exit(cfile);
		return -ENOMEM;
	}
	ctlv_event_build(&ctx, cfile->removed, ctlv->removed_op);
	ctlv_event_done(&ctx, cfile->removed);

	guard(spinlock)(&ctlv->events_lock);
	list_add_tail(&cfile->node, &ctlv->files);
	return 0;
}

void ctlv_event_file_exit(struct ctlv_file *cfile)
{
	struct ctlv_event *event, *next;

	scoped_guard(spinlock, &cfile->ctlv->events_lock)
		list_del_init(&cfile->node);

	/* Unreachable events are ours; reserved events free by identity. */
	list_for_each_entry_safe(event, next, &cfile->events, node)
		if (!event->reserved)
			kfree(event);
	kfree(cfile->overflow);
	kfree(cfile->removed);
	bitmap_free(cfile->subscribed);
	mutex_destroy(&cfile->reading);
}

static void ctlv_event_queue_add(struct ctlv_file *cfile, struct ctlv_event *event)
{
	list_add_tail(&event->node, &cfile->events);
	wake_up_interruptible(&cfile->readers);
}

static void ctlv_event_queue_link(struct ctlv_file *cfile, struct ctlv_event *event)
{
	event->hdr.sequence = ++cfile->sequence;
	ctlv_event_queue_add(cfile, event);
}

/* One reserved marker aggregates each contiguous drop epoch. */
static void ctlv_event_queue_dropped(struct ctlv_file *cfile, u64 sequence)
{
	if (!cfile->overflowing) {
		cfile->overflowing = true;
		cfile->first_dropped = sequence;
		cfile->dropped = 0;
		if (!cfile->reading_overflow)
			ctlv_event_queue_add(cfile, cfile->overflow);
	}
	cfile->last_dropped = sequence;
	cfile->dropped++;
}

/* Delivered and dropped subscribed events both consume a sequence number. */
static void ctlv_event_queue_event(struct ctlv_file *cfile, const struct ctlv_op_entry *op,
				   struct ctlv_event *event)
{
	u64 sequence;

	lockdep_assert_held(&cfile->ctlv->events_lock);
	if (!test_bit(op->event_index, cfile->subscribed))
		return;
	sequence = ++cfile->sequence;
	if (cfile->overflowing || !event || cfile->queue_used + event->len > cfile->queue_len) {
		ctlv_event_queue_dropped(cfile, sequence);
		return;
	}
	cfile->queue_used += event->len;
	event->hdr.sequence = sequence;
	list_move_tail(&event->node, &cfile->events);
	wake_up_interruptible(&cfile->readers);
}

/* Admission pins the op until emit or abort. */
static int ctlv_event_admit(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
	const struct ctlv_family *family;

	guard(spinlock)(&ctlv->events_lock);
	if (!ctlv->events_live)
		return -ENODEV;
	if (WARN_ON_ONCE(ctlv->active_event_contexts == U32_MAX))
		return -EOVERFLOW;
	family = READ_ONCE(ctlv->family);
	if (!family)
		return -ENODEV;
	get_device(ctlv->dev);
	ctlv->active_event_contexts++;
	ctx->ctlv = ctlv;
	ctx->family = family;
	return 0;
}

struct ctlv_op_ctx *ctlv_event_create(struct ctlv *ctlv, u64 op_id,
				      struct ctlv_cursor **payload, gfp_t gfp)
{
	const struct ctlv_op_entry *op;
	u32 room;
	int ret;

	op = ctlv_op_find(ctlv, op_id);
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(op->schema->type != CTLV_OP_TYPE_EVENT))
		return ERR_PTR(-EINVAL);
	room = ctlv_event_len(op) - sizeof(struct ctlv_event_hdr);

	struct ctlv_op_ctx *ctx __free(kfree) = kzalloc_obj(*ctx, gfp);
	void *out __free(kfree) = kmalloc(room, gfp);

	if (!ctx || !out)
		return ERR_PTR(-ENOMEM);
	ret = ctlv_event_admit(ctlv, ctx);
	if (ret)
		return ERR_PTR(ret);

	ctx->gfp = gfp;
	ctx->op = op;
	ctlv_message_output_init(&ctx->output, ctx, no_free_ptr(out), room, &op->schema->reply);
	ctlv_message_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
	*payload = &ctx->output_root;
	return_ptr(ctx);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_create, "CTLV");

static void ctlv_event_free(struct ctlv_op_ctx *ctx)
{
	struct ctlv *ctlv = ctx->ctlv;
	bool wake;

	kfree(ctx->output.root);
	kfree(ctx);

	scoped_guard(spinlock, &ctlv->events_lock) {
		if (WARN_ON_ONCE(!ctlv->active_event_contexts))
			wake = true;
		else
			wake = !--ctlv->active_event_contexts;
	}
	if (wake)
		wake_up_all(&ctlv->event_context_wait);
	put_device(ctlv->dev);
}

/* Reject an incomplete or schema-invalid family event before queueing. */
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

/* Allocate each subscriber's event before taking queue spinlocks. */
static bool ctlv_event_copies(struct ctlv_op_ctx *ctx, u32 len, struct list_head *spare, u32 wanted)
{
	const struct ctlv_hdr *root = ctx->output.root;
	struct ctlv_event *event;

	while (wanted--) {
		event = ctlv_event_alloc(len, ctx->gfp | __GFP_ACCOUNT);
		if (!event)
			return false;
		event->len = len;
		event->hdr.length = len;
		event->hdr.flags = 0;
		event->hdr.op_id = ctx->op->schema->op_id;
		/* Padded, since what is copied out is the whole event. */
		memcpy_and_pad(event->root, len - sizeof(event->hdr), root, root->len, 0);
		list_add_tail(&event->node, spare);
	}
	return true;
}

static void ctlv_event_deliver(struct ctlv_op_ctx *ctx, struct list_head *spare)
{
	struct ctlv *ctlv = ctx->ctlv;
	struct ctlv_event *event;
	struct ctlv_file *cfile;

	lockdep_assert_held(&ctlv->events_lock);
	if (ctlv->dead_announced)
		return;
	list_for_each_entry(cfile, &ctlv->files, node) {
		event = list_first_entry_or_null(spare, struct ctlv_event, node);
		ctlv_event_queue_event(cfile, ctx->op, event);
	}
}

static u32 ctlv_event_subscribers(struct ctlv_op_ctx *ctx)
{
	struct ctlv *ctlv = ctx->ctlv;
	struct ctlv_file *cfile;
	u32 count = 0;

	lockdep_assert_held(&ctlv->events_lock);
	if (ctlv->dead_announced)
		return 0;
	list_for_each_entry(cfile, &ctlv->files, node)
		if (test_bit(ctx->op->event_index, cfile->subscribed))
			count++;
	return count;
}

/**
 * ctlv_event_emit - Validate and publish a completed event.
 * @ctx: context returned by a generated event create helper
 *
 * This consumes @ctx. It is not for interrupt context.
 *
 * Return: 0, or a negative error.
 */
int ctlv_event_emit(struct ctlv_op_ctx *ctx)
{
	struct ctlv *ctlv = ctx->ctlv;
	struct ctlv_event *event, *next;
	u32 len, wanted, copies = 0;
	LIST_HEAD(spare);
	bool ok;
	int ret;

	if (WARN_ON_ONCE(ctx->file))
		return -EINVAL;
	ret = ctlv_event_finish(ctx);
	if (ret)
		goto out_free;

	len = sizeof(struct ctlv_event_hdr) + CTLV_ALIGN(ctx->output.root->len);
	spin_lock(&ctlv->events_lock);
	/* Files may subscribe while the lock is dropped, so count again. */
	while ((wanted = ctlv_event_subscribers(ctx)) > copies) {
		spin_unlock(&ctlv->events_lock);
		ok = ctlv_event_copies(ctx, len, &spare, wanted - copies);
		spin_lock(&ctlv->events_lock);
		/* Subscribers without a copy see a loss. */
		if (!ok)
			break;
		copies = wanted;
	}
	if (wanted)
		ctlv_event_deliver(ctx, &spare);
	spin_unlock(&ctlv->events_lock);
	list_for_each_entry_safe(event, next, &spare, node)
		kfree(event);

out_free:
	ctlv_event_free(ctx);
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_emit, "CTLV");

/**
 * ctlv_event_abort - Discard an event under construction.
 * @ctx: context returned by a generated event create helper
 *
 * This consumes @ctx. It is not for interrupt context.
 */
void ctlv_event_abort(struct ctlv_op_ctx *ctx)
{
	if (WARN_ON_ONCE(ctx->file))
		return;
	ctlv_event_free(ctx);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_abort, "CTLV");

/**
 * ctlv_event_lost - Report an event that could not be created.
 * @ctlv: CTLV instance
 * @op_id: event operation ID
 *
 * This does not allocate or sleep. It is not for interrupt context.
 */
void ctlv_event_lost(struct ctlv *ctlv, u64 op_id)
{
	const struct ctlv_op_entry *op;
	struct ctlv_file *cfile;

	guard(spinlock)(&ctlv->events_lock);
	if (!ctlv->events_live || ctlv->dead_announced)
		return;
	op = ctlv_op_find(ctlv, op_id);
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(op->schema->type != CTLV_OP_TYPE_EVENT))
		return;
	list_for_each_entry(cfile, &ctlv->files, node)
		ctlv_event_queue_event(cfile, op, NULL);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_lost, "CTLV");

void ctlv_event_op_changed(struct ctlv *ctlv, const struct ctlv_op_entry *op,
			   bool disabled, u64 generation)
{
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctx = ctlv_op_changed_create(ctlv, &payload, GFP_NOWAIT);
	if (IS_ERR(ctx)) {
		ctlv_event_lost(ctlv, CTLV_OP_OP_CHANGED);
		return;
	}
	ctlv_op_changed_id_put(payload, op->schema->op_id);
	ctlv_op_changed_disabled_put(payload, disabled);
	ctlv_op_changed_state_generation_put(payload, generation);
	ctlv_event_emit(ctx);
}

void ctlv_event_device_dead(struct ctlv *ctlv)
{
	struct ctlv_file *cfile;

	guard(spinlock)(&ctlv->events_lock);
	if (WARN_ON_ONCE(ctlv->dead_announced))
		return;
	list_for_each_entry(cfile, &ctlv->files, node)
		ctlv_event_queue_link(cfile, cfile->removed);
	/* Publish dead only after terminal events are queued. */
	ctlv->dead_announced = true;
	list_for_each_entry(cfile, &ctlv->files, node)
		wake_up_interruptible(&cfile->readers);
}

/* Materialize the final loss count when taking the marker. */
static void ctlv_event_overflow_fill(struct ctlv_file *cfile)
{
	struct ctlv *ctlv = cfile->ctlv;
	struct ctlv_op_ctx ctx = { .ctlv = ctlv };
	struct ctlv_cursor *root;

	ctlv_event_build(&ctx, cfile->overflow, ctlv->overflow_op);
	cfile->overflow->hdr.sequence = cfile->first_dropped;
	root = &ctx.output_root;
	ctlv_event_overflow_first_sequence_put(root, cfile->first_dropped);
	ctlv_event_overflow_last_sequence_put(root, cfile->last_dropped);
	ctlv_event_overflow_dropped_count_put(root, cfile->dropped);
	/* Reserved storage is the most this event can be, so it all fits. */
	CTLV_ASSERT(!ctx.output.poison);
	ctlv_event_done(&ctx, cfile->overflow);

	cfile->reading_overflow = true;
	cfile->overflowing = false;
	cfile->first_reported = cfile->first_dropped;
	cfile->last_reported = cfile->last_dropped;
	cfile->dropped_reported = cfile->dropped;
	cfile->dropped = 0;
}

static struct ctlv_event *ctlv_event_queue_take(struct ctlv_file *cfile)
{
	struct ctlv_event *event;

	guard(spinlock)(&cfile->ctlv->events_lock);
	event = list_first_entry_or_null(&cfile->events, struct ctlv_event, node);
	if (!event) {
		/* Drained, and the device is gone. */
		if (cfile->ctlv->dead_announced)
			return ERR_PTR(-ENODEV);
		return NULL;
	}
	list_del_init(&event->node);
	if (event == cfile->overflow)
		ctlv_event_overflow_fill(cfile);
	else if (!event->reserved)
		cfile->queue_used -= event->len;
	return event;
}

/* Restore an undelivered event, merging an overflow marker's epoch. */
static void ctlv_event_queue_untake(struct ctlv_file *cfile, struct ctlv_event *event)
{
	guard(spinlock)(&cfile->ctlv->events_lock);
	if (event == cfile->overflow) {
		cfile->reading_overflow = false;
		cfile->first_dropped = cfile->first_reported;
		if (!cfile->overflowing)
			cfile->last_dropped = cfile->last_reported;
		cfile->dropped += cfile->dropped_reported;
		cfile->overflowing = true;
	} else if (!event->reserved) {
		cfile->queue_used += event->len;
	}
	list_add(&event->node, &cfile->events);
}

/* Requeue a marker if another loss epoch opened while it was out, before the removal event. */
static void ctlv_event_queue_consumed(struct ctlv_file *cfile, struct ctlv_event *event)
{
	if (event == cfile->overflow) {
		guard(spinlock)(&cfile->ctlv->events_lock);
		cfile->reading_overflow = false;
		if (!cfile->overflowing)
			return;
		if (list_empty(&cfile->removed->node))
			ctlv_event_queue_add(cfile, event);
		else
			list_add_tail(&event->node, &cfile->removed->node);
		return;
	}
	if (!event->reserved)
		kfree(event);
}

static bool ctlv_event_waiting(struct ctlv_file *cfile)
{
	struct ctlv *ctlv = cfile->ctlv;

	guard(spinlock)(&ctlv->events_lock);
	return !list_empty(&cfile->events) || ctlv->dead_announced;
}

static ssize_t ctlv_event_read_one(struct ctlv_file *cfile, char __user *buf, size_t len)
{
	struct ctlv_event *event;
	ssize_t ret;

	lockdep_assert_held(&cfile->reading);
	event = ctlv_event_queue_take(cfile);
	if (!event)
		return -EAGAIN;
	if (IS_ERR(event))
		return PTR_ERR(event);
	if (event->len > len) {
		ctlv_event_queue_untake(cfile, event);
		return -EMSGSIZE;
	}
	if (copy_to_user(buf, &event->hdr, event->len)) {
		ctlv_event_queue_untake(cfile, event);
		return -EFAULT;
	}
	ret = event->len;
	ctlv_event_queue_consumed(cfile, event);
	return ret;
}

ssize_t ctlv_event_file_read(struct ctlv_file *cfile, char __user *buf, size_t len, bool blocking)
{
	for (;;) {
		ssize_t ret;

		/* Unlocked, or a non-blocking read would wait for a blocking one. */
		if (blocking &&
		    wait_event_interruptible(cfile->readers, ctlv_event_waiting(cfile)))
			return -ERESTARTSYS;

		/* dup() and SCM_RIGHTS share one serialized queue. */
		ACQUIRE(mutex_intr, reading)(&cfile->reading);
		if (ACQUIRE_ERR(mutex_intr, &reading))
			return -ERESTARTSYS;
		ret = ctlv_event_read_one(cfile, buf, len);
		/* Another reader may have taken what woke this one. */
		if (ret != -EAGAIN || !blocking)
			return ret;
	}
}

__poll_t ctlv_event_file_poll(struct ctlv_file *cfile, struct file *filp,
			      struct poll_table_struct *wait)
{
	struct ctlv *ctlv = cfile->ctlv;
	__poll_t mask = 0;

	poll_wait(filp, &cfile->readers, wait);

	guard(spinlock)(&ctlv->events_lock);
	if (!list_empty(&cfile->events))
		mask |= EPOLLIN | EPOLLRDNORM;
	/* Which a reader sees beside what it has not drained yet. */
	if (ctlv->dead_announced)
		mask |= EPOLLHUP;
	return mask;
}

/* Validate every ID before changing any subscription. */
static const struct ctlv_op_entry *ctlv_event_subscription_find(struct ctlv *ctlv,
								u64 op_id)
{
	const struct ctlv_op_entry *op = ctlv_op_find(ctlv, op_id);

	return op && op->schema->type == CTLV_OP_TYPE_EVENT ? op : NULL;
}

static int ctlv_event_subscriptions_set(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
					bool subscribe)
{
	struct ctlv *ctlv = ctx->ctlv;
	struct ctlv_cursor cursor;

	ctlv_event_subscription_op_ids_for_each(&cursor, request) {
		u64 op_id = ctlv_event_subscription_op_ids_get(&cursor);

		if (!ctlv_event_subscription_find(ctlv, op_id))
			return ctlv_op_errorf(ctx, -EOPNOTSUPP,
					      "op %#llx is not an event this device emits",
					      (unsigned long long)op_id);
	}

	guard(spinlock)(&ctlv->events_lock);
	ctlv_event_subscription_op_ids_for_each(&cursor, request) {
		u64 op_id = ctlv_event_subscription_op_ids_get(&cursor);
		const struct ctlv_op_entry *op = ctlv_event_subscription_find(ctlv, op_id);

		assign_bit(op->event_index, ctx->file->subscribed, subscribe);
	}
	return 0;
}

int ctlv_subscriptions_add_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	return ctlv_event_subscriptions_set(ctx, request, true);
}

int ctlv_subscriptions_remove_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				     const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	return ctlv_event_subscriptions_set(ctx, request, false);
}

int ctlv_event_queue_get_query(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct ctlv_file *cfile = ctx->file;

	scoped_guard(spinlock, &ctlv->events_lock) {
		ctlv_event_queue_get_queue_len_put(reply, cfile->queue_len);
		ctlv_event_queue_get_queue_used_put(reply, cfile->queue_used);
	}
	ctlv_event_queue_get_queue_min_len_put(reply, ctx->ctlv->queue_min);
	ctlv_event_queue_get_queue_max_len_put(reply, CTLV_MAX_EVENT_QUEUE_LEN);
	/* One state, whole, so there is never a second fragment of it. */
	return ctlv_op_query_done(ctx, 0);
}

/* Resizing never discards queued events or reserved capacity. */
int ctlv_event_queue_set_action(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	u64 len = ctlv_event_queue_set_queue_len_get(request);

	if (len < ctlv->queue_min || len > CTLV_MAX_EVENT_QUEUE_LEN)
		return ctlv_op_errorf(ctx, -EINVAL, "event queue length %llu is outside %u..%u",
				      (unsigned long long)len, ctlv->queue_min,
				      CTLV_MAX_EVENT_QUEUE_LEN);

	guard(spinlock)(&ctlv->events_lock);
	if (len < ctx->file->queue_used)
		return ctlv_op_error(ctx, -EBUSY,
				     "event queue contains more data than the requested limit");
	ctx->file->queue_len = len;
	return 0;
}
