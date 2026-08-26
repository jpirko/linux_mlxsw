// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/export.h>
#include <linux/limits.h>

#include <uapi/linux/ctlv_gen.h>

#include <linux/ctlv_schema_gen.h>

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);
static_assert(sizeof(struct ctlv_event_hdr) == 24);

/* families-dump request */
static const struct ctlv_schema_node families_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* families-dump reply */
static const struct ctlv_schema_node families_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* family.entry */
	{
		.attr_id = 0x0000000100000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 2,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* family.id */
	{
		.attr_id = 0x0000000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* family.name */
	{
		.attr_id = 0x0000000100000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 63 },
	},
};

/* ops-dump request */
static const struct ctlv_schema_node ops_dump_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* ops-dump reply */
static const struct ctlv_schema_node ops_dump_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* op.entry */
	{
		.attr_id = 0x000000040000000bULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.first_child = 2,
		.n_children = 9,
		.n_required_children = 2,
	},
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = U64_MAX },
	},
	/* op.type */
	{
		.attr_id = 0x0000000400000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* op.availability */
	{
		.attr_id = 0x0000000400000003ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 2 },
	},
	/* op.disabled */
	{
		.attr_id = 0x0000000400000004ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* op.request-max-len */
	{
		.attr_id = 0x0000000400000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* op.action-reply-max-len */
	{
		.attr_id = 0x0000000400000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* op.query-reply-min-len */
	{
		.attr_id = 0x0000000400000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* op.query-reply-suggested-len */
	{
		.attr_id = 0x0000000400000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* op.event-record-max-len */
	{
		.attr_id = 0x0000000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* op-schema-get request */
static const struct ctlv_schema_node op_schema_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = U64_MAX },
	},
};

/* op-schema-get reply */
static const struct ctlv_schema_node op_schema_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 0,
	},
	/* schema.request */
	{
		.attr_id = 0x0000000500000012ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 3,
		.n_children = 17,
		.n_required_children = 3,
	},
	/* schema.reply */
	{
		.attr_id = 0x0000000500000013ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 20,
		.n_children = 17,
		.n_required_children = 3,
	},
	/* schema.attr-id */
	{
		.attr_id = 0x0000000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.kind */
	{
		.attr_id = 0x0000000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max-occurrences */
	{
		.attr_id = 0x0000000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.min */
	{
		.attr_id = 0x0000000500000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max */
	{
		.attr_id = 0x0000000500000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.min-length */
	{
		.attr_id = 0x0000000500000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max-length */
	{
		.attr_id = 0x0000000500000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.flags-mask */
	{
		.attr_id = 0x0000000500000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.unit */
	{
		.attr_id = 0x0000000500000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.scale */
	{
		.attr_id = 0x000000050000000aULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = S64_MIN, .max = S64_MAX },
	},
	/* schema.blob-direction */
	{
		.attr_id = 0x000000050000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* schema.blob-backings */
	{
		.attr_id = 0x000000050000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.blob-max-length */
	{
		.attr_id = 0x000000050000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.blob-alignment */
	{
		.attr_id = 0x000000050000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.reenters */
	{
		.attr_id = 0x000000050000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = U64_MAX },
	},
	/* schema.attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.reenters = true,
		.first_child = 3,
		.n_children = 17,
		.n_required_children = 3,
	},
	/* schema.optional-attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000011ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.reenters = true,
		.first_child = 3,
		.n_children = 17,
		.n_required_children = 3,
	},
	/* schema.attr-id */
	{
		.attr_id = 0x0000000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.kind */
	{
		.attr_id = 0x0000000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max-occurrences */
	{
		.attr_id = 0x0000000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.min */
	{
		.attr_id = 0x0000000500000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max */
	{
		.attr_id = 0x0000000500000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.min-length */
	{
		.attr_id = 0x0000000500000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.max-length */
	{
		.attr_id = 0x0000000500000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.flags-mask */
	{
		.attr_id = 0x0000000500000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.unit */
	{
		.attr_id = 0x0000000500000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.scale */
	{
		.attr_id = 0x000000050000000aULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.range = { .min = S64_MIN, .max = S64_MAX },
	},
	/* schema.blob-direction */
	{
		.attr_id = 0x000000050000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 3 },
	},
	/* schema.blob-backings */
	{
		.attr_id = 0x000000050000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.blob-max-length */
	{
		.attr_id = 0x000000050000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.blob-alignment */
	{
		.attr_id = 0x000000050000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* schema.reenters */
	{
		.attr_id = 0x000000050000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = U64_MAX },
	},
	/* schema.attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.reenters = true,
		.first_child = 20,
		.n_children = 17,
		.n_required_children = 3,
	},
	/* schema.optional-attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000011ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.reenters = true,
		.first_child = 20,
		.n_children = 17,
		.n_required_children = 3,
	},
};

/* subscriptions-add request */
static const struct ctlv_schema_node subscriptions_add_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* event.op-id */
	{
		.attr_id = 0x0000000600000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.range = { .min = 1, .max = U64_MAX },
	},
};

/* subscriptions-remove request */
static const struct ctlv_schema_node subscriptions_remove_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* event.op-id */
	{
		.attr_id = 0x0000000600000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.range = { .min = 1, .max = U64_MAX },
	},
};

/* event-queue-get request */
static const struct ctlv_schema_node event_queue_get_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* event-queue-get reply */
static const struct ctlv_schema_node event_queue_get_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 2,
		.n_required_children = 2,
	},
	/* event.queue-len */
	{
		.attr_id = 0x0000000600000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 16777216 },
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	/* event.queue-used */
	{
		.attr_id = 0x0000000600000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

/* event-queue-set request */
static const struct ctlv_schema_node event_queue_set_request_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 1,
		.n_required_children = 1,
	},
	/* event.queue-len */
	{
		.attr_id = 0x0000000600000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 16777216 },
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

/* op-changed reply */
static const struct ctlv_schema_node op_changed_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 1, .max = U64_MAX },
	},
	/* op.disabled */
	{
		.attr_id = 0x0000000400000004ULL,
		.kind = CTLV_SCHEMA_BOOL,
		.max_occurrences = 1,
	},
	/* op.state-generation */
	{
		.attr_id = 0x000000040000000aULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* event-overflow reply */
static const struct ctlv_schema_node event_overflow_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* event.first-sequence */
	{
		.attr_id = 0x0000000600000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* event.last-sequence */
	{
		.attr_id = 0x0000000600000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* event.dropped-count */
	{
		.attr_id = 0x0000000600000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

/* device-removed reply */
static const struct ctlv_schema_node device_removed_reply_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
	},
};

/* error-info, which the core builds */
static const struct ctlv_schema_node error_info_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 7,
		.n_required_children = 0,
	},
	/* error.reason */
	{
		.attr_id = 0x0000000300000001ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.range = { .min = 1, .max = 11 },
	},
	/* error.message */
	{
		.attr_id = 0x0000000300000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.length = { .min_len = 0, .max_len = 255 },
	},
	/* error.bad-attr-offset */
	{
		.attr_id = 0x0000000300000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* error.nest-offset */
	{
		.attr_id = 0x0000000300000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* error.missing-attr-id */
	{
		.attr_id = 0x0000000300000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* error.expected-generation */
	{
		.attr_id = 0x0000000300000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
	/* error.current-generation */
	{
		.attr_id = 0x0000000300000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.range = { .min = 0, .max = U64_MAX },
	},
};

const struct ctlv_schema_tree ctlv_error_info_schema =
	CTLV_SCHEMA_TREE(error_info_nodes);
EXPORT_SYMBOL_NS_GPL(ctlv_error_info_schema, "CTLV");

static_assert(CTLV_ERROR_INFO_MAX_LEN <= CTLV_MAX_ERROR_LEN);

/* The ops this family declares, in declaration order. */
const struct ctlv_op_schema ctlv_ctlv_op_schemas[CTLV_N_OPS] = {
	/* families-dump */
	{
		.op_id = 0x0000000000000001ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(families_dump_request_nodes),
		.reply = CTLV_SCHEMA_TREE(families_dump_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 112,
			.query_reply_suggested_len = 65536,
		},
	},
	/* ops-dump */
	{
		.op_id = 0x0000000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(ops_dump_request_nodes),
		.reply = CTLV_SCHEMA_TREE(ops_dump_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 56,
			.query_reply_suggested_len = 65536,
		},
	},
	/* op-schema-get */
	{
		.op_id = 0x0000000000000003ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(op_schema_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(op_schema_get_reply_nodes),
		.sizes = {
			.request_max_len = 40,
			.query_reply_min_len = 552,
			.query_reply_suggested_len = 65536,
		},
	},
	/* subscriptions-add */
	{
		.op_id = 0x0000000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(subscriptions_add_request_nodes),
		.sizes = {
			.request_max_len = 6160,
		},
	},
	/* subscriptions-remove */
	{
		.op_id = 0x0000000000000005ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(subscriptions_remove_request_nodes),
		.sizes = {
			.request_max_len = 6160,
		},
	},
	/* event-queue-get */
	{
		.op_id = 0x0000000000000006ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(event_queue_get_request_nodes),
		.reply = CTLV_SCHEMA_TREE(event_queue_get_reply_nodes),
		.sizes = {
			.request_max_len = 16,
			.query_reply_min_len = 40,
			.query_reply_suggested_len = 65536,
		},
	},
	/* event-queue-set */
	{
		.op_id = 0x0000000000000007ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request = CTLV_SCHEMA_TREE(event_queue_set_request_nodes),
		.sizes = {
			.request_max_len = 40,
		},
	},
	/* op-changed */
	{
		.op_id = 0x0000000000000008ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(op_changed_reply_nodes),
		.sizes = {
			.event_record_max_len = 112,
		},
	},
	/* event-overflow */
	{
		.op_id = 0x0000000000000009ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(event_overflow_reply_nodes),
		.sizes = {
			.event_record_max_len = 112,
		},
	},
	/* device-removed */
	{
		.op_id = 0x000000000000000aULL,
		.type = CTLV_OP_TYPE_EVENT,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.reply = CTLV_SCHEMA_TREE(device_removed_reply_nodes),
		.sizes = {
			.event_record_max_len = 40,
		},
	},
};

static_assert(ARRAY_SIZE(ctlv_ctlv_op_schemas) == CTLV_N_OPS);

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_ctlv_schema = {
	.name = "ctlv",
	.family_id = 0,
	.ops = ctlv_ctlv_op_schemas,
	.n_ops = CTLV_N_OPS,
};
EXPORT_SYMBOL_NS_GPL(ctlv_ctlv_schema, "CTLV");

/* The generated numbers against the limits they were computed from. */
static_assert(65536 <= CTLV_MAX_INLINE_MESSAGE_LEN);
static_assert(112 <= CTLV_MAX_EVENT_QUEUE_LEN);
static_assert(2 <= CTLV_MAX_DECLARED_NEST_DEPTH);
