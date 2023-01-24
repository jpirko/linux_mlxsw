#undef TRACE_SYSTEM
#define TRACE_SYSTEM bridge

#if !defined(_TRACE_BRIDGE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_BRIDGE_H

#include <linux/netdevice.h>
#include <linux/tracepoint.h>

#include "../../../net/bridge/br_private.h"

TRACE_EVENT(br_fdb_add,

	TP_PROTO(struct ndmsg *ndm, struct net_device *dev,
		 const unsigned char *addr, u16 vid, u16 nlh_flags),

	TP_ARGS(ndm, dev, addr, vid, nlh_flags),

	TP_STRUCT__entry(
		__field(u8, ndm_flags)
		__string(dev, dev->name)
		__array(unsigned char, addr, ETH_ALEN)
		__field(u16, vid)
		__field(u16, nlh_flags)
	),

	TP_fast_assign(
		__assign_str(dev, dev->name);
		memcpy(__entry->addr, addr, ETH_ALEN);
		__entry->vid = vid;
		__entry->nlh_flags = nlh_flags;
		__entry->ndm_flags = ndm->ndm_flags;
	),

	TP_printk("dev %s addr %02x:%02x:%02x:%02x:%02x:%02x vid %u nlh_flags %04x ndm_flags %02x",
		  __get_str(dev), __entry->addr[0], __entry->addr[1],
		  __entry->addr[2], __entry->addr[3], __entry->addr[4],
		  __entry->addr[5], __entry->vid,
		  __entry->nlh_flags, __entry->ndm_flags)
);

TRACE_EVENT(br_fdb_external_learn_add,

	TP_PROTO(struct net_bridge *br, struct net_bridge_port *p,
		 const unsigned char *addr, u16 vid),

	TP_ARGS(br, p, addr, vid),

	TP_STRUCT__entry(
		__string(br_dev, br->dev->name)
		__string(dev, p ? p->dev->name : "null")
		__array(unsigned char, addr, ETH_ALEN)
		__field(u16, vid)
	),

	TP_fast_assign(
		__assign_str(br_dev, br->dev->name);
		__assign_str(dev, p ? p->dev->name : "null");
		memcpy(__entry->addr, addr, ETH_ALEN);
		__entry->vid = vid;
	),

	TP_printk("br_dev %s port %s addr %02x:%02x:%02x:%02x:%02x:%02x vid %u",
		  __get_str(br_dev), __get_str(dev), __entry->addr[0],
		  __entry->addr[1], __entry->addr[2], __entry->addr[3],
		  __entry->addr[4], __entry->addr[5], __entry->vid)
);

TRACE_EVENT(fdb_delete,

	TP_PROTO(struct net_bridge *br, struct net_bridge_fdb_entry *f),

	TP_ARGS(br, f),

	TP_STRUCT__entry(
		__string(br_dev, br->dev->name)
		__string(dev, f->dst ? f->dst->dev->name : "null")
		__array(unsigned char, addr, ETH_ALEN)
		__field(u16, vid)
	),

	TP_fast_assign(
		__assign_str(br_dev, br->dev->name);
		__assign_str(dev, f->dst ? f->dst->dev->name : "null");
		memcpy(__entry->addr, f->key.addr.addr, ETH_ALEN);
		__entry->vid = f->key.vlan_id;
	),

	TP_printk("br_dev %s dev %s addr %02x:%02x:%02x:%02x:%02x:%02x vid %u",
		  __get_str(br_dev), __get_str(dev), __entry->addr[0],
		  __entry->addr[1], __entry->addr[2], __entry->addr[3],
		  __entry->addr[4], __entry->addr[5], __entry->vid)
);

TRACE_EVENT(br_fdb_update,

	TP_PROTO(struct net_bridge *br, struct net_bridge_port *source,
		 const unsigned char *addr, u16 vid, unsigned long flags),

	TP_ARGS(br, source, addr, vid, flags),

	TP_STRUCT__entry(
		__string(br_dev, br->dev->name)
		__string(dev, source->dev->name)
		__array(unsigned char, addr, ETH_ALEN)
		__field(u16, vid)
		__field(unsigned long, flags)
	),

	TP_fast_assign(
		__assign_str(br_dev, br->dev->name);
		__assign_str(dev, source->dev->name);
		memcpy(__entry->addr, addr, ETH_ALEN);
		__entry->vid = vid;
		__entry->flags = flags;
	),

	TP_printk("br_dev %s source %s addr %02x:%02x:%02x:%02x:%02x:%02x vid %u flags 0x%lx",
		  __get_str(br_dev), __get_str(dev), __entry->addr[0],
		  __entry->addr[1], __entry->addr[2], __entry->addr[3],
		  __entry->addr[4], __entry->addr[5], __entry->vid,
		  __entry->flags)
);

TRACE_EVENT(br_mdb_full,

	TP_PROTO(const struct net_device *dev,
		 const struct br_ip *group),

	TP_ARGS(dev, group),

	TP_STRUCT__entry(
		__string(dev, dev->name)
		__field(int, af)
		__field(u16, vid)
		__array(__u8, src4, 4)
		__array(__u8, src6, 16)
		__array(__u8, grp4, 4)
		__array(__u8, grp6, 16)
		__array(__u8, grpmac, ETH_ALEN) /* For af == 0. */
	),

	TP_fast_assign(
		__assign_str(dev, dev->name);
		__entry->vid = group->vid;

		if (!group->proto) {
			__entry->af = 0;

			memset(__entry->src4, 0, sizeof(__entry->src4));
			memset(__entry->src6, 0, sizeof(__entry->src6));
			memset(__entry->grp4, 0, sizeof(__entry->grp4));
			memset(__entry->grp6, 0, sizeof(__entry->grp6));
			memcpy(__entry->grpmac, group->dst.mac_addr, ETH_ALEN);
		} else if (group->proto == htons(ETH_P_IP)) {
			__be32 *p32;

			__entry->af = AF_INET;

			p32 = (__be32 *) __entry->src4;
			*p32 = group->src.ip4;

			p32 = (__be32 *) __entry->grp4;
			*p32 = group->dst.ip4;

			memset(__entry->src6, 0, sizeof(__entry->src6));
			memset(__entry->grp6, 0, sizeof(__entry->grp6));
			memset(__entry->grpmac, 0, ETH_ALEN);
#if IS_ENABLED(CONFIG_IPV6)
		} else {
			struct in6_addr *in6;

			__entry->af = AF_INET6;

			in6 = (struct in6_addr *)__entry->src6;
			*in6 = group->src.ip6;

			in6 = (struct in6_addr *)__entry->grp6;
			*in6 = group->dst.ip6;

			memset(__entry->src4, 0, sizeof(__entry->src4));
			memset(__entry->grp4, 0, sizeof(__entry->grp4));
			memset(__entry->grpmac, 0, ETH_ALEN);
#endif
		}
	),

	TP_printk("dev %s af %u src %pI4/%pI6c grp %pI4/%pI6c/%pM vid %u",
		  __get_str(dev), __entry->af, __entry->src4, __entry->src6,
		  __entry->grp4, __entry->grp6, __entry->grpmac, __entry->vid)
);

#endif /* _TRACE_BRIDGE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
