// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Wire-format tests across base and inheriting families.
 */
#include <libgen.h>
#include <limits.h>
#include <poll.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <linux/ctlv_test_child_gen.h>
#include <linux/ctlv_test_gen.h>
#include <linux/dma-heap.h>
#include <linux/udmabuf.h>

#include "../kselftest_harness.h"
#include "ctlv_tlv.h"
#include "ctlv_util.h"

/* An op ID no family can have: no such family may be registered. */
#define CTLV_OP_ID_NOBODY CTLV_OP(0xfffe, 1)

FIXTURE(device) {
	int fd;
	/* Room for any reply either family can produce. */
	char reply[1024];
};

FIXTURE_VARIANT(device) {
	const char *family;
	/* What dynamic-noop answers before anything has toggled it. */
	bool dynamic_disabled;
	/* The output buffer a query of this family insists on. */
	__u32 query_reply_min;
	/* What a query answers into a buffer of exactly that size. */
	const char *query_truncated;
	/* The output buffer echo-reply insists on, its maximum. */
	__u32 echo_reply_max;
	/* Whether the inheriting family's own op is one of this family's. */
	bool inheriting;
};

/* The root of the chain: 16 bytes of reply root plus a 24-byte token. */
FIXTURE_VARIANT_ADD(device, parent) {
	.family = "ctlv-test",
	.dynamic_disabled = true,
	.query_reply_min = 40,
	.query_truncated = "token=7",
	.echo_reply_max = 512,
	.inheriting = false,
};

/* Fills the extension point its parent left empty, so its reply grows. */
FIXTURE_VARIANT_ADD(device, child) {
	.family = "ctlv-test-child",
	.dynamic_disabled = false,
	.query_reply_min = 56,
	.query_truncated = "token=7 extension={}",
	.echo_reply_max = 536,
	.inheriting = true,
};

FIXTURE_SETUP(device)
{
	self->fd = ctlv_open(variant->family, 0, O_RDWR);
	if (self->fd < 0)
		SKIP(return, "no %s device: %s", variant->family,
		     strerror(-self->fd));
}

FIXTURE_TEARDOWN(device)
{
	if (self->fd >= 0)
		close(self->fd);
	/* A case that ended early must not leave a hook refusing everything. */
	ctlv_sysfs_write(CTLV_TEST_FAIL_FAMILY_PRE, "0");
	ctlv_sysfs_write(CTLV_TEST_CHILD_FAIL_FAMILY_PRE, "0");
	ctlv_sysfs_write(CTLV_TEST_FAIL_OP_PRE, "0");
}

static int dynamic_result(bool disabled)
{
	return disabled ? -EOPNOTSUPP : 0;
}

/* Flips dynamic-noop on one device, into a buffer just large enough. */
static int dynamic_toggle(int fd)
{
	char reply[64];

	return ctlv_exec_reply(fd, CTLV_TEST_OP_DYNAMIC_TOGGLE, reply,
			       sizeof(reply));
}

/* One recorded line, which is one op's tokens in the order it entered them. */
static void hooks_line(char *out, size_t len, const char *const *tokens)
{
	size_t used = 0;

	for (; *tokens; tokens++)
		used += snprintf(out + used, len - used, "%s%s",
				 used ? " " : "", *tokens);
}

/*
 * The declaring family's callbacks and op hooks, plus its family-wide hooks
 * when it is also the device family.
 */
#define EXPECT_HOOKS(...) EXPECT_HOOKS_TIMES(1, __VA_ARGS__)

/* And the same chain again for each of @_times invocations of the op. */
#define EXPECT_HOOKS_TIMES(_times, ...) do {				\
	const char *const tokens[] = { __VA_ARGS__, NULL };		\
	char one[CTLV_LOG_LEN], expected[CTLV_LOG_LEN] = "";		\
	char actual[CTLV_LOG_LEN];					\
	unsigned int _n, _used = 0;					\
									\
	hooks_line(one, sizeof(one), tokens);				\
	for (_n = 0; _n < (_times); _n++)				\
		_used += snprintf(expected + _used,			\
				  sizeof(expected) - _used, "%s%s",	\
				  _used ? " " : "", one);		\
	ASSERT_EQ(0, ctlv_hooks_read(actual, sizeof(actual)));		\
	EXPECT_STREQ(expected, actual);					\
} while (0)

/* Nothing at all, which is what an op of another family records here. */
#define EXPECT_NO_HOOKS() do {						\
	char actual[CTLV_LOG_LEN];					\
									\
	ASSERT_EQ(0, ctlv_hooks_read(actual, sizeof(actual)));		\
	EXPECT_STREQ("", actual);					\
} while (0)

/* The child device family's family-wide hooks and local callbacks. */
#define EXPECT_CHILD_HOOKS_TIMES(_times, ...) do {			\
	const char *const tokens[] = { __VA_ARGS__, NULL };		\
	char one[CTLV_LOG_LEN], expected[CTLV_LOG_LEN] = "";		\
	char actual[CTLV_LOG_LEN];					\
	unsigned int _n, _used = 0;					\
									\
	hooks_line(one, sizeof(one), tokens);				\
	for (_n = 0; _n < (_times); _n++)				\
		_used += snprintf(expected + _used,			\
				  sizeof(expected) - _used, "%s%s",	\
				  _used ? " " : "", one);		\
	ASSERT_EQ(0, ctlv_child_hooks_read(actual, sizeof(actual)));	\
	EXPECT_STREQ(expected, actual);					\
} while (0)

#define EXPECT_CHILD_HOOKS(...) EXPECT_CHILD_HOOKS_TIMES(1, __VA_ARGS__)

#define EXPECT_NO_CHILD_HOOKS() do {					\
	char actual[CTLV_LOG_LEN];					\
									\
	ASSERT_EQ(0, ctlv_child_hooks_read(actual, sizeof(actual)));	\
	EXPECT_STREQ("", actual);					\
} while (0)

TEST_F(device, an_unknown_ioctl)
{
	EXPECT_EQ(-1, ioctl(self->fd, _IO(CTLV_IOC_MAGIC, 0x7f)));
	EXPECT_EQ(ENOTTY, errno);
}

TEST_F(device, an_argument_that_is_not_readable)
{
	EXPECT_EQ(-1, ioctl(self->fd, CTLV_IOC_OP_EXEC, NULL));
	EXPECT_EQ(EFAULT, errno);
}

/* What a compatible growth of this ioctl will use, so nothing may set it. */
TEST_F(device, reserved_fields)
{
	struct ctlv_op_exec exec = { .op_id = CTLV_TEST_OP_NOOP };
	unsigned int index;

	ctlv_exec_set_empty_request(&exec);
	ASSERT_EQ(0, ctlv_exec(self->fd, &exec));
	for (index = 0; index < ARRAY_SIZE(exec.reserved); index++) {
		memset(exec.reserved, 0, sizeof(exec.reserved));
		exec.reserved[index] = 1;
		EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &exec)) {
			TH_LOG("reserved[%u]", index);
		}
	}
}

/* Only a query resumes, and only from somewhere it has been told to. */
TEST_F(device, continuation_fields)
{
	struct ctlv_op_exec query = {
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.reply = ctlv_ptr(self->reply),
		.reply_len = variant->query_reply_min,
	};
	struct ctlv_op_exec action = { .op_id = CTLV_TEST_OP_NOOP };

	ctlv_exec_set_empty_request(&query);
	ctlv_exec_set_empty_request(&action);
	action.generation = 1;
	EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &action));
	action.generation = 0;
	action.cursor = 1;
	EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &action));

	/* A generation with no cursor is continuing what never started. */
	query.generation = 1;
	EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &query));

	/* What a cursor means is the family's: probe-get restarts. */
	query.generation = 0;
	query.cursor = 1;
	EXPECT_EQ(0, ctlv_exec(self->fd, &query));
	EXPECT_EQ(0, query.cursor);

	/* And a caller claiming a generation it was never given is stale. */
	query.generation = 1;
	query.cursor = 1;
	EXPECT_EQ(-ESTALE, ctlv_exec(self->fd, &query));
}

TEST_F(device, an_op_that_answers_with_an_ack)
{
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
}

/* Zero is what an unset field looks like, and no family numbers an op zero. */
TEST_F(device, op_id_zero)
{
	EXPECT_EQ(-EINVAL, ctlv_exec_bare(self->fd, 0));
}

TEST_F(device, an_op_of_a_family_that_does_not_exist)
{
	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(self->fd, CTLV_OP_ID_NOBODY));
}

TEST_F(device, an_op_number_this_family_does_not_have)
{
	__u64 op_id = CTLV_OP(CTLV_FAMILY_TEST, 0xffff);

	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(self->fd, op_id));
}

/* A child answers its parent's ops and its own; a parent not its child's. */
TEST_F(device, an_op_of_the_inheriting_family)
{
	int expected = variant->inheriting ? 0 : -EOPNOTSUPP;

	EXPECT_EQ(expected,
		  ctlv_exec_bare(self->fd, CTLV_TEST_CHILD_OP_EXTRA_NOOP));
}

/* An event is delivered to a reader and is not an op anyone may execute. */
TEST_F(device, an_event_is_not_executable)
{
	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(self->fd, CTLV_TEST_OP_PINGED));
}

TEST_F(device, an_observation_file_may_use_read_access_ops)
{
	int fd = ctlv_open(variant->family, 0, O_RDONLY);

	ASSERT_LE(0, fd);
	EXPECT_EQ(0, ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP));
	EXPECT_EQ(0, ctlv_exec_reply(fd, CTLV_TEST_OP_PROBE_GET, self->reply,
				     variant->query_reply_min));
	EXPECT_EQ(-EACCES, ctlv_exec_bare(fd, CTLV_TEST_OP_ECHO));
	EXPECT_EQ(-EACCES, ctlv_exec_bare(fd, CTLV_TEST_OP_SERIES_DUMP));
	close(fd);
}

TEST_F(device, a_write_only_file_is_rejected)
{
	int fd = ctlv_open(variant->family, 0, O_WRONLY);

	EXPECT_EQ(-EACCES, fd);
}

TEST_F(device, a_dup_preserves_observation_authority)
{
	int fd = ctlv_open(variant->family, 0, O_RDONLY);
	int copy;

	ASSERT_LE(0, fd);
	copy = dup(fd);
	ASSERT_LE(0, copy);
	EXPECT_EQ(0, ctlv_exec_bare(copy, CTLV_TEST_OP_NOOP));
	EXPECT_EQ(-EACCES, ctlv_exec_bare(copy, CTLV_TEST_OP_ECHO));
	close(copy);
	close(fd);
}

/* Driven through a query, which changes nothing however malformed. */
TEST_F(device, request_framing)
{
	static const struct {
		const char *name;
		__u32 request_len;
		__u32 root_len;
		__u32 flags;
		__u64 attr_id;
		int expected;
	} cases[] = {
		{ "no request at all",
		  0, 0, 0, 0, -EINVAL },
		{ "an empty root",
		  16, 16, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, 0 },
		{ "the largest schema-known request",
		  40, 40, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, 0 },
		{ "bytes past the root, which are outside the message",
		  4096, 16, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, 0 },
		{ "the largest message CTLV accepts",
		  CTLV_MAX_INLINE_MESSAGE_LEN, 16, CTLV_ATTR_F_NESTED,
		  CTLV_ATTR_ROOT, 0 },
		{ "one byte more than CTLV accepts",
		  CTLV_MAX_INLINE_MESSAGE_LEN + 1, 16, CTLV_ATTR_F_NESTED,
		  CTLV_ATTR_ROOT, -EINVAL },
		{ "a message shorter than a header",
		  15, 15, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, -EINVAL },
		{ "a root shorter than a header",
		  16, 8, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, -EINVAL },
		{ "a root longer than the message",
		  16, 24, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, -EINVAL },
		{ "a malformed root beyond the schema-known maximum",
		  48, 48, CTLV_ATTR_F_NESTED, CTLV_ATTR_ROOT, -EINVAL },
		{ "a root that is not nested",
		  16, 16, 0, CTLV_ATTR_ROOT, -EINVAL },
		{ "a root that fragments, which only a reply may",
		  16, 16, CTLV_ATTR_F_NESTED | CTLV_ATTR_F_FRAG_MORE,
		  CTLV_ATTR_ROOT, -EINVAL },
		{ "a root carrying an attr ID of its own",
		  16, 16, CTLV_ATTR_F_NESTED, CTLV_TEST_ATTR_PROBE_TOKEN,
		  -EINVAL },
	};
	unsigned int index;
	struct ctlv_tlv tlv;
	char *request;

	request = calloc(1, CTLV_MAX_INLINE_MESSAGE_LEN + 1);
	ASSERT_NE(NULL, request);
	/* The one attr this op takes; a case's root length says if it is in. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
	ASSERT_EQ(40, ctlv_tlv_done(&tlv));
	memcpy(request, tlv.buf, 40);

	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		struct ctlv_op_exec exec = {
			.op_id = CTLV_TEST_OP_PROBE_GET,
			.request = ctlv_ptr(request),
			.request_len = cases[index].request_len,
			.reply = ctlv_ptr(self->reply),
			.reply_len = variant->query_reply_min,
		};
		struct ctlv_attr *root = (struct ctlv_attr *)request;

		root->len = cases[index].root_len;
		root->flags = cases[index].flags;
		root->attr_id = cases[index].attr_id;

		EXPECT_EQ(cases[index].expected, ctlv_exec(self->fd, &exec)) {
			TH_LOG("%s", cases[index].name);
		}
	}
	free(request);
}

TEST_F(device, a_request_that_is_not_readable)
{
	struct ctlv_op_exec exec = {
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.request_len = 16,
		.reply = ctlv_ptr(self->reply),
		.reply_len = variant->query_reply_min,
	};

	EXPECT_EQ(-EFAULT, ctlv_exec(self->fd, &exec));
}

/* A message may end where the readable pages do, and no byte later. */
TEST_F(device, a_request_copied_to_its_last_byte)
{
	long page = sysconf(_SC_PAGESIZE);
	struct ctlv_op_exec exec = {
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.reply = ctlv_ptr(self->reply),
		.reply_len = variant->query_reply_min,
	};
	struct ctlv_tlv tlv;
	__u32 len;
	char *area;

	area = mmap(NULL, 2 * page, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	ASSERT_NE(MAP_FAILED, area);
	ASSERT_EQ(0, mprotect(area + page, page, PROT_NONE));

	/* A whole message, ending exactly where the readable pages do. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
	len = ctlv_tlv_done(&tlv);
	memcpy(area + page - len, tlv.buf, len);
	exec.request = ctlv_ptr(area + page - len);
	exec.request_len = len;
	EXPECT_EQ(0, ctlv_exec(self->fd, &exec));

	exec.request_len = len + 1;
	EXPECT_EQ(-EFAULT, ctlv_exec(self->fd, &exec));

	/* And nothing is read beyond the message, however short it is. */
	ctlv_root_init(area + page - 16, 16);
	exec.request = ctlv_ptr(area + page - 16);
	exec.request_len = 16;
	EXPECT_EQ(0, ctlv_exec(self->fd, &exec));

	munmap(area, 2 * page);
}

/* Driven through echo, which reports what the generated getters read. */
static int echo_send(int fd, struct ctlv_tlv *tlv)
{
	return ctlv_exec_request(fd, CTLV_TEST_OP_ECHO, tlv->buf,
				 ctlv_tlv_done(tlv));
}

/* One request as it stands, for a case that broke it after building it. */
static int echo_raw(int fd, struct ctlv_tlv *tlv, __u32 len)
{
	return ctlv_exec_request(fd, CTLV_TEST_OP_ECHO, tlv->buf, len);
}

/* The one member echo requires, plus the one attr a case is about. */
static int echo_attr(int fd, __u64 attr_id, __u32 flags, const void *payload,
		     __u32 len)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	if (attr_id != CTLV_TEST_ATTR_ECHO_U)
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_put(&tlv, attr_id, flags, payload, len);
	return echo_send(fd, &tlv);
}

static int echo_u64(int fd, __u64 attr_id, __u64 value)
{
	return echo_attr(fd, attr_id, 0, &value, sizeof(value));
}

static int echo_s64(int fd, __u64 attr_id, __s64 value)
{
	return echo_attr(fd, attr_id, 0, &value, sizeof(value));
}

/* Every token of the report of the request below, in the order it reads. */
#define ECHO_EVERYTHING							\
	"u=4096 s=-100 flag=0 mode=3 quirks=0x3 text=fifteen-chars!! "	\
	"data=deadbeef count=10 count=20 count=30 "			\
	"item.index=1 item.label=seven!! leaf.index=7 item.index=2 "	\
	"region page"

/* What the getters reported of the last request they were given. */
#define EXPECT_ECHO(_expected) do {					\
	char actual[CTLV_LOG_LEN];					\
									\
	ASSERT_EQ(0, ctlv_echo_read(actual, sizeof(actual)));		\
	EXPECT_STREQ(_expected, actual);				\
} while (0)

/* Child logs record extension members when that module is present. */
#define EXPECT_CHILD_ECHO(_expected) do {				\
	char actual[CTLV_LOG_LEN];					\
									\
	if (ctlv_child_logs_available()) {				\
		ASSERT_EQ(0, ctlv_child_echo_read(actual,		\
						  sizeof(actual)));	\
		EXPECT_STREQ(_expected, actual);			\
	}								\
} while (0)

/* Every attr the op takes, at the extremes of what each declares. */
static void echo_everything(struct ctlv_tlv *tlv, bool inheriting)
{
	static const __u8 data[] = { 0xde, 0xad, 0xbe, 0xef };

	ctlv_tlv_init(tlv);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_U, 4096);
	ctlv_tlv_s64(tlv, CTLV_TEST_ATTR_ECHO_S, -100);
	/* Present and false, which is not the same as absent. */
	ctlv_tlv_bool(tlv, CTLV_TEST_ATTR_ECHO_FLAG, false);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_MODE, CTLV_TEST_ECHO_MODE_AUTO);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_QUIRKS,
		     CTLV_TEST_ECHO_QUIRK_SLOW | CTLV_TEST_ECHO_QUIRK_NOISY);
	ctlv_tlv_str(tlv, CTLV_TEST_ATTR_ECHO_TEXT, "fifteen-chars!!");
	ctlv_tlv_bin(tlv, CTLV_TEST_ATTR_ECHO_DATA, data, sizeof(data));
	/* Every occurrence the attr may have, in the order they were sent. */
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_COUNT, 10);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_COUNT, 20);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ECHO_COUNT, 30);
	/* One item with everything it may carry, and one with the minimum. */
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_ECHO_ITEM);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ITEM_INDEX, 1);
	ctlv_tlv_str(tlv, CTLV_TEST_ATTR_ITEM_LABEL, "seven!!");
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_ITEM_LEAF);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ITEM_INDEX, 7);
	ctlv_tlv_nest_end(tlv);
	ctlv_tlv_nest_end(tlv);
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_ECHO_ITEM);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_ITEM_INDEX, 2);
	ctlv_tlv_nest_end(tlv);
	/* One descriptor of each backing the two blob attrs allow. */
	ctlv_tlv_blob_va(tlv, CTLV_TEST_ATTR_ECHO_REGION, 0x1000, 4096);
	ctlv_tlv_blob_memfd(tlv, CTLV_TEST_ATTR_ECHO_PAGE, 0, 4096, 65536);
	/* The parent's extension point; only the child has a getter inside. */
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_PROBE_EXTENSION);
	if (inheriting)
		ctlv_tlv_u64(tlv, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, 9);
	ctlv_tlv_nest_end(tlv);
}

TEST_F(device, a_request_arrives_as_what_it_was_encoded_from)
{
	struct ctlv_tlv tlv;

	ASSERT_EQ(0, ctlv_echo_clear());
	echo_everything(&tlv, variant->inheriting);
	ASSERT_FALSE(tlv.overflow);

	EXPECT_EQ(0, echo_send(self->fd, &tlv));
	/* The extension point is this family's, its members another's. */
	EXPECT_ECHO(ECHO_EVERYTHING " extension");
	EXPECT_CHILD_ECHO(variant->inheriting ? "child.extra=9" : "");
}

/* And nothing else, so that an absent attr is read as absent. */
TEST_F(device, a_request_carrying_only_what_it_must)
{
	struct ctlv_tlv tlv;

	ASSERT_EQ(0, ctlv_echo_clear());

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	EXPECT_EQ(0, echo_send(self->fd, &tlv));
	EXPECT_ECHO("u=1");
}

/* A member inside it is an attr the parent's schema does not place there. */
TEST_F(device, an_extension_point_only_one_family_can_fill)
{
	struct ctlv_tlv tlv;

	ASSERT_EQ(0, ctlv_echo_clear());

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_PROBE_EXTENSION);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, echo_send(self->fd, &tlv));
	EXPECT_ECHO("u=1 extension");
	/* An extension point with nothing in it is nothing to read. */
	EXPECT_CHILD_ECHO("");

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_PROBE_EXTENSION);
	ctlv_tlv_u64(&tlv, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, 9);
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(variant->inheriting ? 0 : -EINVAL,
		  echo_send(self->fd, &tlv));
	EXPECT_CHILD_ECHO(variant->inheriting ? "child.extra=9" : "");
}

TEST_F(device, a_request_missing_what_it_must_carry)
{
	struct ctlv_tlv tlv;

	/* An empty root, and one carrying everything but the member. */
	ctlv_tlv_init(&tlv);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_ECHO_FLAG, true);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	/* And a nest below the root, which is required of as deeply. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ECHO_ITEM);
	ctlv_tlv_str(&tlv, CTLV_TEST_ATTR_ITEM_LABEL, "nope");
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));
}

/* Each of these would be read past its own end if its length were believed. */
TEST_F(device, lengths_a_message_cannot_have)
{
	struct ctlv_tlv tlv;
	__u32 attr, len;

	/* An attr of no length at all, and one shorter than a header. */
	ctlv_tlv_init(&tlv);
	attr = ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	len = ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, attr)->len = 0;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));

	ctlv_tlv_attr(&tlv, attr)->len = sizeof(struct ctlv_attr) - 1;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));

	/* An attr longer than the nest containing it. */
	ctlv_tlv_attr(&tlv, attr)->len = len;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));

	/* A length that would wrap when it is aligned. */
	ctlv_tlv_attr(&tlv, attr)->len = ~0u - 2;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));

	/* A byte short of its last child, which occupies an aligned span. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	attr = ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_ECHO_FLAG, true);
	len = ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, 0)->len = attr + sizeof(struct ctlv_attr) + 1;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));
}

/* What a caller leaves in the padding is not part of the message. */
TEST_F(device, padding_is_not_read)
{
	struct ctlv_tlv tlv;
	__u32 attr, len;

	ASSERT_EQ(0, ctlv_echo_clear());

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	attr = ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_ECHO_FLAG, true);
	len = ctlv_tlv_done(&tlv);
	/* The seven bytes between the one-byte payload and the next attr. */
	memset(ctlv_tlv_at(&tlv, attr + sizeof(struct ctlv_attr) + 1), 0xff, 7);
	EXPECT_EQ(0, echo_raw(self->fd, &tlv, len));
	EXPECT_ECHO("u=1 flag=1");
}

/* IDs nothing declares: of a family nobody registered, and of this family. */
#define ATTR_FOREIGN CTLV_ATTR(0xfffe, 1, 1)
#define ATTR_FOREIGN_NEST CTLV_ATTR(0xfffe, 1, 2)
#define ATTR_UNDECLARED \
	CTLV_ATTR(CTLV_FAMILY_TEST, CTLV_TEST_GROUP_ECHO, 99)

TEST_F(device, attrs_the_schema_does_not_place_here)
{
	/* An attr of this family, which belongs in a nest and not at a root. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ITEM_INDEX, 1));
	/* An attr of a family nothing registered. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, ATTR_FOREIGN, 1));
	/* And one of this family, whose number it never declared. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, ATTR_UNDECLARED, 1));
	/* An attr this op does not take, which another op of the family does. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_PROBE_TOKEN, 1));
	/* A second root, which is only ever the start of a message. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_ATTR_ROOT,
				     CTLV_ATTR_F_NESTED, NULL, 0));
}

/* One attr the caller can do without, beside the one member echo requires. */
static int echo_ignorable(int fd, __u64 attr_id, __u64 value)
{
	return echo_attr(fd, attr_id, CTLV_ATTR_F_IGNORE_IF_UNKNOWN, &value,
			 sizeof(value));
}

/* IGNORE_IF_UNKNOWN lets newer requests reach older kernels. */
TEST_F(device, attrs_a_caller_offered_to_do_without)
{
	__u32 attr, inner, len;
	struct ctlv_tlv tlv;

	/* At a root: an ID of another family, and one of this family's own. */
	ASSERT_EQ(0, ctlv_echo_clear());
	EXPECT_EQ(0, echo_ignorable(self->fd, ATTR_FOREIGN, 1));
	/* Ignored, so the op ran and read a request without it. */
	EXPECT_ECHO("u=1");

	ASSERT_EQ(0, ctlv_echo_clear());
	EXPECT_EQ(0, echo_ignorable(self->fd, ATTR_UNDECLARED, 1));
	EXPECT_ECHO("u=1");

	/* And inside a nest, which is where a newer member would go. */
	ASSERT_EQ(0, ctlv_echo_clear());
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ECHO_ITEM);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, 1);
	attr = ctlv_tlv_u64(&tlv, ATTR_FOREIGN, 1);
	ctlv_tlv_attr(&tlv, attr)->flags = CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, echo_send(self->fd, &tlv));
	EXPECT_ECHO("u=1 item.index=1");

	/* An ID this schema does place, sent where it does not: a mistake. */
	EXPECT_EQ(-EINVAL, echo_ignorable(self->fd, CTLV_TEST_ATTR_ITEM_INDEX,
					  1));
	EXPECT_EQ(-EINVAL, echo_ignorable(self->fd, CTLV_TEST_ATTR_PROBE_TOKEN,
					  1));
	/* Nor does it excuse a known attr from the rest of its declaration. */
	EXPECT_EQ(-EINVAL, echo_ignorable(self->fd, CTLV_TEST_ATTR_ECHO_U,
					  4097));

	/* The root frames the message and is nothing a device may skip. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	len = ctlv_tlv_done(&tlv);
	ctlv_tlv_attr(&tlv, 0)->flags |= CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	EXPECT_EQ(-EINVAL, echo_raw(self->fd, &tlv, len));

	/* Nor is a second root an ID a device has never heard of. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_ATTR_ROOT,
				     CTLV_ATTR_F_NESTED |
				     CTLV_ATTR_F_IGNORE_IF_UNKNOWN, NULL, 0));

	/* A bit no CTLV version defines is still refused. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, ATTR_FOREIGN,
				     CTLV_ATTR_F_IGNORE_IF_UNKNOWN | (1U << 3),
				     NULL, 0));

	/* An ignored unknown nest is opaque. */
	ASSERT_EQ(0, ctlv_echo_clear());
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	attr = ctlv_tlv_nest_start(&tlv, ATTR_FOREIGN_NEST);
	inner = ctlv_tlv_u64(&tlv, ATTR_FOREIGN, 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_attr(&tlv, attr)->flags |= CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	len = ctlv_tlv_done(&tlv);
	/* A child no walk could step over, in an attr that contains it. */
	ctlv_tlv_attr(&tlv, inner)->len = 0;
	EXPECT_EQ(0, echo_raw(self->fd, &tlv, len));
	EXPECT_ECHO("u=1");
}

TEST_F(device, ignored_attrs_extend_the_known_request_maximum)
{
	struct ctlv_tlv tlv;
	__u32 attr, len;

	/* noop's complete known request is its 16-byte empty root. */
	ctlv_tlv_init(&tlv);
	attr = ctlv_tlv_u64(&tlv, ATTR_FOREIGN, 1);
	ctlv_tlv_attr(&tlv, attr)->flags = CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	len = ctlv_tlv_done(&tlv);
	ASSERT_GT(len, 16);
	EXPECT_EQ(0, ctlv_exec_request(self->fd, CTLV_TEST_OP_NOOP,
				       tlv.buf, len));

	/* probe-get's 40-byte maximum includes its one known token. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
	attr = ctlv_tlv_nest_start(&tlv, ATTR_FOREIGN_NEST);
	ctlv_tlv_u64(&tlv, ATTR_FOREIGN, 1);
	ctlv_tlv_nest_end(&tlv);
	ctlv_tlv_attr(&tlv, attr)->flags |= CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	len = ctlv_tlv_done(&tlv);
	ASSERT_GT(len, 40);
	EXPECT_EQ(0, ctlv_exec_full(self->fd, CTLV_TEST_OP_PROBE_GET,
				    tlv.buf, len, self->reply,
				    variant->query_reply_min));
}

/* A child fill is known on its device and ignorable on the parent. */
TEST_F(device, what_a_later_family_added_reaching_a_device_without_it)
{
	struct ctlv_tlv tlv;
	__u32 attr;

	ASSERT_EQ(0, ctlv_echo_clear());

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_PROBE_EXTENSION);
	attr = ctlv_tlv_u64(&tlv, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, 9);
	ctlv_tlv_attr(&tlv, attr)->flags = CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	ctlv_tlv_nest_end(&tlv);
	EXPECT_EQ(0, echo_send(self->fd, &tlv));
	EXPECT_ECHO("u=1 extension");
	/* Read where the family that declared it answers, and nowhere else. */
	EXPECT_CHILD_ECHO(variant->inheriting ? "child.extra=9" : "");
}

/* Whether an attr is a nest, and what its flags may say, is the schema's. */
TEST_F(device, flags_an_attr_cannot_carry)
{
	__u64 value = 1;

	/* A scalar the sender called a nest, and a nest it called a scalar. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_U,
				     CTLV_ATTR_F_NESTED, &value,
				     sizeof(value)));
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_ITEM, 0,
				     NULL, 0));
	/* Fragmentation, which only a reply of a query is ever part of. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_U,
				     CTLV_ATTR_F_FRAG_MORE, &value,
				     sizeof(value)));
	/* A bit no CTLV version defines is refused. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_U,
				     1U << 31, &value, sizeof(value)));
}

TEST_F(device, more_occurrences_than_an_attr_may_have)
{
	struct ctlv_tlv tlv;
	unsigned int index;

	/* Three is what the attr allows, and the fourth is one too many. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	for (index = 0; index < 4; index++)
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, index);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	/* Two nests is what the attr allows. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	for (index = 0; index < 3; index++) {
		ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ECHO_ITEM);
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, index);
		ctlv_tlv_nest_end(&tlv);
	}
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	/* An attr that may occur once, sent twice, required or optional. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 2);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_str(&tlv, CTLV_TEST_ATTR_ECHO_TEXT, "one");
	ctlv_tlv_str(&tlv, CTLV_TEST_ATTR_ECHO_TEXT, "two");
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));
}

TEST_F(device, occurrences_of_one_attr_are_contiguous)
{
	struct ctlv_tlv tlv;
	__u32 unknown;

	/* No numeric or schema order: the required lower ID may come last. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 1);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 2);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	EXPECT_EQ(0, echo_send(self->fd, &tlv));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 1);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 2);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 1);
	unknown = ctlv_tlv_u64(&tlv, ATTR_FOREIGN, 7);
	ctlv_tlv_attr(&tlv, unknown)->flags = CTLV_ATTR_F_IGNORE_IF_UNKNOWN;
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_COUNT, 2);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));
}

TEST_F(device, values_outside_what_an_attr_declares)
{
	__u8 byte;

	/* An unsigned integer, below its minimum and above its maximum. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ECHO_U, 0));
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ECHO_U, 4097));
	/* A signed one, whose bounds are signed. */
	EXPECT_EQ(-EINVAL, echo_s64(self->fd, CTLV_TEST_ATTR_ECHO_S, -101));
	EXPECT_EQ(-EINVAL, echo_s64(self->fd, CTLV_TEST_ATTR_ECHO_S, 101));
	/* An enum, whose values are numbered from one without a gap. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ECHO_MODE, 0));
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ECHO_MODE,
				    CTLV_TEST_ECHO_MODE_AUTO + 1));
	/* A flags word with a bit no family declared. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ECHO_QUIRKS,
				    CTLV_TEST_ECHO_QUIRK_MASK + 1));
	/* An index of a nest below the root, which is checked as closely. */
	EXPECT_EQ(-EINVAL, echo_u64(self->fd, CTLV_TEST_ATTR_ITEM_INDEX, 8));

	/* A boolean is one byte, and holds one of two values. */
	byte = 2;
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_FLAG, 0,
				     &byte, sizeof(byte)));
	byte = 1;
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_FLAG, 0,
				     &byte, 0));
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_FLAG, 0,
				     &byte, 2));

	/* An integer payload is eight bytes, whatever its value would fit in. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_U, 0,
				     &byte, sizeof(byte)));
}

/* Usable as a C string, which is the bytes and not only their length. */
TEST_F(device, strings_a_family_could_not_use)
{
	/* No terminator at all, and one that is not the last byte. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_TEXT, 0,
				     "abc", 3));
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_TEXT, 0,
				     "ab\0c", 5));
	/* No payload, which is not even a terminator. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_TEXT, 0,
				     NULL, 0));
	/* And the declared bounds, which the terminator is outside of. */
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_TEXT, 0,
				     "", 1));
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_TEXT, 0,
				     "sixteen chars!!!", 17));
}

TEST_F(device, binary_outside_the_length_it_declares)
{
	static const __u8 data[33] = {};

	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_DATA, 0,
				     data, 3));
	EXPECT_EQ(-EINVAL, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_DATA, 0,
				     data, sizeof(data)));
	/* Both bounds are inclusive, and both are reachable. */
	EXPECT_EQ(0, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_DATA, 0, data, 4));
	EXPECT_EQ(0, echo_attr(self->fd, CTLV_TEST_ATTR_ECHO_DATA, 0, data,
			       32));
}

/* The placement fails before the higher transport limit. */
TEST_F(device, a_nest_deeper_than_the_schema_places_one)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ECHO_ITEM);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, 1);
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ITEM_LEAF);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, 2);
	/* One level past where the schema stops, the same attr as above it. */
	ctlv_tlv_nest_start(&tlv, CTLV_TEST_ATTR_ITEM_LEAF);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, 3);
	EXPECT_EQ(-EINVAL, echo_send(self->fd, &tlv));
}

/* Nothing is acquired here: an fd is a number until a later phase. */
struct blob_member {
	__u64 attr_id;
	__u64 value;
};

/* A descriptor made of exactly the members one case names, in that order. */
static int echo_blob(int fd, __u64 attr_id, const struct blob_member *members,
		     unsigned int count)
{
	struct ctlv_tlv tlv;
	unsigned int index;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_nest_start(&tlv, attr_id);
	for (index = 0; index < count; index++)
		ctlv_tlv_u64(&tlv, members[index].attr_id,
			     members[index].value);
	ctlv_tlv_nest_end(&tlv);
	return echo_send(fd, &tlv);
}

TEST_F(device, blob_descriptors)
{
	/*
	 * region takes any backing, at eight-byte alignment and up to a page.
	 * page takes a memfd alone, at page alignment and up to 64 KiB.
	 */
	static const struct {
		const char *name;
		__u64 attr_id;
		struct blob_member members[4];
		unsigned int count;
		int expected;
	} cases[] = {
		{ "an address range",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 4096 } }, 3, 0 },
		{ "a file, whose offset is optional",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_MEMFD },
		    { CTLV_ATTR_BLOB_FD, 0 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 3, 0 },
		{ "a file at an offset",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_DMA_BUF },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_OFFSET, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 4, 0 },
		{ "no type, so no variant",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 2, -EINVAL },
		{ "a type nothing declares",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_DMA_BUF + 1 },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 3, -EINVAL },
		{ "a backing this attr does not allow",
		  CTLV_TEST_ATTR_ECHO_PAGE,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 4096 },
		    { CTLV_ATTR_BLOB_LENGTH, 4096 } }, 3, -EINVAL },
		{ "an address range described by an fd",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 3, -EINVAL },
		{ "a file at an address",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_MEMFD },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 4, -EINVAL },
		{ "an address range at an offset",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_OFFSET, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 4, -EINVAL },
		{ "an fd no process could have",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_MEMFD },
		    { CTLV_ATTR_BLOB_FD, 0x80000000ULL },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 3, -EINVAL },
		{ "no length",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 } }, 2, -EINVAL },
		{ "nothing to map",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 0 } }, 3, -EINVAL },
		{ "more than the attr allows",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 4097 } }, 3, -EINVAL },
		{ "a start the attr's alignment does not allow",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 4 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 3, -EINVAL },
		{ "a file offset the attr's alignment does not allow",
		  CTLV_TEST_ATTR_ECHO_PAGE,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_MEMFD },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_OFFSET, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 4096 } }, 4, -EINVAL },
		{ "a range that wraps",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, ~0ULL - 7 },
		    { CTLV_ATTR_BLOB_LENGTH, 4096 } }, 3, -EINVAL },
		/* The same sum through offset, which is a different member. */
		{ "a file range that wraps",
		  CTLV_TEST_ATTR_ECHO_PAGE,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_MEMFD },
		    { CTLV_ATTR_BLOB_FD, 3 },
		    { CTLV_ATTR_BLOB_OFFSET, ~0ULL - 4095 },
		    { CTLV_ATTR_BLOB_LENGTH, 4096 } }, 4, -EINVAL },
		{ "a member described twice",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 } }, 4, -EINVAL },
		{ "an attr that is not a member of a descriptor",
		  CTLV_TEST_ATTR_ECHO_REGION,
		  { { CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA },
		    { CTLV_ATTR_BLOB_ADDRESS, 8 },
		    { CTLV_ATTR_BLOB_LENGTH, 8 },
		    { CTLV_TEST_ATTR_ECHO_U, 1 } }, 4, -EINVAL },
	};
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		int ret = echo_blob(self->fd, cases[index].attr_id,
				    cases[index].members, cases[index].count);

		EXPECT_EQ(cases[index].expected, ret) {
			TH_LOG("%s", cases[index].name);
		}
	}
}

/* An op that answers with an ACK alone writes nothing, whatever it is given. */
TEST_F(device, an_ack_leaves_the_output_buffer_alone)
{
	unsigned int index;

	memset(self->reply, 0xa5, sizeof(self->reply));
	EXPECT_EQ(0, ctlv_exec_reply(self->fd, CTLV_TEST_OP_NOOP, self->reply,
				     sizeof(self->reply)));
	for (index = 0; index < sizeof(self->reply); index++)
		ASSERT_EQ((char)0xa5, self->reply[index]) {
			TH_LOG("byte %u was written", index);
		}

	/* Including when it is given no output buffer at all. */
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
}

/* A query needs room for one whole fragment before it may run. */
TEST_F(device, a_query_without_room_for_a_fragment)
{
	EXPECT_EQ(-EMSGSIZE,
		  ctlv_exec_reply(self->fd, CTLV_TEST_OP_PROBE_GET,
				  self->reply, variant->query_reply_min - 1));
	EXPECT_EQ(-EMSGSIZE, ctlv_exec_bare(self->fd, CTLV_TEST_OP_PROBE_GET));
}

/* Room for the largest reply, so it cannot fail after changing something. */
TEST_F(device, an_action_that_answers_reaches_no_callback_without_room)
{
	const struct ctlv_attr *root = (const struct ctlv_attr *)self->reply;

	EXPECT_EQ(-EMSGSIZE,
		  ctlv_exec_reply(self->fd, CTLV_TEST_OP_DYNAMIC_TOGGLE,
				  self->reply, 39));
	EXPECT_EQ(-EMSGSIZE,
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_TOGGLE));
	/* Nothing ran, so nothing was flipped. */
	EXPECT_EQ(dynamic_result(variant->dynamic_disabled),
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));

	EXPECT_EQ(0, ctlv_exec_reply(self->fd, CTLV_TEST_OP_DYNAMIC_TOGGLE,
				     self->reply, 40));
	EXPECT_EQ(sizeof(*root), root->len);
	EXPECT_EQ(dynamic_result(!variant->dynamic_disabled),
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));

	EXPECT_EQ(0, dynamic_toggle(self->fd));
}

/* An action that already ran is not rolled back for an unwritable reply. */
TEST_F(device, an_action_that_committed_survives_an_unwritable_reply)
{
	long page = sysconf(_SC_PAGESIZE);
	char *area;

	area = mmap(NULL, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	ASSERT_NE(MAP_FAILED, area);

	EXPECT_EQ(-EFAULT, ctlv_exec_reply(self->fd,
					   CTLV_TEST_OP_DYNAMIC_TOGGLE,
					   area, page));
	EXPECT_EQ(dynamic_result(!variant->dynamic_disabled),
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));

	EXPECT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(dynamic_result(variant->dynamic_disabled),
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
	munmap(area, page);
}

/* Really that large, so a kernel that took the number would not fault. */
TEST_F(device, a_reply_buffer_larger_than_a_message)
{
	size_t len = CTLV_MAX_INLINE_MESSAGE_LEN + CTLV_ALIGNTO;
	struct ctlv_op_exec exec = {
		.op_id = CTLV_TEST_OP_PROBE_GET,
	};
	char *area;

	area = mmap(NULL, len, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	ASSERT_NE(MAP_FAILED, area);

	ctlv_exec_set_empty_request(&exec);
	exec.reply = ctlv_ptr(area);
	exec.reply_len = CTLV_MAX_INLINE_MESSAGE_LEN;
	EXPECT_EQ(0, ctlv_exec(self->fd, &exec));

	exec.reply_len = CTLV_MAX_INLINE_MESSAGE_LEN + 1;
	EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &exec));

	munmap(area, len);
}

/* echo-reply puts the request above back through the serializers. */

/* What this renders each attr of a reply as, and how its payload reads. */
static const struct reply_name {
	__u64 attr_id;
	const char *name;
	/* One of unsigned, signed, hexadecimal, bytes, text, nest. */
	char form;
} reply_names[] = {
	{ CTLV_TEST_ATTR_ECHO_U, "u", 'u' },
	{ CTLV_TEST_ATTR_ECHO_S, "s", 's' },
	{ CTLV_TEST_ATTR_ECHO_FLAG, "flag", 'u' },
	{ CTLV_TEST_ATTR_ECHO_MODE, "mode", 'u' },
	{ CTLV_TEST_ATTR_ECHO_QUIRKS, "quirks", 'x' },
	{ CTLV_TEST_ATTR_ECHO_TEXT, "text", 't' },
	{ CTLV_TEST_ATTR_ECHO_DATA, "data", 'b' },
	{ CTLV_TEST_ATTR_ECHO_COUNT, "count", 'u' },
	{ CTLV_TEST_ATTR_ECHO_ITEM, "item", 'n' },
	{ CTLV_TEST_ATTR_ITEM_INDEX, "index", 'u' },
	{ CTLV_TEST_ATTR_ITEM_LABEL, "label", 't' },
	{ CTLV_TEST_ATTR_ITEM_LEAF, "leaf", 'n' },
	{ CTLV_TEST_ATTR_PROBE_TOKEN, "token", 'u' },
	{ CTLV_TEST_ATTR_PROBE_EXTENSION, "extension", 'n' },
	{ CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, "extra", 'u' },
	{ CTLV_TEST_ATTR_SERIES_CHUNK, "chunk", 'n' },
	{ CTLV_TEST_ATTR_SERIES_ITEM, "item", 'n' },
	{ CTLV_TEST_ATTR_SERIES_VALUE, "value", 'u' },
	{ CTLV_TEST_ATTR_SERIES_TOTAL, "total", 'u' },
	{ CTLV_TEST_ATTR_SERIES_INDEX, "index", 'u' },
	{ CTLV_ATTR_ERROR_REASON, "reason", 'u' },
	{ CTLV_ATTR_ERROR_MESSAGE, "message", 't' },
	{ CTLV_ATTR_ERROR_BAD_ATTR_OFFSET, "bad-attr-offset", 'u' },
	{ CTLV_ATTR_ERROR_NEST_OFFSET, "nest-offset", 'u' },
	{ CTLV_ATTR_ERROR_MISSING_ATTR_ID, "missing-attr-id", 'x' },
	{ CTLV_ATTR_ERROR_EXPECTED_GENERATION, "expected-generation", 'u' },
	{ CTLV_ATTR_ERROR_CURRENT_GENERATION, "current-generation", 'u' },
	{ CTLV_ATTR_FAMILY_ENTRY, "family", 'n' },
	{ CTLV_ATTR_FAMILY_ID, "id", 'u' },
	{ CTLV_ATTR_FAMILY_NAME, "name", 't' },
	{ CTLV_ATTR_OP_ENTRY, "op", 'n' },
	{ CTLV_ATTR_OP_ID, "op-id", 'x' },
	{ CTLV_ATTR_OP_TYPE, "type", 'u' },
	{ CTLV_ATTR_OP_AVAILABILITY, "availability", 'u' },
	{ CTLV_ATTR_OP_DISABLED, "disabled", 'u' },
	{ CTLV_ATTR_OP_REQUEST_MAX_LEN, "request-max", 'u' },
	{ CTLV_ATTR_OP_ACTION_REPLY_MAX_LEN, "action-reply-max", 'u' },
	{ CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN, "query-reply-min", 'u' },
	{ CTLV_ATTR_OP_QUERY_REPLY_SUGGESTED_LEN, "query-reply-suggested",
	  'u' },
	{ CTLV_ATTR_OP_EVENT_RECORD_MAX_LEN, "event-record-max", 'u' },
	{ CTLV_ATTR_OP_FD_ACCESS, "fd-access", 'u' },
	{ CTLV_ATTR_SCHEMA_REQUEST, "request", 'n' },
	{ CTLV_ATTR_SCHEMA_REPLY, "reply", 'n' },
	{ CTLV_ATTR_SCHEMA_ATTRS, "attr", 'n' },
	{ CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS, "optional", 'n' },
	{ CTLV_ATTR_SCHEMA_ATTR_ID, "attr-id", 'x' },
	{ CTLV_ATTR_SCHEMA_KIND, "kind", 'u' },
	{ CTLV_ATTR_SCHEMA_MAX_OCCURRENCES, "occurrences", 'u' },
	{ CTLV_ATTR_SCHEMA_MIN, "min", 'x' },
	{ CTLV_ATTR_SCHEMA_MAX, "max", 'x' },
	{ CTLV_ATTR_SCHEMA_MIN_LENGTH, "min-len", 'u' },
	{ CTLV_ATTR_SCHEMA_MAX_LENGTH, "max-len", 'u' },
	{ CTLV_ATTR_SCHEMA_FLAGS_MASK, "mask", 'x' },
	{ CTLV_ATTR_SCHEMA_UNIT, "unit", 'u' },
	{ CTLV_ATTR_SCHEMA_SCALE, "scale", 's' },
	{ CTLV_ATTR_SCHEMA_BLOB_DIRECTION, "blob-direction", 'u' },
	{ CTLV_ATTR_SCHEMA_BLOB_BACKINGS, "blob-backings", 'x' },
	{ CTLV_ATTR_SCHEMA_BLOB_MAX_LENGTH, "blob-max-len", 'u' },
	{ CTLV_ATTR_SCHEMA_BLOB_ALIGNMENT, "blob-alignment", 'u' },
	{ CTLV_ATTR_SCHEMA_REENTERS, "reenters", 'u' },
};

/* One line of it, as far as it has been rendered. */
struct reply_text {
	char buf[CTLV_LOG_LEN];
	size_t used;
};

static void reply_add(struct reply_text *text, const char *fmt, ...)
{
	size_t room = sizeof(text->buf) - text->used;
	va_list args;
	size_t wrote;

	va_start(args, fmt);
	wrote = vsnprintf(text->buf + text->used, room, fmt, args);
	va_end(args);
	/* Truncation is the case's own mistake; the compared line says so. */
	text->used += wrote < room ? wrote : room - 1;
}

static void reply_render(struct reply_text *text, const struct ctlv_attr *nest,
			 unsigned int skip);

static void reply_attr(struct reply_text *text, const struct ctlv_attr *attr)
{
	const struct reply_name *name = NULL;
	const __u8 *bytes;
	unsigned int i;
	__u32 len;

	for (i = 0; i < ARRAY_SIZE(reply_names); i++)
		if (reply_names[i].attr_id == attr->attr_id)
			name = &reply_names[i];
	if (!name) {
		reply_add(text, "%#llx?", attr->attr_id);
		return;
	}

	switch (name->form) {
	case 'n':
		reply_add(text, "%s={", name->name);
		reply_render(text, attr, 0);
		/* A nest a fragment boundary left open has not ended. */
		if (!(attr->flags & CTLV_ATTR_F_FRAG_MORE))
			reply_add(text, "}");
		break;
	case 'u':
		reply_add(text, "%s=%llu", name->name, ctlv_rd_u64(attr));
		break;
	case 's':
		reply_add(text, "%s=%lld", name->name,
			  (long long)ctlv_rd_u64(attr));
		break;
	case 'x':
		reply_add(text, "%s=%#llx", name->name, ctlv_rd_u64(attr));
		break;
	case 't':
		reply_add(text, "%s=%s", name->name,
			  (const char *)ctlv_rd_payload(attr, &len));
		break;
	default:
		bytes = ctlv_rd_payload(attr, &len);
		reply_add(text, "%s=", name->name);
		for (i = 0; i < len; i++)
			reply_add(text, "%02x", bytes[i]);
		break;
	}
}

/* One space between siblings, and none just inside a nest. */
static void reply_sep(struct reply_text *text)
{
	if (text->used && text->buf[text->used - 1] != '{')
		reply_add(text, " ");
}

/*
 * The children of one nest, @skip levels of a re-emitted path left out so a
 * continuation renders as what it adds. Closing braces are not skipped.
 */
static void reply_render(struct reply_text *text, const struct ctlv_attr *nest,
			 unsigned int skip)
{
	const struct ctlv_attr *child;
	bool first = true;

	ctlv_rd_for_each(child, nest) {
		if (first && skip) {
			reply_render(text, child, skip - 1);
			if (!(child->flags & CTLV_ATTR_F_FRAG_MORE))
				reply_add(text, "}");
		} else {
			reply_sep(text);
			reply_attr(text, child);
		}
		first = false;
	}
}

/* Whatever is in the fixture's output buffer, as that one line. */
#define EXPECT_REPLY(_expected) do {					\
	struct reply_text text = { .used = 0 };				\
									\
	reply_render(&text, (const struct ctlv_attr *)self->reply, 0);	\
	EXPECT_STREQ(_expected, text.buf);				\
} while (0)

/* Every attr of the corpus that echo-reply mirrors, blobs excepted. */
#define REPLY_EVERYTHING						\
	"u=4096 s=-100 flag=0 mode=3 quirks=0x3 text=fifteen-chars!! "	\
	"data=deadbeef count=10 count=20 count=30 "			\
	"item={index=1 label=seven!! leaf={index=7}} item={index=2}"

static int echo_reply_send(int fd, struct ctlv_tlv *tlv, void *reply,
			   __u32 reply_len)
{
	return ctlv_exec_full(fd, CTLV_TEST_OP_ECHO_REPLY, tlv->buf,
			      ctlv_tlv_done(tlv), reply, reply_len);
}

/* Every form, both cardinalities, and all three levels of nesting. */
TEST_F(device, a_reply_carries_what_the_serializers_were_given)
{
	struct ctlv_tlv tlv;

	echo_everything(&tlv, variant->inheriting);
	ASSERT_FALSE(tlv.overflow);

	EXPECT_EQ(0, echo_reply_send(self->fd, &tlv, self->reply,
				     variant->echo_reply_max));
	EXPECT_REPLY(variant->inheriting ?
		     REPLY_EVERYTHING " extension={extra=9}" :
		     REPLY_EVERYTHING " extension={}");
}

TEST_F(device, a_reply_carrying_only_what_the_request_did)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	EXPECT_EQ(0, echo_reply_send(self->fd, &tlv, self->reply,
				     variant->echo_reply_max));
	EXPECT_REPLY("u=1");
}

TEST_F(device, an_action_that_answers_needs_its_whole_maximum)
{
	struct ctlv_tlv tlv;

	echo_everything(&tlv, variant->inheriting);
	ASSERT_FALSE(tlv.overflow);
	ASSERT_GE(sizeof(self->reply), variant->echo_reply_max);

	EXPECT_EQ(-EMSGSIZE, echo_reply_send(self->fd, &tlv, self->reply,
					     variant->echo_reply_max - 1));
	EXPECT_EQ(0, echo_reply_send(self->fd, &tlv, self->reply,
				     variant->echo_reply_max));
}

/* probe-get ends the reply either way, so the caller gets what fit. */
TEST_F(device, a_query_writes_what_the_caller_had_room_for)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);

	EXPECT_EQ(0, ctlv_exec_full(self->fd, CTLV_TEST_OP_PROBE_GET, tlv.buf,
				    ctlv_tlv_done(&tlv), self->reply,
				    sizeof(self->reply)));
	EXPECT_REPLY("token=7 extension={}");

	EXPECT_EQ(0, ctlv_exec_full(self->fd, CTLV_TEST_OP_PROBE_GET, tlv.buf,
				    ctlv_tlv_done(&tlv), self->reply,
				    variant->query_reply_min));
	EXPECT_REPLY(variant->query_truncated);
}

/* The request is a snapshot, so one buffer may serve as both. */
TEST_F(device, one_buffer_for_the_request_and_the_reply)
{
	struct ctlv_tlv tlv;
	__u32 request_len;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
	request_len = ctlv_tlv_done(&tlv);
	ASSERT_GE(sizeof(self->reply), request_len);
	memcpy(self->reply, tlv.buf, request_len);

	EXPECT_EQ(0, ctlv_exec_full(self->fd, CTLV_TEST_OP_PROBE_GET,
				    self->reply, request_len, self->reply,
				    sizeof(self->reply)));
	EXPECT_REPLY("token=7 extension={}");
}

/* series-dump fragments; the kernel keeps nothing between two of them. */

/* Root, chunk, item and one value: what a fragment of this needs room for. */
#define SERIES_REPLY_MIN 72

/* More fragments than any case here takes. */
#define SERIES_MAX_FRAGS 16

/* What one chunk of two items of two values, and two totals, renders as. */
#define SERIES_WHOLE							\
	"chunk={index=1 item={index=1 value=111 value=112} "		\
	"item={index=2 value=121 value=122}} total=1 total=2"

struct series {
	/* The whole logical reply, as its fragments reassemble into it. */
	struct reply_text logical;
	/* And each fragment on its own, boundary marks and all. */
	struct reply_text frag[SERIES_MAX_FRAGS];
	unsigned int frags;
	/* What the last fragment reported, and where it said to carry on. */
	__u64 generation;
	__u64 cursor;
	/* More fragments than this can hold, so nothing else means anything. */
	bool overflow;
	/* A fragment that said two different things about being the last. */
	bool marked_wrong;
};

/* How many nests one fragment left open, which is what the next re-emits. */
static unsigned int frag_open(const struct ctlv_attr *nest)
{
	const struct ctlv_attr *child, *last = NULL;

	ctlv_rd_for_each(child, nest)
		last = child;
	if (!last || !(last->flags & CTLV_ATTR_F_FRAG_MORE))
		return 0;
	return 1 + frag_open(last);
}

/* How much of a series to ask for, in the order the request declares it. */
static void series_request(struct ctlv_tlv *tlv, __u64 chunks, __u64 items,
			   __u64 values, __u64 totals)
{
	ctlv_tlv_init(tlv);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_CHUNKS, chunks);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_ITEMS, items);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_VALUES, values);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_TOTALS, totals);
}

/* The series every case here dumps, unless it says otherwise. */
static void series_corpus(struct ctlv_tlv *tlv)
{
	series_request(tlv, 1, 2, 2, 2);
}

/* One whole dump, with @sleep_us of the caller doing something else. */
static int series_dump(struct series *series, int fd, struct ctlv_tlv *tlv,
		       void *reply, __u32 reply_len, unsigned int sleep_us)
{
	const struct ctlv_attr *root = reply;
	struct ctlv_op_exec exec = {
		.op_id = CTLV_TEST_OP_SERIES_DUMP,
		.request = ctlv_ptr(tlv->buf),
		.request_len = ctlv_tlv_done(tlv),
		.reply = ctlv_ptr(reply),
		.reply_len = reply_len,
	};
	unsigned int open = 0;
	int ret;

	memset(series, 0, sizeof(*series));
	do {
		ret = ctlv_exec(fd, &exec);
		if (ret)
			return ret;
		if (series->frags == SERIES_MAX_FRAGS) {
			series->overflow = true;
			return 0;
		}

		reply_render(&series->frag[series->frags], root, 0);
		/* And again as what it adds to what came before it. */
		reply_render(&series->logical, root, open);
		open = frag_open(root);
		/* The root mark, the cursor and the open nests must agree. */
		if (!(root->flags & CTLV_ATTR_F_FRAG_MORE) != !exec.cursor ||
		    (!exec.cursor && open))
			series->marked_wrong = true;

		series->frags++;
		series->generation = exec.generation;
		series->cursor = exec.cursor;
		if (sleep_us)
			usleep(sleep_us);
	} while (exec.cursor);
	return 0;
}

TEST_F(device, a_dump_that_fits_in_one_fragment)
{
	struct series series;
	struct ctlv_tlv tlv;

	series_corpus(&tlv);
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 sizeof(self->reply), 0));
	EXPECT_EQ(1, series.frags);
	EXPECT_EQ(0, series.cursor);
	EXPECT_STREQ(SERIES_WHOLE, series.logical.buf);
	/* And is told the generation a continuation would have to match. */
	EXPECT_EQ(ctlv_generation_read(), series.generation);
}

/* Cut at every length a caller might offer, it reassembles the same. */
TEST_F(device, fragments_of_one_reply_reassemble_into_it)
{
	struct series series;
	struct ctlv_tlv tlv;
	__u32 len, whole;

	series_corpus(&tlv);
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 sizeof(self->reply), 0));
	whole = ((const struct ctlv_attr *)self->reply)->len;

	for (len = SERIES_REPLY_MIN; len <= whole; len++) {
		ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
					 len, 0));
		ASSERT_FALSE(series.overflow) { TH_LOG("reply_len %u", len); }
		EXPECT_FALSE(series.marked_wrong) {
			TH_LOG("reply_len %u", len);
		}
		EXPECT_EQ(0, series.cursor) { TH_LOG("reply_len %u", len); }
		EXPECT_STREQ(SERIES_WHOLE, series.logical.buf) {
			TH_LOG("reply_len %u", len);
		}
	}
}

/* At the least a caller may offer: no leaf written twice or in halves. */
TEST_F(device, where_a_fragment_ends)
{
	static const char *const expected[] = {
		/* An item opened with no room left for its index. */
		"chunk={index=1 item={",
		/* Which the next fragment writes, and then a value each. */
		"chunk={item={index=1",
		"chunk={item={value=111",
		/* The item ends here, so the next one opens the one after. */
		"chunk={item={value=112}",
		"chunk={item={index=2",
		"chunk={item={value=121",
		/* And here the chunk ends too, the first total not fitting. */
		"chunk={item={value=122}}",
		/* The totals, which are what the chunks are written beside. */
		"total=1 total=2",
	};
	struct series series;
	struct ctlv_tlv tlv;
	unsigned int frag;

	series_corpus(&tlv);
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 SERIES_REPLY_MIN, 0));
	ASSERT_EQ(ARRAY_SIZE(expected), series.frags);
	for (frag = 0; frag < series.frags; frag++)
		EXPECT_STREQ(expected[frag], series.frag[frag].buf) {
			TH_LOG("fragment %u", frag);
		}
	EXPECT_STREQ(SERIES_WHOLE, series.logical.buf);
}

/* One aligned unit short of the whole of it is one leaf short of it. */
TEST_F(device, a_dump_one_leaf_short_of_one_fragment)
{
	struct series series;
	struct ctlv_tlv tlv;
	__u32 whole;

	series_corpus(&tlv);
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 sizeof(self->reply), 0));
	whole = ((const struct ctlv_attr *)self->reply)->len;

	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 whole - CTLV_ALIGNTO, 0));
	EXPECT_EQ(2, series.frags);
	EXPECT_STREQ("total=2", series.frag[1].buf);
	EXPECT_STREQ(SERIES_WHOLE, series.logical.buf);
}

/* One fragment is one invocation, so one whole hook chain each. */
TEST_F(device, the_hooks_of_every_fragment)
{
	struct series series;
	struct ctlv_tlv tlv;

	/* A shorter series, so that the log has room for every chain. */
	series_request(&tlv, 1, 1, 2, 0);
	ASSERT_EQ(0, ctlv_hooks_clear());
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 SERIES_REPLY_MIN, 0));
	ASSERT_EQ(4, series.frags);
	if (variant->inheriting) {
		EXPECT_HOOKS_TIMES(series.frags, "op-pre", "call", "op-post");
		EXPECT_CHILD_HOOKS_TIMES(series.frags, "family-pre",
					 "family-post");
	} else {
		EXPECT_HOOKS_TIMES(series.frags, "family-pre", "op-pre",
				   "call", "op-post", "family-post");
	}
}

/* A series that moved on between two fragments is refused, not answered. */
TEST_F(device, a_continuation_of_a_series_that_moved_on)
{
	char expected[CTLV_LOG_LEN];
	struct ctlv_op_exec exec;
	struct ctlv_tlv tlv;
	__u64 generation;

	series_corpus(&tlv);
	exec = (struct ctlv_op_exec){
		.op_id = CTLV_TEST_OP_SERIES_DUMP,
		.request = ctlv_ptr(tlv.buf),
		.request_len = ctlv_tlv_done(&tlv),
		.reply = ctlv_ptr(self->reply),
		.reply_len = SERIES_REPLY_MIN,
	};
	ASSERT_EQ(0, ctlv_exec(self->fd, &exec));
	ASSERT_NE(0, exec.cursor);
	generation = exec.generation;

	/* A caller may offer a different buffer for every fragment. */
	exec.reply_len = sizeof(self->reply);
	ASSERT_EQ(0, ctlv_generation_bump());
	EXPECT_EQ(-ESTALE, ctlv_exec(self->fd, &exec));
	snprintf(expected, sizeof(expected),
		 "reason=%u expected-generation=%llu current-generation=%llu",
		 CTLV_VALIDATION_REASON_STALE_GENERATION, generation,
		 generation + 1);
	EXPECT_REPLY(expected);
	/* A failure has nothing to continue, so it says nothing about where. */
	EXPECT_EQ(generation, exec.generation);
}

/* A lock held across a fragment is what lockdep would catch here. */
TEST_F(device, a_reader_that_sleeps_between_fragments)
{
	struct series series;
	struct ctlv_tlv tlv;

	series_corpus(&tlv);
	ASSERT_EQ(0, series_dump(&series, self->fd, &tlv, self->reply,
				 SERIES_REPLY_MIN, 2000));
	EXPECT_STREQ(SERIES_WHOLE, series.logical.buf);
}

/* Error information goes where the reply would, all of it or none. */

/* An attr the schema places inside item, sent at the root, 40 bytes in. */
static int error_send(int fd, void *reply, __u32 reply_len)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ECHO_U, 1);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_ITEM_INDEX, 1);
	return ctlv_exec_full(fd, CTLV_TEST_OP_ECHO, tlv.buf,
			      ctlv_tlv_done(&tlv), reply, reply_len);
}

TEST_F(device, error_information_says_what_was_refused)
{
	char expected[CTLV_LOG_LEN];

	snprintf(expected, sizeof(expected),
		 "reason=%u bad-attr-offset=40 nest-offset=0",
		 CTLV_VALIDATION_REASON_UNKNOWN_ATTR);

	EXPECT_EQ(-EINVAL, error_send(self->fd, self->reply,
				      sizeof(self->reply)));
	EXPECT_REPLY(expected);
}

TEST_F(device, error_information_is_all_of_it_or_none_of_it)
{
	const struct ctlv_attr *root = (const struct ctlv_attr *)self->reply;
	unsigned int byte;
	__u32 whole;

	ASSERT_EQ(-EINVAL, error_send(self->fd, self->reply,
				      sizeof(self->reply)));
	whole = root->len;
	ASSERT_LT(sizeof(*root), whole);

	/* One byte short of the whole of it is none of it. */
	memset(self->reply, 0xa5, sizeof(self->reply));
	EXPECT_EQ(-EINVAL, error_send(self->fd, self->reply, whole - 1));
	for (byte = 0; byte < sizeof(self->reply); byte++)
		ASSERT_EQ((char)0xa5, self->reply[byte]) {
			TH_LOG("byte %u was written", byte);
		}

	/* Exactly enough is all of it, and none at all is the errno alone. */
	EXPECT_EQ(-EINVAL, error_send(self->fd, self->reply, whole));
	EXPECT_EQ(whole, root->len);
	EXPECT_EQ(-EINVAL, error_send(self->fd, NULL, 0));
}

/* A device may take an op away before anyone could ever have observed it. */
TEST_F(device, an_op_taken_away_before_registration)
{
	EXPECT_EQ(dynamic_result(variant->dynamic_disabled),
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
}

TEST_F(device, an_op_taken_away_and_given_back)
{
	unsigned int round;

	for (round = 0; round < 4; round++) {
		EXPECT_EQ(dynamic_result(variant->dynamic_disabled),
			  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
		ASSERT_EQ(0, dynamic_toggle(self->fd));
		EXPECT_EQ(dynamic_result(!variant->dynamic_disabled),
			  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
		ASSERT_EQ(0, dynamic_toggle(self->fd));
	}
}

/* Availability is state of a device, and a fixed op has none of it. */
TEST_F(device, a_fixed_op_is_never_taken_away)
{
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
	ASSERT_EQ(0, dynamic_toggle(self->fd));
}

/* Every open file description of a device sees the same availability. */
TEST_F(device, an_availability_change_reaches_every_file)
{
	int other = ctlv_open(variant->family, 0, O_RDWR);

	ASSERT_LE(0, other);
	EXPECT_EQ(dynamic_result(variant->dynamic_disabled),
		  ctlv_exec_bare(other, CTLV_TEST_OP_DYNAMIC_NOOP));
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(dynamic_result(!variant->dynamic_disabled),
		  ctlv_exec_bare(other, CTLV_TEST_OP_DYNAMIC_NOOP));
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	close(other);
}

/* A hook records itself in the test module, and these compare the record. */
TEST_F(device, hooks_around_an_op_with_no_pair_of_its_own)
{
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
	if (variant->inheriting) {
		EXPECT_HOOKS("call");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "call", "family-post");
	}

	/* A child-local op must not enter parent-family hooks. */
	if (!variant->inheriting)
		return;
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_CHILD_OP_EXTRA_NOOP));
	EXPECT_NO_HOOKS();
	EXPECT_CHILD_HOOKS("family-pre", "call", "family-post");
}

TEST_F(device, hooks_around_an_op_with_a_pair_of_its_own)
{
	/* A query, and then an action, both of which asked for a pair. */
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_reply(self->fd, CTLV_TEST_OP_PROBE_GET,
				     self->reply, variant->query_reply_min));
	if (variant->inheriting) {
		EXPECT_HOOKS("op-pre", "call", "op-post");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "op-pre", "call", "op-post",
			     "family-post");
	}

	if (variant->dynamic_disabled)
		ASSERT_EQ(0, dynamic_toggle(self->fd));
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
	if (variant->inheriting) {
		EXPECT_HOOKS("op-pre", "call", "op-post");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "op-pre", "call", "op-post",
			     "family-post");
	}
	if (variant->dynamic_disabled)
		ASSERT_EQ(0, dynamic_toggle(self->fd));
}

/* The availability check is inside both pairs, so both still unwind. */
TEST_F(device, hooks_of_an_op_the_device_took_away)
{
	if (!variant->dynamic_disabled)
		ASSERT_EQ(0, dynamic_toggle(self->fd));

	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
	if (variant->inheriting) {
		EXPECT_HOOKS("op-pre", "op-post");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "op-pre", "op-post",
			     "family-post");
	}

	if (!variant->dynamic_disabled)
		ASSERT_EQ(0, dynamic_toggle(self->fd));
}

/* Nothing inside the family pre runs, and the pre's errno is the result. */
TEST_F(device, a_family_pre_that_refuses)
{
	const char *fail = variant->inheriting ?
		CTLV_TEST_CHILD_FAIL_FAMILY_PRE : CTLV_TEST_FAIL_FAMILY_PRE;

	ASSERT_EQ(0, ctlv_sysfs_write(fail, "1"));

	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(-EIO, ctlv_exec_reply(self->fd, CTLV_TEST_OP_NOOP,
					self->reply, sizeof(self->reply)));
	EXPECT_REPLY(variant->inheriting ?
		     "message=test child family pre hook refused the operation" :
		     "message=test family pre hook refused the operation");
	if (variant->inheriting) {
		EXPECT_NO_HOOKS();
		EXPECT_CHILD_HOOKS("family-pre");
	} else {
		EXPECT_HOOKS("family-pre");
	}

	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(-EIO, ctlv_exec_bare(self->fd, CTLV_TEST_OP_DYNAMIC_NOOP));
	if (variant->inheriting) {
		EXPECT_NO_HOOKS();
		EXPECT_CHILD_HOOKS("family-pre");
	} else {
		EXPECT_HOOKS("family-pre");
	}

	ASSERT_EQ(0, ctlv_sysfs_write(fail, "0"));
	ASSERT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
}

/* An op's pre is inside the family's, whose post still runs once. */
TEST_F(device, an_op_pre_that_refuses)
{
	ASSERT_EQ(0, ctlv_sysfs_write(CTLV_TEST_FAIL_OP_PRE, "1"));

	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(-EBADMSG,
		  ctlv_exec_reply(self->fd, CTLV_TEST_OP_PROBE_GET,
				  self->reply, sizeof(self->reply)));
	EXPECT_REPLY("message=probe-get pre hook refused the operation");
	if (variant->inheriting) {
		EXPECT_HOOKS("op-pre");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "op-pre", "family-post");
	}

	/* And an op with no pair of its own has nothing to refuse it. */
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
	if (variant->inheriting) {
		EXPECT_HOOKS("call");
		EXPECT_CHILD_HOOKS("family-pre", "family-post");
	} else {
		EXPECT_HOOKS("family-pre", "call", "family-post");
	}

	ASSERT_EQ(0, ctlv_sysfs_write(CTLV_TEST_FAIL_OP_PRE, "0"));
}

TEST_F(device, a_core_op_uses_only_the_core_family_hooks)
{
	const char *fail = variant->inheriting ?
		CTLV_TEST_CHILD_FAIL_FAMILY_PRE : CTLV_TEST_FAIL_FAMILY_PRE;

	ASSERT_EQ(0, ctlv_sysfs_write(fail, "1"));
	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(0, ctlv_exec_reply(self->fd, CTLV_OP_EVENT_QUEUE_GET,
				     self->reply, sizeof(self->reply)));
	EXPECT_NO_HOOKS();
	if (ctlv_child_logs_available())
		EXPECT_NO_CHILD_HOOKS();
	ASSERT_EQ(0, ctlv_sysfs_write(fail, "0"));
}

/* What the core refuses on its own never enters the family. */
TEST_F(device, nothing_the_core_refuses_reaches_a_hook)
{
	struct ctlv_op_exec malformed = {
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.request = ctlv_ptr(self->reply),
		.request_len = 16,
		.reply = ctlv_ptr(self->reply),
		.reply_len = variant->query_reply_min,
	};
	int read_only = ctlv_open(variant->family, 0, O_RDONLY);
	char actual[CTLV_LOG_LEN];

	ASSERT_LE(0, read_only);
	ASSERT_EQ(0, ctlv_hooks_clear());

	EXPECT_EQ(-EINVAL, ctlv_exec_bare(self->fd, 0));
	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(self->fd, CTLV_OP_ID_NOBODY));
	/* An event, which is built by a family and never executed. */
	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(self->fd, CTLV_TEST_OP_PINGED));
	/* An op requiring full control on an observation file. */
	EXPECT_EQ(-EACCES, ctlv_exec_bare(read_only, CTLV_TEST_OP_ECHO));
	/* An output buffer too small for what the op could answer. */
	EXPECT_EQ(-EMSGSIZE,
		  ctlv_exec_bare(self->fd, CTLV_TEST_OP_PROBE_GET));
	/* And a request whose root claims to run past the message. */
	ctlv_root_init(self->reply, 24);
	EXPECT_EQ(-EINVAL, ctlv_exec(self->fd, &malformed));

	ASSERT_EQ(0, ctlv_hooks_read(actual, sizeof(actual)));
	EXPECT_STREQ("", actual);
	if (ctlv_child_logs_available())
		EXPECT_NO_CHILD_HOOKS();
	close(read_only);
}

/* Three core queries: the chain, the published ops, and one op's schema. */

/* Any of these replies whole, several times over. */
#define INTRO_REPLY_LEN 16384

/* More fragments than the smallest buffer takes for any of them. */
#define INTRO_MAX_FRAGS 512

/* Every op of the chain: ten of the root family, eleven of the test family. */
#define INTRO_OPS_PARENT 21
#define INTRO_OPS_CHILD (INTRO_OPS_PARENT + 1)

/* A header on its own, which is an empty message, and one with a scalar. */
#define INTRO_ROOT_LEN ((unsigned int)sizeof(struct ctlv_attr))
#define INTRO_SCALAR_LEN \
	((unsigned int)(sizeof(struct ctlv_attr) + sizeof(__u64)))

struct intro {
	/* The whole logical reply, as its fragments reassemble into it. */
	struct reply_text logical;
	unsigned int frags;
	__u64 generation;
	__u64 cursor;
	/* More fragments than this can hold, so nothing else means anything. */
	bool overflow;
	/* A fragment that said two different things about being the last. */
	bool marked_wrong;
};

/* One whole query as the one line its fragments reassemble into. */
static int intro_query(struct intro *intro, int fd, __u64 op_id,
		       struct ctlv_tlv *request, void *reply, __u32 reply_len)
{
	const struct ctlv_attr *root = reply;
	struct ctlv_op_exec exec = {
		.op_id = op_id,
		.reply = ctlv_ptr(reply),
		.reply_len = reply_len,
	};
	unsigned int open = 0;
	int ret;

	if (request) {
		exec.request = ctlv_ptr(request->buf);
		exec.request_len = ctlv_tlv_done(request);
	} else {
		ctlv_exec_set_empty_request(&exec);
	}
	memset(intro, 0, sizeof(*intro));
	do {
		ret = ctlv_exec(fd, &exec);
		if (ret)
			return ret;
		if (intro->frags == INTRO_MAX_FRAGS) {
			intro->overflow = true;
			return 0;
		}
		reply_render(&intro->logical, root, open);
		open = frag_open(root);
		if (!(root->flags & CTLV_ATTR_F_FRAG_MORE) != !exec.cursor ||
		    (!exec.cursor && open))
			intro->marked_wrong = true;
		intro->frags++;
		intro->generation = exec.generation;
		intro->cursor = exec.cursor;
	} while (exec.cursor);
	return 0;
}

/* The schema of one op, which is the one core query with a request. */
static int intro_schema(struct intro *intro, int fd, __u64 op_id, void *reply,
			__u32 reply_len)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID, op_id);
	return intro_query(intro, fd, CTLV_OP_OP_SCHEMA_GET, &tlv, reply,
			   reply_len);
}

/* One entry of an ops dump, found by the op it is about. */
static const struct ctlv_attr *intro_entry(const void *reply, __u64 op_id)
{
	const struct ctlv_attr *entry, *child;

	ctlv_rd_for_each(entry, (const struct ctlv_attr *)reply)
		ctlv_rd_for_each(child, entry)
			if (child->attr_id == CTLV_ATTR_OP_ID &&
			    ctlv_rd_u64(child) == op_id)
				return entry;
	return NULL;
}

static __u64 intro_entry_u64(const struct ctlv_attr *entry, __u64 attr_id)
{
	const struct ctlv_attr *child;

	ctlv_rd_for_each(child, entry)
		if (child->attr_id == attr_id)
			return ctlv_rd_u64(child);
	return 0;
}

/* Asked of the device, so a sweep starts at the first allowed length. */
static __u32 intro_reply_min(int fd, __u64 op_id)
{
	char reply[INTRO_REPLY_LEN];
	const struct ctlv_attr *entry;
	struct intro intro;

	if (intro_query(&intro, fd, CTLV_OP_OPS_DUMP, NULL, reply,
			sizeof(reply)))
		return 0;
	entry = intro_entry(reply, op_id);
	if (!entry)
		return 0;
	return intro_entry_u64(entry, CTLV_ATTR_OP_QUERY_REPLY_MIN_LEN);
}

/* How many times one rendering appears in another. */
static unsigned int intro_count(const char *text, const char *what)
{
	unsigned int found = 0;
	const char *at = text;

	while ((at = strstr(at, what))) {
		found++;
		at++;
	}
	return found;
}

/* Its bounds are what the kind allows rather than what the spec said. */
static void intro_want_uint(struct reply_text *want, const char *placement,
			    __u64 attr_id)
{
	reply_add(want, "%s={attr-id=%#llx kind=%u occurrences=1", placement,
		  (unsigned long long)attr_id, CTLV_SCHEMA_UINT);
	reply_add(want, " min=0 max=0xffffffffffffffff}");
}

FIXTURE(intro) {
	int fd;
	char reply[INTRO_REPLY_LEN];
};

FIXTURE_VARIANT(intro) {
	const char *family;
	/* The chain this device answers with, root first. */
	const char *chain;
	/* Whether the family fills the extension point it inherited. */
	bool inheriting;
	/* What dynamic-noop's entry says before anything has toggled it. */
	bool dynamic_disabled;
};

/* A descendant's chain is its parent's chain and one more record. */
#define INTRO_CHAIN_ROOT "family={id=0 name=ctlv}"
#define INTRO_CHAIN_PARENT INTRO_CHAIN_ROOT " family={id=3 name=ctlv-test}"
#define INTRO_CHAIN_CHILD \
	INTRO_CHAIN_PARENT " family={id=4 name=ctlv-test-child}"

FIXTURE_VARIANT_ADD(intro, parent) {
	.family = "ctlv-test",
	.chain = INTRO_CHAIN_PARENT,
	.inheriting = false,
	.dynamic_disabled = true,
};

FIXTURE_VARIANT_ADD(intro, child) {
	.family = "ctlv-test-child",
	.chain = INTRO_CHAIN_CHILD,
	.inheriting = true,
	.dynamic_disabled = false,
};

FIXTURE_SETUP(intro)
{
	self->fd = ctlv_open(variant->family, 0, O_RDWR);
	if (self->fd < 0)
		SKIP(return, "no %s device: %s", variant->family,
		     strerror(-self->fd));
}

FIXTURE_TEARDOWN(intro)
{
	if (self->fd >= 0)
		close(self->fd);
}

/* Root first and the family that registered last, which names the UAPI. */
TEST_F(intro, the_chain_of_the_device)
{
	struct intro intro;

	ASSERT_EQ(0, intro_query(&intro, self->fd, CTLV_OP_FAMILIES_DUMP, NULL,
				 self->reply, sizeof(self->reply)));
	EXPECT_EQ(1, intro.frags);
	EXPECT_STREQ(variant->chain, intro.logical.buf);
	/* Immutable, so no fragment of it can be stale against another. */
	EXPECT_EQ(0, intro.generation);
}

/* The same chain the device announced to userspace before it was opened. */
TEST_F(intro, the_chain_agrees_with_what_the_device_announced)
{
	char device[CTLV_NAME_LEN], property[64], announced[CTLV_NAME_LEN];
	struct reply_text text = { .used = 0 };
	const struct ctlv_attr *entry, *child;
	struct intro intro;
	unsigned int n = 0;

	ASSERT_EQ(0, ctlv_device_name(variant->family, 0, device,
				      sizeof(device)));
	ASSERT_EQ(0, intro_query(&intro, self->fd, CTLV_OP_FAMILIES_DUMP, NULL,
				 self->reply, sizeof(self->reply)));

	ctlv_rd_for_each(entry, (const struct ctlv_attr *)self->reply) {
		ASSERT_EQ(CTLV_ATTR_FAMILY_ENTRY, entry->attr_id);
		snprintf(property, sizeof(property), "CTLV_COMPATIBLE_%u", n++);
		ASSERT_EQ(0, ctlv_uevent_read(device, property, announced,
					      sizeof(announced)));
		ctlv_rd_for_each(child, entry)
			if (child->attr_id == CTLV_ATTR_FAMILY_NAME)
				EXPECT_STREQ(announced,
					     (const char *)(child + 1));
	}
	/* And is the whole of that chain rather than a prefix of it. */
	ASSERT_EQ(0, ctlv_uevent_read(device, "CTLV_COMPATIBLE_N", announced,
				      sizeof(announced)));
	reply_add(&text, "%u", n);
	EXPECT_STREQ(announced, text.buf);
}

/* What every entry of an executable op begins with. */
static void intro_want_op(struct reply_text *want, __u64 op_id,
			  unsigned int type, unsigned int availability,
			  bool disabled)
{
	reply_add(want, "op-id=%#llx type=%u", (unsigned long long)op_id,
		  type);
	reply_add(want, " availability=%u disabled=%u", availability,
		  disabled);
}

/* What the entry of one op says, or nothing where there is no entry. */
#define EXPECT_OP_ENTRY(_reply, _op_id, _expected) do {			\
	struct reply_text text = { .used = 0 };				\
	const struct ctlv_attr *entry;					\
									\
	entry = intro_entry(_reply, _op_id);				\
	if (entry)							\
		reply_render(&text, entry, 0);				\
	EXPECT_STREQ(_expected, text.buf);				\
} while (0)

/* One entry per published op, carrying the lengths this device admits. */
TEST_F(intro, what_one_entry_says_about_one_op)
{
	struct reply_text want = { .used = 0 };
	struct intro intro;

	ASSERT_EQ(0, intro_query(&intro, self->fd, CTLV_OP_OPS_DUMP, NULL,
				 self->reply, sizeof(self->reply)));
	ASSERT_FALSE(intro.marked_wrong);

	/* An action with an empty request and no reply. */
	intro_want_op(&want, CTLV_TEST_OP_NOOP, CTLV_OP_TYPE_ACTION,
		      CTLV_OP_AVAILABILITY_FIXED, false);
	reply_add(&want, " request-max=%u action-reply-max=0", INTRO_ROOT_LEN);
	reply_add(&want, " fd-access=%u", CTLV_OP_FD_ACCESS_READ);
	EXPECT_OP_ENTRY(self->reply, CTLV_TEST_OP_NOOP, want.buf);

	/* A query, told what to bring, its request growing with the child's. */
	want.used = 0;
	intro_want_op(&want, CTLV_TEST_OP_PROBE_GET, CTLV_OP_TYPE_QUERY,
		      CTLV_OP_AVAILABILITY_FIXED, false);
	reply_add(&want, " request-max=%u",
		  INTRO_ROOT_LEN + INTRO_SCALAR_LEN);
	reply_add(&want, " query-reply-min=%u", variant->inheriting ?
		  2 * INTRO_ROOT_LEN + INTRO_SCALAR_LEN :
		  INTRO_ROOT_LEN + INTRO_SCALAR_LEN);
	reply_add(&want, " query-reply-suggested=65536");
	reply_add(&want, " fd-access=%u", CTLV_OP_FD_ACCESS_READ);
	EXPECT_OP_ENTRY(self->reply, CTLV_TEST_OP_PROBE_GET, want.buf);

	/* An event is produced rather than executed, so it has no state. */
	want.used = 0;
	reply_add(&want, "op-id=%#llx type=%u event-record-max=%u",
		  (unsigned long long)CTLV_TEST_OP_PINGED, CTLV_OP_TYPE_EVENT,
		  (unsigned int)sizeof(struct ctlv_event_hdr) +
		  INTRO_ROOT_LEN);
	EXPECT_OP_ENTRY(self->reply, CTLV_TEST_OP_PINGED, want.buf);
}

/* Listed whichever state it is in, its entry saying which that is. */
TEST_F(intro, a_disabled_op_is_listed_as_disabled)
{
	struct reply_text want;
	struct intro intro;
	unsigned int round;

	for (round = 0; round < 2; round++) {
		bool disabled = round ? !variant->dynamic_disabled :
					variant->dynamic_disabled;

		ASSERT_EQ(0, intro_query(&intro, self->fd, CTLV_OP_OPS_DUMP,
					 NULL, self->reply,
					 sizeof(self->reply)));
		want.used = 0;
		intro_want_op(&want, CTLV_TEST_OP_DYNAMIC_NOOP,
			      CTLV_OP_TYPE_ACTION,
			      CTLV_OP_AVAILABILITY_DYNAMIC, disabled);
		reply_add(&want, " request-max=%u action-reply-max=0",
			  INTRO_ROOT_LEN);
		reply_add(&want, " fd-access=%u",
			  CTLV_OP_FD_ACCESS_READ_WRITE);
		EXPECT_OP_ENTRY(self->reply, CTLV_TEST_OP_DYNAMIC_NOOP,
				want.buf);
		ASSERT_EQ(0, dynamic_toggle(self->fd));
	}
}

TEST_F(intro, every_op_of_the_chain_is_there)
{
	const struct ctlv_attr *entry, *child;
	unsigned int entries = 0;
	struct intro intro;

	ASSERT_EQ(0, intro_query(&intro, self->fd, CTLV_OP_OPS_DUMP, NULL,
				 self->reply, sizeof(self->reply)));

	ctlv_rd_for_each(entry, (const struct ctlv_attr *)self->reply) {
		__u64 previous = 0;

		EXPECT_EQ(CTLV_ATTR_OP_ENTRY, entry->attr_id);
		ctlv_rd_for_each(child, entry) {
			if (child->attr_id != CTLV_ATTR_OP_ID)
				continue;
			/* Ascending, which is the order the index is in. */
			EXPECT_LT(previous, ctlv_rd_u64(child));
			previous = ctlv_rd_u64(child);
		}
		entries++;
	}
	EXPECT_EQ(variant->inheriting ? INTRO_OPS_CHILD : INTRO_OPS_PARENT,
		  entries);
}

/* A change under a continuation is a refusal, not a mixture of two. */
TEST_F(intro, an_availability_change_between_two_fragments)
{
	struct ctlv_op_exec exec = {
		.op_id = CTLV_OP_OPS_DUMP,
		.reply = ctlv_ptr(self->reply),
		/* Room for one entry at a time, so there is a next one. */
		.reply_len = 136,
	};
	char expected[CTLV_LOG_LEN];
	__u64 generation;

	ctlv_exec_set_empty_request(&exec);
	ASSERT_EQ(0, ctlv_exec(self->fd, &exec));
	ASSERT_NE(0, exec.cursor);
	generation = exec.generation;

	ASSERT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(-ESTALE, ctlv_exec(self->fd, &exec));
	snprintf(expected, sizeof(expected),
		 "reason=%u expected-generation=%llu current-generation=%llu",
		 CTLV_VALIDATION_REASON_STALE_GENERATION, generation,
		 generation + 1);
	EXPECT_REPLY(expected);

	/* A dump started after the change reads it and is not stale. */
	exec.cursor = 0;
	exec.generation = 0;
	EXPECT_EQ(0, ctlv_exec(self->fd, &exec));
	EXPECT_EQ(generation + 1, exec.generation);
	ASSERT_EQ(0, dynamic_toggle(self->fd));
}

/* Numbers and no names; a required and an optional placement are two attrs. */
TEST_F(intro, the_schema_of_one_op)
{
	struct reply_text want = { .used = 0 };
	struct intro intro;

	ASSERT_EQ(0, intro_schema(&intro, self->fd, CTLV_TEST_OP_PROBE_GET,
				  self->reply, sizeof(self->reply)));

	/* The root of a message is a node like any other, and carries none. */
	reply_add(&want, "request={attr-id=0 kind=%u occurrences=1 ",
		  CTLV_SCHEMA_NEST);
	intro_want_uint(&want, "optional", CTLV_TEST_ATTR_PROBE_TOKEN);
	reply_add(&want, "} reply={attr-id=0 kind=%u occurrences=1 ",
		  CTLV_SCHEMA_NEST);
	intro_want_uint(&want, "optional", CTLV_TEST_ATTR_PROBE_TOKEN);
	reply_add(&want, " optional={attr-id=%#llx kind=%u occurrences=1",
		  (unsigned long long)CTLV_TEST_ATTR_PROBE_EXTENSION,
		  CTLV_SCHEMA_NEST);
	/* A member of the nest that declared it; empty where nothing did. */
	if (variant->inheriting) {
		reply_add(&want, " ");
		intro_want_uint(&want, "optional",
				CTLV_TEST_CHILD_ATTR_CHILD_EXTRA);
	}
	reply_add(&want, "}}");
	EXPECT_STREQ(want.buf, intro.logical.buf);
}

/* No reply tree at all is how a caller knows the op answers with none. */
TEST_F(intro, the_schema_of_an_op_that_carries_nothing)
{
	struct reply_text want = { .used = 0 };
	struct intro intro;

	ASSERT_EQ(0, intro_schema(&intro, self->fd, CTLV_TEST_OP_NOOP,
				  self->reply, sizeof(self->reply)));
	reply_add(&want, "request={attr-id=0 kind=%u occurrences=1}",
		  CTLV_SCHEMA_NEST);
	EXPECT_STREQ(want.buf, intro.logical.buf);
}

/* Unit and scale are not wire rules, so they appear here and nowhere else. */
TEST_F(intro, what_a_payload_means_rather_than_what_is_valid)
{
	struct reply_text want = { .used = 0 };
	struct intro intro;

	ASSERT_EQ(0, intro_schema(&intro, self->fd, CTLV_OP_EVENT_QUEUE_GET,
				  self->reply, sizeof(self->reply)));
	reply_add(&want, "request={attr-id=0 kind=%u occurrences=1} ",
		  CTLV_SCHEMA_NEST);
	reply_add(&want, "reply={attr-id=0 kind=%u occurrences=1 ",
		  CTLV_SCHEMA_NEST);
	reply_add(&want, "attr={attr-id=%#llx kind=%u occurrences=1 ",
		  (unsigned long long)CTLV_ATTR_EVENT_QUEUE_LEN,
		  CTLV_SCHEMA_UINT);
	reply_add(&want, "min=0x1 max=0x1000000 unit=%u scale=0} ",
		  CTLV_UNIT_BYTES);
	reply_add(&want, "attr={attr-id=%#llx kind=%u occurrences=1 ",
		  (unsigned long long)CTLV_ATTR_EVENT_QUEUE_USED,
		  CTLV_SCHEMA_UINT);
	reply_add(&want, "min=0 max=0xffffffffffffffff unit=%u scale=0}}",
		  CTLV_UNIT_BYTES);
	EXPECT_STREQ(want.buf, intro.logical.buf);
}

/* Re-entering a level is what makes unbounded depth a finite reply. */
TEST_F(intro, a_schema_that_re_enters_a_level)
{
	struct reply_text want = { .used = 0 };
	struct intro intro;

	ASSERT_EQ(0, intro_schema(&intro, self->fd, CTLV_OP_OP_SCHEMA_GET,
				  self->reply, sizeof(self->reply)));

	/* Neither placement carries members of its own. */
	reply_add(&want, "optional={attr-id=%#llx kind=%u occurrences=0 ",
		  (unsigned long long)CTLV_ATTR_SCHEMA_ATTRS,
		  CTLV_SCHEMA_NEST);
	reply_add(&want, "reenters=1} ");
	reply_add(&want, "optional={attr-id=%#llx kind=%u occurrences=0 ",
		  (unsigned long long)CTLV_ATTR_SCHEMA_OPTIONAL_ATTRS,
		  CTLV_SCHEMA_NEST);
	reply_add(&want, "reenters=1}");
	EXPECT_NE(NULL, strstr(intro.logical.buf, want.buf)) {
		TH_LOG("%s", intro.logical.buf);
	}

	/* Once per message of the op, which is where the level is described. */
	EXPECT_EQ(2, intro_count(intro.logical.buf, want.buf));
	EXPECT_EQ(4, intro_count(intro.logical.buf, "reenters="));
}

/* What an op may carry does not change with whether it can be called. */
TEST_F(intro, the_schema_of_an_op_that_is_not_there)
{
	struct intro intro;

	EXPECT_EQ(-EOPNOTSUPP, intro_schema(&intro, self->fd, CTLV_OP_ID_NOBODY,
					    self->reply,
					    sizeof(self->reply)));
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(0, intro_schema(&intro, self->fd, CTLV_TEST_OP_DYNAMIC_NOOP,
				  self->reply, sizeof(self->reply)));
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	EXPECT_EQ(0, intro_schema(&intro, self->fd, CTLV_TEST_OP_DYNAMIC_NOOP,
				  self->reply, sizeof(self->reply)));
}

/* A boundary inside a nested schema is the case that matters. */
TEST_F(intro, fragments_of_an_answer_reassemble_into_it)
{
	static const struct {
		const char *name;
		/* The query being cut up, and the op it is asked about. */
		__u64 query;
		__u64 subject;
	} cases[] = {
		{ "the chain", CTLV_OP_FAMILIES_DUMP },
		{ "the ops", CTLV_OP_OPS_DUMP },
		{ "the schema of a nested reply", CTLV_OP_OP_SCHEMA_GET,
		  CTLV_TEST_OP_ECHO_REPLY },
		{ "the schema of a schema", CTLV_OP_OP_SCHEMA_GET,
		  CTLV_OP_OP_SCHEMA_GET },
	};
	char whole[CTLV_LOG_LEN];
	unsigned int index;
	struct intro intro;
	__u32 len, min, all;

	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		struct ctlv_tlv tlv, *request = NULL;

		if (cases[index].subject) {
			ctlv_tlv_init(&tlv);
			ctlv_tlv_u64(&tlv, CTLV_ATTR_OP_ID,
				     cases[index].subject);
			request = &tlv;
		}
		ASSERT_EQ(0, intro_query(&intro, self->fd, cases[index].query,
					 request, self->reply,
					 sizeof(self->reply)));
		ASSERT_EQ(1, intro.frags) { TH_LOG("%s", cases[index].name); }
		snprintf(whole, sizeof(whole), "%s", intro.logical.buf);
		all = ((const struct ctlv_attr *)self->reply)->len;

		/* Every length from the smallest it takes to the whole. */
		min = intro_reply_min(self->fd, cases[index].query);
		ASSERT_LT(0, min) { TH_LOG("%s", cases[index].name); }
		for (len = min; len <= all; len++) {
			if (request) {
				ctlv_tlv_init(request);
				ctlv_tlv_u64(request, CTLV_ATTR_OP_ID,
					     cases[index].subject);
			}
			ASSERT_EQ(0, intro_query(&intro, self->fd,
						 cases[index].query, request,
						 self->reply, len)) {
				TH_LOG("%s at %u", cases[index].name, len);
			}
			ASSERT_FALSE(intro.overflow) {
				TH_LOG("%s at %u", cases[index].name, len);
			}
			EXPECT_FALSE(intro.marked_wrong) {
				TH_LOG("%s at %u", cases[index].name, len);
			}
			EXPECT_STREQ(whole, intro.logical.buf) {
				TH_LOG("%s at %u", cases[index].name, len);
			}
		}
	}
}

/* A queue is bytes rather than records, which the lengths below are in. */

/* One attr with a payload of this length, as a message places it. */
#define EVENT_ATTR_LEN(_payload) \
	CTLV_ALIGN(sizeof(struct ctlv_attr) + (_payload))

/* And one record: the header, then the payload root and what is inside it. */
#define EVENT_RECORD_LEN(_inside) \
	(sizeof(struct ctlv_event_hdr) + EVENT_ATTR_LEN(_inside))

/* The filler one event may carry at most, which is ping.payload's maximum. */
#define PING_FILLER_LEN 1024

/* pinged carries nothing at all, and beeped a serial and its filler. */
#define PINGED_LEN EVENT_RECORD_LEN(0)
#define BEEPED_LEN EVENT_RECORD_LEN(EVENT_ATTR_LEN(sizeof(__u64)))
#define BEEPED_FILLED_LEN(_filler) (BEEPED_LEN + EVENT_ATTR_LEN(_filler))

/* Root-family event lengths. */
#define OP_CHANGED_LEN							\
	EVENT_RECORD_LEN(2 * EVENT_ATTR_LEN(sizeof(__u64)) +		\
			 EVENT_ATTR_LEN(sizeof(__u8)))
#define EVENT_OVERFLOW_LEN EVENT_RECORD_LEN(3 * EVENT_ATTR_LEN(sizeof(__u64)))
#define DEVICE_REMOVED_LEN EVENT_RECORD_LEN(0)

/* Charged for the two records a queue may not lose, plus one largest event. */
#define EVENT_RESERVED (EVENT_OVERFLOW_LEN + DEVICE_REMOVED_LEN)
#define EVENT_QUEUE_MIN (BEEPED_FILLED_LEN(PING_FILLER_LEN) + EVENT_RESERVED)

/* One record as it was read, in a buffer aligned as a root has to be. */
struct event {
	ssize_t len;
	__u64 buf[2048 / sizeof(__u64)];
};

static const struct ctlv_event_hdr *event_hdr(const struct event *ev)
{
	return (const struct ctlv_event_hdr *)ev->buf;
}

static const struct ctlv_attr *event_root(const struct event *ev)
{
	return (const struct ctlv_attr *)(event_hdr(ev) + 1);
}

/* One record into as much room as a caller offers, or -errno. */
static ssize_t event_read_room(int fd, struct event *ev, size_t room)
{
	memset(ev, 0, sizeof(*ev));
	ev->len = read(fd, ev->buf, room);
	if (ev->len < 0)
		ev->len = -errno;
	return ev->len;
}

/* And into room enough for any of them, -EAGAIN saying nothing arrived. */
static ssize_t event_read(int fd, struct event *ev)
{
	return event_read_room(fd, ev, sizeof(ev->buf));
}

static const struct ctlv_attr *event_attr(const struct event *ev,
					  __u64 attr_id)
{
	const struct ctlv_attr *child;

	ctlv_rd_for_each(child, event_root(ev))
		if (child->attr_id == attr_id)
			return child;
	return NULL;
}

/* What a record says, or zero where it says nothing. */
static __u64 event_u64(const struct event *ev, __u64 attr_id)
{
	const struct ctlv_attr *attr = event_attr(ev, attr_id);

	return attr ? ctlv_rd_u64(attr) : 0;
}

/* The event, its place in the description's numbering, and a whole root. */
#define EXPECT_RECORD(_ev, _op_id, _sequence) do {			\
	const struct event *_r = (_ev);					\
									\
	EXPECT_EQ((_op_id), event_hdr(_r)->op_id);			\
	EXPECT_EQ((_sequence), event_hdr(_r)->sequence);			\
	EXPECT_EQ(0, event_hdr(_r)->flags);				\
	EXPECT_EQ(_r->len, event_hdr(_r)->length);			\
	EXPECT_EQ(CTLV_ATTR_ROOT, event_root(_r)->attr_id);		\
	EXPECT_EQ(CTLV_ATTR_F_NESTED, event_root(_r)->flags);		\
	EXPECT_EQ(_r->len, EVENT_RECORD_LEN(event_root(_r)->len -	\
					    sizeof(struct ctlv_attr)));	\
} while (0)

/* Subscribing to what a list names, or giving all of it up again. */
static int event_subscriptions(int fd, bool add, const __u64 *op_ids,
			       unsigned int ops_count)
{
	struct ctlv_tlv tlv;
	unsigned int index;

	ctlv_tlv_init(&tlv);
	for (index = 0; index < ops_count; index++)
		ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_OP_ID, op_ids[index]);
	return ctlv_exec_request(fd, add ? CTLV_OP_SUBSCRIPTIONS_ADD :
					   CTLV_OP_SUBSCRIPTIONS_REMOVE,
				 tlv.buf, ctlv_tlv_done(&tlv));
}

static int event_subscribe(int fd, __u64 op_id)
{
	return event_subscriptions(fd, true, &op_id, 1);
}

static int event_unsubscribe(int fd, __u64 op_id)
{
	return event_subscriptions(fd, false, &op_id, 1);
}

/* What a description's queue is, as the core reports it. */
struct event_queue {
	__u64 len;
	__u64 used;
};

static int event_queue_get(int fd, struct event_queue *queue)
{
	__u64 reply[16];
	const struct ctlv_attr *root = (const struct ctlv_attr *)reply;
	const struct ctlv_attr *child;
	int ret;

	*queue = (struct event_queue){};
	ret = ctlv_exec_reply(fd, CTLV_OP_EVENT_QUEUE_GET, reply,
			      sizeof(reply));
	if (ret)
		return ret;
	ctlv_rd_for_each(child, root) {
		if (child->attr_id == CTLV_ATTR_EVENT_QUEUE_LEN)
			queue->len = ctlv_rd_u64(child);
		else if (child->attr_id == CTLV_ATTR_EVENT_QUEUE_USED)
			queue->used = ctlv_rd_u64(child);
	}
	return 0;
}

static int event_queue_set_reply(int fd, __u64 len, void *reply,
				 __u32 reply_len)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_QUEUE_LEN, len);
	return ctlv_exec_full(fd, CTLV_OP_EVENT_QUEUE_SET, tlv.buf,
			      ctlv_tlv_done(&tlv), reply, reply_len);
}

static int event_queue_set(int fd, __u64 len)
{
	return event_queue_set_reply(fd, len, NULL, 0);
}

/* @empty selects the event that carries nothing, @abort throws it away. */
struct burst {
	__u64 count;
	__u64 serial;
	__u64 filler;
	bool empty;
	bool abort;
};

static int event_burst(int fd, const struct burst *burst)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	if (burst->count)
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_COUNT, burst->count);
	if (burst->serial)
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_SERIAL, burst->serial);
	if (burst->filler)
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_PAYLOAD, burst->filler);
	if (burst->empty)
		ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_PING_EMPTY, true);
	if (burst->abort)
		ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_PING_ABORT, true);
	return ctlv_exec_request(fd, CTLV_TEST_OP_PING, tlv.buf,
				 ctlv_tlv_done(&tlv));
}

/* The usual burst, which is this many beeped records carrying no filler. */
static int event_beep(int fd, __u64 count)
{
	const struct burst burst = { .count = count };

	return event_burst(fd, &burst);
}

/* One descriptor over a socket, which is the other way a queue is shared. */
static int fd_send(int sock, int fd)
{
	union {
		char bytes[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} control = {};
	struct iovec iov = { .iov_base = &fd, .iov_len = sizeof(fd) };
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = control.bytes,
		.msg_controllen = sizeof(control.bytes),
	};
	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
	memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
	return sendmsg(sock, &msg, 0) < 0 ? -errno : 0;
}

static int fd_recv(int sock)
{
	union {
		char bytes[CMSG_SPACE(sizeof(int))];
		struct cmsghdr align;
	} control = {};
	int fd = -EBADF, payload = 0;
	struct iovec iov = { .iov_base = &payload, .iov_len = sizeof(payload) };
	struct msghdr msg = {
		.msg_iov = &iov,
		.msg_iovlen = 1,
		.msg_control = control.bytes,
		.msg_controllen = sizeof(control.bytes),
	};
	struct cmsghdr *cmsg;

	if (recvmsg(sock, &msg, 0) < 0)
		return -errno;
	cmsg = CMSG_FIRSTHDR(&msg);
	if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS)
		return -EBADF;
	memcpy(&fd, CMSG_DATA(cmsg), sizeof(fd));
	return fd;
}

FIXTURE(events) {
	int fd;
	struct event ev;
	char reply[1024];
};

FIXTURE_VARIANT(events) {
	const char *family;
	/* What dynamic-noop is before anything has toggled it. */
	bool dynamic_disabled;
};

FIXTURE_VARIANT_ADD(events, parent) {
	.family = "ctlv-test",
	.dynamic_disabled = true,
};

FIXTURE_VARIANT_ADD(events, child) {
	.family = "ctlv-test-child",
	.dynamic_disabled = false,
};

/* O_NONBLOCK: an empty queue answering -EAGAIN is how a case says so. */
FIXTURE_SETUP(events)
{
	self->fd = ctlv_open(variant->family, 0, O_RDWR | O_NONBLOCK);
	if (self->fd < 0)
		SKIP(return, "no %s device: %s", variant->family,
		     strerror(-self->fd));
}

FIXTURE_TEARDOWN(events)
{
	if (self->fd >= 0)
		close(self->fd);
}

/* A description starts subscribed to nothing, so a burst reaches nobody. */
TEST_F(events, nothing_arrives_unasked)
{
	ASSERT_EQ(0, event_beep(self->fd, 3));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

TEST_F(events, an_observation_file_can_monitor)
{
	int fd = ctlv_open(variant->family, 0, O_RDONLY | O_NONBLOCK);
	struct event_queue queue;

	ASSERT_LE(0, fd);
	ASSERT_EQ(0, event_queue_get(fd, &queue));
	ASSERT_EQ(0, event_queue_set(fd, queue.len));
	ASSERT_EQ(0, event_subscribe(fd, CTLV_TEST_OP_BEEPED));
	EXPECT_EQ(-EACCES, event_beep(fd, 1));

	ASSERT_EQ(0, event_beep(self->fd, 1));
	ASSERT_EQ(BEEPED_LEN, event_read(fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
	close(fd);
}

/* And one that asked is sent every event of the burst, once, in order. */
TEST_F(events, one_read_is_one_record)
{
	const struct burst burst = { .count = 3, .serial = 5 };
	unsigned int index;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_burst(self->fd, &burst));

	for (index = 0; index < burst.count; index++) {
		ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, index + 1);
		EXPECT_EQ(burst.serial + index,
			  event_u64(&self->ev, CTLV_TEST_ATTR_PING_SERIAL));
	}
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* Everything the event was built from is in the record, filler and all. */
TEST_F(events, a_record_carries_what_the_event_carried)
{
	const struct burst burst = {
		.count = 1,
		.serial = 9,
		.filler = PING_FILLER_LEN,
	};
	const struct ctlv_attr *filler;
	const __u8 *bytes;
	__u32 len, index;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_burst(self->fd, &burst));

	ASSERT_EQ(BEEPED_FILLED_LEN(PING_FILLER_LEN),
		  event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
	EXPECT_EQ(burst.serial,
		  event_u64(&self->ev, CTLV_TEST_ATTR_PING_SERIAL));

	filler = event_attr(&self->ev, CTLV_TEST_ATTR_PING_FILLER);
	ASSERT_NE(NULL, filler);
	bytes = ctlv_rd_payload(filler, &len);
	ASSERT_EQ(PING_FILLER_LEN, len);
	/* Which the family filled with the position of each byte. */
	for (index = 0; index < len; index++)
		ASSERT_EQ((__u8)index, bytes[index]);
}

/* An event whose whole payload is that it happened is a record too. */
TEST_F(events, an_event_that_carries_nothing)
{
	const struct burst burst = { .count = 1, .empty = true };

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_PINGED));
	ASSERT_EQ(0, event_burst(self->fd, &burst));

	ASSERT_EQ(PINGED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_PINGED, 1);
	EXPECT_EQ(sizeof(struct ctlv_attr), event_root(&self->ev)->len);
}

/* An unsubscribed event takes no number from the ones subscribed to. */
TEST_F(events, an_event_other_than_the_one_asked_for)
{
	const struct burst other = { .count = 2, .empty = true };

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_burst(self->fd, &other));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));

	ASSERT_EQ(0, event_beep(self->fd, 1));
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
}

/* An event that was built and thrown away is one that never happened. */
TEST_F(events, an_event_that_was_aborted)
{
	const struct burst aborted = { .count = 4, .abort = true };

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_burst(self->fd, &aborted));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));

	ASSERT_EQ(0, event_beep(self->fd, 1));
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
}

/* A set, so asking twice adds nothing and dropping twice removes nothing. */
TEST_F(events, a_subscription_asked_for_twice)
{
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 1));
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));

	ASSERT_EQ(0, event_unsubscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_unsubscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 1));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* All of a request or none, so a bad ID leaves the set as it was. */
TEST_F(events, a_subscription_to_what_is_not_an_event)
{
	const __u64 both[] = { CTLV_TEST_OP_BEEPED, CTLV_TEST_OP_NOOP };
	struct ctlv_tlv empty;

	EXPECT_EQ(-EOPNOTSUPP, event_subscribe(self->fd, CTLV_TEST_OP_NOOP));
	EXPECT_EQ(-EOPNOTSUPP, event_subscribe(self->fd, CTLV_OP_ID_NOBODY));
	EXPECT_EQ(-EOPNOTSUPP, event_unsubscribe(self->fd, CTLV_TEST_OP_NOOP));
	EXPECT_EQ(-EOPNOTSUPP,
		  event_subscriptions(self->fd, true, both, ARRAY_SIZE(both)));

	/* And a request that names nothing is not a request at all. */
	ctlv_tlv_init(&empty);
	EXPECT_EQ(-EINVAL,
		  ctlv_exec_request(self->fd, CTLV_OP_SUBSCRIPTIONS_ADD,
				    empty.buf, ctlv_tlv_done(&empty)));

	ASSERT_EQ(0, event_beep(self->fd, 1));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* One event, one record per description, each numbered as its own. */
TEST_F(events, every_subscriber_gets_its_own_record)
{
	int other = ctlv_open(variant->family, 0, O_RDWR | O_NONBLOCK);
	struct event ev;

	ASSERT_LE(0, other);
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_subscribe(other, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 2));

	/* The one that did not ask for the burst reads it first. */
	ASSERT_EQ(BEEPED_LEN, event_read(other, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(BEEPED_LEN, event_read(other, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 2);
	EXPECT_EQ(-EAGAIN, event_read(other, &ev));

	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 2);
	close(other);
}

/* A description that never reads spends its own queue and nothing else. */
TEST_F(events, a_description_that_never_reads)
{
	int slow = ctlv_open(variant->family, 0, O_RDWR | O_NONBLOCK);
	const unsigned int records = 64;
	unsigned int index;

	ASSERT_LE(0, slow);
	ASSERT_EQ(0, event_queue_set(slow, EVENT_QUEUE_MIN));
	ASSERT_EQ(0, event_subscribe(slow, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));

	/* Far more than the small queue holds, and far less than a default. */
	for (index = 0; index < records / 4; index++)
		ASSERT_EQ(0, event_beep(self->fd, 4));

	for (index = 0; index < records; index++) {
		ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, index + 1);
	}
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
	close(slow);
}

/* A read that could not take the record leaves it where it was, whole. */
TEST_F(events, a_read_that_could_not_take_the_record)
{
	long page = sysconf(_SC_PAGESIZE);
	char *unwritable;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 1));

	EXPECT_EQ(-EMSGSIZE, event_read_room(self->fd, &self->ev, 0));
	EXPECT_EQ(-EMSGSIZE,
		  event_read_room(self->fd, &self->ev, BEEPED_LEN - 1));

	unwritable = mmap(NULL, page, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS,
			  -1, 0);
	ASSERT_NE(MAP_FAILED, unwritable);
	EXPECT_EQ(-1, read(self->fd, unwritable, BEEPED_LEN));
	EXPECT_EQ(EFAULT, errno);
	ASSERT_EQ(0, munmap(unwritable, page));

	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
}

/* The two records a description may not lose are charged from the start. */
TEST_F(events, what_a_queue_reports)
{
	struct event_queue queue;

	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	EXPECT_EQ(EVENT_RESERVED, queue.used);
	EXPECT_GE(queue.len, EVENT_QUEUE_MIN);

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 2));
	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	EXPECT_EQ(EVENT_RESERVED + 2 * BEEPED_LEN, queue.used);

	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	EXPECT_EQ(EVENT_RESERVED + BEEPED_LEN, queue.used);
}

/* What a description may ask its queue to be, and what it may not. */
TEST_F(events, limits_a_queue_cannot_have)
{
	struct event_queue queue;
	char expected[CTLV_LOG_LEN];

	EXPECT_EQ(-EINVAL, event_queue_set(self->fd, 0));
	/* One byte short of room for the largest event and the two records. */
	EXPECT_EQ(-EINVAL,
		  event_queue_set_reply(self->fd, EVENT_QUEUE_MIN - 1,
					self->reply,
					sizeof(self->reply)));
	snprintf(expected, sizeof(expected),
		 "message=event queue length %llu is outside %llu..%u",
		 (unsigned long long)EVENT_QUEUE_MIN - 1,
		 (unsigned long long)EVENT_QUEUE_MIN,
		 CTLV_MAX_EVENT_QUEUE_LEN);
	EXPECT_REPLY(expected);
	EXPECT_EQ(-EINVAL,
		  event_queue_set(self->fd, CTLV_MAX_EVENT_QUEUE_LEN + 1));

	ASSERT_EQ(0, event_queue_set(self->fd, EVENT_QUEUE_MIN));
	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	EXPECT_EQ(EVENT_QUEUE_MIN, queue.len);
	ASSERT_EQ(0, event_queue_set(self->fd, CTLV_MAX_EVENT_QUEUE_LEN));
	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	EXPECT_EQ(CTLV_MAX_EVENT_QUEUE_LEN, queue.len);
}

/* And nothing queued is thrown away to make a queue smaller. */
TEST_F(events, a_queue_shrunk_below_what_it_holds)
{
	const __u64 record = BEEPED_FILLED_LEN(PING_FILLER_LEN);
	const __u64 room = 2 * record + EVENT_RESERVED;
	const struct burst burst = { .count = 2, .filler = PING_FILLER_LEN };
	struct event_queue queue;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_queue_set(self->fd, room));
	ASSERT_EQ(0, event_burst(self->fd, &burst));
	ASSERT_EQ(0, event_queue_get(self->fd, &queue));
	ASSERT_EQ(room, queue.used);

	EXPECT_EQ(-EBUSY, event_queue_set(self->fd, room - 1));
	/* One record read, and the same limit is one the queue fits in. */
	ASSERT_EQ(record, event_read(self->fd, &self->ev));
	EXPECT_EQ(0, event_queue_set(self->fd, room - 1));
}

/* Three records of room and a burst of five, so the marker says which two. */
TEST_F(events, what_a_queue_had_no_room_for)
{
	const __u64 record = BEEPED_FILLED_LEN(PING_FILLER_LEN);
	const struct burst burst = { .count = 5, .filler = PING_FILLER_LEN };
	unsigned int index;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_queue_set(self->fd, 3 * record + EVENT_RESERVED));
	ASSERT_EQ(0, event_burst(self->fd, &burst));

	for (index = 0; index < 3; index++) {
		ASSERT_EQ(record, event_read(self->fd, &self->ev));
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, index + 1);
	}
	/* Which arrives without having been subscribed to. */
	ASSERT_EQ(EVENT_OVERFLOW_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_OP_EVENT_OVERFLOW, 4);
	EXPECT_EQ(4, event_u64(&self->ev, CTLV_ATTR_EVENT_FIRST_SEQUENCE));
	EXPECT_EQ(5, event_u64(&self->ev, CTLV_ATTR_EVENT_LAST_SEQUENCE));
	EXPECT_EQ(2, event_u64(&self->ev, CTLV_ATTR_EVENT_DROPPED_COUNT));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* The next epoch opens once the marker has been read. */
TEST_F(events, an_epoch_after_the_one_before_it)
{
	const __u64 record = BEEPED_FILLED_LEN(PING_FILLER_LEN);
	struct burst burst = { .count = 4, .filler = PING_FILLER_LEN };
	unsigned int index;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_queue_set(self->fd, 2 * record + EVENT_RESERVED));

	ASSERT_EQ(0, event_burst(self->fd, &burst));
	for (index = 0; index < 2; index++) {
		ASSERT_EQ(record, event_read(self->fd, &self->ev));
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, index + 1);
	}
	ASSERT_EQ(EVENT_OVERFLOW_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_OP_EVENT_OVERFLOW, 3);
	EXPECT_EQ(2, event_u64(&self->ev, CTLV_ATTR_EVENT_DROPPED_COUNT));

	/* An epoch of its own, counting only what the second burst lost. */
	burst.count = 3;
	ASSERT_EQ(0, event_burst(self->fd, &burst));
	for (index = 0; index < 2; index++) {
		ASSERT_EQ(record, event_read(self->fd, &self->ev));
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, index + 5);
	}
	ASSERT_EQ(EVENT_OVERFLOW_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_OP_EVENT_OVERFLOW, 7);
	EXPECT_EQ(7, event_u64(&self->ev, CTLV_ATTR_EVENT_FIRST_SEQUENCE));
	EXPECT_EQ(7, event_u64(&self->ev, CTLV_ATTR_EVENT_LAST_SEQUENCE));
	EXPECT_EQ(1, event_u64(&self->ev, CTLV_ATTR_EVENT_DROPPED_COUNT));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* What several emitters do to one queue too small for what they emit. */
struct emitter {
	pthread_t thread;
	int fd;
	__u64 count;
	int ret;
};

static void *emitter_burst(void *arg)
{
	struct emitter *emitter = arg;

	emitter->ret = event_beep(emitter->fd, emitter->count);
	return NULL;
}

/* Once a drop opens an epoch, every number after the first loss is lost. */
TEST_F(events, an_epoch_under_several_emitters)
{
	struct emitter emitters[4] = {};
	__u64 queued = 0, total = 0;
	unsigned int index;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_queue_set(self->fd, EVENT_QUEUE_MIN));

	for (index = 0; index < ARRAY_SIZE(emitters); index++) {
		emitters[index].fd = self->fd;
		emitters[index].count = 16;
		total += emitters[index].count;
		ASSERT_EQ(0, pthread_create(&emitters[index].thread, NULL,
					    emitter_burst, &emitters[index]));
	}
	for (index = 0; index < ARRAY_SIZE(emitters); index++) {
		ASSERT_EQ(0, pthread_join(emitters[index].thread, NULL));
		ASSERT_EQ(0, emitters[index].ret);
	}

	while (event_read(self->fd, &self->ev) == BEEPED_LEN)
		EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, ++queued);
	/* So the read that ended that loop is the marker for the rest. */
	ASSERT_EQ(EVENT_OVERFLOW_LEN, self->ev.len);
	EXPECT_RECORD(&self->ev, CTLV_OP_EVENT_OVERFLOW, queued + 1);
	EXPECT_EQ(queued + 1,
		  event_u64(&self->ev, CTLV_ATTR_EVENT_FIRST_SEQUENCE));
	EXPECT_EQ(total, event_u64(&self->ev, CTLV_ATTR_EVENT_LAST_SEQUENCE));
	EXPECT_EQ(total - queued,
		  event_u64(&self->ev, CTLV_ATTR_EVENT_DROPPED_COUNT));
	EXPECT_LT(queued, total);
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* One record per real transition, and none for anything else. */
TEST_F(events, a_change_of_availability)
{
	__u64 generation;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_OP_OP_CHANGED));

	ASSERT_EQ(0, dynamic_toggle(self->fd));
	ASSERT_EQ(OP_CHANGED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_OP_OP_CHANGED, 1);
	EXPECT_EQ(CTLV_TEST_OP_DYNAMIC_NOOP,
		  event_u64(&self->ev, CTLV_ATTR_OP_ID));
	EXPECT_EQ(!variant->dynamic_disabled,
		  event_u64(&self->ev, CTLV_ATTR_OP_DISABLED));
	generation = event_u64(&self->ev, CTLV_ATTR_OP_STATE_GENERATION);
	EXPECT_LT(0, generation);

	/* And back, which is another transition and the generation after it. */
	ASSERT_EQ(0, dynamic_toggle(self->fd));
	ASSERT_EQ(OP_CHANGED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_OP_OP_CHANGED, 2);
	EXPECT_EQ(variant->dynamic_disabled,
		  event_u64(&self->ev, CTLV_ATTR_OP_DISABLED));
	EXPECT_EQ(generation + 1,
		  event_u64(&self->ev, CTLV_ATTR_OP_STATE_GENERATION));
	EXPECT_EQ(-EAGAIN, event_read(self->fd, &self->ev));
}

/* A queue belongs to the open file description, so a dup shares all of it. */
TEST_F(events, a_duplicated_descriptor)
{
	int copy;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	copy = dup(self->fd);
	ASSERT_LE(0, copy);
	ASSERT_EQ(0, event_beep(copy, 2));

	ASSERT_EQ(BEEPED_LEN, event_read(copy, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 2);
	EXPECT_EQ(-EAGAIN, event_read(copy, &self->ev));
	close(copy);
}

TEST_F(events, observation_authority_survives_fd_passing)
{
	int observation, socks[2], status;
	pid_t child;

	observation = ctlv_open(variant->family, 0, O_RDONLY);
	ASSERT_LE(0, observation);
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, socks));

	child = fork();
	ASSERT_LE(0, child);
	if (!child) {
		int passed = fd_recv(socks[1]);

		exit(passed < 0 ||
		     ctlv_exec_bare(passed, CTLV_TEST_OP_NOOP) ||
		     ctlv_exec_bare(passed, CTLV_TEST_OP_ECHO) != -EACCES);
	}
	ASSERT_EQ(0, fd_send(socks[0], observation));
	ASSERT_EQ(child, waitpid(child, &status, 0));
	ASSERT_TRUE(WIFEXITED(status));
	EXPECT_EQ(0, WEXITSTATUS(status));

	close(observation);
	close(socks[0]);
	close(socks[1]);
}

/* And so does a descriptor another process was handed: a queue is nobody's. */
TEST_F(events, a_descriptor_passed_to_another_process)
{
	int socks[2], status;
	pid_t child;

	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 2));
	ASSERT_EQ(0, socketpair(AF_UNIX, SOCK_STREAM, 0, socks));

	child = fork();
	ASSERT_LE(0, child);
	if (!child) {
		/* The first record of a queue this process never opened. */
		struct event ev;
		int passed = fd_recv(socks[1]);

		exit(passed < 0 || event_read(passed, &ev) != BEEPED_LEN ||
		     event_hdr(&ev)->sequence != 1);
	}
	ASSERT_EQ(0, fd_send(socks[0], self->fd));
	ASSERT_EQ(child, waitpid(child, &status, 0));
	ASSERT_TRUE(WIFEXITED(status));
	EXPECT_EQ(0, WEXITSTATUS(status));

	/* One queue, so what the child took is gone and the rest is here. */
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &self->ev));
	EXPECT_RECORD(&self->ev, CTLV_TEST_OP_BEEPED, 2);
	close(socks[0]);
	close(socks[1]);
}

TEST_F(events, a_reader_waiting_for_a_record)
{
	int waiting = ctlv_open(variant->family, 0, O_RDWR);
	struct emitter emitter = { .count = 1 };
	struct pollfd pfd;
	struct event ev;

	ASSERT_LE(0, waiting);
	ASSERT_EQ(0, event_subscribe(waiting, CTLV_TEST_OP_BEEPED));
	pfd = (struct pollfd){ .fd = waiting, .events = POLLIN };
	ASSERT_EQ(0, poll(&pfd, 1, 0));

	/* Emitted by another description while this one is already blocked. */
	emitter.fd = self->fd;
	ASSERT_EQ(0, pthread_create(&emitter.thread, NULL, emitter_burst,
				    &emitter));
	ASSERT_EQ(BEEPED_LEN, event_read(waiting, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(0, pthread_join(emitter.thread, NULL));
	ASSERT_EQ(0, emitter.ret);

	ASSERT_EQ(0, poll(&pfd, 1, 0));
	ASSERT_EQ(0, event_beep(self->fd, 1));
	ASSERT_EQ(1, poll(&pfd, 1, 0));
	EXPECT_TRUE(pfd.revents & POLLIN);
	/* Whose device is still there, so this is not the end of anything. */
	EXPECT_FALSE(pfd.revents & POLLHUP);
	close(waiting);
}

/* transfer folds what it reads into a checksum and fills what it writes. */

/* FNV-1a, the fold the test op does, so a checksum can be predicted here. */
#define BLOB_FNV_OFFSET 0xcbf29ce484222325ULL
#define BLOB_FNV_PRIME 0x100000001b3ULL

static __u64 blob_fold(__u64 sum, const void *data, size_t len)
{
	const __u8 *bytes = data;
	size_t i;

	for (i = 0; i < len; i++)
		sum = (sum ^ bytes[i]) * BLOB_FNV_PRIME;
	return sum;
}

static __u64 blob_sum(const void *data, size_t len)
{
	return blob_fold(BLOB_FNV_OFFSET, data, len);
}

/* Bytes that say where they are, so memory read at a wrong offset shows it. */
static void blob_pattern(void *buf, size_t len, unsigned int seed)
{
	__u8 *bytes = buf;
	size_t i;

	for (i = 0; i < len; i++)
		bytes[i] = (__u8)(i * 7 + seed);
}

/* Whether len bytes of buf are all one value, which is what a fill leaves. */
static bool blob_filled(const void *buf, size_t len, __u8 value)
{
	const __u8 *bytes = buf;
	size_t i;

	for (i = 0; i < len; i++)
		if (bytes[i] != value)
			return false;
	return true;
}

static int blob_memfd_new_seals(size_t len, unsigned int seals)
{
	int fd = memfd_create("ctlv-blob", MFD_CLOEXEC | MFD_ALLOW_SEALING);

	if (fd < 0)
		return -errno;
	if (ftruncate(fd, len) || fcntl(fd, F_ADD_SEALS, seals)) {
		close(fd);
		return -errno;
	}
	return fd;
}

/* Stable and writable, so the same helper serves every blob direction. */
static int blob_memfd_new(size_t len)
{
	return blob_memfd_new_seals(len, F_SEAL_SHRINK | F_SEAL_SEAL);
}

static int blob_memfd_reopen(int fd, int flags)
{
	char path[64];
	int reopened;

	snprintf(path, sizeof(path), "/proc/self/fd/%d", fd);
	reopened = open(path, flags | O_CLOEXEC);
	return reopened < 0 ? -errno : reopened;
}

static size_t blob_huge_page_size(void)
{
	unsigned long size_kb = 0;
	char line[256];
	FILE *meminfo;

	meminfo = fopen("/proc/meminfo", "re");
	if (!meminfo)
		return 0;
	while (fgets(line, sizeof(line), meminfo))
		if (sscanf(line, "Hugepagesize: %lu kB", &size_kb) == 1)
			break;
	fclose(meminfo);
	return size_kb * 1024;
}

/* A shared buffer from the system heap, where the kernel has one. */
static int blob_heap_new(size_t len)
{
	struct dma_heap_allocation_data alloc = {
		.len = len,
		.fd_flags = O_RDWR | O_CLOEXEC,
	};
	int fd, ret;

	fd = open("/dev/dma_heap/system", O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -ENODEV;
	ret = ioctl(fd, DMA_HEAP_IOCTL_ALLOC, &alloc) ? -errno :
						       (int)alloc.fd;
	close(fd);
	return ret;
}

/* And one from udmabuf, which exports a sealed memory file's pages. */
static int blob_udmabuf_new(size_t len)
{
	struct udmabuf_create create = {
		.flags = UDMABUF_FLAGS_CLOEXEC,
		.size = len,
	};
	int fd, memfd, ret;

	fd = open("/dev/udmabuf", O_RDWR | O_CLOEXEC);
	if (fd < 0)
		return -ENODEV;
	memfd = blob_memfd_new(len);
	if (memfd < 0) {
		close(fd);
		return memfd;
	}
	create.memfd = memfd;
	ret = ioctl(fd, UDMABUF_CREATE, &create);
	ret = ret < 0 ? -errno : ret;
	close(memfd);
	close(fd);
	return ret;
}

/* One kind of memory a descriptor can name; a shared buffer has two. */
struct blob_kind {
	const char *name;
	__u8 type;
	int (*create)(size_t len);
};

static const struct blob_kind blob_kind_va = {
	.name = "an address range",
	.type = CTLV_BLOB_TYPE_VA,
};

static const struct blob_kind blob_kind_memfd = {
	.name = "a memory file",
	.type = CTLV_BLOB_TYPE_MEMFD,
	.create = blob_memfd_new,
};

static const struct blob_kind blob_kind_heap = {
	.name = "a heap buffer",
	.type = CTLV_BLOB_TYPE_DMA_BUF,
	.create = blob_heap_new,
};

static const struct blob_kind blob_kind_udmabuf = {
	.name = "a udmabuf",
	.type = CTLV_BLOB_TYPE_DMA_BUF,
	.create = blob_udmabuf_new,
};

/* What the cases that are not about one backing work through in turn. */
static const struct blob_kind *const blob_kinds[] = {
	&blob_kind_va,
	&blob_kind_memfd,
	&blob_kind_heap,
	&blob_kind_udmabuf,
};

/* One thing a descriptor can name, and the mapping this side sees it by. */
struct blob_mem {
	const struct blob_kind *kind;
	/* The fd a descriptor names, or -1 where it names an address. */
	int fd;
	void *map;
	size_t len;
};

static void blob_mem_free(struct blob_mem *mem)
{
	if (mem->map)
		munmap(mem->map, mem->len);
	if (mem->fd >= 0)
		close(mem->fd);
	*mem = (struct blob_mem){ .fd = -1 };
}

/* Mapped so this side sees it too; -ENODEV where the kernel exports none. */
static int blob_mem_new(struct blob_mem *mem, const struct blob_kind *kind,
			size_t len)
{
	*mem = (struct blob_mem){ .kind = kind, .fd = -1, .len = len };

	if (kind->create) {
		mem->fd = kind->create(len);
		if (mem->fd < 0)
			return mem->fd;
	}
	mem->map = mmap(NULL, len, PROT_READ | PROT_WRITE,
			mem->fd < 0 ? MAP_PRIVATE | MAP_ANONYMOUS : MAP_SHARED,
			mem->fd, 0);
	if (mem->map == MAP_FAILED) {
		mem->map = NULL;
		blob_mem_free(mem);
		return -errno;
	}
	return 0;
}

/* The descriptor of a range of one of them, whole where len is zero. */
static void blob_desc(struct ctlv_tlv *tlv, __u64 attr_id,
		      const struct blob_mem *mem, __u64 offset, __u64 len)
{
	if (!len)
		len = mem->len - offset;
	if (mem->kind->type == CTLV_BLOB_TYPE_VA)
		ctlv_tlv_blob_va(tlv, attr_id,
				 ctlv_ptr((char *)mem->map + offset), len);
	else
		ctlv_tlv_blob_fd(tlv, attr_id, mem->kind->type, mem->fd,
				 offset, len);
}

/* What one transfer asks for, and what it answered. */
struct transfer {
	const struct blob_mem *source;
	const struct blob_mem *target;
	const struct blob_mem *scratch;
	/* Occurrences of the repeated attr, resolved in the order given. */
	const struct blob_mem *pieces;
	unsigned int pieces_count;
	/* The range of each named blob, the whole of it where len is zero. */
	__u64 offset;
	__u64 len;
	__u8 fill;
	bool skip;
	bool twice;
	bool hold;
	/* And of the reply: what was read, what was written, what was said. */
	__u64 sum;
	__u64 written;
	bool distinct;
	bool answered;
};

/* Into room for any reply; an action that answers is refused without. */
static int transfer_send(int fd, struct ctlv_tlv *tlv, void *reply,
			 __u32 reply_len)
{
	return ctlv_exec_full(fd, CTLV_TEST_OP_TRANSFER, tlv->buf,
			      ctlv_tlv_done(tlv), reply, reply_len);
}

static int transfer_run(int fd, struct transfer *t)
{
	__u64 reply[16] = {};
	const struct ctlv_attr *root = (const struct ctlv_attr *)reply;
	const struct ctlv_attr *child;
	struct ctlv_tlv tlv;
	unsigned int index;
	int ret;

	ctlv_tlv_init(&tlv);
	if (t->source)
		blob_desc(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, t->source,
			  t->offset, t->len);
	if (t->target)
		blob_desc(&tlv, CTLV_TEST_ATTR_TRANSFER_TARGET, t->target,
			  t->offset, t->len);
	if (t->scratch)
		blob_desc(&tlv, CTLV_TEST_ATTR_TRANSFER_SCRATCH, t->scratch,
			  t->offset, t->len);
	for (index = 0; index < t->pieces_count; index++)
		blob_desc(&tlv, CTLV_TEST_ATTR_TRANSFER_PIECE,
			  &t->pieces[index], 0, 0);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, t->fill);
	if (t->skip)
		ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_TRANSFER_SKIP, true);
	if (t->twice)
		ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_TRANSFER_TWICE, true);
	if (t->hold)
		ctlv_tlv_bool(&tlv, CTLV_TEST_ATTR_TRANSFER_HOLD, true);

	t->sum = 0;
	t->written = 0;
	t->distinct = false;
	t->answered = false;
	ret = transfer_send(fd, &tlv, reply, sizeof(reply));
	if (ret)
		return ret;
	ctlv_rd_for_each(child, root) {
		if (child->attr_id == CTLV_TEST_ATTR_TRANSFER_SUM) {
			t->sum = ctlv_rd_u64(child);
		} else if (child->attr_id ==
			   CTLV_TEST_ATTR_TRANSFER_WRITTEN) {
			t->written = ctlv_rd_u64(child);
		} else if (child->attr_id ==
			   CTLV_TEST_ATTR_TRANSFER_DISTINCT) {
			t->distinct = ctlv_rd_u64(child);
			t->answered = true;
		}
	}
	return 0;
}

/* What this process has pinned, in pages, or -1 where nothing says. */
static long blob_pinned_pages(void)
{
	unsigned long kb;
	char line[256];
	long pages = -1;
	FILE *status;

	status = fopen("/proc/self/status", "re");
	if (!status)
		return -1;
	while (fgets(line, sizeof(line), status))
		if (sscanf(line, "VmPin: %lu kB", &kb) == 1) {
			pages = kb * 1024 / getpagesize();
			break;
		}
	fclose(status);
	return pages;
}

/* Until a transfer is holding what it resolved, or until nothing will. */
static int blob_wait_held(void)
{
	unsigned int tries;

	for (tries = 0; tries < 5000; tries++) {
		if (ctlv_hold_count())
			return 0;
		usleep(1000);
	}
	return -ETIMEDOUT;
}

/* One transfer sent from a thread of its own, so this one can watch it. */
struct holder {
	pthread_t thread;
	int fd;
	struct transfer transfer;
	int ret;
};

static void *holder_transfer(void *arg)
{
	struct holder *holder = arg;

	holder->ret = transfer_run(holder->fd, &holder->transfer);
	return NULL;
}

/* Enough of it to span several pages, and to be worth an offset inside. */
#define BLOB_LEN (64 * 1024)
#define BLOB_PIECES 3
#define BLOB_REPLY_LEN 256

static int blob_memfd_transfer(int ctlv_fd, void *reply, __u64 attr_id,
			       int memfd)
{
	struct ctlv_tlv tlv;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, attr_id, memfd, 0, BLOB_LEN);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, 0x5a);
	return transfer_send(ctlv_fd, &tlv, reply, BLOB_REPLY_LEN);
}

FIXTURE(blob) {
	int fd;
	/* Room for any reply the transfer op or a failure of it produces. */
	char reply[BLOB_REPLY_LEN];
	struct blob_mem source;
	struct blob_mem target;
	struct blob_mem pieces[BLOB_PIECES];
	/* What the source was filled with, which is what a sum is over. */
	__u8 *expected;
};

FIXTURE_VARIANT(blob) {
	const char *family;
};

FIXTURE_VARIANT_ADD(blob, parent) {
	.family = "ctlv-test",
};

/* The op is inherited, so the child resolves what the parent declared. */
FIXTURE_VARIANT_ADD(blob, child) {
	.family = "ctlv-test-child",
};

FIXTURE_SETUP(blob)
{
	unsigned int index;

	self->fd = ctlv_open(variant->family, 0, O_RDWR);
	if (self->fd < 0)
		SKIP(return, "no %s device: %s", variant->family,
		     strerror(-self->fd));

	ASSERT_EQ(0, blob_mem_new(&self->source, &blob_kind_va, BLOB_LEN));
	ASSERT_EQ(0, blob_mem_new(&self->target, &blob_kind_va, BLOB_LEN));
	blob_pattern(self->source.map, BLOB_LEN, 1);
	self->expected = malloc(BLOB_LEN);
	ASSERT_NE(NULL, self->expected);
	memcpy(self->expected, self->source.map, BLOB_LEN);

	/* Each piece its own pattern, so their order shows in one checksum. */
	for (index = 0; index < BLOB_PIECES; index++) {
		ASSERT_EQ(0, blob_mem_new(&self->pieces[index],
					  &blob_kind_va, BLOB_LEN));
		blob_pattern(self->pieces[index].map, BLOB_LEN, index + 2);
	}
}

FIXTURE_TEARDOWN(blob)
{
	unsigned int index;

	/* Nothing may be left holding a blob for whatever runs next. */
	ctlv_hold_release();
	free(self->expected);
	for (index = 0; index < BLOB_PIECES; index++)
		blob_mem_free(&self->pieces[index]);
	blob_mem_free(&self->target);
	blob_mem_free(&self->source);
	if (self->fd >= 0)
		close(self->fd);
}

/* A blob is the same memory on both sides rather than a copy of it. */
TEST_F(blob, memory_is_read_and_memory_is_written)
{
	struct transfer transfer = {
		.source = &self->source,
		.target = &self->target,
		.fill = 0xa5,
	};

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(blob_sum(self->expected, BLOB_LEN), transfer.sum);
	EXPECT_EQ(BLOB_LEN, transfer.written);
	EXPECT_TRUE(blob_filled(self->target.map, BLOB_LEN, 0xa5));
	/* And the blob it read is the caller's to keep as it was. */
	EXPECT_EQ(0, memcmp(self->source.map, self->expected, BLOB_LEN));
	/* Nothing was resolved twice, so there is nothing to say about it. */
	EXPECT_FALSE(transfer.answered);
}

/* Which backing a descriptor names changes nothing about what a blob is. */
TEST_F(blob, every_backing_is_the_same_memory)
{
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE(blob_kinds); index++) {
		const struct blob_kind *kind = blob_kinds[index];
		struct blob_mem source, target;
		struct transfer transfer;
		int ret;

		ret = blob_mem_new(&source, kind, BLOB_LEN);
		if (ret == -ENODEV) {
			TH_LOG("%s: this kernel exports none", kind->name);
			continue;
		}
		ASSERT_EQ(0, ret);
		ASSERT_EQ(0, blob_mem_new(&target, kind, BLOB_LEN));
		blob_pattern(source.map, BLOB_LEN, 1);

		transfer = (struct transfer){
			.source = &source,
			.target = &target,
			.fill = 0x5a,
		};
		EXPECT_EQ(0, transfer_run(self->fd, &transfer));
		EXPECT_EQ(blob_sum(source.map, BLOB_LEN), transfer.sum);
		EXPECT_EQ(BLOB_LEN, transfer.written);
		EXPECT_TRUE(blob_filled(target.map, BLOB_LEN, 0x5a));
		blob_mem_free(&target);
		blob_mem_free(&source);
	}
}

/* The range its descriptor named, and nothing on either side of it. */
TEST_F(blob, a_range_inside_what_it_names)
{
	struct transfer transfer = {
		.source = &self->source,
		.target = &self->target,
		.offset = 4096,
		.len = 8192,
		.fill = 0x11,
	};

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(blob_sum(self->expected + 4096, 8192), transfer.sum);
	EXPECT_EQ(8192, transfer.written);
	EXPECT_TRUE(blob_filled((char *)self->target.map + 4096, 8192, 0x11));
	/* Which leaves the bytes outside the range as this test left them. */
	EXPECT_TRUE(blob_filled(self->target.map, 4096, 0));
	EXPECT_TRUE(blob_filled((char *)self->target.map + 12288,
				BLOB_LEN - 12288, 0));
}

/* Where a page count that came out one short would show as a short copy. */
TEST_F(blob, a_range_that_crosses_pages)
{
	long page = sysconf(_SC_PAGESIZE);
	struct transfer transfer = {
		.source = &self->source,
		.target = &self->target,
		.offset = page - 8,
		.len = 16,
		.fill = 0x22,
	};
	char *target = self->target.map;

	ASSERT_LT(page + 8, BLOB_LEN);
	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(blob_sum(self->expected + page - 8, 16), transfer.sum);
	EXPECT_EQ(16, transfer.written);
	EXPECT_TRUE(blob_filled(target + page - 8, 16, 0x22));
	/* Both pages of it were reached, and neither of them beyond it. */
	EXPECT_TRUE(blob_filled(target, page - 8, 0));
	EXPECT_TRUE(blob_filled(target + page + 8, BLOB_LEN - page - 8, 0));
}

/* An address narrowed to fit would name memory this process does have. */
TEST_F(blob, an_address_no_process_could_have)
{
	static const struct {
		const char *name;
		__u64 bit;
	} highs[] = {
		{ "the second-highest bit of an address", 1ULL << 62 },
		{ "the highest bit of an address", 1ULL << 63 },
	};
	__u64 real = ctlv_ptr(self->source.map);
	char expected[CTLV_LOG_LEN];
	struct ctlv_tlv tlv;
	unsigned int index;
	__u32 offset;

	for (index = 0; index < ARRAY_SIZE(highs); index++) {
		ctlv_tlv_init(&tlv);
		offset = ctlv_tlv_blob_va(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
					  real | highs[index].bit, BLOB_LEN);
		EXPECT_EQ(-EFAULT, transfer_send(self->fd, &tlv, self->reply,
						 sizeof(self->reply)))
			TH_LOG("%s", highs[index].name);
		/* Which of the request's blobs it was, as a refusal says. */
		snprintf(expected, sizeof(expected),
			 "reason=%u bad-attr-offset=%u nest-offset=0",
			 CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR, offset);
		EXPECT_REPLY(expected);
	}

	/* The highest range at all: one byte more and validation refuses it. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_va(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
			 (__u64)-(BLOB_LEN + 8), BLOB_LEN);
	EXPECT_EQ(-EFAULT, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
}

/* A blob declared both ways is read as it was and then written over. */
TEST_F(blob, memory_read_and_then_written)
{
	struct transfer transfer = {
		.scratch = &self->source,
		.fill = 0x77,
	};

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(blob_sum(self->expected, BLOB_LEN), transfer.sum);
	EXPECT_EQ(BLOB_LEN, transfer.written);
	EXPECT_TRUE(blob_filled(self->source.map, BLOB_LEN, 0x77));
}

TEST_F(blob, every_occurrence_in_the_order_it_arrived)
{
	struct transfer transfer = {
		.pieces = self->pieces,
		.pieces_count = BLOB_PIECES,
	};
	__u64 expected = BLOB_FNV_OFFSET;
	unsigned int index;

	for (index = 0; index < BLOB_PIECES; index++)
		expected = blob_fold(expected, self->pieces[index].map,
				     BLOB_LEN);

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(expected, transfer.sum);
	EXPECT_EQ(0, transfer.written);
}

/* The second call hands back an object of its own, released separately. */
TEST_F(blob, one_descriptor_resolved_twice)
{
	struct transfer transfer = {
		.source = &self->source,
		.twice = true,
	};

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_TRUE(transfer.answered);
	EXPECT_TRUE(transfer.distinct);
	EXPECT_EQ(blob_sum(self->expected, BLOB_LEN), transfer.sum);
}

/* Nothing acquired, so nothing read and nothing written. */
TEST_F(blob, a_descriptor_nothing_resolved)
{
	struct transfer transfer = {
		.source = &self->source,
		.target = &self->target,
		.fill = 0xff,
		.skip = true,
	};

	ASSERT_EQ(0, transfer_run(self->fd, &transfer));
	EXPECT_EQ(0, transfer.sum);
	EXPECT_EQ(0, transfer.written);
	EXPECT_TRUE(blob_filled(self->target.map, BLOB_LEN, 0));
}

/* Refused at resolve rather than at validation, with the backing's errno. */
TEST_F(blob, a_descriptor_of_memory_the_caller_does_not_have)
{
	struct blob_mem unsealed = { .fd = -1 };
	char expected[CTLV_LOG_LEN];
	struct ctlv_tlv tlv;
	void *unmapped;
	__u32 offset;
	int sink;

	/* An address range this process has just given back. */
	unmapped = self->target.map;
	blob_mem_free(&self->target);
	ctlv_tlv_init(&tlv);
	offset = ctlv_tlv_blob_va(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
				  ctlv_ptr(unmapped), BLOB_LEN);
	EXPECT_EQ(-EFAULT, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
	/* Which of the request's blobs it was, where validation says it. */
	snprintf(expected, sizeof(expected),
		 "reason=%u bad-attr-offset=%u nest-offset=0",
		 CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR, offset);
	EXPECT_REPLY(expected);

	/* A number that is not an open descriptor at all. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
			    STDERR_FILENO + 1000, 0, BLOB_LEN);
	EXPECT_EQ(-EBADF, transfer_send(self->fd, &tlv, self->reply,
					sizeof(self->reply)));

	/* One that is open and is not a memory file. */
	sink = open("/dev/null", O_RDWR | O_CLOEXEC);
	ASSERT_LE(0, sink);
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, sink, 0,
			    BLOB_LEN);
	EXPECT_EQ(-EINVAL, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
	/* And one that is not a shared buffer either. */
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_dma_buf(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, sink, 0,
			      BLOB_LEN);
	EXPECT_EQ(-EINVAL, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
	close(sink);

	/* A memory file that could still be truncated under the kernel. */
	unsealed.fd = memfd_create("ctlv-blob", MFD_CLOEXEC);
	ASSERT_LE(0, unsealed.fd);
	ASSERT_EQ(0, ftruncate(unsealed.fd, BLOB_LEN));
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, unsealed.fd,
			    0, BLOB_LEN);
	EXPECT_EQ(-EINVAL, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
	blob_mem_free(&unsealed);

	/* And a sealed one that is shorter than the range described in it. */
	ASSERT_EQ(0, blob_mem_new(&self->target, &blob_kind_memfd, BLOB_LEN));
	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
			    self->target.fd, BLOB_LEN, BLOB_LEN);
	EXPECT_EQ(-EINVAL, transfer_send(self->fd, &tlv, self->reply,
					 sizeof(self->reply)));
}

TEST_F(blob, a_memfd_open_mode_matches_its_direction)
{
	int fd, read_only, write_only;

	fd = blob_memfd_new(BLOB_LEN);
	ASSERT_LE(0, fd);
	read_only = blob_memfd_reopen(fd, O_RDONLY);
	ASSERT_LE(0, read_only);
	write_only = blob_memfd_reopen(fd, O_WRONLY);
	ASSERT_LE(0, write_only);

	EXPECT_EQ(0, blob_memfd_transfer(self->fd, self->reply,
					 CTLV_TEST_ATTR_TRANSFER_SOURCE,
					 read_only));
	EXPECT_EQ(-EBADF,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_TARGET,
				      read_only));
	EXPECT_EQ(-EBADF,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_SCRATCH,
				      read_only));

	EXPECT_EQ(-EBADF,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_SOURCE,
				      write_only));
	EXPECT_EQ(0, blob_memfd_transfer(self->fd, self->reply,
					 CTLV_TEST_ATTR_TRANSFER_TARGET,
					 write_only));
	EXPECT_EQ(-EBADF,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_SCRATCH,
				      write_only));

	EXPECT_EQ(0, blob_memfd_transfer(self->fd, self->reply,
					 CTLV_TEST_ATTR_TRANSFER_SCRATCH, fd));
	close(write_only);
	close(read_only);
	close(fd);
}

TEST_F(blob, a_memfd_append_flag_does_not_move_the_range)
{
	struct stat status;
	__u8 first;
	int flags;
	int fd;

	fd = blob_memfd_new(BLOB_LEN);
	ASSERT_LE(0, fd);
	flags = fcntl(fd, F_GETFL);
	ASSERT_LE(0, flags);
	ASSERT_EQ(0, fcntl(fd, F_SETFL, flags | O_APPEND));

	ASSERT_EQ(0, blob_memfd_transfer(self->fd, self->reply,
					 CTLV_TEST_ATTR_TRANSFER_TARGET, fd));
	ASSERT_EQ(0, fstat(fd, &status));
	EXPECT_EQ(BLOB_LEN, status.st_size);
	ASSERT_EQ(1, pread(fd, &first, sizeof(first), 0));
	EXPECT_EQ(0x5a, first);
	close(fd);
}

TEST_F(blob, a_memfd_write_seal_is_input_only)
{
	static const unsigned int write_seals[] = {
		F_SEAL_WRITE,
		F_SEAL_FUTURE_WRITE,
	};
	unsigned int index;

	for (index = 0; index < ARRAY_SIZE(write_seals); index++) {
		int fd;

		fd = blob_memfd_new_seals(BLOB_LEN,
					  F_SEAL_SHRINK |
					  write_seals[index]);
		ASSERT_LE(0, fd);
		EXPECT_EQ(0,
			  blob_memfd_transfer(self->fd, self->reply,
					      CTLV_TEST_ATTR_TRANSFER_SOURCE,
					      fd));
		EXPECT_EQ(-EPERM,
			  blob_memfd_transfer(self->fd, self->reply,
					      CTLV_TEST_ATTR_TRANSFER_TARGET,
					      fd));
		EXPECT_EQ(-EPERM,
			  blob_memfd_transfer(self->fd, self->reply,
					      CTLV_TEST_ATTR_TRANSFER_SCRATCH,
					      fd));
		close(fd);
	}
}

TEST_F(blob, a_hugetlb_memfd_is_input_only)
{
	size_t huge_page_size = blob_huge_page_size();
	int saved_errno;
	int fd;

	if (!huge_page_size)
		SKIP(return, "default huge page size is unavailable");
	fd = memfd_create("ctlv-hugetlb",
			  MFD_CLOEXEC | MFD_ALLOW_SEALING | MFD_HUGETLB);
	if (fd < 0)
		SKIP(return, "hugetlb memfd: %s", strerror(errno));
	if (ftruncate(fd, huge_page_size) ||
	    fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK | F_SEAL_SEAL)) {
		saved_errno = errno;
		close(fd);
		SKIP(return, "preparing hugetlb memfd: %s",
		     strerror(saved_errno));
	}

	EXPECT_EQ(0, blob_memfd_transfer(self->fd, self->reply,
					 CTLV_TEST_ATTR_TRANSFER_SOURCE, fd));
	EXPECT_EQ(-EOPNOTSUPP,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_TARGET, fd));
	EXPECT_EQ(-EOPNOTSUPP,
		  blob_memfd_transfer(self->fd, self->reply,
				      CTLV_TEST_ATTR_TRANSFER_SCRATCH, fd));
	close(fd);
}

/* A pin exists while the op runs and is gone once the ioctl returned. */
TEST_F(blob, a_pin_lasts_as_long_as_the_callback)
{
	struct holder holder = { .fd = self->fd };
	long before, during;

	before = blob_pinned_pages();
	if (before < 0)
		SKIP(return, "the kernel does not report pinned pages");

	holder.transfer = (struct transfer){
		.source = &self->source,
		.hold = true,
	};
	ASSERT_EQ(0, pthread_create(&holder.thread, NULL, holder_transfer,
				    &holder));
	ASSERT_EQ(0, blob_wait_held());
	during = blob_pinned_pages();
	EXPECT_GE(during - before, BLOB_LEN / getpagesize());

	ASSERT_EQ(0, ctlv_hold_release());
	ASSERT_EQ(0, pthread_join(holder.thread, NULL));
	EXPECT_EQ(0, holder.ret);
	EXPECT_EQ(before, blob_pinned_pages());
}

/* And a descriptor nothing resolved never had one, however long the op runs. */
TEST_F(blob, an_unresolved_descriptor_pins_nothing)
{
	struct holder holder = { .fd = self->fd };
	long before;

	before = blob_pinned_pages();
	if (before < 0)
		SKIP(return, "the kernel does not report pinned pages");

	holder.transfer = (struct transfer){
		.source = &self->source,
		.skip = true,
		.hold = true,
	};
	ASSERT_EQ(0, pthread_create(&holder.thread, NULL, holder_transfer,
				    &holder));
	ASSERT_EQ(0, blob_wait_held());
	EXPECT_EQ(before, blob_pinned_pages());

	ASSERT_EQ(0, ctlv_hold_release());
	ASSERT_EQ(0, pthread_join(holder.thread, NULL));
	EXPECT_EQ(0, holder.ret);
}

/* The mapping unmapped and reused, the descriptor number taken by a sink. */
static void blob_take_back(struct blob_mem *mem, int sink)
{
	void *addr = mem->map;

	munmap(mem->map, mem->len);
	mem->map = NULL;
	if (mem->kind->type == CTLV_BLOB_TYPE_VA) {
		mem->map = mmap(addr, mem->len, PROT_READ | PROT_WRITE,
				MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1,
				0);
		if (mem->map == MAP_FAILED)
			mem->map = NULL;
		else
			memset(mem->map, 0xee, mem->len);
		return;
	}
	dup2(sink, mem->fd);
}

/* The bytes the callback reads afterwards are still the ones it acquired. */
TEST_F(blob, what_was_acquired_outlives_the_handles_it_came_from)
{
	unsigned int index;
	int sink;

	sink = open("/dev/null", O_RDWR | O_CLOEXEC);
	ASSERT_LE(0, sink);

	for (index = 0; index < ARRAY_SIZE(blob_kinds); index++) {
		const struct blob_kind *kind = blob_kinds[index];
		struct holder holder = { .fd = self->fd };
		struct blob_mem mem;
		__u64 expected;
		int ret;

		ret = blob_mem_new(&mem, kind, BLOB_LEN);
		if (ret == -ENODEV) {
			TH_LOG("%s: this kernel exports none", kind->name);
			continue;
		}
		ASSERT_EQ(0, ret);
		blob_pattern(mem.map, BLOB_LEN, index + 1);
		expected = blob_sum(mem.map, BLOB_LEN);

		holder.transfer = (struct transfer){
			.source = &mem,
			.hold = true,
		};
		ASSERT_EQ(0, pthread_create(&holder.thread, NULL,
					    holder_transfer, &holder));
		ASSERT_EQ(0, blob_wait_held());
		/* A sealed file cannot be shortened under the kernel. */
		if (kind == &blob_kind_memfd)
			EXPECT_EQ(-1, ftruncate(mem.fd, 0));
		blob_take_back(&mem, sink);

		ASSERT_EQ(0, ctlv_hold_release());
		ASSERT_EQ(0, pthread_join(holder.thread, NULL));
		EXPECT_EQ(0, holder.ret);
		EXPECT_EQ(expected, holder.transfer.sum);
		blob_mem_free(&mem);
	}
	close(sink);
}

/* More than a message may carry, which is the size a blob is for. */
#define BLOB_LARGE_LEN (100 * 1024 * 1024)

/* udmabuf caps its exports at 64 MiB, raised where this kernel has the knob. */
#define UDMABUF_SIZE_LIMIT "/sys/module/udmabuf/parameters/size_limit_mb"

static void blob_udmabuf_limit_raise(size_t len)
{
	unsigned long mb = (len + (1 << 20) - 1) >> 20;
	char value[32];

	if (ctlv_sysfs_read(UDMABUF_SIZE_LIMIT, value, sizeof(value)))
		return;
	if (strtoul(value, NULL, 10) >= mb)
		return;
	snprintf(value, sizeof(value), "%lu", mb);
	ctlv_sysfs_write(UDMABUF_SIZE_LIMIT, value);
}

/* Pinning that much needs a limit that allows it, which root may raise. */
static void blob_memlock_raise(size_t len)
{
	struct rlimit limit;

	if (getrlimit(RLIMIT_MEMLOCK, &limit))
		return;
	if (limit.rlim_cur != RLIM_INFINITY && limit.rlim_cur < len) {
		limit.rlim_cur = len;
		if (limit.rlim_max != RLIM_INFINITY &&
		    limit.rlim_max < limit.rlim_cur)
			limit.rlim_max = limit.rlim_cur;
		setrlimit(RLIMIT_MEMLOCK, &limit);
	}
}

TEST(a_hundred_mebibytes_of_every_backing)
{
	unsigned int index;
	int fd;

	fd = ctlv_open("ctlv-test", 0, O_RDWR);
	if (fd < 0)
		SKIP(return, "no ctlv-test device: %s", strerror(-fd));
	blob_memlock_raise(BLOB_LARGE_LEN);
	blob_udmabuf_limit_raise(BLOB_LARGE_LEN);

	for (index = 0; index < ARRAY_SIZE(blob_kinds); index++) {
		const struct blob_kind *kind = blob_kinds[index];
		struct blob_mem source, target;
		struct transfer transfer;
		int ret;

		ret = blob_mem_new(&source, kind, BLOB_LARGE_LEN);
		if (ret == -ENODEV || ret == -ENOMEM) {
			TH_LOG("%s: %s", kind->name, strerror(-ret));
			continue;
		}
		ASSERT_EQ(0, ret);
		ASSERT_EQ(0, blob_mem_new(&target, kind, BLOB_LARGE_LEN));
		blob_pattern(source.map, BLOB_LARGE_LEN, 3);

		transfer = (struct transfer){
			.source = &source,
			.target = &target,
			.fill = 0x42,
		};
		EXPECT_EQ(0, transfer_run(fd, &transfer));
		EXPECT_EQ(blob_sum(source.map, BLOB_LARGE_LEN), transfer.sum);
		EXPECT_EQ(BLOB_LARGE_LEN, transfer.written);
		EXPECT_TRUE(blob_filled(target.map, BLOB_LARGE_LEN, 0x42));
		blob_mem_free(&target);
		blob_mem_free(&source);
	}
	close(fd);
}

/* One instance is a parent device and a child, and flipping one is its own. */
TEST(availability_is_per_device)
{
	int parent, child;

	parent = ctlv_open("ctlv-test", 0, O_RDWR);
	if (parent < 0)
		SKIP(return, "no ctlv-test device: %s", strerror(-parent));
	child = ctlv_open("ctlv-test-child", 0, O_RDWR);
	ASSERT_LE(0, child);

	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(parent, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(0, ctlv_exec_bare(child, CTLV_TEST_OP_DYNAMIC_NOOP));

	ASSERT_EQ(0, dynamic_toggle(parent));
	EXPECT_EQ(0, ctlv_exec_bare(parent, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(0, ctlv_exec_bare(child, CTLV_TEST_OP_DYNAMIC_NOOP));

	ASSERT_EQ(0, dynamic_toggle(child));
	EXPECT_EQ(0, ctlv_exec_bare(parent, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(child, CTLV_TEST_OP_DYNAMIC_NOOP));

	/* Left as they were found, for whatever runs next. */
	ASSERT_EQ(0, dynamic_toggle(parent));
	ASSERT_EQ(0, dynamic_toggle(child));
	close(child);
	close(parent);
}

/* Listen first, enumerate second, so no device can slip past in between. */

/* One addition, as much of it as deciding what the device is takes. */
struct discovery_event {
	char devname[CTLV_NAME_LEN];
	char family[CTLV_NAME_LEN];
	char devpath[CTLV_PATH_LEN];
	/* The chain, rendered the way the device's own dump renders it. */
	struct reply_text chain;
	unsigned int families;
};

static unsigned int discovery_family_id(const char *name)
{
	if (!strcmp(name, "ctlv"))
		return CTLV_FAMILY_ROOT;
	if (!strcmp(name, "ctlv-test"))
		return CTLV_FAMILY_TEST;
	if (!strcmp(name, "ctlv-test-child"))
		return CTLV_FAMILY_TEST_CHILD;
	return UINT_MAX;
}

/* Ordered from the oldest family, and as long as the count says. */
static void discovery_chain(struct __test_metadata *_metadata,
			    struct discovery_event *event,
			    const struct ctlv_monitor *mon)
{
	char property[64], value[CTLV_NAME_LEN];
	unsigned int i;

	ASSERT_EQ(0, ctlv_monitor_read(mon, "CTLV_COMPATIBLE_N", value,
				       sizeof(value)));
	event->families = atoi(value);
	ASSERT_LT(0, event->families);
	for (i = 0; i < event->families; i++) {
		unsigned int family_id;

		snprintf(property, sizeof(property), "CTLV_COMPATIBLE_%u", i);
		ASSERT_EQ(0, ctlv_monitor_read(mon, property, value,
					       sizeof(value)));
		family_id = discovery_family_id(value);
		ASSERT_NE(UINT_MAX, family_id);
		reply_add(&event->chain, i ? " family={id=%u name=%s}" :
			  "family={id=%u name=%s}", family_id, value);
	}
	/* And nothing beyond what the count promised. */
	snprintf(property, sizeof(property), "CTLV_COMPATIBLE_%u",
		 event->families);
	EXPECT_EQ(-ENOENT, ctlv_monitor_read(mon, property, value,
					     sizeof(value)));
}

/* Everything one addition announces, gathered as a monitor gathers it. */
static void discovery_next(struct __test_metadata *_metadata,
			   struct discovery_event *event,
			   struct ctlv_monitor *mon)
{
	memset(event, 0, sizeof(*event));
	ASSERT_EQ(0, ctlv_monitor_wait(mon, "add"));
	ASSERT_EQ(0, ctlv_monitor_read(mon, "DEVNAME", event->devname,
				       sizeof(event->devname)));
	ASSERT_EQ(0, ctlv_monitor_read(mon, "CTLV_FAMILY", event->family,
				       sizeof(event->family)));
	ASSERT_EQ(0, ctlv_monitor_read(mon, "DEVPATH", event->devpath,
				       sizeof(event->devpath)));
	discovery_chain(_metadata, event, mon);
}

FIXTURE(discovery) {
	struct ctlv_monitor mon;
	bool added;
};

FIXTURE_SETUP(discovery)
{
	int ret;

	if (!ctlv_instances_available())
		SKIP(return, "no %s control", CTLV_TEST_DEVICE_ADD);
	ret = ctlv_monitor_open(&self->mon);
	if (ret)
		SKIP(return, "no uevent monitor: %s", strerror(-ret));
}

FIXTURE_TEARDOWN(discovery)
{
	if (self->added)
		ctlv_instances_set(1);
	ctlv_monitor_close(&self->mon);
}

/* Registered after the listener was bound and before the enumeration ran. */
TEST_F(discovery, an_addition_between_listening_and_looking)
{
	struct discovery_event parent, child;
	char name[CTLV_NAME_LEN];

	/* Nothing yet, so nothing an enumeration could have seen. */
	ASSERT_EQ(-ENODEV, ctlv_device_name("ctlv-test", 1, name,
					    sizeof(name)));
	ASSERT_EQ(0, ctlv_instances_set(2));
	self->added = true;

	/* An instance is a family and a descendant of it, registered in turn. */
	discovery_next(_metadata, &parent, &self->mon);
	EXPECT_STREQ("ctlv-test", parent.family);
	EXPECT_EQ(2, parent.families);
	EXPECT_STREQ(INTRO_CHAIN_PARENT, parent.chain.buf);

	discovery_next(_metadata, &child, &self->mon);
	EXPECT_STREQ("ctlv-test-child", child.family);
	EXPECT_EQ(3, child.families);
	EXPECT_STREQ(INTRO_CHAIN_CHILD, child.chain.buf);

	/* And the enumeration now finds what the events already described. */
	ASSERT_EQ(0, ctlv_device_name("ctlv-test", 1, name, sizeof(name)));
	EXPECT_STREQ(parent.devname, name);
	ASSERT_EQ(0, ctlv_device_name("ctlv-test-child", 1, name,
				      sizeof(name)));
	EXPECT_STREQ(child.devname, name);
}

/* What the event says the device is, the device says of itself. */
TEST_F(discovery, an_event_and_the_device_it_announces_agree)
{
	char path[CTLV_PATH_LEN];
	char reply[INTRO_REPLY_LEN];
	struct discovery_event event;
	struct intro intro;
	int fd;

	ASSERT_EQ(0, ctlv_instances_set(2));
	self->added = true;
	discovery_next(_metadata, &event, &self->mon);

	snprintf(path, sizeof(path), "/dev/%s", event.devname);
	fd = open(path, O_RDWR);
	ASSERT_LE(0, fd) { TH_LOG("%s: %s", path, strerror(errno)); }
	ASSERT_EQ(0, intro_query(&intro, fd, CTLV_OP_FAMILIES_DUMP, NULL,
				 reply, sizeof(reply)));
	EXPECT_STREQ(event.chain.buf, intro.logical.buf);
	close(fd);
}

/* The event carries the path, so the parent is string work, not a walk. */
TEST_F(discovery, an_addition_names_what_it_hangs_off)
{
	char link[CTLV_PATH_LEN], target[CTLV_PATH_LEN];
	struct discovery_event event;
	ssize_t len;
	char *at;

	ASSERT_EQ(0, ctlv_instances_set(2));
	self->added = true;
	discovery_next(_metadata, &event, &self->mon);

	/* The path without the node and without the class directory. */
	at = strrchr(event.devpath, '/');
	ASSERT_NE(NULL, at);
	EXPECT_STREQ(event.devname, at + 1);
	*at = '\0';
	at = strrchr(event.devpath, '/');
	ASSERT_NE(NULL, at);
	EXPECT_STREQ("ctlv", at + 1);
	*at = '\0';
	EXPECT_STREQ("ctlv_test", basename(event.devpath));

	/* Which is where the device itself says it hangs, by another road. */
	snprintf(link, sizeof(link), "%s/%s/device", CTLV_CLASS_DIR,
		 event.devname);
	len = readlink(link, target, sizeof(target) - 1);
	ASSERT_LT(0, len);
	target[len] = '\0';
	EXPECT_STREQ(basename(event.devpath), basename(target));
}

/* No third place for the event and the device to disagree in. */
TEST_F(discovery, an_addition_adds_no_family_attributes)
{
	struct discovery_event event;
	char buf[CTLV_NAME_LEN];

	ASSERT_EQ(0, ctlv_instances_set(2));
	self->added = true;
	discovery_next(_metadata, &event, &self->mon);

	EXPECT_EQ(-ENOENT, ctlv_attr_read(event.devname, "family", buf,
					  sizeof(buf)));
	EXPECT_EQ(-ENOENT, ctlv_attr_read(event.devname, "ancestors", buf,
					  sizeof(buf)));
	/* The node is what a device offers, and the event named it. */
	EXPECT_EQ(0, ctlv_attr_read(event.devname, "dev", buf, sizeof(buf)));
}

TEST_F(discovery, a_removal_reaches_a_listener)
{
	struct discovery_event parent, child;
	char gone[2][CTLV_NAME_LEN];
	char name[CTLV_NAME_LEN];
	unsigned int i;

	ASSERT_EQ(0, ctlv_instances_set(2));
	self->added = true;
	discovery_next(_metadata, &parent, &self->mon);
	discovery_next(_metadata, &child, &self->mon);

	ASSERT_EQ(0, ctlv_instances_set(1));
	self->added = false;
	for (i = 0; i < ARRAY_SIZE(gone); i++) {
		ASSERT_EQ(0, ctlv_monitor_wait(&self->mon, "remove"));
		ASSERT_EQ(0, ctlv_monitor_read(&self->mon, "DEVNAME", gone[i],
					       sizeof(gone[i])));
	}
	/* Both of them, whichever order the instance took them down in. */
	EXPECT_TRUE(!strcmp(gone[0], parent.devname) ||
		    !strcmp(gone[1], parent.devname));
	EXPECT_TRUE(!strcmp(gone[0], child.devname) ||
		    !strcmp(gone[1], child.devname));
	EXPECT_STRNE(gone[0], gone[1]);

	/* And the enumeration agrees with the events again. */
	EXPECT_EQ(-ENODEV, ctlv_device_name("ctlv-test", 1, name,
					    sizeof(name)));
}

TEST(the_class_is_registered)
{
	struct stat st;

	ASSERT_EQ(0, stat(CTLV_CLASS_DIR, &st)) {
		TH_LOG("%s is missing, is CONFIG_CTLV enabled?",
		       CTLV_CLASS_DIR);
	}
	ASSERT_TRUE(S_ISDIR(st.st_mode));
}

/* The family it registered as, then its chain from the oldest family. */
TEST(a_device_announces_its_family_and_its_chain)
{
	char name[CTLV_NAME_LEN], buf[CTLV_NAME_LEN];

	if (ctlv_device_name("ctlv-test", 0, name, sizeof(name)))
		SKIP(return, "no ctlv-test device");
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_FAMILY", buf, sizeof(buf)));
	EXPECT_STREQ("ctlv-test", buf);
	/* The root family is the oldest in every chain. */
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_0", buf,
				      sizeof(buf)));
	EXPECT_STREQ("ctlv", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_1", buf,
				      sizeof(buf)));
	EXPECT_STREQ("ctlv-test", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_N", buf,
				      sizeof(buf)));
	EXPECT_STREQ("2", buf);
	EXPECT_EQ(-ENOENT, ctlv_uevent_read(name, "CTLV_COMPATIBLE_2", buf,
					    sizeof(buf)));

	ASSERT_EQ(0, ctlv_device_name("ctlv-test-child", 0, name,
				      sizeof(name)));
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_FAMILY", buf, sizeof(buf)));
	EXPECT_STREQ("ctlv-test-child", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_0", buf,
				      sizeof(buf)));
	EXPECT_STREQ("ctlv", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_1", buf,
				      sizeof(buf)));
	EXPECT_STREQ("ctlv-test", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_2", buf,
				      sizeof(buf)));
	EXPECT_STREQ("ctlv-test-child", buf);
	ASSERT_EQ(0, ctlv_uevent_read(name, "CTLV_COMPATIBLE_N", buf,
				      sizeof(buf)));
	EXPECT_STREQ("3", buf);
}

/* Nothing about a family is a sysfs attribute of its own. */
TEST(a_device_has_no_family_attributes)
{
	char name[CTLV_NAME_LEN], buf[CTLV_NAME_LEN];

	if (ctlv_device_name("ctlv-test", 0, name, sizeof(name)))
		SKIP(return, "no ctlv-test device");
	EXPECT_EQ(-ENOENT, ctlv_attr_read(name, "family", buf, sizeof(buf)));
	EXPECT_EQ(-ENOENT, ctlv_attr_read(name, "ancestors", buf,
					  sizeof(buf)));
}

/* The node under /dev is the device the class directory describes. */
TEST(a_device_has_one_node)
{
	char name[CTLV_NAME_LEN], buf[CTLV_PATH_LEN], path[CTLV_PATH_LEN];
	unsigned int expect_major, expect_minor;
	struct stat st;
	ssize_t len;
	int parsed;

	if (ctlv_device_name("ctlv-test", 0, name, sizeof(name)))
		SKIP(return, "no ctlv-test device");

	ASSERT_EQ(0, ctlv_attr_read(name, "dev", buf, sizeof(buf)));
	parsed = sscanf(buf, "%u:%u", &expect_major, &expect_minor);
	ASSERT_EQ(2, parsed);

	snprintf(path, sizeof(path), "/dev/%s", name);
	ASSERT_EQ(0, stat(path, &st));
	EXPECT_TRUE(S_ISCHR(st.st_mode));
	EXPECT_EQ(expect_major, major(st.st_rdev));
	EXPECT_EQ(expect_minor, minor(st.st_rdev));

	/* And it hangs off whatever the family registered it under. */
	snprintf(path, sizeof(path), "%s/%s/device", CTLV_CLASS_DIR, name);
	len = readlink(path, buf, sizeof(buf) - 1);
	ASSERT_LT(0, len);
	buf[len] = '\0';
	EXPECT_STREQ("ctlv_test", basename(buf));
}

/* Every case here leaves the module at one instance, as the rest expects. */
FIXTURE(instances) {
	int fd;
};

FIXTURE_SETUP(instances)
{
	self->fd = -1;
	if (!ctlv_instances_available())
		SKIP(return, "no %s control", CTLV_TEST_DEVICE_ADD);
}

FIXTURE_TEARDOWN(instances)
{
	if (self->fd >= 0)
		close(self->fd);
	/* Whatever a case narrowed is gone with it, so this is whole again. */
	ctlv_instances_set(1);
}

/* Instances are independent, and there are exactly the ones asked for. */
TEST_F(instances, a_second_instance)
{
	char buf[CTLV_NAME_LEN];
	int first, second;

	ASSERT_EQ(0, ctlv_instances_set(2));
	EXPECT_EQ(0, ctlv_device_name("ctlv-test", 1, buf, sizeof(buf)));
	EXPECT_EQ(-ENODEV, ctlv_device_name("ctlv-test", 2, buf, sizeof(buf)));

	second = ctlv_open("ctlv-test", 1, O_RDWR);
	ASSERT_LE(0, second);
	self->fd = second;
	first = ctlv_open("ctlv-test", 0, O_RDWR);
	ASSERT_LE(0, first);

	/* Both devices of one family start the same way. */
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(first, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(second, CTLV_TEST_OP_DYNAMIC_NOOP));

	/* And what one of them then does is its own. */
	ASSERT_EQ(0, ctlv_exec_reply(second, CTLV_TEST_OP_DYNAMIC_TOGGLE, buf,
				     sizeof(buf)));
	EXPECT_EQ(0, ctlv_exec_bare(second, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(first, CTLV_TEST_OP_DYNAMIC_NOOP));
	close(first);
}

/* Refused where an op of no family is; narrowed on the second instance. */
TEST_F(instances, ops_a_device_never_published)
{
	char dump[INTRO_REPLY_LEN];
	char actual[CTLV_LOG_LEN];
	int narrowed, whole, child;
	struct intro intro;
	char reply[64];

	ASSERT_EQ(0, ctlv_instances_set_narrowed(2));
	narrowed = ctlv_open("ctlv-test", 1, O_RDWR);
	ASSERT_LE(0, narrowed);
	self->fd = narrowed;

	ASSERT_EQ(0, ctlv_hooks_clear());
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(narrowed, CTLV_TEST_OP_DYNAMIC_NOOP));
	/* The op needs an output buffer this withholds: gone answers first. */
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(narrowed, CTLV_TEST_OP_DYNAMIC_TOGGLE));
	/* Which is exactly what an op nobody ever declared answers. */
	EXPECT_EQ(-EOPNOTSUPP, ctlv_exec_bare(narrowed, CTLV_OP_ID_NOBODY));
	ASSERT_EQ(0, ctlv_hooks_read(actual, sizeof(actual)));
	EXPECT_STREQ("", actual);

	/* What it kept, it answers. */
	EXPECT_EQ(0, ctlv_exec_bare(narrowed, CTLV_TEST_OP_NOOP));

	/* An op it never published has no entry and no schema either. */
	EXPECT_EQ(-EOPNOTSUPP,
		  intro_schema(&intro, narrowed, CTLV_TEST_OP_DYNAMIC_NOOP,
			       dump, sizeof(dump)));
	EXPECT_EQ(-EOPNOTSUPP, intro_schema(&intro, narrowed, CTLV_OP_ID_NOBODY,
					    dump, sizeof(dump)));
	ASSERT_EQ(0, intro_query(&intro, narrowed, CTLV_OP_OPS_DUMP, NULL, dump,
				 sizeof(dump)));
	EXPECT_OP_ENTRY(dump, CTLV_TEST_OP_DYNAMIC_NOOP, "");
	EXPECT_OP_ENTRY(dump, CTLV_TEST_OP_DYNAMIC_TOGGLE, "");
	/* What it kept is there, whichever family declared it. */
	EXPECT_NE(NULL, intro_entry(dump, CTLV_TEST_OP_NOOP));
	EXPECT_NE(NULL, intro_entry(dump, CTLV_OP_FAMILIES_DUMP));

	/* The inherited op is gone from the inheriting family's device too. */
	child = ctlv_open("ctlv-test-child", 1, O_RDWR);
	ASSERT_LE(0, child);
	EXPECT_EQ(-EOPNOTSUPP,
		  ctlv_exec_bare(child, CTLV_TEST_OP_DYNAMIC_NOOP));
	EXPECT_EQ(0, ctlv_exec_bare(child, CTLV_TEST_CHILD_OP_EXTRA_NOOP));
	close(child);

	/* And the op is the family's, so the device that kept it has it. */
	whole = ctlv_open("ctlv-test", 0, O_RDWR);
	ASSERT_LE(0, whole);
	EXPECT_EQ(-EMSGSIZE, ctlv_exec_bare(whole, CTLV_TEST_OP_DYNAMIC_TOGGLE));
	ASSERT_EQ(0, ctlv_exec_reply(whole, CTLV_TEST_OP_DYNAMIC_TOGGLE, reply,
				     sizeof(reply)));
	EXPECT_EQ(0, ctlv_exec_bare(whole, CTLV_TEST_OP_DYNAMIC_NOOP));
	/* Left as it was found, for whatever runs next. */
	ASSERT_EQ(0, ctlv_exec_reply(whole, CTLV_TEST_OP_DYNAMIC_TOGGLE, reply,
				     sizeof(reply)));
	close(whole);
}

/* The instance stays allocated, so nothing follows a table that is gone. */
TEST_F(instances, a_file_that_outlives_its_device)
{
	char name[CTLV_NAME_LEN], path[CTLV_PATH_LEN];

	ASSERT_EQ(0, ctlv_device_name("ctlv-test", 0, name, sizeof(name)));
	self->fd = ctlv_open("ctlv-test", 0, O_RDWR);
	ASSERT_LE(0, self->fd);
	ASSERT_EQ(0, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));

	ASSERT_EQ(0, ctlv_instances_set(0));

	/* Death answers everything, before anything is even looked up. */
	EXPECT_EQ(-ENODEV, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
	EXPECT_EQ(-ENODEV, ctlv_exec_bare(self->fd, 0));
	EXPECT_EQ(-ENODEV, ctlv_exec_bare(self->fd, CTLV_OP_ID_NOBODY));

	/* And the device is gone, so nothing can arrive at it again. */
	EXPECT_EQ(-ENODEV, ctlv_open("ctlv-test", 0, O_RDWR));
	snprintf(path, sizeof(path), "/dev/%s", name);
	EXPECT_EQ(-1, access(path, F_OK));
}

/* A reader drains what it was sent, then reads the record that ends it. */
TEST_F(instances, a_queue_that_outlives_its_device)
{
	struct pollfd pfd;
	struct event ev;

	self->fd = ctlv_open("ctlv-test", 0, O_RDWR | O_NONBLOCK);
	ASSERT_LE(0, self->fd);
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	ASSERT_EQ(0, event_beep(self->fd, 2));

	ASSERT_EQ(0, ctlv_instances_set(0));

	/* Both at once: there is something to read, and it is the last of it. */
	pfd = (struct pollfd){ .fd = self->fd, .events = POLLIN };
	ASSERT_EQ(1, poll(&pfd, 1, 0));
	EXPECT_TRUE(pfd.revents & POLLIN);
	EXPECT_TRUE(pfd.revents & POLLHUP);

	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(BEEPED_LEN, event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 2);
	/* Which nobody subscribed to, and behind which nothing is queued. */
	ASSERT_EQ(DEVICE_REMOVED_LEN, event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_OP_DEVICE_REMOVED, 3);

	EXPECT_EQ(-ENODEV, event_read(self->fd, &ev));
	EXPECT_EQ(-ENODEV, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
	ASSERT_EQ(1, poll(&pfd, 1, 0));
	EXPECT_FALSE(pfd.revents & POLLIN);
	EXPECT_TRUE(pfd.revents & POLLHUP);
}

/* And a queue with no room left has room for it, that room being reserved. */
TEST_F(instances, a_full_queue_that_outlives_its_device)
{
	const struct burst burst = { .count = 8, .filler = PING_FILLER_LEN };
	struct event ev;

	self->fd = ctlv_open("ctlv-test", 0, O_RDWR | O_NONBLOCK);
	ASSERT_LE(0, self->fd);
	ASSERT_EQ(0, event_subscribe(self->fd, CTLV_TEST_OP_BEEPED));
	/* The shortest queue there is, which holds one such record. */
	ASSERT_EQ(0, event_queue_set(self->fd, EVENT_QUEUE_MIN));
	ASSERT_EQ(0, event_burst(self->fd, &burst));

	ASSERT_EQ(0, ctlv_instances_set(0));

	ASSERT_EQ(BEEPED_FILLED_LEN(PING_FILLER_LEN), event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_TEST_OP_BEEPED, 1);
	ASSERT_EQ(EVENT_OVERFLOW_LEN, event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_OP_EVENT_OVERFLOW, 2);
	EXPECT_EQ(burst.count - 1,
		  event_u64(&ev, CTLV_ATTR_EVENT_DROPPED_COUNT));
	ASSERT_EQ(DEVICE_REMOVED_LEN, event_read(self->fd, &ev));
	EXPECT_RECORD(&ev, CTLV_OP_DEVICE_REMOVED, burst.count + 1);
	EXPECT_EQ(-ENODEV, event_read(self->fd, &ev));
}

/* All racing, so this checks only that no result is outside the races. */
struct worker {
	pthread_t thread;
	atomic_bool *stop;
	int fd;
	const char *family;
	unsigned int rounds;
	/* The first result that no race could have produced. */
	int unexpected;
};

static void worker_note(struct worker *worker, int ret, bool may_be_disabled)
{
	if (!ret || ret == -ENODEV)
		return;
	if (may_be_disabled && ret == -EOPNOTSUPP)
		return;
	if (!worker->unexpected)
		worker->unexpected = ret;
}

static void *worker_execute(void *arg)
{
	struct worker *worker = arg;
	char reply[64];

	while (!atomic_load(worker->stop)) {
		worker_note(worker,
			    ctlv_exec_bare(worker->fd, CTLV_TEST_OP_NOOP),
			    false);
		worker_note(worker,
			    ctlv_exec_bare(worker->fd,
					   CTLV_TEST_OP_DYNAMIC_NOOP),
			    true);
		worker_note(worker,
			    ctlv_exec_reply(worker->fd,
					    CTLV_TEST_OP_DYNAMIC_TOGGLE,
					    reply, sizeof(reply)),
			    false);
		worker->rounds++;
	}
	return NULL;
}

static void *worker_open(void *arg)
{
	struct worker *worker = arg;

	while (!atomic_load(worker->stop)) {
		int fd = ctlv_open(worker->family, 0, O_RDWR);

		/* The device may be gone, and its node with it. */
		if (fd == -ENODEV || fd == -ENOENT) {
			usleep(100);
			continue;
		}
		if (fd < 0) {
			worker_note(worker, fd, false);
			break;
		}
		worker_note(worker, ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP),
			    false);
		close(fd);
		worker->rounds++;
	}
	return NULL;
}

TEST_F(instances, execution_while_the_devices_go_away)
{
	struct worker workers[6] = {};
	atomic_bool stop = false;
	unsigned int index;

	/* Two instances, so that the removal has more than one to tear down. */
	ASSERT_EQ(0, ctlv_instances_set(2));
	self->fd = ctlv_open("ctlv-test", 0, O_RDWR);
	ASSERT_LE(0, self->fd);

	for (index = 0; index < ARRAY_SIZE(workers); index++) {
		workers[index].stop = &stop;
		workers[index].fd = self->fd;
		workers[index].family = "ctlv-test";
		ASSERT_EQ(0, pthread_create(&workers[index].thread, NULL,
					    index ? worker_execute :
						    worker_open,
					    &workers[index]));
	}

	usleep(20000);
	EXPECT_EQ(0, ctlv_instances_set(0));
	usleep(20000);
	atomic_store(&stop, true);

	for (index = 0; index < ARRAY_SIZE(workers); index++) {
		ASSERT_EQ(0, pthread_join(workers[index].thread, NULL));
		EXPECT_EQ(0, workers[index].unexpected) {
			TH_LOG("worker %u: %s", index,
			       strerror(-workers[index].unexpected));
		}
		EXPECT_LT(0, workers[index].rounds);
	}
	EXPECT_EQ(-ENODEV, ctlv_exec_bare(self->fd, CTLV_TEST_OP_NOOP));
}

int main(int argc, char **argv)
{
	/* Nothing else in a test image loads what these devices come from. */
	ctlv_test_module_load();
	return test_harness_run(argc, argv);
}
