// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2019 Mellanox Technologies. All rights reserved */

#include <linux/kernel.h>
#include <net/devlink.h>
#include <uapi/linux/devlink.h>

#include "core.h"
#include "reg.h"
#include "spectrum.h"

#define MLXSW_SP_TRAP_METADATA						      \
	{								      \
		.port_type = DEVLINK_PORT_TYPE_ETH,			      \
		.in_port = 1,						      \
	}

/* All trap groups must be documented in
 * Documentation/networking/devlink-trap-mlxsw.rst
 */
enum {
	MLXSW_SP_TRAP_GROUP_ID_L2_DROPS,
};

#define MLXSW_SP_TRAP_GROUP_NAME_L2_DROPS	"l2_drops"

#define MLXSW_SP_TRAP_GROUP(_id)					      \
	DEVLINK_TRAP_GROUP(MLXSW_SP_TRAP_GROUP_ID_##_id,		      \
			   MLXSW_SP_TRAP_GROUP_NAME_##_id)

static void mlxsw_sp_rx_drop_listener(struct sk_buff *skb, u8 local_port,
				      void *priv);

#define MLXSW_SP_TRAP_DROP(_id, _group_id)				      \
	DEVLINK_TRAP_GENERIC(DROP, _id, MLXSW_SP_TRAP_GROUP(_group_id),	      \
			     MLXSW_SP_TRAP_METADATA)

#define MLXSW_SP_RXL_DISCARD(_id, _group_id)				      \
	MLXSW_RXL(mlxsw_sp_rx_drop_listener, DISCARD_##_id, DISCARD, false,   \
		  SP_##_group_id, DISCARD)

static struct devlink_trap mlxsw_sp_traps_arr[] = {
	MLXSW_SP_TRAP_DROP(INGRESS_SMAC_MC_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(INGRESS_VLAN_TAG_ALLOW_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(INGRESS_VLAN_FILTER_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(INGRESS_STP_FILTER_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(UC_EMPTY_TX_LIST_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(MC_EMPTY_TX_LIST_DROP, L2_DROPS),
	MLXSW_SP_TRAP_DROP(UC_LOOPBACK_FILTER_DROP, L2_DROPS),
};

/* Order must match mlxsw_sp_traps_arr */
static struct mlxsw_listener mlxsw_sp_listeners_arr[] = {
	MLXSW_SP_RXL_DISCARD(ING_PACKET_SMAC_MC, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(ING_SWITCH_VTAG_ALLOW, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(ING_SWITCH_VLAN, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(ING_SWITCH_STP, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(LOOKUP_SWITCH_UC, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(LOOKUP_SWITCH_MC_NULL, L2_DISCARDS),
	MLXSW_SP_RXL_DISCARD(LOOKUP_SWITCH_LB, L2_DISCARDS),
};

static int mlxsw_sp_rx_listener(struct mlxsw_sp *mlxsw_sp, struct sk_buff *skb,
				u8 local_port,
				struct mlxsw_sp_port *mlxsw_sp_port)
{
	struct mlxsw_sp_port_pcpu_stats *pcpu_stats;

	if (unlikely(!mlxsw_sp_port)) {
		dev_warn_ratelimited(mlxsw_sp->bus_info->dev, "Port %d: skb received for non-existent port\n",
				     local_port);
		kfree_skb(skb);
		return -EINVAL;
	}

	skb->dev = mlxsw_sp_port->dev;

	pcpu_stats = this_cpu_ptr(mlxsw_sp_port->pcpu_stats);
	u64_stats_update_begin(&pcpu_stats->syncp);
	pcpu_stats->rx_packets++;
	pcpu_stats->rx_bytes += skb->len;
	u64_stats_update_end(&pcpu_stats->syncp);

	return 0;
}

static void mlxsw_sp_rx_drop_listener(struct sk_buff *skb, u8 local_port,
				      void *priv)
{
	struct devlink_trap_item *trap_item = priv;
	struct devlink_port *in_devlink_port;
	struct mlxsw_sp_port *mlxsw_sp_port;
	struct mlxsw_sp *mlxsw_sp;
	struct devlink *devlink;

	mlxsw_sp = trap_item->priv;
	mlxsw_sp_port = mlxsw_sp->ports[local_port];

	if (mlxsw_sp_rx_listener(mlxsw_sp, skb, local_port, mlxsw_sp_port))
		return;

	devlink = priv_to_devlink(mlxsw_sp->core);
	in_devlink_port = mlxsw_core_port_devlink_port_get(mlxsw_sp->core,
							   local_port);
	devlink_trap_report(devlink, skb, trap_item, in_devlink_port);
	consume_skb(skb);
}

static struct mlxsw_listener *mlxsw_sp_trap_listener_lookup(u16 trap_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(mlxsw_sp_traps_arr); i++) {
		if (mlxsw_sp_traps_arr[i].id == trap_id)
			return &mlxsw_sp_listeners_arr[i];
	}

	return NULL;
}

static int mlxsw_sp_devlink_trap_init(struct devlink_trap_item *trap_item,
				      struct devlink *devlink)
{
	struct mlxsw_core *mlxsw_core = devlink_priv(devlink);
	struct mlxsw_sp *mlxsw_sp = mlxsw_core_driver_priv(mlxsw_core);
	struct mlxsw_listener *listener;

	trap_item->priv = mlxsw_sp;
	listener = mlxsw_sp_trap_listener_lookup(trap_item->trap->id);
	if (WARN_ON(!listener))
		return -ENOENT;

	switch (listener->action) {
	case MLXSW_REG_HPKT_ACTION_DISCARD:
		trap_item->action = DEVLINK_TRAP_ACTION_DROP;
		break;
	default:
		WARN_ON(1);
		return -EINVAL;
	}

	return mlxsw_core_trap_register(mlxsw_core, listener, trap_item);
}

static void mlxsw_sp_devlink_trap_fini(struct devlink_trap_item *trap_item,
				       struct devlink *devlink)
{
	struct mlxsw_core *mlxsw_core = devlink_priv(devlink);
	struct mlxsw_listener *listener;

	listener = mlxsw_sp_trap_listener_lookup(trap_item->trap->id);
	if (WARN_ON(!listener))
		return;

	mlxsw_core_trap_unregister(mlxsw_core, listener, trap_item);
}

static int
mlxsw_sp_devlink_trap_action_set(struct devlink_trap_item *trap_item,
				 struct devlink *devlink,
				 enum devlink_trap_action action)
{
	struct mlxsw_core *mlxsw_core = devlink_priv(devlink);
	enum mlxsw_reg_hpkt_action hw_action;
	struct mlxsw_listener *listener;

	listener = mlxsw_sp_trap_listener_lookup(trap_item->trap->id);
	if (WARN_ON(!listener))
		return -ENOENT;

	switch (action) {
	case DEVLINK_TRAP_ACTION_DROP:
		hw_action = MLXSW_REG_HPKT_ACTION_DISCARD;
		break;
	case DEVLINK_TRAP_ACTION_TRAP:
		/* Drop traps need to use TRAP_EXCEPTION, so that an error
		 * will be counted.
		 */
		if (trap_item->trap->type == DEVLINK_TRAP_TYPE_DROP)
			hw_action = MLXSW_REG_HPKT_ACTION_TRAP_EXCEPTION_TO_CPU;
		else
			hw_action = MLXSW_REG_HPKT_ACTION_TRAP_TO_CPU;
		break;
	default:
		return -EINVAL;
	}

	return mlxsw_core_trap_action_set(mlxsw_core, listener, hw_action);
}

static const struct devlink_trap_ops mlxsw_sp_devlink_trap_ops = {
	.init		= mlxsw_sp_devlink_trap_init,
	.fini		= mlxsw_sp_devlink_trap_fini,
	.action_set	= mlxsw_sp_devlink_trap_action_set,
};

#define MLXSW_SP_DISCARD_POLICER_ID	(MLXSW_REG_HTGT_TRAP_GROUP_MAX + 1)

static int
mlxsw_sp_trap_group_policer_init(struct mlxsw_sp *mlxsw_sp,
				 struct devlink_trap_group_item *group_item)
{
	u32 bandwidth = mlxsw_sp->bus_info->bandwidth;
	enum mlxsw_reg_qpcr_ir_units ir_units;
	char qpcr_pl[MLXSW_REG_QPCR_LEN];
	u16 policer_id;
	u8 burst_size;
	bool is_bytes;
	u32 rate;

	switch (group_item->group->id) {
	case MLXSW_SP_TRAP_GROUP_ID_L2_DROPS:
		policer_id = MLXSW_SP_DISCARD_POLICER_ID;
		ir_units = MLXSW_REG_QPCR_IR_UNITS_M;
		is_bytes = true;
		rate = DIV_ROUND_UP(bandwidth * 8, 10);	/* 80% */
		burst_size = 4;
		break;
	default:
		return -EINVAL;
	}

	mlxsw_reg_qpcr_pack(qpcr_pl, policer_id, ir_units, is_bytes, rate,
			    burst_size);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(qpcr), qpcr_pl);
}

static int
__mlxsw_sp_trap_group_init(struct mlxsw_sp *mlxsw_sp,
			   struct devlink_trap_group_item *group_item)
{
	char htgt_pl[MLXSW_REG_HTGT_LEN];
	u8 priority, tc, group_id;
	u16 policer_id;

	switch (group_item->group->id) {
	case MLXSW_SP_TRAP_GROUP_ID_L2_DROPS:
		group_id = MLXSW_REG_HTGT_TRAP_GROUP_SP_L2_DISCARDS;
		policer_id = MLXSW_SP_DISCARD_POLICER_ID;
		priority = 0;
		tc = 0;
		break;
	default:
		return -EINVAL;
	}

	mlxsw_reg_htgt_pack(htgt_pl, group_id, policer_id, priority, tc);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(htgt), htgt_pl);
}

static int
mlxsw_sp_devlink_trap_group_init(struct devlink_trap_group_item *group_item,
				 struct devlink *devlink)
{
	struct mlxsw_core *mlxsw_core = devlink_priv(devlink);
	struct mlxsw_sp *mlxsw_sp = mlxsw_core_driver_priv(mlxsw_core);
	int err;

	err = mlxsw_sp_trap_group_policer_init(mlxsw_sp, group_item);
	if (err)
		return err;

	err = __mlxsw_sp_trap_group_init(mlxsw_sp, group_item);
	if (err)
		return err;

	return 0;
}

static const struct devlink_trap_group_ops mlxsw_sp_devlink_trap_group_ops = {
	.init = mlxsw_sp_devlink_trap_group_init,
};

int mlxsw_sp_devlink_traps_init(struct mlxsw_sp *mlxsw_sp)
{
	struct devlink *devlink = priv_to_devlink(mlxsw_sp->core);

	return devlink_traps_register(devlink, mlxsw_sp_traps_arr,
				      ARRAY_SIZE(mlxsw_sp_traps_arr),
				      &mlxsw_sp_devlink_trap_ops,
				      &mlxsw_sp_devlink_trap_group_ops);
}

void mlxsw_sp_devlink_traps_fini(struct mlxsw_sp *mlxsw_sp)
{
	struct devlink *devlink = priv_to_devlink(mlxsw_sp->core);

	devlink_traps_unregister(devlink, mlxsw_sp_traps_arr,
				 ARRAY_SIZE(mlxsw_sp_traps_arr));
}
