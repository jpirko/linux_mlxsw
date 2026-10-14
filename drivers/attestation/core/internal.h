/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ATTESTATION_INTERNAL_H
#define _ATTESTATION_INTERNAL_H

#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/mutex.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_gen.h"

#define ATTESTATION_REGISTERED XA_MARK_1

bool attestation_registered(struct attestation_dev *att);
void attestation_stop(struct attestation_dev *att);

int attestation_provider_validate(const struct attestation_provider *provider,
				  const struct ctlv_family *family);
void attestation_provide_ops(struct attestation_dev *att);

#endif /* _ATTESTATION_INTERNAL_H */
