/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ATTESTATION_INTERNAL_H
#define _ATTESTATION_INTERNAL_H

#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/mutex.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_gen.h"

#define ATTESTATION_REGISTERED XA_MARK_1

struct attestation {
	struct ctlv_device *ctlvdev;
	const struct attestation_provider *provider;
	unsigned int index;

	/* Mutable instance state. */
	struct mutex lock;

	bool initialized;
};

int attestation_core_init(struct attestation *att,
			  struct ctlv_device *ctlvdev,
			  const struct attestation_provider *provider);
void attestation_core_fini(struct attestation *att);
bool attestation_registered(struct attestation *att);
void attestation_stop(struct attestation *att);

int attestation_provider_validate(const struct attestation_provider *provider);
void attestation_provide_ops(struct attestation *att);

#endif /* _ATTESTATION_INTERNAL_H */
