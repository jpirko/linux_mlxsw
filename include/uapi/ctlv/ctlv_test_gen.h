/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-test CTLV family. Types and op IDs are complete; the message format
 * carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv header is included here, so one include covers the whole effective schema.
 */

#ifndef _UAPI_CTLV_CTLV_TEST_GEN_H
#define _UAPI_CTLV_CTLV_TEST_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>
#include <ctlv/ctlv_gen.h>
#include <linux/const.h>

/* enum echo-mode */
enum _ctlv_test_echo_mode {
	/* The first value, which means nothing of its own. */
	CTLV_TEST_ECHO_MODE_IDLE = 1,
	/* The second, likewise. */
	CTLV_TEST_ECHO_MODE_BUSY = 2,
	/* The third and highest, likewise. */
	CTLV_TEST_ECHO_MODE_AUTO = 3,
};

/* flags echo-quirk */
enum _ctlv_test_echo_quirk_bit {
	CTLV_TEST_ECHO_QUIRK_SLOW_BIT = 0,
	CTLV_TEST_ECHO_QUIRK_NOISY_BIT = 1,
};

/* Bit 0, which means nothing of its own. */
#define CTLV_TEST_ECHO_QUIRK_SLOW _BITULL(CTLV_TEST_ECHO_QUIRK_SLOW_BIT)
/* Bit 1 and the highest, likewise. */
#define CTLV_TEST_ECHO_QUIRK_NOISY _BITULL(CTLV_TEST_ECHO_QUIRK_NOISY_BIT)
#define CTLV_TEST_ECHO_QUIRK_MASK \
	(CTLV_TEST_ECHO_QUIRK_SLOW | \
	 CTLV_TEST_ECHO_QUIRK_NOISY)

/* echo-values */
/* The one required field, an integer bounded on both sides. */
#define CTLV_TEST_T_ECHO_VALUES_U \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 1)
/* A signed integer, bounded on both sides. */
#define CTLV_TEST_T_ECHO_VALUES_S \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 2)
/* A bool. */
#define CTLV_TEST_T_ECHO_VALUES_FLAG \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 3)
/* An enum. */
#define CTLV_TEST_T_ECHO_VALUES_MODE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 4)
/* A flags. */
#define CTLV_TEST_T_ECHO_VALUES_QUIRKS \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 5)
/* A string of 1 to 15 characters. */
#define CTLV_TEST_T_ECHO_VALUES_TEXT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 6)
/* A binary of 4 to 32 bytes. */
#define CTLV_TEST_T_ECHO_VALUES_DATA \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 7)
/* An integer that may occur three times. */
#define CTLV_TEST_T_ECHO_VALUES_COUNT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 8)
/* A record in place that may occur twice. */
#define CTLV_TEST_T_ECHO_VALUES_ITEM \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 9)
/* Filled by ctlv-test-child, and empty on a ctlv-test device. */
#define CTLV_TEST_T_ECHO_VALUES_EXTENSION \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 10)
/* Valued by ctlv-test-child, and taking nothing on a ctlv-test device. */
#define CTLV_TEST_T_ECHO_VALUES_VARIANT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00010000, 11)

/* echo-values.item */
/* A member the item requires. */
#define CTLV_TEST_T_ECHO_VALUES_ITEM_INDEX \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00011000, 1)
/* A member it may leave out. */
#define CTLV_TEST_T_ECHO_VALUES_ITEM_LABEL \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00011000, 2)
/* The third level, where a record inside a record ends. */
#define CTLV_TEST_T_ECHO_VALUES_ITEM_LEAF \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00011000, 3)

/* echo-values.item.leaf */
/* The leaf's one member. */
#define CTLV_TEST_T_ECHO_VALUES_LEAF_INDEX \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00011100, 1)

/* echo-message */
/* Any backing at eight bytes. */
#define CTLV_TEST_T_ECHO_MESSAGE_REGION \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00020000, 1)
/* A memfd at a page. */
#define CTLV_TEST_T_ECHO_MESSAGE_PAGE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00020000, 2)

/* token-info */
/* What probe-get answers with as it was sent. */
#define CTLV_TEST_T_TOKEN_INFO_TOKEN \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x00030000, 1)

/* probe-get reply */
/* Filled by ctlv-test-child, so one selftest run sees both states. */
#define CTLV_TEST_T_PROBE_GET_EXTENSION \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0020000, 1)

/* series-get request */
/* How many chunks. */
#define CTLV_TEST_T_SERIES_GET_CHUNKS \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80080000, 1)
/* How many items in each chunk. */
#define CTLV_TEST_T_SERIES_GET_ITEMS \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80080000, 2)
/* How many values in each item. */
#define CTLV_TEST_T_SERIES_GET_VALUES \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80080000, 3)
/* How many totals after the chunks. */
#define CTLV_TEST_T_SERIES_GET_TOTALS \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80080000, 4)

/* series-get reply */
/* One chunk, as many as chunks asks for. */
#define CTLV_TEST_T_SERIES_GET_CHUNK \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0080000, 1)
/* One total, as many as totals asks for, numbered from one. */
#define CTLV_TEST_T_SERIES_GET_TOTAL \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0080000, 2)

/* series-get chunk */
/* The chunk's number, from one. */
#define CTLV_TEST_T_SERIES_GET_CHUNK_INDEX \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0081000, 1)
/* One item, as many as items asks for. */
#define CTLV_TEST_T_SERIES_GET_CHUNK_ITEM \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0081000, 2)

/* series-get chunk.item */
/* The item's number in its chunk, from one. */
#define CTLV_TEST_T_SERIES_GET_ITEM_INDEX \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0081100, 1)
/*
 * One value, as many as values asks for, each the chunk's number times 100, plus the item's times
 * 10, plus its own.
 */
#define CTLV_TEST_T_SERIES_GET_ITEM_VALUE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc0081100, 2)

/* ping request */
/* How many events, one when absent. */
#define CTLV_TEST_T_PING_COUNT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80090000, 1)
/* Held against each event of the burst. */
#define CTLV_TEST_T_PING_SERIAL \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80090000, 2)
/* How many bytes of filler each event carries. */
#define CTLV_TEST_T_PING_PAYLOAD \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80090000, 3)
/* Build every event of the burst and throw it away instead of emitting it. */
#define CTLV_TEST_T_PING_ABORT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80090000, 4)
/* Emit the event that carries nothing: a header and an empty root. */
#define CTLV_TEST_T_PING_EMPTY \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x80090000, 5)

/* beeped payload */
/* The request's serial plus the event's place in its burst, from zero. */
#define CTLV_TEST_T_BEEPED_SERIAL \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc00a0000, 1)
/* As many bytes as the request's payload asks for, byte n being n % 256. */
#define CTLV_TEST_T_BEEPED_FILLER \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc00a0000, 2)

/* transfer request */
/*
 * What is summed. It and the two blobs below take up to a quarter of a gibibyte each, more than an
 * inline message can hold.
 */
#define CTLV_TEST_T_TRANSFER_SOURCE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 1)
/* What the pattern is written over. */
#define CTLV_TEST_T_TRANSFER_TARGET \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 2)
/* What is summed and then written over. */
#define CTLV_TEST_T_TRANSFER_SCRATCH \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 3)
/* A blob a request may carry more than one of, each its own acquisition. */
#define CTLV_TEST_T_TRANSFER_PIECE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 4)
/* What the pattern a write leaves behind is seeded from. */
#define CTLV_TEST_T_TRANSFER_FILL \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 5)
/* Resolve nothing at all, the descriptors being valid and unused. */
#define CTLV_TEST_T_TRANSFER_SKIP \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 6)
/* Resolve the source twice, two calls being two acquisitions. */
#define CTLV_TEST_T_TRANSFER_TWICE \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 7)
/*
 * Hold everything resolved until the module is told to let go, so a test can close, unmap or reuse
 * what a descriptor named while the kernel has it.
 */
#define CTLV_TEST_T_TRANSFER_HOLD \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0x800b0000, 8)

/* transfer reply */
/* What every readable byte added up to. */
#define CTLV_TEST_T_TRANSFER_SUM \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc00b0000, 1)
/* How many bytes were written. */
#define CTLV_TEST_T_TRANSFER_WRITTEN \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc00b0000, 2)
/* Whether resolving one descriptor twice produced two objects. */
#define CTLV_TEST_T_TRANSFER_DISTINCT \
	CTLV_TYPE(CTLV_FAMILY_TEST, 0xc00b0000, 3)

/* ops */
#define CTLV_TEST_OP_NOOP CTLV_OP(CTLV_FAMILY_TEST, 1)
#define CTLV_TEST_OP_PROBE_GET CTLV_OP(CTLV_FAMILY_TEST, 2)
#define CTLV_TEST_OP_DYNAMIC_NOOP CTLV_OP(CTLV_FAMILY_TEST, 3)
#define CTLV_TEST_OP_DYNAMIC_TOGGLE CTLV_OP(CTLV_FAMILY_TEST, 4)
#define CTLV_TEST_OP_PINGED CTLV_OP(CTLV_FAMILY_TEST, 5)
#define CTLV_TEST_OP_ECHO CTLV_OP(CTLV_FAMILY_TEST, 6)
#define CTLV_TEST_OP_ECHO_REPLY CTLV_OP(CTLV_FAMILY_TEST, 7)
#define CTLV_TEST_OP_SERIES_GET CTLV_OP(CTLV_FAMILY_TEST, 8)
#define CTLV_TEST_OP_PING CTLV_OP(CTLV_FAMILY_TEST, 9)
#define CTLV_TEST_OP_BEEPED CTLV_OP(CTLV_FAMILY_TEST, 10)
#define CTLV_TEST_OP_TRANSFER CTLV_OP(CTLV_FAMILY_TEST, 11)

#endif /* _UAPI_CTLV_CTLV_TEST_GEN_H */
