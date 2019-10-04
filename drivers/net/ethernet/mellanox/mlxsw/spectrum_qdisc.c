// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2017-2018 Mellanox Technologies. All rights reserved */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <net/pkt_cls.h>
#include <net/red.h>

#include "spectrum.h"
#include "reg.h"

#define MLXSW_SP_PRIO_BAND_TO_TCLASS(band) (IEEE_8021QAZ_MAX_TCS - band - 1)
#define MLXSW_SP_PRIO_CHILD_TO_TCLASS(child) \
	MLXSW_SP_PRIO_BAND_TO_TCLASS((child - 1))

enum mlxsw_sp_qdisc_type {
	MLXSW_SP_QDISC_NO_QDISC,
	MLXSW_SP_QDISC_RED,
	MLXSW_SP_QDISC_PRIO,
	MLXSW_SP_QDISC_DRR,
	MLXSW_SP_QDISC_CBS,
};

struct mlxsw_sp_qdisc_type_mapping {
	const char *id;
	enum mlxsw_sp_qdisc_type type;
};

static const struct mlxsw_sp_qdisc_type_mapping mlxsw_sp_qdisc_type_map[] = {
	{"noop", MLXSW_SP_QDISC_NO_QDISC},
	{"pfifo", MLXSW_SP_QDISC_NO_QDISC},
	{"bfifo", MLXSW_SP_QDISC_NO_QDISC},
	{"prio", MLXSW_SP_QDISC_PRIO},
	{"drr", MLXSW_SP_QDISC_DRR},
	{"red", MLXSW_SP_QDISC_RED},
	{"cbs", MLXSW_SP_QDISC_CBS},
};

static enum mlxsw_sp_qdisc_type mlxsw_sp_qdisc_type(struct Qdisc *sch)
{
	int i;

	if (sch)
		for (i = 0; i < ARRAY_SIZE(mlxsw_sp_qdisc_type_map); i++) {
			if (!strcmp(mlxsw_sp_qdisc_type_map[i].id,
				    sch->ops->id))
				return mlxsw_sp_qdisc_type_map[i].type;
		}

	return MLXSW_SP_QDISC_NO_QDISC;
}

struct mlxsw_sp_qdisc_ops {
	enum mlxsw_sp_qdisc_type type;
	int (*check_params)(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    void *params);
	int (*replace)(struct mlxsw_sp_port *mlxsw_sp_port,
		       struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, void *params);
	int (*destroy)(struct mlxsw_sp_port *mlxsw_sp_port,
		       struct mlxsw_sp_qdisc *mlxsw_sp_qdisc);
	int (*get_stats)(struct mlxsw_sp_port *mlxsw_sp_port,
			 struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			 struct tc_qopt_offload_stats *stats_ptr);
	int (*get_xstats)(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			  void *xstats_ptr);
	void (*clean_stats)(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc);
	/* unoffload - to be used for a qdisc that stops being offloaded without
	 * being destroyed.
	 */
	void (*unoffload)(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, void *params);
};

struct mlxsw_sp_qdisc {
	u32 handle;
	u8 tclass_num;
	u8 prio_bitmap;
	union {
		struct red_stats red;
	} xstats_base;
	struct mlxsw_sp_qdisc_stats {
		u64 tx_bytes;
		u64 tx_packets;
		u64 drops;
		u64 overlimits;
		u64 backlog;
	} stats_base;

	struct mlxsw_sp_qdisc_ops *ops;
};

static bool
mlxsw_sp_qdisc_compare(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, u32 handle,
		       enum mlxsw_sp_qdisc_type type)
{
	return mlxsw_sp_qdisc && mlxsw_sp_qdisc->ops &&
	       mlxsw_sp_qdisc->ops->type == type &&
	       mlxsw_sp_qdisc->handle == handle;
}

static struct mlxsw_sp_qdisc *
mlxsw_sp_qdisc_find(struct mlxsw_sp_port *mlxsw_sp_port, u32 parent,
		    bool root_only)
{
	int tclass, child_index;

	if (parent == TC_H_ROOT)
		return mlxsw_sp_port->root_qdisc;

	if (root_only || !mlxsw_sp_port->root_qdisc ||
	    !mlxsw_sp_port->root_qdisc->ops ||
	    TC_H_MAJ(parent) != mlxsw_sp_port->root_qdisc->handle ||
	    TC_H_MIN(parent) > IEEE_8021QAZ_MAX_TCS)
		return NULL;

	child_index = TC_H_MIN(parent);
	tclass = MLXSW_SP_PRIO_CHILD_TO_TCLASS(child_index);
	return &mlxsw_sp_port->tclass_qdiscs[tclass];
}

static struct mlxsw_sp_qdisc *
mlxsw_sp_qdisc_find_by_handle(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle)
{
	int i;

	if (mlxsw_sp_port->root_qdisc->handle == handle)
		return mlxsw_sp_port->root_qdisc;

	if (mlxsw_sp_port->root_qdisc->handle == TC_H_UNSPEC)
		return NULL;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
		if (mlxsw_sp_port->tclass_qdiscs[i].handle == handle)
			return &mlxsw_sp_port->tclass_qdiscs[i];

	return NULL;
}

static int
mlxsw_sp_qdisc_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
		       struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	int err = 0;

	if (!mlxsw_sp_qdisc)
		return 0;

	if (mlxsw_sp_qdisc->ops && mlxsw_sp_qdisc->ops->destroy)
		err = mlxsw_sp_qdisc->ops->destroy(mlxsw_sp_port,
						   mlxsw_sp_qdisc);

	mlxsw_sp_qdisc->handle = TC_H_UNSPEC;
	mlxsw_sp_qdisc->ops = NULL;
	return err;
}

static int
mlxsw_sp_qdisc_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
		       struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
		       struct mlxsw_sp_qdisc_ops *ops, void *params)
{
	int err;

	if (mlxsw_sp_qdisc->ops && mlxsw_sp_qdisc->ops->type != ops->type)
		/* In case this location contained a different qdisc of the
		 * same type we can override the old qdisc configuration.
		 * Otherwise, we need to remove the old qdisc before setting the
		 * new one.
		 */
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, mlxsw_sp_qdisc);
	err = ops->check_params(mlxsw_sp_port, mlxsw_sp_qdisc, params);
	if (err)
		goto err_bad_param;

	err = ops->replace(mlxsw_sp_port, mlxsw_sp_qdisc, params);
	if (err)
		goto err_config;

	if (mlxsw_sp_qdisc->handle != handle) {
		mlxsw_sp_qdisc->ops = ops;
		if (ops->clean_stats)
			ops->clean_stats(mlxsw_sp_port, mlxsw_sp_qdisc);
	}

	mlxsw_sp_qdisc->handle = handle;
	return 0;

err_bad_param:
err_config:
	if (mlxsw_sp_qdisc->handle == handle && ops->unoffload)
		ops->unoffload(mlxsw_sp_port, mlxsw_sp_qdisc, params);

	mlxsw_sp_qdisc_destroy(mlxsw_sp_port, mlxsw_sp_qdisc);
	return err;
}

static int
mlxsw_sp_qdisc_get_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			 struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			 struct tc_qopt_offload_stats *stats_ptr)
{
	if (mlxsw_sp_qdisc && mlxsw_sp_qdisc->ops &&
	    mlxsw_sp_qdisc->ops->get_stats)
		return mlxsw_sp_qdisc->ops->get_stats(mlxsw_sp_port,
						      mlxsw_sp_qdisc,
						      stats_ptr);

	return -EOPNOTSUPP;
}

static int
mlxsw_sp_qdisc_get_xstats(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			  void *xstats_ptr)
{
	if (mlxsw_sp_qdisc && mlxsw_sp_qdisc->ops &&
	    mlxsw_sp_qdisc->ops->get_xstats)
		return mlxsw_sp_qdisc->ops->get_xstats(mlxsw_sp_port,
						      mlxsw_sp_qdisc,
						      xstats_ptr);

	return -EOPNOTSUPP;
}

static void
mlxsw_sp_qdisc_bstats_per_priority_get(struct mlxsw_sp_port_xstats *xstats,
				       u8 prio_bitmap, u64 *tx_packets,
				       u64 *tx_bytes)
{
	int i;

	*tx_packets = 0;
	*tx_bytes = 0;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		if (prio_bitmap & BIT(i)) {
			*tx_packets += xstats->tx_packets[i];
			*tx_bytes += xstats->tx_bytes[i];
		}
	}
}

static int
mlxsw_sp_tclass_congestion_enable(struct mlxsw_sp_port *mlxsw_sp_port,
				  int tclass_num, u32 min, u32 max,
				  u32 probability, bool is_ecn)
{
	char cwtpm_cmd[MLXSW_REG_CWTPM_LEN];
	char cwtp_cmd[MLXSW_REG_CWTP_LEN];
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	int err;

	mlxsw_reg_cwtp_pack(cwtp_cmd, mlxsw_sp_port->local_port, tclass_num);
	mlxsw_reg_cwtp_profile_pack(cwtp_cmd, MLXSW_REG_CWTP_DEFAULT_PROFILE,
				    roundup(min, MLXSW_REG_CWTP_MIN_VALUE),
				    roundup(max, MLXSW_REG_CWTP_MIN_VALUE),
				    probability);

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(cwtp), cwtp_cmd);
	if (err)
		return err;

	mlxsw_reg_cwtpm_pack(cwtpm_cmd, mlxsw_sp_port->local_port, tclass_num,
			     MLXSW_REG_CWTP_DEFAULT_PROFILE, true, is_ecn);

	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(cwtpm), cwtpm_cmd);
}

static int
mlxsw_sp_tclass_congestion_disable(struct mlxsw_sp_port *mlxsw_sp_port,
				   int tclass_num)
{
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	char cwtpm_cmd[MLXSW_REG_CWTPM_LEN];

	mlxsw_reg_cwtpm_pack(cwtpm_cmd, mlxsw_sp_port->local_port, tclass_num,
			     MLXSW_REG_CWTPM_RESET_PROFILE, false, false);
	return mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(cwtpm), cwtpm_cmd);
}

static void
mlxsw_sp_setup_tc_qdisc_red_clean_stats(struct mlxsw_sp_port *mlxsw_sp_port,
					struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	u8 tclass_num = mlxsw_sp_qdisc->tclass_num;
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;
	struct red_stats *red_base;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats_base = &mlxsw_sp_qdisc->stats_base;
	red_base = &mlxsw_sp_qdisc->xstats_base.red;

	mlxsw_sp_qdisc_bstats_per_priority_get(xstats,
					       mlxsw_sp_qdisc->prio_bitmap,
					       &stats_base->tx_packets,
					       &stats_base->tx_bytes);
	red_base->prob_mark = xstats->ecn;
	red_base->prob_drop = xstats->wred_drop[tclass_num];
	red_base->pdrop = xstats->tail_drop[tclass_num];

	stats_base->overlimits = red_base->prob_drop + red_base->prob_mark;
	stats_base->drops = red_base->prob_drop + red_base->pdrop;

	stats_base->backlog = 0;
}

static int
mlxsw_sp_qdisc_red_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	struct mlxsw_sp_qdisc *root_qdisc = mlxsw_sp_port->root_qdisc;

	if (root_qdisc != mlxsw_sp_qdisc)
		root_qdisc->stats_base.backlog -=
					mlxsw_sp_qdisc->stats_base.backlog;

	return mlxsw_sp_tclass_congestion_disable(mlxsw_sp_port,
						  mlxsw_sp_qdisc->tclass_num);
}

static int
mlxsw_sp_qdisc_red_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
				void *params)
{
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	struct tc_red_qopt_offload_params *p = params;

	if (p->min > p->max) {
		dev_err(mlxsw_sp->bus_info->dev,
			"spectrum: RED: min %u is bigger then max %u\n", p->min,
			p->max);
		return -EINVAL;
	}
	if (p->max > MLXSW_CORE_RES_GET(mlxsw_sp->core, MAX_BUFFER_SIZE)) {
		dev_err(mlxsw_sp->bus_info->dev,
			"spectrum: RED: max value %u is too big\n", p->max);
		return -EINVAL;
	}
	if (p->min == 0 || p->max == 0) {
		dev_err(mlxsw_sp->bus_info->dev,
			"spectrum: RED: 0 value is illegal for min and max\n");
		return -EINVAL;
	}
	return 0;
}

static int
mlxsw_sp_qdisc_red_replace(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			   void *params)
{
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	struct tc_red_qopt_offload_params *p = params;
	u8 tclass_num = mlxsw_sp_qdisc->tclass_num;
	u32 min, max;
	u64 prob;

	/* calculate probability in percentage */
	prob = p->probability;
	prob *= 100;
	prob = DIV_ROUND_UP(prob, 1 << 16);
	prob = DIV_ROUND_UP(prob, 1 << 16);
	min = mlxsw_sp_bytes_cells(mlxsw_sp, p->min);
	max = mlxsw_sp_bytes_cells(mlxsw_sp, p->max);
	return mlxsw_sp_tclass_congestion_enable(mlxsw_sp_port, tclass_num, min,
						 max, prob, p->is_ecn);
}

static void
mlxsw_sp_qdisc_red_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     void *params)
{
	struct tc_red_qopt_offload_params *p = params;
	u64 backlog;

	backlog = mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
				       mlxsw_sp_qdisc->stats_base.backlog);
	p->qstats->backlog -= backlog;
	mlxsw_sp_qdisc->stats_base.backlog = 0;
}

static int
mlxsw_sp_qdisc_get_red_xstats(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      void *xstats_ptr)
{
	struct red_stats *xstats_base = &mlxsw_sp_qdisc->xstats_base.red;
	u8 tclass_num = mlxsw_sp_qdisc->tclass_num;
	struct mlxsw_sp_port_xstats *xstats;
	struct red_stats *res = xstats_ptr;
	int early_drops, marks, pdrops;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;

	early_drops = xstats->wred_drop[tclass_num] - xstats_base->prob_drop;
	marks = xstats->ecn - xstats_base->prob_mark;
	pdrops = xstats->tail_drop[tclass_num] - xstats_base->pdrop;

	res->pdrop += pdrops;
	res->prob_drop += early_drops;
	res->prob_mark += marks;

	xstats_base->pdrop += pdrops;
	xstats_base->prob_drop += early_drops;
	xstats_base->prob_mark += marks;
	return 0;
}

static int
mlxsw_sp_qdisc_get_red_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     struct tc_qopt_offload_stats *stats_ptr)
{
	u64 tx_bytes, tx_packets, overlimits, drops, backlog;
	u8 tclass_num = mlxsw_sp_qdisc->tclass_num;
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats_base = &mlxsw_sp_qdisc->stats_base;

	mlxsw_sp_qdisc_bstats_per_priority_get(xstats,
					       mlxsw_sp_qdisc->prio_bitmap,
					       &tx_packets, &tx_bytes);
	tx_bytes = tx_bytes - stats_base->tx_bytes;
	tx_packets = tx_packets - stats_base->tx_packets;

	overlimits = xstats->wred_drop[tclass_num] + xstats->ecn -
		     stats_base->overlimits;
	drops = xstats->wred_drop[tclass_num] + xstats->tail_drop[tclass_num] -
		stats_base->drops;
	backlog = xstats->backlog[tclass_num];

	_bstats_update(stats_ptr->bstats, tx_bytes, tx_packets);
	stats_ptr->qstats->overlimits += overlimits;
	stats_ptr->qstats->drops += drops;
	stats_ptr->qstats->backlog +=
				mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
						     backlog) -
				mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
						     stats_base->backlog);

	stats_base->backlog = backlog;
	stats_base->drops +=  drops;
	stats_base->overlimits += overlimits;
	stats_base->tx_bytes += tx_bytes;
	stats_base->tx_packets += tx_packets;
	return 0;
}

#define MLXSW_SP_PORT_DEFAULT_TCLASS 0

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_red = {
	.type = MLXSW_SP_QDISC_RED,
	.check_params = mlxsw_sp_qdisc_red_check_params,
	.replace = mlxsw_sp_qdisc_red_replace,
	.unoffload = mlxsw_sp_qdisc_red_unoffload,
	.destroy = mlxsw_sp_qdisc_red_destroy,
	.get_stats = mlxsw_sp_qdisc_get_red_stats,
	.get_xstats = mlxsw_sp_qdisc_get_red_xstats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_red_clean_stats,
};

int mlxsw_sp_setup_tc_red(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct tc_red_qopt_offload *p)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc;

	mlxsw_sp_qdisc = mlxsw_sp_qdisc_find(mlxsw_sp_port, p->parent, false);
	if (!mlxsw_sp_qdisc)
		return -EOPNOTSUPP;

	if (p->command == TC_RED_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle,
					      mlxsw_sp_qdisc,
					      &mlxsw_sp_qdisc_ops_red,
					      &p->set);

	if (!mlxsw_sp_qdisc_compare(mlxsw_sp_qdisc, p->handle,
				    MLXSW_SP_QDISC_RED))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_RED_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, mlxsw_sp_qdisc);
	case TC_RED_XSTATS:
		return mlxsw_sp_qdisc_get_xstats(mlxsw_sp_port, mlxsw_sp_qdisc,
						 p->xstats);
	case TC_RED_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, mlxsw_sp_qdisc,
						&p->stats);
	default:
		return -EOPNOTSUPP;
	}
}

static int
mlxsw_sp_qdisc_prio_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	int i;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		mlxsw_sp_port_prio_tc_set(mlxsw_sp_port, i,
					  MLXSW_SP_PORT_DEFAULT_TCLASS);
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port,
				       &mlxsw_sp_port->tclass_qdiscs[i]);
		mlxsw_sp_port->tclass_qdiscs[i].prio_bitmap = 0;
	}

	return 0;
}

static int
mlxsw_sp_qdisc_prio_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				 struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
				 void *params)
{
	struct tc_prio_qopt_offload_params *p = params;

	if (p->bands > IEEE_8021QAZ_MAX_TCS)
		return -EOPNOTSUPP;

	return 0;
}

static int
mlxsw_sp_qdisc_prio_replace(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    void *params)
{
	struct tc_prio_qopt_offload_params *p = params;
	struct mlxsw_sp_qdisc *child_qdisc;
	int tclass, i, band, backlog;
	u8 old_priomap;
	int err;

	for (band = 0; band < p->bands; band++) {
		tclass = MLXSW_SP_PRIO_BAND_TO_TCLASS(band);
		child_qdisc = &mlxsw_sp_port->tclass_qdiscs[tclass];
		old_priomap = child_qdisc->prio_bitmap;
		child_qdisc->prio_bitmap = 0;
		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
			if (p->priomap[i] == band) {
				child_qdisc->prio_bitmap |= BIT(i);
				if (BIT(i) & old_priomap)
					continue;
				err = mlxsw_sp_port_prio_tc_set(mlxsw_sp_port,
								i, tclass);
				if (err)
					return err;
			}
		}
		if (old_priomap != child_qdisc->prio_bitmap &&
		    child_qdisc->ops && child_qdisc->ops->clean_stats) {
			backlog = child_qdisc->stats_base.backlog;
			child_qdisc->ops->clean_stats(mlxsw_sp_port,
						      child_qdisc);
			child_qdisc->stats_base.backlog = backlog;
		}
	}
	for (; band < IEEE_8021QAZ_MAX_TCS; band++) {
		tclass = MLXSW_SP_PRIO_BAND_TO_TCLASS(band);
		child_qdisc = &mlxsw_sp_port->tclass_qdiscs[tclass];
		child_qdisc->prio_bitmap = 0;
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, child_qdisc);
	}
	return 0;
}

static void
mlxsw_sp_qdisc_prio_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      void *params)
{
	struct tc_prio_qopt_offload_params *p = params;
	u64 backlog;

	backlog = mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
				       mlxsw_sp_qdisc->stats_base.backlog);
	p->qstats->backlog -= backlog;
}

static int
mlxsw_sp_qdisc_get_prio_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      struct tc_qopt_offload_stats *stats_ptr)
{
	u64 tx_bytes, tx_packets, drops = 0, backlog = 0;
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;
	struct rtnl_link_stats64 *stats;
	int i;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats = &mlxsw_sp_port->periodic_hw_stats.stats;
	stats_base = &mlxsw_sp_qdisc->stats_base;

	tx_bytes = stats->tx_bytes - stats_base->tx_bytes;
	tx_packets = stats->tx_packets - stats_base->tx_packets;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		drops += xstats->tail_drop[i];
		drops += xstats->wred_drop[i];
		backlog += xstats->backlog[i];
	}
	drops = drops - stats_base->drops;

	_bstats_update(stats_ptr->bstats, tx_bytes, tx_packets);
	stats_ptr->qstats->drops += drops;
	stats_ptr->qstats->backlog +=
				mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
						     backlog) -
				mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
						     stats_base->backlog);
	stats_base->backlog = backlog;
	stats_base->drops += drops;
	stats_base->tx_bytes += tx_bytes;
	stats_base->tx_packets += tx_packets;
	return 0;
}

static void
mlxsw_sp_setup_tc_qdisc_prio_clean_stats(struct mlxsw_sp_port *mlxsw_sp_port,
					 struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;
	struct rtnl_link_stats64 *stats;
	int i;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats = &mlxsw_sp_port->periodic_hw_stats.stats;
	stats_base = &mlxsw_sp_qdisc->stats_base;

	stats_base->tx_packets = stats->tx_packets;
	stats_base->tx_bytes = stats->tx_bytes;

	stats_base->drops = 0;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		stats_base->drops += xstats->tail_drop[i];
		stats_base->drops += xstats->wred_drop[i];
	}

	mlxsw_sp_qdisc->stats_base.backlog = 0;
}

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_prio = {
	.type = MLXSW_SP_QDISC_PRIO,
	.check_params = mlxsw_sp_qdisc_prio_check_params,
	.replace = mlxsw_sp_qdisc_prio_replace,
	.unoffload = mlxsw_sp_qdisc_prio_unoffload,
	.destroy = mlxsw_sp_qdisc_prio_destroy,
	.get_stats = mlxsw_sp_qdisc_get_prio_stats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_prio_clean_stats,
};

/* Grafting is not supported in mlxsw. It will result in un-offloading of the
 * grafted qdisc as well as the qdisc in the qdisc new location.
 * (However, if the graft is to the location where the qdisc is already at, it
 * will be ignored completely and won't cause un-offloading).
 */
static int
mlxsw_sp_qdisc_prio_graft(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			  struct tc_prio_qopt_offload_graft_params *p)
{
	int tclass_num = MLXSW_SP_PRIO_BAND_TO_TCLASS(p->band);
	struct mlxsw_sp_qdisc *old_qdisc;

	/* Check if the grafted qdisc is already in its "new" location. If so -
	 * nothing needs to be done.
	 */
	if (p->band < IEEE_8021QAZ_MAX_TCS &&
	    mlxsw_sp_port->tclass_qdiscs[tclass_num].handle == p->child_handle)
		return 0;

	/* See if the grafted qdisc is already offloaded on any tclass. If so,
	 * unoffload it.
	 */
	old_qdisc = mlxsw_sp_qdisc_find_by_handle(mlxsw_sp_port,
						  p->child_handle);
	if (old_qdisc)
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, old_qdisc);

	mlxsw_sp_qdisc_destroy(mlxsw_sp_port,
			       &mlxsw_sp_port->tclass_qdiscs[tclass_num]);
	return -EOPNOTSUPP;
}

struct mlxsw_sp_qdisc_tc_config {
	unsigned int group;
	bool has_shaper;
	bool dwrr;
	bool red;
	unsigned int shaper;
	unsigned int weight;
};

struct mlxsw_sp_qdisc_config {
	unsigned int num_tcs;
	unsigned int prio_tc[8];
	struct mlxsw_sp_qdisc_tc_config tc[8];
};

struct mlxsw_sp_qdisc_walk_state {
	struct mlxsw_sp_qdisc_config config;
	unsigned int num_grs;
	int dwrr; /* -1=none, 0=strict, 1=dwrr*/
};

static int mlxsw_sp_qdisc_new_band(struct mlxsw_sp_qdisc_walk_state *wst,
				   u8 priomap, bool dwrr)
{
	int prio;
	int tc;

	if (wst->config.num_tcs == 8)
		return -ENOBUFS;

	if (wst->dwrr != dwrr) {
		if (wst->num_grs == 8)
			return -ENOBUFS;
		wst->num_grs++;
		wst->dwrr = dwrr;
	}

	tc = wst->config.num_tcs++;
	wst->config.tc[tc] = (struct mlxsw_sp_qdisc_tc_config) {
		.group = wst->num_grs - 1,
		.dwrr = dwrr,
	};

	for (prio = 0; prio < 8; prio++) {
		if (priomap & (1 << prio))
			wst->config.prio_tc[prio] = tc;
	}

	return tc;
}

static int mlxsw_sp_qdisc_handle_red(struct mlxsw_sp_qdisc_tc_config *tcc,
				     struct Qdisc *sch)
{
	tcc->red = true;
	return 0;
}

static int mlxsw_sp_qdisc_handle_cbs(struct mlxsw_sp_qdisc_tc_config *tcc,
				     struct Qdisc *sch)
{
	tcc->has_shaper = true;
	tcc->shaper = 123;
	return 0;
}

static int mlxsw_sp_qdisc_handle_leaf(struct mlxsw_sp_qdisc_tc_config *tcc,
				      struct Qdisc *sch)
{
	switch (mlxsw_sp_qdisc_type(sch)) {
	case MLXSW_SP_QDISC_NO_QDISC:
		return 0;
	case MLXSW_SP_QDISC_RED:
		return mlxsw_sp_qdisc_handle_red(tcc, sch);
	case MLXSW_SP_QDISC_CBS:
		return mlxsw_sp_qdisc_handle_cbs(tcc, sch);

	case MLXSW_SP_QDISC_PRIO:
	case MLXSW_SP_QDISC_DRR:
		break;
	}

	return -EOPNOTSUPP;
}

struct mlxsw_sp_qdisc_drr_walker {
	struct qdisc_walker w;
	struct mlxsw_sp_qdisc_walk_state *wst;
	u8 priomap;
	int err;
};

static int mlxsw_sp_qdisc_walk_drr_class(struct Qdisc *sch, unsigned long cl,
					 struct qdisc_walker *w)
{
	struct mlxsw_sp_qdisc_drr_walker *dw =
		container_of(w, struct mlxsw_sp_qdisc_drr_walker, w);
	struct Qdisc *leaf = sch->ops->cl_ops->leaf(sch, cl);
	u16 cl_priomap = sch->ops->cl_ops->priomap(sch, cl);
	u8 common_priomap = dw->priomap & (u8)cl_priomap;
	int err;
	int tc;

	if (!common_priomap)
		return 0;

	tc = mlxsw_sp_qdisc_new_band(dw->wst, common_priomap, true);
	if (tc < 0) {
		dw->err = tc;
		return -1;
	}

	err = mlxsw_sp_qdisc_handle_leaf(&dw->wst->config.tc[tc], leaf);
	if (err) {
		dw->err = err;
		return -1;
	}

	dw->priomap &= ~common_priomap;
	return 0;
}

static int mlxsw_sp_qdisc_walk_drr(struct mlxsw_sp_qdisc_walk_state *wst,
				   struct Qdisc *sch, u8 priomap)
{
	struct mlxsw_sp_qdisc_drr_walker dw = {
		.w.fn = mlxsw_sp_qdisc_walk_drr_class,
		.wst = wst,
		.priomap = priomap,
	};
	int tc;

	sch->ops->cl_ops->walk(sch, &dw.w);
	if (dw.err)
		return dw.err;

	if (dw.priomap) {
		tc = mlxsw_sp_qdisc_new_band(wst, dw.priomap, true);
		if (tc < 0)
			return tc;

		wst->config.tc[tc].has_shaper = true;
		wst->config.tc[tc].shaper = 0;
	}

	wst->dwrr = -1;
	return 0;
}

static int mlxsw_sp_qdisc_walk_prio_child(struct mlxsw_sp_qdisc_walk_state *wst,
					  struct Qdisc *sch, u8 priomap)
{
	enum mlxsw_sp_qdisc_type type = mlxsw_sp_qdisc_type(sch);
	int tc;

	if (!priomap)
		return 0;

	if (type == MLXSW_SP_QDISC_DRR)
		return mlxsw_sp_qdisc_walk_drr(wst, sch, priomap);

	tc = mlxsw_sp_qdisc_new_band(wst, priomap, false);
	if (tc < 0)
		return tc;

	return mlxsw_sp_qdisc_handle_leaf(&wst->config.tc[tc], sch);
}

struct mlxsw_sp_qdisc_prio_walker {
	struct qdisc_walker w;
	struct mlxsw_sp_qdisc_walk_state *wst;
	int err;
};

static int mlxsw_sp_qdisc_walk_prio_band(struct Qdisc *sch, unsigned long cl,
					 struct qdisc_walker *w)
{
	struct mlxsw_sp_qdisc_prio_walker *pw =
		container_of(w, struct mlxsw_sp_qdisc_prio_walker, w);
	struct Qdisc *leaf = sch->ops->cl_ops->leaf(sch, cl);
	u16 priomap = sch->ops->cl_ops->priomap(sch, cl);
	int err;

	err = mlxsw_sp_qdisc_walk_prio_child(pw->wst, leaf, priomap);
	if (err) {
		pw->err = err;
		return -1;
	}
	return 0;
}

static int mlxsw_sp_qdisc_walk_prio(struct mlxsw_sp_qdisc_walk_state *wst,
				     struct Qdisc *sch)
{
	struct mlxsw_sp_qdisc_prio_walker pw = {
		.w.fn = mlxsw_sp_qdisc_walk_prio_band,
		.wst = wst,
	};

	sch->ops->cl_ops->walk(sch, &pw.w);
	return pw.err;
}

static void dump_config(struct mlxsw_sp_qdisc_config *cfg)
{
	int prio;
	int tc;

	for (prio = 0; prio < 8; prio++) {
		printk(KERN_WARNING "prio %d -> TC %d\n",
		       prio, cfg->prio_tc[prio]);
	}

	for (tc = 0; tc < cfg->num_tcs; tc++) {
		struct mlxsw_sp_qdisc_tc_config *tcc = &cfg->tc[tc];
		char buf[100] = "strict";

		if (tcc->dwrr)
			sprintf(buf, "dwrr w=%d", tcc->weight);
		printk(KERN_WARNING "SG %d -(%s)-> GR %d\n",
		       tc, buf, tcc->group);
		if (tcc->has_shaper)
			printk(KERN_WARNING " - max_shaper %d\n", tcc->shaper);
		if (tcc->red)
			printk(KERN_WARNING " - RED\n");
	}
}

static void mlxsw_sp_qdisc_walk(struct mlxsw_sp_port *mlxsw_sp_port)
{
	struct Qdisc *sch = mlxsw_sp_port->dev->qdisc;
	struct mlxsw_sp_qdisc_walk_state wst;

	memset(&wst, 0, sizeof(wst));
	wst.dwrr = -1;

	switch (mlxsw_sp_qdisc_type(sch)) {
	case MLXSW_SP_QDISC_NO_QDISC:
		break;
	case MLXSW_SP_QDISC_PRIO:
		mlxsw_sp_qdisc_walk_prio(&wst, sch);
		break;
	case MLXSW_SP_QDISC_DRR:
	case MLXSW_SP_QDISC_RED:
	case MLXSW_SP_QDISC_CBS:
		mlxsw_sp_qdisc_walk_prio_child(&wst, sch, 0xff);
		break;
	}

	dump_config(&wst.config);
}

int mlxsw_sp_setup_tc_prio(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct tc_prio_qopt_offload *p)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc;

	mlxsw_sp_qdisc = mlxsw_sp_qdisc_find(mlxsw_sp_port, p->parent, true);
	if (!mlxsw_sp_qdisc)
		return -EOPNOTSUPP;

	if (p->command == TC_PRIO_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle,
					      mlxsw_sp_qdisc,
					      &mlxsw_sp_qdisc_ops_prio,
					      &p->replace_params);

	if (!mlxsw_sp_qdisc_compare(mlxsw_sp_qdisc, p->handle,
				    MLXSW_SP_QDISC_PRIO))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_PRIO_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, mlxsw_sp_qdisc);
	case TC_PRIO_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, mlxsw_sp_qdisc,
						&p->stats);
	case TC_PRIO_GRAFT:
		return mlxsw_sp_qdisc_prio_graft(mlxsw_sp_port, mlxsw_sp_qdisc,
						 &p->graft_params);
	default:
		return -EOPNOTSUPP;
	}
}

int mlxsw_sp_setup_tc_post_change(struct mlxsw_sp_port *mlxsw_sp_port,
				  struct tc_post_change_qopt_offload *p)
{
	switch (p->command) {
	case TC_POST_CHANGE:
		mlxsw_sp_qdisc_walk(mlxsw_sp_port);
	}

	return 0;
}

int mlxsw_sp_tc_qdisc_init(struct mlxsw_sp_port *mlxsw_sp_port)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc;
	int i;

	mlxsw_sp_qdisc = kzalloc(sizeof(*mlxsw_sp_qdisc), GFP_KERNEL);
	if (!mlxsw_sp_qdisc)
		goto err_root_qdisc_init;

	mlxsw_sp_port->root_qdisc = mlxsw_sp_qdisc;
	mlxsw_sp_port->root_qdisc->prio_bitmap = 0xff;
	mlxsw_sp_port->root_qdisc->tclass_num = MLXSW_SP_PORT_DEFAULT_TCLASS;

	mlxsw_sp_qdisc = kcalloc(IEEE_8021QAZ_MAX_TCS,
				 sizeof(*mlxsw_sp_qdisc),
				 GFP_KERNEL);
	if (!mlxsw_sp_qdisc)
		goto err_tclass_qdiscs_init;

	mlxsw_sp_port->tclass_qdiscs = mlxsw_sp_qdisc;
	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
		mlxsw_sp_port->tclass_qdiscs[i].tclass_num = i;

	return 0;

err_tclass_qdiscs_init:
	kfree(mlxsw_sp_port->root_qdisc);
err_root_qdisc_init:
	return -ENOMEM;
}

void mlxsw_sp_tc_qdisc_fini(struct mlxsw_sp_port *mlxsw_sp_port)
{
	kfree(mlxsw_sp_port->tclass_qdiscs);
	kfree(mlxsw_sp_port->root_qdisc);
}
