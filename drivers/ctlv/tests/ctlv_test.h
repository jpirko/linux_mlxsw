/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_TEST_H
#define _CTLV_TEST_H

#include <linux/ctlv.h>
#include <linux/mutex.h>
#include <linux/types.h>

#define CTLV_TEST_MAX_DEVICES 8

struct ctlv_test_init_info {
	bool withhold_ops;
	bool dynamic_disabled;
};

/*
 * What the family keeps per device, in the struct of whichever subsystem
 * gives the device, and of an inheriting family's device.
 */
struct ctlv_test_device {
	struct ctlv ctlv;
	/* Held across the flip, which no core lock covers. */
	struct mutex toggle_lock;
	bool dynamic_disabled;
	u32 magic;
};

int ctlv_test_device_init(struct ctlv_test_device *test, struct device *dev,
			  const struct ctlv_family *family,
			  const struct ctlv_hooks *hooks,
			  const struct ctlv_test_init_info *info);
void ctlv_test_device_fini(struct ctlv_test_device *test);

void ctlv_test_transfers_drain_begin(void);
void ctlv_test_transfers_drain_end(void);

#endif /* _CTLV_TEST_H */
