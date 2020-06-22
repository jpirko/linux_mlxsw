// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * net/sched/sch_fifo.c	The simplest FIFO queue.
 *
 * Authors:	Alexey Kuznetsov, <kuznet@ms2.inr.ac.ru>
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/pkt_cls.h>

/* 1 band FIFO pseudo-"scheduler" */

struct fifo_sched {
	bool new_style;
	struct tcf_qevent qe_tail_drop;
};

static DEFINE_STATIC_KEY_FALSE(fifo_qevents_used);

static int fifo_tail_drop(struct sk_buff *skb, struct Qdisc *sch, struct sk_buff **to_free)
{
	struct fifo_sched *q = qdisc_priv(sch);
	int ret;

	if (static_branch_unlikely(&fifo_qevents_used)) {
		skb = tcf_qevent_handle(&q->qe_tail_drop, sch, skb, to_free, &ret);
		if (!skb)
			return NET_XMIT_CN | ret;
	}

	return qdisc_drop(skb, sch, to_free);
}

static int bfifo_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			 struct sk_buff **to_free)
{
	if (likely(sch->qstats.backlog + qdisc_pkt_len(skb) <= sch->limit))
		return qdisc_enqueue_tail(skb, sch);

	return fifo_tail_drop(skb, sch, to_free);
}

static int pfifo_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			 struct sk_buff **to_free)
{
	if (likely(sch->q.qlen < sch->limit))
		return qdisc_enqueue_tail(skb, sch);

	return fifo_tail_drop(skb, sch, to_free);
}

static int pfifo_tail_enqueue(struct sk_buff *skb, struct Qdisc *sch,
			      struct sk_buff **to_free)
{
	unsigned int prev_backlog;

	if (likely(sch->q.qlen < sch->limit))
		return qdisc_enqueue_tail(skb, sch);

	prev_backlog = sch->qstats.backlog;
	/* queue full, remove one skb to fulfill the limit */
	__qdisc_queue_drop_head(sch, &sch->q, to_free);
	qdisc_qstats_drop(sch);
	qdisc_enqueue_tail(skb, sch);

	qdisc_tree_reduce_backlog(sch, 0, prev_backlog - sch->qstats.backlog);
	return NET_XMIT_CN;
}

static void fifo_offload_init(struct Qdisc *sch)
{
	struct net_device *dev = qdisc_dev(sch);
	struct tc_fifo_qopt_offload qopt;

	if (!tc_can_offload(dev) || !dev->netdev_ops->ndo_setup_tc)
		return;

	qopt.command = TC_FIFO_REPLACE;
	qopt.handle = sch->handle;
	qopt.parent = sch->parent;
	dev->netdev_ops->ndo_setup_tc(dev, TC_SETUP_QDISC_FIFO, &qopt);
}

static void fifo_offload_destroy(struct Qdisc *sch)
{
	struct net_device *dev = qdisc_dev(sch);
	struct tc_fifo_qopt_offload qopt;

	if (!tc_can_offload(dev) || !dev->netdev_ops->ndo_setup_tc)
		return;

	qopt.command = TC_FIFO_DESTROY;
	qopt.handle = sch->handle;
	qopt.parent = sch->parent;
	dev->netdev_ops->ndo_setup_tc(dev, TC_SETUP_QDISC_FIFO, &qopt);
}

static int fifo_offload_dump(struct Qdisc *sch)
{
	struct tc_fifo_qopt_offload qopt;

	qopt.command = TC_FIFO_STATS;
	qopt.handle = sch->handle;
	qopt.parent = sch->parent;
	qopt.stats.bstats = &sch->bstats;
	qopt.stats.qstats = &sch->qstats;

	return qdisc_offload_dump_helper(sch, TC_SETUP_QDISC_FIFO, &qopt);
}

static const struct nla_policy fifo_policy[TCA_FIFO_MAX + 1] = {
	[TCA_FIFO_LIMIT] = { .type = NLA_U32 },
	[TCA_FIFO_TAIL_DROP_BLOCK] = { .type = NLA_U32 },
};

static int fifo_parse_attr(struct Qdisc *sch, struct nlattr *opt, u32 *plimit, struct nlattr **tb,
			   struct netlink_ext_ack *extack)
{
	struct fifo_sched *q = qdisc_priv(sch);
	struct nlattr *nla;
	int err;

	/* Convert tc_fifo_qopt at the beginning of the payload to an NLA header. */
	BUILD_BUG_ON(sizeof(struct tc_fifo_qopt) != nla_attr_size(0));
	nla = nla_data(opt);
	nla->nla_len = opt->nla_len - sizeof(struct tc_fifo_qopt);
	nla->nla_type = opt->nla_type;

	err = nla_parse_nested(tb, TCA_FIFO_MAX, nla, fifo_policy, extack);
	if (err < 0)
		return err;

	if (!tb[TCA_FIFO_LIMIT]) {
		NL_SET_ERR_MSG_MOD(extack, "FIFO limit is a required attribute");
		return -EINVAL;
	}

	q->new_style = true;
	*plimit = nla_get_u32(tb[TCA_FIFO_LIMIT]);
	return 0;
}

static int fifo_parse_opt(struct Qdisc *sch, struct nlattr *opt, u32 *plimit, struct nlattr **tb,
			  struct netlink_ext_ack *extack)
{
	bool is_bfifo = sch->ops == &bfifo_qdisc_ops;

	if (opt == NULL) {
		u32 limit = qdisc_dev(sch)->tx_queue_len;

		if (is_bfifo)
			limit *= psched_mtu(qdisc_dev(sch));

		*plimit = limit;
	} else {
		struct tc_fifo_qopt *ctl = nla_data(opt);

		if (nla_len(opt) < sizeof(*ctl))
			return -EINVAL;

		if (!tb || nla_len(opt) == sizeof(*ctl) || ctl->limit)
			*plimit = ctl->limit;
		else
			return fifo_parse_attr(sch, opt, plimit, tb, extack);
	}

	return 0;
}

static int fifo_configure_limit(struct Qdisc *sch, u32 limit)
{
	bool bypass;
	bool is_bfifo = sch->ops == &bfifo_qdisc_ops;

	sch->limit = limit;

	if (is_bfifo)
		bypass = sch->limit >= psched_mtu(qdisc_dev(sch));
	else
		bypass = sch->limit >= 1;

	if (bypass)
		sch->flags |= TCQ_F_CAN_BYPASS;
	else
		sch->flags &= ~TCQ_F_CAN_BYPASS;

	return 0;
}

static int fifo_init(struct Qdisc *sch, struct nlattr *opt,
		     struct netlink_ext_ack *extack)
{
	struct nlattr *tb[TCA_FIFO_MAX + 1] = {};
	struct fifo_sched *q = qdisc_priv(sch);
	u32 limit;
	int err;

	err = fifo_parse_opt(sch, opt, &limit, tb, extack);
	if (err)
		return err;

	fifo_configure_limit(sch, limit);
	fifo_offload_init(sch);

	err = tcf_qevent_init(&q->qe_tail_drop, sch,
			      FLOW_BLOCK_BINDER_TYPE_FIFO_TAIL_DROP,
			      tb[TCA_FIFO_TAIL_DROP_BLOCK], extack);
	if (err)
		goto err_tail_init;

	if (q->qe_tail_drop.info.block_index)
		static_branch_inc(&fifo_qevents_used);

	return 0;

err_tail_init:
	fifo_offload_destroy(sch);
	return err;
}

static int fifo_hd_init(struct Qdisc *sch, struct nlattr *opt,
			struct netlink_ext_ack *extack)
{
	u32 limit;
	int err;

	err = fifo_parse_opt(sch, opt, &limit, NULL, extack);
	if (err)
		return err;

	fifo_configure_limit(sch, limit);
	return 0;
}

static void fifo_destroy(struct Qdisc *sch)
{
	struct fifo_sched *q = qdisc_priv(sch);

	if (q->qe_tail_drop.info.block_index)
		static_branch_dec(&fifo_qevents_used);
	tcf_qevent_destroy(&q->qe_tail_drop, sch);
	fifo_offload_destroy(sch);
}

static int fifo_change(struct Qdisc *sch, struct nlattr *opt,
		       struct netlink_ext_ack *extack)
{
	struct nlattr *tb[TCA_FIFO_MAX + 1] = {};
	struct fifo_sched *q = qdisc_priv(sch);
	u32 limit;
	int err;

	err = fifo_parse_opt(sch, opt, &limit, tb, extack);
	if (err)
		return err;

	err = tcf_qevent_validate_change(&q->qe_tail_drop, tb[TCA_FIFO_TAIL_DROP_BLOCK], extack);
	if (err)
		return err;

	fifo_configure_limit(sch, limit);
	fifo_offload_init(sch);
	return 0;
}

static int __fifo_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };

	if (nla_put(skb, TCA_OPTIONS, sizeof(opt), &opt))
		goto nla_put_failure;
	return skb->len;

nla_put_failure:
	return -1;
}

static int fifo_dump_new_style(struct Qdisc *sch, struct sk_buff *skb)
{
	struct tc_fifo_qopt opt = { .limit = sch->limit };
	struct fifo_sched *q = qdisc_priv(sch);
	struct nlattr *opts = NULL;

	opts = nla_nest_start_noflag(skb, TCA_OPTIONS);
	if (opts == NULL)
		return -ENOSPC;

	if (nla_put_nohdr(skb, sizeof(opt), &opt) ||
	    nla_put_u32(skb, TCA_FIFO_LIMIT, sch->limit) ||
	    tcf_qevent_dump(skb, TCA_FIFO_TAIL_DROP_BLOCK, &q->qe_tail_drop))
		goto nla_put_failure;

	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

static int fifo_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct fifo_sched *q = qdisc_priv(sch);
	int err;

	err = fifo_offload_dump(sch);
	if (err)
		return err;

	if (q->new_style)
		return fifo_dump_new_style(sch, skb);
	else
		return __fifo_dump(sch, skb);
}

static int fifo_hd_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	return __fifo_dump(sch, skb);
}

struct Qdisc_ops pfifo_qdisc_ops __read_mostly = {
	.id		=	"pfifo",
	.priv_size	=	sizeof(struct fifo_sched),
	.enqueue	=	pfifo_enqueue,
	.dequeue	=	qdisc_dequeue_head,
	.peek		=	qdisc_peek_head,
	.init		=	fifo_init,
	.destroy	=	fifo_destroy,
	.reset		=	qdisc_reset_queue,
	.change		=	fifo_change,
	.dump		=	fifo_dump,
	.owner		=	THIS_MODULE,
};
EXPORT_SYMBOL(pfifo_qdisc_ops);

struct Qdisc_ops bfifo_qdisc_ops __read_mostly = {
	.id		=	"bfifo",
	.priv_size	=	sizeof(struct fifo_sched),
	.enqueue	=	bfifo_enqueue,
	.dequeue	=	qdisc_dequeue_head,
	.peek		=	qdisc_peek_head,
	.init		=	fifo_init,
	.destroy	=	fifo_destroy,
	.reset		=	qdisc_reset_queue,
	.change		=	fifo_change,
	.dump		=	fifo_dump,
	.owner		=	THIS_MODULE,
};
EXPORT_SYMBOL(bfifo_qdisc_ops);

struct Qdisc_ops pfifo_head_drop_qdisc_ops __read_mostly = {
	.id		=	"pfifo_head_drop",
	.priv_size	=	0,
	.enqueue	=	pfifo_tail_enqueue,
	.dequeue	=	qdisc_dequeue_head,
	.peek		=	qdisc_peek_head,
	.init		=	fifo_hd_init,
	.reset		=	qdisc_reset_queue,
	.change		=	fifo_hd_init,
	.dump		=	fifo_hd_dump,
	.owner		=	THIS_MODULE,
};

/* Pass size change message down to embedded FIFO */
int fifo_set_limit(struct Qdisc *q, unsigned int limit)
{
	struct nlattr *nla;
	int ret = -ENOMEM;

	/* Hack to avoid sending change message to non-FIFO */
	if (strncmp(q->ops->id + 1, "fifo", 4) != 0)
		return 0;

	if (!q->ops->change)
		return 0;

	nla = kmalloc(nla_attr_size(sizeof(struct tc_fifo_qopt)), GFP_KERNEL);
	if (nla) {
		nla->nla_type = RTM_NEWQDISC;
		nla->nla_len = nla_attr_size(sizeof(struct tc_fifo_qopt));
		((struct tc_fifo_qopt *)nla_data(nla))->limit = limit;

		ret = q->ops->change(q, nla, NULL);
		kfree(nla);
	}
	return ret;
}
EXPORT_SYMBOL(fifo_set_limit);

struct Qdisc *fifo_create_dflt(struct Qdisc *sch, struct Qdisc_ops *ops,
			       unsigned int limit,
			       struct netlink_ext_ack *extack)
{
	struct Qdisc *q;
	int err = -ENOMEM;

	q = qdisc_create_dflt(sch->dev_queue, ops, TC_H_MAKE(sch->handle, 1),
			      extack);
	if (q) {
		err = fifo_set_limit(q, limit);
		if (err < 0) {
			qdisc_put(q);
			q = NULL;
		}
	}

	return q ? : ERR_PTR(err);
}
EXPORT_SYMBOL(fifo_create_dflt);
