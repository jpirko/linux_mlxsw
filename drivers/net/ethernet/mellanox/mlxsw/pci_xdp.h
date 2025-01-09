/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */
/* Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef _PCI_XDP_H
#define _PCI_XDP_H

enum mlxsw_xdp_status {
	MLXSW_XDP_STATUS_DROP,
	MLXSW_XDP_STATUS_FAIL,
};

int mlxsw_xdp_buff_init(struct xdp_buff *xdp_buff,
			const struct mlxsw_pci_rx_pkt_info *rx_pkt_info,
			struct xdp_rxq_info *xdp_rxq);

enum mlxsw_xdp_status
mlxsw_xdp_run(struct xdp_buff *xdp_buff, struct bpf_prog *prog,
	      struct net_device *netdev);

#endif
