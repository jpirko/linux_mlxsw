/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef __UAPI_TC_DPXFER_H
#define __UAPI_TC_DPXFER_H

#include <linux/types.h>
#include <linux/pkt_cls.h>

enum {
	TCA_DPXFER_UNSPEC,
	TCA_DPXFER_PAD,
	TCA_DPXFER_PARMS,
	__TCA_DPXFER_MAX
};

#define TCA_DPXFER_MAX (__TCA_DPXFER_MAX - 1)

struct tc_dpxfer {
	tc_gen;
};

#endif /* __UAPI_TC_DPXFER_H */
