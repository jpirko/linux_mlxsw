/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation.yaml */
/* CTLV-GEN kernel header */

/* Kernel metadata and driver API of the ctlv-attestation CTLV family. */

#ifndef _LINUX_CTLV_ATTESTATION_GEN_H
#define _LINUX_CTLV_ATTESTATION_GEN_H

#include <linux/ctlv.h>
#include <linux/err.h>
#include <linux/errno.h>

#include <uapi/linux/ctlv_attestation_gen.h>

/* Ops this family declares, in declaration order. */
#define CTLV_ATTESTATION_N_OPS 14

extern const struct ctlv_op_schema
	ctlv_attestation_ctlv_op_schemas[CTLV_ATTESTATION_N_OPS];

extern const struct ctlv_family_schema ctlv_attestation_ctlv_schema;

/* Implemented by this family's driver, one per action and query it declares. */
int ctlv_attestation_device_get_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request,
				       struct ctlv_cursor *reply);
int ctlv_attestation_evidence_get_action(void *priv, struct ctlv_op_ctx *ctx,
					 const struct ctlv_cursor *request,
					 struct ctlv_cursor *reply);
int ctlv_attestation_evidence_accept_action(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply);
int
ctlv_attestation_evidence_accept_clear_action(void *priv,
					      struct ctlv_op_ctx *ctx,
					      const struct ctlv_cursor *request,
					      struct ctlv_cursor *reply);
int ctlv_attestation_mr_get_action(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request,
				   struct ctlv_cursor *reply);
int ctlv_attestation_mr_dump_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request,
				   struct ctlv_cursor *reply);
int ctlv_attestation_mr_read_query(void *priv, struct ctlv_op_ctx *ctx,
				   const struct ctlv_cursor *request,
				   struct ctlv_cursor *reply);
int ctlv_attestation_mr_extend_action(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request,
				      struct ctlv_cursor *reply);
int ctlv_attestation_mr_log_get_query(void *priv, struct ctlv_op_ctx *ctx,
				      const struct ctlv_cursor *request,
				      struct ctlv_cursor *reply);
int ctlv_attestation_devsec_state_get_query(void *priv, struct ctlv_op_ctx *ctx,
					    const struct ctlv_cursor *request,
					    struct ctlv_cursor *reply);
int ctlv_attestation_devsec_run_action(void *priv, struct ctlv_op_ctx *ctx,
				       const struct ctlv_cursor *request,
				       struct ctlv_cursor *reply);

/*
 * Called around every effective non-core action or query,
 * on a device created with this family.
 */
int ctlv_attestation_op_pre(void *priv, struct ctlv_op_ctx *ctx);
void ctlv_attestation_op_post(void *priv, struct ctlv_op_ctx *ctx);

/* The lifecycle of this family's core-allocated priv. */
size_t ctlv_attestation_priv_size(void);
int ctlv_attestation_init(struct ctlv_device *ctlvdev, const void *init_info,
			  void *priv);
int ctlv_attestation_post_init(struct ctlv_device *ctlvdev, void *priv);
void ctlv_attestation_pre_fini(struct ctlv_device *ctlvdev, void *priv);
void ctlv_attestation_fini(struct ctlv_device *ctlvdev, void *priv);

extern const struct ctlv_family ctlv_attestation_ctlv_family;

/* What family initialization may decide about one op before publication. */
static inline void
ctlv_attestation_evidence_get_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000002ULL);
}

static inline void
ctlv_attestation_evidence_get_enable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x0003000000000002ULL, false);
}

static inline void
ctlv_attestation_evidence_get_disable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x0003000000000002ULL, true);
}

static inline void
ctlv_attestation_evidence_accept_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000003ULL);
}

static inline void
ctlv_attestation_evidence_accept_clear_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000004ULL);
}

static inline void ctlv_attestation_mr_get_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000005ULL);
}

static inline void ctlv_attestation_mr_dump_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000006ULL);
}

static inline void ctlv_attestation_mr_read_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000007ULL);
}

static inline void
ctlv_attestation_mr_extend_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000008ULL);
}

static inline void
ctlv_attestation_mr_log_get_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x0003000000000009ULL);
}

static inline void
ctlv_attestation_devsec_state_get_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x000300000000000aULL);
}

static inline void
ctlv_attestation_devsec_run_provide(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_provide(ctlvdev, 0x000300000000000bULL);
}

static inline void
ctlv_attestation_devsec_run_enable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x000300000000000bULL, false);
}

static inline void
ctlv_attestation_devsec_run_disable(struct ctlv_device *ctlvdev)
{
	ctlv_device_op_state_set(ctlvdev, 0x000300000000000bULL, true);
}

/* Reading a validated message. A blob has a resolver instead of a getter. */

#define ctlv_attestation_evidence_get_evidence_challenge_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE, 0, 0, \
		CTLV_SCHEMA_BINARY })

static inline bool
ctlv_attestation_evidence_get_evidence_challenge_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_evidence_challenge_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_attestation_evidence_get_evidence_challenge_get(
	const struct ctlv_cursor *nest, u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_evidence_challenge_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_attestation_evidence_get_evidence_object_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT, 0, 1, \
		CTLV_SCHEMA_NEST })

#define ctlv_attestation_evidence_get_evidence_object_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
			 ctlv_attestation_evidence_get_evidence_object_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_attestation_evidence_get_device_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 2, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_device_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_device_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_device_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_device_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_evidence_selection_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION, 0, 3, \
		CTLV_SCHEMA_NEST })

#define ctlv_attestation_evidence_get_evidence_selection_for_each(_attr, \
								  _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
		      ctlv_attestation_evidence_get_evidence_selection_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_attestation_evidence_get_evidence_privlevel_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL, 0, 4, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_evidence_privlevel_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_evidence_privlevel_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_evidence_privlevel_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_evidence_privlevel_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_evidence_service_provider_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER, 0, 5, \
		CTLV_SCHEMA_STRING })

static inline bool
ctlv_attestation_evidence_get_evidence_service_provider_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		 ctlv_attestation_evidence_get_evidence_service_provider_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_attestation_evidence_get_evidence_service_provider_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		 ctlv_attestation_evidence_get_evidence_service_provider_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_attestation_evidence_get_evidence_service_guid_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID, 0, 6, \
		CTLV_SCHEMA_BINARY })

static inline bool
ctlv_attestation_evidence_get_evidence_service_guid_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		     ctlv_attestation_evidence_get_evidence_service_guid_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_attestation_evidence_get_evidence_service_guid_get(
	const struct ctlv_cursor *nest, u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		     ctlv_attestation_evidence_get_evidence_service_guid_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_attestation_evidence_get_evidence_service_manifest_version_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION, 0, 7, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_evidence_service_manifest_version_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	 ctlv_attestation_evidence_get_evidence_service_manifest_version_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_evidence_service_manifest_version_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	 ctlv_attestation_evidence_get_evidence_service_manifest_version_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_evidence_options_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS, 0, 8, \
		CTLV_SCHEMA_NEST })

static inline bool
ctlv_attestation_evidence_get_evidence_options_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_evidence_options_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_attestation_evidence_get_evidence_options_get(
	struct ctlv_cursor *nested, const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest,
			ctlv_attestation_evidence_get_evidence_options_ref());
}

#define ctlv_attestation_evidence_get_object_evidence_blob_class_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS, 2, 0, \
		CTLV_SCHEMA_ENUM })

static inline bool
ctlv_attestation_evidence_get_object_evidence_blob_class_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		ctlv_attestation_evidence_get_object_evidence_blob_class_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_object_evidence_blob_class_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		ctlv_attestation_evidence_get_object_evidence_blob_class_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_object_evidence_format_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT, 2, 1, \
		CTLV_SCHEMA_STRING })

static inline bool
ctlv_attestation_evidence_get_object_evidence_format_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		    ctlv_attestation_evidence_get_object_evidence_format_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_attestation_evidence_get_object_evidence_format_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		    ctlv_attestation_evidence_get_object_evidence_format_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_attestation_evidence_get_object_evidence_representation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION, 2, 2, \
		CTLV_SCHEMA_ENUM })

static inline bool
ctlv_attestation_evidence_get_object_evidence_representation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	    ctlv_attestation_evidence_get_object_evidence_representation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_object_evidence_representation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	    ctlv_attestation_evidence_get_object_evidence_representation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_object_evidence_data_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_DATA, 2, 3, CTLV_SCHEMA_BLOB })

static inline bool
ctlv_attestation_evidence_get_object_evidence_data_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		      ctlv_attestation_evidence_get_object_evidence_data_ref());
	return ctlv_attr_ok(&attr);
}

static inline struct ctlv_blob *
ctlv_attestation_evidence_get_object_evidence_data_resolve(
	struct ctlv_op_ctx *ctx, const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		      ctlv_attestation_evidence_get_object_evidence_data_ref());
	return ctlv_blob_resolve(ctx, &attr);
}

#define ctlv_attestation_evidence_get_object_evidence_blob_instance_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE, 2, 4, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_object_evidence_blob_instance_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	     ctlv_attestation_evidence_get_object_evidence_blob_instance_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_object_evidence_blob_instance_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	     ctlv_attestation_evidence_get_object_evidence_blob_instance_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_object_evidence_hash_algo_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO, 2, 5, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_object_evidence_hash_algo_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		 ctlv_attestation_evidence_get_object_evidence_hash_algo_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_object_evidence_hash_algo_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		 ctlv_attestation_evidence_get_object_evidence_hash_algo_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_selection_mr_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_MR_ID, 4, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_selection_mr_id_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_selection_mr_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_selection_mr_id_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_evidence_get_selection_mr_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_get_selection_evidence_hash_algo_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[1].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO, 4, 1, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_get_selection_evidence_hash_algo_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	      ctlv_attestation_evidence_get_selection_evidence_hash_algo_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_get_selection_evidence_hash_algo_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
	      ctlv_attestation_evidence_get_selection_evidence_hash_algo_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_accept_device_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[2].request, \
		CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 0, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_accept_device_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		      ctlv_attestation_evidence_accept_device_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_accept_device_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		      ctlv_attestation_evidence_accept_device_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_evidence_accept_evidence_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[2].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION, 0, 1, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_evidence_accept_evidence_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		    ctlv_attestation_evidence_accept_evidence_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_evidence_accept_evidence_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
		    ctlv_attestation_evidence_accept_evidence_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_get_mr_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[4].request, \
		CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_get_mr_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_get_mr_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_get_mr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_get_mr_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_read_mr_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[6].request, \
		CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_read_mr_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_read_mr_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_read_mr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_read_mr_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_read_device_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[6].request, \
		CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 1, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_read_device_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_read_device_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_read_device_generation_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_read_device_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_read_mr_options_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[6].request, \
		CTLV_ATTESTATION_ATTR_MR_OPTIONS, 0, 2, CTLV_SCHEMA_NEST })

static inline bool
ctlv_attestation_mr_read_mr_options_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_read_mr_options_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_attestation_mr_read_mr_options_get(struct ctlv_cursor *nested,
					const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest,
			ctlv_attestation_mr_read_mr_options_ref());
}

#define ctlv_attestation_mr_extend_mr_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_MR_ID, 0, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_extend_mr_id_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_extend_mr_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_extend_mr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest, ctlv_attestation_mr_extend_mr_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_extend_mr_value_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_MR_VALUE, 0, 1, CTLV_SCHEMA_NEST })

#define ctlv_attestation_mr_extend_mr_value_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
				ctlv_attestation_mr_extend_mr_value_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_attestation_mr_extend_device_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 2, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_extend_device_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_device_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_extend_device_generation_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_device_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_extend_mr_options_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_MR_OPTIONS, 0, 3, CTLV_SCHEMA_NEST })

static inline bool
ctlv_attestation_mr_extend_mr_options_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_mr_options_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_attestation_mr_extend_mr_options_get(struct ctlv_cursor *nested,
					  const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest,
			ctlv_attestation_mr_extend_mr_options_ref());
}

#define ctlv_attestation_mr_extend_record_item_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_RECORD_ITEM, 0, 4, CTLV_SCHEMA_NEST })

static inline bool
ctlv_attestation_mr_extend_record_item_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_record_item_ref());
	return ctlv_attr_ok(&attr);
}

static inline void
ctlv_attestation_mr_extend_record_item_get(struct ctlv_cursor *nested,
					   const struct ctlv_cursor *nest)
{
	ctlv_input_attr(nested, nest,
			ctlv_attestation_mr_extend_record_item_ref());
}

#define ctlv_attestation_mr_extend_value_mr_hash_algo_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_MR_HASH_ALGO, 2, 0, CTLV_SCHEMA_UINT })

#define ctlv_attestation_mr_extend_value_mr_hash_algo_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
			 ctlv_attestation_mr_extend_value_mr_hash_algo_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

static inline u64
ctlv_attestation_mr_extend_value_mr_hash_algo_get(
	const struct ctlv_cursor *attr)
{
	if (!ctlv_input_cursor(attr,
			   ctlv_attestation_mr_extend_value_mr_hash_algo_ref()))
		return 0;
	return ctlv_attr_u64(attr);
}

#define ctlv_attestation_mr_extend_value_mr_data_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_MR_DATA, 2, 1, CTLV_SCHEMA_BINARY })

static inline bool
ctlv_attestation_mr_extend_value_mr_data_present(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_value_mr_data_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_attestation_mr_extend_value_mr_data_get(const struct ctlv_cursor *nest,
					     u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_value_mr_data_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_attestation_mr_extend_item_record_format_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_RECORD_FORMAT, 5, 0, \
		CTLV_SCHEMA_STRING })

static inline bool
ctlv_attestation_mr_extend_item_record_format_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_item_record_format_ref());
	return ctlv_attr_ok(&attr);
}

static inline const char *
ctlv_attestation_mr_extend_item_record_format_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_item_record_format_ref());
	return ctlv_attr_string(&attr);
}

#define ctlv_attestation_mr_extend_item_record_data_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[7].request, \
		CTLV_ATTESTATION_ATTR_RECORD_DATA, 5, 1, CTLV_SCHEMA_BINARY })

static inline bool
ctlv_attestation_mr_extend_item_record_data_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_item_record_data_ref());
	return ctlv_attr_ok(&attr);
}

static inline const void *
ctlv_attestation_mr_extend_item_record_data_get(const struct ctlv_cursor *nest,
						u32 *len)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_extend_item_record_data_ref());
	return ctlv_attr_binary(&attr, len);
}

#define ctlv_attestation_mr_log_get_mr_selection_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[8].request, \
		CTLV_ATTESTATION_ATTR_MR_SELECTION, 0, 0, CTLV_SCHEMA_NEST })

#define ctlv_attestation_mr_log_get_mr_selection_for_each(_attr, _nest) \
	for (struct ctlv_cursor *__ctlv_attr = \
		ctlv_input_attr(_attr, _nest, \
			      ctlv_attestation_mr_log_get_mr_selection_ref()); \
	     ctlv_attr_ok(__ctlv_attr); \
	     ctlv_attr_next(__ctlv_attr))

#define ctlv_attestation_mr_log_get_mr_start_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[8].request, \
		CTLV_ATTESTATION_ATTR_MR_START_GENERATION, 0, 1, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_log_get_mr_start_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_mr_start_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_log_get_mr_start_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_mr_start_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_log_get_mr_end_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[8].request, \
		CTLV_ATTESTATION_ATTR_MR_END_GENERATION, 0, 2, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_log_get_mr_end_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_mr_end_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_log_get_mr_end_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_mr_end_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_mr_log_get_selection_mr_id_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[8].request, \
		CTLV_ATTESTATION_ATTR_MR_ID, 1, 0, CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_mr_log_get_selection_mr_id_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_selection_mr_id_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_mr_log_get_selection_mr_id_get(const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_mr_log_get_selection_mr_id_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_devsec_run_device_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[10].request, \
		CTLV_ATTESTATION_ATTR_DEVICE_GENERATION, 0, 0, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_devsec_run_device_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_devsec_run_device_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_devsec_run_device_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_devsec_run_device_generation_ref());
	return ctlv_attr_u64(&attr);
}

#define ctlv_attestation_devsec_run_evidence_generation_ref() \
	((struct ctlv_input_ref){ \
		&ctlv_attestation_ctlv_op_schemas[10].request, \
		CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION, 0, 1, \
		CTLV_SCHEMA_UINT })

static inline bool
ctlv_attestation_devsec_run_evidence_generation_present(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_devsec_run_evidence_generation_ref());
	return ctlv_attr_ok(&attr);
}

static inline u64
ctlv_attestation_devsec_run_evidence_generation_get(
	const struct ctlv_cursor *nest)
{
	struct ctlv_cursor attr;

	ctlv_input_attr(&attr, nest,
			ctlv_attestation_devsec_run_evidence_generation_ref());
	return ctlv_attr_u64(&attr);
}

/* Writing a reply. An append is atomic; a nest ends with ctlv_nest_end(). */

static inline bool
ctlv_attestation_device_generation_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_DEVICE_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_device_capabilities_start(struct ctlv_cursor *nest,
					   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_DEVICE_CAPABILITIES,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_device_info_start(struct ctlv_cursor *nest,
				   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_DEVICE_INFO,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_capability_evidence_start(struct ctlv_cursor *nest,
					   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_CAPABILITY_EVIDENCE,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_capability_mr_start(struct ctlv_cursor *nest,
				     struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_CAPABILITY_MR,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_capability_devsec_start(struct ctlv_cursor *nest,
					 struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_CAPABILITY_DEVSEC,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_capability_min_challenge_length_put(struct ctlv_cursor *nest,
						     u64 value)
{
	return ctlv_put_u64(nest,
			  CTLV_ATTESTATION_ATTR_CAPABILITY_MIN_CHALLENGE_LENGTH,
			  CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_capability_max_challenge_length_put(struct ctlv_cursor *nest,
						     u64 value)
{
	return ctlv_put_u64(nest,
			  CTLV_ATTESTATION_ATTR_CAPABILITY_MAX_CHALLENGE_LENGTH,
			  CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_capability_max_total_output_length_put(
	struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest,
		       CTLV_ATTESTATION_ATTR_CAPABILITY_MAX_TOTAL_OUTPUT_LENGTH,
		       CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_capability_evidence_flags_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest,
			    CTLV_ATTESTATION_ATTR_CAPABILITY_EVIDENCE_FLAGS,
			    CTLV_SCHEMA_FLAGS, value);
}

static inline bool
ctlv_attestation_capability_mr_flags_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_MR_FLAGS,
			    CTLV_SCHEMA_FLAGS, value);
}

static inline bool
ctlv_attestation_capability_devsec_flags_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_DEVSEC_FLAGS,
			    CTLV_SCHEMA_FLAGS, value);
}

static inline bool
ctlv_attestation_capability_object_start(struct ctlv_cursor *nest,
					 struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_CAPABILITY_OBJECT,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_capability_blob_class_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_BLOB_CLASS,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_capability_blob_instance_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest,
			    CTLV_ATTESTATION_ATTR_CAPABILITY_BLOB_INSTANCE,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_capability_format_put(struct ctlv_cursor *nest,
				       const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_FORMAT,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool
ctlv_attestation_capability_representation_put(struct ctlv_cursor *nest,
					       u64 value)
{
	return ctlv_put_u64(nest,
			    CTLV_ATTESTATION_ATTR_CAPABILITY_REPRESENTATION,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_capability_hash_algo_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_HASH_ALGO,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_capability_max_length_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_CAPABILITY_MAX_LENGTH,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_generation_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_accepted_generation_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest,
			    CTLV_ATTESTATION_ATTR_EVIDENCE_ACCEPTED_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_challenge_put(struct ctlv_cursor *nest,
					const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_CHALLENGE,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool
ctlv_attestation_evidence_object_start(struct ctlv_cursor *nest,
				       struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_evidence_blob_class_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_CLASS,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_evidence_blob_instance_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_BLOB_INSTANCE,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_format_put(struct ctlv_cursor *nest,
				     const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_FORMAT,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool
ctlv_attestation_evidence_representation_put(struct ctlv_cursor *nest,
					     u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_REPRESENTATION,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_evidence_hash_algo_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_HASH_ALGO,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_selection_start(struct ctlv_cursor *nest,
					  struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_EVIDENCE_SELECTION,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_evidence_privlevel_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_PRIVLEVEL,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_service_provider_put(struct ctlv_cursor *nest,
					       const char *value)
{
	return ctlv_put_string(nest,
			       CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_PROVIDER,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool
ctlv_attestation_evidence_service_guid_put(struct ctlv_cursor *nest,
					   const void *value, u32 len)
{
	return ctlv_put_binary(nest,
			       CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_GUID,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool
ctlv_attestation_evidence_service_manifest_version_put(struct ctlv_cursor *nest,
						       u64 value)
{
	return ctlv_put_u64(nest,
			CTLV_ATTESTATION_ATTR_EVIDENCE_SERVICE_MANIFEST_VERSION,
			CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_evidence_options_start(struct ctlv_cursor *nest,
					struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_EVIDENCE_OPTIONS,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_evidence_object_length_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_EVIDENCE_OBJECT_LENGTH,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool ctlv_attestation_mr_id_put(struct ctlv_cursor *nest,
					      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_ID, CTLV_SCHEMA_UINT,
			    value);
}

static inline bool ctlv_attestation_mr_name_put(struct ctlv_cursor *nest,
						const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_ATTR_MR_NAME,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool ctlv_attestation_mr_hash_algo_put(struct ctlv_cursor *nest,
						     u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_HASH_ALGO,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_mr_register_index_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_REGISTER_INDEX,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_mr_capabilities_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_CAPABILITIES,
			    CTLV_SCHEMA_FLAGS, value);
}

static inline bool ctlv_attestation_mr_state_flags_put(struct ctlv_cursor *nest,
						       u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_STATE_FLAGS,
			    CTLV_SCHEMA_FLAGS, value);
}

static inline bool ctlv_attestation_mr_item_start(struct ctlv_cursor *nest,
						  struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_MR_ITEM,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_mr_options_start(struct ctlv_cursor *nest,
						     struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_MR_OPTIONS,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_mr_value_start(struct ctlv_cursor *nest,
						   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_MR_VALUE,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_mr_data_put(struct ctlv_cursor *nest,
						const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_ATTESTATION_ATTR_MR_DATA,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool ctlv_attestation_mr_generation_put(struct ctlv_cursor *nest,
						      u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_mr_selection_start(struct ctlv_cursor *nest,
				    struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_MR_SELECTION,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_mr_start_generation_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_START_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_mr_end_generation_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_MR_END_GENERATION,
			    CTLV_SCHEMA_UINT, value);
}

static inline bool
ctlv_attestation_record_item_start(struct ctlv_cursor *nest,
				   struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_RECORD_ITEM,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_record_format_put(struct ctlv_cursor *nest,
						      const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_ATTR_RECORD_FORMAT,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool ctlv_attestation_record_data_put(struct ctlv_cursor *nest,
						    const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_ATTESTATION_ATTR_RECORD_DATA,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool ctlv_attestation_log_entry_start(struct ctlv_cursor *nest,
						    struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_LOG_ENTRY,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_log_event_put(struct ctlv_cursor *nest,
						  u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_LOG_EVENT,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool ctlv_attestation_log_mr_start(struct ctlv_cursor *nest,
						 struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_LOG_MR,
			       CTLV_SCHEMA_NEST);
}

static inline bool ctlv_attestation_log_ima_start(struct ctlv_cursor *nest,
						  struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested, CTLV_ATTESTATION_ATTR_LOG_IMA,
			       CTLV_SCHEMA_NEST);
}

static inline bool
ctlv_attestation_log_template_name_put(struct ctlv_cursor *nest,
				       const char *value)
{
	return ctlv_put_string(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_NAME,
			       CTLV_SCHEMA_STRING, value);
}

static inline bool
ctlv_attestation_log_template_data_put(struct ctlv_cursor *nest,
				       const void *value, u32 len)
{
	return ctlv_put_binary(nest, CTLV_ATTESTATION_ATTR_LOG_TEMPLATE_DATA,
			       CTLV_SCHEMA_BINARY, value, len);
}

static inline bool ctlv_attestation_log_violation_put(struct ctlv_cursor *nest,
						      bool value)
{
	return ctlv_put_bool(nest, CTLV_ATTESTATION_ATTR_LOG_VIOLATION,
			     CTLV_SCHEMA_BOOL, value);
}

static inline bool
ctlv_attestation_devsec_tdisp_state_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_DEVSEC_TDISP_STATE,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_devsec_device_trust_put(struct ctlv_cursor *nest, u64 value)
{
	return ctlv_put_u64(nest, CTLV_ATTESTATION_ATTR_DEVSEC_DEVICE_TRUST,
			    CTLV_SCHEMA_ENUM, value);
}

static inline bool
ctlv_attestation_notification_entry_start(struct ctlv_cursor *nest,
					  struct ctlv_cursor *nested)
{
	return ctlv_nest_start(nest, nested,
			       CTLV_ATTESTATION_ATTR_NOTIFICATION_ENTRY,
			       CTLV_SCHEMA_NEST);
}

/* Building one event, passed to ctlv_op_emit() or ctlv_op_abort() once. */

static inline struct ctlv_op_ctx *
ctlv_attestation_device_state_changed_create(struct ctlv_device *ctlvdev,
					     struct ctlv_cursor **reply,
					     gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x000300000000000cULL, reply, gfp);
}

static inline struct ctlv_op_ctx *
ctlv_attestation_evidence_state_changed_create(struct ctlv_device *ctlvdev,
					       struct ctlv_cursor **reply,
					       gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x000300000000000dULL, reply, gfp);
}

static inline struct ctlv_op_ctx *
ctlv_attestation_mr_log_entry_appended_create(struct ctlv_device *ctlvdev,
					      struct ctlv_cursor **reply,
					      gfp_t gfp)
{
	return ctlv_event_create(ctlvdev, 0x000300000000000eULL, reply, gfp);
}

#endif /* _LINUX_CTLV_ATTESTATION_GEN_H */
