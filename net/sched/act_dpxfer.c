// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * act_dpxfer.c -- A datapath transfer actions
 *
 * Datapath transfer actions serve primarily to transfer packets between
 * datapath realms, such as from an offloaded HW datapath to the
 * Linux-based SW datapath.
 */

static unsigned int dpxfer_net_id;

static int tcf_dpxfer_act(struct sk_buff *skb, const struct tc_action *a,
			  struct tcf_result *res)
{
	return TC_ACT_OK;
}

static int tcf_dpxfer_dump(struct sk_buff *skb, struct tc_action *a, int bind,
			   int ref)
{
	unsigned char *b = skb_tail_pointer(skb);
	struct tcf_dpxfer *s = to_dpxfer(a);
	struct tc_dpxfer opt = {
		.index      = s->tcf_index,
		.refcnt     = refcount_read(&s->tcf_refcnt) - ref,
		.bindcnt    = atomic_read(&s->tcf_bindcnt) - bind,
	};
	struct tcf_t t;

	spin_lock_bh(&s->tcf_lock);

	opt.action = s->tcf_action;
	if (nla_put(skb, TCA_DPXFER_PARMS, sizeof(opt), &opt))
		goto nla_put_failure;

	tcf_tm_dump(&t, &s->tcf_tm);
	if (nla_put_64bit(skb, TCA_DPXFER_TM, sizeof(t), &t, TCA_DPXFER_PAD))
		goto nla_put_failure;

	spin_unlock_bh(&s->tcf_lock);

	return skb->len;

nla_put_failure:
	spin_unlock_bh(&s->tcf_lock);
	nlmsg_trim(skb, b);
	return -1;
}

static int tcf_dpxfer_init(struct net *net, struct nlattr *nla,
			   struct nlattr *est, struct tc_action **a, int ovr,
			   int bind, bool rtnl_held, struct tcf_proto *tp,
			   u32 flags, struct netlink_ext_ack *extack)
{
	struct tc_action_net *tn = net_generic(net, gate_net_id);
	enum tk_offsets tk_offset = TK_OFFS_TAI;
	struct nlattr *tb[TCA_GATE_MAX + 1];
	struct tcf_chain *goto_ch = NULL;
	u64 cycletime = 0, basetime = 0;
	struct tcf_gate_params *p;
	s32 clockid = CLOCK_TAI;
	struct tcf_gate *gact;
	struct tc_gate *parm;
	int ret = 0, err;
	u32 gflags = 0;
	s32 prio = -1;
	ktime_t start;
	bool exists;
	u32 index;

	if (!nla)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_DPXFER_MAX, nla, dpxfer_policy, extack);
	if (err < 0)
		return err;

	if (!tb[TCA_DPXFER_PARMS])
		return -EINVAL;

	parm = nla_data(tb[TCA_GATE_PARMS]);
	index = parm->index;

	err = tcf_idr_check_alloc(tn, &index, a, bind);
	if (err < 0)
		return err;

	exists = err;
	if (exists && bind)
		return 0;

	if (!exists) {
		ret = tcf_idr_create(tn, index, est, a,
				     &act_gate_ops, bind, false, 0);
		if (ret) {
			tcf_idr_cleanup(tn, index);
			return ret;
		}

		ret = ACT_P_CREATED;
	} else if (!ovr) {
		tcf_idr_release(*a, bind);
		return -EEXIST;
	}

	if (tb[TCA_GATE_PRIORITY])
		prio = nla_get_s32(tb[TCA_GATE_PRIORITY]);

	if (tb[TCA_GATE_BASE_TIME])
		basetime = nla_get_u64(tb[TCA_GATE_BASE_TIME]);

	if (tb[TCA_GATE_FLAGS])
		gflags = nla_get_u32(tb[TCA_GATE_FLAGS]);

	gact = to_gate(*a);
	if (ret == ACT_P_CREATED)
		INIT_LIST_HEAD(&gact->param.entries);

	err = tcf_action_check_ctrlact(parm->action, tp, &goto_ch, extack);
	if (err < 0)
		goto release_idr;

	spin_lock_bh(&gact->tcf_lock);
	p = &gact->param;

	if (tb[TCA_GATE_CYCLE_TIME])
		cycletime = nla_get_u64(tb[TCA_GATE_CYCLE_TIME]);

	if (tb[TCA_GATE_ENTRY_LIST]) {
		err = parse_gate_list(tb[TCA_GATE_ENTRY_LIST], p, extack);
		if (err < 0)
			goto chain_put;
	}

	if (!cycletime) {
		struct tcfg_gate_entry *entry;
		ktime_t cycle = 0;

		list_for_each_entry(entry, &p->entries, list)
			cycle = ktime_add_ns(cycle, entry->interval);
		cycletime = cycle;
		if (!cycletime) {
			err = -EINVAL;
			goto chain_put;
		}
	}
	p->tcfg_cycletime = cycletime;

	if (tb[TCA_GATE_CYCLE_TIME_EXT])
		p->tcfg_cycletime_ext =
			nla_get_u64(tb[TCA_GATE_CYCLE_TIME_EXT]);

	gate_setup_timer(gact, basetime, tk_offset, clockid,
			 ret == ACT_P_CREATED);
	p->tcfg_priority = prio;
	p->tcfg_flags = gflags;
	gate_get_start_time(gact, &start);

	gact->current_close_time = start;
	gact->current_gate_status = GATE_ACT_GATE_OPEN | GATE_ACT_PENDING;

	gact->next_entry = list_first_entry(&p->entries,
					    struct tcfg_gate_entry, list);

	goto_ch = tcf_action_set_ctrlact(*a, parm->action, goto_ch);

	gate_start_timer(gact, start);

	spin_unlock_bh(&gact->tcf_lock);

	if (goto_ch)
		tcf_chain_put_by_act(goto_ch);

	return ret;

chain_put:
	spin_unlock_bh(&gact->tcf_lock);

	if (goto_ch)
		tcf_chain_put_by_act(goto_ch);
release_idr:
	/* action is not inserted in any list: it's safe to init hitimer
	 * without taking tcf_lock.
	 */
	if (ret == ACT_P_CREATED)
		gate_setup_timer(gact, gact->param.tcfg_basetime,
				 gact->tk_offset, gact->param.tcfg_clockid,
				 true);
	tcf_idr_release(*a, bind);
	return err;
}

static struct tc_action_ops act_dpxfer_ops = {
	.kind		=	"dpxfer",
	.id		=	TCA_ID_DPXFER,
	.owner		=	THIS_MODULE,
	.act		=	tcf_dpxfer_act,
	.stats_update	=	tcf_stats_update,
	.dump		=	tcf_dpxfer_dump,
	.init		=	tcf_dpxfer_init,
	.walk		=	tcf_dpxfer_walker,
	.lookup		=	tcf_dpxfer_search,
	.get_fill_size	=	tcf_dpxfer_get_fill_size,
	.size		=	sizeof(struct tcf_dpxfer),
	.get_dev	=	tcf_dpxfer_get_dev,
};

static __net_init int dpxfer_init_net(struct net *net)
{
	struct tc_action_net *tn = net_generic(net, dpxfer_net_id);

	return tc_action_net_init(net, tn, &act_dpxfer_ops);
}

static void __net_exit dpxfer_exit_net(struct list_head *net_list)
{
	tc_action_net_exit(net_list, dpxfer_net_id);
}

static struct pernet_operations dpxfer_net_ops = {
	.init = dpxfer_init_net,
	.exit_batch = dpxfer_exit_net,
	.id   = &dpxfer_net_id,
	.size = sizeof(struct tc_action_net),
};

MODULE_DESCRIPTION("Datapath transfer actions");
MODULE_LICENSE("GPL");

static int __init dpxfer_init_module(void)
{
	return tcf_register_action(&act_dpxfer_ops, &dpxfer_net_ops);
}

static void __exit dpxfer_cleanup_module(void)
{
	tcf_unregister_action(&act_dpxfer_ops, &dpxfer_net_ops);
}

module_init(dpxfer_init_module);
module_exit(dpxfer_cleanup_module);
