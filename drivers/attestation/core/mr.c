// SPDX-License-Identifier: GPL-2.0-only
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/limits.h>

#include "internal.h"

/* @values follow the banks' order, so each bank comes once at most. */
int attestation_mr_values_validate(const struct attestation_mr_props *props,
				   const struct attestation_digest_set *values, bool all_banks)
{
	size_t bank = 0, index;

	if (!values || !values->values || !values->values_count ||
	    (all_banks && values->values_count != props->algorithms_count))
		return -EINVAL;
	for (index = 0; index < values->values_count; index++, bank++) {
		while (bank < props->algorithms_count &&
		       props->algorithms[bank] != values->values[index].algorithm)
			bank++;
		if (bank == props->algorithms_count)
			return -EINVAL;
	}
	return 0;
}

int attestation_mr_extend_locked(struct attestation_dev *att, struct attestation_mr *mr,
				 const struct attestation_digest_set *values)
{
	int ret;

	lockdep_assert_held(&att->lock);
	if (att->mr_generation == UINT_MAX) {
		dev_warn(att->node.dev.parent,
			 "attestation instance %u stopped: mr generation exhausted\n",
			 att->index);
		attestation_stop(att);
		return -EOVERFLOW;
	}
	ret = att->provider->mr_extend(att, mr->props->id, values);
	if (ret)
		return ret;
	attestation_mr_changed(att);
	return 0;
}

int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr)
{
	u64 caps = 0;
	size_t index;
	bool ok = true;

	ok &= ctlv_attestation_mr_get_items_id_put(reply, mr->props->id);
	ok &= ctlv_attestation_mr_desc_name_put(reply, mr->props->name);
	for (index = 0; index < mr->props->algorithms_count; index++)
		ok &= ctlv_attestation_mr_desc_supported_hash_algorithms_put(
			reply, mr->props->algorithms[index]);
	if (mr->props->has_register_index)
		ok &= ctlv_attestation_mr_desc_register_index_put(reply, mr->props->register_index);
	if (mr->props->extendable)
		caps |= CTLV_ATTESTATION_MR_CAPS_EXTEND;
	ok &= ctlv_attestation_mr_desc_caps_put(reply, caps);
	return ok ? 0 : -EMSGSIZE;
}
