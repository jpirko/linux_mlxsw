// SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
/*
 * Generated from Documentation/ctlv/specs/ctlv-attestation-sim.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN kernel source */

#define _CTLV_GENERATED_METADATA_ONLY
#include <linux/array_size.h>
#include <linux/build_bug.h>
#include <linux/ctlv_family.h>
#include <linux/export.h>

#include <uapi/ctlv/ctlv_gen.h>
#include <uapi/ctlv/ctlv_attestation_sim_gen.h>

#include "ctlv_attestation_sim_gen.h"

static const struct ctlv_node_schema device_info_fill_node_schemas[] = {
	CTLV_NODE_SCHEMA_ROOT(1, 1, 1),
	CTLV_NODE_SCHEMA_STRING(CTLV_ATTESTATION_SIM_T_DEVICE_INFO_NAME, 1, 1, 63),
};

/* The extension points this family fills. */
static const struct ctlv_ext_fill_schema ctlv_attestation_sim_ext_fill_schemas[] = {
	CTLV_EXT_FILL_SCHEMA(CTLV_FAMILY_ATTESTATION, CTLV_ATTESTATION_EXT_DEVICE_INFO,
			     device_info),
};

/* The enum extension points this family fills. */
static const struct ctlv_enum_fill_schema ctlv_attestation_sim_enum_fill_schemas[] = {
	CTLV_ENUM_FILL_SCHEMA(CTLV_FAMILY_ATTESTATION,
			      CTLV_ATTESTATION_ENUM_EXT_EVIDENCE_VENDOR_FORMAT, 3),
};

/* The family ID is its position in the central registry. */
const struct ctlv_family_schema ctlv_attestation_sim_schema = {
	.name = "ctlv-attestation-sim",
	.family_id = 5,
	.ext_fill_schemas = ctlv_attestation_sim_ext_fill_schemas,
	.ext_fill_schemas_count = ARRAY_SIZE(ctlv_attestation_sim_ext_fill_schemas),
	.enum_fill_schemas = ctlv_attestation_sim_enum_fill_schemas,
	.enum_fill_schemas_count = ARRAY_SIZE(ctlv_attestation_sim_enum_fill_schemas),
};

/* Maximum uevent cost for this family chain. */
static_assert(150 <= CTLV_UEVENT_MAX_LEN);
static_assert(5 <= CTLV_UEVENT_MAX_VARS);

/* Extension-point callbacks. */
static const struct ctlv_ext_ops ctlv_attestation_sim_ext_ops[] = {
	CTLV_EXT_OPS(CTLV_FAMILY_ATTESTATION, CTLV_ATTESTATION_EXT_DEVICE_INFO,
		     ctlv_attestation_sim_device_info),
};

/* Family descriptor used by ctlv_init(). */
const struct ctlv_family ctlv_attestation_sim_family = {
	.schema = &ctlv_attestation_sim_schema,
	.parent = &ctlv_attestation_family,
	.ext_ops = ctlv_attestation_sim_ext_ops,
	.ext_ops_count = ARRAY_SIZE(ctlv_attestation_sim_ext_ops),
};
EXPORT_SYMBOL_NS_GPL(ctlv_attestation_sim_family, "CTLV");
