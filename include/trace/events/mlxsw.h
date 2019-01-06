/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */
/* Copyright (c) 2018 Mellanox Technologies. All rights reserved */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mlxsw

#if !defined(_MLXSW_TRACEPOINT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MLXSW_TRACEPOINT_H

#include <linux/tracepoint.h>

struct mlxsw_sp;
struct mlxsw_sp_acl_tcam_vregion;

TRACE_EVENT(mlxsw_sp_acl_tcam_vregion_rehash,
	TP_PROTO(const struct mlxsw_sp *mlxsw_sp,
		 const struct mlxsw_sp_acl_tcam_vregion *vregion),

	TP_ARGS(mlxsw_sp, vregion),

	TP_STRUCT__entry(
		__field(const void *, mlxsw_sp)
		__field(const void *, vregion)
	),

	TP_fast_assign(
		__entry->mlxsw_sp = mlxsw_sp;
		__entry->vregion = vregion;
	),

	TP_printk("mlxsw_sp %p, vregion %p",
		  __entry->mlxsw_sp, __entry->vregion)
);

TRACE_EVENT(mlxsw_sp_acl_tcam_vregion_migrate,
	TP_PROTO(const struct mlxsw_sp *mlxsw_sp,
		 const struct mlxsw_sp_acl_tcam_vregion *vregion),

	TP_ARGS(mlxsw_sp, vregion),

	TP_STRUCT__entry(
		__field(const void *, mlxsw_sp)
		__field(const void *, vregion)
	),

	TP_fast_assign(
		__entry->mlxsw_sp = mlxsw_sp;
		__entry->vregion = vregion;
	),

	TP_printk("mlxsw_sp %p, vregion %p",
		  __entry->mlxsw_sp, __entry->vregion)
);

TRACE_EVENT(mlxsw_sp_acl_tcam_vregion_rehash_dis,
	TP_PROTO(const struct mlxsw_sp *mlxsw_sp,
		 const struct mlxsw_sp_acl_tcam_vregion *vregion),

	TP_ARGS(mlxsw_sp, vregion),

	TP_STRUCT__entry(
		__field(const void *, mlxsw_sp)
		__field(const void *, vregion)
	),

	TP_fast_assign(
		__entry->mlxsw_sp = mlxsw_sp;
		__entry->vregion = vregion;
	),

	TP_printk("mlxsw_sp %p, vregion %p",
		  __entry->mlxsw_sp, __entry->vregion)
);

#endif /* _MLXSW_TRACEPOINT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
