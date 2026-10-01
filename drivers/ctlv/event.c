// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Per-open event queues and loss reporting.
 */

#include <kunit/static_stub.h>
#include <kunit/visibility.h>
#include <linux/bitmap.h>
#include <linux/build_bug.h>
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
#include "device.h"
#include "event.h"
#include "internal.h"
#include "message.h"
#include "schema.h"

/* Header and root form one naturally aligned copy range. */
static_assert(offsetof(struct ctlv_event_record, root) ==
	      offsetof(struct ctlv_event_record, hdr) + sizeof(struct ctlv_event_hdr));
static_assert(sizeof(struct ctlv_event_hdr) % CTLV_ALIGNTO == 0);

static u32 ctlv_event_len(const struct ctlv_op_entry *op)
{
	return op->sizes.event_record_max_len;
}

int ctlv_event_sizes(struct ctlv_device *ctlvdev)
{
	u32 largest = 0;
	u32 index;

	for (index = 0; index < ctlvdev->ops_count; index++) {
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

	ctlvdev->queue_reserved =
		ctlv_event_len(ctlvdev->overflow_op) + ctlv_event_len(ctlvdev->removed_op);
	ctlvdev->queue_min = largest + ctlvdev->queue_reserved;
	if (ctlvdev->queue_min > CTLV_MAX_EVENT_QUEUE_LEN)
		return -EMSGSIZE;
	ctlvdev->queue_default =
		clamp(max(SZ_256K, 4 * largest), ctlvdev->queue_min, CTLV_MAX_EVENT_QUEUE_LEN);
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_sizes);

void ctlv_event_device_init(struct ctlv_device *ctlvdev)
{
	spin_lock_init(&ctlvdev->events_lock);
	INIT_LIST_HEAD(&ctlvdev->files);
	init_waitqueue_head(&ctlvdev->event_context_wait);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_device_init);

void ctlv_event_contexts_start(struct ctlv_device *ctlvdev)
{
	guard(spinlock)
		(&ctlvdev->events_lock);
	if (WARN_ON_ONCE(ctlvdev->events_live || ctlvdev->active_event_contexts))
		return;
	ctlvdev->events_live = true;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_contexts_start);

void ctlv_event_contexts_stop(struct ctlv_device *ctlvdev)
{
	guard(spinlock)
		(&ctlvdev->events_lock);
	ctlvdev->events_live = false;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_contexts_stop);

static bool ctlv_event_contexts_idle(struct ctlv_device *ctlvdev)
{
	guard(spinlock)
		(&ctlvdev->events_lock);
	return !ctlvdev->active_event_contexts;
}

void ctlv_event_contexts_wait(struct ctlv_device *ctlvdev)
{
	wait_event(ctlvdev->event_context_wait, ctlv_event_contexts_idle(ctlvdev));
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_contexts_wait);

/* @room bounds storage from the header through the root. */
VISIBLE_IF_KUNIT
struct ctlv_event_record *ctlv_event_record_alloc(u32 room, gfp_t gfp)
{
	struct ctlv_event_record *rec;

	KUNIT_STATIC_STUB_REDIRECT(ctlv_event_record_alloc, room, gfp);

	rec = kmalloc(struct_size(rec, root, room - sizeof(rec->hdr)), gfp);
	if (!rec)
		return NULL;
	INIT_LIST_HEAD(&rec->node);
	rec->len = 0;
	rec->room = room;
	rec->reserved = false;
	return rec;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_record_alloc);

/* Core events use root schemas and may outlive the device family. */
static void ctlv_event_record_build(struct ctlv_op_ctx *ctx, struct ctlv_event_record *rec,
				    const struct ctlv_op_entry *op)
{
	ctx->family = &ctlv_family;
	ctx->op = op;
	ctlv_message_output_init(&ctx->output, ctx, rec->root, rec->room - sizeof(rec->hdr),
				 &op->schema->reply);
	ctlv_message_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
}

/* The padding is written rather than left as the allocation found it. */
static void ctlv_event_record_done(struct ctlv_op_ctx *ctx, struct ctlv_event_record *rec)
{
	u32 root_len = ctx->output.root->len;

	rec->len = sizeof(rec->hdr) + CTLV_ALIGN(root_len);
	rec->hdr.length = rec->len;
	memset(rec->root + root_len, 0, rec->len - sizeof(rec->hdr) - root_len);
}

static struct ctlv_event_record *ctlv_event_record_reserve(struct ctlv_device *ctlvdev,
							   const struct ctlv_op_entry *op)
{
	struct ctlv_event_record *rec;

	rec = ctlv_event_record_alloc(ctlv_event_len(op), GFP_KERNEL_ACCOUNT);
	if (!rec)
		return NULL;
	rec->hdr.flags = 0;
	rec->hdr.op_id = op->schema->op_id;
	rec->reserved = true;
	return rec;
}

int ctlv_event_file_init(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_op_ctx ctx = { .ctlvdev = ctlvdev };

	INIT_LIST_HEAD(&cfile->node);
	INIT_LIST_HEAD(&cfile->records);
	init_waitqueue_head(&cfile->readers);
	mutex_init(&cfile->reading);
	cfile->queue_len = ctlvdev->queue_default;
	cfile->queue_used = ctlvdev->queue_reserved;

	cfile->subscribed = bitmap_zalloc(ctlvdev->events_count, GFP_KERNEL_ACCOUNT);
	cfile->overflow = ctlv_event_record_reserve(ctlvdev, ctlvdev->overflow_op);
	cfile->removed = ctlv_event_record_reserve(ctlvdev, ctlvdev->removed_op);
	if (!cfile->subscribed || !cfile->overflow || !cfile->removed) {
		ctlv_event_file_exit(cfile);
		return -ENOMEM;
	}
	ctlv_event_record_build(&ctx, cfile->removed, ctlvdev->removed_op);
	ctlv_event_record_done(&ctx, cfile->removed);

	guard(spinlock)
		(&ctlvdev->events_lock);
	list_add_tail(&cfile->node, &ctlvdev->files);
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_file_init);

void ctlv_event_file_exit(struct ctlv_file *cfile)
{
	struct ctlv_event_record *rec, *next;

	scoped_guard(spinlock, &cfile->ctlvdev->events_lock)
		list_del_init(&cfile->node);

	/* Unreachable records are ours; reserved records free by identity. */
	list_for_each_entry_safe(rec, next, &cfile->records, node)
		if (!rec->reserved)
			kfree(rec);
	kfree(cfile->overflow);
	kfree(cfile->removed);
	bitmap_free(cfile->subscribed);
	mutex_destroy(&cfile->reading);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_file_exit);

static void ctlv_event_queue_add(struct ctlv_file *cfile, struct ctlv_event_record *rec)
{
	list_add_tail(&rec->node, &cfile->records);
	wake_up_interruptible(&cfile->readers);
}

static void ctlv_event_queue_link(struct ctlv_file *cfile, struct ctlv_event_record *rec)
{
	rec->hdr.sequence = ++cfile->sequence;
	ctlv_event_queue_add(cfile, rec);
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
				   struct ctlv_event_record *rec)
{
	u64 sequence;

	lockdep_assert_held(&cfile->ctlvdev->events_lock);
	if (!test_bit(op->event_index, cfile->subscribed))
		return;
	sequence = ++cfile->sequence;
	if (cfile->overflowing || !rec || cfile->queue_used + rec->len > cfile->queue_len) {
		ctlv_event_queue_dropped(cfile, sequence);
		return;
	}
	cfile->queue_used += rec->len;
	rec->hdr.sequence = sequence;
	list_move_tail(&rec->node, &cfile->records);
	wake_up_interruptible(&cfile->readers);
}

static void ctlv_event_free(struct ctlv_op_ctx *ctx);

struct ctlv_op_ctx *ctlv_event_create(struct ctlv_device *ctlvdev, u64 op_id,
				      struct ctlv_cursor **reply, gfp_t gfp)
{
	const struct ctlv_op_entry *op;
	struct ctlv_op_ctx *ctx;
	void *out;
	u32 room;
	int ret;

	KUNIT_STATIC_STUB_REDIRECT(ctlv_event_create, ctlvdev, op_id, reply, gfp);

	ctx = kzalloc_obj(*ctx, gfp);
	if (!ctx)
		return ERR_PTR(-ENOMEM);
	scoped_guard(spinlock, &ctlvdev->events_lock) {
		const struct ctlv_family *family;

		if (!ctlvdev->events_live) {
			ret = -ENODEV;
			goto err_free_ctx;
		}
		if (WARN_ON_ONCE(ctlvdev->active_event_contexts == U32_MAX)) {
			ret = -EOVERFLOW;
			goto err_free_ctx;
		}
		/* Admission pins the op until emit or abort. */
		family = READ_ONCE(ctlvdev->family);
		if (!family) {
			ret = -ENODEV;
			goto err_free_ctx;
		}
		get_device(&ctlvdev->dev);
		ctlvdev->active_event_contexts++;
		ctx->ctlvdev = ctlvdev;
		ctx->family = family;
	}

	op = ctlv_op_find(ctlvdev, op_id);
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(op->schema->type != CTLV_OP_TYPE_EVENT)) {
		ret = -EINVAL;
		goto err_put_ctx;
	}
	room = ctlv_event_len(op) - sizeof(struct ctlv_event_hdr);
	out = kmalloc(room, gfp);
	if (!out) {
		ret = -ENOMEM;
		goto err_put_ctx;
	}

	ctx->gfp = gfp;
	ctx->op = op;
	ctlv_message_output_init(&ctx->output, ctx, out, room, &op->schema->reply);
	ctlv_message_root(&ctx->output_root, &ctx->output);
	ctx->deepest = &ctx->output_root;
	*reply = &ctx->output_root;
	return ctx;

err_put_ctx:
	ctlv_event_free(ctx);
	return ERR_PTR(ret);
err_free_ctx:
	kfree(ctx);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_create, "CTLV");

static void ctlv_event_free(struct ctlv_op_ctx *ctx)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	bool wake;

	kfree(ctx->output.root);
	kfree(ctx);

	scoped_guard(spinlock, &ctlvdev->events_lock) {
		if (WARN_ON_ONCE(!ctlvdev->active_event_contexts))
			wake = true;
		else
			wake = !--ctlvdev->active_event_contexts;
	}
	if (wake)
		wake_up_all(&ctlvdev->event_context_wait);
	put_device(&ctlvdev->dev);
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

/* Allocate subscriber records before taking queue spinlocks. */
static void ctlv_event_copies(struct ctlv_op_ctx *ctx, u32 len, struct list_head *spare, u32 wanted)
{
	const struct ctlv_attr *root = ctx->output.root;
	struct ctlv_event_record *rec;

	while (wanted--) {
		rec = ctlv_event_record_alloc(len, ctx->gfp);
		if (!rec)
			return;
		rec->len = len;
		rec->hdr.length = len;
		rec->hdr.flags = 0;
		rec->hdr.op_id = ctx->op->schema->op_id;
		/* Padded, since what is copied out is the whole record. */
		memcpy_and_pad(rec->root, len - sizeof(rec->hdr), root, root->len, 0);
		list_add_tail(&rec->node, spare);
	}
}

static void ctlv_event_deliver(struct ctlv_op_ctx *ctx, struct list_head *spare)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_event_record *rec;
	struct ctlv_file *cfile;

	guard(spinlock)
		(&ctlvdev->events_lock);
	if (ctlvdev->dead_announced)
		return;
	list_for_each_entry(cfile, &ctlvdev->files, node) {
		rec = list_first_entry_or_null(spare, struct ctlv_event_record, node);
		ctlv_event_queue_event(cfile, ctx->op, rec);
	}
}

int ctlv_event_emit(struct ctlv_op_ctx *ctx)
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
EXPORT_SYMBOL_NS_GPL(ctlv_event_emit, "CTLV");

void ctlv_event_abort(struct ctlv_op_ctx *ctx)
{
	if (WARN_ON_ONCE(ctx->file))
		return;
	ctlv_event_free(ctx);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_abort, "CTLV");

void ctlv_event_lost(struct ctlv_device *ctlvdev, u64 op_id)
{
	const struct ctlv_op_entry *op;
	struct ctlv_file *cfile;

	guard(spinlock)
		(&ctlvdev->events_lock);
	if (!ctlvdev->events_live || ctlvdev->dead_announced)
		return;
	op = ctlv_op_find(ctlvdev, op_id);
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(op->schema->type != CTLV_OP_TYPE_EVENT))
		return;
	list_for_each_entry(cfile, &ctlvdev->files, node)
		ctlv_event_queue_event(cfile, op, NULL);
}
EXPORT_SYMBOL_NS_GPL(ctlv_event_lost, "CTLV");

void ctlv_event_op_changed(struct ctlv_device *ctlvdev, const struct ctlv_op_entry *op,
			   bool disabled, u64 generation)
{
	struct ctlv_cursor *reply;
	struct ctlv_op_ctx *ctx;

	ctx = ctlv_op_changed_create(ctlvdev, &reply, GFP_NOWAIT);
	if (IS_ERR(ctx)) {
		ctlv_event_lost(ctlvdev, CTLV_OP_OP_CHANGED);
		return;
	}
	ctlv_op_id_put(reply, op->schema->op_id);
	ctlv_op_disabled_put(reply, disabled);
	ctlv_op_state_generation_put(reply, generation);
	ctlv_event_emit(ctx);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_op_changed);

void ctlv_event_device_dead(struct ctlv_device *ctlvdev)
{
	struct ctlv_file *cfile;

	guard(spinlock)
		(&ctlvdev->events_lock);
	if (WARN_ON_ONCE(ctlvdev->dead_announced))
		return;
	list_for_each_entry(cfile, &ctlvdev->files, node)
		ctlv_event_queue_link(cfile, cfile->removed);
	/* Publish dead only after terminal records are queued. */
	ctlvdev->dead_announced = true;
	list_for_each_entry(cfile, &ctlvdev->files, node)
		wake_up_interruptible(&cfile->readers);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_event_device_dead);

/* Materialize the final loss count when taking the marker. */
static void ctlv_event_overflow_fill(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_op_ctx ctx = { .ctlvdev = ctlvdev };
	struct ctlv_cursor *root;

	ctlv_event_record_build(&ctx, cfile->overflow, ctlvdev->overflow_op);
	cfile->overflow->hdr.sequence = cfile->first_dropped;
	root = &ctx.output_root;
	ctlv_event_first_sequence_put(root, cfile->first_dropped);
	ctlv_event_last_sequence_put(root, cfile->last_dropped);
	ctlv_event_dropped_count_put(root, cfile->dropped);
	/* Reserved storage is the most this event can be, so it all fits. */
	WARN_ON_ONCE(ctx.output.poison);
	ctlv_event_record_done(&ctx, cfile->overflow);

	cfile->reading_overflow = true;
	cfile->overflowing = false;
	cfile->first_reported = cfile->first_dropped;
	cfile->last_reported = cfile->last_dropped;
	cfile->dropped_reported = cfile->dropped;
	cfile->dropped = 0;
}

static struct ctlv_event_record *ctlv_event_queue_take(struct ctlv_file *cfile)
{
	struct ctlv_event_record *rec;

	guard(spinlock)
		(&cfile->ctlvdev->events_lock);
	rec = list_first_entry_or_null(&cfile->records, struct ctlv_event_record, node);
	if (!rec)
		return NULL;
	list_del_init(&rec->node);
	if (rec == cfile->overflow)
		ctlv_event_overflow_fill(cfile);
	else if (!rec->reserved)
		cfile->queue_used -= rec->len;
	return rec;
}

/* Restore an undelivered record, merging an overflow marker's epoch. */
static void ctlv_event_queue_untake(struct ctlv_file *cfile, struct ctlv_event_record *rec)
{
	guard(spinlock)
		(&cfile->ctlvdev->events_lock);
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

/* Requeue a marker if another loss epoch opened while it was out. */
static void ctlv_event_queue_consumed(struct ctlv_file *cfile, struct ctlv_event_record *rec)
{
	if (rec == cfile->overflow) {
		guard(spinlock)
			(&cfile->ctlvdev->events_lock);
		cfile->reading_overflow = false;
		if (cfile->overflowing)
			ctlv_event_queue_add(cfile, rec);
		return;
	}
	if (!rec->reserved)
		kfree(rec);
}

static bool ctlv_event_waiting(struct ctlv_file *cfile)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;

	guard(spinlock)
		(&ctlvdev->events_lock);
	return !list_empty(&cfile->records) || ctlvdev->dead_announced;
}

ssize_t ctlv_event_file_read(struct ctlv_file *cfile, char __user *buf, size_t len, bool blocking)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_event_record *rec;
	ssize_t ret;

	/* dup() and SCM_RIGHTS share one serialized queue. */
	if (mutex_lock_interruptible(&cfile->reading))
		return -ERESTARTSYS;

	while (!(rec = ctlv_event_queue_take(cfile))) {
		/* Drained, and the device is gone. */
		if (READ_ONCE(ctlvdev->dead_announced)) {
			ret = -ENODEV;
			goto out_unlock;
		}
		if (!blocking) {
			ret = -EAGAIN;
			goto out_unlock;
		}
		ret = wait_event_interruptible(cfile->readers, ctlv_event_waiting(cfile));
		if (ret) {
			ret = -ERESTARTSYS;
			goto out_unlock;
		}
	}

	if (rec->len > len) {
		ctlv_event_queue_untake(cfile, rec);
		ret = -EMSGSIZE;
		goto out_unlock;
	}
	if (copy_to_user(buf, &rec->hdr, rec->len)) {
		ctlv_event_queue_untake(cfile, rec);
		ret = -EFAULT;
		goto out_unlock;
	}
	ret = rec->len;
	ctlv_event_queue_consumed(cfile, rec);

out_unlock:
	mutex_unlock(&cfile->reading);
	return ret;
}

__poll_t ctlv_event_file_poll(struct ctlv_file *cfile, struct file *filp,
			      struct poll_table_struct *wait)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	__poll_t mask = 0;

	poll_wait(filp, &cfile->readers, wait);

	guard(spinlock)
		(&ctlvdev->events_lock);
	if (!list_empty(&cfile->records))
		mask |= EPOLLIN | EPOLLRDNORM;
	/* Which a reader sees beside what it has not drained yet. */
	if (ctlvdev->dead_announced)
		mask |= EPOLLHUP;
	return mask;
}

/* Validate every ID before changing any subscription. */
static const struct ctlv_op_entry *ctlv_event_subscription_find(struct ctlv_device *ctlvdev,
								u64 op_id)
{
	const struct ctlv_op_entry *op = ctlv_op_find(ctlvdev, op_id);

	return op && op->schema->type == CTLV_OP_TYPE_EVENT ? op : NULL;
}

static int ctlv_event_subscriptions_set(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
					struct ctlv_input_ref ref, bool subscribe)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	struct ctlv_cursor attr;

	for (ctlv_input_attr(&attr, request, ref); ctlv_attr_ok(&attr); ctlv_attr_next(&attr)) {
		u64 op_id = ctlv_attr_u64(&attr);

		if (!ctlv_event_subscription_find(ctlvdev, op_id))
			return ctlv_op_errorf(ctx, -EOPNOTSUPP,
					      "op %#llx is not an event this device emits",
					      (unsigned long long)op_id);
	}

	guard(spinlock)
		(&ctlvdev->events_lock);
	for (ctlv_input_attr(&attr, request, ref); ctlv_attr_ok(&attr); ctlv_attr_next(&attr)) {
		u64 op_id = ctlv_attr_u64(&attr);
		const struct ctlv_op_entry *op = ctlv_event_subscription_find(ctlvdev, op_id);

		if (subscribe)
			set_bit(op->event_index, ctx->file->subscribed);
		else
			clear_bit(op->event_index, ctx->file->subscribed);
	}
	return 0;
}

int ctlv_subscriptions_add_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	return ctlv_event_subscriptions_set(ctx, request, ctlv_subscriptions_add_event_op_id_ref(),
					    true);
}

int ctlv_subscriptions_remove_action(void *priv, struct ctlv_op_ctx *ctx,
				     const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	return ctlv_event_subscriptions_set(ctx, request,
					    ctlv_subscriptions_remove_event_op_id_ref(), false);
}

int ctlv_event_queue_get_query(void *priv, struct ctlv_op_ctx *ctx,
			       const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct ctlv_file *cfile = ctx->file;

	scoped_guard(spinlock, &ctx->ctlvdev->events_lock) {
		ctlv_event_queue_len_put(reply, cfile->queue_len);
		ctlv_event_queue_used_put(reply, cfile->queue_used);
	}
	/* One state, whole, so there is never a second fragment of it. */
	return ctlv_op_query_done(ctx, 0);
}

/* Resizing never discards queued records or reserved capacity. */
int ctlv_event_queue_set_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request, struct ctlv_cursor *reply)
{
	struct ctlv_device *ctlvdev = ctx->ctlvdev;
	u64 len = ctlv_event_queue_set_event_queue_len_get(request);

	if (len < ctlvdev->queue_min || len > CTLV_MAX_EVENT_QUEUE_LEN)
		return ctlv_op_errorf(ctx, -EINVAL, "event queue length %llu is outside %u..%u",
				      (unsigned long long)len, ctlvdev->queue_min,
				      CTLV_MAX_EVENT_QUEUE_LEN);

	guard(spinlock)
		(&ctlvdev->events_lock);
	if (len < ctx->file->queue_used)
		return ctlv_op_error(ctx, -EBUSY,
				     "event queue contains more data than the requested limit");
	ctx->file->queue_len = len;
	return 0;
}
