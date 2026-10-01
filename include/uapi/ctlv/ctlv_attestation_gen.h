/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN uapi header */

/*
 * Constants of the ctlv-attestation CTLV family. Types and op IDs are complete; the message format
 * carrying them is in <ctlv/ctlv.h>.
 *
 * The inherited ctlv header is included here, so one include covers the whole effective schema.
 */

#ifndef _UAPI_CTLV_CTLV_ATTESTATION_GEN_H
#define _UAPI_CTLV_CTLV_ATTESTATION_GEN_H

#include <ctlv/ctlv.h>
#include <ctlv/ctlv_families_gen.h>
#include <ctlv/ctlv_gen.h>
#include <linux/const.h>
#include <linux/hash_info.h>

/* enum evidence-class */
enum _ctlv_attestation_evidence_class {
	/* Made by evidence-get over its challenge, and signed by the root of trust. */
	CTLV_ATTESTATION_EVIDENCE_CLASS_SIGNED = 1,
	/*
	 * The platform's log of what firmware and the boot chain measured into the MRs. Later
	 * extends, through mr-extend or otherwise, are not in it.
	 */
	CTLV_ATTESTATION_EVIDENCE_CLASS_EVENT_LOG = 2,
};

/* enum evidence-format */
enum _ctlv_attestation_evidence_format {
	/* The log area of an ACPI CCEL table, a TCG crypto-agile event log. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL = 1,
};

/* flags mr-device-caps */
enum _ctlv_attestation_mr_device_caps_bit {
	CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND_BIT = 0,
};

/* mr-extend is provided. */
#define CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND _BITULL(CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND_BIT)
#define CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND

/* flags mr-caps */
enum _ctlv_attestation_mr_caps_bit {
	CTLV_ATTESTATION_MR_CAPS_EXTEND_BIT = 0,
};

/* The MR may be extended. */
#define CTLV_ATTESTATION_MR_CAPS_EXTEND _BITULL(CTLV_ATTESTATION_MR_CAPS_EXTEND_BIT)
#define CTLV_ATTESTATION_MR_CAPS_MASK CTLV_ATTESTATION_MR_CAPS_EXTEND

/* mr-desc */
/* The platform's name for the MR, such as rtmr0. */
#define CTLV_ATTESTATION_T_MR_DESC_NAME \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 1)
/* Its banks, each once, in the provider's order. */
#define CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 2)
/* What it allows. */
#define CTLV_ATTESTATION_T_MR_DESC_CAPS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 3)
/* The hardware's number for the MR, where it has one: 0 for rtmr0. */
#define CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 4)

/* mr-value */
/* The bank's algorithm. */
#define CTLV_ATTESTATION_T_MR_VALUE_HASH_ALGO \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00020000, 1)
/* The digest, as long as hash-algo's. */
#define CTLV_ATTESTATION_T_MR_VALUE_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00020000, 2)

/* mr-generation */
/* The device's MR generation, which each extend bumps. */
#define CTLV_ATTESTATION_T_MR_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00030000, 1)

/* mr-values */
/* Values for banks of the MR, each bank once. */
#define CTLV_ATTESTATION_T_MR_VALUES \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00040000, 1)

/* device-get reply */
/* What the device supports, fixed for its life. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CAPS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0010000, 1)
/* The device's family's fields. */
#define CTLV_ATTESTATION_T_DEVICE_GET_INFO \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0010000, 2)

/* device-get caps */
/* Present when the device produces evidence. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CAPS_EVIDENCE \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011000, 1)
/* Present when the device has MRs. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CAPS_MR \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011000, 2)

/* device-get caps.evidence */
/* One per object evidence-get can produce. */
#define CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_OBJECTS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011100, 1)

/* device-get caps.evidence.objects */
/* The device's handle for the object in requests. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 1)
/* What the object is. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CLASS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 2)
/*
 * The layouts a standard defines that the object may have. At least one of formats and
 * vendor-formats is present.
 */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_FORMATS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 3)
/* The layouts its vendor defines that the object may have. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_VENDOR_FORMATS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 4)
/* The longest the object gets; a blob this long always holds it. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_MAX_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 5)
/*
 * A signed object's challenge lengths: each entry is one length, or a range from min-length to
 * max-length.
 */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_CHALLENGE_LENGTHS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 6)

/* device-get caps.evidence.objects.challenge-lengths */
/* A single length. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011111, 1)
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH_UNIT CTLV_UNIT_BYTES
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_LENGTH_SCALE 0
/* The shortest of a range. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011111, 2)
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH_UNIT CTLV_UNIT_BYTES
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MIN_LENGTH_SCALE 0
/* The longest of a range. */
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011111, 3)
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH_UNIT CTLV_UNIT_BYTES
#define CTLV_ATTESTATION_T_DEVICE_GET_CHALLENGE_LENGTHS_MAX_LENGTH_SCALE 0

/* device-get caps.mr */
/* The MR operations the device provides. */
#define CTLV_ATTESTATION_T_DEVICE_GET_MR_FLAGS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011200, 1)

/* evidence-get request */
/*
 * Bound into each signed object named: pass a verifier's nonce, or a hash binding one to your
 * context, never data a peer chose. Present exactly when a signed object is named, and of a length
 * each of them takes.
 */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_CHALLENGE \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 1)
/* The objects to produce, each named once. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 2)

/* evidence-get objects */
/* The object, by the ID device-get lists. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80021000, 1)
/*
 * Where the object is written. One too short for it fails the op with EMSGSIZE, and error-info's
 * required-length says how long it has to be.
 */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_OBJECTS_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80021000, 2)

/* evidence-get reply */
/* One per object, in request order. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0020000, 1)

/* evidence-get results */
/*
 * The layout it came in, where a standard defines it. Exactly one of format and vendor-format is
 * present.
 */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_FORMAT \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021000, 1)
/* The layout it came in, where its vendor defines it. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_VENDOR_FORMAT \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021000, 2)
/* The bytes written to its blob. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_LENGTH \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021000, 3)

/* mr-get request */
/* Only this MR; every MR without it. */
#define CTLV_ATTESTATION_T_MR_GET_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80030000, 1)

/* mr-get reply */
/* One per MR, ordered by ID. */
#define CTLV_ATTESTATION_T_MR_GET_ITEMS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0030000, 1)

/* mr-get items */
/* The device's handle for the MR in requests. */
#define CTLV_ATTESTATION_T_MR_GET_ITEMS_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0031000, 1)

/* mr-read request */
/* The MR to read, by the ID mr-get lists. */
#define CTLV_ATTESTATION_T_MR_READ_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80040000, 1)

/* mr-extend request */
/* The MR to extend, by the ID mr-get lists. */
#define CTLV_ATTESTATION_T_MR_EXTEND_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80050000, 1)

/* ops */
#define CTLV_ATTESTATION_OP_DEVICE_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 1)
#define CTLV_ATTESTATION_OP_EVIDENCE_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 2)
#define CTLV_ATTESTATION_OP_MR_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 3)
#define CTLV_ATTESTATION_OP_MR_READ CTLV_OP(CTLV_FAMILY_ATTESTATION, 4)
#define CTLV_ATTESTATION_OP_MR_EXTEND CTLV_OP(CTLV_FAMILY_ATTESTATION, 5)

#endif /* _UAPI_CTLV_CTLV_ATTESTATION_GEN_H */
