// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <linux/bpf_trace.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <net/xdp.h>

#include "core.h"
#include "pci.h"
#include "pci_xdp.h"

static int
mlxsw_xdp_frags_init(struct xdp_buff *xdp_buff,
		     const struct mlxsw_pci_rx_pkt_info *rx_pkt_info)
{
	struct skb_shared_info *skb_shared_info;
	int i;

	skb_shared_info = xdp_get_shared_info_from_buff(xdp_buff);
	skb_shared_info->nr_frags = 0;
	skb_shared_info->xdp_frags_size = 0;

	for (i = 1; i < rx_pkt_info->num_sg_entries; i++) {
		unsigned int frag_size;
		struct page *page;
		bool is_added;

		page = rx_pkt_info->pages[i];
		frag_size = rx_pkt_info->sg_entries_size[i];

		is_added = xdp_buff_add_frag(xdp_buff, page_to_netmem(page), 0,
					     frag_size, PAGE_SIZE);
		if (!is_added)
			return -ENOMEM;
	}

	return 0;
}

int mlxsw_xdp_buff_init(struct xdp_buff *xdp_buff,
			const struct mlxsw_pci_rx_pkt_info *rx_pkt_info,
			struct xdp_rxq_info *xdp_rxq)
{
	void *page_addr;

	page_addr = page_address(rx_pkt_info->pages[0]);

	xdp_init_buff(xdp_buff, PAGE_SIZE, xdp_rxq);
	xdp_prepare_buff(xdp_buff, page_addr, MLXSW_PCI_RX_BUF_HEADROOM,
			 rx_pkt_info->sg_entries_size[0], false);

	return mlxsw_xdp_frags_init(xdp_buff, rx_pkt_info);
}

static enum mlxsw_xdp_status
mlxsw_xdp_tx(struct mlxsw_pci *mlxsw_pci, struct xdp_buff *xdp_buff,
	     struct bpf_prog *prog, struct net_device *netdev, u16 local_port)
{
	const struct mlxsw_txhdr_info txhdr_info = {
		.tx_info.local_port = local_port,
		.tx_info.is_emad = false,
	};
	struct xdp_frame *xdpf;
	int err;

	xdpf = xdp_convert_buff_to_frame(xdp_buff);
	if (!xdpf)
		goto err_convert_buff_to_frame;

	if (unlikely(xdpf->len < ETH_ZLEN))
		goto err_xdpf_too_small;

	err = __mlxsw_pci_xdp_frame_transmit(mlxsw_pci, xdpf, &txhdr_info,
					     false);
	if (err)
		goto err_xdp_frame_transmit;

	return MLXSW_XDP_STATUS_TX;

err_xdp_frame_transmit:
err_xdpf_too_small:
err_convert_buff_to_frame:
	trace_xdp_exception(netdev, prog, XDP_TX);
	return MLXSW_XDP_STATUS_FAIL;
}

enum mlxsw_xdp_status
mlxsw_xdp_run(struct mlxsw_pci *mlxsw_pci, struct xdp_buff *xdp_buff,
	      struct bpf_prog *prog, struct net_device *netdev, u16 local_port)
{
	u32 act;

	act = bpf_prog_run_xdp(prog, xdp_buff);
	switch (act) {
	case XDP_ABORTED:
		break;
	case XDP_DROP:
		return MLXSW_XDP_STATUS_DROP;
	case XDP_PASS:
		return MLXSW_XDP_STATUS_PASS;
	case XDP_TX:
		return mlxsw_xdp_tx(mlxsw_pci, xdp_buff, prog, netdev,
				    local_port);
	default:
		bpf_warn_invalid_xdp_action(netdev, prog, act);
	}

	trace_xdp_exception(netdev, prog, act);
	return MLXSW_XDP_STATUS_FAIL;
}
