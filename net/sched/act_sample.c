/*
 * net/sched/act_sample.c	packet sampling tc action
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Yotam Gigi <yotamg@mellanox.com> (2016)
 *
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gfp.h>
#include <net/net_namespace.h>
#include <net/netlink.h>
#include <net/pkt_sched.h>
#include <linux/tc_act/tc_sample.h>
#include <net/tc_act/tc_sample.h>
#include <net/ife.h>

#include <linux/if_arp.h>

#define SAMPLE_TAB_MASK     7
static int sample_net_id;
static struct tc_action_ops act_sample_ops;

static const struct nla_policy sample_policy[TCA_SAMPLE_MAX + 1] = {
	[TCA_SAMPLE_PARMS]	= { .len = sizeof(struct tc_sample) },
};

static int tcf_sample_init(struct net *net, struct nlattr *nla,
			   struct nlattr *est, struct tc_action **a, int ovr,
			   int bind)
{
	struct tc_action_net *tn = net_generic(net, sample_net_id);
	struct nlattr *tb[TCA_SAMPLE_MAX + 1];
	struct tc_sample *parm;
	struct tcf_sample *s;
	int ret;
	bool exists = false;

	if (!nla)
		return -EINVAL;
	ret = nla_parse_nested(tb, TCA_SAMPLE_MAX, nla, sample_policy);
	if (ret < 0)
		return ret;
	if (!tb[TCA_SAMPLE_PARMS])
		return -EINVAL;
	parm = nla_data(tb[TCA_SAMPLE_PARMS]);

	exists = tcf_hash_check(tn, parm->index, a, bind);
	if (exists && bind)
		return 0;

	if (!exists) {
		ret = tcf_hash_create(tn, parm->index, est, a,
				      &act_sample_ops, bind, false);
		if (ret)
			return ret;
		ret = ACT_P_CREATED;
	} else {
		tcf_hash_release(*a, bind);
		if (!ovr)
			return -EEXIST;
	}
	s = to_sample(*a);

	ASSERT_RTNL();
	s->tcf_action = parm->action;
	s->rate = parm->rate;
	s->mark = parm->mark;
	s->truncate = parm->truncate;
	s->trunc_size = parm->trunc_size;
	s->eth_type = parm->eth_type;
	s->eth_type_set = parm->eth_type_set;
	s->packet_counter = 0;

	if (parm->eth_dst)
		ether_addr_copy(s->eth_dst, parm->eth_dst);
	else
		eth_zero_addr(s->eth_dst);
	if (parm->eth_src)
		ether_addr_copy(s->eth_src, parm->eth_src);
	else
		eth_zero_addr(s->eth_src);

	if (ret == ACT_P_CREATED)
		tcf_hash_insert(tn, *a);
	return ret;
}

static bool dev_ok_push(struct net_device *dev)
{
	switch (dev->type) {
	case ARPHRD_TUNNEL:
	case ARPHRD_TUNNEL6:
	case ARPHRD_SIT:
	case ARPHRD_IPGRE:
	case ARPHRD_VOID:
	case ARPHRD_NONE:
		return false;
	default:
		return true;
	}
}

struct ethhdr *sample_packet_pack(struct sk_buff *skb,
				  struct sample_packet_metadata *metadata)
{
	int sample_size;
	int orig_size;
	void *ifetlv;
	int ifindex;
	u16 metalen;

	metalen = nla_total_size(sizeof(metadata->ifindex)) +
		  nla_total_size(sizeof(metadata->orig_size)) +
		  nla_total_size(sizeof(metadata->sample_size));

	ifindex = htonl(metadata->ifindex);
	orig_size = htonl(metadata->orig_size);
	sample_size = htonl(metadata->sample_size);

	ifetlv = ife_encode(skb, metalen);
	if (!ifetlv)
		return NULL;

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_IFINDEX,
				      sizeof(ifindex), &ifindex);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_ORIGSIZE,
				      sizeof(orig_size), &orig_size);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_SIZE,
				      sizeof(sample_size), &sample_size);

	return (struct ethhdr *) skb->data;
}
EXPORT_SYMBOL(sample_packet_pack);

static int tcf_sample(struct sk_buff *skb, const struct tc_action *a,
		      struct tcf_result *res)
{
	struct tcf_sample *s = to_sample(a);
	struct sample_packet_metadata metadata;
	static struct ethhdr *ethhdr;
	struct sk_buff *skb2;
	int retval;
	u32 at;

	tcf_lastuse_update(&s->tcf_tm);
	bstats_cpu_update(this_cpu_ptr(s->common.cpu_bstats), skb);

	rcu_read_lock();
	retval = READ_ONCE(s->tcf_action);

	if (++s->packet_counter % s->rate == 0) {
		skb2 = skb_copy(skb, GFP_ATOMIC);
		if (!skb2)
			goto out;

		if (s->truncate)
			skb_trim(skb2, s->trunc_size);

		at = G_TC_AT(skb->tc_verd);
		skb2->mac_len = skb->mac_len;

		/* on ingress, the mac header gets poped, so push it back */
		if (!(at & AT_EGRESS) && dev_ok_push(skb->dev))
			skb_push(skb2, skb2->mac_len);

		metadata.ifindex = skb->dev->ifindex;
		metadata.orig_size = skb->len + skb->dev->hard_header_len;
		metadata.sample_size = skb2->len;
		ethhdr = sample_packet_pack(skb2, (void *)&metadata);
		if (!ethhdr)
			goto out;

		if (!is_zero_ether_addr(s->eth_src))
			ether_addr_copy(ethhdr->h_source, s->eth_src);
		if (!is_zero_ether_addr(s->eth_dst))
			ether_addr_copy(ethhdr->h_dest, s->eth_dst);
		if (s->eth_type_set)
			ethhdr->h_proto = htons(s->eth_type);

		skb2->mark = s->mark;
		netif_receive_skb(skb2);

		/* mirror is always swallowed */
		skb2->tc_verd = SET_TC_FROM(skb2->tc_verd, at);
	}
out:
	rcu_read_unlock();

	return retval;
}

static int tcf_sample_dump(struct sk_buff *skb, struct tc_action *a,
			   int bind, int ref)
{
	unsigned char *b = skb_tail_pointer(skb);
	struct tcf_sample *s = to_sample(a);
	struct tc_sample opt = {
		.index      = s->tcf_index,
		.action     = s->tcf_action,
		.refcnt     = s->tcf_refcnt - ref,
		.bindcnt    = s->tcf_bindcnt - bind,
		.rate       = s->rate,
		.mark       = s->mark,
		.trunc_size = s->trunc_size,
		.truncate   = s->truncate,
	};
	struct tcf_t t;

	if (nla_put(skb, TCA_SAMPLE_PARMS, sizeof(opt), &opt))
		goto nla_put_failure;

	tcf_tm_dump(&t, &s->tcf_tm);
	if (nla_put_64bit(skb, TCA_SAMPLE_TM, sizeof(t), &t, TCA_SAMPLE_PAD))
		goto nla_put_failure;
	return skb->len;

nla_put_failure:
	nlmsg_trim(skb, b);
	return -1;
}

static int tcf_sample_walker(struct net *net, struct sk_buff *skb,
			     struct netlink_callback *cb, int type,
			     const struct tc_action_ops *ops)
{
	struct tc_action_net *tn = net_generic(net, sample_net_id);

	return tcf_generic_walker(tn, skb, cb, type, ops);
}

static int tcf_sample_search(struct net *net, struct tc_action **a, u32 index)
{
	struct tc_action_net *tn = net_generic(net, sample_net_id);

	return tcf_hash_search(tn, a, index);
}

static struct tc_action_ops act_sample_ops = {
	.kind		=	"sample",
	.type		=	TCA_ACT_SAMPLE,
	.owner		=	THIS_MODULE,
	.act		=	tcf_sample,
	.dump		=	tcf_sample_dump,
	.init		=	tcf_sample_init,
	.walk		=	tcf_sample_walker,
	.lookup		=	tcf_sample_search,
	.size		=	sizeof(struct tcf_sample),
};

static __net_init int sample_init_net(struct net *net)
{
	struct tc_action_net *tn = net_generic(net, sample_net_id);

	return tc_action_net_init(tn, &act_sample_ops, SAMPLE_TAB_MASK);
}

static void __net_exit sample_exit_net(struct net *net)
{
	struct tc_action_net *tn = net_generic(net, sample_net_id);

	tc_action_net_exit(tn);
}

static struct pernet_operations sample_net_ops = {
	.init = sample_init_net,
	.exit = sample_exit_net,
	.id   = &sample_net_id,
	.size = sizeof(struct tc_action_net),
};

MODULE_AUTHOR("Yotam Gigi (2016)");
MODULE_DESCRIPTION("Packet sampling action");
MODULE_LICENSE("GPL");

static int __init sample_init_module(void)
{
	return tcf_register_action(&act_sample_ops, &sample_net_ops);
}

static void __exit sample_cleanup_module(void)
{
	tcf_unregister_action(&act_sample_ops, &sample_net_ops);
}

module_init(sample_init_module);
module_exit(sample_cleanup_module);
