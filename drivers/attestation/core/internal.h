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
	ATTESTATION_MR_LOG_ENTRY_INFO,
	ATTESTATION_MR_LOG_ENTRY_EXTEND,
	ATTESTATION_MR_LOG_ENTRY_RECORD_EXTEND,
	ATTESTATION_MR_LOG_ENTRY_IMA_EXTEND,
};

struct attestation_evidence_range {
	unsigned int start_generation;
	unsigned int end_generation;
	bool mr_dependent;
	unsigned int mr_generation;
};

struct attestation_mr_log_entry {
	struct attestation_mr_log_entry __rcu *next;
	unsigned int mr_generation;
	unsigned int device_generation;
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

struct attestation_mr_log_entry_record_extend {
	struct attestation_mr_log_entry_extend extend;
	char *record_format;
	u8 *record_data;
	size_t record_data_length;
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
	bool frozen;
	struct xarray entries;
	struct attestation_mr_log_entry *tail;
};

struct attestation {
	struct ctlv_device *ctlvdev;
	const struct attestation_provider *provider;
	unsigned int index;

	/* Mutable instance, MR, verdict, and journal state. */
	struct mutex lock;
	unsigned int device_generation;
	unsigned int evidence_generation;
	unsigned int evidence_floor;
	struct xarray evidence_ranges;
	bool verdict_recorded;
	unsigned int accepted_device_generation;
	unsigned int accepted_evidence_generation;
	unsigned int mr_read_generation;
	unsigned int devsec_version;

	struct xarray mrs;
	struct attestation_mr_log *log;
	bool initialized;
	bool kexec_restored;
};

struct attestation_mr {
	struct attestation *att;
	const struct attestation_mr_props *props;
	unsigned int start_generation;
	unsigned int state_flags;
	bool removable;
};

struct attestation_mr_target {
	unsigned int index;
	unsigned int mr_id;
	unsigned int start_generation;
	bool ima_owner;
};

struct attestation_event {
	struct ctlv_op_ctx *ctx;
	struct ctlv_cursor *reply;
};

struct attestation_event_batch {
	struct attestation_event device;
	struct attestation_event evidence;
	struct attestation_event mr_log;
};

struct attestation_mr_change {
	struct attestation_mr *mr;
	struct attestation_mr_log_entry_extend *entry;
	struct attestation_event_batch events;
};

extern struct xarray attestation_instances;
extern struct srcu_struct attestation_instances_srcu;

int attestation_core_init(struct attestation *att, struct ctlv_device *ctlvdev,
			  const struct attestation_provider *provider);
void attestation_core_fini(struct attestation *att);
bool attestation_registered(struct attestation *att);
struct attestation *attestation_instance_lock(unsigned int index);
void attestation_instance_unlock(struct attestation *att);
void attestation_stop(struct attestation *att);
int attestation_preflight(struct attestation *att, bool device, bool evidence, bool mr);
void attestation_device_changed(struct attestation *att);
void attestation_mr_changed(struct attestation *att);
void attestation_evidence_verdict_changed(struct attestation *att);
void attestation_evidence_accept_clear_locked(struct attestation *att);

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
			      const struct attestation_digest_set *values,
			      const char *record_format, const u8 *record_data,
			      size_t record_data_length);
struct attestation_mr_change *
attestation_mr_ima_change_prepare(struct attestation *att, struct attestation_mr *mr,
				  const struct attestation_digest_set *values,
				  const char *template_name, const u8 *template_data,
				  size_t template_data_length, bool violation);
const struct attestation_mr_log_entry *
attestation_mr_extend_locked(struct attestation *att, struct attestation_mr_change *change,
			     const struct ctlv_cursor *mr_options);
void attestation_mr_change_free(struct attestation_mr_change *change);

int attestation_events_prepare(struct attestation *att, struct attestation_event_batch *events,
			       bool device, bool evidence, bool mr_log);
void attestation_events_abort(struct attestation_event_batch *events);
void attestation_device_event_emit(struct attestation_event_batch *events, unsigned int generation);
void attestation_evidence_event_emit(struct attestation_event_batch *events,
				     const struct attestation *att);
void attestation_mr_log_event_emit(struct attestation_event_batch *events,
				   const struct attestation_mr_log_entry *entry);

int attestation_mr_log_entry_encode(struct ctlv_cursor *reply,
				    const struct attestation_mr_log_entry *entry);
int attestation_mr_props_encode(struct ctlv_cursor *reply, const struct attestation_mr *mr,
				bool include_id);

int attestation_kexec_restore(struct attestation *att);
void attestation_kexec_invalidate(struct attestation *att);
int attestation_kexec_init(void);
void attestation_kexec_exit(void);

#endif /* _ATTESTATION_INTERNAL_H */
