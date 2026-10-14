/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _ATTESTATION_INTERNAL_H
#define _ATTESTATION_INTERNAL_H

#include <linux/attestation.h>
#include <linux/ctlv.h>
#include <linux/mutex.h>
#include <linux/xarray.h>

#include "../generated/ctlv_attestation_gen.h"

#define ATTESTATION_REGISTERED XA_MARK_1

enum attestation_mr_log_entry_type {
	ATTESTATION_MR_LOG_ENTRY_INFO,
	ATTESTATION_MR_LOG_ENTRY_EXTEND,
};

struct attestation_mr_log_entry {
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

struct attestation_mr_log {
	unsigned int generation;
	struct xarray entries;
};

struct attestation {
	struct ctlv_device *ctlvdev;
	const struct attestation_provider *provider;
	unsigned int index;

	/* Mutable instance, MR, and journal state. */
	struct mutex lock;
	unsigned int mr_read_generation;

	struct xarray mrs;
	struct attestation_mr_log *log;
	bool initialized;
};

struct attestation_mr {
	const struct attestation_mr_props *props;
};

struct attestation_event {
	struct ctlv_op_ctx *ctx;
	struct ctlv_cursor *reply;
};

struct attestation_event_batch {
	struct attestation_event mr_log;
};

struct attestation_mr_change {
	struct attestation_mr *mr;
	struct attestation_mr_log_entry_extend *entry;
	struct attestation_event_batch events;
};

int attestation_core_init(struct attestation *att, struct ctlv_device *ctlvdev,
			  const struct attestation_provider *provider);
void attestation_core_fini(struct attestation *att);
bool attestation_registered(struct attestation *att);
void attestation_stop(struct attestation *att);
void attestation_mr_changed(struct attestation *att);

int attestation_provider_validate(const struct attestation_provider *provider);
void attestation_provide_ops(struct attestation *att);

struct attestation_mr *attestation_mr_lookup_live(struct attestation *att, unsigned int mr_id);
int attestation_mr_values_validate(const struct attestation_mr *mr,
				   const struct attestation_digest_set *values);
int attestation_mr_log_init(struct attestation *att);
struct attestation_mr_log_entry_info *attestation_mr_info_alloc(const struct attestation_mr *mr);
void attestation_mr_log_entry_free(struct attestation_mr_log_entry *entry);
void attestation_mr_log_free(struct attestation_mr_log *log);
void attestation_mr_log_append(struct attestation *att, struct attestation_mr_log_entry *entry);
struct attestation_mr_change *
attestation_mr_change_prepare(struct attestation *att, struct attestation_mr *mr,
			      const struct attestation_digest_set *values);
const struct attestation_mr_log_entry *
attestation_mr_extend_locked(struct attestation *att, struct attestation_mr_change *change);
void attestation_mr_change_free(struct attestation_mr_change *change);

int attestation_events_prepare(struct attestation *att, struct attestation_event_batch *events);
void attestation_events_abort(struct attestation_event_batch *events);
void attestation_mr_log_event_emit(struct attestation_event_batch *events,
				   const struct attestation_mr_log_entry *entry);

int attestation_mr_log_entry_encode(struct ctlv_cursor *reply,
				    const struct attestation_mr_log_entry *entry);
int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr,
				bool include_id);

#endif /* _ATTESTATION_INTERNAL_H */
