// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Per-path allocation-failure sweep. Run with lockdep, KASAN, kmemleak, and
 * panic_on_warn.
 */
#include <stdbool.h>
#include <sys/mman.h>

#include <linux/ctlv_test_gen.h>

#include "../kselftest_harness.h"
#include "ctlv_tlv.h"
#include "ctlv_util.h"

#define FAILSLAB "/sys/kernel/debug/failslab"
#define FAIL_PAGE_ALLOC "/sys/kernel/debug/fail_page_alloc"

/* The family every case here drives, the one the test module registers. */
#define FAMILY "ctlv-test"

/* Deeper than any sweep here goes, so reaching it is a bug in the case. */
#define FAIL_NTH_LIMIT 512

static bool fail_nth_configured;

/*
 * Per task, so nothing but this case loses memory. ignore-gfp-wait has to go
 * because the core's allocations are GFP_KERNEL, and the log is quiet because
 * a sweep looks for a splat rather than a record of every injection.
 */
static void __attribute__((constructor)) fail_nth_configure(void)
{
	if (ctlv_sysfs_write(FAILSLAB "/ignore-gfp-wait", "N"))
		return;
	ctlv_sysfs_write(FAILSLAB "/verbose", "0");
	ctlv_sysfs_write(FAIL_PAGE_ALLOC "/ignore-gfp-wait", "N");
	ctlv_sysfs_write(FAIL_PAGE_ALLOC "/verbose", "0");
	fail_nth_configured = true;
}

/* The countdown of one task, which is a file of that task. */
struct fail_nth {
	int fd;
};

static int fail_nth_open(struct fail_nth *nth)
{
	char path[CTLV_PATH_LEN];

	/* Every case here is single threaded, so the task is the process. */
	snprintf(path, sizeof(path), "/proc/self/task/%u/fail-nth", getpid());
	nth->fd = open(path, O_RDWR);
	return nth->fd < 0 ? -errno : 0;
}

static void fail_nth_close(struct fail_nth *nth)
{
	if (nth->fd >= 0)
		close(nth->fd);
	nth->fd = -1;
}

/* Which allocation from here on is to fail, counted from one. */
static void fail_nth_arm(struct fail_nth *nth, unsigned int which)
{
	char value[CTLV_PARAM_LEN];
	int len;

	len = snprintf(value, sizeof(value), "%u", which);
	pwrite(nth->fd, value, len, 0);
}

/*
 * Whether the arming fired. Reading allocates nothing; the write that disarms
 * does and may itself be the failing one, hence the second go.
 */
static bool fail_nth_fired(struct fail_nth *nth)
{
	char value[CTLV_PARAM_LEN];
	unsigned int tries;
	ssize_t got;

	got = pread(nth->fd, value, sizeof(value) - 1, 0);
	if (got < 0)
		return false;
	value[got] = '\0';
	for (tries = 0; tries < 2; tries++)
		if (pwrite(nth->fd, "0", 1, 0) == 1)
			break;
	return atoi(value) == 0;
}

/* One sequence with nothing failing: the device still opens and answers. */
static int fail_nth_settled(void)
{
	int fd, ret;

	fd = ctlv_open(FAMILY, 0, O_RDWR);
	if (fd < 0)
		return fd;
	ret = ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP);
	close(fd);
	return ret;
}

/*
 * One sequence each, from opening a device to closing it, so the open is swept
 * too. Nothing a step returns is looked at: giving up part way is the point.
 */

/* Opening a device, which is where a description gets its own state. */
static void case_open(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDWR);

	if (fd >= 0)
		close(fd);
}

/* An op with neither a request nor a reply, the shortest path in. */
static void case_action(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDWR);

	if (fd < 0)
		return;
	ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP);
	close(fd);
}

/* A query with a request to parse and a reply to build. */
static void case_query(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDONLY);
	struct ctlv_tlv tlv;
	__u64 reply[8];

	if (fd < 0)
		return;
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
	ctlv_exec_full(fd, CTLV_TEST_OP_PROBE_GET, tlv.buf,
		       ctlv_tlv_done(&tlv), reply, sizeof(reply));
	close(fd);
}

/* The one path that keeps a caller's state between two ioctls. */
static void case_fragmented_query(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDWR);
	struct ctlv_op_exec exec = {};
	struct ctlv_tlv tlv;
	__u64 reply[16];

	if (fd < 0)
		return;
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_CHUNKS, 4);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_ITEMS, 4);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_VALUES, 4);
	exec.op_id = CTLV_TEST_OP_SERIES_DUMP;
	exec.request = ctlv_ptr(tlv.buf);
	exec.request_len = ctlv_tlv_done(&tlv);
	exec.reply = ctlv_ptr(reply);
	exec.reply_len = sizeof(reply);
	do {
		if (ctlv_exec(fd, &exec))
			break;
	} while (exec.cursor);
	close(fd);
}

/* The largest replies the core builds, and from metadata not a family. */
static void case_introspection(void)
{
	static char reply[8192];
	struct ctlv_op_exec exec = {};
	int fd;

	fd = ctlv_open(FAMILY, 0, O_RDONLY);
	if (fd < 0)
		return;
	exec.op_id = CTLV_OP_OPS_DUMP;
	ctlv_exec_set_empty_request(&exec);
	exec.reply = ctlv_ptr(reply);
	exec.reply_len = sizeof(reply);
	do {
		if (ctlv_exec(fd, &exec))
			break;
	} while (exec.cursor);
	close(fd);
}

/* Subscribe, size a queue, build a burst, read one back. */
static void case_event(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDWR | O_NONBLOCK);
	char record[512];
	struct ctlv_tlv tlv;

	if (fd < 0)
		return;
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_OP_ID, CTLV_TEST_OP_BEEPED);
	if (ctlv_exec_request(fd, CTLV_OP_SUBSCRIPTIONS_ADD, tlv.buf,
			      ctlv_tlv_done(&tlv))) {
		close(fd);
		return;
	}
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_QUEUE_LEN, 1 << 16);
	ctlv_exec_request(fd, CTLV_OP_EVENT_QUEUE_SET, tlv.buf,
			  ctlv_tlv_done(&tlv));
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_COUNT, 2);
	ctlv_exec_request(fd, CTLV_TEST_OP_PING, tlv.buf,
			  ctlv_tlv_done(&tlv));
	read(fd, record, sizeof(record));
	close(fd);
}

/* A case takes no arguments, so the blob case carries its fd here. */
static int case_blob_fd = -1;
#define CASE_BLOB_LEN (1 << 16)

/* Where the core pins a caller's pages and must give them all back. */
static void case_blob(void)
{
	int fd = ctlv_open(FAMILY, 0, O_RDWR);
	struct ctlv_tlv tlv;
	__u64 reply[16];

	if (fd < 0)
		return;
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, case_blob_fd,
			    0, CASE_BLOB_LEN);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, 0);
	ctlv_exec_full(fd, CTLV_TEST_OP_TRANSFER, tlv.buf,
		       ctlv_tlv_done(&tlv), reply, sizeof(reply));
	close(fd);
}

FIXTURE(fail_nth) {
	struct fail_nth nth;
};

FIXTURE_SETUP(fail_nth)
{
	int ret;

	self->nth.fd = -1;
	if (!fail_nth_configured)
		SKIP(return, "no fault injection: %s is not writable",
		     FAILSLAB);
	ret = fail_nth_settled();
	if (ret)
		SKIP(return, "no %s device: %s", FAMILY, strerror(-ret));
	ret = fail_nth_open(&self->nth);
	if (ret)
		SKIP(return, "no fail-nth for this task: %s", strerror(-ret));
}

FIXTURE_TEARDOWN(fail_nth)
{
	if (self->nth.fd >= 0)
		fail_nth_arm(&self->nth, 0);
	fail_nth_close(&self->nth);
}

/*
 * The same sequence over and over, one allocation further in each time. The
 * check between two armings is what names the arming that broke the core.
 */
static void fail_nth_sweep(struct __test_metadata *_metadata,
			   struct fail_nth *nth, void (*run)(void))
{
	unsigned int which;

	for (which = 1; which <= FAIL_NTH_LIMIT; which++) {
		fail_nth_arm(nth, which);
		run();
		if (!fail_nth_fired(nth))
			break;
		ASSERT_EQ(0, fail_nth_settled()) {
			TH_LOG("unusable after failing allocation %u", which);
		}
	}
	ASSERT_GT(FAIL_NTH_LIMIT, which) {
		TH_LOG("%u allocations deep and still going", which);
	}
	TH_LOG("swept %u allocations", which - 1);
}

TEST_F(fail_nth, an_open)
{
	fail_nth_sweep(_metadata, &self->nth, case_open);
}

TEST_F(fail_nth, an_action)
{
	fail_nth_sweep(_metadata, &self->nth, case_action);
}

TEST_F(fail_nth, a_query)
{
	fail_nth_sweep(_metadata, &self->nth, case_query);
}

TEST_F(fail_nth, a_fragmented_query)
{
	fail_nth_sweep(_metadata, &self->nth, case_fragmented_query);
}

TEST_F(fail_nth, an_introspection_query)
{
	fail_nth_sweep(_metadata, &self->nth, case_introspection);
}

TEST_F(fail_nth, an_event)
{
	fail_nth_sweep(_metadata, &self->nth, case_event);
}

TEST_F(fail_nth, a_blob)
{
	case_blob_fd = memfd_create("ctlv-fail-nth", MFD_CLOEXEC);
	ASSERT_LE(0, case_blob_fd);
	ASSERT_EQ(0, ftruncate(case_blob_fd, CASE_BLOB_LEN));
	fail_nth_sweep(_metadata, &self->nth, case_blob);
	close(case_blob_fd);
	case_blob_fd = -1;
}

TEST_HARNESS_MAIN
