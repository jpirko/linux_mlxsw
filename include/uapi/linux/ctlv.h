/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV transport UAPI: the TLV wire format, the op-execution ioctl, the event
 * record header and the framework limits. Schema-free; a family's constants
 * are generated into <linux/<family>_gen.h>, the root family's into
 * <linux/ctlv_gen.h>.
 */
#ifndef _UAPI_LINUX_CTLV_H
#define _UAPI_LINUX_CTLV_H

#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * struct ctlv_attr - Attr header.
 * @len: this header plus its payload, excluding this attr's trailing padding
 * @flags: CTLV_ATTR_F_* bits
 * @attr_id: complete attr ID, or CTLV_ATTR_ID_ROOT for a message root
 *
 * Attrs are aligned to CTLV_ALIGNTO, which is why @attr_id is __aligned_u64: a
 * 32-bit ABI would otherwise align this structure to four bytes. A nested
 * payload is the concatenation of CTLV_ALIGN(child.len) bytes per child, so
 * child padding counts inside the containing attr's @len while a leaf's own
 * padding does not. Scalar payloads are native-endian.
 */
struct ctlv_attr {
	__u32 len;
	__u32 flags;
	__aligned_u64 attr_id;
};

#define CTLV_ALIGNTO 8
#define CTLV_ALIGN(_len) (((_len) + CTLV_ALIGNTO - 1) & ~(CTLV_ALIGNTO - 1))

/* Set exactly on root and nested attrs, clear on scalar attrs. */
#define CTLV_ATTR_F_NESTED	(1U << 0)
/* Query fragments: set on the root and each nest logically open at the end. */
#define CTLV_ATTR_F_FRAG_MORE	(1U << 1)

/* Root attr ID: carries CTLV_ATTR_F_NESTED, valid only at a message start. */
#define CTLV_ATTR_ID_ROOT 0ULL

/* Complete attr ID: family in bits 63..48, group in 47..32, attr in 31..0. */
#define CTLV_ATTR_ID(_family, _group, _attr)	\
	(((__u64)(_family) << 48) |		\
	 ((__u64)(_group) << 32) |		\
	 (__u64)(_attr))

#define CTLV_ATTR_ID_FAMILY(_id)	((__u16)((_id) >> 48))
#define CTLV_ATTR_ID_GROUP(_id)		((__u16)((_id) >> 32))
#define CTLV_ATTR_ID_NUMBER(_id)	((__u32)(_id))

/* Complete op ID: family in bits 63..48, zero in 47..32, op in 31..0. */
#define CTLV_OP_ID(_family, _op)	\
	(((__u64)(_family) << 48) | (__u64)(_op))

#define CTLV_OP_ID_FAMILY(_id)		((__u16)((_id) >> 48))
#define CTLV_OP_ID_NUMBER(_id)		((__u32)(_id))

/**
 * enum ctlv_unit - Framework-owned unit of an integer attr payload.
 * @CTLV_UNIT_NONE: dimensionless
 * @CTLV_UNIT_SECONDS: seconds
 * @CTLV_UNIT_HERTZ: hertz
 * @CTLV_UNIT_BYTES: bytes
 * @CTLV_UNIT_CELSIUS: degrees Celsius
 *
 * A payload is value * 10^scale of this unit, from the decimal scale the attr
 * declares alongside. Changing either is an ABI break.
 */
enum ctlv_unit {
	CTLV_UNIT_NONE = 0,
	CTLV_UNIT_SECONDS,
	CTLV_UNIT_HERTZ,
	CTLV_UNIT_BYTES,
	CTLV_UNIT_CELSIUS,
};

/**
 * enum ctlv_schema_kind - Framework-owned form of one attr payload.
 * @CTLV_SCHEMA_UINT: unsigned integer payload bounded by a range
 * @CTLV_SCHEMA_SINT: signed integer payload bounded by a range
 * @CTLV_SCHEMA_BOOL: one payload byte holding zero or one
 * @CTLV_SCHEMA_STRING: NUL-terminated string payload bounded by length
 * @CTLV_SCHEMA_BINARY: opaque payload bounded by length
 * @CTLV_SCHEMA_ENUM: unsigned integer payload restricted to a range
 * @CTLV_SCHEMA_FLAGS: unsigned integer payload restricted to a mask
 * @CTLV_SCHEMA_NEST: nested payload described by further nodes
 * @CTLV_SCHEMA_BLOB: blob descriptor nest
 *
 * Exactly one form applies to an attr. These are the values op-schema-get
 * reports, and the set is append-only.
 */
enum ctlv_schema_kind {
	CTLV_SCHEMA_UINT,
	CTLV_SCHEMA_SINT,
	CTLV_SCHEMA_BOOL,
	CTLV_SCHEMA_STRING,
	CTLV_SCHEMA_BINARY,
	CTLV_SCHEMA_ENUM,
	CTLV_SCHEMA_FLAGS,
	CTLV_SCHEMA_NEST,
	CTLV_SCHEMA_BLOB,
};

/**
 * struct ctlv_op_exec - Op-execution ioctl argument.
 * @op_id: complete op ID; zero is invalid
 * @request: struct ctlv_attr __user *, the complete request root
 * @reply: struct ctlv_attr __user *, the success reply or the error
 *	information
 * @request_len: readable bytes beginning at @request; the transport copy
 *	boundary, which may exceed the root's own length
 * @reply_len: writable bytes beginning at @reply; input only, never
 *	overwritten by the kernel
 * @generation: query only; zero on a first fragment, the previously returned
 *	value on continuation, and the query generation on output. Actions must
 *	set zero.
 * @cursor: query only; zero starts a query and a nonzero value resumes it. On
 *	output it is the next cursor when the root carries
 *	CTLV_ATTR_F_FRAG_MORE and zero when no further fragment exists. Actions
 *	must set zero.
 * @reserved: must be zero
 *
 * No size field: the ioctl number encodes the structure size and @reserved
 * covers compatible growth. @request and @reply are fixed-width
 * representations of userspace pointers so that the compat layout is identical.
 */
struct ctlv_op_exec {
	__aligned_u64 op_id;
	__aligned_u64 request;
	__aligned_u64 reply;
	__u32 request_len;
	__u32 reply_len;
	__aligned_u64 generation;
	__aligned_u64 cursor;
	__aligned_u64 reserved[2];
};

#define CTLV_IOC_MAGIC 0xBB

#define CTLV_IOC_OP_EXEC _IOWR(CTLV_IOC_MAGIC, 0x00, struct ctlv_op_exec)

/**
 * struct ctlv_event_hdr - Header of one queued event record.
 * @length: complete record length, this header plus the aligned root
 * @flags: must be zero
 * @op_id: complete op ID of the event op that produced the record
 * @sequence: per open file description sequence number, assigned to every
 *	eligible enqueue attempt including dropped ones
 *
 * Exactly one root attr follows the header, and one read() returns one record.
 */
struct ctlv_event_hdr {
	__u32 length;
	__u32 flags;
	__aligned_u64 op_id;
	__aligned_u64 sequence;
};

/* Implementation limits, not wire-field maxima: raising them is compatible. */

/* Request roots, action replies, query reply fragments and event roots. */
#define CTLV_MAX_INLINE_MESSAGE_LEN	0x100000	/* 1 MiB */
/* Includes the error information root. */
#define CTLV_MAX_ERROR_LEN		0x4000		/* 16 KiB */
/* The root attr is at depth zero. */
#define CTLV_MAX_NEST_DEPTH		32
#define CTLV_DEFAULT_QUERY_REPLY_LEN	0x10000		/* 64 KiB */
#define CTLV_MAX_EVENT_QUEUE_LEN	0x1000000	/* 16 MiB */

#endif /* _UAPI_LINUX_CTLV_H */
