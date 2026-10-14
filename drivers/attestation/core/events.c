// SPDX-License-Identifier: GPL-2.0-only
#include <linux/err.h>

#include "internal.h"

int attestation_events_prepare(struct attestation *att, struct attestation_event_batch *events)
{
	struct attestation_event *event = &events->mr_log;

	lockdep_assert_held(&att->lock);
	event->ctx = ctlv_attestation_mr_log_entry_appended_create(att->ctlvdev, &event->reply,
								   GFP_KERNEL);
	if (IS_ERR(event->ctx)) {
		int ret = PTR_ERR(event->ctx);

		event->ctx = NULL;
		event->reply = NULL;
		return ret;
	}
	return 0;
}

void attestation_events_abort(struct attestation_event_batch *events)
{
	struct attestation_event *event = &events->mr_log;

	if (!event->ctx)
		return;
	ctlv_event_abort(event->ctx);
	event->ctx = NULL;
	event->reply = NULL;
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
