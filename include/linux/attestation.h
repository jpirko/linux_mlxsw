/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_ATTESTATION_H
#define _LINUX_ATTESTATION_H

#include <crypto/hash.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/ctlv_device.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>

#include <uapi/ctlv/ctlv_attestation_gen.h>
#include <uapi/linux/hash_info.h>

struct attestation_dev;
struct ctlv_blob;
struct ctlv_family;

enum attestation_evidence_class {
	ATTESTATION_EVIDENCE_SIGNED = CTLV_ATTESTATION_EVIDENCE_CLASS_SIGNED,
	ATTESTATION_EVIDENCE_EVENT_LOG = CTLV_ATTESTATION_EVIDENCE_CLASS_EVENT_LOG,
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
 * struct attestation_digest_set - Digests for an MR's banks, in the MR's order.
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
 * @algorithms: bank hash algorithms, each once, in the order replies list them
 * @algorithms_count: number of algorithms
 * @has_register_index: whether @register_index is valid
 * @register_index: native register index
 * @extendable: MR may be extended. Fixed: the provider instead refuses an
 *	extend the platform forbids at the time, e.g. -EPERM for a TPM locality.
 */
struct attestation_mr_props {
	unsigned int id;
	const char *name;
	const enum hash_algo *algorithms;
	size_t algorithms_count;
	bool has_register_index;
	unsigned int register_index;
	bool extendable;
};

/**
 * struct attestation_evidence_format - The layout of an evidence object.
 * @vendor: @value is an evidence-vendor-format value the device's family
 *	gives, not an evidence-format one
 * @value: the value, such as CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL
 */
struct attestation_evidence_format {
	bool vendor;
	u32 value;
};

/**
 * struct attestation_evidence_challenge_length - Challenge lengths taken.
 * @min: shortest challenge bytes, at least 1
 * @max: longest challenge bytes, @min for one exact length, at most 4096
 */
struct attestation_evidence_challenge_length {
	size_t min;
	size_t max;
};

/**
 * struct attestation_evidence_supported_object - One evidence object.
 * @evidence_class: what the object is
 * @formats: the layouts it may have, at most four of each kind
 * @formats_count: number of formats
 * @challenge_lengths: a signed object's challenge lengths, ascending and
 *	disjoint; none for any other object
 * @challenge_lengths_count: number of challenge lengths, at most 8
 * @max_length: longest the object gets, in bytes
 * @binds_mrs: the object covers MR values, as they are or by a digest, so
 *	the reply carries mr-generation
 *
 * The object's index in the provider's objects is its ID.
 */
struct attestation_evidence_supported_object {
	enum attestation_evidence_class evidence_class;
	const struct attestation_evidence_format *formats;
	size_t formats_count;
	const struct attestation_evidence_challenge_length *challenge_lengths;
	size_t challenge_lengths_count;
	size_t max_length;
	bool binds_mrs;
};

/**
 * struct attestation_evidence_caps - Immutable evidence capabilities.
 * @objects: supported evidence objects, at most 16
 * @objects_count: number of objects
 */
struct attestation_evidence_caps {
	const struct attestation_evidence_supported_object *objects;
	size_t objects_count;
};

/**
 * struct attestation_evidence_object_request - One resolved output object.
 * @supported_object: the object requested
 * @output: resolved output blob
 */
struct attestation_evidence_object_request {
	const struct attestation_evidence_supported_object *supported_object;
	struct ctlv_blob *output;
};

/**
 * struct attestation_evidence_request - Validated evidence request.
 * @challenge: challenge bytes, of a length every signed object requested
 *	takes, or NULL where the request names no signed object
 * @challenge_length: challenge length, 0 without a challenge
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
 * struct attestation_evidence_object_result - What a provider made of one object.
 * @format: the layout it came in, one of the object's formats
 * @length: bytes written
 */
struct attestation_evidence_object_result {
	struct attestation_evidence_format format;
	size_t length;
};

/**
 * struct attestation_evidence_result - Provider evidence result.
 * @objects: one for every requested object, in request order
 */
struct attestation_evidence_result {
	struct attestation_evidence_object_result *objects;
};

/**
 * struct attestation_provider - Immutable provider operations and properties.
 * @evidence_get: produce requested evidence, each object written with
 *	attestation_evidence_write() and its result filled in
 * @mr_read: read one MR; every MR can be read, so required with @mrs
 * @mr_extend: extend the banks of one MR it gets digests for, leaving the
 *	others, or fail with -EOPNOTSUPP before changing any if it cannot
 * @evidence_caps: evidence capabilities, or NULL
 * @mrs: fixed MR properties
 * @mrs_count: number of fixed MRs
 */
struct attestation_provider {
	int (*evidence_get)(struct attestation_dev *att,
			    const struct attestation_evidence_request *request,
			    struct attestation_evidence_result *result);
	int (*mr_read)(struct attestation_dev *att, unsigned int mr_id,
		       struct attestation_digest_set *values);
	int (*mr_extend)(struct attestation_dev *att, unsigned int mr_id,
			 const struct attestation_digest_set *values);
	const struct attestation_evidence_caps *evidence_caps;
	const struct attestation_mr_props *mrs;
	size_t mrs_count;
};

/**
 * struct attestation_dev - One attestation device.
 * @node: the device's node, whose release frees the driver's struct embedding
 *	this
 *
 * A driver embeds this first in its own struct, allocated by
 * attestation_alloc(), and reaches that struct with container_of().
 */
struct attestation_dev {
	struct ctlv_device node;
	/* private: */
	struct ctlv ctlv;
	const struct attestation_provider *provider;
	unsigned int index;

	/* Mutable instance state. */
	struct mutex lock;

};

/* What a CTLV callback of a family inheriting ctlv-attestation was handed. */
static inline struct attestation_dev *attestation_from_ctlv(struct ctlv *ctlv)
{
	return container_of(ctlv, struct attestation_dev, ctlv);
}

struct attestation_dev *_attestation_alloc(struct device *parent,
					   const struct attestation_provider *provider,
					   const struct ctlv_family *family, size_t size,
					   size_t offset);

/**
 * attestation_alloc - Allocate a driver struct embedding an attestation device.
 * @parent: the device attested
 * @provider: provider operations and properties, which must outlive the device
 * @family: the driver's CTLV family, which inherits ctlv-attestation
 * @type: the driver's struct
 * @member: its struct attestation_dev, the first member
 *
 * Return: the driver's struct, released by attestation_put(), or an error
 * pointer.
 */
#define attestation_alloc(parent, provider, family, type, member)		\
	((type *)_attestation_alloc(parent, provider, family, sizeof(type),	\
				    offsetof(type, member)))

int attestation_register(struct attestation_dev *att);
void attestation_unregister(struct attestation_dev *att);
void attestation_put(struct attestation_dev *att);

int attestation_evidence_write(const struct attestation_evidence_object_request *object,
			       const void *data, size_t length);
#endif /* _LINUX_ATTESTATION_H */
