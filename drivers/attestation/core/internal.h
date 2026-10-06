/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ATTESTATION_INTERNAL_H
#define _ATTESTATION_INTERNAL_H

#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/srcu.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_gen.h"

#define ATTESTATION_REGISTERED XA_MARK_1

enum attestation_mr_log_entry_type {
	ATTESTATION_MR_LOG_ENTRY_INFO = 0,
	ATTESTATION_MR_LOG_ENTRY_EXTEND = 1,
	ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND = 2,
};

struct attestation_mr_log_entry {
	struct attestation_mr_log_entry __rcu *next;
	unsigned int mr_generation;
	unsigned int mr_id;
	enum attestation_mr_log_entry_type type;
};

struct attestation_mr_log_entry_info {
	struct attestation_mr_log_entry entry;
	struct attestation_mr_props props;
};

struct attestation_mr_log_entry_extend {
	struct attestation_mr_log_entry entry;
	struct attestation_digest_set values;
};

struct attestation_mr_log_entry_ima_extend {
	struct attestation_mr_log_entry_extend extend;
	char *template_name;
	u8 *template_data;
	size_t template_data_length;
	bool violation;
};

struct attestation_mr_log {
	unsigned int generation;
	struct xarray entries;
	struct attestation_mr_log_entry *tail;
};

struct attestation_mr {
	const struct attestation_mr_props *props;
	unsigned int start_generation;
	unsigned int state_flags;
};

struct attestation_mr_target {
	unsigned int index;
	unsigned int mr_id;
	unsigned int start_generation;
	bool ima_owner;
};

struct attestation_event {
	struct ctlv_op_ctx *ctx;
	struct ctlv_cursor *payload;
};

struct attestation_event_batch {
	struct attestation_event mr_log;
};

struct attestation_mr_change {
	struct attestation_mr *mr;
	struct attestation_mr_log_entry_extend *entry;
	struct attestation_event_batch events;
};

extern struct xarray attestation_instances;
extern struct srcu_struct attestation_instances_srcu;

bool attestation_registered(struct attestation_dev *att);
struct attestation_dev *attestation_instance_lock(unsigned int index);
void attestation_instance_unlock(struct attestation_dev *att);
void attestation_stop(struct attestation_dev *att);
void attestation_mr_changed(struct attestation_dev *att);

int attestation_provider_validate(const struct attestation_provider *provider,
				  const struct ctlv_family *family);
void attestation_provide_ops(struct attestation_dev *att);

struct attestation_mr *attestation_mr_lookup_live(struct attestation_dev *att, unsigned int mr_id);
int attestation_mr_values_validate(const struct attestation_mr_props *props,
				   const struct attestation_digest_set *values, bool all_banks);
int attestation_mr_log_init(struct attestation_dev *att);
struct attestation_mr_log_entry_info *attestation_mr_info_alloc(const struct attestation_mr *mr);
void attestation_mr_log_entry_free(struct attestation_mr_log_entry *entry);
void attestation_mr_log_free(struct attestation_mr_log *log);
void attestation_mr_log_append(struct attestation_dev *att, struct attestation_mr_log_entry *entry);
struct attestation_mr_change *
attestation_mr_change_prepare(struct attestation_dev *att, struct attestation_mr *mr,
			      const struct attestation_digest_set *values);
struct attestation_mr_change *
attestation_mr_ima_change_prepare(struct attestation_dev *att, struct attestation_mr *mr,
				  const struct attestation_digest_set *values,
				  const char *template_name, const u8 *template_data,
				  size_t template_data_length, bool violation);
const struct attestation_mr_log_entry *
attestation_mr_extend_locked(struct attestation_dev *att, struct attestation_mr_change *change);
void attestation_mr_change_free(struct attestation_mr_change *change);

int attestation_events_prepare(struct attestation_dev *att, struct attestation_event_batch *events);
void attestation_events_abort(struct attestation_event_batch *events);
void attestation_mr_log_event_emit(struct attestation_event_batch *events,
				   const struct attestation_mr_log_entry *entry);

int attestation_mr_log_entry_encode(struct ctlv_cursor *reply,
				    const struct attestation_mr_log_entry *entry);
int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr);

#endif /* _ATTESTATION_INTERNAL_H */
