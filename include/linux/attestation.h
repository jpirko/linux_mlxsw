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
	ATTESTATION_DEVICE_TRUST_TCB =
		CTLV_ATTESTATION_DEVICE_TRUST_STATE_TCB,
};

struct attestation_mr_caps {
	bool supports_dynamic_topology;
};

struct attestation_digest {
	enum hash_algo algorithm;
	u8 data[HASH_MAX_DIGESTSIZE];
};

struct attestation_digest_set {
	size_t values_count;
	struct attestation_digest *values;
};

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

struct attestation_evidence_object_request {
	const struct attestation_evidence_supported_object *supported_object;
	enum attestation_evidence_representation representation;
	enum hash_algo hash_algorithm;
	struct ctlv_blob *output;
};

struct attestation_evidence_selection_mr {
	unsigned int mr_id;
	enum hash_algo hash_algorithm;
};

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

struct attestation_evidence_result {
	size_t *object_lengths;
};

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

struct attestation_mr *
attestation_mr_create(struct attestation *att,
		      const struct attestation_mr_props *props);
void attestation_mr_destroy(struct attestation_mr *mr);
void attestation_state_changed(struct attestation *att);

#endif /* _LINUX_ATTESTATION_H */
