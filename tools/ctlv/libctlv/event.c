// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Asking for events, and reading the records that follow. A subscription is
 * per open file description, one read() returns one whole record, and a queue
 * that could not hold a record says so with a record of its own.
 */

#include <poll.h>
#include <unistd.h>

#include "internal.h"

/* Where a reader starts and how far it grows; a long record stays queued. */
#define EVENT_ROOM_MIN 512
#define EVENT_ROOM_MAX \
	(CTLV_MAX_INLINE_MESSAGE_LEN + (__u32)sizeof(struct ctlv_event_hdr))

/* Longest subscription request the core admits, from its own schema. */
#define SUBSCRIPTIONS_MAX 256

static const struct ctlv_event_hdr *event_hdr(const struct ctlv_event *event)
{
	return (const struct ctlv_event_hdr *)event->buf;
}

static int subscriptions(struct ctlv_dev *dev, __u64 op_id,
			 const __u64 *op_ids, unsigned int n,
			 struct ctlv_error *error)
{
	struct ctlv_msg *request;
	unsigned int i;
	int ret;

	if (!n || n > SUBSCRIPTIONS_MAX) {
		ctlv_error_set(error, -EINVAL);
		return -EINVAL;
	}
	ret = ctlv_msg_new(n * 32 + 32, &request);
	if (ret) {
		ctlv_error_set(error, ret);
		return ret;
	}
	for (i = 0; i < n; i++)
		ctlv_put_u64(request, CTLV_ATTR_EVENT_OP_ID, op_ids[i]);

	ret = ctlv_action(dev, op_id, request, NULL, error);
	ctlv_msg_free(request);
	return ret;
}

int ctlv_subscribe(struct ctlv_dev *dev, const __u64 *op_ids, unsigned int n,
		   struct ctlv_error *error)
{
	return subscriptions(dev, CTLV_OP_SUBSCRIPTIONS_ADD, op_ids, n, error);
}

int ctlv_unsubscribe(struct ctlv_dev *dev, const __u64 *op_ids, unsigned int n,
		     struct ctlv_error *error)
{
	return subscriptions(dev, CTLV_OP_SUBSCRIPTIONS_REMOVE, op_ids, n,
			     error);
}

int ctlv_queue_get(struct ctlv_dev *dev, __u64 *len, __u64 *used,
		   struct ctlv_error *error)
{
	const struct ctlv_attr *root, *attr;
	struct ctlv_reply *reply = NULL;
	int ret;

	ret = ctlv_query(dev, CTLV_OP_EVENT_QUEUE_GET, NULL, &reply, error);
	if (ret)
		return ret;

	root = ctlv_reply_root(reply);
	ret = -EPROTO;
	attr = ctlv_attr_find(root, CTLV_ATTR_EVENT_QUEUE_LEN);
	if (!attr || ctlv_get_u64(attr, len))
		goto out;
	attr = ctlv_attr_find(root, CTLV_ATTR_EVENT_QUEUE_USED);
	if (!attr || ctlv_get_u64(attr, used))
		goto out;
	ret = 0;
out:
	if (ret)
		ctlv_error_set(error, ret);
	ctlv_reply_free(reply);
	return ret;
}

int ctlv_queue_set(struct ctlv_dev *dev, __u64 len, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_msg_new(64, &request);
	if (ret) {
		ctlv_error_set(error, ret);
		return ret;
	}
	ctlv_put_u64(request, CTLV_ATTR_EVENT_QUEUE_LEN, len);

	ret = ctlv_action(dev, CTLV_OP_EVENT_QUEUE_SET, request, NULL, error);
	ctlv_msg_free(request);
	return ret;
}

void ctlv_event_free(struct ctlv_event *event)
{
	if (!event)
		return;
	free(event->buf);
	free(event);
}

int ctlv_event_read(struct ctlv_dev *dev, struct ctlv_event **eventp)
{
	struct ctlv_event *event;
	__u32 room = EVENT_ROOM_MIN;
	ssize_t got;
	int ret;

	event = calloc(1, sizeof(*event));
	if (!event)
		return -ENOMEM;

	for (;;) {
		__u8 *buf = realloc(event->buf, room);

		if (!buf) {
			ret = -ENOMEM;
			goto err;
		}
		event->buf = buf;
		got = read(dev->fd, event->buf, room);
		if (got >= 0)
			break;
		if (errno != EMSGSIZE || room == EVENT_ROOM_MAX) {
			ret = -errno;
			goto err;
		}
		room = room > EVENT_ROOM_MAX / 2 ? EVENT_ROOM_MAX : room * 2;
	}

	/*
	 * A record is its header and one root, the header saying how long the
	 * whole is. Reading anything else as a message would read past it.
	 */
	ret = -EPROTO;
	if ((size_t)got < sizeof(struct ctlv_event_hdr) + sizeof(struct ctlv_attr))
		goto err;
	event->len = got;
	if (event_hdr(event)->length != event->len)
		goto err;
	ret = ctlv_msg_check(event->buf + sizeof(struct ctlv_event_hdr),
			     event->len - sizeof(struct ctlv_event_hdr));
	if (ret)
		goto err;

	*eventp = event;
	return 0;
err:
	ctlv_event_free(event);
	return ret;
}

int ctlv_event_wait(struct ctlv_dev *dev, int timeout_ms)
{
	struct pollfd pollfd = { .fd = dev->fd, .events = POLLIN };
	int ret;

	do {
		ret = poll(&pollfd, 1, timeout_ms);
	} while (ret < 0 && errno == EINTR);
	if (ret < 0)
		return -errno;
	if (!ret)
		return -ETIMEDOUT;
	return 0;
}

__u64 ctlv_event_op_id(const struct ctlv_event *event)
{
	return event_hdr(event)->op_id;
}

__u64 ctlv_event_sequence(const struct ctlv_event *event)
{
	return event_hdr(event)->sequence;
}

const struct ctlv_attr *ctlv_event_root(const struct ctlv_event *event)
{
	return (const struct ctlv_attr *)(event->buf +
					  sizeof(struct ctlv_event_hdr));
}

/* One required u64 of a root-family record, absent or unreadable alike. */
static int record_u64(const struct ctlv_attr *root, __u64 attr_id, __u64 *value)
{
	const struct ctlv_attr *attr = ctlv_attr_find(root, attr_id);

	if (!attr || ctlv_get_u64(attr, value))
		return -EPROTO;
	return 0;
}

int ctlv_event_op_change(const struct ctlv_event *event,
			 struct ctlv_op_change *change)
{
	const struct ctlv_attr *root = ctlv_event_root(event);
	const struct ctlv_attr *attr;

	if (ctlv_event_op_id(event) != CTLV_OP_OP_CHANGED)
		return -EINVAL;
	if (record_u64(root, CTLV_ATTR_OP_ID, &change->op_id) ||
	    record_u64(root, CTLV_ATTR_OP_STATE_GENERATION,
		       &change->generation))
		return -EPROTO;
	attr = ctlv_attr_find(root, CTLV_ATTR_OP_DISABLED);
	if (!attr || ctlv_get_bool(attr, &change->disabled))
		return -EPROTO;
	return 0;
}

int ctlv_event_overflow(const struct ctlv_event *event,
			struct ctlv_overflow *overflow)
{
	const struct ctlv_attr *root = ctlv_event_root(event);

	if (ctlv_event_op_id(event) != CTLV_OP_EVENT_OVERFLOW)
		return -EINVAL;
	if (record_u64(root, CTLV_ATTR_EVENT_FIRST_SEQUENCE,
		       &overflow->first_sequence) ||
	    record_u64(root, CTLV_ATTR_EVENT_LAST_SEQUENCE,
		       &overflow->last_sequence) ||
	    record_u64(root, CTLV_ATTR_EVENT_DROPPED_COUNT,
		       &overflow->dropped_count))
		return -EPROTO;
	return 0;
}
