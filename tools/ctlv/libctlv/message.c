// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Writing one CTLV message and reading one back. A writer appends and keeps
 * the first thing that did not fit, so a request is a run of calls with one
 * check after it. A reader bounds every step by the enclosing nest, what it
 * walks having come from a device.
 */

#include <stdint.h>

#include "internal.h"

/* The wire alignment applied to a length that may be close to overflowing. */
static int aligned(__u32 len, __u32 *out)
{
	if (len > UINT32_MAX - (CTLV_ALIGNTO - 1))
		return -EMSGSIZE;
	*out = CTLV_ALIGN(len);
	return 0;
}

static int msg_room(struct ctlv_msg *msg, __u32 need)
{
	__u32 capacity = msg->capacity;
	__u8 *buf;

	if (need > msg->max - msg->len)
		return -EMSGSIZE;
	need += msg->len;
	if (need <= capacity)
		return 0;
	while (capacity < need) {
		if (capacity > msg->max / 2) {
			capacity = msg->max;
			break;
		}
		capacity *= 2;
	}
	buf = realloc(msg->buf, capacity);
	if (!buf)
		return -ENOMEM;
	msg->buf = buf;
	msg->capacity = capacity;
	return 0;
}

/* Zeroes up to the next attr boundary; a message's padding is zero. */
static int msg_pad(struct ctlv_msg *msg)
{
	__u32 padded;
	int ret;

	ret = aligned(msg->len, &padded);
	if (ret)
		return ret;
	ret = msg_room(msg, padded - msg->len);
	if (ret)
		return ret;
	memset(msg->buf + msg->len, 0, padded - msg->len);
	msg->len = padded;
	return 0;
}

static struct ctlv_attr *msg_attr(struct ctlv_msg *msg, __u32 offset)
{
	return (struct ctlv_attr *)(msg->buf + offset);
}

/* One attr at the next boundary; a writer holding an error appends none. */
int ctlv_msg_put(struct ctlv_msg *msg, __u64 attr_id, __u32 flags,
		 const void *payload, __u32 len)
{
	struct ctlv_attr attr = {
		.flags = flags,
		.attr_id = attr_id,
	};
	int ret;

	if (msg->err)
		return msg->err;
	if (len > UINT32_MAX - sizeof(attr)) {
		msg->err = -EMSGSIZE;
		return msg->err;
	}
	attr.len = sizeof(attr) + len;

	ret = msg_pad(msg);
	if (!ret)
		ret = msg_room(msg, attr.len);
	if (ret) {
		msg->err = ret;
		return ret;
	}
	memcpy(msg->buf + msg->len, &attr, sizeof(attr));
	msg->len += sizeof(attr);
	if (len) {
		memcpy(msg->buf + msg->len, payload, len);
		msg->len += len;
	}
	return 0;
}

int ctlv_msg_copy(struct ctlv_msg *msg, const struct ctlv_attr *attr)
{
	return ctlv_msg_put(msg, attr->attr_id,
			    attr->flags & ~CTLV_ATTR_F_FRAG_MORE, attr + 1,
			    attr->len - sizeof(*attr));
}

int ctlv_msg_new_max(__u32 capacity, __u32 max, struct ctlv_msg **msgp)
{
	struct ctlv_msg *msg;

	if (!capacity || capacity > max)
		return -EINVAL;
	msg = calloc(1, sizeof(*msg));
	if (!msg)
		return -ENOMEM;
	/* Separate from the handle, so growing the buffer does not move it. */
	msg->buf = malloc(capacity);
	if (!msg->buf) {
		free(msg);
		return -ENOMEM;
	}
	msg->capacity = capacity;
	msg->max = max;
	*msgp = msg;
	return ctlv_nest_start(msg, CTLV_ATTR_ID_ROOT);
}

int ctlv_msg_new(__u32 capacity, struct ctlv_msg **msgp)
{
	return ctlv_msg_new_max(capacity, CTLV_MAX_INLINE_MESSAGE_LEN, msgp);
}

void ctlv_msg_free(struct ctlv_msg *msg)
{
	if (!msg)
		return;
	free(msg->buf);
	free(msg);
}

void ctlv_msg_reset(struct ctlv_msg *msg)
{
	msg->len = 0;
	msg->depth = 0;
	msg->err = 0;
	ctlv_nest_start(msg, CTLV_ATTR_ID_ROOT);
}

int ctlv_msg_err(const struct ctlv_msg *msg)
{
	return msg->err;
}

int ctlv_nest_start(struct ctlv_msg *msg, __u64 attr_id)
{
	__u32 offset;
	int ret;

	if (msg->err)
		return msg->err;
	if (msg->depth == CTLV_DEPTH_MAX) {
		msg->err = -ELOOP;
		return msg->err;
	}
	/* Pad first, so the recorded offset is where the header lands. */
	ret = msg_pad(msg);
	if (ret) {
		msg->err = ret;
		return ret;
	}
	offset = msg->len;
	ret = ctlv_msg_put(msg, attr_id, CTLV_ATTR_F_NESTED, NULL, 0);
	if (ret)
		return ret;
	msg->open[msg->depth++] = offset;
	return 0;
}

/* A nest counts the padding of its last child, so a walk steps aligned. */
int ctlv_nest_end(struct ctlv_msg *msg)
{
	int ret;

	if (msg->err)
		return msg->err;
	if (!msg->depth) {
		msg->err = -EINVAL;
		return msg->err;
	}
	ret = msg_pad(msg);
	if (ret) {
		msg->err = ret;
		return ret;
	}
	msg->depth--;
	msg_attr(msg, msg->open[msg->depth])->len =
		msg->len - msg->open[msg->depth];
	return 0;
}

int ctlv_msg_done(struct ctlv_msg *msg)
{
	while (msg->depth && !msg->err)
		ctlv_nest_end(msg);
	return msg->err;
}

const struct ctlv_attr *ctlv_msg_root(const struct ctlv_msg *msg)
{
	return (const struct ctlv_attr *)msg->buf;
}

__u32 ctlv_msg_len(const struct ctlv_msg *msg)
{
	return msg->len;
}

int ctlv_put_u64(struct ctlv_msg *msg, __u64 attr_id, __u64 value)
{
	return ctlv_msg_put(msg, attr_id, 0, &value, sizeof(value));
}

int ctlv_put_s64(struct ctlv_msg *msg, __u64 attr_id, __s64 value)
{
	return ctlv_msg_put(msg, attr_id, 0, &value, sizeof(value));
}

int ctlv_put_bool(struct ctlv_msg *msg, __u64 attr_id, bool value)
{
	__u8 byte = value;

	return ctlv_msg_put(msg, attr_id, 0, &byte, sizeof(byte));
}

/* The terminator is part of the payload, and a declared length excludes it. */
int ctlv_put_str(struct ctlv_msg *msg, __u64 attr_id, const char *value)
{
	size_t len = strlen(value) + 1;

	if (len > UINT32_MAX)
		return -EMSGSIZE;
	return ctlv_msg_put(msg, attr_id, 0, value, len);
}

int ctlv_put_bin(struct ctlv_msg *msg, __u64 attr_id, const void *value,
		 __u32 len)
{
	return ctlv_msg_put(msg, attr_id, 0, value, len);
}

/* A blob attr carries a nest of framework members, chosen by the backing. */
int ctlv_put_blob_va(struct ctlv_msg *msg, __u64 attr_id, const void *address,
		     __u64 length)
{
	ctlv_nest_start(msg, attr_id);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_ADDRESS, (__u64)(uintptr_t)address);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_LENGTH, length);
	return ctlv_nest_end(msg);
}

static int put_blob_fd(struct ctlv_msg *msg, __u64 attr_id, __u64 type, int fd,
		       __u64 offset, __u64 length)
{
	if (fd < 0)
		return -EBADF;
	ctlv_nest_start(msg, attr_id);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_TYPE, type);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_FD, fd);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_OFFSET, offset);
	ctlv_put_u64(msg, CTLV_ATTR_BLOB_LENGTH, length);
	return ctlv_nest_end(msg);
}

int ctlv_put_blob_memfd(struct ctlv_msg *msg, __u64 attr_id, int fd,
			__u64 offset, __u64 length)
{
	return put_blob_fd(msg, attr_id, CTLV_BLOB_TYPE_MEMFD, fd, offset,
			   length);
}

int ctlv_put_blob_dma_buf(struct ctlv_msg *msg, __u64 attr_id, int fd,
			  __u64 offset, __u64 length)
{
	return put_blob_fd(msg, attr_id, CTLV_BLOB_TYPE_DMA_BUF, fd, offset,
			   length);
}

bool ctlv_attr_is_nest(const struct ctlv_attr *attr)
{
	return attr->flags & CTLV_ATTR_F_NESTED;
}

__u32 ctlv_attr_payload_len(const struct ctlv_attr *attr)
{
	return attr->len - sizeof(*attr);
}

const void *ctlv_attr_payload(const struct ctlv_attr *attr, __u32 *len)
{
	if (len)
		*len = ctlv_attr_payload_len(attr);
	return attr + 1;
}

const struct ctlv_attr *ctlv_attr_first(const struct ctlv_attr *nest)
{
	if (nest->len <= sizeof(*nest))
		return NULL;
	/* A nest too short to hold the header it announces holds no child. */
	if (nest->len - sizeof(*nest) < sizeof(*nest))
		return NULL;
	return nest + 1;
}

/*
 * The next child is a step of this one's aligned length. Both the step and the
 * header it lands on have to stay inside the nest.
 */
const struct ctlv_attr *ctlv_attr_next(const struct ctlv_attr *nest,
				       const struct ctlv_attr *child)
{
	const char *end = (const char *)nest + nest->len;
	const char *next = (const char *)child;
	__u32 step;

	if (aligned(child->len, &step))
		return NULL;
	next += step;
	if (next < (const char *)child || next > end)
		return NULL;
	if ((size_t)(end - next) < sizeof(*child))
		return NULL;
	return (const struct ctlv_attr *)next;
}

const struct ctlv_attr *ctlv_attr_find(const struct ctlv_attr *nest,
				       __u64 attr_id)
{
	const struct ctlv_attr *child;

	ctlv_for_each(child, nest)
		if (child->attr_id == attr_id)
			return child;
	return NULL;
}

unsigned int ctlv_attr_count(const struct ctlv_attr *nest, __u64 attr_id)
{
	const struct ctlv_attr *child;
	unsigned int n = 0;

	ctlv_for_each(child, nest)
		if (child->attr_id == attr_id)
			n++;
	return n;
}

/*
 * Framing, checked before anything reads it: a message that fails this is not
 * walked, which is what lets the walkers above trust the lengths they step.
 */
static int check_nest(const struct ctlv_attr *nest, unsigned int depth)
{
	const char *p = (const char *)(nest + 1);
	const char *end = (const char *)nest + nest->len;

	if (depth >= CTLV_DEPTH_MAX)
		return -ELOOP;
	while (p < end) {
		const struct ctlv_attr *child = (const struct ctlv_attr *)p;
		__u32 step;
		int ret;

		if ((size_t)(end - p) < sizeof(*child))
			return -EBADMSG;
		if (child->len < sizeof(*child))
			return -EBADMSG;
		if (child->len > (__u32)(end - p))
			return -EBADMSG;
		if (child->flags & CTLV_ATTR_F_NESTED) {
			ret = check_nest(child, depth + 1);
			if (ret)
				return ret;
		}
		ret = aligned(child->len, &step);
		if (ret)
			return ret;
		/* Padding is in the nest, so the step lands on its end. */
		if (step > (__u32)(end - p))
			return -EBADMSG;
		p += step;
	}
	return 0;
}

int ctlv_msg_check(const void *buf, __u32 len)
{
	const struct ctlv_attr *root = buf;

	if (len < sizeof(*root))
		return -EBADMSG;
	if (root->attr_id != CTLV_ATTR_ID_ROOT)
		return -EBADMSG;
	if (!(root->flags & CTLV_ATTR_F_NESTED))
		return -EBADMSG;
	if (root->len < sizeof(*root) || root->len > len)
		return -EBADMSG;
	return check_nest(root, 0);
}

/*
 * A scalar payload is eight bytes on the wire whatever it holds, so another
 * width is a corrupt message rather than something to interpret.
 */
static int get_scalar(const struct ctlv_attr *attr, __u64 *value)
{
	__u64 raw;

	if (ctlv_attr_is_nest(attr))
		return -EINVAL;
	if (ctlv_attr_payload_len(attr) != sizeof(raw))
		return -EBADMSG;
	memcpy(&raw, attr + 1, sizeof(raw));
	*value = raw;
	return 0;
}

int ctlv_get_u64(const struct ctlv_attr *attr, __u64 *value)
{
	return get_scalar(attr, value);
}

int ctlv_get_s64(const struct ctlv_attr *attr, __s64 *value)
{
	__u64 raw;
	int ret;

	ret = get_scalar(attr, &raw);
	if (ret)
		return ret;
	*value = (__s64)raw;
	return 0;
}

int ctlv_get_bool(const struct ctlv_attr *attr, bool *value)
{
	const __u8 *byte;

	if (ctlv_attr_is_nest(attr))
		return -EINVAL;
	if (ctlv_attr_payload_len(attr) != sizeof(*byte))
		return -EBADMSG;
	byte = (const __u8 *)(attr + 1);
	if (*byte > 1)
		return -EBADMSG;
	*value = *byte;
	return 0;
}

int ctlv_get_str(const struct ctlv_attr *attr, const char **value)
{
	const char *str = (const char *)(attr + 1);
	__u32 len = ctlv_attr_payload_len(attr);

	if (ctlv_attr_is_nest(attr))
		return -EINVAL;
	if (!len || str[len - 1] != '\0')
		return -EBADMSG;
	*value = str;
	return 0;
}

int ctlv_get_bin(const struct ctlv_attr *attr, const void **value, __u32 *len)
{
	if (ctlv_attr_is_nest(attr))
		return -EINVAL;
	*value = attr + 1;
	if (len)
		*len = ctlv_attr_payload_len(attr);
	return 0;
}

/*
 * Neither refuses a value from a device newer than its caller: dropping the
 * reply would make every added value an ABI break.
 */
int ctlv_get_enum(const struct ctlv_attr *attr, __u64 values, __u64 *value,
		  bool *known)
{
	int ret;

	ret = get_scalar(attr, value);
	if (ret)
		return ret;
	if (known)
		*known = *value >= 1 && *value <= values;
	return 0;
}

int ctlv_get_flags(const struct ctlv_attr *attr, __u64 mask, __u64 *value,
		   __u64 *unknown)
{
	int ret;

	ret = get_scalar(attr, value);
	if (ret)
		return ret;
	if (unknown)
		*unknown = *value & ~mask;
	return 0;
}
