// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * 32-bit CTLV ABI crossing tests; compat32.sh emits KTAP.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <linux/ctlv_test_gen.h>

#include "ctlv_tlv.h"
#include "ctlv_util.h"

#define FAMILY "ctlv-test"

/* kselftest's exit codes, without the harness this program does not use. */
#define EXIT_SKIP 4

/* Room for the largest reply asked for here, and for one event record. */
#define REPLY_LEN 4096
#define RECORD_LEN 512

/* A series that fits one fragment, and one FRAGMENT_LEN cannot hold. */
#define SERIES_SMALL 2
#define SERIES_LARGE 8

/* What a fragmented answer is asked to squeeze through. */
#define FRAGMENT_LEN 256

/* More fragments than the series above needs, so a stuck cursor ends here. */
#define MAX_FRAGS 1024

/* The blob this program hands over, and the pattern it holds. */
#define BLOB_LEN 4096
#define BLOB_BYTE 0x5a

static unsigned int failures;

static void report(const char *what, bool ok, const char *detail)
{
	printf("%s: %s%s%s\n", ok ? "ok" : "not ok", what,
	       detail ? ", " : "", detail ? : "");
	if (!ok)
		failures++;
}

static void report_errno(const char *what, int ret)
{
	report(what, !ret, ret ? strerror(-ret) : NULL);
}

/* An action that carries nothing: the ioctl itself. */
static void an_action(int fd)
{
	report_errno("an action crosses",
		     ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP));
}

/* The first child of a reply's root carrying attr_id, or NULL. */
static const struct ctlv_attr *find_attr(const void *reply, __u64 attr_id)
{
	const struct ctlv_attr *root = reply;
	const struct ctlv_attr *child;

	ctlv_rd_for_each(child, root)
		if (child->attr_id == attr_id)
			return child;
	return NULL;
}

/*
 * Both buffers sit in the low half of the fields naming them, and the token is
 * wide enough that a truncated copy would not come back equal.
 */
static void a_query(int fd)
{
	static const __u64 token = 0x0123456789abcdefULL;
	const struct ctlv_attr *echoed;
	char reply[REPLY_LEN];
	struct ctlv_tlv tlv;
	int ret;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PROBE_TOKEN, token);
	ret = ctlv_exec_full(fd, CTLV_TEST_OP_PROBE_GET, tlv.buf,
			     ctlv_tlv_done(&tlv), reply, sizeof(reply));
	report_errno("a query crosses", ret);
	if (ret)
		return;

	echoed = find_attr(reply, CTLV_TEST_ATTR_PROBE_TOKEN);
	if (!echoed) {
		report("a token comes back whole", false, "nothing echoed");
		return;
	}
	report("a token comes back whole", ctlv_rd_u64(echoed) == token,
	       ctlv_rd_u64(echoed) == token ? NULL : "a different value");
}

/*
 * The cursor and the generation are 64 bits wide and neither is a pointer, so
 * this is where a compat layout mistake would show.
 */
static void a_fragmented_query(int fd)
{
	char reply[FRAGMENT_LEN];
	struct ctlv_op_exec exec = {};
	unsigned int frags = 0;
	struct ctlv_tlv tlv;
	bool resumed = false;
	int ret;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_CHUNKS, SERIES_LARGE);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_ITEMS, SERIES_SMALL);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_SERIES_VALUES, SERIES_SMALL);

	exec.op_id = CTLV_TEST_OP_SERIES_DUMP;
	exec.request = ctlv_ptr(tlv.buf);
	exec.request_len = ctlv_tlv_done(&tlv);
	exec.reply = ctlv_ptr(reply);
	exec.reply_len = sizeof(reply);
	do {
		ret = ctlv_exec(fd, &exec);
		if (ret)
			break;
		if (exec.cursor)
			resumed = true;
	} while (exec.cursor && ++frags < MAX_FRAGS);

	if (ret) {
		report_errno("a fragmented query crosses", ret);
		return;
	}
	report("a fragmented query crosses", frags < MAX_FRAGS,
	       frags < MAX_FRAGS ? NULL : "the cursor never cleared");
	report("a cursor survives the crossing", resumed,
	       resumed ? NULL : "the answer fit, nothing was resumed");
}

/* The fd, offset and length are 64-bit fields a narrow caller half-fills. */
static void a_blob(int fd)
{
	char reply[REPLY_LEN];
	struct ctlv_tlv tlv;
	int blob, ret;

	blob = memfd_create("ctlv-compat32", MFD_CLOEXEC | MFD_ALLOW_SEALING);
	if (blob < 0) {
		report("a blob crosses", false, strerror(errno));
		return;
	}
	/* Sealed against shrinking, which is what a memfd blob has to be. */
	if (ftruncate(blob, BLOB_LEN) ||
	    fcntl(blob, F_ADD_SEALS, F_SEAL_SHRINK)) {
		report("a blob crosses", false, strerror(errno));
		close(blob);
		return;
	}

	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_memfd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, blob, 0,
			    BLOB_LEN);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, BLOB_BYTE);
	ret = ctlv_exec_full(fd, CTLV_TEST_OP_TRANSFER, tlv.buf,
			     ctlv_tlv_done(&tlv), reply, sizeof(reply));
	report_errno("a blob crosses", ret);
	close(blob);
}

/* Reject nonzero high VA bits from a 32-bit caller rather than truncating. */
static void a_va_blob(int fd)
{
	/* A bit above anything a pointer of this width can address. */
	const __u64 beyond = sizeof(void *) == 4 ? 1ULL << 32 : 1ULL << 63;
	char reply[REPLY_LEN];
	struct ctlv_tlv tlv;
	void *map;
	int ret;

	map = mmap(NULL, BLOB_LEN, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (map == MAP_FAILED) {
		report("an address range crosses", false, strerror(errno));
		return;
	}
	memset(map, BLOB_BYTE, BLOB_LEN);

	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_va(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, ctlv_ptr(map),
			 BLOB_LEN);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, BLOB_BYTE);
	ret = ctlv_exec_full(fd, CTLV_TEST_OP_TRANSFER, tlv.buf,
			     ctlv_tlv_done(&tlv), reply, sizeof(reply));
	report_errno("an address range crosses", ret);

	ctlv_tlv_init(&tlv);
	ctlv_tlv_blob_va(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE,
			 ctlv_ptr(map) | beyond, BLOB_LEN);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, BLOB_BYTE);
	ret = ctlv_exec_full(fd, CTLV_TEST_OP_TRANSFER, tlv.buf,
			     ctlv_tlv_done(&tlv), reply, sizeof(reply));
	report("an address wider than this caller is refused", ret == -EFAULT,
	       ret ? strerror(-ret) : "it was answered");

	munmap(map, BLOB_LEN);
}

/* A caller of either ABI finds the attrs after the header in one place. */
static void an_event(int fd)
{
	const struct ctlv_event_hdr *hdr;
	char record[RECORD_LEN];
	struct ctlv_tlv tlv;
	ssize_t got;
	int ret;

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_OP_ID, CTLV_TEST_OP_BEEPED);
	ret = ctlv_exec_request(fd, CTLV_OP_SUBSCRIPTIONS_ADD, tlv.buf,
				ctlv_tlv_done(&tlv));
	if (ret) {
		report_errno("an event crosses", ret);
		return;
	}

	ctlv_tlv_init(&tlv);
	ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_COUNT, 1);
	ret = ctlv_exec_request(fd, CTLV_TEST_OP_PING, tlv.buf,
				ctlv_tlv_done(&tlv));
	if (ret) {
		report_errno("an event crosses", ret);
		return;
	}

	got = read(fd, record, sizeof(record));
	if (got < 0) {
		report("an event crosses", false, strerror(errno));
		return;
	}
	if ((size_t)got < sizeof(*hdr)) {
		report("an event crosses", false, "a headerless record");
		return;
	}
	hdr = (const struct ctlv_event_hdr *)record;
	report("an event crosses", hdr->length == (__u32)got &&
	       hdr->op_id == CTLV_TEST_OP_BEEPED,
	       hdr->op_id == CTLV_TEST_OP_BEEPED ? NULL : "another event");
}

int main(void)
{
	int fd;

	printf("a caller of %zu-bit pointers, %zu-byte exec\n",
	       sizeof(void *) * 8, sizeof(struct ctlv_op_exec));

	fd = ctlv_open(FAMILY, 0, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		printf("no %s device: %s\n", FAMILY, strerror(-fd));
		return EXIT_SKIP;
	}

	an_action(fd);
	a_query(fd);
	a_fragmented_query(fd);
	a_blob(fd);
	a_va_blob(fd);
	an_event(fd);

	close(fd);
	return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
