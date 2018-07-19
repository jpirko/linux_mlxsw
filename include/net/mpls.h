/*
 * Copyright (c) 2014 Nicira, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 */

#ifndef _NET_MPLS_H
#define _NET_MPLS_H 1

#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <net/fib_notifier.h>

#define MPLS_HLEN 4

struct mpls_nh { /* next hop label forwarding entry */
	struct net_device __rcu *nh_dev;

	/* nh_flags is accessed under RCU in the packet path; it is
	 * modified handling netdev events with rtnl lock held
	 */
	unsigned int		nh_flags;
	u8			nh_labels;
	u8			nh_via_alen;
	u8			nh_via_table;
	u8			nh_reserved1;

	u32			nh_label[0];
};

/* The route, nexthops and vias are stored together in the same memory
 * block:
 *
 * +----------------------+
 * | mpls_route           |
 * +----------------------+
 * | mpls_nh 0            |
 * +----------------------+
 * | alignment padding    |   4 bytes for odd number of labels
 * +----------------------+
 * | via[rt_max_alen] 0   |
 * +----------------------+
 * | alignment padding    |   via's aligned on sizeof(unsigned long)
 * +----------------------+
 * | ...                  |
 * +----------------------+
 * | mpls_nh n-1          |
 * +----------------------+
 * | via[rt_max_alen] n-1 |
 * +----------------------+
 */
struct mpls_route { /* next hop label forwarding entry */
	struct rcu_head		rt_rcu;
	u8			rt_protocol;
	u8			rt_payload_type;
	u8			rt_max_alen;
	u8			rt_ttl_propagate;
	u8			rt_nhn;
	/* rt_nhn_alive is accessed under RCU in the packet path; it
	 * is modified handling netdev events with rtnl lock held
	 */
	u8			rt_nhn_alive;
	u8			rt_nh_size;
	u8			rt_via_offset;
	u8			rt_reserved1;
	struct mpls_nh		rt_nh[0];
};

#define for_nexthops(rt) {						\
	int nhsel; struct mpls_nh *nh;  u8 *__nh;			\
	for (nhsel = 0, nh = (rt)->rt_nh, __nh = (u8 *)((rt)->rt_nh);	\
	     nhsel < (rt)->rt_nhn;					\
	     __nh += rt->rt_nh_size, nh = (struct mpls_nh *)__nh, nhsel++)

#define change_nexthops(rt) {						\
	int nhsel; struct mpls_nh *nh; u8 *__nh;			\
	for (nhsel = 0, nh = (struct mpls_nh *)((rt)->rt_nh),		\
			__nh = (u8 *)((rt)->rt_nh);			\
	     nhsel < (rt)->rt_nhn;					\
	     __nh += rt->rt_nh_size, nh = (struct mpls_nh *)__nh, nhsel++)

#define endfor_nexthops(rt) }

struct mpls_route_entry_notifier_info {
	struct fib_notifier_info info;
	unsigned index;
	struct mpls_route *rt;
};

struct mpls_shim_hdr {
	__be32 label_stack_entry;
};

static inline bool eth_p_mpls(__be16 eth_type)
{
	return eth_type == htons(ETH_P_MPLS_UC) ||
		eth_type == htons(ETH_P_MPLS_MC);
}

static inline struct mpls_shim_hdr *mpls_hdr(const struct sk_buff *skb)
{
	return (struct mpls_shim_hdr *)skb_network_header(skb);
}

static inline u8 *__mpls_nh_via(struct mpls_route *rt, struct mpls_nh *nh)
{
	return (u8 *)nh + rt->rt_via_offset;
}

static inline const u8 *mpls_nh_via(const struct mpls_route *rt,
				    const struct mpls_nh *nh)
{
	return __mpls_nh_via((struct mpls_route *)rt, (struct mpls_nh *)nh);
}

#endif
