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
	/* The certificates that chain a signing key to its vendor's root. */
	CTLV_ATTESTATION_EVIDENCE_CLASS_CERT_CHAIN = 3,
	/* Anything else a verifier reads beside the signed objects. */
	CTLV_ATTESTATION_EVIDENCE_CLASS_AUXILIARY = 4,
};

/* enum evidence-format */
enum _ctlv_attestation_evidence_format {
	/* The log area of an ACPI CCEL table, a TCG crypto-agile event log. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL = 1,
	/* An SPDM certificate chain. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_SPDM_CERT_CHAIN = 2,
	/* An SPDM message transcript, starting with VCA. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_SPDM_TRANSCRIPT = 3,
	/* An SPDM GET_MEASUREMENTS response. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_SPDM_MEASUREMENTS = 4,
	/* A TDISP interface report. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_TDISP_REPORT = 5,
	/* TPM2_Quote's attestation structure and signature. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_QUOTE = 6,
	/* A TCG PC Client crypto-agile event log. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EVENT_LOG = 7,
	/* A TPM's EK certificate, read from its NV. */
	CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EK_CERT = 8,
};

/* enum mr-log-event */
enum _ctlv_attestation_mr_log_event {
	/* Describes an MR. */
	CTLV_ATTESTATION_MR_LOG_EVENT_INFO = 1,
	/* Records an extension of an MR. */
	CTLV_ATTESTATION_MR_LOG_EVENT_EXTEND = 2,
};

/* enum tdisp-state */
enum _ctlv_attestation_tdisp_state {
	/* TDISP's CONFIG_UNLOCKED. */
	CTLV_ATTESTATION_TDISP_STATE_UNLOCKED = 1,
	/* TDISP's CONFIG_LOCKED. */
	CTLV_ATTESTATION_TDISP_STATE_LOCKED = 2,
	/* TDISP's RUN. */
	CTLV_ATTESTATION_TDISP_STATE_RUN = 3,
	/* TDISP's ERROR. */
	CTLV_ATTESTATION_TDISP_STATE_ERROR = 4,
};

/* flags mr-device-caps */
enum _ctlv_attestation_mr_device_caps_bit {
	CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND_BIT = 0,
	CTLV_ATTESTATION_MR_DEVICE_CAPS_DYNAMIC_TOPOLOGY_BIT = 1,
};

/* mr-extend is provided. */
#define CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND _BITULL(CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND_BIT)
/* MRs may come and go while the device lives. */
#define CTLV_ATTESTATION_MR_DEVICE_CAPS_DYNAMIC_TOPOLOGY \
	_BITULL(CTLV_ATTESTATION_MR_DEVICE_CAPS_DYNAMIC_TOPOLOGY_BIT)
#define CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK \
	(CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND | \
	 CTLV_ATTESTATION_MR_DEVICE_CAPS_DYNAMIC_TOPOLOGY)

/* flags mr-caps */
enum _ctlv_attestation_mr_caps_bit {
	CTLV_ATTESTATION_MR_CAPS_EXTEND_BIT = 0,
	CTLV_ATTESTATION_MR_CAPS_REMOVABLE_BIT = 1,
};

/* The MR may be extended. */
#define CTLV_ATTESTATION_MR_CAPS_EXTEND _BITULL(CTLV_ATTESTATION_MR_CAPS_EXTEND_BIT)
/* The provider may remove the MR. */
#define CTLV_ATTESTATION_MR_CAPS_REMOVABLE _BITULL(CTLV_ATTESTATION_MR_CAPS_REMOVABLE_BIT)
#define CTLV_ATTESTATION_MR_CAPS_MASK \
	(CTLV_ATTESTATION_MR_CAPS_EXTEND | \
	 CTLV_ATTESTATION_MR_CAPS_REMOVABLE)

/* flags mr-state-flags */
enum _ctlv_attestation_mr_state_flags_bit {
	CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE_BIT = 0,
};

/* IMA extends this MR. */
#define CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE \
	_BITULL(CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE_BIT)
#define CTLV_ATTESTATION_MR_STATE_FLAGS_MASK CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE

/* flags evidence-caps */
enum _ctlv_attestation_evidence_caps_bit {
	CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT_BIT = 0,
	CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION_BIT = 1,
	CTLV_ATTESTATION_EVIDENCE_CAPS_PRIVLEVEL_BIT = 2,
	CTLV_ATTESTATION_EVIDENCE_CAPS_SERVICE_BIT = 3,
};

/* evidence-accept is provided. */
#define CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT \
	_BITULL(CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT_BIT)
/* evidence-get takes mr-selection. */
#define CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION \
	_BITULL(CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION_BIT)
/* evidence-get takes privlevel. */
#define CTLV_ATTESTATION_EVIDENCE_CAPS_PRIVLEVEL \
	_BITULL(CTLV_ATTESTATION_EVIDENCE_CAPS_PRIVLEVEL_BIT)
/* evidence-get takes service. */
#define CTLV_ATTESTATION_EVIDENCE_CAPS_SERVICE _BITULL(CTLV_ATTESTATION_EVIDENCE_CAPS_SERVICE_BIT)
#define CTLV_ATTESTATION_EVIDENCE_CAPS_MASK \
	(CTLV_ATTESTATION_EVIDENCE_CAPS_EVIDENCE_ACCEPT | \
	 CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION | \
	 CTLV_ATTESTATION_EVIDENCE_CAPS_PRIVLEVEL | \
	 CTLV_ATTESTATION_EVIDENCE_CAPS_SERVICE)

/* mr-desc */
/* The platform's name for the MR, such as rtmr0. */
#define CTLV_ATTESTATION_T_MR_DESC_NAME \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 1)
/* Its banks, each once, in the provider's order. */
#define CTLV_ATTESTATION_T_MR_DESC_SUPPORTED_HASH_ALGORITHMS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 2)
/* What it allows, and whether the provider may remove it. */
#define CTLV_ATTESTATION_T_MR_DESC_CAPS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 3)
/* The hardware's number for the MR, where it has one: 0 for rtmr0. */
#define CTLV_ATTESTATION_T_MR_DESC_REGISTER_INDEX \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 4)
/* ima-active while IMA extends it. */
#define CTLV_ATTESTATION_T_MR_DESC_STATE_FLAGS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00010000, 5)

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

/* mr-log-entry */
/* The ID of the MR the entry is about. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00050000, 1)
/* What the entry records. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_EVENT \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00050000, 2)
/* The MR an info entry describes. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00050000, 3)
/* IMA's measurement, on an extend entry IMA made. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00050000, 4)
/* What was measured, on an extend entry mr-extend made with a record. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00050000, 5)

/* mr-log-entry.mr */
/* The platform's name for the MR. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_NAME \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00051000, 1)
/* Its banks, each once, in the provider's order. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_SUPPORTED_HASH_ALGORITHMS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00051000, 2)
/* What it allows, and whether the provider may remove it. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_CAPS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00051000, 3)
/* The hardware's number for the MR, where it has one. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_MR_REGISTER_INDEX \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00051000, 4)

/* mr-log-entry.ima */
/* The IMA template, such as ima-ng. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_NAME \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00052000, 1)
/* The template's fields, as IMA's binary measurement list has them. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_TEMPLATE_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00052000, 2)
/* The entry records an IMA violation, not a measurement. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_IMA_VIOLATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00052000, 3)

/* mr-log-entry.record */
/* The layout of data, in printable ASCII without spaces. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_FORMAT \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00053000, 1)
/* What was measured, laid out as format says. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_RECORD_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00053000, 2)

/* mr-start-generation */
/* The first MR generation of the range mr-log-get selects and reports. */
#define CTLV_ATTESTATION_T_MR_START_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00060000, 1)

/* mr-end-generation */
/* The last MR generation of that range. */
#define CTLV_ATTESTATION_T_MR_END_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00070000, 1)

/* device-generation */
/* Advances with each change of the device's state. */
#define CTLV_ATTESTATION_T_DEVICE_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x00080000, 1)

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
/* The optional evidence features the device provides. */
#define CTLV_ATTESTATION_T_DEVICE_GET_EVIDENCE_FLAGS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011100, 2)

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
/* Tells apart the objects of one kind, such as an SPDM slot. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_INSTANCE \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 7)
/* Where the TCB vouches for the object with a digest, its algorithm. */
#define CTLV_ATTESTATION_T_DEVICE_GET_OBJECTS_TCB_DIGEST_ALGO \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0011110, 8)

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
/* The MR operations the device provides, and whether MRs come and go. */
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
/* The MR banks the evidence reports, where the device takes mr-selection. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 3)
/* The privilege level to report for, as configfs-tsm's privlevel. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_PRIVLEVEL \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 4)
/*
 * The service to attest, as configfs-tsm's service_provider; only with service-guid and
 * service-manifest-version.
 */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_PROVIDER \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 5)
/* The service's GUID, as configfs-tsm's service_guid. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_GUID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 6)
/* Its manifest version, as configfs-tsm's service_manifest_version. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SERVICE_MANIFEST_VERSION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 7)
/* The device's family's fields. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_OPTIONS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80020000, 8)

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

/* evidence-get selection */
/* The MR's ID. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80022000, 1)
/* The bank's algorithm. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_SELECTION_HASH_ALGO \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80022000, 2)

/* evidence-get reply */
/* One per object, in request order. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0020000, 1)
/* On a device with evidence-accept, numbers this evidence for it. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_EVIDENCE_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0020000, 2)

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
/* Where the TCB vouches for the object, the digest it vouches with. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_RESULTS_TCB_DIGEST \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021000, 4)

/* evidence-get results.tcb-digest */
/* The object's tcb-digest-algo. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_HASH_ALGO \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021100, 1)
/* The digest, as long as hash-algo's. */
#define CTLV_ATTESTATION_T_EVIDENCE_GET_TCB_DIGEST_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0021100, 2)

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
/* The device's family's fields. */
#define CTLV_ATTESTATION_T_MR_READ_OPTIONS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80040000, 2)

/* mr-extend request */
/* The MR to extend, by the ID mr-get lists. */
#define CTLV_ATTESTATION_T_MR_EXTEND_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80050000, 1)
/* Data the device hashes for each bank, in place of the caller's digests. */
#define CTLV_ATTESTATION_T_MR_EXTEND_RAW_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80050000, 2)
/* The device's family's fields. */
#define CTLV_ATTESTATION_T_MR_EXTEND_OPTIONS \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80050000, 3)
/* What the data extended by is, recorded in the journal with it. */
#define CTLV_ATTESTATION_T_MR_EXTEND_RECORD \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80050000, 4)

/* mr-extend record */
/* The layout of data, in printable ASCII without spaces. */
#define CTLV_ATTESTATION_T_MR_EXTEND_RECORD_FORMAT \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80051000, 1)
/* What was measured, laid out as format says. */
#define CTLV_ATTESTATION_T_MR_EXTEND_RECORD_DATA \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80051000, 2)

/* mr-log-entry-appended payload */
/* The entry, as mr-log-get returns it. */
#define CTLV_ATTESTATION_T_MR_LOG_ENTRY_APPENDED_ENTRY \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0070000, 1)

/* evidence-accept request */
/* The evidence the verifier accepted, as evidence-get numbered it. */
#define CTLV_ATTESTATION_T_EVIDENCE_ACCEPT_EVIDENCE_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x80080000, 1)

/* devsec-state-get reply */
/* The device interface's TDISP state, where the provider reports one. */
#define CTLV_ATTESTATION_T_DEVSEC_STATE_GET_TDISP_STATE \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc0090000, 1)

/* evidence-state-changed payload */
/* The latest evidence, once evidence of this device state exists. */
#define CTLV_ATTESTATION_T_EVIDENCE_STATE_CHANGED_EVIDENCE_GENERATION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc00a0000, 1)

/* mr-log-get request */
/* Only these MRs' entries; every MR's without it. */
#define CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x800b0000, 1)

/* mr-log-get selection */
/* An MR's ID. */
#define CTLV_ATTESTATION_T_MR_LOG_GET_SELECTION_ID \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0x800b1000, 1)

/* mr-log-get reply */
/* The entries, in journal order. */
#define CTLV_ATTESTATION_T_MR_LOG_GET_ENTRIES \
	CTLV_TYPE(CTLV_FAMILY_ATTESTATION, 0xc00b0000, 1)

/* ops */
#define CTLV_ATTESTATION_OP_DEVICE_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 1)
#define CTLV_ATTESTATION_OP_EVIDENCE_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 2)
#define CTLV_ATTESTATION_OP_MR_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 3)
#define CTLV_ATTESTATION_OP_MR_READ CTLV_OP(CTLV_FAMILY_ATTESTATION, 4)
#define CTLV_ATTESTATION_OP_MR_EXTEND CTLV_OP(CTLV_FAMILY_ATTESTATION, 5)
#define CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED CTLV_OP(CTLV_FAMILY_ATTESTATION, 6)
#define CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED CTLV_OP(CTLV_FAMILY_ATTESTATION, 7)
#define CTLV_ATTESTATION_OP_EVIDENCE_ACCEPT CTLV_OP(CTLV_FAMILY_ATTESTATION, 8)
#define CTLV_ATTESTATION_OP_DEVSEC_STATE_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 9)
#define CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED CTLV_OP(CTLV_FAMILY_ATTESTATION, 10)
#define CTLV_ATTESTATION_OP_MR_LOG_GET CTLV_OP(CTLV_FAMILY_ATTESTATION, 11)

#endif /* _UAPI_CTLV_CTLV_ATTESTATION_GEN_H */
