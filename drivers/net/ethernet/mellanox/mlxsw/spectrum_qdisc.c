// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2017-2018 Mellanox Technologies. All rights reserved */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/netdevice.h>
#include <net/pkt_cls.h>
#include <net/red.h>

#include "spectrum.h"
#include "spectrum_span.h"
#include "reg.h"

#define MLXSW_SP_PRIO_BAND_TO_TCLASS(band) (IEEE_8021QAZ_MAX_TCS - band - 1)
#define MLXSW_SP_PRIO_CHILD_TO_TCLASS(child) \
	MLXSW_SP_PRIO_BAND_TO_TCLASS((child - 1))

enum mlxsw_sp_qdisc_type {
	MLXSW_SP_QDISC_NO_QDISC,
	MLXSW_SP_QDISC_RED,
	MLXSW_SP_QDISC_PRIO,
	MLXSW_SP_QDISC_ETS,
	MLXSW_SP_QDISC_TBF,
	MLXSW_SP_QDISC_FIFO,
};

struct mlxsw_sp_qdisc;
struct mlxsw_sp_qdisc_class;

struct mlxsw_sp_qdisc_ops {
	enum mlxsw_sp_qdisc_type type;
	int (*check_params)(struct mlxsw_sp_port *mlxsw_sp_port,
			    void *params);
	int (*replace)(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
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

	struct mlxsw_sp_qdisc_class *(*find_class)(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
						   u32 parent);
	unsigned int num_classes;
};

struct mlxsw_sp_qdisc {
	u32 handle;
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
	struct mlxsw_sp_qdisc_class *classes;
	struct mlxsw_sp_qdisc_class *parent_class;
	unsigned int num_classes;
};

struct mlxsw_sp_qdisc_class {
	struct mlxsw_sp_qdisc *qdisc;
	struct mlxsw_sp_qdisc *parent_qdisc;
	u8 tclass_num;
	u8 prio_bitmap;
};

struct mlxsw_sp_qdisc_state {
	struct mlxsw_sp_qdisc_class root_class;

	/* When a PRIO or ETS are added, the invisible FIFOs in their bands are
	 * created first. When notifications for these FIFOs arrive, it is not
	 * known what qdisc their parent handle refers to. It could be a
	 * newly-created PRIO that will replace the currently-offloaded one, or
	 * it could be e.g. a RED that will be attached below it.
	 *
	 * As the notifications start to arrive, use them to note what the
	 * future parent handle is, and keep track of which child FIFOs were
	 * seen. Then when the parent is known, retroactively offload those
	 * FIFOs.
	 */
	u32 future_handle;
	bool future_fifos[IEEE_8021QAZ_MAX_TCS];
};

/* Filters out events pertaining to qdiscs that either never were offloaded or
 * are already unoffloaded, e.g. due to having been replaced by a new qdisc.
 */
static bool mlxsw_sp_qdisc_compare(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, u32 handle)
{
	return mlxsw_sp_qdisc && mlxsw_sp_qdisc->handle == handle;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_walk(struct mlxsw_sp_qdisc_class *class,
		    struct mlxsw_sp_qdisc_class *(*pre)(struct mlxsw_sp_qdisc_class *, void *),
		    void *data)
{
	struct mlxsw_sp_qdisc_class *tmp;
	unsigned i;

	if (pre) {
		tmp = pre(class, data);
		if (tmp)
			return tmp;
	}

	if (class->qdisc) {
		for (i = 0; i < class->qdisc->num_classes; i++) {
			tmp = &class->qdisc->classes[i];
			if (tmp->qdisc) {
				tmp = mlxsw_sp_qdisc_walk(tmp, pre, data);
				if (tmp)
					return tmp;
			}
		}
	}

	return NULL;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_walk_cb_find_class(struct mlxsw_sp_qdisc_class *class, void *data)
{
	u32 parent = *(u32 *) data;

	if (class->qdisc && TC_H_MAJ(class->qdisc->handle) == TC_H_MAJ(parent)) {
		if (class->qdisc->ops->find_class)
			return class->qdisc->ops->find_class(class->qdisc, parent);
	}

	return NULL;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_find_class(struct mlxsw_sp_port *mlxsw_sp_port, u32 parent)
{
	if (parent == TC_H_ROOT)
		return &mlxsw_sp_port->qdisc->root_class;
	return mlxsw_sp_qdisc_walk(&mlxsw_sp_port->qdisc->root_class,
				   mlxsw_sp_qdisc_walk_cb_find_class, &parent);
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_walk_cb_find_by_handle(struct mlxsw_sp_qdisc_class *class, void *data)
{
	u32 handle = *(u32 *) data;

	if (class->qdisc && class->qdisc->handle == handle)
		return class;
	return NULL;
}

static struct mlxsw_sp_qdisc *
mlxsw_sp_qdisc_find_by_handle(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle)
{
	struct mlxsw_sp_qdisc_class *class;

	class = mlxsw_sp_qdisc_walk(&mlxsw_sp_port->qdisc->root_class,
				    mlxsw_sp_qdisc_walk_cb_find_by_handle, &handle);
	if (class)
		return class->qdisc;
	return NULL;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_walk_cb_set_priomap(struct mlxsw_sp_qdisc_class *class, void *data)
{
	class->prio_bitmap = *(u8 *) data;
	return NULL;
}

static void mlxsw_sp_qdisc_subtree_set_priomap(struct mlxsw_sp_qdisc_class *class, u8 prio_bitmap)
{
	mlxsw_sp_qdisc_walk(class, mlxsw_sp_qdisc_walk_cb_set_priomap, &prio_bitmap);
}

static void mlxsw_sp_qdisc_reduce_parent_backlog(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	struct mlxsw_sp_qdisc *tmp;

	for (tmp = mlxsw_sp_qdisc->parent_class->parent_qdisc; tmp;
	     tmp = tmp->parent_class->parent_qdisc)
		tmp->stats_base.backlog -= mlxsw_sp_qdisc->stats_base.backlog;
}

static int
mlxsw_sp_qdisc_destroy(struct mlxsw_sp_port *mlxsw_sp_port, struct mlxsw_sp_qdisc_class *class)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc = class->qdisc;
	int err = 0;

	if (WARN_ON(!mlxsw_sp_qdisc))
		return 0;

	mlxsw_sp_qdisc_reduce_parent_backlog(mlxsw_sp_qdisc);
	if (mlxsw_sp_qdisc->ops->destroy)
		err = mlxsw_sp_qdisc->ops->destroy(mlxsw_sp_port, mlxsw_sp_qdisc);

	kfree(mlxsw_sp_qdisc->classes);
	kfree(mlxsw_sp_qdisc);
	class->qdisc = NULL;
	return err;
}

static int mlxsw_sp_qdisc_create(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
				 struct mlxsw_sp_qdisc_class *class,
				 struct mlxsw_sp_qdisc_ops *ops, void *params)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc;
	unsigned int i;
	int err;

	printk(KERN_WARNING "qdisc_create\n");
	mlxsw_sp_qdisc = kzalloc(sizeof(*mlxsw_sp_qdisc), GFP_KERNEL);
	if (!mlxsw_sp_qdisc)
		return -ENOMEM;

	if (ops->num_classes) {
		mlxsw_sp_qdisc->num_classes = ops->num_classes;
		mlxsw_sp_qdisc->classes = kcalloc(ops->num_classes,
						  sizeof(*mlxsw_sp_qdisc->classes), GFP_KERNEL);
		if (!mlxsw_sp_qdisc->classes)
			goto err_classes;

		for (i = 0; i < ops->num_classes; i++)
			mlxsw_sp_qdisc->classes[i].parent_qdisc = mlxsw_sp_qdisc;
	}

	mlxsw_sp_qdisc->ops = ops;
	mlxsw_sp_qdisc->handle = handle;
	mlxsw_sp_qdisc->parent_class = class;
	err = ops->replace(mlxsw_sp_port, handle, mlxsw_sp_qdisc, params);
	if (err)
		goto err_replace;

	class->qdisc = mlxsw_sp_qdisc;
	return 0;

err_replace:
	kfree(mlxsw_sp_qdisc->classes);
err_classes:
	kfree(mlxsw_sp_qdisc);
	return err;
}

static int mlxsw_sp_qdisc_change(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
				 struct mlxsw_sp_qdisc_class *class,
				 struct mlxsw_sp_qdisc_ops *ops, void *params)
{
	struct mlxsw_sp_qdisc *mlxsw_sp_qdisc = class->qdisc;
	int err;

	printk(KERN_WARNING "qdisc_change\n");
	err = ops->replace(mlxsw_sp_port, handle, mlxsw_sp_qdisc, params);
	if (err)
		goto unoffload;

	/* Check if the Qdisc changed. That includes a situation where an
	 * invisible Qdisc replaces another one, or is being added for the
	 * first time.
	 */
	if (mlxsw_sp_qdisc->handle != handle) {
		if (ops->clean_stats)
			ops->clean_stats(mlxsw_sp_port, mlxsw_sp_qdisc);
	}

	mlxsw_sp_qdisc->ops = ops;
	mlxsw_sp_qdisc->handle = handle;
	return 0;

unoffload:
	if (mlxsw_sp_qdisc->handle == handle && ops->unoffload)
		ops->unoffload(mlxsw_sp_port, mlxsw_sp_qdisc, params);
	mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	return err;
}

static int
mlxsw_sp_qdisc_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
		       struct mlxsw_sp_qdisc_class *class,
		       struct mlxsw_sp_qdisc_ops *ops, void *params)
{
	int err;

	if (class->qdisc && class->qdisc->ops->type != ops->type) {
		/* In case this location contained a different qdisc of the
		 * same type we can override the old qdisc configuration.
		 * Otherwise, we need to remove the old qdisc before setting the
		 * new one.
		 */
		printk(KERN_WARNING "killing previous tenant\n");
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	}

	err = ops->check_params(mlxsw_sp_port, params);
	if (err)
		return err;

	if (!class->qdisc)
		return mlxsw_sp_qdisc_create(mlxsw_sp_port, handle, class, ops, params);
	else
		return mlxsw_sp_qdisc_change(mlxsw_sp_port, handle, class, ops, params);
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

static u64
mlxsw_sp_xstats_backlog(struct mlxsw_sp_port_xstats *xstats, int tclass_num)
{
	return xstats->backlog[tclass_num] +
	       xstats->backlog[tclass_num + 8];
}

static u64
mlxsw_sp_xstats_tail_drop(struct mlxsw_sp_port_xstats *xstats, int tclass_num)
{
	return xstats->tail_drop[tclass_num] +
	       xstats->tail_drop[tclass_num + 8];
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

static void
mlxsw_sp_qdisc_collect_tc_stats(struct mlxsw_sp_port *mlxsw_sp_port,
				u8 prio_bitmap, int tclass_num,
				u64 *p_tx_bytes, u64 *p_tx_packets,
				u64 *p_drops, u64 *p_backlog)
{
	struct mlxsw_sp_port_xstats *xstats;
	u64 tx_bytes, tx_packets;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	mlxsw_sp_qdisc_bstats_per_priority_get(xstats, prio_bitmap, &tx_packets, &tx_bytes);

	*p_tx_packets += tx_packets;
	*p_tx_bytes += tx_bytes;
	*p_drops += xstats->wred_drop[tclass_num] +
		    mlxsw_sp_xstats_tail_drop(xstats, tclass_num);
	*p_backlog += mlxsw_sp_xstats_backlog(xstats, tclass_num);
}

static void
mlxsw_sp_qdisc_update_stats(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    u64 tx_bytes, u64 tx_packets,
			    u64 drops, u64 backlog,
			    struct tc_qopt_offload_stats *stats_ptr)
{
	struct mlxsw_sp_qdisc_stats *stats_base = &mlxsw_sp_qdisc->stats_base;

	tx_bytes -= stats_base->tx_bytes;
	tx_packets -= stats_base->tx_packets;
	drops -= stats_base->drops;
	backlog -= stats_base->backlog;

	_bstats_update(stats_ptr->bstats, tx_bytes, tx_packets);
	stats_ptr->qstats->drops += drops;
	stats_ptr->qstats->backlog += mlxsw_sp_cells_bytes(mlxsw_sp, backlog);

	stats_base->backlog += backlog;
	stats_base->drops += drops;
	stats_base->tx_bytes += tx_bytes;
	stats_base->tx_packets += tx_packets;
}

static void
mlxsw_sp_qdisc_get_tc_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    struct tc_qopt_offload_stats *stats_ptr)
{
	u8 prio_bitmap = mlxsw_sp_qdisc->parent_class->prio_bitmap;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	u64 tx_packets = 0;
	u64 tx_bytes = 0;
	u64 backlog = 0;
	u64 drops = 0;

	mlxsw_sp_qdisc_collect_tc_stats(mlxsw_sp_port, prio_bitmap, tclass_num,
					&tx_bytes, &tx_packets,
					&drops, &backlog);
	mlxsw_sp_qdisc_update_stats(mlxsw_sp_port->mlxsw_sp, mlxsw_sp_qdisc,
				    tx_bytes, tx_packets, drops, backlog,
				    stats_ptr);
}

static int
mlxsw_sp_tclass_congestion_enable(struct mlxsw_sp_port *mlxsw_sp_port,
				  int tclass_num, u32 min, u32 max,
				  u32 probability, bool is_wred, bool is_ecn)
{
	char cwtpm_cmd[MLXSW_REG_CWTPM_LEN];
	char cwtp_cmd[MLXSW_REG_CWTP_LEN];
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	int err;

	printk(KERN_WARNING "congestion_enable %s TC %d RED %d ECN %d\n",
	       mlxsw_sp_port->dev->name, tclass_num, is_wred, is_ecn);
	mlxsw_reg_cwtp_pack(cwtp_cmd, mlxsw_sp_port->local_port, tclass_num);
	mlxsw_reg_cwtp_profile_pack(cwtp_cmd, MLXSW_REG_CWTP_DEFAULT_PROFILE,
				    roundup(min, MLXSW_REG_CWTP_MIN_VALUE),
				    roundup(max, MLXSW_REG_CWTP_MIN_VALUE),
				    probability);

	err = mlxsw_reg_write(mlxsw_sp->core, MLXSW_REG(cwtp), cwtp_cmd);
	if (err)
		return err;

	mlxsw_reg_cwtpm_pack(cwtpm_cmd, mlxsw_sp_port->local_port, tclass_num,
			     MLXSW_REG_CWTP_DEFAULT_PROFILE, is_wred, is_ecn);

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
	u8 prio_bitmap = mlxsw_sp_qdisc->parent_class->prio_bitmap;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;
	struct red_stats *red_base;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats_base = &mlxsw_sp_qdisc->stats_base;
	red_base = &mlxsw_sp_qdisc->xstats_base.red;

	mlxsw_sp_qdisc_bstats_per_priority_get(xstats, prio_bitmap,
					       &stats_base->tx_packets,
					       &stats_base->tx_bytes);
	red_base->prob_drop = xstats->wred_drop[tclass_num];
	red_base->pdrop = mlxsw_sp_xstats_tail_drop(xstats, tclass_num);

	stats_base->overlimits = red_base->prob_drop + red_base->prob_mark;
	stats_base->drops = red_base->prob_drop + red_base->pdrop;

	stats_base->backlog = 0;
}

static int
mlxsw_sp_qdisc_red_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	return mlxsw_sp_tclass_congestion_disable(mlxsw_sp_port,
						  mlxsw_sp_qdisc->parent_class->tclass_num);
}

static int
mlxsw_sp_qdisc_red_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
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
	if (p->max > MLXSW_CORE_RES_GET(mlxsw_sp->core,
					GUARANTEED_SHARED_BUFFER)) {
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
mlxsw_sp_qdisc_red_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			   void *params)
{
	u8 prio_bitmap = mlxsw_sp_qdisc->parent_class->prio_bitmap;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	struct tc_red_qopt_offload_params *p = params;
	u32 min, max;
	u64 prob;

	printk(KERN_WARNING "red_replace\n");
	mlxsw_sp_qdisc->classes[0].tclass_num = tclass_num;
	mlxsw_sp_qdisc->classes[0].prio_bitmap = prio_bitmap;

	/* calculate probability in percentage */
	prob = p->probability;
	prob *= 100;
	prob = DIV_ROUND_UP(prob, 1 << 16);
	prob = DIV_ROUND_UP(prob, 1 << 16);
	min = mlxsw_sp_bytes_cells(mlxsw_sp, p->min);
	max = mlxsw_sp_bytes_cells(mlxsw_sp, p->max);
	return mlxsw_sp_tclass_congestion_enable(mlxsw_sp_port, tclass_num,
						 min, max, prob,
						 !p->is_nodrop, p->is_ecn);
}

static void
mlxsw_sp_qdisc_leaf_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      struct gnet_stats_queue *qstats)
{
	u64 backlog;

	backlog = mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
				       mlxsw_sp_qdisc->stats_base.backlog);
	qstats->backlog -= backlog;
	mlxsw_sp_qdisc->stats_base.backlog = 0;
}

static void
mlxsw_sp_qdisc_red_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     void *params)
{
	struct tc_red_qopt_offload_params *p = params;

	mlxsw_sp_qdisc_leaf_unoffload(mlxsw_sp_port, mlxsw_sp_qdisc, p->qstats);
}

static int
mlxsw_sp_qdisc_get_red_xstats(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      void *xstats_ptr)
{
	struct red_stats *xstats_base = &mlxsw_sp_qdisc->xstats_base.red;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	struct mlxsw_sp_port_xstats *xstats;
	struct red_stats *res = xstats_ptr;
	int early_drops, pdrops;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;

	early_drops = xstats->wred_drop[tclass_num] - xstats_base->prob_drop;
	pdrops = mlxsw_sp_xstats_tail_drop(xstats, tclass_num) -
		 xstats_base->pdrop;

	res->pdrop += pdrops;
	res->prob_drop += early_drops;

	xstats_base->pdrop += pdrops;
	xstats_base->prob_drop += early_drops;
	return 0;
}

static int
mlxsw_sp_qdisc_get_red_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     struct tc_qopt_offload_stats *stats_ptr)
{
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	struct mlxsw_sp_qdisc_stats *stats_base;
	struct mlxsw_sp_port_xstats *xstats;
	u64 overlimits;

	xstats = &mlxsw_sp_port->periodic_hw_stats.xstats;
	stats_base = &mlxsw_sp_qdisc->stats_base;

	mlxsw_sp_qdisc_get_tc_stats(mlxsw_sp_port, mlxsw_sp_qdisc, stats_ptr);
	overlimits = xstats->wred_drop[tclass_num] - stats_base->overlimits;

	stats_ptr->qstats->overlimits += overlimits;
	stats_base->overlimits += overlimits;

	return 0;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_leaf_find_class(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, u32 parent)
{
	if (WARN_ON(mlxsw_sp_qdisc->num_classes != 1))
		return NULL;

	/* RED and TBF are formally classful qdiscs, but all class references,
	 * including X:0, just refer to the same one class.
	 */
	return &mlxsw_sp_qdisc->classes[0];
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
	.find_class = mlxsw_sp_qdisc_leaf_find_class,
	.num_classes = 1,
};

int mlxsw_sp_setup_tc_red(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct tc_red_qopt_offload *p)
{
	struct mlxsw_sp_qdisc_class *class;

	class = mlxsw_sp_qdisc_find_class(mlxsw_sp_port, p->parent);
	if (!class) {
		printk(KERN_WARNING "no class %#x\n", p->parent);
		return -EOPNOTSUPP;
	}

	if (p->command == TC_RED_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle, class,
					      &mlxsw_sp_qdisc_ops_red,
					      &p->set);

	if (!mlxsw_sp_qdisc_compare(class->qdisc, p->handle))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_RED_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	case TC_RED_XSTATS:
		return mlxsw_sp_qdisc_get_xstats(mlxsw_sp_port, class->qdisc,
						 p->xstats);
	case TC_RED_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, class->qdisc,
						&p->stats);
	default:
		return -EOPNOTSUPP;
	}
}

static void
mlxsw_sp_setup_tc_qdisc_leaf_clean_stats(struct mlxsw_sp_port *mlxsw_sp_port,
					 struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	u8 prio_bitmap = mlxsw_sp_qdisc->parent_class->prio_bitmap;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	u64 backlog_cells = 0;
	u64 tx_packets = 0;
	u64 tx_bytes = 0;
	u64 drops = 0;

	mlxsw_sp_qdisc_collect_tc_stats(mlxsw_sp_port, prio_bitmap, tclass_num,
					&tx_bytes, &tx_packets,
					&drops, &backlog_cells);

	mlxsw_sp_qdisc->stats_base.tx_packets = tx_packets;
	mlxsw_sp_qdisc->stats_base.tx_bytes = tx_bytes;
	mlxsw_sp_qdisc->stats_base.drops = drops;
	mlxsw_sp_qdisc->stats_base.backlog = 0;
}

static int
mlxsw_sp_qdisc_tbf_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;

	return mlxsw_sp_port_ets_maxrate_set(mlxsw_sp_port,
					     MLXSW_REG_QEEC_HR_SUBGROUP,
					     tclass_num, 0,
					     MLXSW_REG_QEEC_MAS_DIS, 0);
}

static int
mlxsw_sp_qdisc_tbf_bs(struct mlxsw_sp_port *mlxsw_sp_port,
		      u32 max_size, u8 *p_burst_size)
{
	/* TBF burst size is configured in bytes. The ASIC burst size value is
	 * ((2 ^ bs) * 512 bits. Convert the TBF bytes to 512-bit units.
	 */
	u32 bs512 = max_size / 64;
	u8 bs = fls(bs512);

	if (!bs)
		return -EINVAL;
	--bs;

	/* Demand a power of two. */
	if ((1 << bs) != bs512)
		return -EINVAL;

	if (bs < mlxsw_sp_port->mlxsw_sp->lowest_shaper_bs ||
	    bs > MLXSW_REG_QEEC_HIGHEST_SHAPER_BS)
		return -EINVAL;

	*p_burst_size = bs;
	return 0;
}

static u32
mlxsw_sp_qdisc_tbf_max_size(u8 bs)
{
	return (1U << bs) * 64;
}

static u64
mlxsw_sp_qdisc_tbf_rate_kbps(struct tc_tbf_qopt_offload_replace_params *p)
{
	/* TBF interface is in bytes/s, whereas Spectrum ASIC is configured in
	 * Kbits/s.
	 */
	return div_u64(p->rate.rate_bytes_ps, 1000) * 8;
}

static int
mlxsw_sp_qdisc_tbf_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				void *params)
{
	struct tc_tbf_qopt_offload_replace_params *p = params;
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	u64 rate_kbps = mlxsw_sp_qdisc_tbf_rate_kbps(p);
	u8 burst_size;
	int err;

	if (rate_kbps >= MLXSW_REG_QEEC_MAS_DIS) {
		dev_err(mlxsw_sp_port->mlxsw_sp->bus_info->dev,
			"spectrum: TBF: rate of %lluKbps must be below %u\n",
			rate_kbps, MLXSW_REG_QEEC_MAS_DIS);
		return -EINVAL;
	}

	err = mlxsw_sp_qdisc_tbf_bs(mlxsw_sp_port, p->max_size, &burst_size);
	if (err) {
		u8 highest_shaper_bs = MLXSW_REG_QEEC_HIGHEST_SHAPER_BS;

		dev_err(mlxsw_sp->bus_info->dev,
			"spectrum: TBF: invalid burst size of %u, must be a power of two between %u and %u",
			p->max_size,
			mlxsw_sp_qdisc_tbf_max_size(mlxsw_sp->lowest_shaper_bs),
			mlxsw_sp_qdisc_tbf_max_size(highest_shaper_bs));
		return -EINVAL;
	}

	return 0;
}

static int
mlxsw_sp_qdisc_tbf_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			   void *params)
{
	u8 prio_bitmap = mlxsw_sp_qdisc->parent_class->prio_bitmap;
	u8 tclass_num = mlxsw_sp_qdisc->parent_class->tclass_num;
	struct tc_tbf_qopt_offload_replace_params *p = params;
	u64 rate_kbps = mlxsw_sp_qdisc_tbf_rate_kbps(p);
	u8 burst_size;
	int err;

	mlxsw_sp_qdisc->classes[0].tclass_num = tclass_num;
	mlxsw_sp_qdisc->classes[0].prio_bitmap = prio_bitmap;

	err = mlxsw_sp_qdisc_tbf_bs(mlxsw_sp_port, p->max_size, &burst_size);
	if (WARN_ON_ONCE(err))
		/* check_params above was supposed to reject this value. */
		return -EINVAL;

	/* Configure subgroup shaper, so that both UC and MC traffic is subject
	 * to shaping. That is unlike RED, however UC queue lengths are going to
	 * be different than MC ones due to different pool and quota
	 * configurations, so the configuration is not applicable. For shaper on
	 * the other hand, subjecting the overall stream to the configured
	 * shaper makes sense. Also note that that is what we do for
	 * ieee_setmaxrate().
	 */
	return mlxsw_sp_port_ets_maxrate_set(mlxsw_sp_port,
					     MLXSW_REG_QEEC_HR_SUBGROUP,
					     tclass_num, 0,
					     rate_kbps, burst_size);
}

static void
mlxsw_sp_qdisc_tbf_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     void *params)
{
	struct tc_tbf_qopt_offload_replace_params *p = params;

	mlxsw_sp_qdisc_leaf_unoffload(mlxsw_sp_port, mlxsw_sp_qdisc, p->qstats);
}

static int
mlxsw_sp_qdisc_get_tbf_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     struct tc_qopt_offload_stats *stats_ptr)
{
	mlxsw_sp_qdisc_get_tc_stats(mlxsw_sp_port, mlxsw_sp_qdisc,
				    stats_ptr);
	return 0;
}

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_tbf = {
	.type = MLXSW_SP_QDISC_TBF,
	.check_params = mlxsw_sp_qdisc_tbf_check_params,
	.replace = mlxsw_sp_qdisc_tbf_replace,
	.unoffload = mlxsw_sp_qdisc_tbf_unoffload,
	.destroy = mlxsw_sp_qdisc_tbf_destroy,
	.get_stats = mlxsw_sp_qdisc_get_tbf_stats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_leaf_clean_stats,
	.find_class = mlxsw_sp_qdisc_leaf_find_class,
	.num_classes = 1,
};

int mlxsw_sp_setup_tc_tbf(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct tc_tbf_qopt_offload *p)
{
	struct mlxsw_sp_qdisc_class *class;

	class = mlxsw_sp_qdisc_find_class(mlxsw_sp_port, p->parent);
	if (!class)
		return -EOPNOTSUPP;

	if (p->command == TC_TBF_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle, class,
					      &mlxsw_sp_qdisc_ops_tbf,
					      &p->replace_params);

	if (!mlxsw_sp_qdisc_compare(class->qdisc, p->handle))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_TBF_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	case TC_TBF_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, class->qdisc,
						&p->stats);
	default:
		return -EOPNOTSUPP;
	}
}

static int
mlxsw_sp_qdisc_fifo_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				 void *params)
{
	return 0;
}

static int
mlxsw_sp_qdisc_fifo_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    void *params)
{
	return 0;
}

static int
mlxsw_sp_qdisc_get_fifo_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      struct tc_qopt_offload_stats *stats_ptr)
{
	mlxsw_sp_qdisc_get_tc_stats(mlxsw_sp_port, mlxsw_sp_qdisc,
				    stats_ptr);
	return 0;
}

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_fifo = {
	.type = MLXSW_SP_QDISC_FIFO,
	.check_params = mlxsw_sp_qdisc_fifo_check_params,
	.replace = mlxsw_sp_qdisc_fifo_replace,
	.get_stats = mlxsw_sp_qdisc_get_fifo_stats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_leaf_clean_stats,
	.num_classes = 0,
};

int mlxsw_sp_setup_tc_fifo(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct tc_fifo_qopt_offload *p)
{
	struct mlxsw_sp_qdisc_state *qdisc_state = mlxsw_sp_port->qdisc;
	struct mlxsw_sp_qdisc_class *class;
	int tclass, child_index;
	u32 parent_handle;

	/* Invisible FIFOs are tracked in future_handle and future_fifos. Make
	 * sure that not more than one qdisc is created for a port at a time.
	 * RTNL is a simple proxy for that.
	 */
	// xxx all the class management stuff needs this property. Just
	// introduce a qdisc lock.
	ASSERT_RTNL();

	class = mlxsw_sp_qdisc_find_class(mlxsw_sp_port, p->parent);
	if (!class && p->handle == TC_H_UNSPEC) {
		parent_handle = TC_H_MAJ(p->parent);
		if (parent_handle != qdisc_state->future_handle) {
			/* This notifications is for a different Qdisc than
			 * previously. Wipe the future cache.
			 */
			memset(qdisc_state->future_fifos, 0,
			       sizeof(qdisc_state->future_fifos));
			qdisc_state->future_handle = parent_handle;
		}

		child_index = TC_H_MIN(p->parent);
		tclass = MLXSW_SP_PRIO_CHILD_TO_TCLASS(child_index);
		if (tclass < IEEE_8021QAZ_MAX_TCS) {
			if (p->command == TC_FIFO_REPLACE)
				qdisc_state->future_fifos[tclass] = true;
			else if (p->command == TC_FIFO_DESTROY)
				qdisc_state->future_fifos[tclass] = false;
		}
	}
	if (!class)
		return -EOPNOTSUPP;

	if (p->command == TC_FIFO_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle, class,
					      &mlxsw_sp_qdisc_ops_fifo, NULL);

	if (!mlxsw_sp_qdisc_compare(class->qdisc, p->handle))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_FIFO_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	case TC_FIFO_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, class->qdisc,
						&p->stats);
	case TC_FIFO_REPLACE: /* Handled above. */
		break;
	}

	return -EOPNOTSUPP;
}

static int
__mlxsw_sp_qdisc_ets_destroy(struct mlxsw_sp_port *mlxsw_sp_port)
{
	//struct mlxsw_sp_qdisc_state *qdisc_state = mlxsw_sp_port->qdisc;
	int i;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		mlxsw_sp_port_prio_tc_set(mlxsw_sp_port, i,
					  MLXSW_SP_PORT_DEFAULT_TCLASS);
		mlxsw_sp_port_ets_set(mlxsw_sp_port,
				      MLXSW_REG_QEEC_HR_SUBGROUP,
				      i, 0, false, 0);
		/* xxx this should have already been done, no? What's the path to trigger this?
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port,
				       &qdisc_state->tclass_qdiscs[i]);
		qdisc_state->tclass_qdiscs[i].prio_bitmap = 0;
		*/
	}

	return 0;
}

static int
mlxsw_sp_qdisc_prio_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	return __mlxsw_sp_qdisc_ets_destroy(mlxsw_sp_port);
}

static int
__mlxsw_sp_qdisc_ets_check_params(unsigned int nbands)
{
	if (nbands > IEEE_8021QAZ_MAX_TCS)
		return -EOPNOTSUPP;

	return 0;
}

static int
mlxsw_sp_qdisc_prio_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				 void *params)
{
	struct tc_prio_qopt_offload_params *p = params;

	return __mlxsw_sp_qdisc_ets_check_params(p->bands);
}

static int
__mlxsw_sp_qdisc_ets_replace(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     u32 handle, unsigned int nbands,
			     const unsigned int *quanta,
			     const unsigned int *weights,
			     const u8 *priomap)
{
	struct mlxsw_sp_qdisc_state *qdisc_state = mlxsw_sp_port->qdisc;
	int i, band, backlog;
	int err;

	for (band = 0; band < nbands; band++) {
		struct mlxsw_sp_qdisc_class *class = &mlxsw_sp_qdisc->classes[band];
		int tclass = MLXSW_SP_PRIO_BAND_TO_TCLASS(band);
		u8 old_priomap = class->prio_bitmap;
		u8 new_priomap = 0;

		class->tclass_num = tclass;
		class->prio_bitmap = 0;
		err = mlxsw_sp_port_ets_set(mlxsw_sp_port,
					    MLXSW_REG_QEEC_HR_SUBGROUP,
					    tclass, 0, !!quanta[band],
					    weights[band]);
		if (err)
			return err;

		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
			if (priomap[i] == band) {
				new_priomap |= BIT(i);
				if (BIT(i) & old_priomap)
					continue;
				err = mlxsw_sp_port_prio_tc_set(mlxsw_sp_port,
								i, tclass);
				if (err)
					return err;
			}
		}

		mlxsw_sp_qdisc_subtree_set_priomap(class, new_priomap);

		if (old_priomap != new_priomap &&
		    class->qdisc && class->qdisc->ops->clean_stats) {
			backlog = class->qdisc->stats_base.backlog;
			class->qdisc->ops->clean_stats(mlxsw_sp_port, class->qdisc);
			class->qdisc->stats_base.backlog = backlog;
		}

		if (handle == qdisc_state->future_handle &&
		    qdisc_state->future_fifos[tclass]) {
			err = mlxsw_sp_qdisc_replace(mlxsw_sp_port, TC_H_UNSPEC,
						     class, &mlxsw_sp_qdisc_ops_fifo, NULL);
			if (err)
				return err;
		}
	}
	for (; band < IEEE_8021QAZ_MAX_TCS; band++) {
		struct mlxsw_sp_qdisc_class *class = &mlxsw_sp_qdisc->classes[band];
		int tclass = MLXSW_SP_PRIO_BAND_TO_TCLASS(band);

		class->tclass_num = tclass;
		class->prio_bitmap = 0;
		// xxx either this is not necessary because we get the
		// notifications, or it is not enough because it needs to be
		// done for the whole subtree
		if (class->qdisc)
			mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
		mlxsw_sp_port_ets_set(mlxsw_sp_port,
				      MLXSW_REG_QEEC_HR_SUBGROUP,
				      tclass, 0, false, 0);
	}

	qdisc_state->future_handle = TC_H_UNSPEC;
	memset(qdisc_state->future_fifos, 0, sizeof(qdisc_state->future_fifos));
	return 0;
}

static int
mlxsw_sp_qdisc_prio_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
			    struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			    void *params)
{
	struct tc_prio_qopt_offload_params *p = params;
	unsigned int zeroes[TCQ_ETS_MAX_BANDS] = {0};

	return __mlxsw_sp_qdisc_ets_replace(mlxsw_sp_port, mlxsw_sp_qdisc, handle, p->bands,
					    zeroes, zeroes, p->priomap);
}

static void
__mlxsw_sp_qdisc_ets_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			       struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			       struct gnet_stats_queue *qstats)
{
	u64 backlog;

	backlog = mlxsw_sp_cells_bytes(mlxsw_sp_port->mlxsw_sp,
				       mlxsw_sp_qdisc->stats_base.backlog);
	qstats->backlog -= backlog;
}

static void
mlxsw_sp_qdisc_prio_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      void *params)
{
	struct tc_prio_qopt_offload_params *p = params;

	__mlxsw_sp_qdisc_ets_unoffload(mlxsw_sp_port, mlxsw_sp_qdisc,
				       p->qstats);
}

static int
mlxsw_sp_qdisc_get_prio_stats(struct mlxsw_sp_port *mlxsw_sp_port,
			      struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			      struct tc_qopt_offload_stats *stats_ptr)
{
	u64 tx_packets = 0;
	u64 tx_bytes = 0;
	u64 backlog = 0;
	u64 drops = 0;
	int i;

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		struct mlxsw_sp_qdisc_class *class = &mlxsw_sp_qdisc->classes[i];
		u8 prio_bitmap = class->prio_bitmap;
		u8 tclass_num = class->tclass_num;

		mlxsw_sp_qdisc_collect_tc_stats(mlxsw_sp_port, prio_bitmap, tclass_num,
						&tx_bytes, &tx_packets,
						&drops, &backlog);
	}

	mlxsw_sp_qdisc_update_stats(mlxsw_sp_port->mlxsw_sp, mlxsw_sp_qdisc,
				    tx_bytes, tx_packets, drops, backlog,
				    stats_ptr);
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
		stats_base->drops += mlxsw_sp_xstats_tail_drop(xstats, i);
		stats_base->drops += xstats->wred_drop[i];
	}

	mlxsw_sp_qdisc->stats_base.backlog = 0;
}

static struct mlxsw_sp_qdisc_class *
mlxsw_sp_qdisc_prio_find_class(struct mlxsw_sp_qdisc *mlxsw_sp_qdisc, u32 parent)
{
	int child_index = TC_H_MIN(parent);

	if (child_index < 1 || child_index > mlxsw_sp_qdisc->num_classes)
		return NULL;
	return &mlxsw_sp_qdisc->classes[child_index - 1];
}

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_prio = {
	.type = MLXSW_SP_QDISC_PRIO,
	.check_params = mlxsw_sp_qdisc_prio_check_params,
	.replace = mlxsw_sp_qdisc_prio_replace,
	.unoffload = mlxsw_sp_qdisc_prio_unoffload,
	.destroy = mlxsw_sp_qdisc_prio_destroy,
	.get_stats = mlxsw_sp_qdisc_get_prio_stats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_prio_clean_stats,
	.find_class = mlxsw_sp_qdisc_prio_find_class,
	.num_classes = 8,
};

static int
mlxsw_sp_qdisc_ets_check_params(struct mlxsw_sp_port *mlxsw_sp_port,
				void *params)
{
	struct tc_ets_qopt_offload_replace_params *p = params;

	return __mlxsw_sp_qdisc_ets_check_params(p->bands);
}

static int
mlxsw_sp_qdisc_ets_replace(struct mlxsw_sp_port *mlxsw_sp_port, u32 handle,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			   void *params)
{
	struct tc_ets_qopt_offload_replace_params *p = params;

	return __mlxsw_sp_qdisc_ets_replace(mlxsw_sp_port, mlxsw_sp_qdisc, handle, p->bands,
					    p->quanta, p->weights, p->priomap);
}

static void
mlxsw_sp_qdisc_ets_unoffload(struct mlxsw_sp_port *mlxsw_sp_port,
			     struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			     void *params)
{
	struct tc_ets_qopt_offload_replace_params *p = params;

	__mlxsw_sp_qdisc_ets_unoffload(mlxsw_sp_port, mlxsw_sp_qdisc,
				       p->qstats);
}

static int
mlxsw_sp_qdisc_ets_destroy(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc)
{
	return __mlxsw_sp_qdisc_ets_destroy(mlxsw_sp_port);
}

static struct mlxsw_sp_qdisc_ops mlxsw_sp_qdisc_ops_ets = {
	.type = MLXSW_SP_QDISC_ETS,
	.check_params = mlxsw_sp_qdisc_ets_check_params,
	.replace = mlxsw_sp_qdisc_ets_replace,
	.unoffload = mlxsw_sp_qdisc_ets_unoffload,
	.destroy = mlxsw_sp_qdisc_ets_destroy,
	.get_stats = mlxsw_sp_qdisc_get_prio_stats,
	.clean_stats = mlxsw_sp_setup_tc_qdisc_prio_clean_stats,
	.find_class = mlxsw_sp_qdisc_prio_find_class,
	.num_classes = 8,
};

/* Linux allows linking of Qdiscs to arbitrary classes (so long as the resulting
 * graph is free of cycles). These operations do not change the parent handle
 * though, which means it can be incomplete (if there is more than one class
 * where the Qdisc in question is grafted) or outright wrong (if the Qdisc was
 * linked to a different class and then removed from the original class).
 *
 * E.g. consider this sequence of operations:
 *
 *  # tc qdisc add dev swp1 root handle 1: prio
 *  # tc qdisc add dev swp1 parent 1:3 handle 13: red limit 1000000 avpkt 10000
 *  RED: set bandwidth to 10Mbit
 *  # tc qdisc link dev swp1 handle 13: parent 1:2
 *
 * At this point, both 1:2 and 1:3 have the same RED Qdisc instance as their
 * child. But RED will still only claim that 1:3 is its parent. If it's removed
 * from that band, its only parent will be 1:2, but it will continue to claim
 * that it is in fact 1:3.
 *
 * The notification for child Qdisc replace (e.g. TC_RED_REPLACE) comes before
 * the notification for parent graft (e.g. TC_PRIO_GRAFT). We take the replace
 * notification to offload the child Qdisc, based on its parent handle, and use
 * the graft operation to validate that the class where the child is actually
 * grafted corresponds to the parent handle. If the two don't match, we
 * unoffload the child.
 */
static int
__mlxsw_sp_qdisc_ets_graft(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			   u8 band, u32 child_handle)
{
	struct mlxsw_sp_qdisc_class *class;
	struct mlxsw_sp_qdisc *old_qdisc;

	if (band < mlxsw_sp_qdisc->num_classes) {
		class = &mlxsw_sp_qdisc->classes[band];
		if (class->qdisc && class->qdisc->handle == child_handle)
			return 0;
	}

	if (!child_handle) {
		/* This is an invisible FIFO replacing the original Qdisc.
		 * Ignore it--the original Qdisc's destroy will follow.
		 */
		// xxx why is this necessary?
		return 0;
	}

	/* See if the grafted qdisc is already offloaded on any tclass. If so,
	 * unoffload it.
	 */
	old_qdisc = mlxsw_sp_qdisc_find_by_handle(mlxsw_sp_port, child_handle);
	if (old_qdisc)
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, old_qdisc->parent_class);

	class = mlxsw_sp_qdisc_prio_find_class(mlxsw_sp_qdisc, band);
	if (!WARN_ON(!class))
		mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);

	return -EOPNOTSUPP;
}

static int
mlxsw_sp_qdisc_prio_graft(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct mlxsw_sp_qdisc *mlxsw_sp_qdisc,
			  struct tc_prio_qopt_offload_graft_params *p)
{
	return __mlxsw_sp_qdisc_ets_graft(mlxsw_sp_port, mlxsw_sp_qdisc,
					  p->band, p->child_handle);
}

int mlxsw_sp_setup_tc_prio(struct mlxsw_sp_port *mlxsw_sp_port,
			   struct tc_prio_qopt_offload *p)
{
	struct mlxsw_sp_qdisc_class *class;

	class = mlxsw_sp_qdisc_find_class(mlxsw_sp_port, p->parent);
	if (!class)
		return -EOPNOTSUPP;

	if (p->command == TC_PRIO_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle, class,
					      &mlxsw_sp_qdisc_ops_prio,
					      &p->replace_params);

	if (!mlxsw_sp_qdisc_compare(class->qdisc, p->handle))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_PRIO_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	case TC_PRIO_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, class->qdisc,
						&p->stats);
	case TC_PRIO_GRAFT:
		return mlxsw_sp_qdisc_prio_graft(mlxsw_sp_port, class->qdisc,
						 &p->graft_params);
	default:
		return -EOPNOTSUPP;
	}
}

int mlxsw_sp_setup_tc_ets(struct mlxsw_sp_port *mlxsw_sp_port,
			  struct tc_ets_qopt_offload *p)
{
	struct mlxsw_sp_qdisc_class *class;

	class = mlxsw_sp_qdisc_find_class(mlxsw_sp_port, p->parent);
	if (!class)
		return -EOPNOTSUPP;

	if (p->command == TC_ETS_REPLACE)
		return mlxsw_sp_qdisc_replace(mlxsw_sp_port, p->handle, class,
					      &mlxsw_sp_qdisc_ops_ets,
					      &p->replace_params);

	if (!mlxsw_sp_qdisc_compare(class->qdisc, p->handle))
		return -EOPNOTSUPP;

	switch (p->command) {
	case TC_ETS_DESTROY:
		return mlxsw_sp_qdisc_destroy(mlxsw_sp_port, class);
	case TC_ETS_STATS:
		return mlxsw_sp_qdisc_get_stats(mlxsw_sp_port, class->qdisc,
						&p->stats);
	case TC_ETS_GRAFT:
		return __mlxsw_sp_qdisc_ets_graft(mlxsw_sp_port, class->qdisc,
						  p->graft_params.band,
						  p->graft_params.child_handle);
	default:
		return -EOPNOTSUPP;
	}
}

struct mlxsw_sp_qevent_block {
	struct list_head binding_list;
	struct list_head mall_entry_list;
	struct mlxsw_sp *mlxsw_sp;
};

// xxx really similar to flow_block_binding. Probably can be united.
struct mlxsw_sp_qevent_binding {
	struct list_head list;
	struct mlxsw_sp_port *mlxsw_sp_port;
	int tclass_num;
	enum mlxsw_sp_span_trigger span_trigger;
};

static LIST_HEAD(mlxsw_sp_qevent_block_cb_list);

struct mlxsw_sp_qevent_ops {
	int (*qevent_block_cb)(struct mlxsw_sp_qevent_block *qevent_block,
			       enum tc_setup_type type, void *type_data);
};

static int
mlxsw_sp_qevent_mirror_configure(struct mlxsw_sp *mlxsw_sp,
				 struct mlxsw_sp_mall_entry *mall_entry,
				 struct mlxsw_sp_qevent_binding *qevent_binding)
{
	struct mlxsw_sp_port *mlxsw_sp_port = qevent_binding->mlxsw_sp_port;
	struct mlxsw_sp_span_trigger_parms trigger_parms = {};
	struct mlxsw_sp_span_agent_parms agent_parms = {
		.to_dev = mall_entry->mirror.to_dev,
	};
	int span_id;
	int err;

	err = mlxsw_sp_span_agent_get(mlxsw_sp, &span_id, &agent_parms);
	if (err)
		return err;

	err = mlxsw_sp_span_analyzed_port_get(mlxsw_sp_port, false);
	if (err)
		goto err_analyzed_port_get;

	trigger_parms.span_id = span_id;
	err = mlxsw_sp_span_agent_bind(mlxsw_sp, qevent_binding->span_trigger,
				       mlxsw_sp_port, &trigger_parms);
	if (err)
		goto err_agent_bind;

	err = mlxsw_sp_span_trigger_enable(mlxsw_sp_port,
					   qevent_binding->span_trigger,
					   qevent_binding->tclass_num);
	if (err)
		goto err_trigger_enable;

	printk(KERN_WARNING "configured mirror\n");
	mall_entry->mirror.span_id = span_id;
	return 0;

err_trigger_enable:
	mlxsw_sp_span_agent_unbind(mlxsw_sp, qevent_binding->span_trigger,
				   mlxsw_sp_port, &trigger_parms);
err_agent_bind:
	mlxsw_sp_span_analyzed_port_put(mlxsw_sp_port, false);
err_analyzed_port_get:
	mlxsw_sp_span_agent_put(mlxsw_sp, span_id);
	return err;
}

static void
mlxsw_sp_qevent_mirror_deconfigure(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_mall_entry *mall_entry,
				struct mlxsw_sp_qevent_binding *qevent_binding)
{
	struct mlxsw_sp_port *mlxsw_sp_port = qevent_binding->mlxsw_sp_port;
	struct mlxsw_sp_span_trigger_parms trigger_parms = {
		.span_id = mall_entry->mirror.span_id,
	};

	printk(KERN_WARNING "deconfigured mirror\n");
	mlxsw_sp_span_trigger_disable(mlxsw_sp_port,
				      qevent_binding->span_trigger,
				      qevent_binding->tclass_num);
	mlxsw_sp_span_agent_unbind(mlxsw_sp, qevent_binding->span_trigger,
				   mlxsw_sp_port, &trigger_parms);
	mlxsw_sp_span_analyzed_port_put(mlxsw_sp_port, false);
	mlxsw_sp_span_agent_put(mlxsw_sp, mall_entry->mirror.span_id);
}

static int
mlxsw_sp_qevent_entry_configure(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_mall_entry *mall_entry,
				struct mlxsw_sp_qevent_binding *qevent_binding)
{
	printk(KERN_WARNING "%d @ %s TC%d: configure\n",
	       qevent_binding->span_trigger,
	       qevent_binding->mlxsw_sp_port->dev->name,
	       qevent_binding->tclass_num);

	switch (mall_entry->type) {
	case MLXSW_SP_MALL_ACTION_TYPE_MIRROR:
		return mlxsw_sp_qevent_mirror_configure(mlxsw_sp, mall_entry,
							qevent_binding);

	default:
		// xxx WARN_ON(1), because this should have been validated away
		return -EOPNOTSUPP;
	}
}

static void
mlxsw_sp_qevent_entry_deconfigure(struct mlxsw_sp *mlxsw_sp,
				  struct mlxsw_sp_mall_entry *mall_entry,
				  struct mlxsw_sp_qevent_binding *qevent_binding)
{
	printk(KERN_WARNING "%d @ %s TC%d: deconfigure\n",
	       qevent_binding->span_trigger,
	       qevent_binding->mlxsw_sp_port->dev->name,
	       qevent_binding->tclass_num);

	switch (mall_entry->type) {
	case MLXSW_SP_MALL_ACTION_TYPE_MIRROR:
		return mlxsw_sp_qevent_mirror_deconfigure(mlxsw_sp, mall_entry,
							  qevent_binding);
	default:
		WARN_ON(1);
		return;
	}
}

static int
mlxsw_sp_qevent_binding_configure(struct mlxsw_sp_qevent_block *qevent_block,
				  struct mlxsw_sp_qevent_binding *qevent_binding)
{
	struct mlxsw_sp_mall_entry *mall_entry;
	int err;

	list_for_each_entry(mall_entry, &qevent_block->mall_entry_list, list) {
		err = mlxsw_sp_qevent_entry_configure(qevent_block->mlxsw_sp,
						      mall_entry, qevent_binding);
		if (err)
			goto err_entry_configure;
	}

	return 0;

err_entry_configure:
	list_for_each_entry_continue_reverse(mall_entry,
					     &qevent_block->mall_entry_list,
					     list)
		mlxsw_sp_qevent_entry_deconfigure(qevent_block->mlxsw_sp,
						  mall_entry, qevent_binding);
	return err;
}

static void
mlxsw_sp_qevent_binding_deconfigure(struct mlxsw_sp_qevent_block *qevent_block,
				    struct mlxsw_sp_qevent_binding *qevent_binding)
{
	struct mlxsw_sp_mall_entry *mall_entry;

	list_for_each_entry(mall_entry, &qevent_block->mall_entry_list, list)
		mlxsw_sp_qevent_entry_deconfigure(qevent_block->mlxsw_sp,
						  mall_entry, qevent_binding);
}

static int
mlxsw_sp_qevent_block_configure(struct mlxsw_sp_qevent_block *qevent_block)
{
	struct mlxsw_sp_qevent_binding *qevent_binding;
	int err;

	list_for_each_entry(qevent_binding, &qevent_block->binding_list, list) {
		err = mlxsw_sp_qevent_binding_configure(qevent_block,
							qevent_binding);
		if (err)
			goto err_binding_configure;
	}

	return 0;

err_binding_configure:
	list_for_each_entry_continue_reverse(qevent_binding,
					     &qevent_block->binding_list,
					     list)
		mlxsw_sp_qevent_binding_deconfigure(qevent_block,
						    qevent_binding);
	return err;
}

static void
mlxsw_sp_qevent_block_deconfigure(struct mlxsw_sp_qevent_block *qevent_block)
{
	struct mlxsw_sp_qevent_binding *qevent_binding;

	list_for_each_entry(qevent_binding, &qevent_block->binding_list, list)
		mlxsw_sp_qevent_binding_deconfigure(qevent_block,
						    qevent_binding);
}

static struct mlxsw_sp_mall_entry *
mlxsw_sp_qevent_mall_entry_find(struct mlxsw_sp_qevent_block *block,
				unsigned long cookie)
{
	struct mlxsw_sp_mall_entry *mall_entry;

	list_for_each_entry(mall_entry, &block->mall_entry_list, list)
		if (mall_entry->cookie == cookie)
			return mall_entry;

	return NULL;
}

static int mlxsw_sp1_qevent_block_cb(struct mlxsw_sp_qevent_block *qevent_block,
				     enum tc_setup_type type, void *type_data)
{
	return -EOPNOTSUPP;
}

const struct mlxsw_sp_qevent_ops mlxsw_sp1_qevent_ops = {
	.qevent_block_cb = mlxsw_sp1_qevent_block_cb,
};

static int
mlxsw_sp2_qevent_mall_replace(struct mlxsw_sp *mlxsw_sp,
			      struct mlxsw_sp_qevent_block *qevent_block,
			      struct tc_cls_matchall_offload *f)
{
	struct mlxsw_sp_mall_entry *mall_entry;
	struct flow_action_entry *act;
	int err;

	/* It should not currently be possible to replace a matchall rule. So
	 * this must be a new rule.
	 */
	if (!list_empty(&qevent_block->mall_entry_list)) {
		NL_SET_ERR_MSG(f->common.extack, "At most one filter supported");
		return -EOPNOTSUPP;
	}
	if (f->rule->action.num_entries != 1) {
		NL_SET_ERR_MSG(f->common.extack, "Only singular actions supported");
		return -EOPNOTSUPP;
	}
	if (f->common.chain_index) {
		NL_SET_ERR_MSG(f->common.extack, "Only chain 0 is supported");
		return -EOPNOTSUPP;
	}
	if (f->common.protocol != htons(ETH_P_ALL)) {
		NL_SET_ERR_MSG(f->common.extack, "Protocol matching not supported");
		return -EOPNOTSUPP;
	}

	mall_entry = kzalloc(sizeof(*mall_entry), GFP_KERNEL);
	if (!mall_entry)
		return -ENOMEM;
	mall_entry->cookie = f->cookie;

	act = &f->rule->action.entries[0];
	if (act->id == FLOW_ACTION_MIRRED) {
		mall_entry->type = MLXSW_SP_MALL_ACTION_TYPE_MIRROR;
		mall_entry->mirror.to_dev = act->dev;
	} else if (act->id == FLOW_ACTION_TRAP) {
		mall_entry->type = MLXSW_SP_MALL_ACTION_TYPE_TRAP;
	} else {
		NL_SET_ERR_MSG(f->common.extack, "Unsupported action");
		err = -EOPNOTSUPP;
		goto err_unsupported_action;
	}

	list_add_tail(&mall_entry->list, &qevent_block->mall_entry_list);

	err = mlxsw_sp_qevent_block_configure(qevent_block);
	if (err)
		goto err_block_configure;

	return 0;

err_block_configure:
	list_del(&mall_entry->list);
err_unsupported_action:
	kfree(mall_entry);
	return err;
}

static void
mlxsw_sp2_qevent_mall_destroy(struct mlxsw_sp_qevent_block *qevent_block,
			      struct tc_cls_matchall_offload *f)
{
	struct mlxsw_sp_mall_entry *mall_entry;

	mall_entry = mlxsw_sp_qevent_mall_entry_find(qevent_block, f->cookie);
	if (!mall_entry)
		return;

	mlxsw_sp_qevent_block_deconfigure(qevent_block);

	list_del(&mall_entry->list);
	kfree(mall_entry);
}

static int
mlxsw_sp2_qevent_block_mall_cb(struct mlxsw_sp_qevent_block *qevent_block,
			       struct tc_cls_matchall_offload *f)
{
	struct mlxsw_sp *mlxsw_sp = qevent_block->mlxsw_sp;

	switch (f->command) {
	case TC_CLSMATCHALL_REPLACE:
		return mlxsw_sp2_qevent_mall_replace(mlxsw_sp, qevent_block, f);
	case TC_CLSMATCHALL_DESTROY:
		mlxsw_sp2_qevent_mall_destroy(qevent_block, f);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

static int mlxsw_sp2_qevent_block_cb(struct mlxsw_sp_qevent_block *qevent_block,
				     enum tc_setup_type type, void *type_data)
{
	switch (type) {
	case TC_SETUP_CLSMATCHALL:
		return mlxsw_sp2_qevent_block_mall_cb(qevent_block, type_data);
	default:
		return -EOPNOTSUPP;
	}
}

const struct mlxsw_sp_qevent_ops mlxsw_sp2_qevent_ops = {
	.qevent_block_cb = mlxsw_sp2_qevent_block_cb,
};

static int mlxsw_sp_qevent_block_cb(enum tc_setup_type type,
				    void *type_data, void *cb_priv)
{
	struct mlxsw_sp_qevent_block *qevent_block = cb_priv;
	struct mlxsw_sp *mlxsw_sp = qevent_block->mlxsw_sp;

	return mlxsw_sp->qevent_ops->qevent_block_cb(qevent_block,
						     type, type_data);
}

static struct mlxsw_sp_qevent_block *
mlxsw_sp_qevent_block_create(struct mlxsw_sp *mlxsw_sp, struct net *net)
{
	struct mlxsw_sp_qevent_block *qevent_block;

	qevent_block = kzalloc(sizeof(*qevent_block), GFP_KERNEL);
	if (!qevent_block)
		return NULL;

	INIT_LIST_HEAD(&qevent_block->binding_list);
	INIT_LIST_HEAD(&qevent_block->mall_entry_list);
	qevent_block->mlxsw_sp = mlxsw_sp;
	return qevent_block;
}

static void
mlxsw_sp_qevent_block_destroy(struct mlxsw_sp_qevent_block *qevent_block)
{
	WARN_ON(!list_empty(&qevent_block->binding_list));
	WARN_ON(!list_empty(&qevent_block->mall_entry_list));
	kfree(qevent_block);
}

static void mlxsw_sp_qevent_block_release(void *cb_priv)
{
	struct mlxsw_sp_qevent_block *qevent_block = cb_priv;

	mlxsw_sp_qevent_block_destroy(qevent_block);
}

static struct mlxsw_sp_qevent_binding *
mlxsw_sp_qevent_binding_create(struct mlxsw_sp_port *mlxsw_sp_port,
			       int tclass_num,
			       enum mlxsw_sp_span_trigger span_trigger)
{
	struct mlxsw_sp_qevent_binding *binding;

	binding = kzalloc(sizeof(*binding), GFP_KERNEL);
	if (!binding)
		return ERR_PTR(-ENOMEM);

	binding->mlxsw_sp_port = mlxsw_sp_port;
	binding->tclass_num = tclass_num;
	binding->span_trigger = span_trigger;
	return binding;
}

static void
mlxsw_sp_qevent_binding_destroy(struct mlxsw_sp_qevent_binding *binding)
{
	kfree(binding);
}

static struct mlxsw_sp_qevent_binding *
mlxsw_sp_qevent_binding_lookup(struct mlxsw_sp_qevent_block *block,
			       struct mlxsw_sp_port *mlxsw_sp_port,
			       int tclass_num,
			       enum mlxsw_sp_span_trigger span_trigger)
{
	struct mlxsw_sp_qevent_binding *qevent_binding;

	list_for_each_entry(qevent_binding, &block->binding_list, list)
		if (qevent_binding->mlxsw_sp_port == mlxsw_sp_port &&
		    qevent_binding->tclass_num == tclass_num &&
		    qevent_binding->span_trigger == span_trigger)
			return qevent_binding;
	return NULL;
}

static int
mlxsw_sp_setup_tc_block_qevent_bind(struct mlxsw_sp_port *mlxsw_sp_port,
				    struct flow_block_offload *f,
				    enum mlxsw_sp_span_trigger span_trigger)
{
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	struct mlxsw_sp_qevent_binding *qevent_binding;
	struct mlxsw_sp_qevent_block *qevent_block;
	struct flow_block_cb *block_cb;
	struct mlxsw_sp_qdisc *qdisc;
	bool register_block = false;
	int err;

	block_cb = flow_block_cb_lookup(f->block, mlxsw_sp_qevent_block_cb,
					mlxsw_sp);
	if (!block_cb) {
		qevent_block = mlxsw_sp_qevent_block_create(mlxsw_sp, f->net);
		if (!qevent_block)
			return -ENOMEM;
		block_cb = flow_block_cb_alloc(mlxsw_sp_qevent_block_cb,
					    mlxsw_sp, qevent_block,
					    mlxsw_sp_qevent_block_release);
		if (IS_ERR(block_cb)) {
			mlxsw_sp_qevent_block_destroy(qevent_block);
			return PTR_ERR(block_cb);
		}
		register_block = true;
	} else {
		qevent_block = flow_block_cb_priv(block_cb);
	}
	flow_block_cb_incref(block_cb);

	qdisc = mlxsw_sp_qdisc_find_by_handle(mlxsw_sp_port, f->sch->handle);
	if (!qdisc) {
		NL_SET_ERR_MSG(f->extack, "Qdisc not offloaded");
		err = -ENOENT;
		goto err_find_qdisc;
	}

	if (WARN_ON(mlxsw_sp_qevent_binding_lookup(qevent_block, mlxsw_sp_port,
						   qdisc->parent_class->tclass_num,
						   span_trigger))) {
		err = -EEXIST;
		goto err_binding_exists;
	}

	qevent_binding = mlxsw_sp_qevent_binding_create(mlxsw_sp_port,
							qdisc->parent_class->tclass_num,
							span_trigger);
	if (IS_ERR(qevent_binding)) {
		err = PTR_ERR(qevent_binding);
		goto err_binding_create;
	}

	err = mlxsw_sp_qevent_binding_configure(qevent_block, qevent_binding);
	if (err)
		goto err_binding_configure;

	list_add(&qevent_binding->list, &qevent_block->binding_list);

	if (register_block) {
		flow_block_cb_add(block_cb, f);
		list_add_tail(&block_cb->driver_list,
			      &mlxsw_sp_qevent_block_cb_list);
	}

	return 0;

err_binding_configure:
	mlxsw_sp_qevent_binding_destroy(qevent_binding);
err_binding_create:
err_binding_exists:
err_find_qdisc:
	if (!flow_block_cb_decref(block_cb))
		flow_block_cb_free(block_cb);
	return err;
}

static void
mlxsw_sp_setup_tc_block_qevent_unbind(struct mlxsw_sp_port *mlxsw_sp_port,
				      struct flow_block_offload *f,
				      enum mlxsw_sp_span_trigger span_trigger)
{
	struct mlxsw_sp *mlxsw_sp = mlxsw_sp_port->mlxsw_sp;
	struct mlxsw_sp_qevent_binding *qevent_binding;
	struct mlxsw_sp_qevent_block *qevent_block;
	struct flow_block_cb *block_cb;
	int tclass_num, child_index;

	block_cb = flow_block_cb_lookup(f->block, mlxsw_sp_qevent_block_cb,
					mlxsw_sp);
	if (!block_cb)
		return;
	qevent_block = flow_block_cb_priv(block_cb);

	/* By the time unbind is called, the original qdisc may have been
	 * unoffloaded and replaced by a FIFO. So figure out the TC directly
	 * from the handle. We validate that the qdisc is offloaded during
	 * binding, so the unbind request is legit, too.
	 */
	child_index = TC_H_MIN(f->sch->parent);
	tclass_num = MLXSW_SP_PRIO_CHILD_TO_TCLASS(child_index);

	qevent_binding = mlxsw_sp_qevent_binding_lookup(qevent_block,
							mlxsw_sp_port, tclass_num,
							span_trigger);
	if (!qevent_binding)
		return;

	list_del(&qevent_binding->list);
	mlxsw_sp_qevent_binding_deconfigure(qevent_block, qevent_binding);
	mlxsw_sp_qevent_binding_destroy(qevent_binding);

	if (!flow_block_cb_decref(block_cb)) {
		flow_block_cb_remove(block_cb, f);
		list_del(&block_cb->driver_list);
	}
}

int mlxsw_sp_setup_tc_block_qevent(struct mlxsw_sp_port *mlxsw_sp_port,
				   struct flow_block_offload *f)
{
	enum mlxsw_sp_span_trigger span_trigger;

	switch (f->binder_type) {
	case FLOW_BLOCK_BINDER_TYPE_RED_EARLY:
		span_trigger = MLXSW_SP_SPAN_TRIGGER_EARLY_DROP;
		break;
	case FLOW_BLOCK_BINDER_TYPE_RED_MARK:
		span_trigger = MLXSW_SP_SPAN_TRIGGER_ECN;
		break;
	default:
		return -EINVAL;
	}

	f->driver_block_list = &mlxsw_sp_qevent_block_cb_list;

	switch (f->command) {
	case FLOW_BLOCK_BIND:
		return mlxsw_sp_setup_tc_block_qevent_bind(mlxsw_sp_port, f,
							   span_trigger);
	case FLOW_BLOCK_UNBIND:
		mlxsw_sp_setup_tc_block_qevent_unbind(mlxsw_sp_port, f,
						      span_trigger);
		return 0;
	default:
		return -EOPNOTSUPP;
	}
}

int mlxsw_sp_tc_qdisc_init(struct mlxsw_sp_port *mlxsw_sp_port)
{
	struct mlxsw_sp_qdisc_state *qdisc_state;

	qdisc_state = kzalloc(sizeof(*qdisc_state), GFP_KERNEL);
	if (!qdisc_state)
		return -ENOMEM;

	qdisc_state->root_class.prio_bitmap = 0xff;
	mlxsw_sp_port->qdisc = qdisc_state;
	return 0;
}

void mlxsw_sp_tc_qdisc_fini(struct mlxsw_sp_port *mlxsw_sp_port)
{
	WARN_ON(mlxsw_sp_port->qdisc->root_class.qdisc);
	kfree(mlxsw_sp_port->qdisc);
}
