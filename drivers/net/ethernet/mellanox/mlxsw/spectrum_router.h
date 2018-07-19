/*
 * drivers/net/ethernet/mellanox/mlxsw/spectrum_router.h
 * Copyright (c) 2017 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2017 Arkadi Sharshevsky <arkadis@mellanox.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _MLXSW_ROUTER_H_
#define _MLXSW_ROUTER_H_

#include "spectrum.h"
#include "reg.h"

struct mlxsw_sp_fib;
struct mlxsw_sp_vr;
struct mlxsw_sp_lpm_tree;
struct mlxsw_sp_rif_ops;

struct mlxsw_sp_router {
	struct mlxsw_sp *mlxsw_sp;
	struct mlxsw_sp_rif **rifs;
	struct mlxsw_sp_vr *vrs;
	struct rhashtable neigh_ht;
	struct rhashtable nexthop_group_ht;
	struct rhashtable nexthop_ht;
	struct list_head nexthop_list;
	struct {
		/* One tree for each protocol: IPv4 and IPv6 */
		struct mlxsw_sp_lpm_tree *proto_trees[2];
		struct mlxsw_sp_lpm_tree *trees;
		unsigned int tree_count;
	} lpm;
	struct {
		struct delayed_work dw;
		unsigned long interval;	/* ms */
	} neighs_update;
	struct delayed_work nexthop_probe_dw;
#define MLXSW_SP_UNRESOLVED_NH_PROBE_INTERVAL 5000 /* ms */
	struct list_head nexthop_neighs_list;
	struct list_head ipip_list;
	bool aborted;
	struct notifier_block fib_nb;
	struct notifier_block netevent_nb;
	const struct mlxsw_sp_rif_ops **rif_ops_arr;
	const struct mlxsw_sp_ipip_ops **ipip_ops_arr;
	struct rhashtable mpls_ht;
};

struct mlxsw_sp_nexthop_group;
struct mlxsw_sp_fib_node;

enum mlxsw_sp_fib_entry_type {
	MLXSW_SP_FIB_ENTRY_TYPE_REMOTE,
	MLXSW_SP_FIB_ENTRY_TYPE_LOCAL,
	MLXSW_SP_FIB_ENTRY_TYPE_TRAP,

	/* This is a special case of local delivery, where a packet should be
	 * decapsulated on reception. Note that there is no corresponding ENCAP,
	 * because that's a type of next hop, not of FIB entry. (There can be
	 * several next hops in a REMOTE entry, and some of them may be
	 * encapsulating entries.)
	 */
	MLXSW_SP_FIB_ENTRY_TYPE_IPIP_DECAP,
};

struct mlxsw_sp_fib_entry_decap {
	struct mlxsw_sp_ipip_entry *ipip_entry;
	u32 tunnel_index;
};

struct mlxsw_sp_fib_entry {
	struct list_head list;
	struct mlxsw_sp_fib_node *fib_node;
	enum mlxsw_sp_fib_entry_type type;
	struct list_head nexthop_group_node;
	struct mlxsw_sp_nexthop_group *nh_group;
	struct mlxsw_sp_fib_entry_decap decap; /* Valid for decap entries. */
};

enum mlxsw_sp_nexthop_type {
	MLXSW_SP_NEXTHOP_TYPE_ETH,
	MLXSW_SP_NEXTHOP_TYPE_IPIP,
};

struct mlxsw_sp_nexthop_key {
	struct fib_nh *fib_nh;
};

struct mlxsw_sp_nexthop {
	struct list_head neigh_list_node; /* member of neigh entry list */
	struct list_head rif_list_node;
	struct list_head router_list_node;
	struct mlxsw_sp_nexthop_group *nh_grp; /* pointer back to the group
						* this belongs to
						*/
	struct rhash_head ht_node;
	struct mlxsw_sp_nexthop_key key;
	unsigned char gw_addr[sizeof(struct in6_addr)];
	int ifindex;
	int nh_weight;
	int norm_nh_weight;
	int num_adj_entries;
	struct mlxsw_sp_rif *rif;
	struct neigh_table *neigh_tbl;
	u8 should_offload:1, /* set indicates this neigh is connected and
			      * should be put to KVD linear area of this group.
			      */
	   offloaded:1, /* set in case the neigh is actually put into
			 * KVD linear area of this group.
			 */
	   update:1; /* set indicates that MAC of this neigh should be
		      * updated in HW
		      */
	enum mlxsw_sp_nexthop_type type;
	union {
		struct mlxsw_sp_neigh_entry *neigh_entry;
		struct mlxsw_sp_ipip_entry *ipip_entry;
	};
	unsigned int counter_index;
	bool counter_valid;
};

struct mlxsw_sp_nexthop_group_ops {
	int (*group_update)(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_nexthop_group *nh_grp,
			    bool reallocate);
	int (*fib_update)(struct mlxsw_sp *mlxsw_sp,
			  struct mlxsw_sp_nexthop_group *nh_grp);
	int (*adj_index_mass_update)(struct mlxsw_sp *mlxsw_sp,
				     struct mlxsw_sp_nexthop_group *nh_grp,
				     u32 old_adj_index, u16 old_ecmp_size);
	int (*fib_refresh)(struct mlxsw_sp_nexthop_group *nh_grp);
	void (*adj_group_size_update)(struct mlxsw_sp_nexthop_group *nh_grp);
	void (*ecmp_size_update)(struct mlxsw_sp_nexthop_group *nh_grp);
};

enum mlxsw_sp_nexthop_group_type {
	MLXSW_SP_NEXTHOP_GROUP_TYPE_IPV4,
	MLXSW_SP_NEXTHOP_GROUP_TYPE_IPV6,
	MLXSW_SP_NEXTHOP_GROUP_TYPE_MPLS,
};

struct mlxsw_sp_nexthop_group {
	void *priv;
	struct rhash_head ht_node;
	struct list_head fib_list; /* list of fib entries that use this group */
	enum mlxsw_sp_nexthop_group_type group_type;
	u8 adj_index_valid:1,
	   gateway:1; /* routes using the group use a gateway */
	u32 adj_index;
	u16 ecmp_size;
	u16 count;
	int sum_norm_weight;
	u16 adj_group_size;
	const struct mlxsw_sp_nexthop_group_ops *ops;
	struct mlxsw_sp_nexthop nexthops[0];
#define nh_rif	nexthops[0].rif
};

struct mlxsw_sp_neigh_key {
	struct neighbour *n;
};

struct mlxsw_sp_neigh_entry {
	struct list_head rif_list_node;
	struct rhash_head ht_node;
	struct mlxsw_sp_neigh_key key;
	u16 rif;
	bool connected;
	unsigned char ha[ETH_ALEN];
	struct list_head nexthop_list; /* list of nexthops using
					* this neigh entry
					*/
	struct list_head nexthop_neighs_list_node;
	unsigned int counter_index;
	bool counter_valid;
};

enum mlxsw_sp_l3proto {
	MLXSW_SP_L3_PROTO_IPV4,
	MLXSW_SP_L3_PROTO_IPV6,
#define MLXSW_SP_L3_PROTO_MAX	(MLXSW_SP_L3_PROTO_IPV6 + 1)
};

union mlxsw_sp_l3addr {
	__be32 addr4;
	struct in6_addr addr6;
};

struct mlxsw_sp_rif_ipip_lb;
struct mlxsw_sp_rif_ipip_lb_config {
	enum mlxsw_reg_ritr_loopback_ipip_type lb_ipipt;
	u32 okey;
	enum mlxsw_sp_l3proto ul_protocol; /* Underlay. */
	union mlxsw_sp_l3addr saddr;
};

enum mlxsw_sp_rif_counter_dir {
	MLXSW_SP_RIF_COUNTER_INGRESS,
	MLXSW_SP_RIF_COUNTER_EGRESS,
};

struct mlxsw_sp_neigh_entry;
struct mlxsw_sp_nexthop;
struct mlxsw_sp_ipip_entry;

struct mlxsw_sp_rif *mlxsw_sp_rif_find_by_dev(const struct mlxsw_sp *mlxsw_sp,
					      const struct net_device *dev);
struct mlxsw_sp_rif *mlxsw_sp_rif_by_index(const struct mlxsw_sp *mlxsw_sp,
					   u16 rif_index);
u16 mlxsw_sp_rif_index(const struct mlxsw_sp_rif *rif);
u16 mlxsw_sp_ipip_lb_rif_index(const struct mlxsw_sp_rif_ipip_lb *rif);
u16 mlxsw_sp_ipip_lb_ul_vr_id(const struct mlxsw_sp_rif_ipip_lb *rif);
u32 mlxsw_sp_ipip_dev_ul_tb_id(const struct net_device *ol_dev);
int mlxsw_sp_rif_dev_ifindex(const struct mlxsw_sp_rif *rif);
u8 mlxsw_sp_router_port(const struct mlxsw_sp *mlxsw_sp);
const struct net_device *mlxsw_sp_rif_dev(const struct mlxsw_sp_rif *rif);
struct mlxsw_sp_fid *mlxsw_sp_rif_fid(const struct mlxsw_sp_rif *rif);
int mlxsw_sp_rif_counter_value_get(struct mlxsw_sp *mlxsw_sp,
				   struct mlxsw_sp_rif *rif,
				   enum mlxsw_sp_rif_counter_dir dir,
				   u64 *cnt);
void mlxsw_sp_rif_counter_free(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_rif *rif,
			       enum mlxsw_sp_rif_counter_dir dir);
int mlxsw_sp_rif_counter_alloc(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_rif *rif,
			       enum mlxsw_sp_rif_counter_dir dir);
struct mlxsw_sp_neigh_entry *
mlxsw_sp_rif_neigh_next(struct mlxsw_sp_rif *rif,
			struct mlxsw_sp_neigh_entry *neigh_entry);
int mlxsw_sp_neigh_entry_type(struct mlxsw_sp_neigh_entry *neigh_entry);
unsigned char *
mlxsw_sp_neigh_entry_ha(struct mlxsw_sp_neigh_entry *neigh_entry);
u32 mlxsw_sp_neigh4_entry_dip(struct mlxsw_sp_neigh_entry *neigh_entry);
struct in6_addr *
mlxsw_sp_neigh6_entry_dip(struct mlxsw_sp_neigh_entry *neigh_entry);

#define mlxsw_sp_rif_neigh_for_each(neigh_entry, rif)				\
	for (neigh_entry = mlxsw_sp_rif_neigh_next(rif, NULL); neigh_entry;	\
	     neigh_entry = mlxsw_sp_rif_neigh_next(rif, neigh_entry))
int mlxsw_sp_neigh_counter_get(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_neigh_entry *neigh_entry,
			       u64 *p_counter);
void
mlxsw_sp_neigh_entry_counter_update(struct mlxsw_sp *mlxsw_sp,
				    struct mlxsw_sp_neigh_entry *neigh_entry,
				    bool adding);
bool mlxsw_sp_neigh_ipv6_ignore(struct mlxsw_sp_neigh_entry *neigh_entry);
int __mlxsw_sp_ipip_entry_update_tunnel(struct mlxsw_sp *mlxsw_sp,
					struct mlxsw_sp_ipip_entry *ipip_entry,
					bool recreate_loopback,
					bool keep_encap,
					bool update_nexthops,
					struct netlink_ext_ack *extack);
void mlxsw_sp_ipip_entry_demote_tunnel(struct mlxsw_sp *mlxsw_sp,
				       struct mlxsw_sp_ipip_entry *ipip_entry);
bool
mlxsw_sp_ipip_demote_tunnel_by_saddr(struct mlxsw_sp *mlxsw_sp,
				     enum mlxsw_sp_l3proto ul_proto,
				     union mlxsw_sp_l3addr saddr,
				     u32 ul_tb_id,
				     const struct mlxsw_sp_ipip_entry *except);
struct mlxsw_sp_nexthop *mlxsw_sp_nexthop_next(struct mlxsw_sp_router *router,
					       struct mlxsw_sp_nexthop *nh);
bool mlxsw_sp_nexthop_offload(struct mlxsw_sp_nexthop *nh);
unsigned char *mlxsw_sp_nexthop_ha(struct mlxsw_sp_nexthop *nh);
int mlxsw_sp_nexthop_indexes(struct mlxsw_sp_nexthop *nh, u32 *p_adj_index,
			     u32 *p_adj_size, u32 *p_adj_hash_index);
struct mlxsw_sp_rif *mlxsw_sp_nexthop_rif(struct mlxsw_sp_nexthop *nh);
bool mlxsw_sp_nexthop_group_has_ipip(struct mlxsw_sp_nexthop *nh);
#define mlxsw_sp_nexthop_for_each(nh, router)				\
	for (nh = mlxsw_sp_nexthop_next(router, NULL); nh;		\
	     nh = mlxsw_sp_nexthop_next(router, nh))
int mlxsw_sp_nexthop_counter_get(struct mlxsw_sp *mlxsw_sp,
				 struct mlxsw_sp_nexthop *nh, u64 *p_counter);
int mlxsw_sp_nexthop_update(struct mlxsw_sp *mlxsw_sp, u32 adj_index,
			    struct mlxsw_sp_nexthop *nh);
void mlxsw_sp_nexthop_counter_alloc(struct mlxsw_sp *mlxsw_sp,
				    struct mlxsw_sp_nexthop *nh);
void mlxsw_sp_nexthop_counter_free(struct mlxsw_sp *mlxsw_sp,
				   struct mlxsw_sp_nexthop *nh);

static inline bool mlxsw_sp_l3addr_eq(const union mlxsw_sp_l3addr *addr1,
				      const union mlxsw_sp_l3addr *addr2)
{
	return !memcmp(addr1, addr2, sizeof(*addr1));
}

void
mlxsw_sp_nexthop_group_refresh(struct mlxsw_sp *mlxsw_sp,
			       struct mlxsw_sp_nexthop_group *nh_grp);
struct mlxsw_sp_rif *
mlxsw_sp_rif_find_by_dev(const struct mlxsw_sp *mlxsw_sp,
			 const struct net_device *dev);
void mlxsw_sp_nexthop_rif_init(struct mlxsw_sp_nexthop *nh,
			       struct mlxsw_sp_rif *rif);
void mlxsw_sp_nexthop_rif_fini(struct mlxsw_sp_nexthop *nh);
int mlxsw_sp_nexthop_neigh_init(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_nexthop *nh);
int mlxsw_sp_nexthop_insert(struct mlxsw_sp *mlxsw_sp,
			    struct mlxsw_sp_nexthop *nh);
void mlxsw_sp_nexthop_remove(struct mlxsw_sp *mlxsw_sp,
			     struct mlxsw_sp_nexthop *nh);
void mlxsw_sp_nexthop_type_fini(struct mlxsw_sp *mlxsw_sp,
				struct mlxsw_sp_nexthop *nh);

#endif /* _MLXSW_ROUTER_H_*/
