/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * CTLV transport UAPI: the TLV message format and the op-execution ioctl.
 * Schema-free; a family's constants are generated into <ctlv/<family>_gen.h>,
 * the root family's into <ctlv/ctlv_gen.h>.
 */
#ifndef _UAPI_CTLV_CTLV_H
#define _UAPI_CTLV_CTLV_H

#include <linux/const.h>
#include <linux/ioctl.h>
#include <linux/types.h>

/**
 * struct ctlv_hdr - TLV header.
 * @len: this header plus its payload, excluding this TLV's trailing padding
 * @flags: CTLV_HDR_F_* bits
 * @type: family, record and field, or CTLV_TYPE_ROOT for a message root
 *
 * TLVs are aligned to CTLV_ALIGNTO, which is why @type is __aligned_u64: a
 * 32-bit ABI would otherwise align this structure to four bytes. A record's
 * payload is the concatenation of CTLV_ALIGN(child.len) bytes per child, so
 * child padding counts inside the containing record's @len while a TLV's own
 * padding does not. Scalar payloads are native-endian. Within each request
 * root or record, the TLVs of one @type are contiguous; different types
 * otherwise have no required order.
 */
struct ctlv_hdr {
	__u32 len;
	__u32 flags;
	__aligned_u64 type;
};

#define CTLV_ALIGNTO 8
#define CTLV_ALIGN(_len) __ALIGN_KERNEL((_len), CTLV_ALIGNTO)

/* Set exactly on the root and on records, clear on scalar TLVs. */
#define CTLV_HDR_F_RECORD		_BITUL(0)
/* Query fragments: set on the root and each record still open at the end. */
#define CTLV_HDR_F_FRAG_MORE		_BITUL(1)
/*
 * A request TLV, never the root, that a device whose schema does not declare
 * its type skips whole instead of refusing the message. A type that schema
 * does declare is held to it wherever it appears, so this asks to be forgiven
 * what this kernel has never heard of and nothing else.
 */
#define CTLV_HDR_F_IGNORE_IF_UNKNOWN	_BITUL(2)

/* The root: carries CTLV_HDR_F_RECORD, valid only at a message start. */
#define CTLV_TYPE_ROOT 0ULL

/**
 * enum ctlv_type_kind - What the record of a type starts from.
 * @CTLV_TYPE_KIND_NAMED: a named record
 * @CTLV_TYPE_KIND_RESERVED: reserved, so no type has it
 * @CTLV_TYPE_KIND_REQUEST: an op's request
 * @CTLV_TYPE_KIND_REPLY: an op's reply
 *
 * Bits 47..46 of a type. The index in 45..32 is the named record's place in
 * the family's records, or the op's number.
 */
enum ctlv_type_kind {
	CTLV_TYPE_KIND_NAMED = 0,
	CTLV_TYPE_KIND_RESERVED = 1,
	CTLV_TYPE_KIND_REQUEST = 2,
	CTLV_TYPE_KIND_REPLY = 3,
};

/*
 * Family in bits 63..48, record in 47..16, field in 15..0. The record is
 * the type kind, the index, then the path of unnamed records below them, four
 * bits a level (31..16).
 */
#define CTLV_TYPE(_family, _record, _field)	\
	(((__u64)(_family) << 48) |		\
	 ((__u64)(_record) << 16) |		\
	 (__u64)(_field))

#define CTLV_TYPE_FAMILY(_type)	((__u16)((_type) >> 48))
#define CTLV_TYPE_KIND(_type)	((__u8)(((_type) >> 46) & 3))
#define CTLV_TYPE_RECORD(_type)	((__u32)((_type) >> 16))
#define CTLV_TYPE_FIELD(_type)	((__u16)(_type))

/* Complete op: family in bits 63..48, zero in 47..32, op in 31..0. */
#define CTLV_OP(_family, _op)	\
	(((__u64)(_family) << 48) | (__u64)(_op))

#define CTLV_OP_FAMILY(_op)		((__u16)((_op) >> 48))
#define CTLV_OP_NUMBER(_op)		((__u32)(_op))

/**
 * enum ctlv_unit - CTLV-defined unit of an integer field.
 * @CTLV_UNIT_NONE: dimensionless
 * @CTLV_UNIT_SECONDS: seconds
 * @CTLV_UNIT_HERTZ: hertz
 * @CTLV_UNIT_BYTES: bytes
 * @CTLV_UNIT_CELSIUS: degrees Celsius
 *
 * A payload is value * 10^scale of this unit, from the decimal scale the field
 * declares alongside. Changing either is an ABI break.
 */
enum ctlv_unit {
	CTLV_UNIT_NONE = 0,
	CTLV_UNIT_SECONDS = 1,
	CTLV_UNIT_HERTZ = 2,
	CTLV_UNIT_BYTES = 3,
	CTLV_UNIT_CELSIUS = 4,
};

/**
 * enum ctlv_schema_kind - CTLV-defined schema kind of a field.
 * @CTLV_SCHEMA_UINT: unsigned integer payload bounded by a range
 * @CTLV_SCHEMA_SINT: signed integer payload bounded by a range
 * @CTLV_SCHEMA_BOOL: one payload byte holding zero or one
 * @CTLV_SCHEMA_STRING: NUL-terminated string payload bounded by length
 * @CTLV_SCHEMA_BINARY: opaque payload bounded by length
 * @CTLV_SCHEMA_ENUM: unsigned integer payload restricted to a range
 * @CTLV_SCHEMA_FLAGS: unsigned integer payload restricted to a mask
 * @CTLV_SCHEMA_RECORD: a record, its fields described by further nodes
 * @CTLV_SCHEMA_BLOB: blob descriptor record
 *
 * Exactly one schema kind applies to a field. These are the values
 * op-schema-get reports, and the set is append-only.
 */
enum ctlv_schema_kind {
	CTLV_SCHEMA_UINT = 0,
	CTLV_SCHEMA_SINT = 1,
	CTLV_SCHEMA_BOOL = 2,
	CTLV_SCHEMA_STRING = 3,
	CTLV_SCHEMA_BINARY = 4,
	CTLV_SCHEMA_ENUM = 5,
	CTLV_SCHEMA_FLAGS = 6,
	CTLV_SCHEMA_RECORD = 7,
	CTLV_SCHEMA_BLOB = 8,
};

/**
 * struct ctlv_op_exec - Op-execution ioctl argument.
 * @size: sizeof(struct ctlv_op_exec) as the caller was built with it
 * @flags: must be zero
 * @op_id: complete op ID; zero is invalid
 * @request: struct ctlv_hdr __user *, the mandatory complete request root
 * @reply: struct ctlv_hdr __user *, the success reply or the error
 *	information
 * @request_len: readable bytes beginning at @request, at least one root header;
 *	the transport copy boundary, which may exceed the root's own length
 * @reply_len: writable bytes beginning at @reply; input only, never
 *	overwritten by the kernel
 * @generation: query only; zero on a first fragment, the previously returned
 *	value on continuation, and the query generation on output. Actions must
 *	set zero.
 * @cursor: query only; zero starts a query and a nonzero value resumes it. On
 *	output it is the next cursor when the root carries
 *	CTLV_HDR_F_FRAG_MORE and zero when no further fragment exists. Actions
 *	must set zero.
 *
 * The structure only grows at its end. The kernel reads @size bytes, at least
 * the 56 of this first version, refuses a tail it does not know unless it is
 * zero, and writes back only within @size. @request and @reply are fixed-width
 * representations of userspace pointers so that the compat layout is identical.
 */
struct ctlv_op_exec {
	__u32 size;
	__u32 flags;
	__aligned_u64 op_id;
	__aligned_u64 request;
	__aligned_u64 reply;
	__u32 request_len;
	__u32 reply_len;
	__aligned_u64 generation;
	__aligned_u64 cursor;
};

#define CTLV_IOC_MAGIC 0xBB

#define CTLV_IOC_OP_EXEC _IO(CTLV_IOC_MAGIC, 0x00)

#endif /* _UAPI_CTLV_CTLV_H */
