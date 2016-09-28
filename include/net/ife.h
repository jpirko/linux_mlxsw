#ifndef __NET_IFE_H
#define __NET_IFE_H

#include <uapi/linux/ife.h>
#include <linux/etherdevice.h>
#include <linux/rtnetlink.h>
#include <linux/module.h>
#include <uapi/linux/ife.h>

void *ife_encode(struct sk_buff *skb, u16 metalen);
void *ife_decode(struct sk_buff *skb, u16 *metalen);

void *ife_tlv_meta_decode(void *skbdata, u16 *attrtype, u16 *dlen, u16 *totlen);
int ife_tlv_meta_encode(void *skbdata, u16 attrtype, u16 dlen,
			const void *dval);

void *ife_tlv_meta_next(void *skbdata);

#endif // __NET_IFE_H
