#ifndef __NET_TC_SAMPLE_H
#define __NET_TC_SAMPLE_H

#include <net/act_api.h>
#include <linux/tc_act/tc_sample.h>

struct tcf_sample {
	struct tc_action	common;
	u32			rate;
	u32			mark;
	bool			truncate;
	u32			trunc_size;
	u32			packet_counter;
	u8			eth_dst[ETH_ALEN];
	u8			eth_src[ETH_ALEN];
	u16			eth_type;
	bool			eth_type_set;
	struct list_head	tcfm_list;
};
#define to_sample(a) ((struct tcf_sample *)a)

struct sample_packet_metadata {
	int sample_size;
	int orig_size;
	int ifindex;
};

struct ethhdr *sample_packet_pack(struct sk_buff *skb,
				  struct sample_packet_metadata *metadata);

static inline bool is_tcf_sample(const struct tc_action *a)
{
#ifdef CONFIG_NET_CLS_ACT
	return a->ops && a->ops->type == TCA_ACT_SAMPLE;
#else
	return false;
#endif
}

static inline __u32 tcf_sample_mark(const struct tc_action *a)
{
	return to_sample(a)->mark;
}

static inline __u32 tcf_sample_rate(const struct tc_action *a)
{
	return to_sample(a)->rate;
}

static inline bool tcf_sample_truncate(const struct tc_action *a)
{
	return to_sample(a)->truncate;
}

static inline int tcf_sample_trunc_size(const struct tc_action *a)
{
	return to_sample(a)->trunc_size;
}

static inline u16 tcf_sample_eth_type(const struct tc_action *a)
{
	return to_sample(a)->eth_type;
}

static inline bool tcf_sample_eth_type_set(const struct tc_action *a)
{
	return to_sample(a)->eth_type_set;
}

static inline void tcf_sample_eth_dst_addr(const struct tc_action *a, u8 *dst)
{
	ether_addr_copy(dst, to_sample(a)->eth_dst);
}

static inline void tcf_sample_eth_src_addr(const struct tc_action *a, u8 *src)
{
	ether_addr_copy(src, to_sample(a)->eth_src);
}

#endif /* __NET_TC_SAMPLE_H */
