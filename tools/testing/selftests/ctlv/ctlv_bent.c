// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Seeded message mutation with reply canaries and device-liveness checks.
 * Run with KASAN, lockdep, and panic_on_warn.
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>

#include <linux/ctlv_test_gen.h>

#include "../kselftest_harness.h"
#include "ctlv_tlv.h"
#include "ctlv_util.h"

#define FAMILY "ctlv-test"

/* Enough rounds to bend every message several ways, and quick enough for CI. */
#define BENT_ROUNDS 4000

/* What follows a reply buffer, so a write past its end is caught here. */
#define BENT_CANARY 0x5a
#define BENT_CANARY_LEN 64

/* The largest reply any round offers, and the largest request it sends. */
#define BENT_REPLY_LEN 4096
#define BENT_REQUEST_LEN CTLV_TLV_LEN

/* More fragments than any answer here needs, however small the buffer. */
#define BENT_MAX_FRAGS 4096

/* One message of the corpus: a valid one, and the op it belongs to. */
struct valid_msg {
	const char *what;
	__u64 op_id;
	void (*build)(struct ctlv_tlv *tlv);
};

static void msg_nothing(struct ctlv_tlv *tlv)
{
}

static void msg_token(struct ctlv_tlv *tlv)
{
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_PROBE_TOKEN, 7);
}

static void msg_series(struct ctlv_tlv *tlv)
{
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_CHUNKS, 3);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_ITEMS, 3);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_VALUES, 3);
}

static void msg_burst(struct ctlv_tlv *tlv)
{
	static const char filler[64] = {};

	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_PING_COUNT, 2);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_PING_SERIAL, 5);
	ctlv_tlv_bin(tlv, CTLV_TEST_ATTR_PING_FILLER, filler, sizeof(filler));
}

static void msg_nest(struct ctlv_tlv *tlv)
{
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_SERIES_CHUNK);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_INDEX, 1);
	ctlv_tlv_nest_start(tlv, CTLV_TEST_ATTR_SERIES_ITEM);
	ctlv_tlv_u64(tlv, CTLV_TEST_ATTR_SERIES_VALUE, 2);
	ctlv_tlv_nest_end(tlv);
	ctlv_tlv_nest_end(tlv);
}

static void msg_subscription(struct ctlv_tlv *tlv)
{
	ctlv_tlv_u64(tlv, CTLV_ATTR_EVENT_OP_ID, CTLV_TEST_OP_BEEPED);
}

static void msg_queue(struct ctlv_tlv *tlv)
{
	ctlv_tlv_u64(tlv, CTLV_ATTR_EVENT_QUEUE_LEN, 1 << 16);
}

static void msg_schema(struct ctlv_tlv *tlv)
{
	ctlv_tlv_u64(tlv, CTLV_ATTR_OP_ID, CTLV_TEST_OP_PROBE_GET);
}

static const struct valid_msg valid_corpus[] = {
	{ "an ack", CTLV_TEST_OP_NOOP, msg_nothing },
	{ "a query", CTLV_TEST_OP_PROBE_GET, msg_token },
	{ "a fragmented query", CTLV_TEST_OP_SERIES_DUMP, msg_series },
	{ "a burst", CTLV_TEST_OP_PING, msg_burst },
	{ "a nest", CTLV_TEST_OP_SERIES_DUMP, msg_nest },
	{ "a subscription", CTLV_OP_SUBSCRIPTIONS_ADD, msg_subscription },
	{ "a queue", CTLV_OP_EVENT_QUEUE_SET, msg_queue },
	{ "a schema", CTLV_OP_OP_SCHEMA_GET, msg_schema },
	{ "a dump", CTLV_OP_OPS_DUMP, msg_nothing },
	{ "a chain", CTLV_OP_FAMILIES_DUMP, msg_nothing },
};

/* Lengths a field is most likely to break the parser with. */
static const __u32 edge_lens[] = {
	0, 1, 7, 8, 15, 16, 17, 24, 0xffff, 0x10000,
	CTLV_MAX_INLINE_MESSAGE_LEN, CTLV_MAX_INLINE_MESSAGE_LEN + 1,
	0x7fffffff, 0x80000000, 0xfffffff8, 0xffffffff,
};

/* A generator whose whole state is the seed, so a round is reproducible. */
static __u64 rnd_next(__u64 *state)
{
	__u64 x = *state;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*state = x;
	return x;
}

static __u32 rnd_below(__u64 *state, __u32 bound)
{
	return bound ? rnd_next(state) % bound : 0;
}

FIXTURE(bent) {
	int fd;
	__u64 state;
	unsigned int rounds;
	/* One request being bent, and one reply with its canary behind it. */
	char request[BENT_REQUEST_LEN];
	char reply[BENT_REPLY_LEN + BENT_CANARY_LEN];
};

FIXTURE_SETUP(bent)
{
	const char *env;

	/* Never blocking, since a round reads a queue that is often empty. */
	self->fd = ctlv_open(FAMILY, 0, O_RDWR | O_NONBLOCK);
	if (self->fd < 0)
		SKIP(return, "no %s device: %s", FAMILY,
		     strerror(-self->fd));

	env = getenv("CTLV_BENT_SEED");
	self->state = env ? strtoull(env, NULL, 0) : 0x243f6a8885a308d3ULL;
	/* Zero would leave the generator where it started, forever. */
	if (!self->state)
		self->state = 1;
	env = getenv("CTLV_BENT_ROUNDS");
	self->rounds = env ? strtoul(env, NULL, 0) : BENT_ROUNDS;
	TH_LOG("seed %#llx, %u rounds",
	       (unsigned long long)self->state, self->rounds);
}

FIXTURE_TEARDOWN(bent)
{
	if (self->fd >= 0)
		close(self->fd);
}

/* A reply buffer with a canary behind it, fresh for one round. */
static void bent_reply_init(char *reply, __u32 len)
{
	memset(reply, 0, len);
	memset(reply + len, BENT_CANARY, BENT_CANARY_LEN);
}

static bool bent_canary_intact(const char *reply, __u32 len)
{
	__u32 at;

	for (at = 0; at < BENT_CANARY_LEN; at++)
		if ((unsigned char)reply[len + at] != BENT_CANARY)
			return false;
	return true;
}

/*
 * One bend of a valid message. Returns the length to submit, which is not
 * always the length the message says it is.
 */
static __u32 bend(__u64 *state, char *buf, __u32 len)
{
	struct ctlv_attr *root = (struct ctlv_attr *)buf;
	struct ctlv_attr *attr;
	__u32 at;

	switch (rnd_below(state, 8)) {
	case 0:
		/* One bit of one byte, like a corrupt copy. */
		at = rnd_below(state, len);
		buf[at] ^= 1 << rnd_below(state, 8);
		break;
	case 1:
		/* A length the root could not have. */
		root->len = edge_lens[rnd_below(state,
						ARRAY_SIZE(edge_lens))];
		break;
	case 2:
		/* Flags nothing declares. */
		root->flags = (__u32)rnd_next(state);
		break;
	case 3:
		/* An ID of nobody's, on the root or on a child. */
		attr = root;
		if (len > sizeof(*root) + sizeof(*attr) &&
		    rnd_below(state, 2))
			attr = (struct ctlv_attr *)(buf + sizeof(*root));
		attr->attr_id = rnd_next(state);
		break;
	case 4:
		/* A child claiming a length of its own choosing. */
		if (len <= sizeof(*root) + sizeof(*attr))
			break;
		attr = (struct ctlv_attr *)(buf + sizeof(*root));
		attr->len = edge_lens[rnd_below(state,
						ARRAY_SIZE(edge_lens))];
		break;
	case 5:
		/* A child that says it is a nest, or says it is not. */
		if (len <= sizeof(*root) + sizeof(*attr))
			break;
		attr = (struct ctlv_attr *)(buf + sizeof(*root));
		attr->flags ^= CTLV_ATTR_F_NESTED;
		break;
	case 6:
		/* Less of the message than the message says there is. */
		return rnd_below(state, len + 1);
	case 7: {
		/* A whole word of payload, where there is any. */
		__u64 word = rnd_next(state);

		if (len <= sizeof(*root) + sizeof(*attr))
			break;
		at = sizeof(*root) + sizeof(*attr);
		if (at + sizeof(word) <= len)
			memcpy(buf + at, &word, sizeof(word));
		break;
		}
	}
	return len;
}

/* The device answers something it was never asked to bend. */
static int bent_alive(int fd)
{
	return ctlv_exec_bare(fd, CTLV_TEST_OP_NOOP);
}

/* Requests of every op of the corpus, each bent one way per round. */
TEST_F(bent, a_request)
{
	unsigned int round;

	for (round = 0; round < self->rounds; round++) {
		const struct valid_msg *msg;
		struct ctlv_op_exec exec = {};
		struct ctlv_tlv tlv;
		__u32 len;

		msg = &valid_corpus[rnd_below(&self->state,
					      ARRAY_SIZE(valid_corpus))];
		ctlv_tlv_init(&tlv);
		msg->build(&tlv);
		len = ctlv_tlv_done(&tlv);
		memcpy(self->request, tlv.buf, len);
		len = bend(&self->state, self->request, len);

		bent_reply_init(self->reply, BENT_REPLY_LEN);
		exec.op_id = msg->op_id;
		exec.request = ctlv_ptr(self->request);
		exec.request_len = len;
		exec.reply = ctlv_ptr(self->reply);
		exec.reply_len = BENT_REPLY_LEN;
		ctlv_exec(self->fd, &exec);

		ASSERT_TRUE(bent_canary_intact(self->reply, BENT_REPLY_LEN)) {
			TH_LOG("round %u: wrote past the reply buffer, %s",
			       round, msg->what);
		}
		ASSERT_EQ(0, bent_alive(self->fd)) {
			TH_LOG("round %u: device stopped answering, %s",
			       round, msg->what);
		}
	}
}

/* A root header of the caller's invention followed by noise. */
TEST_F(bent, arbitrary_bytes)
{
	unsigned int round;

	for (round = 0; round < self->rounds; round++) {
		struct ctlv_attr *root = (struct ctlv_attr *)self->request;
		struct ctlv_op_exec exec = {};
		__u32 len, at;
		__u64 word;

		len = rnd_below(&self->state, 256) + sizeof(*root);
		for (at = 0; at < len; at += sizeof(word)) {
			word = rnd_next(&self->state);
			memcpy(self->request + at, &word, sizeof(word));
		}
		/* Half the rounds keep a plausible root, half do not. */
		if (rnd_below(&self->state, 2)) {
			root->len = len;
			root->flags = CTLV_ATTR_F_NESTED;
			root->attr_id = CTLV_ATTR_ROOT;
		}

		bent_reply_init(self->reply, BENT_REPLY_LEN);
		exec.op_id = rnd_below(&self->state, 2) ?
			     CTLV_TEST_OP_PING : CTLV_TEST_OP_PROBE_GET;
		exec.request = ctlv_ptr(self->request);
		exec.request_len = len;
		exec.reply = ctlv_ptr(self->reply);
		exec.reply_len = BENT_REPLY_LEN;
		ctlv_exec(self->fd, &exec);

		ASSERT_TRUE(bent_canary_intact(self->reply, BENT_REPLY_LEN)) {
			TH_LOG("round %u: wrote past the reply buffer", round);
		}
		ASSERT_EQ(0, bent_alive(self->fd)) {
			TH_LOG("round %u: device stopped answering", round);
		}
	}
}

/*
 * Reply buffers of every awkward size, including sizes too small for a
 * fragment: the serializer's own accounting rather than the parser's.
 */
TEST_F(bent, a_reply_buffer_of_any_size)
{
	unsigned int round;

	for (round = 0; round < self->rounds; round++) {
		struct ctlv_op_exec exec = {};
		__u32 reply_len, frags;
		struct ctlv_tlv tlv;

		ctlv_tlv_init(&tlv);
		msg_series(&tlv);
		reply_len = rnd_below(&self->state, BENT_REPLY_LEN + 1);
		bent_reply_init(self->reply, reply_len);

		exec.op_id = CTLV_TEST_OP_SERIES_DUMP;
		exec.request = ctlv_ptr(tlv.buf);
		exec.request_len = ctlv_tlv_done(&tlv);
		exec.reply = ctlv_ptr(self->reply);
		exec.reply_len = reply_len;
		/* Bounded, so a device stuck on one fragment fails here. */
		for (frags = 0; frags < BENT_MAX_FRAGS; frags++) {
			if (ctlv_exec(self->fd, &exec) || !exec.cursor)
				break;
			ASSERT_TRUE(bent_canary_intact(self->reply,
						       reply_len)) {
				TH_LOG("round %u: wrote past %u bytes",
				       round, reply_len);
			}
			bent_reply_init(self->reply, reply_len);
		}
		ASSERT_GT(BENT_MAX_FRAGS, frags) {
			TH_LOG("round %u: %u fragments into %u bytes", round,
			       frags, reply_len);
		}

		ASSERT_TRUE(bent_canary_intact(self->reply, reply_len)) {
			TH_LOG("round %u: wrote past %u bytes of reply",
			       round, reply_len);
		}
		ASSERT_EQ(0, bent_alive(self->fd)) {
			TH_LOG("round %u: device stopped answering", round);
		}
	}
}

/* Descriptors nothing can resolve: the resolver refuses, acquiring nothing. */
TEST_F(bent, a_blob_descriptor)
{
	unsigned int round;
	int blob;

	blob = memfd_create("ctlv-bent", MFD_CLOEXEC);
	ASSERT_LE(0, blob);
	ASSERT_EQ(0, ftruncate(blob, 1 << 16));

	for (round = 0; round < self->rounds; round++) {
		struct ctlv_op_exec exec = {};
		__u64 length, offset, type, which;
		struct ctlv_tlv tlv;
		__u32 len;

		/*
		 * Each draw in its own statement: argument evaluation order is
		 * unspecified and a round has to come out the same way twice.
		 */
		type = rnd_below(&self->state, 4);
		which = rnd_next(&self->state);
		if (rnd_below(&self->state, 2))
			which = blob;
		offset = rnd_next(&self->state);
		if (rnd_below(&self->state, 4))
			offset = 0;
		length = rnd_next(&self->state);
		if (rnd_below(&self->state, 4))
			length = 1 << 16;

		ctlv_tlv_init(&tlv);
		ctlv_tlv_blob_fd(&tlv, CTLV_TEST_ATTR_TRANSFER_SOURCE, type,
				 which, offset, length);
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_TRANSFER_FILL, 0);
		len = ctlv_tlv_done(&tlv);
		memcpy(self->request, tlv.buf, len);
		/* And every so often, bent on top of that. */
		if (!rnd_below(&self->state, 4))
			len = bend(&self->state, self->request, len);

		bent_reply_init(self->reply, BENT_REPLY_LEN);
		exec.op_id = CTLV_TEST_OP_TRANSFER;
		exec.request = ctlv_ptr(self->request);
		exec.request_len = len;
		exec.reply = ctlv_ptr(self->reply);
		exec.reply_len = BENT_REPLY_LEN;
		ctlv_exec(self->fd, &exec);

		ASSERT_TRUE(bent_canary_intact(self->reply, BENT_REPLY_LEN)) {
			TH_LOG("round %u: wrote past the reply buffer", round);
		}
		ASSERT_EQ(0, bent_alive(self->fd)) {
			TH_LOG("round %u: device stopped answering", round);
		}
	}
	close(blob);
}

/* The event path at random: any ID, any queue length, any read size. */
TEST_F(bent, an_event_path_at_random)
{
	unsigned int round;

	for (round = 0; round < self->rounds; round++) {
		__u64 op_id, queue_len, count;
		struct ctlv_tlv tlv;
		char record[512];
		__u32 room;

		/* Subscribing to an event, or to something that is not one. */
		op_id = rnd_next(&self->state);
		if (rnd_below(&self->state, 2))
			op_id = CTLV_TEST_OP_BEEPED;
		ctlv_tlv_init(&tlv);
		ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_OP_ID, op_id);
		ctlv_exec_request(self->fd, rnd_below(&self->state, 2) ?
					    CTLV_OP_SUBSCRIPTIONS_ADD :
					    CTLV_OP_SUBSCRIPTIONS_REMOVE,
				  tlv.buf, ctlv_tlv_done(&tlv));

		queue_len = rnd_next(&self->state);
		if (rnd_below(&self->state, 2))
			queue_len = 1 << 16;
		ctlv_tlv_init(&tlv);
		ctlv_tlv_u64(&tlv, CTLV_ATTR_EVENT_QUEUE_LEN, queue_len);
		ctlv_exec_request(self->fd, CTLV_OP_EVENT_QUEUE_SET, tlv.buf,
				  ctlv_tlv_done(&tlv));

		count = rnd_below(&self->state, 8);
		ctlv_tlv_init(&tlv);
		ctlv_tlv_u64(&tlv, CTLV_TEST_ATTR_PING_COUNT, count);
		ctlv_exec_request(self->fd, CTLV_TEST_OP_PING, tlv.buf,
				  ctlv_tlv_done(&tlv));

		/* A read that may lack room, of a queue that may be empty. */
		room = rnd_below(&self->state, sizeof(record) + 1);
		read(self->fd, record, room);

		ASSERT_EQ(0, bent_alive(self->fd)) {
			TH_LOG("round %u: device stopped answering", round);
		}
	}
}

TEST_HARNESS_MAIN
