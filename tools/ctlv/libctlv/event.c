// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Event subscription and reading.
 */

#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "internal.h"

/* Where a reader starts; an event too long for it stays queued. */
#define EVENT_ROOM_MIN 512

/* Longest subscription request the core admits, from its own schema. */
#define SUBSCRIPTIONS_MAX 256

static const struct ctlv_event_hdr *event_hdr(const struct ctlv_event *event)
{
	return (const struct ctlv_event_hdr *)event->buf;
}

/*
 * The file of a device in another subsystem reads as that subsystem says,
 * so events are read from an event fd, as blocking as the device's file.
 */
static int event_fd(struct ctlv_dev *dev)
{
	int flags, fd, ret;

	if (dev->event_fd >= 0)
		return dev->event_fd;
	flags = fcntl(dev->fd, F_GETFL);
	if (flags < 0)
		return -errno;
	fd = ioctl(dev->fd, CTLV_IOC_EVENT_FD, 0);
	if (fd < 0)
		return -errno;
	if ((flags & O_NONBLOCK) && fcntl(fd, F_SETFL, O_NONBLOCK)) {
		ret = -errno;
		close(fd);
		return ret;
	}
	dev->event_fd = fd;
	return fd;
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
		ctlv_put_u64(request, CTLV_T_EVENT_SUBSCRIPTION_OP_IDS,
			     op_ids[i]);

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

int ctlv_queue_get(struct ctlv_dev *dev, struct ctlv_queue *queue,
		   struct ctlv_error *error)
{
	const struct ctlv_hdr *root, *hdr;
	struct ctlv_reply *reply = NULL;
	int ret;

	*queue = (struct ctlv_queue){};
	ret = ctlv_query(dev, CTLV_OP_EVENT_QUEUE_GET, NULL, &reply, error);
	if (ret)
		return ret;

	root = ctlv_reply_root(reply);
	ret = -EPROTO;
	hdr = ctlv_hdr_find(root, CTLV_T_EVENT_QUEUE_GET_QUEUE_LEN);
	if (!hdr || ctlv_get_u64(hdr, &queue->len))
		goto out;
	hdr = ctlv_hdr_find(root, CTLV_T_EVENT_QUEUE_GET_QUEUE_USED);
	if (!hdr || ctlv_get_u64(hdr, &queue->used))
		goto out;
	hdr = ctlv_hdr_find(root, CTLV_T_EVENT_QUEUE_GET_QUEUE_MIN_LEN);
	if (hdr && ctlv_get_u64(hdr, &queue->min_len))
		goto out;
	hdr = ctlv_hdr_find(root, CTLV_T_EVENT_QUEUE_GET_QUEUE_MAX_LEN);
	if (hdr && ctlv_get_u64(hdr, &queue->max_len))
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
	ctlv_put_u64(request, CTLV_T_EVENT_QUEUE_SET_QUEUE_LEN, len);

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

/*
 * How far a reader grows: the longest event of any op. A device already
 * gone cannot say, and its queue is still read, so then only a read ends it.
 */
static __u32 event_room_max(struct ctlv_dev *dev)
{
	const struct ctlv_ops *sizes = ctlv_dev_sizes(dev);
	__u64 room = 0;
	unsigned int i;

	if (!sizes)
		return UINT32_MAX;
	for (i = 0; i < sizes->ops_count; i++)
		if (room < sizes->ops[i].event_max_len)
			room = sizes->ops[i].event_max_len;
	return room < UINT32_MAX ? room : UINT32_MAX;
}

int ctlv_event_read(struct ctlv_dev *dev, struct ctlv_event **eventp)
{
	__u32 room = EVENT_ROOM_MIN, room_max = 0;
	struct ctlv_event *event;
	int fd = event_fd(dev);
	ssize_t got;
	int ret;

	if (fd < 0)
		return fd;
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
		got = read(fd, event->buf, room);
		if (got >= 0)
			break;
		ret = -errno;
		if (ret != -EMSGSIZE)
			goto err;
		if (!room_max)
			room_max = event_room_max(dev);
		if (room >= room_max)
			goto err;
		room = room > room_max / 2 ? room_max : room * 2;
	}

	/*
	 * An event is its header and one root, the header saying how long the
	 * whole is. Reading anything else as a message would read past it.
	 */
	ret = -EPROTO;
	if ((size_t)got <
	    sizeof(struct ctlv_event_hdr) + sizeof(struct ctlv_hdr))
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
	struct pollfd pollfd = { .fd = event_fd(dev), .events = POLLIN };
	int ret;

	if (pollfd.fd < 0)
		return pollfd.fd;
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

const struct ctlv_hdr *ctlv_event_root(const struct ctlv_event *event)
{
	return (const struct ctlv_hdr *)(event->buf +
					 sizeof(struct ctlv_event_hdr));
}

/* One required u64 of a root-family event, absent or unreadable alike. */
static int event_u64(const struct ctlv_hdr *root, __u64 type, __u64 *value)
{
	const struct ctlv_hdr *hdr = ctlv_hdr_find(root, type);

	if (!hdr || ctlv_get_u64(hdr, value))
		return -EPROTO;
	return 0;
}

int ctlv_event_op_change(const struct ctlv_event *event,
			 struct ctlv_op_change *change)
{
	const struct ctlv_hdr *root = ctlv_event_root(event);
	const struct ctlv_hdr *hdr;

	if (ctlv_event_op_id(event) != CTLV_OP_OP_CHANGED)
		return -EINVAL;
	if (event_u64(root, CTLV_T_OP_CHANGED_ID, &change->op_id) ||
	    event_u64(root, CTLV_T_OP_CHANGED_STATE_GENERATION,
		      &change->generation))
		return -EPROTO;
	hdr = ctlv_hdr_find(root, CTLV_T_OP_CHANGED_DISABLED);
	if (!hdr || ctlv_get_bool(hdr, &change->disabled))
		return -EPROTO;
	return 0;
}

int ctlv_event_overflow(const struct ctlv_event *event,
			struct ctlv_overflow *overflow)
{
	const struct ctlv_hdr *root = ctlv_event_root(event);

	if (ctlv_event_op_id(event) != CTLV_OP_EVENT_OVERFLOW)
		return -EINVAL;
	if (event_u64(root, CTLV_T_EVENT_OVERFLOW_FIRST_SEQUENCE,
		      &overflow->first_sequence) ||
	    event_u64(root, CTLV_T_EVENT_OVERFLOW_LAST_SEQUENCE,
		      &overflow->last_sequence) ||
	    event_u64(root, CTLV_T_EVENT_OVERFLOW_DROPPED_COUNT,
		      &overflow->dropped_count))
		return -EPROTO;
	return 0;
}
