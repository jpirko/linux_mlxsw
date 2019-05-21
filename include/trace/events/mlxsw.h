/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */
/* Copyright (c) 2019 Mellanox Technologies. All rights reserved */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mlxsw

#if !defined(_MLXSW_TRACEPOINT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _MLXSW_TRACEPOINT_H

#include <linux/tracepoint.h>

struct mlxsw_sp;
struct mlxsw_sp_acl_atcam_region;
struct mlxsw_sp_acl_tcam_vregion;

TRACE_EVENT(mlxsw_sp_acl_atcam_entry_add_ctcam_spill,
	TP_PROTO(const struct mlxsw_sp *mlxsw_sp,
		 const struct mlxsw_sp_acl_atcam_region *aregion),

	TP_ARGS(mlxsw_sp, aregion),

	TP_STRUCT__entry(
		__field(const void *, mlxsw_sp)
		__field(const void *, aregion)
	),

	TP_fast_assign(
		__entry->mlxsw_sp = mlxsw_sp;
		__entry->aregion = aregion;
	),

	TP_printk("mlxsw_sp %p, aregion %p",
		  __entry->mlxsw_sp, __entry->aregion)
);

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

TRACE_EVENT(mlxsw_sp_acl_tcam_vregion_migrate_end,
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

TRACE_EVENT(mlxsw_sp_acl_tcam_vregion_rehash_rollback_failed,
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

#define MLXSW_SP1_PTP_UNMATCHED_EVENT(NAME)				\
	TRACE_EVENT(NAME,						\
		TP_PROTO(const struct mlxsw_sp *mlxsw_sp,		\
			 bool ingress,					\
			 int local_port,				\
			 u8 message_type,				\
			 u16 sequence_id,				\
			 u8 domain_number,				\
			 u64 timestamp),				\
									\
		TP_ARGS(mlxsw_sp, ingress, local_port, message_type,	\
			sequence_id, domain_number, timestamp),		\
									\
		TP_STRUCT__entry(					\
			__field(const void *, mlxsw_sp)			\
			__field(bool, ingress)				\
			__field(int, local_port)			\
			__field(u8, message_type)			\
			__field(u16, sequence_id)			\
			__field(u8, domain_number)			\
			__field(u64, timestamp)				\
		),							\
									\
		TP_fast_assign(						\
			__entry->mlxsw_sp = mlxsw_sp;			\
			__entry->ingress = ingress;			\
			__entry->local_port = local_port;		\
			__entry->message_type = message_type;		\
			__entry->sequence_id = sequence_id;		\
			__entry->domain_number = domain_number;		\
			__entry->timestamp = timestamp;			\
		),							\
									\
		TP_printk("mlxsw_sp %p, ingress %d, local_port %d, message_type %d, sequence_id %d, domain_number %d, timestamp %lld", \
			  __entry->mlxsw_sp, __entry->ingress,		\
			  __entry->local_port, __entry->message_type,	\
			  __entry->sequence_id, __entry->domain_number, \
			  __entry->timestamp)				\
	);

MLXSW_SP1_PTP_UNMATCHED_EVENT(mlxsw_sp1_ptp_unmatched_save)
MLXSW_SP1_PTP_UNMATCHED_EVENT(mlxsw_sp1_ptp_unmatched_remove)
MLXSW_SP1_PTP_UNMATCHED_EVENT(mlxsw_sp1_ptp_unmatched_collect)

#undef MLXSW_SP1_PTP_UNMATCHED_EVENT

#endif /* _MLXSW_TRACEPOINT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
