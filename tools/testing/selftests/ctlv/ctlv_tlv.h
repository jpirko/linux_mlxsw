/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Direct message encoding for wire-format tests.
 */
#ifndef CTLV_TLV_H
#define CTLV_TLV_H

#include <stdbool.h>
#include <string.h>

#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>

/* Room for the largest message and nests deeper than CTLV accepts. */
#define CTLV_TLV_LEN 8192
#define CTLV_TLV_DEPTH (CTLV_MAX_NEST_DEPTH + 4)

/* Offsets survive buffer growth and match kernel error locations. */
struct ctlv_tlv {
	__u32 len;
	__u32 open[CTLV_TLV_DEPTH];
	unsigned int depth;
	/* Something did not fit: the message says nothing about the kernel. */
	bool overflow;
	__u64 buf[CTLV_TLV_LEN / sizeof(__u64)];
};

static void *ctlv_tlv_at(struct ctlv_tlv *tlv, __u32 offset)
{
	return (char *)tlv->buf + offset;
}

static struct ctlv_attr *ctlv_tlv_attr(struct ctlv_tlv *tlv, __u32 offset)
{
	return ctlv_tlv_at(tlv, offset);
}

/* Zeroes up to the next attr boundary, as the padding of a message is zero. */
static void ctlv_tlv_pad(struct ctlv_tlv *tlv)
{
	__u32 padded = CTLV_ALIGN(tlv->len);

	if (padded > sizeof(tlv->buf)) {
		tlv->overflow = true;
		return;
	}
	memset(ctlv_tlv_at(tlv, tlv->len), 0, padded - tlv->len);
	tlv->len = padded;
}

/* Bytes exactly where the message has reached, headers and padding aside. */
static __u32 ctlv_tlv_raw(struct ctlv_tlv *tlv, const void *bytes, __u32 len)
{
	__u32 offset = tlv->len;

	if (len > sizeof(tlv->buf) - tlv->len) {
		tlv->overflow = true;
		return offset;
	}
	memcpy(ctlv_tlv_at(tlv, offset), bytes, len);
	tlv->len += len;
	return offset;
}

/* One attr, aligned and complete. Returns the offset a test patches. */
static __u32 ctlv_tlv_put(struct ctlv_tlv *tlv, __u64 attr_id, __u32 flags,
			  const void *payload, __u32 len)
{
	struct ctlv_attr attr = {
		.len = sizeof(attr) + len,
		.flags = flags,
		.attr_id = attr_id,
	};
	__u32 offset;

	ctlv_tlv_pad(tlv);
	offset = ctlv_tlv_raw(tlv, &attr, sizeof(attr));
	if (len)
		ctlv_tlv_raw(tlv, payload, len);
	return offset;
}

static __u32 ctlv_tlv_u64(struct ctlv_tlv *tlv, __u64 attr_id, __u64 value)
{
	return ctlv_tlv_put(tlv, attr_id, 0, &value, sizeof(value));
}

static __u32 ctlv_tlv_s64(struct ctlv_tlv *tlv, __u64 attr_id, __s64 value)
{
	return ctlv_tlv_put(tlv, attr_id, 0, &value, sizeof(value));
}

/* One byte, which is the whole of a boolean payload. */
static __u32 ctlv_tlv_bool(struct ctlv_tlv *tlv, __u64 attr_id, bool value)
{
	__u8 byte = value;

	return ctlv_tlv_put(tlv, attr_id, 0, &byte, sizeof(byte));
}

/* The terminator is part of the payload, and a schema length excludes it. */
static __u32 ctlv_tlv_str(struct ctlv_tlv *tlv, __u64 attr_id,
			  const char *value)
{
	return ctlv_tlv_put(tlv, attr_id, 0, value, strlen(value) + 1);
}

static __u32 ctlv_tlv_bin(struct ctlv_tlv *tlv, __u64 attr_id,
			  const void *value, __u32 len)
{
	return ctlv_tlv_put(tlv, attr_id, 0, value, len);
}

/* Nesting past the array this tracks overruns the writer, not the kernel. */
static __u32 ctlv_tlv_nest_start(struct ctlv_tlv *tlv, __u64 attr_id)
{
	__u32 offset = ctlv_tlv_put(tlv, attr_id, CTLV_ATTR_F_NESTED, NULL, 0);

	if (tlv->depth == CTLV_TLV_DEPTH) {
		tlv->overflow = true;
		return offset;
	}
	tlv->open[tlv->depth++] = offset;
	return offset;
}

/* Children are aligned spans, so the length covers the last one's padding. */
static void ctlv_tlv_nest_end(struct ctlv_tlv *tlv)
{
	__u32 offset;

	if (!tlv->depth) {
		tlv->overflow = true;
		return;
	}
	ctlv_tlv_pad(tlv);
	offset = tlv->open[--tlv->depth];
	ctlv_tlv_attr(tlv, offset)->len = tlv->len - offset;
}

/* The root, which a test may leave for ctlv_tlv_done() to end. */
static void ctlv_tlv_init(struct ctlv_tlv *tlv)
{
	*tlv = (struct ctlv_tlv){};
	ctlv_tlv_nest_start(tlv, CTLV_ATTR_ROOT);
}

static __u32 ctlv_tlv_done(struct ctlv_tlv *tlv)
{
	while (tlv->depth)
		ctlv_tlv_nest_end(tlv);
	return tlv->len;
}

/* Complete descriptors; a test about an incomplete one writes the members. */
static __u32 ctlv_tlv_blob_va(struct ctlv_tlv *tlv, __u64 attr_id,
			      __u64 address, __u64 length)
{
	__u32 offset = ctlv_tlv_nest_start(tlv, attr_id);

	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_TYPE, CTLV_BLOB_TYPE_VA);
	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_ADDRESS, address);
	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_LENGTH, length);
	ctlv_tlv_nest_end(tlv);
	return offset;
}

/* Until a resolver runs the fd is only a number, not a real descriptor. */
static __u32 ctlv_tlv_blob_fd(struct ctlv_tlv *tlv, __u64 attr_id, __u64 type,
			      __u64 fd, __u64 offset_in, __u64 length)
{
	__u32 offset = ctlv_tlv_nest_start(tlv, attr_id);

	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_TYPE, type);
	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_FD, fd);
	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_OFFSET, offset_in);
	ctlv_tlv_u64(tlv, CTLV_ATTR_BLOB_LENGTH, length);
	ctlv_tlv_nest_end(tlv);
	return offset;
}

static __u32 ctlv_tlv_blob_memfd(struct ctlv_tlv *tlv, __u64 attr_id, __u64 fd,
				 __u64 offset_in, __u64 length)
{
	return ctlv_tlv_blob_fd(tlv, attr_id, CTLV_BLOB_TYPE_MEMFD, fd,
				offset_in, length);
}

static __u32 ctlv_tlv_blob_dma_buf(struct ctlv_tlv *tlv, __u64 attr_id,
				   __u64 fd, __u64 offset_in, __u64 length)
{
	return ctlv_tlv_blob_fd(tlv, attr_id, CTLV_BLOB_TYPE_DMA_BUF, fd,
				offset_in, length);
}

/* A nest is its padded children concatenated: a step is one aligned length. */
static const struct ctlv_attr *ctlv_rd_first(const struct ctlv_attr *nest)
{
	return nest->len > sizeof(*nest) ? nest + 1 : NULL;
}

static const struct ctlv_attr *ctlv_rd_next(const struct ctlv_attr *nest,
					    const struct ctlv_attr *child)
{
	const char *next = (const char *)child + CTLV_ALIGN(child->len);

	if (next >= (const char *)nest + nest->len)
		return NULL;
	return (const struct ctlv_attr *)next;
}

#define ctlv_rd_for_each(_child, _nest)					\
	for ((_child) = ctlv_rd_first(_nest); (_child);			\
	     (_child) = ctlv_rd_next((_nest), (_child)))

static const void *ctlv_rd_payload(const struct ctlv_attr *attr, __u32 *len)
{
	*len = attr->len - sizeof(*attr);
	return attr + 1;
}

static __u64 ctlv_rd_u64(const struct ctlv_attr *attr)
{
	__u64 value = 0;
	__u32 len;

	memcpy(&value, ctlv_rd_payload(attr, &len),
	       len < sizeof(value) ? len : sizeof(value));
	return value;
}

#endif /* CTLV_TLV_H */
