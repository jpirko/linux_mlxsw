// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV operation benchmark with fixed-ioctl and netlink baselines.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_test_gen.h>
#include <linux/genetlink.h>
#include <linux/netlink.h>

/* Samples per case, and how many are thrown away first. */
#define BENCH_SAMPLES 20000
#define BENCH_WARMUP 2000

/* The largest tree a case builds, and the reply buffer every case offers. */
#define BENCH_MSG_LEN (1024 * 1024)
#define BENCH_REPLY_LEN (1024 * 1024)

/*
 * The bounds the test family declares for the two shapes measured here, which
 * are the family's rather than the core's.
 */
#define BENCH_ECHO_COUNTS 3
#define BENCH_ECHO_ITEMS 2
#define BENCH_ECHO_TEXT_LEN 15
#define BENCH_ECHO_DATA_LEN 32
#define BENCH_ITEM_LABEL_LEN 7
#define BENCH_SERIES_MAX 8

/* A device is in its subsystem's class, so every char device is looked at. */
#define CHAR_DIR "/sys/dev/char"
#define NAME_LEN 256
#define PATH_LEN 1024

static char *request_buf;
static char *reply_buf;
static unsigned long long *samples;

static const struct ctlv_hdr empty_request = {
	.len = sizeof(struct ctlv_hdr),
	.flags = CTLV_HDR_F_RECORD,
	.type = CTLV_TYPE_ROOT,
};

static unsigned long long now_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long long)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static int compare_ns(const void *a, const void *b)
{
	unsigned long long x = *(const unsigned long long *)a;
	unsigned long long y = *(const unsigned long long *)b;

	return x < y ? -1 : x > y;
}

/* One call at a time, a batch inside one timing would hide the tail. */
static void bench(const char *what, unsigned int bytes,
		  int (*once)(void *arg), void *arg)
{
	unsigned int taken = 0, i;
	unsigned long long start;
	int ret = 0;

	for (i = 0; i < BENCH_WARMUP && !ret; i++)
		ret = once(arg);
	if (ret) {
		/*
		 * With the reason: a refused case is nearly always this
		 * program asking for what the family does not declare.
		 */
		printf("%-34s %s (%s)\n", what, "unavailable",
		       strerror(ret < 0 ? -ret : ret));
		return;
	}
	for (i = 0; i < BENCH_SAMPLES; i++) {
		start = now_ns();
		if (once(arg))
			continue;
		samples[taken++] = now_ns() - start;
	}
	if (!taken) {
		printf("%-34s %s\n", what, "unavailable");
		return;
	}
	qsort(samples, taken, sizeof(*samples), compare_ns);
	printf("%-34s %8llu %8llu %10u\n", what, samples[taken / 2],
	       samples[taken - taken / 100 - 1], bytes);
}

/* Local serializers exclude linked-library overhead. */
struct msg {
	char *buf;
	__u32 len;
};

static void msg_init(struct msg *msg, char *buf)
{
	struct ctlv_hdr *root = (struct ctlv_hdr *)buf;

	msg->buf = buf;
	msg->len = sizeof(*root);
	root->len = sizeof(*root);
	root->flags = CTLV_HDR_F_RECORD;
	root->type = CTLV_TYPE_ROOT;
}

/* Room for one TLV's payload, the message and the root grown to hold it. */
static void *msg_payload(struct msg *msg, __u64 type, __u32 len)
{
	struct ctlv_hdr *hdr = (struct ctlv_hdr *)(msg->buf + msg->len);

	hdr->len = sizeof(*hdr) + len;
	hdr->flags = 0;
	hdr->type = type;
	msg->len += CTLV_ALIGN(hdr->len);
	((struct ctlv_hdr *)msg->buf)->len = msg->len;
	return hdr + 1;
}

static void msg_u64(struct msg *msg, __u64 type, __u64 value)
{
	memcpy(msg_payload(msg, type, sizeof(value)), &value,
	       sizeof(value));
}

static void msg_bytes(struct msg *msg, __u64 type, const void *src,
		      __u32 len)
{
	memcpy(msg_payload(msg, type, len), src, len);
}

/*
 * Every member is eight-aligned and so is this header, so what msg_record_end()
 * measures is exactly the header plus the aligned members.
 */
static struct ctlv_hdr *msg_record_start(struct msg *msg, __u64 type)
{
	struct ctlv_hdr *hdr = (struct ctlv_hdr *)(msg->buf + msg->len);

	hdr->len = sizeof(*hdr);
	hdr->flags = CTLV_HDR_F_RECORD;
	hdr->type = type;
	msg->len += sizeof(*hdr);
	((struct ctlv_hdr *)msg->buf)->len = msg->len;
	return hdr;
}

static void msg_record_end(struct msg *msg, struct ctlv_hdr *record)
{
	record->len = msg->buf + msg->len - (char *)record;
}

struct exec_case {
	int fd;
	__u64 op_id;
	__u32 request_len;
	__u32 reply_len;
};

static int once_exec(void *arg)
{
	const struct exec_case *c = arg;
	struct ctlv_op_exec exec = {
		.size = sizeof(exec),
		.op_id = c->op_id,
		.request = (__u64)(uintptr_t)request_buf,
		.request_len = c->request_len,
		.reply = (__u64)(uintptr_t)reply_buf,
		.reply_len = c->reply_len,
	};

	if (!c->reply_len)
		exec.reply = 0;
	do {
		if (ioctl(c->fd, CTLV_IOC_OP_EXEC, &exec))
			return -errno;
	} while (exec.cursor);
	return 0;
}

/*
 * The structure copied in and validated, then refused for naming an op the
 * family has not got: everything a plain ioctl does except the write-back.
 */
static int once_refused_op(void *arg)
{
	int fd = *(int *)arg;
	struct ctlv_op_exec exec = {
		.size = sizeof(exec),
		.op_id = ~0ULL,
		.request = (__u64)(uintptr_t)&empty_request,
		.request_len = sizeof(empty_request),
	};

	ioctl(fd, CTLV_IOC_OP_EXEC, &exec);
	return 0;
}

/* The floor: a syscall on the same descriptor that is refused at the door. */
static int once_syscall_floor(void *arg)
{
	int fd = *(int *)arg;

	ioctl(fd, _IO(CTLV_IOC_MAGIC, 0x7f));
	return 0;
}

/*
 * One generic-netlink round trip that parses one attribute and answers with a
 * tree of them, the closest thing in the kernel to a CTLV query.
 */
struct netlink_case {
	int fd;
	char request[256];
	unsigned int request_len;
	char reply[8192];
};

static int netlink_open(struct netlink_case *c)
{
	struct sockaddr_nl addr = { .nl_family = AF_NETLINK };
	struct nlmsghdr *nlh = (struct nlmsghdr *)c->request;
	struct genlmsghdr *genl;
	struct nlattr *nla;
	static const char family[] = "nlctrl";
	unsigned int at;

	c->fd = socket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, NETLINK_GENERIC);
	if (c->fd < 0)
		return -errno;
	if (bind(c->fd, (struct sockaddr *)&addr, sizeof(addr))) {
		close(c->fd);
		c->fd = -1;
		return -errno;
	}

	memset(c->request, 0, sizeof(c->request));
	at = NLMSG_HDRLEN;
	genl = (struct genlmsghdr *)(c->request + at);
	genl->cmd = CTRL_CMD_GETFAMILY;
	genl->version = 1;
	at += NLMSG_ALIGN(sizeof(*genl));
	nla = (struct nlattr *)(c->request + at);
	nla->nla_type = CTRL_ATTR_FAMILY_NAME;
	nla->nla_len = NLA_HDRLEN + sizeof(family);
	memcpy(nla + 1, family, sizeof(family));
	at += NLA_ALIGN(nla->nla_len);

	nlh->nlmsg_len = at;
	nlh->nlmsg_type = GENL_ID_CTRL;
	nlh->nlmsg_flags = NLM_F_REQUEST;
	nlh->nlmsg_seq = 1;
	c->request_len = at;
	return 0;
}

static int once_netlink(void *arg)
{
	struct netlink_case *c = arg;
	ssize_t got;

	if (send(c->fd, c->request, c->request_len, 0) < 0)
		return -errno;
	got = recv(c->fd, c->reply, sizeof(c->reply), 0);
	if (got < 0)
		return -errno;
	return 0;
}

static int read_sysfs(const char *path, char *buf, size_t len)
{
	ssize_t got;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return -errno;
	got = read(fd, buf, len - 1);
	close(fd);
	if (got < 0)
		return -errno;
	while (got && buf[got - 1] == '\n')
		got--;
	buf[got] = '\0';
	return 0;
}

/* What the uevent @text gives @property, to the end of its line. */
static int uevent_value(const char *text, const char *property, char *buf,
			size_t len)
{
	size_t prefix = strlen(property);
	const char *at = text;

	while (strncmp(at, property, prefix) || at[prefix] != '=') {
		at = strchr(at, '\n');
		if (!at)
			return -ENOENT;
		at++;
	}
	at += prefix + 1;
	snprintf(buf, len, "%.*s", (int)strcspn(at, "\n"), at);
	return 0;
}

/* The first device answering family, by what it announces. */
static int open_family(const char *family)
{
	char path[PATH_LEN], found[NAME_LEN], devname[NAME_LEN];
	char text[NAME_LEN * 4];
	struct dirent **entries;
	int n, i, fd = -ENODEV;

	n = scandir(CHAR_DIR, &entries, NULL, alphasort);
	if (n < 0)
		return -errno;
	for (i = 0; i < n; i++) {
		const char *name = entries[i]->d_name;

		if (name[0] == '.' || fd >= 0)
			continue;
		snprintf(path, sizeof(path), "%s/%s/uevent", CHAR_DIR, name);
		if (read_sysfs(path, text, sizeof(text)) ||
		    uevent_value(text, "CTLV_FAMILY", found, sizeof(found)) ||
		    strcmp(found, family) ||
		    uevent_value(text, "DEVNAME", devname, sizeof(devname)))
			continue;
		snprintf(path, sizeof(path), "/dev/%s", devname);
		fd = open(path, O_RDWR);
		if (fd < 0)
			fd = -errno;
	}
	for (i = 0; i < n; i++)
		free(entries[i]);
	free(entries);
	return fd;
}

int main(void)
{
	struct exec_case ack, small, wide, series;
	struct netlink_case netlink;
	int fd;
	unsigned int i;
	struct msg msg;

	request_buf = aligned_alloc(CTLV_ALIGNTO, BENCH_MSG_LEN);
	reply_buf = aligned_alloc(CTLV_ALIGNTO, BENCH_REPLY_LEN);
	samples = calloc(BENCH_SAMPLES, sizeof(*samples));
	if (!request_buf || !reply_buf || !samples) {
		fprintf(stderr, "out of memory\n");
		return 1;
	}

	fd = open_family("ctlv-test");
	if (fd < 0) {
		fprintf(stderr, "no ctlv-test device: %s\n", strerror(-fd));
		return 1;
	}

	printf("%-34s %8s %8s %10s\n", "case", "p50 ns", "p99 ns", "bytes");

	msg_init(&msg, request_buf);
	ack = (struct exec_case){
		.fd = fd,
		.op_id = CTLV_TEST_OP_NOOP,
		.request_len = msg.len,
	};
	bench("ctlv action, empty request", msg.len, once_exec, &ack);

	msg_init(&msg, request_buf);
	msg_u64(&msg, CTLV_TEST_T_TOKEN_INFO_TOKEN, 7);
	small = (struct exec_case){
		.fd = fd,
		.op_id = CTLV_TEST_OP_PROBE_GET,
		.request_len = msg.len,
		.reply_len = 4096,
	};
	bench("ctlv query, one field each way", msg.len, once_exec, &small);

	/*
	 * The two blob fields are left out on purpose: what they would add is a
	 * pin and a mapping rather than any parsing.
	 */
	msg_init(&msg, request_buf);
	msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_U, 4096);
	msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_S, (__u64)-100);
	msg_bytes(&msg, CTLV_TEST_T_ECHO_VALUES_FLAG, "\1", 1);
	msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_MODE, CTLV_TEST_ECHO_MODE_AUTO);
	msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_QUIRKS,
		CTLV_TEST_ECHO_QUIRK_MASK);
	msg_bytes(&msg, CTLV_TEST_T_ECHO_VALUES_TEXT, "123456789012345",
		  BENCH_ECHO_TEXT_LEN + 1);
	msg_bytes(&msg, CTLV_TEST_T_ECHO_VALUES_DATA, request_buf,
		  BENCH_ECHO_DATA_LEN);
	for (i = 0; i < BENCH_ECHO_COUNTS; i++)
		msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_COUNT, i);
	for (i = 0; i < BENCH_ECHO_ITEMS; i++) {
		struct ctlv_hdr *item;

		item = msg_record_start(&msg, CTLV_TEST_T_ECHO_VALUES_ITEM);
		msg_u64(&msg, CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX, i);
		msg_bytes(&msg, CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL, "123456",
			  BENCH_ITEM_LABEL_LEN);
		msg_record_end(&msg, item);
	}
	wide = (struct exec_case){
		.fd = fd,
		.op_id = CTLV_TEST_OP_ECHO,
		.request_len = msg.len,
	};
	bench("ctlv action, widest request", msg.len, once_exec, &wide);

	msg_init(&msg, request_buf);
	msg_u64(&msg, CTLV_TEST_T_SERIES_GET_CHUNKS, BENCH_SERIES_MAX);
	msg_u64(&msg, CTLV_TEST_T_SERIES_GET_ITEMS, BENCH_SERIES_MAX);
	msg_u64(&msg, CTLV_TEST_T_SERIES_GET_VALUES, BENCH_SERIES_MAX);
	series = (struct exec_case){
		.fd = fd,
		.op_id = CTLV_TEST_OP_SERIES_GET,
		.request_len = msg.len,
		.reply_len = BENCH_REPLY_LEN,
	};
	bench("ctlv query, 512-leaf reply", msg.len, once_exec, &series);

	bench("syscall floor, refused ioctl", 0, once_syscall_floor, &fd);
	bench("exec floor, refused op", sizeof(struct ctlv_op_exec),
	      once_refused_op, &fd);

	if (!netlink_open(&netlink)) {
		bench("genetlink round trip", netlink.request_len,
		      once_netlink, &netlink);
		close(netlink.fd);
	} else {
		printf("%-34s %s\n", "genetlink round trip", "unavailable");
	}

	close(fd);
	free(samples);
	free(reply_buf);
	free(request_buf);
	return 0;
}
