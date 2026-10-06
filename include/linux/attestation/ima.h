/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _LINUX_ATTESTATION_IMA_H
#define _LINUX_ATTESTATION_IMA_H

#include <linux/attestation.h>
#include <linux/err.h>
#include <linux/srcu.h>

struct attestation_mr_log_entry;
struct attestation_mr_target;

extern struct srcu_struct attestation_instances_srcu;

struct attestation_mr_target *
attestation_mr_target_get(const char *family_name, const char *bus_name,
			  const char *device_name, unsigned int mr_id);
void attestation_mr_target_put(struct attestation_mr_target *target);
int attestation_mr_target_check(const struct attestation_mr_target *target,
				const char *family_name, const char *bus_name,
				const char *device_name, unsigned int mr_id);
struct attestation_mr_target *
attestation_mr_target_clone(const struct attestation_mr_target *target);
unsigned int
attestation_mr_target_start_generation(const struct attestation_mr_target *target);
enum hash_algo *
attestation_mr_target_algorithms(const struct attestation_mr_target *target,
				 size_t *algorithms_count);
int attestation_mr_target_extend(const struct attestation_mr_target *target,
				 const struct attestation_digest_set *values,
				 const char *template_name,
				 const u8 *template_data,
				 size_t template_data_length, bool violation);
int attestation_mr_log_end_generation(const struct attestation_mr_target *target,
				      unsigned int *end_generation);
const struct attestation_mr_log_entry *
attestation_mr_log_iter_seek(const struct attestation_mr_target *target,
			     unsigned int generation,
			     unsigned int end_generation);
const struct attestation_mr_log_entry *
attestation_mr_log_iter_next(const struct attestation_mr_log_entry *entry,
			     unsigned int end_generation);

#define attestation_mr_log_for_each_entry_srcu(entry, target, generation, end) \
	for ((entry) = attestation_mr_log_iter_seek((target), (generation), \
						    (end)); \
	     !IS_ERR_OR_NULL(entry); \
	     (entry) = attestation_mr_log_iter_next((entry), (end)))

bool attestation_mr_log_entry_selected(const struct attestation_mr_target *target,
				       const struct attestation_mr_log_entry *entry);
enum _ctlv_attestation_mr_log_event
attestation_mr_log_entry_event(const struct attestation_mr_log_entry *entry);
unsigned int
attestation_mr_log_entry_generation(const struct attestation_mr_log_entry *entry);
const struct attestation_digest *
attestation_mr_log_entry_values(const struct attestation_mr_log_entry *entry,
				size_t *values_count);
const char *
attestation_mr_log_entry_template_name(const struct attestation_mr_log_entry *entry);
const u8 *
attestation_mr_log_entry_template_data(const struct attestation_mr_log_entry *entry,
				       size_t *length);
bool
attestation_mr_log_entry_violation(const struct attestation_mr_log_entry *entry);

#endif /* _LINUX_ATTESTATION_IMA_H */
