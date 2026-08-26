// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN user source */

#include <stdint.h>

#include <ctlv.h>

/* What each op's messages may hold, composed as the core composes them. */

/* families-dump request */
static const struct ctlv_schema_node families_dump_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* family.entry, attrs */
static const struct ctlv_schema_node families_dump_reply_entry_attrs[] = {
	/* family.id */
	{
		.attr_id = 0x0000000100000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* family.name */
	{
		.attr_id = 0x0000000100000002ULL,
		.kind = CTLV_SCHEMA_STRING,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN_LENGTH |
			   CTLV_SCHEMA_HAS_MAX_LENGTH,
		.min_length = 0,
		.max_length = 63,
	},
};

/* root, attrs */
static const struct ctlv_schema_node families_dump_reply_attrs[] = {
	/* family.entry */
	{
		.attr_id = 0x0000000100000003ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = families_dump_reply_entry_attrs,
		.n_attrs = 2,
	},
};

/* families-dump reply */
static const struct ctlv_schema_node families_dump_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = families_dump_reply_attrs,
	.n_attrs = 1,
};

/* ops-dump request */
static const struct ctlv_schema_node ops_dump_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* op.entry, attrs */
static const struct ctlv_schema_node ops_dump_reply_entry_attrs[] = {
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	/* op.type */
	{
		.attr_id = 0x0000000400000002ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
};

/* op.entry, optional attrs */
static const struct ctlv_schema_node ops_dump_reply_entry_optional_attrs[] = {
	/* op.availability */
	{
		.attr_id = 0x0000000400000003ULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 2,
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
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* op.action-reply-max-len */
	{
		.attr_id = 0x0000000400000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* op.query-reply-min-len */
	{
		.attr_id = 0x0000000400000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* op.query-reply-suggested-len */
	{
		.attr_id = 0x0000000400000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* op.event-record-max-len */
	{
		.attr_id = 0x0000000400000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* root, attrs */
static const struct ctlv_schema_node ops_dump_reply_attrs[] = {
	/* op.entry */
	{
		.attr_id = 0x000000040000000bULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.attrs = ops_dump_reply_entry_attrs,
		.n_attrs = 2,
		.optional_attrs = ops_dump_reply_entry_optional_attrs,
		.n_optional_attrs = 7,
	},
};

/* ops-dump reply */
static const struct ctlv_schema_node ops_dump_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = ops_dump_reply_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node op_schema_get_request_attrs[] = {
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

/* op-schema-get request */
static const struct ctlv_schema_node op_schema_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = op_schema_get_request_attrs,
	.n_attrs = 1,
};

/* schema.request, attrs */
static const struct ctlv_schema_node op_schema_get_reply_request_attrs[] = {
	/* schema.attr-id */
	{
		.attr_id = 0x0000000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.kind */
	{
		.attr_id = 0x0000000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max-occurrences */
	{
		.attr_id = 0x0000000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* schema.request, optional attrs */
static const struct ctlv_schema_node
	op_schema_get_reply_request_optional_attrs[] = {
	/* schema.min */
	{
		.attr_id = 0x0000000500000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max */
	{
		.attr_id = 0x0000000500000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.min-length */
	{
		.attr_id = 0x0000000500000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max-length */
	{
		.attr_id = 0x0000000500000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.flags-mask */
	{
		.attr_id = 0x0000000500000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.unit */
	{
		.attr_id = 0x0000000500000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.scale */
	{
		.attr_id = 0x000000050000000aULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	/* schema.blob-direction */
	{
		.attr_id = 0x000000050000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	/* schema.blob-backings */
	{
		.attr_id = 0x000000050000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.blob-max-length */
	{
		.attr_id = 0x000000050000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.blob-alignment */
	{
		.attr_id = 0x000000050000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.reenters */
	{
		.attr_id = 0x000000050000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	/* schema.attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	/* schema.optional-attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000011ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* schema.reply, attrs */
static const struct ctlv_schema_node op_schema_get_reply_reply_attrs[] = {
	/* schema.attr-id */
	{
		.attr_id = 0x0000000500000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.kind */
	{
		.attr_id = 0x0000000500000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max-occurrences */
	{
		.attr_id = 0x0000000500000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* schema.reply, optional attrs */
static const struct ctlv_schema_node
	op_schema_get_reply_reply_optional_attrs[] = {
	/* schema.min */
	{
		.attr_id = 0x0000000500000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max */
	{
		.attr_id = 0x0000000500000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.min-length */
	{
		.attr_id = 0x0000000500000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.max-length */
	{
		.attr_id = 0x0000000500000007ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.flags-mask */
	{
		.attr_id = 0x0000000500000008ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.unit */
	{
		.attr_id = 0x0000000500000009ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.scale */
	{
		.attr_id = 0x000000050000000aULL,
		.kind = CTLV_SCHEMA_SINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = (__u64)INT64_MIN,
		.max = INT64_MAX,
	},
	/* schema.blob-direction */
	{
		.attr_id = 0x000000050000000bULL,
		.kind = CTLV_SCHEMA_ENUM,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = 3,
	},
	/* schema.blob-backings */
	{
		.attr_id = 0x000000050000000cULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.blob-max-length */
	{
		.attr_id = 0x000000050000000dULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.blob-alignment */
	{
		.attr_id = 0x000000050000000eULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* schema.reenters */
	{
		.attr_id = 0x000000050000000fULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
	/* schema.attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000010ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
	/* schema.optional-attrs, again from schema-node */
	{
		.attr_id = 0x0000000500000011ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 0, /* unlimited */
		.present = CTLV_SCHEMA_HAS_REENTERS,
		.reenters = 1,
	},
};

/* root, optional attrs */
static const struct ctlv_schema_node op_schema_get_reply_optional_attrs[] = {
	/* schema.request */
	{
		.attr_id = 0x0000000500000012ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = op_schema_get_reply_request_attrs,
		.n_attrs = 3,
		.optional_attrs = op_schema_get_reply_request_optional_attrs,
		.n_optional_attrs = 14,
	},
	/* schema.reply */
	{
		.attr_id = 0x0000000500000013ULL,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.attrs = op_schema_get_reply_reply_attrs,
		.n_attrs = 3,
		.optional_attrs = op_schema_get_reply_reply_optional_attrs,
		.n_optional_attrs = 14,
	},
};

/* op-schema-get reply */
static const struct ctlv_schema_node op_schema_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.optional_attrs = op_schema_get_reply_optional_attrs,
	.n_optional_attrs = 2,
};

/* root, attrs */
static const struct ctlv_schema_node subscriptions_add_request_attrs[] = {
	/* event.op-id */
	{
		.attr_id = 0x0000000600000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

/* subscriptions-add request */
static const struct ctlv_schema_node subscriptions_add_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = subscriptions_add_request_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node subscriptions_remove_request_attrs[] = {
	/* event.op-id */
	{
		.attr_id = 0x0000000600000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 256,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
	},
};

/* subscriptions-remove request */
static const struct ctlv_schema_node subscriptions_remove_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = subscriptions_remove_request_attrs,
	.n_attrs = 1,
};

/* event-queue-get request */
static const struct ctlv_schema_node event_queue_get_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* root, attrs */
static const struct ctlv_schema_node event_queue_get_reply_attrs[] = {
	/* event.queue-len */
	{
		.attr_id = 0x0000000600000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX |
			   CTLV_SCHEMA_HAS_UNIT | CTLV_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 0x1000000ULL,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
	/* event.queue-used */
	{
		.attr_id = 0x0000000600000003ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX |
			   CTLV_SCHEMA_HAS_UNIT | CTLV_SCHEMA_HAS_SCALE,
		.min = 0,
		.max = UINT64_MAX,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

/* event-queue-get reply */
static const struct ctlv_schema_node event_queue_get_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_queue_get_reply_attrs,
	.n_attrs = 2,
};

/* root, attrs */
static const struct ctlv_schema_node event_queue_set_request_attrs[] = {
	/* event.queue-len */
	{
		.attr_id = 0x0000000600000002ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX |
			   CTLV_SCHEMA_HAS_UNIT | CTLV_SCHEMA_HAS_SCALE,
		.min = 1,
		.max = 0x1000000ULL,
		.unit = CTLV_UNIT_BYTES,
		.scale = 0,
	},
};

/* event-queue-set request */
static const struct ctlv_schema_node event_queue_set_request = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_queue_set_request_attrs,
	.n_attrs = 1,
};

/* root, attrs */
static const struct ctlv_schema_node op_changed_reply_attrs[] = {
	/* op.id */
	{
		.attr_id = 0x0000000400000001ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 1,
		.max = UINT64_MAX,
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
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* op-changed reply */
static const struct ctlv_schema_node op_changed_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = op_changed_reply_attrs,
	.n_attrs = 3,
};

/* root, attrs */
static const struct ctlv_schema_node event_overflow_reply_attrs[] = {
	/* event.first-sequence */
	{
		.attr_id = 0x0000000600000004ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* event.last-sequence */
	{
		.attr_id = 0x0000000600000005ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
	/* event.dropped-count */
	{
		.attr_id = 0x0000000600000006ULL,
		.kind = CTLV_SCHEMA_UINT,
		.max_occurrences = 1,
		.present = CTLV_SCHEMA_HAS_MIN | CTLV_SCHEMA_HAS_MAX,
		.min = 0,
		.max = UINT64_MAX,
	},
};

/* event-overflow reply */
static const struct ctlv_schema_node event_overflow_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
	.attrs = event_overflow_reply_attrs,
	.n_attrs = 3,
};

/* device-removed reply */
static const struct ctlv_schema_node device_removed_reply = {
	.attr_id = CTLV_ATTR_ID_ROOT,
	.kind = CTLV_SCHEMA_NEST,
	.max_occurrences = 1,
};

/* Every op a device of this family answers, in chain declaration order. */
static const struct ctlv_op_spec ctlv_ctlv_op_specs[] = {
	/* families-dump */
	{
		.op_id = 0x0000000000000001ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 112,
		.query_reply_suggested_len = 65536,
		.request = &families_dump_request,
		.reply = &families_dump_reply,
	},
	/* ops-dump */
	{
		.op_id = 0x0000000000000002ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 56,
		.query_reply_suggested_len = 65536,
		.request = &ops_dump_request,
		.reply = &ops_dump_reply,
	},
	/* op-schema-get */
	{
		.op_id = 0x0000000000000003ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.query_reply_min_len = 552,
		.query_reply_suggested_len = 65536,
		.request = &op_schema_get_request,
		.reply = &op_schema_get_reply,
	},
	/* subscriptions-add */
	{
		.op_id = 0x0000000000000004ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 6160,
		.request = &subscriptions_add_request,
	},
	/* subscriptions-remove */
	{
		.op_id = 0x0000000000000005ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 6160,
		.request = &subscriptions_remove_request,
	},
	/* event-queue-get */
	{
		.op_id = 0x0000000000000006ULL,
		.type = CTLV_OP_TYPE_QUERY,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 16,
		.query_reply_min_len = 40,
		.query_reply_suggested_len = 65536,
		.request = &event_queue_get_request,
		.reply = &event_queue_get_reply,
	},
	/* event-queue-set */
	{
		.op_id = 0x0000000000000007ULL,
		.type = CTLV_OP_TYPE_ACTION,
		.availability = CTLV_OP_AVAILABILITY_FIXED,
		.request_max_len = 40,
		.request = &event_queue_set_request,
	},
	/* op-changed */
	{
		.op_id = 0x0000000000000008ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
		.reply = &op_changed_reply,
	},
	/* event-overflow */
	{
		.op_id = 0x0000000000000009ULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 112,
		.reply = &event_overflow_reply,
	},
	/* device-removed */
	{
		.op_id = 0x000000000000000aULL,
		.type = CTLV_OP_TYPE_EVENT,
		.event_record_max_len = 40,
		.reply = &device_removed_reply,
	},
};

/* What a device of this family is, for a caller holding one against it. */
const struct ctlv_family_spec ctlv_ctlv_spec = {
	.name = "ctlv",
	.family_id = 0,
	.op = ctlv_ctlv_op_specs,
	.n_op = 10,
};
