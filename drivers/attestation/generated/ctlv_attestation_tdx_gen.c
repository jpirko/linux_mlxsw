// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-attestation-tdx.yaml */
/* CTLV-GEN kernel source */

#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv.h>
#include <linux/export.h>

#include <uapi/linux/ctlv_gen.h>

#include "ctlv_attestation_tdx_gen.h"

/* Every length below was computed from these. */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(CTLV_ALIGNTO == 8);

/* device-info of ctlv-attestation, filled here */
static const struct ctlv_schema_node device_info_fill_nodes[] = {
	/* root */
	{
		.attr_id = CTLV_ATTR_ID_ROOT,
		.kind = CTLV_SCHEMA_NEST,
		.max_occurrences = 1,
		.first_child = 1,
		.n_children = 3,
		.n_required_children = 3,
	},
	/* tdx.mrconfigid */
	{
		.attr_id = 0x0005000100000001ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 48, .max_len = 48 },
	},
	/* tdx.mrowner */
	{
		.attr_id = 0x0005000100000002ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 48, .max_len = 48 },
	},
	/* tdx.mrownerconfig */
	{
		.attr_id = 0x0005000100000003ULL,
		.kind = CTLV_SCHEMA_BINARY,
		.max_occurrences = 1,
		.length = { .min_len = 48, .max_len = 48 },
	},
};

/* The extension points this family fills. */
static const struct ctlv_stub_fill ctlv_attestation_tdx_ctlv_fills[] = {
	/* device-info */
	{
		.family_id = 3,
		.set_id = 1,
		.members = CTLV_SCHEMA_TREE(device_info_fill_nodes),
	},
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_attestation_tdx_ctlv_schema = {
	.name = "ctlv-attestation-tdx",
	.family_id = 5,
	.fills = ctlv_attestation_tdx_ctlv_fills,
	.n_fills = ARRAY_SIZE(ctlv_attestation_tdx_ctlv_fills),
};

/* What this family and the chain below it cost a uevent. */
static_assert(150 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* One entry per action and query this family declares, in declaration order. */
#define CTLV_ATTESTATION_TDX_N_BINDINGS 0

/* What this family does with each extension point it fills. */
static const struct ctlv_stub_ops ctlv_attestation_tdx_ctlv_stubs[] = {
	/* device-info */
	{
		.family_id = 3,
		.set_id = 1,
		.fill = ctlv_attestation_tdx_device_info_fill,
		.read = ctlv_attestation_tdx_device_info_read,
	},
};

/* What a driver creates devices with. */
const struct ctlv_family ctlv_attestation_tdx_ctlv_family = {
	.schema = &ctlv_attestation_tdx_ctlv_schema,
	.parent = &ctlv_attestation_ctlv_family,
	.n_ops = CTLV_ATTESTATION_TDX_N_BINDINGS,
	.pre = ctlv_attestation_tdx_op_pre,
	.post = ctlv_attestation_tdx_op_post,
	.priv_size = ctlv_attestation_tdx_priv_size,
	.pre_init = ctlv_attestation_tdx_pre_init,
	.init = ctlv_attestation_tdx_init,
	.fini = ctlv_attestation_tdx_fini,
	.stubs = ctlv_attestation_tdx_ctlv_stubs,
	.n_stubs = ARRAY_SIZE(ctlv_attestation_tdx_ctlv_stubs),
};
EXPORT_SYMBOL_NS_GPL(ctlv_attestation_tdx_ctlv_family, "CTLV");
