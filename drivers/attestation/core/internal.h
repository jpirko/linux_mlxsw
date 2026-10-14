/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ATTESTATION_INTERNAL_H
#define _ATTESTATION_INTERNAL_H

#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/mutex.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_gen.h"

#define ATTESTATION_REGISTERED XA_MARK_1

struct attestation_mr {
	const struct attestation_mr_props *props;
};

bool attestation_registered(struct attestation_dev *att);
void attestation_stop(struct attestation_dev *att);
void attestation_mr_changed(struct attestation_dev *att);

int attestation_provider_validate(const struct attestation_provider *provider,
				  const struct ctlv_family *family);
void attestation_provide_ops(struct attestation_dev *att);

struct attestation_mr *attestation_mr_lookup_live(struct attestation_dev *att, unsigned int mr_id);
int attestation_mr_values_validate(const struct attestation_mr_props *props,
				   const struct attestation_digest_set *values, bool all_banks);
int attestation_mr_extend_locked(struct attestation_dev *att, struct attestation_mr *mr,
				 const struct attestation_digest_set *values);

int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr);

#endif /* _ATTESTATION_INTERNAL_H */
