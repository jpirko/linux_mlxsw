/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_TEST_H
#define _CTLV_TEST_H

#include <linux/types.h>

#define CTLV_TEST_MAX_DEVICES 8

struct ctlv_test_init_info {
	bool withhold_ops;
	bool dynamic_disabled;
};

void ctlv_test_transfers_drain_begin(void);
void ctlv_test_transfers_drain_end(void);

#endif /* _CTLV_TEST_H */
