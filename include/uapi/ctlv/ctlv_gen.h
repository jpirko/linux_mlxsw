/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv CTLV family. Types and op IDs are complete; the message format carrying
 * them is in <ctlv/ctlv.h>.
 */

#ifndef _UAPI_CTLV_CTLV_GEN_H
#define _UAPI_CTLV_CTLV_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>

/* enum op-type */
enum _ctlv_op_type {
	/* Executed and acknowledged. */
	CTLV_OP_TYPE_ACTION = 1,
	/* Executed and answered, in as many fragments as the answer needs. */
	CTLV_OP_TYPE_QUERY = 2,
	/* Produced by the device and read from a subscribed file description. */
	CTLV_OP_TYPE_EVENT = 3,
};

/* enum op-availability */
enum _ctlv_op_availability {
	/* A device that publishes the op always answers it. */
	CTLV_OP_AVAILABILITY_FIXED = 1,
	/* A device may disable and enable the op at any time. */
	CTLV_OP_AVAILABILITY_DYNAMIC = 2,
};

/* enum blob-type */
enum _ctlv_blob_type {
	/* Anonymous memory of the calling process, at address. */
	CTLV_BLOB_TYPE_VA = 1,
	/* A memfd, by fd, at offset. */
	CTLV_BLOB_TYPE_MEMFD = 2,
	/* An exported dma-buf, by fd, at offset. */
	CTLV_BLOB_TYPE_DMA_BUF = 3,
};

/* enum blob-direction */
enum _ctlv_blob_direction {
	/* From the caller to the device, which only reads the blob. */
	CTLV_BLOB_DIRECTION_INPUT = 1,
	/* From the device to the caller. */
	CTLV_BLOB_DIRECTION_OUTPUT = 2,
	/* Both ways: the device reads the blob and writes it. */
	CTLV_BLOB_DIRECTION_INOUT = 3,
};

/* enum validation-reason */
enum _ctlv_validation_reason {
	/*
	 * The TLVs do not frame: the root is not one, a length runs past its record, or a TLV is a
	 * record where its schema has a scalar or the reverse.
	 */
	CTLV_VALIDATION_REASON_MALFORMED_MESSAGE = 1,
	/*
	 * A TLV of a type its record does not place there, not marked IGNORE_IF_UNKNOWN or of a
	 * type the schema places elsewhere.
	 */
	CTLV_VALIDATION_REASON_UNKNOWN_TYPE = 2,
	/*
	 * A header flag the message may not carry, or a flags payload with a bit its field does not
	 * declare.
	 */
	CTLV_VALIDATION_REASON_UNKNOWN_FLAG = 3,
	/* Records nested deeper than the core follows. */
	CTLV_VALIDATION_REASON_RECORD_TOO_DEEP = 4,
	/* A required field is absent, and missing-type says which. */
	CTLV_VALIDATION_REASON_MISSING_FIELD = 5,
	/* A field occurs more often than its schema allows. */
	CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES = 6,
	/* An integer, an enum value or a bool outside what its field takes. */
	CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE = 7,
	/* A payload of the wrong length for its kind, or outside its field's length bounds. */
	CTLV_VALIDATION_REASON_BAD_LENGTH = 8,
	/* A string payload that is not exactly one NUL-terminated string. */
	CTLV_VALIDATION_REASON_BAD_STRING = 9,
	/*
	 * A blob descriptor its field does not take, or one whose memory could not be acquired or
	 * is too short for what the op writes there.
	 */
	CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR = 10,
	/* A query continuation of a generation the query no longer has. */
	CTLV_VALIDATION_REASON_STALE_GENERATION = 11,
	/* The TLVs of one type are not contiguous in their record. */
	CTLV_VALIDATION_REASON_NONCONTIGUOUS_TYPE = 12,
};

/* enum op-fd-access */
enum _ctlv_op_fd_access {
	/* Read access, which an observation fd has as well as a full-control one. */
	CTLV_OP_FD_ACCESS_READ = 1,
	/* Read-write access, which only a full-control fd has. */
	CTLV_OP_FD_ACCESS_READ_WRITE = 2,
};

/* blob */
/* What holds the bytes, which says which of the fields below are present. */
#define CTLV_T_BLOB_TYPE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00010000, 1)
/* Where a va blob starts in the calling process. */
#define CTLV_T_BLOB_ADDRESS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00010000, 2)
/* The file descriptor of a memfd or dma-buf blob. */
#define CTLV_T_BLOB_FD \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00010000, 3)
/* Where in fd the blob starts, zero when absent. */
#define CTLV_T_BLOB_OFFSET \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00010000, 4)
/* The length of the blob in bytes, at most what its field takes. */
#define CTLV_T_BLOB_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00010000, 5)

/* error-info */
/* The rule the message broke. */
#define CTLV_T_ERROR_INFO_REASON \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 1)
/* What went wrong, for a person to read. */
#define CTLV_T_ERROR_INFO_MESSAGE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 2)
/*
 * The offset of the offending TLV from the start of the message, or of its record when what is
 * wrong is something the record lacks.
 */
#define CTLV_T_ERROR_INFO_BAD_TLV_OFFSET \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 3)
/* The offset of the record holding it, from the start of the message. */
#define CTLV_T_ERROR_INFO_RECORD_OFFSET \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 4)
/* The type of the required field that is absent. */
#define CTLV_T_ERROR_INFO_MISSING_TYPE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 5)
/* The generation the query continuation carried. */
#define CTLV_T_ERROR_INFO_EXPECTED_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 6)
/* The generation the query has now. */
#define CTLV_T_ERROR_INFO_CURRENT_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 7)
/* How long the output blob at bad-tlv-offset has to be for the op. */
#define CTLV_T_ERROR_INFO_REQUIRED_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00020000, 8)

/* schema-node */
/* Zero in a message root node, as in the root TLV. */
#define CTLV_T_SCHEMA_NODE_TYPE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 1)
/* enum ctlv_schema_kind. */
#define CTLV_T_SCHEMA_NODE_KIND \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 2)
/* Zero where the node may repeat as often as the message has room. */
#define CTLV_T_SCHEMA_NODE_MAX_OCCURRENCES \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 3)
/*
 * The inclusive lower bound of an integer or enum payload, a raw 64-bit value read according to
 * kind. An enum's values are numbered without gaps.
 */
#define CTLV_T_SCHEMA_NODE_MIN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 4)
/* The inclusive upper bound, read the same way. */
#define CTLV_T_SCHEMA_NODE_MAX \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 5)
/* The shortest payload of a string or binary field, a string's terminator excluded. */
#define CTLV_T_SCHEMA_NODE_MIN_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 6)
/* The longest payload, likewise. */
#define CTLV_T_SCHEMA_NODE_MAX_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 7)
/* The bits a flags payload may carry. */
#define CTLV_T_SCHEMA_NODE_FLAGS_MASK \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 8)
/* enum ctlv_unit, absent when the payload is dimensionless. */
#define CTLV_T_SCHEMA_NODE_UNIT \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 9)
/*
 * The decimal exponent of the unit, a payload being value * 10^scale of it, present only with a
 * unit.
 */
#define CTLV_T_SCHEMA_NODE_SCALE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 10)
/* Which way a blob field's bytes go. */
#define CTLV_T_SCHEMA_NODE_BLOB_DIRECTION \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 11)
/* Mask of BIT(enum ctlv_blob_type): the backings this node takes. */
#define CTLV_T_SCHEMA_NODE_BLOB_BACKINGS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 12)
/* Maximum external payload length, unrelated to the inline message limit. */
#define CTLV_T_SCHEMA_NODE_BLOB_MAX_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 13)
/* The power of two a blob's address or offset is a multiple of. */
#define CTLV_T_SCHEMA_NODE_BLOB_ALIGNMENT \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 14)
/*
 * Levels to climb to the node whose members are this one's as well, one meaning the node this one
 * is a member of. A node that carries it lists no members itself.
 */
#define CTLV_T_SCHEMA_NODE_REENTERS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 15)
/* The required nodes one level in. */
#define CTLV_T_SCHEMA_NODE_FIELDS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 16)
/* The optional nodes one level in. */
#define CTLV_T_SCHEMA_NODE_OPTIONAL_FIELDS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00030000, 17)

/* event-subscription */
/* The op ID of an event the device emits. */
#define CTLV_T_EVENT_SUBSCRIPTION_OP_IDS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x00040000, 1)

/* family-get reply */
/* One entry per family of the chain. */
#define CTLV_T_FAMILY_GET_FAMILIES \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0010000, 1)

/* family-get families */
/* The family ID, which bits 63..48 of the family's types carry. */
#define CTLV_T_FAMILY_GET_FAMILIES_ID \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0011000, 1)
/*
 * The registry name, also what a uevent carries. Bounded here, this being the only place a message
 * carries a family name.
 */
#define CTLV_T_FAMILY_GET_FAMILIES_NAME \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0011000, 2)

/* op-get reply */
/* One entry per op the device publishes. */
#define CTLV_T_OP_GET_OPS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0020000, 1)

/* op-get ops */
/* The op ID, which CTLV_IOC_OP_EXEC and an event header carry. */
#define CTLV_T_OP_GET_OPS_ID \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 1)
/* Action, query or event. */
#define CTLV_T_OP_GET_OPS_TYPE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 2)
/* Whether the device may disable and enable the op. */
#define CTLV_T_OP_GET_OPS_AVAILABILITY \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 3)
/* Whether the device has the op disabled right now. */
#define CTLV_T_OP_GET_OPS_DISABLED \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 4)
/* The longest request composed from the schema. */
#define CTLV_T_OP_GET_OPS_REQUEST_MAX_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 5)
/* The reply buffer an action needs, zero for one that answers nothing. */
#define CTLV_T_OP_GET_OPS_ACTION_REPLY_MAX_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 6)
/* The shortest reply buffer a query takes. */
#define CTLV_T_OP_GET_OPS_QUERY_REPLY_MIN_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 7)
/* The reply buffer length the family suggests for a query. */
#define CTLV_T_OP_GET_OPS_QUERY_REPLY_SUGGESTED_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 8)
/* The access a file description needs to execute the op. */
#define CTLV_T_OP_GET_OPS_FD_ACCESS \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 9)
/* The longest error information the op can return. */
#define CTLV_T_OP_GET_OPS_ERROR_INFO_MAX_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 10)
/* The longest message an event of the op carries. */
#define CTLV_T_OP_GET_OPS_EVENT_MAX_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0021000, 11)

/* op-schema-get request */
/* The ID of the op to describe. */
#define CTLV_T_OP_SCHEMA_GET_ID \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x80030000, 1)

/* op-schema-get reply */
/* The root of the op's request tree, absent when it has no request. */
#define CTLV_T_OP_SCHEMA_GET_REQUEST \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0030000, 1)
/*
 * The root of its reply tree, absent when it has no reply. An event's one tree, its payload's, is
 * reported here.
 */
#define CTLV_T_OP_SCHEMA_GET_REPLY \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0030000, 2)

/* event-queue-get reply */
/* How long the queue is. */
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0060000, 1)
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_LEN_UNIT CTLV_UNIT_BYTES
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_LEN_SCALE 0
/* How much of it the queued events take. */
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_USED \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0060000, 2)
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_USED_UNIT CTLV_UNIT_BYTES
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_USED_SCALE 0
/* The shortest queue-len this device takes. */
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MIN_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0060000, 3)
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MIN_LEN_UNIT CTLV_UNIT_BYTES
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MIN_LEN_SCALE 0
/* The longest queue-len this device takes. */
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MAX_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0060000, 4)
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MAX_LEN_UNIT CTLV_UNIT_BYTES
#define CTLV_T_EVENT_QUEUE_GET_QUEUE_MAX_LEN_SCALE 0

/* event-queue-set request */
/* The new length, within what event-queue-get reports. */
#define CTLV_T_EVENT_QUEUE_SET_QUEUE_LEN \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0x80070000, 1)
#define CTLV_T_EVENT_QUEUE_SET_QUEUE_LEN_UNIT CTLV_UNIT_BYTES
#define CTLV_T_EVENT_QUEUE_SET_QUEUE_LEN_SCALE 0

/* op-changed payload */
/* The op that changed. */
#define CTLV_T_OP_CHANGED_ID \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0080000, 1)
/* Whether the op is disabled now. */
#define CTLV_T_OP_CHANGED_DISABLED \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0080000, 2)
/* Advances once per op state change on one device. */
#define CTLV_T_OP_CHANGED_STATE_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0080000, 3)

/* event-overflow payload */
/* The sequence number of the first event lost. */
#define CTLV_T_EVENT_OVERFLOW_FIRST_SEQUENCE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0090000, 1)
/* The sequence number of the last event lost. */
#define CTLV_T_EVENT_OVERFLOW_LAST_SEQUENCE \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0090000, 2)
/* How many events were lost. */
#define CTLV_T_EVENT_OVERFLOW_DROPPED_COUNT \
	CTLV_TYPE(CTLV_FAMILY_ROOT, 0xc0090000, 3)

/* ops */
#define CTLV_OP_FAMILY_GET CTLV_OP(CTLV_FAMILY_ROOT, 1)
#define CTLV_OP_OP_GET CTLV_OP(CTLV_FAMILY_ROOT, 2)
#define CTLV_OP_OP_SCHEMA_GET CTLV_OP(CTLV_FAMILY_ROOT, 3)
#define CTLV_OP_SUBSCRIPTIONS_ADD CTLV_OP(CTLV_FAMILY_ROOT, 4)
#define CTLV_OP_SUBSCRIPTIONS_REMOVE CTLV_OP(CTLV_FAMILY_ROOT, 5)
#define CTLV_OP_EVENT_QUEUE_GET CTLV_OP(CTLV_FAMILY_ROOT, 6)
#define CTLV_OP_EVENT_QUEUE_SET CTLV_OP(CTLV_FAMILY_ROOT, 7)
#define CTLV_OP_OP_CHANGED CTLV_OP(CTLV_FAMILY_ROOT, 8)
#define CTLV_OP_EVENT_OVERFLOW CTLV_OP(CTLV_FAMILY_ROOT, 9)
#define CTLV_OP_DEVICE_REMOVED CTLV_OP(CTLV_FAMILY_ROOT, 10)

#endif /* _UAPI_CTLV_CTLV_GEN_H */
