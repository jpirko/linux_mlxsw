// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Event production, queue, loss, and teardown tests.
 */

#include <kunit/static_stub.h>
#include <kunit/test.h>
#include <linux/array_size.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/completion.h>
#include <linux/ctlv.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/gfp_types.h>
#include <linux/jiffies.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/wait.h>

#include <ctlv/ctlv_fixture_gen.h>
#include <uapi/ctlv/ctlv.h>
#include <uapi/ctlv/ctlv_gen.h>

#include "../event.h"
#include "../internal.h"
#include "ctlv_fixture_gen.h"

/* A binding has to exist to be resolved; nothing here ever calls one. */
static int ctlv_event_test_callback(struct ctlv *ctlv, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request,
				    struct ctlv_cursor *reply)
{
	return 0;
}

static int ctlv_event_test_pre(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
	return 0;
}

static void ctlv_event_test_post(struct ctlv *ctlv, struct ctlv_op_ctx *ctx)
{
}

static struct ctlv_op_ctx *
ctlv_event_test_create_nomem(struct ctlv *ctlv, u64 op_id,
			     struct ctlv_cursor **payload, gfp_t gfp)
{
	return ERR_PTR(-ENOMEM);
}

static struct ctlv_event *ctlv_event_test_record_nomem(u32 room, gfp_t gfp)
{
	return NULL;
}

static struct ctlv_event *ctlv_event_test_record_unexpected(u32 room, gfp_t gfp)
{
	KUNIT_FAIL(kunit_get_current_test(), "lost event allocated an event");
	return NULL;
}

/*
 * One device of the fixture family with one open file description on it. Its
 * events are a payload-free one and one whose payload has required fields.
 */
struct ctlv_event_test_fixture {
	struct device dev;
	struct ctlv ctlv;
	struct ctlv_file cfile;
	struct ctlv_file other;
	struct ctlv_family family;
	struct ctlv_op *bindings;
	bool opened;
	bool other_opened;
};

static void ctlv_event_test_device_release(struct device *dev)
{
}

static void ctlv_event_test_fixture_close(void *arg)
{
	struct ctlv_event_test_fixture *fx = arg;

	ctlv_event_contexts_stop(&fx->ctlv);
	ctlv_event_contexts_wait(&fx->ctlv);
	if (fx->other_opened)
		ctlv_event_file_exit(&fx->other);
	if (fx->opened)
		ctlv_event_file_exit(&fx->cfile);
	ctlv_fini(&fx->ctlv);
	put_device(&fx->dev);
}

/*
 * What ctlv_init() and ctlv_publish() prepare about ops and
 * events, without the claim on a family ID that a real family may need later.
 */
static struct ctlv_event_test_fixture *ctlv_event_test_fixture_open(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx;
	u32 index, bound = 0;

	fx = kunit_kzalloc(test, sizeof(*fx), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, fx);
	fx->bindings =
		kunit_kcalloc(test, ctlv_fixture_schema.ops_count,
			      sizeof(*fx->bindings), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, fx->bindings);

	for (index = 0; index < ctlv_fixture_schema.ops_count; index++) {
		const struct ctlv_op_schema *op =
			&ctlv_fixture_op_schemas[index];

		if (op->type == CTLV_OP_TYPE_EVENT)
			continue;
		fx->bindings[bound].action = ctlv_event_test_callback;
		if (op->pre_post) {
			fx->bindings[bound].pre = ctlv_event_test_pre;
			fx->bindings[bound].post = ctlv_event_test_post;
		}
		bound++;
	}
	fx->family = (struct ctlv_family){
		.schema = &ctlv_fixture_schema,
		.parent = &ctlv_family,
		.ops = fx->bindings,
		.ops_count = bound,
	};

	KUNIT_ASSERT_EQ(test, ctlv_op_resolution_init(&fx->ctlv, &fx->family), 0);
	spin_lock_init(&fx->ctlv.transition_lock);
	ctlv_event_device_init(&fx->ctlv);
	fx->ctlv.dev = &fx->dev;
	fx->dev.release = ctlv_event_test_device_release;
	device_initialize(&fx->dev);
	kunit_add_action(test, ctlv_event_test_fixture_close, fx);
	KUNIT_ASSERT_EQ(test, ctlv_event_sizes(&fx->ctlv), 0);
	fx->ctlv.family = &fx->family;
	ctlv_op_publish_all(&fx->ctlv);
	ctlv_event_contexts_start(&fx->ctlv);

	fx->cfile.ctlv = &fx->ctlv;
	KUNIT_ASSERT_EQ(test, ctlv_event_file_init(&fx->cfile), 0);
	fx->opened = true;
	return fx;
}

static struct ctlv_file *ctlv_event_test_other_open(struct kunit *test,
					  struct ctlv_event_test_fixture *fx)
{
	fx->other.ctlv = &fx->ctlv;
	KUNIT_ASSERT_EQ(test, ctlv_event_file_init(&fx->other), 0);
	fx->other_opened = true;
	return &fx->other;
}

static void ctlv_event_test_file_subscribe(struct kunit *test, struct ctlv_event_test_fixture *fx,
				 struct ctlv_file *cfile, u64 op_id)
{
	const struct ctlv_op_entry *op = ctlv_op_find(&fx->ctlv, op_id);

	KUNIT_ASSERT_NOT_NULL(test, op);
	set_bit(op->event_index, cfile->subscribed);
}

static void ctlv_event_test_subscribe(struct kunit *test, struct ctlv_event_test_fixture *fx,
			    u64 op_id)
{
	ctlv_event_test_file_subscribe(test, fx, &fx->cfile, op_id);
}

/* The first event of the queue, or NULL, taken out of it either way. */
static struct ctlv_event *ctlv_event_test_take(struct ctlv_event_test_fixture *fx)
{
	struct ctlv_event *event;

	guard(spinlock)(&fx->ctlv.events_lock);
	event = list_first_entry_or_null(&fx->cfile.events,
					 struct ctlv_event, node);
	if (event)
		list_del_init(&event->node);
	return event;
}

static u32 ctlv_event_test_file_queued(struct ctlv_file *cfile)
{
	struct ctlv_event *event;
	u32 queued = 0;

	guard(spinlock)(&cfile->ctlv->events_lock);
	list_for_each_entry(event, &cfile->events, node)
		queued++;
	return queued;
}

static u32 ctlv_event_test_queued(struct ctlv_event_test_fixture *fx)
{
	return ctlv_event_test_file_queued(&fx->cfile);
}

struct ctlv_event_test_reader_waiter {
	struct ctlv_file *cfile;
	struct completion entered;
	struct completion done;
	int result;
};

static int ctlv_event_test_reader_wait(void *arg)
{
	struct ctlv_event_test_reader_waiter *waiter = arg;

	complete(&waiter->entered);
	waiter->result =
		wait_event_interruptible(waiter->cfile->readers,
					 ctlv_event_test_file_queued(waiter->cfile));
	complete(&waiter->done);
	return 0;
}

/* queue_min is the largest event plus the two reserved events. */
static void ctlv_event_test_sizes(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv *ctlv = &fx->ctlv;
	u32 largest = 0, index;

	for (index = 0; index < ctlv->ops_count; index++) {
		const struct ctlv_op_entry *op = &ctlv->ops[index];

		if (op->schema->type == CTLV_OP_TYPE_EVENT)
			largest = max(largest,
				      op->sizes.event_max_len);
	}
	KUNIT_EXPECT_GT(test, largest, 0);

	/* Both are the core's own ops, not the family's. */
	KUNIT_ASSERT_NOT_NULL(test, ctlv->overflow_op);
	KUNIT_ASSERT_NOT_NULL(test, ctlv->removed_op);
	KUNIT_EXPECT_EQ(test, ctlv->overflow_op->schema->op_id,
			CTLV_OP_EVENT_OVERFLOW);
	KUNIT_EXPECT_EQ(test, ctlv->removed_op->schema->op_id,
			CTLV_OP_DEVICE_REMOVED);
	KUNIT_EXPECT_EQ(test, ctlv->queue_reserved,
			ctlv->overflow_op->sizes.event_max_len +
			ctlv->removed_op->sizes.event_max_len);
	KUNIT_EXPECT_EQ(test, ctlv->queue_min,
			largest + ctlv->queue_reserved);
	KUNIT_EXPECT_GE(test, ctlv->queue_default, ctlv->queue_min);
	KUNIT_EXPECT_LE(test, ctlv->queue_default,
			CTLV_MAX_EVENT_QUEUE_LEN);

	/* An open is charged for the reserved storage from the start. */
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_len, ctlv->queue_default);
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_used, ctlv->queue_reserved);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_TRUE(test, bitmap_empty(fx->cfile.subscribed,
					     ctlv->events_count));
}

static void ctlv_event_test_creator_misuse(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	kunit_warning_suppress(test) {
		/* An op of this device that is executed rather than emitted. */
		ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_NOOP,
					&payload, GFP_KERNEL);
		KUNIT_EXPECT_EQ(test, PTR_ERR(ctx), -EINVAL);
		/* An op of a family this device is not of. */
		ctx = ctlv_event_create(&fx->ctlv,
					CTLV_OP(CTLV_FAMILY_FIXTURE + 1,
						1),
					&payload, GFP_KERNEL);
		KUNIT_EXPECT_EQ(test, PTR_ERR(ctx), -EINVAL);
		/* And one of no family at all. */
		ctx = ctlv_event_create(&fx->ctlv, 0, &payload, GFP_KERNEL);
		KUNIT_EXPECT_EQ(test, PTR_ERR(ctx), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 3);
	}
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
}

static void ctlv_event_test_a_dead_device(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;
	struct ctlv_event *event;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	WRITE_ONCE(fx->ctlv.family, NULL);
	ctlv_event_device_dead(&fx->ctlv);

	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_EXPECT_EQ(test, PTR_ERR(ctx), -ENODEV);

	/* What was queued instead, out of storage this open reserved. */
	event = ctlv_event_test_take(fx);
	KUNIT_ASSERT_NOT_NULL(test, event);
	KUNIT_EXPECT_TRUE(test, event->reserved);
	KUNIT_EXPECT_EQ(test, event->hdr.op_id, CTLV_OP_DEVICE_REMOVED);
	KUNIT_EXPECT_EQ(test, event->hdr.sequence, 1);
	KUNIT_EXPECT_EQ(test, event->hdr.length, event->len);
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_used, fx->ctlv.queue_reserved);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
}

/* Dropped without queueing, and without taking a sequence number. */
static void ctlv_event_test_an_event_nobody_wants(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);

	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_used, fx->ctlv.queue_reserved);
}

static void ctlv_event_test_explicit_loss(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_event_test_reader_waiter waiter = {
		.cfile = &fx->cfile,
	};
	struct ctlv_file *other = ctlv_event_test_other_open(test, fx);
	struct task_struct *task;
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	init_completion(&waiter.entered);
	init_completion(&waiter.done);
	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	task = kthread_run(ctlv_event_test_reader_wait, &waiter, "ctlv-event-reader");
	KUNIT_ASSERT_FALSE(test, IS_ERR(task));
	wait_for_completion(&waiter.entered);

	kunit_activate_static_stub(test, ctlv_event_create,
				   ctlv_event_test_create_nomem);
	kunit_activate_static_stub(test, ctlv_event_alloc,
				   ctlv_event_test_record_unexpected);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED,
				&payload, GFP_KERNEL);
	KUNIT_EXPECT_EQ(test, PTR_ERR(ctx), -ENOMEM);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
	ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	kunit_deactivate_static_stub(test, ctlv_event_alloc);
	kunit_deactivate_static_stub(test, ctlv_event_create);

	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&waiter.done, HZ),
			0UL);
	kthread_stop(task);
	KUNIT_EXPECT_EQ(test, waiter.result, 0);

	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED,
				&payload, GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);
	ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);

	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 3);
	KUNIT_EXPECT_TRUE(test, fx->cfile.overflowing);
	KUNIT_EXPECT_EQ(test, fx->cfile.first_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.last_dropped, 3);
	KUNIT_EXPECT_EQ(test, fx->cfile.dropped, 3);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 1);
	KUNIT_EXPECT_EQ(test, other->sequence, 0);
	KUNIT_EXPECT_FALSE(test, other->overflowing);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_file_queued(other), 0);
}

static void ctlv_event_test_full_queue_is_one_loss(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	scoped_guard(spinlock, &fx->ctlv.events_lock)
		fx->cfile.queue_len = fx->ctlv.queue_reserved;

	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);

	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 1);
	KUNIT_EXPECT_TRUE(test, fx->cfile.overflowing);
	KUNIT_EXPECT_EQ(test, fx->cfile.first_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.last_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.dropped, 1);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 1);
}

static void
ctlv_event_test_copy_allocation_failure_is_one_loss(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));

	kunit_activate_static_stub(test, ctlv_event_alloc,
				   ctlv_event_test_record_nomem);
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);
	kunit_deactivate_static_stub(test, ctlv_event_alloc);

	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 1);
	KUNIT_EXPECT_TRUE(test, fx->cfile.overflowing);
	KUNIT_EXPECT_EQ(test, fx->cfile.first_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.last_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.dropped, 1);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 1);
}

/* Subscribes fx->cfile, at the head of the file list, while emit allocates. */
static struct ctlv_event *
ctlv_event_test_record_late_subscriber(u32 room, gfp_t gfp)
{
	struct kunit *test = kunit_get_current_test();

	kunit_deactivate_static_stub(test, ctlv_event_alloc);
	ctlv_event_test_subscribe(test, test->priv, CTLV_FIXTURE_OP_PINGED);
	return ctlv_event_alloc(room, gfp);
}

/* A late subscriber gets a copy of its own, not that of one behind it. */
static void ctlv_event_test_a_late_subscriber_gets_a_copy(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_file *other = ctlv_event_test_other_open(test, fx);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctlv_event_test_file_subscribe(test, fx, other, CTLV_FIXTURE_OP_PINGED);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));

	test->priv = fx;
	kunit_activate_static_stub(test, ctlv_event_alloc,
				   ctlv_event_test_record_late_subscriber);
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);

	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 1);
	KUNIT_EXPECT_FALSE(test, fx->cfile.overflowing);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 1);
	KUNIT_EXPECT_EQ(test, other->sequence, 1);
	KUNIT_EXPECT_FALSE(test, other->overflowing);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_file_queued(other), 1);
}

static void ctlv_event_test_lost_misuse_and_teardown(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_event *event;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	kunit_warning_suppress(test) {
		ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_NOOP);
		ctlv_event_lost(&fx->ctlv, 0);
		ctlv_event_lost(&fx->ctlv,
				CTLV_OP(CTLV_FAMILY_FIXTURE + 1, 1));
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 3);
	}
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);

	ctlv_event_device_dead(&fx->ctlv);
	ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 1);

	event = ctlv_event_test_take(fx);
	KUNIT_ASSERT_NOT_NULL(test, event);
	KUNIT_EXPECT_EQ(test, event->hdr.op_id, CTLV_OP_DEVICE_REMOVED);
	KUNIT_EXPECT_TRUE(test, event->reserved);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
}

static void ctlv_event_test_op_changed_allocation_failure(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	const struct ctlv_op_entry *op;
	struct ctlv_event *event;

	ctlv_event_test_subscribe(test, fx, CTLV_OP_OP_CHANGED);
	op = ctlv_op_find(&fx->ctlv, CTLV_FIXTURE_OP_SCALARS_SET);
	KUNIT_ASSERT_NOT_NULL(test, op);

	kunit_activate_static_stub(test, ctlv_event_create,
				   ctlv_event_test_create_nomem);
	ctlv_event_op_changed(&fx->ctlv, op, true, 1);
	ctlv_event_op_changed(&fx->ctlv, op, false, 2);
	kunit_deactivate_static_stub(test, ctlv_event_create);

	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 2);
	KUNIT_EXPECT_TRUE(test, fx->cfile.overflowing);
	KUNIT_EXPECT_EQ(test, fx->cfile.first_dropped, 1);
	KUNIT_EXPECT_EQ(test, fx->cfile.last_dropped, 2);
	KUNIT_EXPECT_EQ(test, fx->cfile.dropped, 2);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 1);
	event = ctlv_event_test_take(fx);
	KUNIT_EXPECT_PTR_EQ(test, event, fx->cfile.overflow);
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_used,
			fx->ctlv.queue_reserved);
}

/* One that was asked for costs the queue what it went in as. */
static void ctlv_event_test_an_event_one_description_wants(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	const struct ctlv_op_entry *op;
	struct ctlv_event *event;
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;
	u32 used;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	used = fx->cfile.queue_used;

	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 1);

	event = ctlv_event_test_take(fx);
	KUNIT_ASSERT_NOT_NULL(test, event);
	KUNIT_EXPECT_FALSE(test, event->reserved);
	KUNIT_EXPECT_EQ(test, event->hdr.op_id, CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, event->hdr.sequence, 1);
	KUNIT_EXPECT_EQ(test, event->hdr.flags, 0);
	/* A payload-free event: a header and an empty root. */
	KUNIT_EXPECT_EQ(test, event->hdr.length,
			sizeof(struct ctlv_event_hdr) +
			sizeof(struct ctlv_hdr));
	KUNIT_EXPECT_EQ(test, fx->cfile.queue_used, used + event->len);

	op = ctlv_op_find(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_ASSERT_NOT_NULL(test, op);
	KUNIT_EXPECT_LE(test, event->len, op->sizes.event_max_len);
	kfree(event);
}

static void ctlv_event_test_an_unfinished_event(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_THING_CHANGED);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_THING_CHANGED,
				&payload, GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));

	/* Emitted without the fields its payload declares as required. */
	kunit_warning_suppress(test) {
		KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), -EINVAL);
		KUNIT_EXPECT_SUPPRESSED_WARNING_COUNT(test, 1);
	}
	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
}

static void ctlv_event_test_an_aborted_event(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *ctx;

	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_THING_CHANGED);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_THING_CHANGED,
				&payload, GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	/* Unfinished, which an aborted event need not be. */
	ctlv_event_abort(ctx);

	KUNIT_EXPECT_EQ(test, ctlv_event_test_queued(fx), 0);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);
}

struct ctlv_event_test_waiter {
	struct ctlv *ctlv;
	struct completion entered;
	struct completion done;
};

static int ctlv_event_test_context_wait(void *arg)
{
	struct ctlv_event_test_waiter *waiter = arg;

	complete(&waiter->entered);
	ctlv_event_contexts_wait(waiter->ctlv);
	complete(&waiter->done);
	return 0;
}

static void ctlv_event_test_context_lifetime(struct kunit *test)
{
	struct ctlv_event_test_fixture *fx = ctlv_event_test_fixture_open(test);
	struct ctlv_event_test_waiter waiter = {
		.ctlv = &fx->ctlv,
	};
	unsigned int refs = kref_read(&fx->dev.kobj.kref);
	const struct ctlv_op_entry *op;
	struct ctlv_event *event;
	struct task_struct *task;
	struct ctlv_cursor *payload;
	struct ctlv_op_ctx *refused;
	struct ctlv_op_ctx *ctx;

	init_completion(&waiter.entered);
	init_completion(&waiter.done);
	ctlv_event_test_subscribe(test, fx, CTLV_FIXTURE_OP_PINGED);
	ctx = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED, &payload,
				GFP_KERNEL);
	KUNIT_ASSERT_FALSE(test, IS_ERR(ctx));
	KUNIT_EXPECT_EQ(test, fx->ctlv.active_event_contexts, 1);
	KUNIT_EXPECT_EQ(test, kref_read(&fx->dev.kobj.kref), refs + 1);

	ctlv_event_contexts_stop(&fx->ctlv);
	refused = ctlv_event_create(&fx->ctlv, CTLV_FIXTURE_OP_PINGED,
				    &payload, GFP_KERNEL);
	KUNIT_EXPECT_EQ(test, PTR_ERR(refused), -ENODEV);

	task = kthread_run(ctlv_event_test_context_wait, &waiter, "ctlv-event-wait");
	if (IS_ERR(task)) {
		ctlv_event_abort(ctx);
		KUNIT_FAIL(test, "failed to start event-context waiter");
		return;
	}
	wait_for_completion(&waiter.entered);
	KUNIT_EXPECT_EQ(test, wait_for_completion_timeout(&waiter.done, 1), 0UL);

	op = ctlv_op_find(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_NOT_NULL(test, op);
	if (op) {
		scoped_guard(spinlock, &fx->ctlv.events_lock) {
			clear_bit(op->event_index, fx->cfile.subscribed);
			set_bit(op->event_index, fx->cfile.subscribed);
		}
	}
	ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 0);

	KUNIT_EXPECT_EQ(test, ctlv_event_emit(ctx), 0);
	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&waiter.done, HZ),
			0UL);
	kthread_stop(task);
	KUNIT_EXPECT_EQ(test, fx->ctlv.active_event_contexts, 0);
	KUNIT_EXPECT_EQ(test, kref_read(&fx->dev.kobj.kref), refs);

	ctlv_event_device_dead(&fx->ctlv);
	ctlv_event_lost(&fx->ctlv, CTLV_FIXTURE_OP_PINGED);
	KUNIT_EXPECT_EQ(test, fx->cfile.sequence, 2);
	event = ctlv_event_test_take(fx);
	KUNIT_ASSERT_NOT_NULL(test, event);
	KUNIT_EXPECT_EQ(test, event->hdr.op_id, CTLV_FIXTURE_OP_PINGED);
	kfree(event);
	event = ctlv_event_test_take(fx);
	KUNIT_ASSERT_NOT_NULL(test, event);
	KUNIT_EXPECT_EQ(test, event->hdr.op_id, CTLV_OP_DEVICE_REMOVED);
	KUNIT_EXPECT_TRUE(test, event->reserved);
}

static struct kunit_case ctlv_event_test_cases[] = {
	KUNIT_CASE(ctlv_event_test_sizes),
	KUNIT_CASE(ctlv_event_test_creator_misuse),
	KUNIT_CASE(ctlv_event_test_a_dead_device),
	KUNIT_CASE(ctlv_event_test_an_event_nobody_wants),
	KUNIT_CASE(ctlv_event_test_explicit_loss),
	KUNIT_CASE(ctlv_event_test_full_queue_is_one_loss),
	KUNIT_CASE(ctlv_event_test_copy_allocation_failure_is_one_loss),
	KUNIT_CASE(ctlv_event_test_a_late_subscriber_gets_a_copy),
	KUNIT_CASE(ctlv_event_test_lost_misuse_and_teardown),
	KUNIT_CASE(ctlv_event_test_op_changed_allocation_failure),
	KUNIT_CASE(ctlv_event_test_an_event_one_description_wants),
	KUNIT_CASE(ctlv_event_test_an_unfinished_event),
	KUNIT_CASE(ctlv_event_test_an_aborted_event),
	KUNIT_CASE(ctlv_event_test_context_lifetime),
	{},
};

static struct kunit_suite ctlv_event_test_suite = {
	.name = "ctlv_event",
	.test_cases = ctlv_event_test_cases,
};

kunit_test_suite(ctlv_event_test_suite);
