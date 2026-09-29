// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/bits.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_attestation_gen.h"

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);
static_assert(sizeof(struct ctlv_event_hdr) == 24);

/* device-get request */
static const struct ctlv_schema_node device_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* device-get reply */
static const struct ctlv_schema_node device_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* device.capabilities */
	{
		.attr_id = 0x0003000100000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 4,
		.n_children = 3,
		.n_required_children = 0,
	},
	/* device.info */
	{
		.attr_id = 0x0003000100000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* capability.evidence */
	{
		.attr_id = 0x0003000200000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 7,
		.n_children = 5,
		.n_required_children = 5,
	},
	/* capability.mr */
	{
		.attr_id = 0x0003000200000002ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 12,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* capability.devsec */
	{
		.attr_id = 0x0003000200000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 13,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* capability.min-challenge-length */
	{
		.attr_id = 0x0003000200000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* capability.max-challenge-length */
	{
		.attr_id = 0x0003000200000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* capability.max-total-output-length */
	{
		.attr_id = 0x0003000200000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* capability.evidence-flags */
	{
		.attr_id = 0x0003000200000007ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0xfULL,
	},
	/* capability.object */
	{
		.attr_id = 0x000300020000000aULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.first_child = 14,
		.n_children = 6,
		.n_required_children = 4,
	},
	/* capability.mr-flags */
	{
		.attr_id = 0x0003000200000008ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* capability.devsec-flags */
	{
		.attr_id = 0x0003000200000009ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x1ULL,
	},
	/* capability.blob-class */
	{
		.attr_id = 0x000300020000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 9 },
	},
	/* capability.format */
	{
		.attr_id = 0x000300020000000dULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* capability.representation */
	{
		.attr_id = 0x000300020000000eULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 2,
		.range = { .min = 1, .max = 2 },
	},
	/* capability.max-length */
	{
		.attr_id = 0x0003000200000010ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* capability.blob-instance */
	{
		.attr_id = 0x000300020000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* capability.hash-algo */
	{
		.attr_id = 0x000300020000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
};

/* device-get reply extension points */
static const struct ctlv_stub_ref device_get_reply_stubs[] = {
	/* device-info */
	{
		.family_id = 3,
		.set_id = 1,
		.node = 3,
	},
};

/* evidence-get request */
static const struct ctlv_schema_node evidence_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 9,
		.n_required_children = 2,
	},
	/* evidence.challenge */
	{
		.attr_id = 0x0003000300000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
	/* evidence.object */
	{
		.attr_id = 0x0003000300000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 16,
		.first_child = 10,
		.n_children = 6,
		.n_required_children = 4,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.selection */
	{
		.attr_id = 0x000300030000000bULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.first_child = 16,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* evidence.privlevel */
	{
		.attr_id = 0x000300030000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.service-provider */
	{
		.attr_id = 0x000300030000000dULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* evidence.service-guid */
	{
		.attr_id = 0x000300030000000eULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 16, .max_len = 16 },
	},
	/* evidence.service-manifest-version */
	{
		.attr_id = 0x000300030000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.options */
	{
		.attr_id = 0x0003000300000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* evidence.blob-class */
	{
		.attr_id = 0x0003000300000005ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 9 },
	},
	/* evidence.format */
	{
		.attr_id = 0x0003000300000007ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* evidence.representation */
	{
		.attr_id = 0x0003000300000008ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 2 },
	},
	/* evidence.data */
	{
		.attr_id = 0x000300030000000aULL,
		.kind = CTLV_SCHEMA_BLOB,
		.max_occurrences = 1,
		.blob = {
			.max_len = 4294967295,
			.alignment = 1,
			.backings = BIT(CTLV_BLOB_TYPE_VA) |
				    BIT(CTLV_BLOB_TYPE_MEMFD) |
				    BIT(CTLV_BLOB_TYPE_DMA_BUF),
			.direction = CTLV_BLOB_DIRECTION_OUTPUT,
		},
	},
	/* evidence.blob-instance */
	{
		.attr_id = 0x0003000300000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.hash-algo */
	{
		.attr_id = 0x0003000300000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.hash-algo */
	{
		.attr_id = 0x0003000300000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 20 },
	},
};

/* evidence-get request extension points */
static const struct ctlv_stub_ref evidence_get_request_stubs[] = {
	/* evidence-options */
	{
		.family_id = 3,
		.set_id = 2,
		.node = 9,
	},
};

/* evidence-get reply */
static const struct ctlv_schema_node evidence_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 4,
		.n_required_children = 2,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.object-length */
	{
		.attr_id = 0x0003000300000011ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 16,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* evidence-accept request */
static const struct ctlv_schema_node evidence_accept_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* evidence-accept-clear request */
static const struct ctlv_schema_node evidence_accept_clear_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* mr-get request */
static const struct ctlv_schema_node mr_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-get reply */
static const struct ctlv_schema_node mr_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 5,
		.n_required_children = 4,
	},
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* mr.state-flags */
	{
		.attr_id = 0x0003000400000006ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x1ULL,
	},
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-dump request */
static const struct ctlv_schema_node mr_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* mr-dump reply */
static const struct ctlv_schema_node mr_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* mr.item */
	{
		.attr_id = 0x0003000400000007ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 2,
		.n_children = 6,
		.n_required_children = 5,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* mr.state-flags */
	{
		.attr_id = 0x0003000400000006ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x1ULL,
	},
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-read request */
static const struct ctlv_schema_node mr_read_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 1,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.options */
	{
		.attr_id = 0x0003000400000008ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* mr-read request extension points */
static const struct ctlv_stub_ref mr_read_request_stubs[] = {
	/* mr-options */
	{
		.family_id = 3,
		.set_id = 3,
		.node = 3,
	},
};

/* mr-read reply */
static const struct ctlv_schema_node mr_read_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.first_child = 4,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 16, .max_len = 64 },
	},
};

/* mr-extend request */
static const struct ctlv_schema_node mr_extend_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 5,
		.n_required_children = 2,
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.first_child = 6,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.options */
	{
		.attr_id = 0x0003000400000008ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 8,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 16, .max_len = 64 },
	},
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
};

/* mr-extend request extension points */
static const struct ctlv_stub_ref mr_extend_request_stubs[] = {
	/* mr-options */
	{
		.family_id = 3,
		.set_id = 3,
		.node = 4,
	},
};

/* mr-extend reply */
static const struct ctlv_schema_node mr_extend_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-log-get request */
static const struct ctlv_schema_node mr_log_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 0,
	},
	/* mr.selection */
	{
		.attr_id = 0x000300040000000cULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 256,
		.first_child = 4,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* mr.start-generation */
	{
		.attr_id = 0x000300040000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.end-generation */
	{
		.attr_id = 0x000300040000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-log-get reply */
static const struct ctlv_schema_node mr_log_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 0,
	},
	/* mr.start-generation */
	{
		.attr_id = 0x000300040000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.end-generation */
	{
		.attr_id = 0x000300040000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* log.entry */
	{
		.attr_id = 0x0003000600000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 4,
		.n_children = 8,
		.n_required_children = 4,
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* log.event */
	{
		.attr_id = 0x0003000600000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 2 },
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.first_child = 12,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 14,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* log.mr */
	{
		.attr_id = 0x0003000600000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 16,
		.n_children = 4,
		.n_required_children = 3,
	},
	/* log.ima */
	{
		.attr_id = 0x0003000600000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 20,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 16, .max_len = 64 },
	},
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* log.template-name */
	{
		.attr_id = 0x0003000600000005ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* log.template-data */
	{
		.attr_id = 0x0003000600000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 65536 },
	},
	/* log.violation */
	{
		.attr_id = 0x0003000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* devsec-state-get request */
static const struct ctlv_schema_node devsec_state_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* devsec-state-get reply */
static const struct ctlv_schema_node devsec_state_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 5,
		.n_required_children = 1,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.accepted-generation */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* devsec.tdisp-state */
	{
		.attr_id = 0x0003000700000001ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 4 },
	},
	/* devsec.device-trust */
	{
		.attr_id = 0x0003000700000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 5 },
	},
};

/* devsec-run request */
static const struct ctlv_schema_node devsec_run_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* devsec-run reply */
static const struct ctlv_schema_node devsec_run_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* device-state-changed reply */
static const struct ctlv_schema_node device_state_changed_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* evidence-state-changed reply */
static const struct ctlv_schema_node evidence_state_changed_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 1,
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.generation */
	{
		.attr_id = 0x0003000300000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* evidence.accepted-generation */
	{
		.attr_id = 0x0003000300000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
};

/* mr-log-entry-appended reply */
static const struct ctlv_schema_node mr_log_entry_appended_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* notification.entry */
	{
		.attr_id = 0x0003000800000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 2,
		.n_children = 8,
		.n_required_children = 4,
	},
	/* mr.generation */
	{
		.attr_id = 0x000300040000000bULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* device.generation */
	{
		.attr_id = 0x0003000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* mr.id */
	{
		.attr_id = 0x0003000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* log.event */
	{
		.attr_id = 0x0003000600000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 2 },
	},
	/* mr.value */
	{
		.attr_id = 0x0003000400000009ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 21,
		.first_child = 10,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* record.item */
	{
		.attr_id = 0x0003000500000001ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 12,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* log.mr */
	{
		.attr_id = 0x0003000600000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 14,
		.n_children = 4,
		.n_required_children = 3,
	},
	/* log.ima */
	{
		.attr_id = 0x0003000600000004ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 18,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.data */
	{
		.attr_id = 0x000300040000000aULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 16, .max_len = 64 },
	},
	/* record.format */
	{
		.attr_id = 0x0003000500000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* record.data */
	{
		.attr_id = 0x0003000500000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 4096 },
	},
	/* mr.name */
	{
		.attr_id = 0x0003000400000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* mr.hash-algo */
	{
		.attr_id = 0x0003000400000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 21,
		.range = { .min = 0, .max = 20 },
	},
	/* mr.capabilities */
	{
		.attr_id = 0x0003000400000005ULL,
		.kind = CTLV_SCHEMA_FLAGS,
		.max_occurrences = 1,
		.flags_mask = 0x7ULL,
	},
	/* mr.register-index */
	{
		.attr_id = 0x0003000400000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = 4294967295 },
	},
	/* log.template-name */
	{
		.attr_id = 0x0003000600000005ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 1, .max_len = 128 },
	},
	/* log.template-data */
	{
		.attr_id = 0x0003000600000006ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 65536 },
	},
	/* log.violation */
	{
		.attr_id = 0x0003000600000007ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
};

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema
	ctlv_attestation_ctlv_op_schemas[CTLV_ATTESTATION_N_OPS] = {
	/* device-get */
	{
		.op_id = 0x0003000000000001ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(device_get_request_nodes),
		.reply =
			CTLV_SCHEMA_TREE_STUBS(device_get_reply_nodes,
					       device_get_reply_stubs),
		.sizes = {
			.request_max_len = 16,
			.action_reply_max_len = 12936,
		},
	},
	/* evidence-get */
	{
		.op_id = 0x0003000000000002ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request =
			CTLV_SCHEMA_TREE_STUBS(evidence_get_request_nodes,
					       evidence_get_request_stubs),
		.reply = CTLV_SCHEMA_TREE(evidence_get_reply_nodes),
		.sizes = {
			.request_max_len = 26800,
			.action_reply_max_len = 472,
		},
	},
	/* evidence-accept */
	{
		.op_id = 0x0003000000000003ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(evidence_accept_request_nodes),
		.sizes = {
			.request_max_len = 64,
		},
	},
	/* evidence-accept-clear */
	{
		.op_id = 0x0003000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request =
			CTLV_SCHEMA_TREE(evidence_accept_clear_request_nodes),
		.sizes = {
			.request_max_len = 16,
		},
	},
	/* mr-get */
	{
		.op_id = 0x0003000000000005ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(mr_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(mr_get_reply_nodes),
		.sizes = {
			.request_max_len = 40,
			.action_reply_max_len = 744,
		},
	},
	/* mr-dump */
	{
		.op_id = 0x0003000000000006ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(mr_dump_request_nodes),
		.reply = CTLV_SCHEMA_TREE(mr_dump_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 184,
			.query_reply_suggested_len = 4096,
		},
	},
	/* mr-read */
	{
		.op_id = 0x0003000000000007ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request =
			CTLV_SCHEMA_TREE_STUBS(mr_read_request_nodes,
					       mr_read_request_stubs),
		.reply = CTLV_SCHEMA_TREE(mr_read_reply_nodes),
		.sizes = {
			.request_max_len = 80,
			.query_reply_min_len = 112,
			.query_reply_suggested_len = 4096,
		},
	},
	/* mr-extend */
	{
		.op_id = 0x0003000000000008ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request =
			CTLV_SCHEMA_TREE_STUBS(mr_extend_request_nodes,
					       mr_extend_request_stubs),
		.reply = CTLV_SCHEMA_TREE(mr_extend_reply_nodes),
		.sizes = {
			.request_max_len = 16960,
			.action_reply_max_len = 64,
		},
	},
	/* mr-log-get */
	{
		.op_id = 0x0003000000000009ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(mr_log_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(mr_log_get_reply_nodes),
		.sizes = {
			.request_max_len = 10304,
			.query_reply_min_len = 65600,
			.query_reply_suggested_len = 131072,
		},
	},
	/* devsec-state-get */
	{
		.op_id = 0x000300000000000aULL,
		.type = CTLV_OP_TYPE_QUERY,
		.fd_access = CTLV_OP_FD_ACCESS_READ,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(devsec_state_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(devsec_state_get_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 40,
			.query_reply_suggested_len = 1024,
		},
	},
	/* devsec-run */
	{
		.op_id = 0x000300000000000bULL,
		.type = CTLV_OP_TYPE_ACTION,
		.fd_access = CTLV_OP_FD_ACCESS_READ_WRITE,
		.availability = CTLV_OP_AVAILABILITY_DYNAMIC,
		.optional = true,
		.request = CTLV_SCHEMA_TREE(devsec_run_request_nodes),
		.reply = CTLV_SCHEMA_TREE(devsec_run_reply_nodes),
		.sizes = {
			.request_max_len = 64,
			.action_reply_max_len = 40,
		},
	},
	/* device-state-changed */
	{
		.op_id = 0x000300000000000cULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(device_state_changed_reply_nodes),
		.sizes = {
			.event_record_max_len = 64,
		},
	},
	/* evidence-state-changed */
	{
		.op_id = 0x000300000000000dULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(evidence_state_changed_reply_nodes),
		.sizes = {
			.event_record_max_len = 112,
		},
	},
	/* mr-log-entry-appended */
	{
		.op_id = 0x000300000000000eULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(mr_log_entry_appended_reply_nodes),
		.sizes = {
			.event_record_max_len = 83496,
		},
	},
};

static_assert(ARRAY_SIZE(ctlv_attestation_ctlv_op_schemas) ==
	      CTLV_ATTESTATION_N_OPS);

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_attestation_ctlv_schema = {
	.name = "ctlv-attestation",
	.family_id = 3,
	.ops = ctlv_attestation_ctlv_op_schemas,
	.n_ops = CTLV_ATTESTATION_N_OPS,
};

/* What this family and the chain below it cost a uevent. */
static_assert(107 <= CTLV_UEVENT_MAX_LEN);
static_assert(4 <= CTLV_UEVENT_MAX_VARS);

/* The generated numbers against the limits they were computed from. */
static_assert(131072 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(83496 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(3 <= CTLV_MAX_DECLARED_NEST_DEPTH);

/* One entry per action and query this family declares, in declaration order. */
#define CTLV_ATTESTATION_N_BINDINGS 11

static const struct ctlv_op
	ctlv_attestation_ctlv_ops[CTLV_ATTESTATION_N_BINDINGS] = {
	/* device-get */
	{
		.action = ctlv_attestation_device_get_action,
	},
	/* evidence-get */
	{
		.action = ctlv_attestation_evidence_get_action,
	},
	/* evidence-accept */
	{
		.action = ctlv_attestation_evidence_accept_action,
	},
	/* evidence-accept-clear */
	{
		.action = ctlv_attestation_evidence_accept_clear_action,
	},
	/* mr-get */
	{
		.action = ctlv_attestation_mr_get_action,
	},
	/* mr-dump */
	{
		.query = ctlv_attestation_mr_dump_query,
	},
	/* mr-read */
	{
		.query = ctlv_attestation_mr_read_query,
	},
	/* mr-extend */
	{
		.action = ctlv_attestation_mr_extend_action,
	},
	/* mr-log-get */
	{
		.query = ctlv_attestation_mr_log_get_query,
	},
	/* devsec-state-get */
	{
		.query = ctlv_attestation_devsec_state_get_query,
	},
	/* devsec-run */
	{
		.action = ctlv_attestation_devsec_run_action,
	},
};

static_assert(ARRAY_SIZE(ctlv_attestation_ctlv_ops) ==
	      CTLV_ATTESTATION_N_BINDINGS);

/* What a driver creates devices with. */
const struct ctlv_family ctlv_attestation_ctlv_family = {
	.schema = &ctlv_attestation_ctlv_schema,
	.parent = &ctlv_ctlv_family,
	.ops = ctlv_attestation_ctlv_ops,
	.n_ops = CTLV_ATTESTATION_N_BINDINGS,
	.pre = ctlv_attestation_op_pre,
	.post = ctlv_attestation_op_post,
	.priv_size = ctlv_attestation_priv_size,
	.init = ctlv_attestation_init,
	.post_init = ctlv_attestation_post_init,
	.pre_fini = ctlv_attestation_pre_fini,
	.fini = ctlv_attestation_fini,
};
EXPORT_SYMBOL_NS_GPL(ctlv_attestation_ctlv_family, "CTLV");
