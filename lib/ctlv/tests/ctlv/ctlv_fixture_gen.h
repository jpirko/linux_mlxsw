/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Generated from tools/ctlv/tests/specs/ctlv-fixture.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-fixture CTLV family. Types and op IDs are complete; the message format
 * carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv header is included here, so one include covers the whole effective schema.
 */

#ifndef _UAPI_CTLV_CTLV_FIXTURE_GEN_H
#define _UAPI_CTLV_CTLV_FIXTURE_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>
#include <ctlv/ctlv_gen.h>
#include <linux/const.h>
#include <linux/hash_info.h>

/* enum mode */
enum _ctlv_fixture_mode {
	/* The first value. */
	CTLV_FIXTURE_MODE_DISABLED = 1,
	/* The second value. */
	CTLV_FIXTURE_MODE_ENABLED = 2,
	/* The third value. */
	CTLV_FIXTURE_MODE_AUTO = 3,
};

/* flags quirks */
enum _ctlv_fixture_quirks_bit {
	CTLV_FIXTURE_QUIRKS_SLOW_BIT = 0,
	CTLV_FIXTURE_QUIRKS_NOISY_BIT = 1,
	CTLV_FIXTURE_QUIRKS_FLAKY_BIT = 2,
};

/* Bit 0. */
#define CTLV_FIXTURE_QUIRKS_SLOW _BITULL(CTLV_FIXTURE_QUIRKS_SLOW_BIT)
/* Bit 1. */
#define CTLV_FIXTURE_QUIRKS_NOISY _BITULL(CTLV_FIXTURE_QUIRKS_NOISY_BIT)
/* Bit 2. */
#define CTLV_FIXTURE_QUIRKS_FLAKY _BITULL(CTLV_FIXTURE_QUIRKS_FLAKY_BIT)
#define CTLV_FIXTURE_QUIRKS_MASK \
	(CTLV_FIXTURE_QUIRKS_SLOW | \
	 CTLV_FIXTURE_QUIRKS_NOISY | \
	 CTLV_FIXTURE_QUIRKS_FLAKY)

/* scalar */
/* No bounds at all, so both signed defaults have to be filled in. */
#define CTLV_FIXTURE_T_SCALAR_DELTA \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 1)
/* An integer with a unit and a scale. */
#define CTLV_FIXTURE_T_SCALAR_FREQ \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 2)
#define CTLV_FIXTURE_T_SCALAR_FREQ_UNIT CTLV_UNIT_HERTZ
#define CTLV_FIXTURE_T_SCALAR_FREQ_SCALE 3
/* An integer with a unit and no scale. */
#define CTLV_FIXTURE_T_SCALAR_SIZE \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 3)
#define CTLV_FIXTURE_T_SCALAR_SIZE_UNIT CTLV_UNIT_BYTES
#define CTLV_FIXTURE_T_SCALAR_SIZE_SCALE 0
/* An enum. */
#define CTLV_FIXTURE_T_SCALAR_MODE \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 4)
/* A flags. */
#define CTLV_FIXTURE_T_SCALAR_QUIRKS \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 5)
/* One byte, and nothing a bound or a unit could apply to. */
#define CTLV_FIXTURE_T_SCALAR_FLAG \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 6)
/* A string bounded on both sides. */
#define CTLV_FIXTURE_T_SCALAR_TEXT \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 7)
/* A binary of one length. */
#define CTLV_FIXTURE_T_SCALAR_FIXED \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 8)
/* A binary bounded on both sides. */
#define CTLV_FIXTURE_T_SCALAR_BOUNDED \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00010000, 9)

/* u */
/* Placed wherever a key or a pair is, with one type everywhere. */
#define CTLV_FIXTURE_T_U \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00020000, 1)

/* s */
/* Placed as u is, and signed. */
#define CTLV_FIXTURE_T_S \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x00030000, 1)

/* counts-get request */
/* An integer that may occur three times. */
#define CTLV_FIXTURE_T_COUNTS_GET_FEW \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 1)
/* An enum. */
#define CTLV_FIXTURE_T_COUNTS_GET_MODE \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 2)
/* A named record that may occur twice. */
#define CTLV_FIXTURE_T_COUNTS_GET_REQUEST \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 3)
/* An enum a header numbers. */
#define CTLV_FIXTURE_T_COUNTS_GET_ALGO \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 4)
/* An integer that may occur once per value of an enum. */
#define CTLV_FIXTURE_T_COUNTS_GET_PER_MODE \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 5)
/* Each value of the header's enum, at most once. */
#define CTLV_FIXTURE_T_COUNTS_GET_ALGOS \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80030000, 6)

/* counts-get reply */
/* An integer that may occur as often as the reply has room. */
#define CTLV_FIXTURE_T_COUNTS_GET_MANY \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0xc0030000, 1)

/* records-dump reply */
/* A record in place that occurs twice. */
#define CTLV_FIXTURE_T_RECORDS_DUMP_LEVEL1 \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0xc0040000, 1)
/* Filled by ctlv-fixture-child. */
#define CTLV_FIXTURE_T_RECORDS_DUMP_EXTENSION \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0xc0040000, 2)

/* records-dump level1 */
/* A record in place one level in, unlimited. */
#define CTLV_FIXTURE_T_RECORDS_DUMP_LEVEL1_LEVEL2 \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0xc0041000, 1)

/* records-dump level1.level2 */
/* A named record two levels in, unlimited. */
#define CTLV_FIXTURE_T_RECORDS_DUMP_LEVEL2_LEVEL3 \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0xc0041100, 1)

/* blobs-submit request */
/* Memory of the caller, which the device reads. */
#define CTLV_FIXTURE_T_BLOBS_SUBMIT_INPUT_VA \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80050000, 1)
/* A memfd or a dma-buf at a page, which the device writes. */
#define CTLV_FIXTURE_T_BLOBS_SUBMIT_OUTPUT_FD \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80050000, 2)
/* Any backing at eight bytes, which the device reads and writes. */
#define CTLV_FIXTURE_T_BLOBS_SUBMIT_INOUT_ANY \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80050000, 3)
/* Memory of the caller, up to three blobs of it. */
#define CTLV_FIXTURE_T_BLOBS_SUBMIT_PIECES \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80050000, 4)

/* optional-check request */
/* An enum. */
#define CTLV_FIXTURE_T_OPTIONAL_CHECK_MODE \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80060000, 1)
/* An enum whose values the device's family gives. */
#define CTLV_FIXTURE_T_OPTIONAL_CHECK_VARIANT \
	CTLV_TYPE(CTLV_FAMILY_FIXTURE, 0x80060000, 2)

/* ops */
#define CTLV_FIXTURE_OP_NOOP CTLV_OP(CTLV_FAMILY_FIXTURE, 1)
#define CTLV_FIXTURE_OP_SCALARS_SET CTLV_OP(CTLV_FAMILY_FIXTURE, 2)
#define CTLV_FIXTURE_OP_COUNTS_GET CTLV_OP(CTLV_FAMILY_FIXTURE, 3)
#define CTLV_FIXTURE_OP_RECORDS_DUMP CTLV_OP(CTLV_FAMILY_FIXTURE, 4)
#define CTLV_FIXTURE_OP_BLOBS_SUBMIT CTLV_OP(CTLV_FAMILY_FIXTURE, 5)
#define CTLV_FIXTURE_OP_OPTIONAL_CHECK CTLV_OP(CTLV_FAMILY_FIXTURE, 6)
#define CTLV_FIXTURE_OP_THING_CHANGED CTLV_OP(CTLV_FAMILY_FIXTURE, 7)
#define CTLV_FIXTURE_OP_PINGED CTLV_OP(CTLV_FAMILY_FIXTURE, 8)

#endif /* _UAPI_CTLV_CTLV_FIXTURE_GEN_H */
