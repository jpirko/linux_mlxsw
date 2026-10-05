// SPDX-License-Identifier: GPL-2.0-only
#include <linux/err.h>

#include "internal.h"

static int attestation_event_create(struct attestation *att, struct attestation_event *event,
				    u64 op_id)
{
	switch (op_id) {
	case CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED:
		event->ctx = ctlv_attestation_device_state_changed_create(att->ctlvdev,
									  &event->reply,
									  GFP_KERNEL);
		break;
	case CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED:
		event->ctx = ctlv_attestation_evidence_state_changed_create(att->ctlvdev,
									    &event->reply,
									    GFP_KERNEL);
		break;
	case CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED:
		event->ctx = ctlv_attestation_mr_log_entry_appended_create(att->ctlvdev,
									   &event->reply,
									   GFP_KERNEL);
		break;
	default:
		return -EINVAL;
	}
	if (IS_ERR(event->ctx)) {
		int ret = PTR_ERR(event->ctx);

		event->ctx = NULL;
		event->reply = NULL;
		return ret;
	}
	return 0;
}

int attestation_events_prepare(struct attestation *att, struct attestation_event_batch *events,
			       bool device, bool evidence, bool mr_log)
{
	int ret;

	lockdep_assert_held(&att->lock);
	if (device) {
		ret = attestation_event_create(att, &events->device,
					       CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED);
		if (ret)
			goto err;
	}
	if (evidence) {
		ret = attestation_event_create(att, &events->evidence,
					       CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);
		if (ret)
			goto err;
	}
	if (mr_log) {
		ret = attestation_event_create(att, &events->mr_log,
					       CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED);
		if (ret)
			goto err;
	}
	return 0;

err:
	attestation_events_abort(events);
	return ret;
}

void attestation_events_abort(struct attestation_event_batch *events)
{
	struct attestation_event *batch[] = {
		&events->device,
		&events->evidence,
		&events->mr_log,
	};
	size_t index;

	for (index = 0; index < ARRAY_SIZE(batch); index++) {
		struct attestation_event *event = batch[index];

		if (!event->ctx)
			continue;
		ctlv_event_abort(event->ctx);
		event->ctx = NULL;
		event->reply = NULL;
	}
}

static void attestation_event_finish(struct attestation_event *event, int ret)
{
	if (ret) {
		pr_err("attestation event encoding failed: %d\n", ret);
		ctlv_event_abort(event->ctx);
	} else {
		ret = ctlv_event_emit(event->ctx);
		if (ret)
			pr_err("attestation event emission failed: %d\n", ret);
	}
	event->ctx = NULL;
	event->reply = NULL;
}

void attestation_device_event_emit(struct attestation_event_batch *events, unsigned int generation)
{
	struct attestation_event *event = &events->device;

	if (!event->ctx)
		return;
	attestation_event_finish(event,
				 ctlv_attestation_device_generation_put(event->reply, generation) ?
					 0 :
					 -EMSGSIZE);
}

void attestation_evidence_event_emit(struct attestation_event_batch *events,
				     const struct attestation *att)
{
	struct attestation_event *event = &events->evidence;
	bool ok;

	lockdep_assert_held(&att->lock);
	if (!event->ctx)
		return;
	ok = ctlv_attestation_device_generation_put(event->reply, att->device_generation);
	if (att->evidence_generation > att->evidence_floor)
		ok &= ctlv_attestation_evidence_generation_put(event->reply,
							       att->evidence_generation);
	if (att->verdict_recorded)
		ok &= ctlv_attestation_evidence_accepted_generation_put(event->reply,
									att->accepted_evidence_generation);
	attestation_event_finish(event, ok ? 0 : -EMSGSIZE);
}

void attestation_mr_log_event_emit(struct attestation_event_batch *events,
				   const struct attestation_mr_log_entry *entry)
{
	struct attestation_event *event = &events->mr_log;
	struct ctlv_cursor nested;
	int ret;

	if (!event->ctx)
		return;
	if (!ctlv_attestation_notification_entry_start(event->reply, &nested)) {
		ret = -EMSGSIZE;
		goto out;
	}
	ret = attestation_mr_log_entry_encode(&nested, entry);
	ctlv_nest_end(&nested);
out:
	attestation_event_finish(event, ret);
}
