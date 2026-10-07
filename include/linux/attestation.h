/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_ATTESTATION_H
#define _LINUX_ATTESTATION_H

#include <crypto/hash.h>
#include <linux/types.h>
#include <linux/uuid.h>

#include <uapi/linux/ctlv_attestation_gen.h>
#include <uapi/linux/hash_info.h>

struct attestation;
struct attestation_mr;
struct ctlv_blob;
struct ctlv_cursor;

enum attestation_evidence_representation {
	ATTESTATION_EVIDENCE_REPRESENTATION_RAW =
		CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW,
	ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST =
		CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST,
};

enum attestation_evidence_blob_class {
	ATTESTATION_EVIDENCE_BLOB_PRIMARY =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY,
	ATTESTATION_EVIDENCE_BLOB_AUXILIARY =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_AUXILIARY,
	ATTESTATION_EVIDENCE_BLOB_MANIFEST =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_MANIFEST,
	ATTESTATION_EVIDENCE_BLOB_CERT_CHAIN =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_CERT_CHAIN,
	ATTESTATION_EVIDENCE_BLOB_SPDM_VCA =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_SPDM_VCA,
	ATTESTATION_EVIDENCE_BLOB_SPDM_MEASUREMENTS =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_SPDM_MEASUREMENTS,
	ATTESTATION_EVIDENCE_BLOB_TDISP_REPORT =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_TDISP_REPORT,
	ATTESTATION_EVIDENCE_BLOB_SIGNATURE =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_SIGNATURE,
	ATTESTATION_EVIDENCE_BLOB_EVENT_LOG =
		CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_EVENT_LOG,
};

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
 * struct attestation_mr_caps - Provider MR capabilities.
 * @supports_dynamic_topology: MRs may be created and removed
 */
struct attestation_mr_caps {
	bool supports_dynamic_topology;
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
 * @blob_class: semantic evidence class
 * @has_blob_instance: whether @blob_instance is valid
 * @blob_instance: provider-defined class instance
 * @format: evidence format name
 * @representations: supported representations
 * @representations_count: number of representations
 * @hash_algorithms: supported TCB-digest algorithms
 * @hash_algorithms_count: number of hash algorithms
 * @max_length: maximum output bytes
 * @binds_mrs: evidence validity depends on MR state
 */
struct attestation_evidence_supported_object {
	enum attestation_evidence_blob_class blob_class;
	bool has_blob_instance;
	unsigned int blob_instance;
	const char *format;
	const enum attestation_evidence_representation *representations;
	size_t representations_count;
	const enum hash_algo *hash_algorithms;
	size_t hash_algorithms_count;
	size_t max_length;
	bool binds_mrs;
};

/**
 * struct attestation_evidence_caps - Immutable evidence capabilities.
 * @min_challenge_length: minimum challenge bytes
 * @max_challenge_length: maximum challenge bytes
 * @max_total_output_length: maximum bytes across all objects
 * @supports_mr_selection: requests may select MRs
 * @supports_privlevel: requests may select a privilege level
 * @supports_service_attestation: requests may identify a service
 * @objects: supported evidence objects
 * @objects_count: number of objects
 */
struct attestation_evidence_caps {
	size_t min_challenge_length;
	size_t max_challenge_length;
	size_t max_total_output_length;
	bool supports_mr_selection;
	bool supports_privlevel;
	bool supports_service_attestation;
	const struct attestation_evidence_supported_object *objects;
	size_t objects_count;
};

/**
 * struct attestation_evidence_object_request - One resolved output object.
 * @supported_object: matching capability entry
 * @representation: selected representation
 * @hash_algorithm: selected TCB-digest algorithm
 * @output: resolved output blob
 */
struct attestation_evidence_object_request {
	const struct attestation_evidence_supported_object *supported_object;
	enum attestation_evidence_representation representation;
	enum hash_algo hash_algorithm;
	struct ctlv_blob *output;
};

/**
 * struct attestation_evidence_selection_mr - One selected MR.
 * @mr_id: MR ID
 * @hash_algorithm: selected hash algorithm
 */
struct attestation_evidence_selection_mr {
	unsigned int mr_id;
	enum hash_algo hash_algorithm;
};

/**
 * struct attestation_evidence_request - Validated evidence request.
 * @challenge: challenge bytes
 * @challenge_length: challenge length
 * @evidence_options: family extension options
 * @objects: requested output objects
 * @objects_count: number of objects
 * @selection_mrs: selected MRs
 * @selection_mrs_count: number of selected MRs
 * @has_privlevel: whether @privlevel is valid
 * @privlevel: requested privilege level
 * @service_provider: requested service provider
 * @service_guid: requested service GUID
 * @service_manifest_version: requested service manifest version
 */
struct attestation_evidence_request {
	const u8 *challenge;
	size_t challenge_length;
	const struct ctlv_cursor *evidence_options;
	const struct attestation_evidence_object_request *objects;
	size_t objects_count;
	const struct attestation_evidence_selection_mr *selection_mrs;
	size_t selection_mrs_count;
	bool has_privlevel;
	unsigned int privlevel;
	const char *service_provider;
	guid_t service_guid;
	unsigned int service_manifest_version;
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
 * @mr_caps: MR capabilities, or NULL
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
		       const struct ctlv_cursor *mr_options,
		       struct attestation_digest_set *values);
	int (*mr_extend)(struct attestation *att, unsigned int mr_id,
			 const struct ctlv_cursor *mr_options,
			 const struct attestation_digest_set *values);
	int (*tdisp_state_get)(struct attestation *att,
			       enum attestation_tdisp_state *state);
	int (*device_trust_get)(struct attestation *att,
				enum attestation_device_trust_state *trust);
	int (*devsec_run)(struct attestation *att);
	const struct attestation_evidence_caps *evidence_caps;
	const struct attestation_mr_caps *mr_caps;
	const struct attestation_mr_props *mrs;
	size_t mrs_count;
};

/**
 * attestation_mr_create - Create a dynamic MR.
 * @att: provider instance
 * @props: immutable MR properties
 *
 * Return: MR handle, or an error pointer.
 */
struct attestation_mr *
attestation_mr_create(struct attestation *att,
		      const struct attestation_mr_props *props);

/**
 * attestation_mr_destroy - Destroy a dynamic MR.
 * @mr: MR handle returned by attestation_mr_create()
 */
void attestation_mr_destroy(struct attestation_mr *mr);

/**
 * attestation_state_changed - Report a native provider state change.
 * @att: provider instance
 *
 * Call without conflicting native locks held.
 */
void attestation_state_changed(struct attestation *att);

#endif /* _LINUX_ATTESTATION_H */
