/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __NET_TC_DPXFER_H
#define __NET_TC_DPXFER_H

#include <net/act_api.h>
#include <linux/tc_act/tc_dpxfer.h>
#include <net/pdpxfer.h>

struct tcf_dpxfer {
	struct tc_action common;
};
#define to_dpxfer(a) ((struct tcf_dpxfer *)a)

static inline bool is_tcf_dpxfer(const struct tc_action *a)
{
#ifdef CONFIG_NET_CLS_ACT
	return a->ops && a->ops->id == TCA_ID_DPXFER;
#else
	return false;
#endif
}

#endif /* __NET_TC_DPXFER_H */
