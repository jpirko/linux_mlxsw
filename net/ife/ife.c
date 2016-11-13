/*
 * net/ife/ife.c - Inter-FE protocol based on ForCES WG InterFE LFB
 * Copyright (c) 2015 Jamal Hadi Salim <jhs@mojatatu.com>
 * Copyright (c) 2016 Yotam Gigi <yotamg@mellanox.com>
 *
 * Refer to: draft-ietf-forces-interfelfb-03 and netdev01 paper:
 * "Distributing Linux Traffic Control Classifier-Action Subsystem"
 * Authors: Jamal Hadi Salim and Damascene M. Joachimpillai
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/skbuff.h>
#include <linux/rtnetlink.h>
#include <linux/module.h>
#include <linux/init.h>
#include <net/net_namespace.h>
#include <net/netlink.h>
#include <net/pkt_sched.h>
#include <linux/etherdevice.h>
#include <net/ife.h>

struct ifeheadr {
	__be16 metalen;
	u8 tlv_data[];
};

void *ife_encode(struct sk_buff *skb, u16 metalen)
{
	/* OUTERHDR:TOTMETALEN:{TLVHDR:Metadatum:TLVHDR..}:ORIGDATA
	 * where ORIGDATA = original ethernet header ...
	 */
	int hdrm = metalen + IFE_METAHDRLEN;
	int total_push = hdrm + skb->dev->hard_header_len;
	struct ifeheadr *ifehdr;
	struct ethhdr *iethh;	/* inner ether header */
	int skboff = 0;
	int err;

	err = skb_cow_head(skb, total_push);
	if (unlikely(err))
		return NULL;

	iethh = (struct ethhdr *) skb->data;

	__skb_push(skb, total_push);
	memcpy(skb->data, iethh, skb->dev->hard_header_len);
	skb_reset_mac_header(skb);
	skboff += skb->dev->hard_header_len;

	/* total metadata length */
	ifehdr = (struct ifeheadr *) (skb->data + skboff);
	metalen += IFE_METAHDRLEN;
	ifehdr->metalen = htons(metalen);

	return ifehdr->tlv_data;
}
EXPORT_SYMBOL_GPL(ife_encode);

void *ife_decode(struct sk_buff *skb, u16 *metalen)
{
	struct ifeheadr *ifehdr;
	int total_pull;
	u16 ifehdrln;

	ifehdr = (struct ifeheadr *) (skb->data + skb->dev->hard_header_len);
	ifehdrln = ifehdr->metalen;
	ifehdrln = ntohs(ifehdrln);
	total_pull = skb->dev->hard_header_len + ifehdrln;

	if (unlikely(ifehdrln < 2))
		return NULL;

	if (unlikely(!pskb_may_pull(skb, total_pull)))
		return NULL;

	skb_set_mac_header(skb, total_pull);
	__skb_pull(skb, total_pull);
	*metalen = ifehdrln - IFE_METAHDRLEN;

	return &ifehdr->tlv_data;
}
EXPORT_SYMBOL_GPL(ife_decode);

struct meta_tlvhdr {
	__be16 type;
	__be16 len;
};

/* Caller takes care of presenting data in network order
 */
void *ife_tlv_meta_decode(void *skbdata, u16 *attrtype, u16 *dlen, u16 *totlen)
{
	struct meta_tlvhdr *tlv = (struct meta_tlvhdr *) skbdata;

	*dlen = ntohs(tlv->len) - NLA_HDRLEN;
	*attrtype = ntohs(tlv->type);

	if (totlen)
		*totlen = nla_total_size(*dlen);

	return skbdata + sizeof(struct meta_tlvhdr);
}
EXPORT_SYMBOL_GPL(ife_tlv_meta_decode);

void *ife_tlv_meta_next(void *skbdata)
{
	struct meta_tlvhdr *tlv = (struct meta_tlvhdr *) skbdata;
	u16 tlvlen = tlv->len;

	tlvlen = ntohs(tlvlen);
	tlvlen = NLA_ALIGN(tlvlen);

	return skbdata + tlvlen;
}
EXPORT_SYMBOL_GPL(ife_tlv_meta_next);

/* Caller takes care of presenting data in network order
 */
int ife_tlv_meta_encode(void *skbdata, u16 attrtype, u16 dlen, const void *dval)
{
	__be32 *tlv = (u32 *) (skbdata);
	u16 totlen = nla_total_size(dlen);	/*alignment + hdr */
	char *dptr = (char *) tlv + NLA_HDRLEN;
	u32 htlv = attrtype << 16 | (dlen + NLA_HDRLEN);

	*tlv = htonl(htlv);
	memset(dptr, 0, totlen - NLA_HDRLEN);
	memcpy(dptr, dval, dlen);

	return totlen;
}
EXPORT_SYMBOL_GPL(ife_tlv_meta_encode);

struct ethhdr *ife_packet_info_pack(struct sk_buff *skb, int orig_size,
				    int in_ifindex, int out_ifindex,
				    u32 sampler_id, u32 seq)
{
	int curr_size;
	void *ifetlv;
	u16 metalen;

	curr_size = skb->len;

	metalen = nla_total_size(sizeof(orig_size)) +
		  nla_total_size(sizeof(curr_size)) +
		  nla_total_size(sizeof(sampler_id)) +
		  nla_total_size(sizeof(seq));

	if (in_ifindex)
		metalen += nla_total_size(sizeof(in_ifindex));
	if (out_ifindex)
		metalen += nla_total_size(sizeof(out_ifindex));

	in_ifindex = htonl(in_ifindex);
	out_ifindex = htonl(out_ifindex);
	orig_size = htonl(orig_size);
	curr_size = htonl(curr_size);
	sampler_id = htonl(sampler_id);
	seq = htonl(seq);

	ifetlv = ife_encode(skb, metalen);
	if (!ifetlv)
		return NULL;

	if (in_ifindex)
		ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_IN_IFINDEX,
					      sizeof(in_ifindex), &in_ifindex);

	if (out_ifindex)
		ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_OUT_IFINDEX,
					      sizeof(out_ifindex),
					      &out_ifindex);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_ORIGSIZE,
				      sizeof(orig_size), &orig_size);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_SIZE,
				      sizeof(curr_size), &curr_size);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_SAMPLER_ID,
				      sizeof(sampler_id), &sampler_id);

	ifetlv += ife_tlv_meta_encode(ifetlv, IFE_META_SEQ, sizeof(seq), &seq);

	return (struct ethhdr *) skb->data;
}
EXPORT_SYMBOL(ife_packet_info_pack);

MODULE_AUTHOR("Jamal Hadi Salim <jhs@mojatatu.com>");
MODULE_AUTHOR("Yotam Gigi <yotamg@mellanox.com>");
MODULE_DESCRIPTION("Inter-FE LFB action");
MODULE_LICENSE("GPL");
