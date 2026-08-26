// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv.yaml */
/* CTLV-GEN family source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>

#include "ctlv_family_gen.h"

/* One entry per action and query of the root family, in declaration order. */
#define CTLV_N_BINDINGS 7

static const struct ctlv_op ctlv_ctlv_ops[CTLV_N_BINDINGS] = {
	/* families-dump */
	{
		.query = ctlv_families_dump,
	},
	/* ops-dump */
	{
		.query = ctlv_ops_dump,
	},
	/* op-schema-get */
	{
		.query = ctlv_op_schema_get,
	},
	/* subscriptions-add */
	{
		.action = ctlv_subscriptions_add,
	},
	/* subscriptions-remove */
	{
		.action = ctlv_subscriptions_remove,
	},
	/* event-queue-get */
	{
		.query = ctlv_event_queue_get,
	},
	/* event-queue-set */
	{
		.action = ctlv_event_queue_set,
	},
};

static_assert(ARRAY_SIZE(ctlv_ctlv_ops) == CTLV_N_BINDINGS);

/* What the framework answers the root family's ops through. */
const struct ctlv_family ctlv_ctlv_family = {
	.schema = &ctlv_ctlv_schema,
	.ops = ctlv_ctlv_ops,
	.n_ops = CTLV_N_BINDINGS,
	.pre = ctlv_op_pre,
	.post = ctlv_op_post,
};
