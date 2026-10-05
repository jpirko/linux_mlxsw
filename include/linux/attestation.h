/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_ATTESTATION_H
#define _LINUX_ATTESTATION_H

#include <crypto/hash.h>
#include <linux/types.h>

#include <uapi/linux/ctlv_attestation_gen.h>
#include <uapi/linux/hash_info.h>

struct attestation;
struct ctlv_blob;

enum attestation_tdisp_state {
	ATTESTATION_TDISP_STATE_UNLOCKED =
		CTLV_ATTESTATION_TDISP_STATE_UNLOCKED,
	ATTESTATION_TDISP_STATE_LOCKED = CTLV_ATTESTATION_TDISP_STATE_LOCKED,
	ATTESTATION_TDISP_STATE_RUN = CTLV_ATTESTATION_TDISP_STATE_RUN,
	ATTESTATION_TDISP_STATE_ERROR = CTLV_ATTESTATION_TDISP_STATE_ERROR,
};

enum attestation_device_trust_state {
	ATTESTATION_DEVICE_TRUST_UNSET =
		CTLV_ATTESTATION_DEVICE_TRUST_STATE_UNSET,
	ATTESTATION_DEVICE_TRUST_NONE =
		CTLV_ATTESTATION_DEVICE_TRUST_STATE_NONE,
	ATTESTATION_DEVICE_TRUST_ADVERSARY =
		CTLV_ATTESTATION_DEVICE_TRUST_STATE_ADVERSARY,
	ATTESTATION_DEVICE_TRUST_AUTO =
		CTLV_ATTESTATION_DEVICE_TRUST_STATE_AUTO,
	ATTESTATION_DEVICE_TRUST_TCB = CTLV_ATTESTATION_DEVICE_TRUST_STATE_TCB,
};

/**
 * struct attestation_digest - One MR digest.
 * @algorithm: hash algorithm
 * @data: digest bytes
 */
struct attestation_digest {
	enum hash_algo algorithm;
	u8 data[HASH_MAX_DIGESTSIZE];
};

/**
 * struct attestation_digest_set - Digests ordered as the MR algorithms.
 * @values_count: number of digests
 * @values: digest array
 */
struct attestation_digest_set {
	size_t values_count;
	struct attestation_digest *values;
};

/**
 * struct attestation_mr_props - Immutable MR properties.
 * @id: provider-unique MR ID
 * @name: MR name
 * @algorithms: ordered hash algorithms
 * @algorithms_count: number of algorithms
 * @has_register_index: whether @register_index is valid
 * @register_index: native register index
 * @readable: MR can be read
 * @extendable: MR can be extended
 */
struct attestation_mr_props {
	unsigned int id;
	const char *name;
	const enum hash_algo *algorithms;
	size_t algorithms_count;
	bool has_register_index;
	unsigned int register_index;
	bool readable;
	bool extendable;
};

/**
 * struct attestation_evidence_supported_object - Evidence output description.
 * @format: evidence format name
 * @max_length: maximum output bytes
 * @binds_mrs: evidence validity depends on MR state
 */
struct attestation_evidence_supported_object {
	const char *format;
	size_t max_length;
	bool binds_mrs;
};

/**
 * struct attestation_evidence_caps - Immutable evidence capabilities.
 * @min_challenge_length: minimum challenge bytes
 * @max_challenge_length: maximum challenge bytes
 * @max_total_output_length: maximum bytes across all objects
 * @objects: supported evidence objects
 * @objects_count: number of objects
 */
struct attestation_evidence_caps {
	size_t min_challenge_length;
	size_t max_challenge_length;
	size_t max_total_output_length;
	const struct attestation_evidence_supported_object *objects;
	size_t objects_count;
};

/**
 * struct attestation_evidence_object_request - One resolved output object.
 * @supported_object: matching capability entry
 * @output: resolved output blob
 */
struct attestation_evidence_object_request {
	const struct attestation_evidence_supported_object *supported_object;
	struct ctlv_blob *output;
};

/**
 * struct attestation_evidence_request - Validated evidence request.
 * @challenge: challenge bytes
 * @challenge_length: challenge length
 * @objects: requested output objects
 * @objects_count: number of objects
 */
struct attestation_evidence_request {
	const u8 *challenge;
	size_t challenge_length;
	const struct attestation_evidence_object_request *objects;
	size_t objects_count;
};

/**
 * struct attestation_evidence_result - Provider evidence result.
 * @object_lengths: produced length for every requested object
 */
struct attestation_evidence_result {
	size_t *object_lengths;
};

/**
 * struct attestation_provider - Immutable provider operations and properties.
 * @evidence_get: produce requested evidence
 * @evidence_accept: record acceptance of the current evidence
 * @evidence_accept_clear: clear a recorded acceptance
 * @mr_read: read one MR
 * @mr_extend: extend one MR
 * @tdisp_state_get: get the TDISP state
 * @device_trust_get: get the provider trust state
 * @devsec_run: enter the provider's secure state
 * @evidence_caps: evidence capabilities, or NULL
 * @mrs: fixed MR properties
 * @mrs_count: number of fixed MRs
 */
struct attestation_provider {
	int (*evidence_get)(struct attestation *att,
			    const struct attestation_evidence_request *request,
			    struct attestation_evidence_result *result);
	int (*evidence_accept)(struct attestation *att);
	void (*evidence_accept_clear)(struct attestation *att);
	int (*mr_read)(struct attestation *att, unsigned int mr_id,
		       struct attestation_digest_set *values);
	int (*mr_extend)(struct attestation *att, unsigned int mr_id,
			 const struct attestation_digest_set *values);
	int (*tdisp_state_get)(struct attestation *att,
			       enum attestation_tdisp_state *state);
	int (*device_trust_get)(struct attestation *att,
				enum attestation_device_trust_state *trust);
	int (*devsec_run)(struct attestation *att);
	const struct attestation_evidence_caps *evidence_caps;
	const struct attestation_mr_props *mrs;
	size_t mrs_count;
};

/**
 * attestation_state_changed - Report a native provider state change.
 * @att: provider instance
 *
 * Call without conflicting native locks held.
 */
void attestation_state_changed(struct attestation *att);

#endif /* _LINUX_ATTESTATION_H */
