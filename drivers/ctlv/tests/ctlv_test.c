// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV family used by tools/testing/selftests/ctlv/.
 */
#define pr_fmt(fmt) "ctlv_test: " fmt
#include <linux/atomic.h>
#include <linux/cleanup.h>
#include <linux/ctlv.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kstrtox.h>
#include <linux/minmax.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "ctlv_test.h"
#include "ctlv_test_gen.h"

struct ctlv_test_device {
	struct ctlv_device *ctlvdev;
	/* Held across the flip, which no core lock covers. */
	struct mutex toggle_lock;
	bool dynamic_disabled;
	u32 magic;
};

#define CTLV_TEST_PRIV_MAGIC 0x63746c76

size_t ctlv_test_priv_size(void)
{
	return sizeof(struct ctlv_test_device);
}

int ctlv_test_init(struct ctlv_device *ctlvdev, const void *init_info,
		   void *priv)
{
	const struct ctlv_test_init_info *info = init_info;
	struct ctlv_test_device *test = priv;

	test->ctlvdev = ctlvdev;
	test->magic = CTLV_TEST_PRIV_MAGIC;
	mutex_init(&test->toggle_lock);
	if (!info->withhold_ops) {
		ctlv_test_dynamic_noop_provide(ctlvdev);
		ctlv_test_dynamic_toggle_provide(ctlvdev);
		test->dynamic_disabled = info->dynamic_disabled;
		if (test->dynamic_disabled)
			ctlv_test_dynamic_noop_disable(ctlvdev);
	}
	return 0;
}

int ctlv_test_post_init(struct ctlv_device *ctlvdev, void *priv)
{
	struct ctlv_test_device *test = priv;

	if (WARN_ON_ONCE(test->magic != CTLV_TEST_PRIV_MAGIC))
		return -EINVAL;
	return 0;
}

void ctlv_test_pre_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct ctlv_test_device *test = priv;

	WARN_ON_ONCE(test->magic != CTLV_TEST_PRIV_MAGIC);
}

void ctlv_test_fini(struct ctlv_device *ctlvdev, void *priv)
{
	struct ctlv_test_device *test = priv;

	test->magic = 0;
	mutex_destroy(&test->toggle_lock);
}

/*
 * Two logs a selftest reads through debugfs: hooks for what the core entered,
 * echo for what the getters saw. A full one stops recording.
 */

#define CTLV_TEST_LOG_LEN 1024
#define CTLV_TEST_TOKEN_LEN 128

struct ctlv_test_log {
	/* Serializes the recording of one token against every other. */
	spinlock_t lock;
	size_t len;
	char text[CTLV_TEST_LOG_LEN];
};

static struct ctlv_test_log ctlv_test_hooks_log = {
	.lock = __SPIN_LOCK_UNLOCKED(ctlv_test_hooks_log.lock),
};

static struct ctlv_test_log ctlv_test_echo_log = {
	.lock = __SPIN_LOCK_UNLOCKED(ctlv_test_echo_log.lock),
};

/*
 * What series-dump reports. A selftest moves it on between two fragments, so
 * that a continuation of stale data has to be refused.
 */
static atomic64_t ctlv_test_series_generation = ATOMIC64_INIT(1);

/* One space-separated token, dropped whole if the log has no room for it. */
static __printf(2, 3) void ctlv_test_log_add(struct ctlv_test_log *log,
					     const char *fmt, ...)
{
	char token[CTLV_TEST_TOKEN_LEN];
	va_list args;
	size_t len;

	va_start(args, fmt);
	len = vscnprintf(token, sizeof(token), fmt, args);
	va_end(args);

	guard(spinlock)(&log->lock);
	if (log->len + len + 2 > sizeof(log->text))
		return;
	log->len += scnprintf(log->text + log->len,
			      sizeof(log->text) - log->len, "%s%s",
			      log->len ? " " : "", token);
}

/*
 * Records this family's callbacks and op hooks wherever they run. Its
 * family-wide hooks run only when this is the device family.
 */
static void ctlv_test_device_log(const struct ctlv_test_device *test, const char *what)
{
	ctlv_test_log_add(&ctlv_test_hooks_log, "%s",
			  test && test->magic == CTLV_TEST_PRIV_MAGIC ?
			  what : "wrong-device");
}

/*
 * One token per attr the getters found, in schema order, so a token missing
 * from a report is an attr the caller did not send.
 */
#define echo(fmt, ...) ctlv_test_log_add(&ctlv_test_echo_log, fmt, ##__VA_ARGS__)

static void ctlv_test_echo_scalars(const struct ctlv_cursor *request)
{
	const void *data;
	u32 len;

	echo("u=%llu", ctlv_test_echo_u_get(request));
	if (ctlv_test_echo_s_present(request))
		echo("s=%lld", ctlv_test_echo_s_get(request));
	/* An absent boolean and a present false one are different things. */
	if (ctlv_test_echo_flag_present(request))
		echo("flag=%d", ctlv_test_echo_flag_get(request));
	if (ctlv_test_echo_mode_present(request))
		echo("mode=%llu", ctlv_test_echo_mode_get(request));
	if (ctlv_test_echo_quirks_present(request))
		echo("quirks=%#llx", ctlv_test_echo_quirks_get(request));
	/* Validated as one NUL-terminated string, so printable as one. */
	if (ctlv_test_echo_text_present(request))
		echo("text=%s", ctlv_test_echo_text_get(request));
	if (ctlv_test_echo_data_present(request)) {
		data = ctlv_test_echo_data_get(request, &len);
		echo("data=%*phN", len, data);
	}
}

static void ctlv_test_echo_nests(const struct ctlv_cursor *request)
{
	struct ctlv_cursor count, item, leaf;

	ctlv_test_echo_count_for_each(&count, request)
		echo("count=%llu", ctlv_test_echo_count_get(&count));

	ctlv_test_echo_item_for_each(&item, request) {
		echo("item.index=%llu",
		     ctlv_test_echo_item_index_get(&item));
		if (ctlv_test_echo_item_label_present(&item))
			echo("item.label=%s",
			     ctlv_test_echo_item_label_get(&item));
		if (!ctlv_test_echo_item_leaf_present(&item))
			continue;
		ctlv_test_echo_item_leaf_get(&leaf, &item);
		echo("leaf.index=%llu",
		     ctlv_test_echo_item_leaf_index_get(&leaf));
	}
}

/* This op resolves nothing, so it only says a descriptor arrived. */
static void ctlv_test_echo_blobs(const struct ctlv_cursor *request)
{
	if (ctlv_test_echo_region_present(request))
		echo("region");
	if (ctlv_test_echo_page_present(request))
		echo("page");
}

/* The filling family logs members of this family's extension point. */
static void ctlv_test_echo_extension(struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request)
{
	struct ctlv_cursor extension;

	if (!ctlv_test_echo_probe_extension_present(request))
		return;
	ctlv_test_echo_probe_extension_get(&extension, request);
	echo("extension");
	ctlv_stub_read(ctx, &extension);
}

int ctlv_test_echo_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply)
{
	ctlv_test_device_log(priv, "call");
	ctlv_test_echo_scalars(request);
	ctlv_test_echo_nests(request);
	ctlv_test_echo_blobs(request);
	ctlv_test_echo_extension(ctx, request);
	return 0;
}

#undef echo

/*
 * The getters' attrs are written back through the serializer of each form. An
 * action's buffer fits its whole schema, so no append here is checked.
 */

static void ctlv_test_reply_scalars(const struct ctlv_cursor *request,
			       struct ctlv_cursor *reply)
{
	const void *data;
	u32 len;

	ctlv_test_echo_u_put(reply,
			     ctlv_test_echo_reply_echo_u_get(request));
	if (ctlv_test_echo_reply_echo_s_present(request))
		ctlv_test_echo_s_put(reply,
				     ctlv_test_echo_reply_echo_s_get(request));
	if (ctlv_test_echo_reply_echo_flag_present(request))
		ctlv_test_echo_flag_put(reply,
			ctlv_test_echo_reply_echo_flag_get(request));
	if (ctlv_test_echo_reply_echo_mode_present(request))
		ctlv_test_echo_mode_put(reply,
			ctlv_test_echo_reply_echo_mode_get(request));
	if (ctlv_test_echo_reply_echo_quirks_present(request))
		ctlv_test_echo_quirks_put(reply,
			ctlv_test_echo_reply_echo_quirks_get(request));
	if (ctlv_test_echo_reply_echo_text_present(request))
		ctlv_test_echo_text_put(reply,
			ctlv_test_echo_reply_echo_text_get(request));
	if (ctlv_test_echo_reply_echo_data_present(request)) {
		data = ctlv_test_echo_reply_echo_data_get(request, &len);
		ctlv_test_echo_data_put(reply, data, len);
	}
}

/*
 * One occurrence written per occurrence read, in order. A nest is opened onto
 * a cursor of this frame's own and closed before the next one is opened.
 */
static void ctlv_test_reply_nests(const struct ctlv_cursor *request,
			     struct ctlv_cursor *reply)
{
	struct ctlv_cursor count, item, leaf;
	struct ctlv_cursor out_item, out_leaf;
	u64 leaf_index;

	ctlv_test_echo_reply_echo_count_for_each(&count, request)
		ctlv_test_echo_count_put(reply,
			ctlv_test_echo_reply_echo_count_get(&count));

	ctlv_test_echo_reply_echo_item_for_each(&item, request) {
		if (!ctlv_test_echo_item_start(reply, &out_item))
			return;
		ctlv_test_item_index_put(&out_item,
			ctlv_test_echo_reply_item_index_get(&item));
		if (ctlv_test_echo_reply_item_label_present(&item))
			ctlv_test_item_label_put(&out_item,
				ctlv_test_echo_reply_item_label_get(&item));
		if (ctlv_test_echo_reply_item_leaf_present(&item)) {
			ctlv_test_echo_reply_item_leaf_get(&leaf, &item);
			if (ctlv_test_item_leaf_start(&out_item, &out_leaf)) {
				leaf_index =
					ctlv_test_echo_reply_item_leaf_index_get(&leaf);
				ctlv_test_item_index_put(&out_leaf, leaf_index);
				ctlv_nest_end(&out_leaf);
			}
		}
		ctlv_nest_end(&out_item);
	}
}

/*
 * The extension point is answered with what filled it, which on a device of
 * this family alone is nothing: the nest goes back as empty as it can be.
 */
static int ctlv_test_reply_extension(struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply)
{
	struct ctlv_cursor extension, out;
	int ret;

	if (!ctlv_test_echo_reply_probe_extension_present(request))
		return 0;
	ctlv_test_echo_reply_probe_extension_get(&extension, request);
	if (!ctlv_test_probe_extension_start(reply, &out))
		return 0;
	ret = ctlv_stub_fill(ctx, &extension, &out);
	ctlv_nest_end(&out);
	return ret;
}

int ctlv_test_echo_reply_action(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply)
{
	ctlv_test_device_log(priv, "call");
	ctlv_test_reply_scalars(request, reply);
	ctlv_test_reply_nests(request, reply);
	return ctlv_test_reply_extension(ctx, request, reply);
}

/* A query short enough to answer whole, so no cursor is ever needed. */
int ctlv_test_probe_get_query(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply)
{
	struct ctlv_cursor extension;
	int ret;

	ctlv_test_device_log(priv, "call");
	if (ctlv_test_probe_get_probe_token_present(request))
		ctlv_test_probe_token_put(reply,
			ctlv_test_probe_get_probe_token_get(request));
	/* This reply-only extension has no corresponding request scope. */
	if (ctlv_test_probe_extension_start(reply, &extension)) {
		ret = ctlv_stub_fill(ctx, request, &extension);
		ctlv_nest_end(&extension);
		if (ret)
			return ret;
	}
	return ctlv_op_query_done(ctx, 0);
}

/* One u64 cursor resumes chunks, items, values, then totals. */

/* How much of a series there is to answer with, which the request states. */
struct ctlv_test_series_shape {
	u64 chunks;
	u64 items;
	u64 values;
	u64 totals;
};

/* Zero child indices mean the parent nest still needs its index. */
struct ctlv_test_series_pos {
	u64 chunk;
	u64 item;
	u64 value;
	u64 total;
};

/* The nests one fragment has open, none of them deeper than the reply is. */
struct ctlv_test_series_out {
	struct ctlv_cursor chunk;
	struct ctlv_cursor item;
};

/* Four counters in one u64, none of which the request lets past eight. */
#define CTLV_TEST_SERIES_FIELD_BITS 16
#define CTLV_TEST_SERIES_FIELD_MASK ((1ULL << CTLV_TEST_SERIES_FIELD_BITS) - 1)

static u64 ctlv_test_series_cursor(const struct ctlv_test_series_pos *pos)
{
	return pos->chunk << (3 * CTLV_TEST_SERIES_FIELD_BITS) |
	       pos->item << (2 * CTLV_TEST_SERIES_FIELD_BITS) |
	       pos->value << CTLV_TEST_SERIES_FIELD_BITS | pos->total;
}

static void ctlv_test_series_resume(struct ctlv_test_series_pos *pos, u64 cursor)
{
	/* Nothing written yet, so the first chunk and the first total. */
	if (!cursor) {
		*pos = (struct ctlv_test_series_pos){ .chunk = 1, .total = 1 };
		return;
	}
	pos->chunk = (cursor >> (3 * CTLV_TEST_SERIES_FIELD_BITS)) & CTLV_TEST_SERIES_FIELD_MASK;
	pos->item = (cursor >> (2 * CTLV_TEST_SERIES_FIELD_BITS)) & CTLV_TEST_SERIES_FIELD_MASK;
	pos->value = (cursor >> CTLV_TEST_SERIES_FIELD_BITS) & CTLV_TEST_SERIES_FIELD_MASK;
	pos->total = cursor & CTLV_TEST_SERIES_FIELD_MASK;
}

/* Distinct per position, so a value also says which item wrote it. */
static u64 ctlv_test_series_value(const struct ctlv_test_series_pos *pos)
{
	return pos->chunk * 100 + pos->item * 10 + pos->value;
}

/*
 * One item, from wherever a boundary left off inside it. One a boundary fell
 * inside is opened again without its index, already written.
 */
static bool ctlv_test_series_item(struct ctlv_test_series_pos *pos,
			const struct ctlv_test_series_shape *shape,
			struct ctlv_test_series_out *out)
{
	if (!ctlv_test_series_item_start(&out->chunk, &out->item))
		return false;
	if (!pos->value) {
		if (!ctlv_test_series_index_put(&out->item, pos->item))
			return false;
		pos->value = 1;
	}
	for (; pos->value <= shape->values; pos->value++)
		if (!ctlv_test_series_value_put(&out->item,
						ctlv_test_series_value(pos)))
			return false;
	ctlv_nest_end(&out->item);
	pos->value = 0;
	return true;
}

/* One chunk of them, the same way. */
static bool ctlv_test_series_chunk(struct ctlv_test_series_pos *pos,
			 const struct ctlv_test_series_shape *shape,
			 struct ctlv_cursor *reply, struct ctlv_test_series_out *out)
{
	if (!ctlv_test_series_chunk_start(reply, &out->chunk))
		return false;
	if (!pos->item) {
		if (!ctlv_test_series_index_put(&out->chunk, pos->chunk))
			return false;
		pos->item = 1;
	}
	for (; pos->item <= shape->items; pos->item++)
		if (!ctlv_test_series_item(pos, shape, out))
			return false;
	ctlv_nest_end(&out->chunk);
	pos->item = 0;
	return true;
}

static void ctlv_test_series_shape_read(struct ctlv_test_series_shape *shape,
			      const struct ctlv_cursor *request)
{
	if (ctlv_test_series_dump_series_chunks_present(request))
		shape->chunks =
			ctlv_test_series_dump_series_chunks_get(request);
	if (ctlv_test_series_dump_series_items_present(request))
		shape->items =
			ctlv_test_series_dump_series_items_get(request);
	if (ctlv_test_series_dump_series_values_present(request))
		shape->values =
			ctlv_test_series_dump_series_values_get(request);
	if (ctlv_test_series_dump_series_totals_present(request))
		shape->totals =
			ctlv_test_series_dump_series_totals_get(request);
}

int ctlv_test_series_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				const struct ctlv_cursor *request,
				struct ctlv_cursor *reply)
{
	struct ctlv_test_series_shape shape = {};
	struct ctlv_test_series_out out = {};
	struct ctlv_test_series_pos pos = {};
	u64 generation;

	ctlv_test_device_log(priv, "call");
	ctlv_test_series_shape_read(&shape, request);
	/* Read before anything is written, so a reply is about one series. */
	generation = atomic64_read(&ctlv_test_series_generation);
	ctlv_test_series_resume(&pos, ctlv_op_query_cursor(ctx));

	for (; pos.chunk <= shape.chunks; pos.chunk++)
		if (!ctlv_test_series_chunk(&pos, &shape, reply, &out))
			goto more;
	for (; pos.total <= shape.totals; pos.total++)
		if (!ctlv_test_series_total_put(reply, pos.total))
			goto more;
	return ctlv_op_query_done(ctx, generation);

more:
	return ctlv_op_query_more(ctx, generation, ctlv_test_series_cursor(&pos));
}

/*
 * ping emits as many events as its request says, with the serial the burst
 * started from and the filler it asked for. An aborted burst is thrown away.
 */

#define CTLV_TEST_PING_FILLER_LEN 1024

/* Filled once, with the position of each byte, so a reader can check it. */
static u8 ctlv_test_ping_filler[CTLV_TEST_PING_FILLER_LEN];

/* What one burst is, which the request states. */
struct ctlv_test_ping_burst {
	u64 count;
	u64 serial;
	u64 payload;
	bool abort;
	bool empty;
};

static void ctlv_test_ping_burst_read(struct ctlv_test_ping_burst *burst,
			    const struct ctlv_cursor *request)
{
	burst->count = 1;
	if (ctlv_test_ping_count_present(request))
		burst->count = ctlv_test_ping_count_get(request);
	if (ctlv_test_ping_serial_present(request))
		burst->serial = ctlv_test_ping_serial_get(request);
	if (ctlv_test_ping_payload_present(request))
		burst->payload = ctlv_test_ping_payload_get(request);
	if (ctlv_test_ping_abort_present(request))
		burst->abort = ctlv_test_ping_abort_get(request);
	if (ctlv_test_ping_empty_present(request))
		burst->empty = ctlv_test_ping_empty_get(request);
}

static int ctlv_test_ping_event(struct ctlv_device *ctlvdev,
		      const struct ctlv_test_ping_burst *burst, u64 serial)
{
	struct ctlv_cursor *event;
	struct ctlv_op_ctx *ctx;

	if (burst->empty)
		ctx = ctlv_test_pinged_create(ctlvdev, &event, GFP_KERNEL);
	else
		ctx = ctlv_test_beeped_create(ctlvdev, &event, GFP_KERNEL);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	if (!burst->empty) {
		ctlv_test_ping_serial_put(event, serial);
		if (burst->payload)
			ctlv_test_ping_filler_put(event, ctlv_test_ping_filler,
						  burst->payload);
	}
	if (burst->abort) {
		ctlv_event_abort(ctx);
		return 0;
	}
	return ctlv_event_emit(ctx);
}

int ctlv_test_ping_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply)
{
	struct ctlv_test_device *test = priv;
	struct ctlv_test_ping_burst burst = {};
	u64 index;
	int ret;

	ctlv_test_device_log(test, "call");
	ctlv_test_ping_burst_read(&burst, request);
	for (index = 0; index < burst.count; index++) {
		ret = ctlv_test_ping_event(test->ctlvdev, &burst, burst.serial + index);
		if (ret)
			return ret;
	}
	return 0;
}

/* Checksum inputs, pattern-fill outputs, and exercise resolver edge cases. */

#define CTLV_TEST_XFER_CHUNK 4096

/* FNV-1a, so that a checksum says where a byte was and not only that it was. */
#define CTLV_TEST_FNV_OFFSET 0xcbf29ce484222325ULL
#define CTLV_TEST_FNV_PRIME 0x100000001b3ULL

static u64 ctlv_test_xfer_fold(u64 sum, const u8 *data, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
		sum = (sum ^ data[i]) * CTLV_TEST_FNV_PRIME;
	return sum;
}

/* What one transfer was asked to do, and what it has done so far. */
struct ctlv_test_xfer {
	u8 fill;
	bool skip;
	bool twice;
	bool hold;
	u64 sum;
	u64 written;
	bool distinct;
	void *chunk;
};

/*
 * A holding transfer waits for the release counter to move, so a release that
 * lands before the wait does not leave it waiting for the next one.
 */
static DECLARE_WAIT_QUEUE_HEAD(ctlv_test_transfer_wq);
static atomic_t ctlv_test_transfer_holding = ATOMIC_INIT(0);
static atomic64_t ctlv_test_transfer_release = ATOMIC64_INIT(0);
static atomic_t ctlv_test_transfer_draining = ATOMIC_INIT(0);

static int ctlv_test_xfer_hold(void)
{
	u64 seq = atomic64_read(&ctlv_test_transfer_release);
	int ret;

	atomic_inc(&ctlv_test_transfer_holding);
	wake_up(&ctlv_test_transfer_wq);
	ret = wait_event_interruptible(ctlv_test_transfer_wq,
				       atomic_read(&ctlv_test_transfer_draining) ||
				       atomic64_read(&ctlv_test_transfer_release) !=
				       seq);
	atomic_dec(&ctlv_test_transfer_holding);
	return ret;
}

static int ctlv_test_xfer_read(struct ctlv_test_xfer *xfer, struct ctlv_blob *blob)
{
	u64 len = ctlv_blob_len(blob);
	u64 offset;
	size_t part;
	int ret;

	for (offset = 0; offset < len; offset += part) {
		part = min_t(u64, len - offset, CTLV_TEST_XFER_CHUNK);
		ret = ctlv_blob_read(blob, offset, xfer->chunk, part);
		if (ret)
			return ret;
		xfer->sum = ctlv_test_xfer_fold(xfer->sum, xfer->chunk, part);
	}
	return 0;
}

static int ctlv_test_xfer_write(struct ctlv_test_xfer *xfer, struct ctlv_blob *blob)
{
	u64 len = ctlv_blob_len(blob);
	u64 offset;
	size_t part;
	int ret;

	memset(xfer->chunk, xfer->fill, CTLV_TEST_XFER_CHUNK);
	for (offset = 0; offset < len; offset += part) {
		part = min_t(u64, len - offset, CTLV_TEST_XFER_CHUNK);
		ret = ctlv_blob_write(blob, offset, xfer->chunk, part);
		if (ret)
			return ret;
		xfer->written += part;
	}
	return 0;
}

/*
 * One resolved descriptor, released here however the frame went. The hold comes
 * before the copies, so a reader afterwards sees what resolving acquired.
 */
static int ctlv_test_xfer_blob(struct ctlv_test_xfer *xfer, struct ctlv_blob *blob,
		     bool read, bool write)
{
	int put_ret;
	int ret = 0;

	if (IS_ERR(blob))
		return PTR_ERR(blob);
	if (xfer->hold)
		ret = ctlv_test_xfer_hold();
	if (!ret && read)
		ret = ctlv_test_xfer_read(xfer, blob);
	if (!ret && write)
		ret = ctlv_test_xfer_write(xfer, blob);
	put_ret = ctlv_blob_put(blob);
	if (!ret)
		ret = put_ret;
	return ret;
}

/*
 * The source, and on request the same descriptor resolved again while the
 * first is still held: the two have to be objects of their own.
 */
static int ctlv_test_xfer_source(struct ctlv_test_xfer *xfer, struct ctlv_op_ctx *ctx,
		       const struct ctlv_cursor *request)
{
	struct ctlv_blob *blob, *again;
	int ret;

	blob = ctlv_test_transfer_source_resolve(ctx, request);
	if (IS_ERR(blob))
		return PTR_ERR(blob);
	if (xfer->twice) {
		again = ctlv_test_transfer_source_resolve(ctx, request);
		if (IS_ERR(again)) {
			ctlv_blob_put(blob);
			return PTR_ERR(again);
		}
		xfer->distinct = again != blob;
		ret = ctlv_blob_put(again);
		if (ret) {
			ctlv_blob_put(blob);
			return ret;
		}
	}
	return ctlv_test_xfer_blob(xfer, blob, true, false);
}

static void ctlv_test_xfer_request_read(struct ctlv_test_xfer *xfer,
			      const struct ctlv_cursor *request)
{
	if (ctlv_test_transfer_fill_present(request))
		xfer->fill = ctlv_test_transfer_fill_get(request);
	if (ctlv_test_transfer_skip_present(request))
		xfer->skip = ctlv_test_transfer_skip_get(request);
	if (ctlv_test_transfer_twice_present(request))
		xfer->twice = ctlv_test_transfer_twice_get(request);
	if (ctlv_test_transfer_hold_present(request))
		xfer->hold = ctlv_test_transfer_hold_get(request);
}

/*
 * Every descriptor the request named, in schema order. Occurrences of piece
 * are folded in order, so the checksum also says they arrived in order.
 */
static int ctlv_test_xfer_blobs(struct ctlv_test_xfer *xfer, struct ctlv_op_ctx *ctx,
		      const struct ctlv_cursor *request)
{
	struct ctlv_cursor piece;
	int ret;

	if (ctlv_test_transfer_source_present(request)) {
		ret = ctlv_test_xfer_source(xfer, ctx, request);
		if (ret)
			return ret;
	}
	if (ctlv_test_transfer_target_present(request)) {
		ret = ctlv_test_xfer_blob(xfer,
				ctlv_test_transfer_target_resolve(ctx, request),
				false, true);
		if (ret)
			return ret;
	}
	/* Both directions on one blob, read before it is written over. */
	if (ctlv_test_transfer_scratch_present(request)) {
		ret = ctlv_test_xfer_blob(xfer,
				ctlv_test_transfer_scratch_resolve(ctx, request),
				true, true);
		if (ret)
			return ret;
	}
	ctlv_test_transfer_piece_for_each(&piece, request) {
		ret = ctlv_test_xfer_blob(xfer,
				ctlv_test_transfer_piece_resolve(ctx, &piece),
				true, false);
		if (ret)
			return ret;
	}
	return 0;
}

int ctlv_test_transfer_action(void *priv, struct ctlv_op_ctx *ctx,
			      const struct ctlv_cursor *request,
			      struct ctlv_cursor *reply)
{
	struct ctlv_test_xfer xfer = { .sum = CTLV_TEST_FNV_OFFSET };
	int ret;

	ctlv_test_device_log(priv, "call");
	ctlv_test_xfer_request_read(&xfer, request);
	/* Nothing is resolved, so a named blob holds none of its memory. */
	if (xfer.skip)
		return xfer.hold ? ctlv_test_xfer_hold() : 0;

	xfer.chunk = kmalloc(CTLV_TEST_XFER_CHUNK, GFP_KERNEL);
	if (!xfer.chunk)
		return -ENOMEM;
	ret = ctlv_test_xfer_blobs(&xfer, ctx, request);
	kfree(xfer.chunk);
	if (ret)
		return ret;

	ctlv_test_transfer_sum_put(reply, xfer.sum);
	ctlv_test_transfer_written_put(reply, xfer.written);
	if (xfer.twice)
		ctlv_test_transfer_distinct_put(reply, xfer.distinct);
	return 0;
}

/* Flips dynamic-noop on the device this was executed on, whosever it is. */
int ctlv_test_dynamic_toggle_action(void *priv, struct ctlv_op_ctx *ctx,
				    const struct ctlv_cursor *request,
				    struct ctlv_cursor *reply)
{
	struct ctlv_test_device *test = priv;

	ctlv_test_device_log(test, "call");
	guard(mutex)(&test->toggle_lock);
	test->dynamic_disabled = !test->dynamic_disabled;
	if (test->dynamic_disabled)
		ctlv_test_dynamic_noop_disable(test->ctlvdev);
	else
		ctlv_test_dynamic_noop_enable(test->ctlvdev);
	return 0;
}

int ctlv_test_noop_action(void *priv, struct ctlv_op_ctx *ctx,
			  const struct ctlv_cursor *request,
			  struct ctlv_cursor *reply)
{
	ctlv_test_device_log(priv, "call");
	return 0;
}

int ctlv_test_dynamic_noop_action(void *priv, struct ctlv_op_ctx *ctx,
				  const struct ctlv_cursor *request,
				  struct ctlv_cursor *reply)
{
	ctlv_test_device_log(priv, "call");
	return 0;
}

/*
 * The family pair wraps every non-core op on this family's devices. An op pair
 * remains with its callback on devices of an inheriting family.
 */

static bool ctlv_test_fail_family_pre;
static bool ctlv_test_fail_op_pre;

int ctlv_test_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_device_log(priv, "family-pre");
	if (ctlv_test_fail_family_pre)
		return ctlv_op_error(ctx, -EIO,
				     "test family pre hook refused the operation");
	return 0;
}

void ctlv_test_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_device_log(priv, "family-post");
}

static int ctlv_test_op_pre_common(void *priv, struct ctlv_op_ctx *ctx,
		       const char *op_name)
{
	ctlv_test_device_log(priv, "op-pre");
	if (ctlv_test_fail_op_pre)
		return ctlv_op_errorf(ctx, -EBADMSG,
				      "%s pre hook refused the operation",
				      op_name);
	return 0;
}

static void ctlv_test_op_post_common(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_device_log(priv, "op-post");
}

int ctlv_test_probe_get_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return ctlv_test_op_pre_common(priv, ctx, "probe-get");
}

void ctlv_test_probe_get_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_op_post_common(priv, ctx);
}

int ctlv_test_dynamic_noop_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return ctlv_test_op_pre_common(priv, ctx, "dynamic-noop");
}

void ctlv_test_dynamic_noop_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_op_post_common(priv, ctx);
}

int ctlv_test_series_dump_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return ctlv_test_op_pre_common(priv, ctx, "series-dump");
}

void ctlv_test_series_dump_post(void *priv, struct ctlv_op_ctx *ctx)
{
	ctlv_test_op_post_common(priv, ctx);
}

/* What every device of this family is made under, there being no hardware. */
static struct device *ctlv_test_parent;

static void ctlv_test_device_remove(struct ctlv_device *ctlvdev)
{
	ctlv_destroy(ctlvdev);
}

/* Base and inheriting devices start dynamic-noop in opposite states. */
static struct ctlv_device *ctlv_test_device_add(bool withhold_ops)
{
	struct ctlv_test_init_info init_info = {
		.withhold_ops = withhold_ops,
		.dynamic_disabled = true,
	};

	return ctlv_create(ctlv_test_parent, &ctlv_test_family,
			   &init_info);
}

static struct ctlv_device *ctlv_test_devices[CTLV_TEST_MAX_DEVICES];
/* Protects ctlv_test_devices. */
static DEFINE_MUTEX(ctlv_test_devices_lock);

static int ctlv_test_device_add_parse(const char __user *buf, size_t len,
				      unsigned int *index, bool *withhold_ops)
{
	unsigned int withhold;
	char *withhold_text;
	char *index_text;
	char text[32];
	char *at;

	if (!len || len >= sizeof(text))
		return -EINVAL;
	if (copy_from_user(text, buf, len))
		return -EFAULT;
	text[len] = '\0';
	at = skip_spaces(text);
	index_text = strsep(&at, " \t\r\n");
	if (!at)
		return -EINVAL;
	at = skip_spaces(at);
	withhold_text = strsep(&at, " \t\r\n");
	if (!*index_text || !*withhold_text ||
	    (at && *skip_spaces(at)) ||
	    kstrtouint(index_text, 10, index) ||
	    kstrtouint(withhold_text, 10, &withhold) ||
	    *index >= ARRAY_SIZE(ctlv_test_devices) || withhold > 1)
		return -EINVAL;
	*withhold_ops = withhold;
	return 0;
}

static int ctlv_test_device_remove_parse(const char __user *buf, size_t len,
					 unsigned int *index)
{
	char *index_text;
	char text[32];
	char *at;

	if (!len || len >= sizeof(text))
		return -EINVAL;
	if (copy_from_user(text, buf, len))
		return -EFAULT;
	text[len] = '\0';
	at = skip_spaces(text);
	index_text = strsep(&at, " \t\r\n");
	if (!*index_text || (at && *skip_spaces(at)) ||
	    kstrtouint(index_text, 10, index) ||
	    *index >= ARRAY_SIZE(ctlv_test_devices))
		return -EINVAL;
	return 0;
}

/* Write "<index> <withhold_ops>"; adding an existing index is a no-op. */
static ssize_t ctlv_test_device_add_write(struct file *file,
					  const char __user *buf, size_t len,
					  loff_t *pos)
{
	struct ctlv_device *ctlvdev;
	unsigned int index;
	bool withhold_ops;
	int ret;

	ret = ctlv_test_device_add_parse(buf, len, &index, &withhold_ops);
	if (ret)
		return ret;
	guard(mutex)(&ctlv_test_devices_lock);
	if (ctlv_test_devices[index])
		return len;
	ctlvdev = ctlv_test_device_add(withhold_ops);
	if (IS_ERR(ctlvdev))
		return PTR_ERR(ctlvdev);
	ctlv_test_devices[index] = ctlvdev;
	return len;
}

static const struct file_operations ctlv_test_device_add_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = ctlv_test_device_add_write,
	.llseek = noop_llseek,
};

static void ctlv_test_device_slot_remove(unsigned int index)
{
	lockdep_assert_held(&ctlv_test_devices_lock);
	if (!ctlv_test_devices[index])
		return;
	ctlv_test_device_remove(ctlv_test_devices[index]);
	ctlv_test_devices[index] = NULL;
}

static ssize_t ctlv_test_device_remove_write(struct file *file,
					     const char __user *buf,
					     size_t len, loff_t *pos)
{
	unsigned int index;
	int ret;

	ret = ctlv_test_device_remove_parse(buf, len, &index);
	if (ret)
		return ret;
	guard(mutex)(&ctlv_test_devices_lock);
	ctlv_test_device_slot_remove(index);
	return len;
}

static const struct file_operations ctlv_test_device_remove_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.write = ctlv_test_device_remove_write,
	.llseek = noop_llseek,
};

static int ctlv_test_devices_init(void)
{
	/* No hardware to be the child of, so a parent is invented. */
	ctlv_test_parent = root_device_register("ctlv_test");
	return PTR_ERR_OR_ZERO(ctlv_test_parent);
}

static void ctlv_test_devices_fini(void)
{
	unsigned int i;

	scoped_guard(mutex, &ctlv_test_devices_lock)
		for (i = 0; i < ARRAY_SIZE(ctlv_test_devices); i++)
			ctlv_test_device_slot_remove(i);
	root_device_unregister(ctlv_test_parent);
}

/* Debugfs controls failures and exposes this family's logs. */

static struct dentry *ctlv_test_debugfs;

static ssize_t ctlv_test_number_read(char __user *buf, size_t len, loff_t *pos,
				     s64 value)
{
	char text[32];
	size_t at;

	at = scnprintf(text, sizeof(text), "%lld\n", value);
	return simple_read_from_buffer(buf, len, pos, text, at);
}

/* Reading a log says what it recorded, and writing anything forgets it. */
static ssize_t ctlv_test_log_read(struct file *file, char __user *buf,
				  size_t len, loff_t *pos)
{
	struct ctlv_test_log *log = file->private_data;
	size_t at;

	/* A whole log is more than a 32-bit build wants on the stack. */
	char *text __free(kfree) =
		kmalloc(CTLV_TEST_LOG_LEN + 2, GFP_KERNEL);

	if (!text)
		return -ENOMEM;
	scoped_guard(spinlock, &log->lock)
		at = scnprintf(text, CTLV_TEST_LOG_LEN + 2, "%s\n", log->text);
	return simple_read_from_buffer(buf, len, pos, text, at);
}

static ssize_t ctlv_test_log_write(struct file *file, const char __user *buf,
				   size_t len, loff_t *pos)
{
	struct ctlv_test_log *log = file->private_data;

	guard(spinlock)(&log->lock);
	log->text[0] = '\0';
	log->len = 0;
	return len;
}

static const struct file_operations ctlv_test_log_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ctlv_test_log_read,
	.write = ctlv_test_log_write,
	.llseek = default_llseek,
};

/* Writing anything moves it on, standing in for data changing. */
static ssize_t ctlv_test_generation_read(struct file *file, char __user *buf,
					 size_t len, loff_t *pos)
{
	s64 at = atomic64_read(&ctlv_test_series_generation);

	return ctlv_test_number_read(buf, len, pos, at);
}

static ssize_t ctlv_test_generation_write(struct file *file,
					  const char __user *buf, size_t len,
					  loff_t *pos)
{
	atomic64_inc(&ctlv_test_series_generation);
	return len;
}

static const struct file_operations ctlv_test_generation_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ctlv_test_generation_read,
	.write = ctlv_test_generation_write,
	.llseek = default_llseek,
};

/*
 * Reading it says how many transfers are waiting with what they resolved still
 * held; writing anything lets all of them go.
 */
static ssize_t ctlv_test_hold_read(struct file *file, char __user *buf,
				   size_t len, loff_t *pos)
{
	int held = atomic_read(&ctlv_test_transfer_holding);

	return ctlv_test_number_read(buf, len, pos, held);
}

static void ctlv_test_transfers_release(void)
{
	atomic64_inc(&ctlv_test_transfer_release);
	wake_up_all(&ctlv_test_transfer_wq);
}

void ctlv_test_transfers_drain_begin(void)
{
	atomic_inc(&ctlv_test_transfer_draining);
	ctlv_test_transfers_release();
}
EXPORT_SYMBOL_GPL(ctlv_test_transfers_drain_begin);

void ctlv_test_transfers_drain_end(void)
{
	WARN_ON_ONCE(!atomic_add_unless(&ctlv_test_transfer_draining, -1, 0));
}
EXPORT_SYMBOL_GPL(ctlv_test_transfers_drain_end);

static ssize_t ctlv_test_hold_write(struct file *file, const char __user *buf,
				    size_t len, loff_t *pos)
{
	ctlv_test_transfers_release();
	return len;
}

static const struct file_operations ctlv_test_hold_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ctlv_test_hold_read,
	.write = ctlv_test_hold_write,
	.llseek = default_llseek,
};

static void ctlv_test_debugfs_add(void)
{
	struct dentry *dir = debugfs_create_dir("ctlv-test", NULL);

	ctlv_test_debugfs = dir;
	debugfs_create_file("device_add", 0200, dir, NULL,
			    &ctlv_test_device_add_fops);
	debugfs_create_file("device_remove", 0200, dir, NULL,
			    &ctlv_test_device_remove_fops);
	debugfs_create_file("hooks", 0600, dir, &ctlv_test_hooks_log,
			    &ctlv_test_log_fops);
	debugfs_create_file("echo", 0600, dir, &ctlv_test_echo_log,
			    &ctlv_test_log_fops);
	debugfs_create_bool("fail_family_pre", 0600, dir,
			    &ctlv_test_fail_family_pre);
	debugfs_create_bool("fail_op_pre", 0600, dir, &ctlv_test_fail_op_pre);
	debugfs_create_file("series_generation", 0600, dir, NULL,
			    &ctlv_test_generation_fops);
	debugfs_create_file("transfer_hold", 0600, dir, NULL,
			    &ctlv_test_hold_fops);
}

static int __init ctlv_test_module_init(void)
{
	int ret;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(ctlv_test_ping_filler); i++)
		ctlv_test_ping_filler[i] = i;

	ret = ctlv_test_devices_init();
	if (ret)
		return ret;
	ctlv_test_debugfs_add();
	return 0;
}

static void __exit ctlv_test_module_exit(void)
{
	ctlv_test_transfers_drain_begin();
	debugfs_remove_recursive(ctlv_test_debugfs);
	ctlv_test_devices_fini();
}

module_init(ctlv_test_module_init);
module_exit(ctlv_test_module_exit);
MODULE_DESCRIPTION("CTLV test device");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("CTLV");
