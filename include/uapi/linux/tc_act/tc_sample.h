#ifndef __LINUX_TC_SAMPLE_H
#define __LINUX_TC_SAMPLE_H

#include <linux/types.h>
#include <linux/pkt_cls.h>
#include <linux/if_ether.h>

#define TCA_ACT_SAMPLE 26

struct tc_sample {
	tc_gen;
	__u32		rate;		/* sample rate */
	__u32		mark;		/* mark to put on the sampled packets */
	bool		truncate;	/* whether to truncate the packets */
	__u32		trunc_size;	/* truncation size */
	__u8		eth_dst[ETH_ALEN]; /* encapsulated mac destination */
	__u8		eth_src[ETH_ALEN]; /* encapsulated mac source */
	bool		eth_type_set;	   /* whether to overrid ethtype */
	__u16		eth_type;	   /* encapsulated mac ethtype */
};

enum {
	TCA_SAMPLE_UNSPEC,
	TCA_SAMPLE_TM,
	TCA_SAMPLE_PARMS,
	TCA_SAMPLE_PAD,
	__TCA_SAMPLE_MAX
};
#define TCA_SAMPLE_MAX (__TCA_SAMPLE_MAX - 1)

#endif
