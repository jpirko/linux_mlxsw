/*
 * Copyright (c) 2018 Cumulus Networks. All rights reserved.
 * Copyright (c) 2018 David Ahern <dsa@cumulusnetworks.com>
 *
 * This software is licensed under the GNU General License Version 2,
 * June 1991 as shown in the file COPYING in the top-level directory of this
 * source tree.
 *
 * THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE PROGRAM "AS IS"
 * WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE
 * OF THE PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME
 * THE COST OF ALL NECESSARY SERVICING, REPAIR OR CORRECTION.
 */

#include <linux/in6.h>
#include <linux/kernel.h>
#include <linux/rhashtable.h>
#include <net/fib_notifier.h>
#include <net/ip_fib.h>
#include <net/ip6_fib.h>
#include <net/fib_rules.h>
#include <net/net_namespace.h>

#include "netdevsim.h"

struct nsim_fib_entry {
	u64 max;
	u64 num;
};

struct nsim_per_fib_data {
	struct nsim_fib_entry fib;
	struct nsim_fib_entry rules;
};

struct nsim_fib_data {
	struct notifier_block fib_nb;
	struct nsim_per_fib_data ipv4;
	struct nsim_per_fib_data ipv6;
	struct rhashtable fib_rt_ht;
	struct devlink *devlink;
};

struct nsim_fib_rt_key {
	unsigned char addr[sizeof(struct in6_addr)];
	unsigned char prefix_len;
	int family;
	u32 tb_id;
};

struct nsim_fib_rt {
	struct nsim_fib_rt_key key;
	struct rhash_head ht_node;
};

struct nsim_fib4_rt {
	struct nsim_fib_rt common;
	struct fib_info *fi;
	u8 type;
	u8 tos;
};

static const struct rhashtable_params nsim_fib_rt_ht_params = {
	.key_offset = offsetof(struct nsim_fib_rt, key),
	.head_offset = offsetof(struct nsim_fib_rt, ht_node),
	.key_len = sizeof(struct nsim_fib_rt_key),
	.automatic_shrinking = true,
};

u64 nsim_fib_get_val(struct nsim_fib_data *fib_data,
		     enum nsim_resource_id res_id, bool max)
{
	struct nsim_fib_entry *entry;

	switch (res_id) {
	case NSIM_RESOURCE_IPV4_FIB:
		entry = &fib_data->ipv4.fib;
		break;
	case NSIM_RESOURCE_IPV4_FIB_RULES:
		entry = &fib_data->ipv4.rules;
		break;
	case NSIM_RESOURCE_IPV6_FIB:
		entry = &fib_data->ipv6.fib;
		break;
	case NSIM_RESOURCE_IPV6_FIB_RULES:
		entry = &fib_data->ipv6.rules;
		break;
	default:
		return 0;
	}

	return max ? entry->max : entry->num;
}

static void nsim_fib_set_max(struct nsim_fib_data *fib_data,
			     enum nsim_resource_id res_id, u64 val)
{
	struct nsim_fib_entry *entry;

	switch (res_id) {
	case NSIM_RESOURCE_IPV4_FIB:
		entry = &fib_data->ipv4.fib;
		break;
	case NSIM_RESOURCE_IPV4_FIB_RULES:
		entry = &fib_data->ipv4.rules;
		break;
	case NSIM_RESOURCE_IPV6_FIB:
		entry = &fib_data->ipv6.fib;
		break;
	case NSIM_RESOURCE_IPV6_FIB_RULES:
		entry = &fib_data->ipv6.rules;
		break;
	default:
		WARN_ON(1);
		return;
	}
	entry->max = val;
}

static int nsim_fib_rule_account(struct nsim_fib_entry *entry, bool add,
				 struct netlink_ext_ack *extack)
{
	int err = 0;

	if (add) {
		if (entry->num < entry->max) {
			entry->num++;
		} else {
			err = -ENOSPC;
			NL_SET_ERR_MSG_MOD(extack, "Exceeded number of supported fib rule entries");
		}
	} else {
		entry->num--;
	}

	return err;
}

static int nsim_fib_rule_event(struct nsim_fib_data *data,
			       struct fib_notifier_info *info, bool add)
{
	struct netlink_ext_ack *extack = info->extack;
	int err = 0;

	switch (info->family) {
	case AF_INET:
		err = nsim_fib_rule_account(&data->ipv4.rules, add, extack);
		break;
	case AF_INET6:
		err = nsim_fib_rule_account(&data->ipv6.rules, add, extack);
		break;
	}

	return err;
}

static int nsim_fib_account(struct nsim_fib_entry *entry, bool add,
			    struct netlink_ext_ack *extack)
{
	int err = 0;

	if (add) {
		if (entry->num < entry->max) {
			entry->num++;
		} else {
			err = -ENOSPC;
			NL_SET_ERR_MSG_MOD(extack, "Exceeded number of supported fib entries");
		}
	} else {
		entry->num--;
	}

	return err;
}

static void nsim_fib_rt_init(struct nsim_fib_rt *fib_rt, const void *addr,
			     size_t addr_len, unsigned int prefix_len,
			     int family, u32 tb_id)
{
	memcpy(fib_rt->key.addr, addr, addr_len);
	fib_rt->key.prefix_len = prefix_len;
	fib_rt->key.family = family;
	fib_rt->key.tb_id = tb_id;
}

static struct nsim_fib_rt *nsim_fib_rt_lookup(struct rhashtable *fib_rt_ht,
					      const void *addr, size_t addr_len,
					      unsigned int prefix_len,
					      int family, u32 tb_id)
{
	struct nsim_fib_rt_key key;

	memset(&key, 0, sizeof(key));
	memcpy(key.addr, addr, addr_len);
	key.prefix_len = prefix_len;
	key.family = family;
	key.tb_id = tb_id;

	return rhashtable_lookup_fast(fib_rt_ht, &key, nsim_fib_rt_ht_params);
}

static struct nsim_fib4_rt *
nsim_fib4_rt_create(struct fib_entry_notifier_info *fen_info)
{
	struct nsim_fib4_rt *fib4_rt;

	fib4_rt = kzalloc(sizeof(*fib4_rt), GFP_ATOMIC);
	if (!fib4_rt)
		return NULL;

	nsim_fib_rt_init(&fib4_rt->common, &fen_info->dst, sizeof(u32),
			 fen_info->dst_len, AF_INET, fen_info->tb_id);

	fib4_rt->fi = fen_info->fi;
	fib_info_hold(fib4_rt->fi);
	fib4_rt->tos = fen_info->tos;
	fib4_rt->type = fen_info->type;

	return fib4_rt;
}

static void nsim_fib4_rt_destroy(struct nsim_fib4_rt *fib4_rt)
{
	fib_info_put(fib4_rt->fi);
	kfree(fib4_rt);
}

static struct nsim_fib4_rt *
nsim_fib4_rt_lookup(struct rhashtable *fib_rt_ht,
		    const struct fib_entry_notifier_info *fen_info)
{
	struct nsim_fib_rt *fib_rt;

	fib_rt = nsim_fib_rt_lookup(fib_rt_ht, &fen_info->dst, sizeof(u32),
				    fen_info->dst_len, AF_INET,
				    fen_info->tb_id);
	if (!fib_rt)
		return NULL;

	return container_of(fib_rt, struct nsim_fib4_rt, common);
}

static int nsim_fib4_rt_add(struct nsim_fib_data *data,
			    struct nsim_fib4_rt *fib4_rt,
			    struct netlink_ext_ack *extack)
{
	struct nsim_fib_rt *fib_rt = &fib4_rt->common;
	struct net *net = devlink_net(data->devlink);
	u32 *addr = (u32 *) fib_rt->key.addr;
	int err;

	err = nsim_fib_account(&data->ipv4.fib, true, extack);
	if (err)
		return err;

	err = rhashtable_insert_fast(&data->fib_rt_ht,
				     &fib4_rt->common.ht_node,
				     nsim_fib_rt_ht_params);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to insert IPv4 route");
		goto err_fib_dismiss;
	}

	fib_alias_in_hw_set(net, *addr, fib_rt->key.prefix_len, fib4_rt->fi,
			    fib4_rt->tos, fib4_rt->type, fib_rt->key.tb_id);

	return 0;

err_fib_dismiss:
	nsim_fib_account(&data->ipv4.fib, false, extack);
	return err;
}

static int nsim_fib4_rt_replace(struct nsim_fib_data *data,
				struct nsim_fib4_rt *fib4_rt,
				struct nsim_fib4_rt *fib4_rt_old,
				struct netlink_ext_ack *extack)
{
	struct nsim_fib_rt *fib_rt_old = &fib4_rt_old->common;
	struct nsim_fib_rt *fib_rt = &fib4_rt->common;
	struct net *net = devlink_net(data->devlink);
	u32 *addr = (u32 *) fib_rt->key.addr;
	int err;

	/* We are replacing a route, so no need to change the accounting. */
	err = rhashtable_replace_fast(&data->fib_rt_ht,
				      &fib4_rt_old->common.ht_node,
				      &fib4_rt->common.ht_node,
				      nsim_fib_rt_ht_params);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to replace IPv4 route");
		return err;
	}

	fib_alias_in_hw_clear(net, *addr, fib_rt->key.prefix_len,
			      fib4_rt_old->fi, fib4_rt_old->tos,
			      fib4_rt_old->type, fib_rt->key.tb_id);
	nsim_fib4_rt_destroy(fib4_rt_old);

	fib_alias_in_hw_set(net, *addr, fib_rt->key.prefix_len,
			    fib4_rt->fi, fib4_rt->tos, fib4_rt->type,
			    fib_rt->key.tb_id);

	return 0;
}

static int nsim_fib4_rt_insert(struct nsim_fib_data *data,
			       struct fib_entry_notifier_info *fen_info)
{
	struct netlink_ext_ack *extack = fen_info->info.extack;
	struct nsim_fib4_rt *fib4_rt, *fib4_rt_old;
	int err;

	fib4_rt = nsim_fib4_rt_create(fen_info);
	if (!fib4_rt)
		return -ENOMEM;

	fib4_rt_old = nsim_fib4_rt_lookup(&data->fib_rt_ht, fen_info);
	if (!fib4_rt_old)
		err = nsim_fib4_rt_add(data, fib4_rt, extack);
	else
		err = nsim_fib4_rt_replace(data, fib4_rt, fib4_rt_old, extack);

	if (err)
		nsim_fib4_rt_destroy(fib4_rt);

	return err;
}

static void nsim_fib4_rt_remove(struct nsim_fib_data *data,
				const struct fib_entry_notifier_info *fen_info)
{
	struct netlink_ext_ack *extack = fen_info->info.extack;
	struct nsim_fib4_rt *fib4_rt;

	fib4_rt = nsim_fib4_rt_lookup(&data->fib_rt_ht, fen_info);
	if (WARN_ON_ONCE(!fib4_rt))
		return;

	rhashtable_remove_fast(&data->fib_rt_ht, &fib4_rt->common.ht_node,
			       nsim_fib_rt_ht_params);
	nsim_fib_account(&data->ipv4.fib, false, extack);
	nsim_fib4_rt_destroy(fib4_rt);
}

static int nsim_fib4_event(struct nsim_fib_data *data,
			   struct fib_notifier_info *info, bool add)
{
	struct fib_entry_notifier_info *fen_info;
	int err = 0;

	fen_info = container_of(info, struct fib_entry_notifier_info, info);

	if (add)
		err = nsim_fib4_rt_insert(data, fen_info);
	else
		nsim_fib4_rt_remove(data, fen_info);

	return err;
}

static int nsim_fib_event(struct nsim_fib_data *data,
			  struct fib_notifier_info *info, bool add)
{
	struct netlink_ext_ack *extack = info->extack;
	int err = 0;

	switch (info->family) {
	case AF_INET:
		err = nsim_fib4_event(data, info, add);
		break;
	case AF_INET6:
		err = nsim_fib_account(&data->ipv6.fib, add, extack);
		break;
	}

	return err;
}

static int nsim_fib_event_nb(struct notifier_block *nb, unsigned long event,
			     void *ptr)
{
	struct nsim_fib_data *data = container_of(nb, struct nsim_fib_data,
						  fib_nb);
	struct fib_notifier_info *info = ptr;
	int err = 0;

	switch (event) {
	case FIB_EVENT_RULE_ADD: /* fall through */
	case FIB_EVENT_RULE_DEL:
		err = nsim_fib_rule_event(data, info,
					  event == FIB_EVENT_RULE_ADD);
		break;

	case FIB_EVENT_ENTRY_REPLACE:  /* fall through */
	case FIB_EVENT_ENTRY_ADD:  /* fall through */
	case FIB_EVENT_ENTRY_DEL:
		err = nsim_fib_event(data, info, event != FIB_EVENT_ENTRY_DEL);
		break;
	}

	return notifier_from_errno(err);
}

/* inconsistent dump, trying again */
static void nsim_fib_dump_inconsistent(struct notifier_block *nb)
{
	struct nsim_fib_data *data = container_of(nb, struct nsim_fib_data,
						  fib_nb);

	data->ipv4.fib.num = 0ULL;
	data->ipv4.rules.num = 0ULL;
	data->ipv6.fib.num = 0ULL;
	data->ipv6.rules.num = 0ULL;
}

static u64 nsim_fib_ipv4_resource_occ_get(void *priv)
{
	struct nsim_fib_data *data = priv;

	return nsim_fib_get_val(data, NSIM_RESOURCE_IPV4_FIB, false);
}

static u64 nsim_fib_ipv4_rules_res_occ_get(void *priv)
{
	struct nsim_fib_data *data = priv;

	return nsim_fib_get_val(data, NSIM_RESOURCE_IPV4_FIB_RULES, false);
}

static u64 nsim_fib_ipv6_resource_occ_get(void *priv)
{
	struct nsim_fib_data *data = priv;

	return nsim_fib_get_val(data, NSIM_RESOURCE_IPV6_FIB, false);
}

static u64 nsim_fib_ipv6_rules_res_occ_get(void *priv)
{
	struct nsim_fib_data *data = priv;

	return nsim_fib_get_val(data, NSIM_RESOURCE_IPV6_FIB_RULES, false);
}

static void nsim_fib_set_max_all(struct nsim_fib_data *data,
				 struct devlink *devlink)
{
	enum nsim_resource_id res_ids[] = {
		NSIM_RESOURCE_IPV4_FIB, NSIM_RESOURCE_IPV4_FIB_RULES,
		NSIM_RESOURCE_IPV6_FIB, NSIM_RESOURCE_IPV6_FIB_RULES
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(res_ids); i++) {
		int err;
		u64 val;

		err = devlink_resource_size_get(devlink, res_ids[i], &val);
		if (err)
			val = (u64) -1;
		nsim_fib_set_max(data, res_ids[i], val);
	}
}

static void nsim_fib4_rt_free(struct nsim_fib_rt *fib_rt,
			      struct devlink *devlink)
{
	u32 *addr = (u32 *) fib_rt->key.addr;
	struct nsim_fib4_rt *fib4_rt;

	fib4_rt = container_of(fib_rt, struct nsim_fib4_rt, common);
	fib_alias_in_hw_clear(devlink_net(devlink), *addr,
			      fib_rt->key.prefix_len, fib4_rt->fi, fib4_rt->tos,
			      fib4_rt->type, fib_rt->key.tb_id);
	nsim_fib4_rt_destroy(fib4_rt);
}

static void nsim_fib_rt_free(void *ptr, void *arg)
{
	struct nsim_fib_rt *fib_rt = ptr;
	struct devlink *devlink = arg;

	switch (fib_rt->key.family) {
	case AF_INET:
		nsim_fib4_rt_free(fib_rt, devlink);
		break;
	default:
		WARN_ON_ONCE(1);
	}
}

struct nsim_fib_data *nsim_fib_create(struct devlink *devlink,
				      struct netlink_ext_ack *extack)
{
	struct nsim_fib_data *data;
	int err;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return ERR_PTR(-ENOMEM);
	data->devlink = devlink;

	err = rhashtable_init(&data->fib_rt_ht, &nsim_fib_rt_ht_params);
	if (err)
		goto err_data_free;

	nsim_fib_set_max_all(data, devlink);

	data->fib_nb.notifier_call = nsim_fib_event_nb;
	err = register_fib_notifier(devlink_net(devlink), &data->fib_nb,
				    nsim_fib_dump_inconsistent, extack);
	if (err) {
		pr_err("Failed to register fib notifier\n");
		goto err_rhashtable_destroy;
	}

	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV4_FIB,
					  nsim_fib_ipv4_resource_occ_get,
					  data);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV4_FIB_RULES,
					  nsim_fib_ipv4_rules_res_occ_get,
					  data);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV6_FIB,
					  nsim_fib_ipv6_resource_occ_get,
					  data);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV6_FIB_RULES,
					  nsim_fib_ipv6_rules_res_occ_get,
					  data);
	return data;

err_rhashtable_destroy:
	rhashtable_free_and_destroy(&data->fib_rt_ht, nsim_fib_rt_free,
				    devlink);
err_data_free:
	kfree(data);
	return ERR_PTR(err);
}

void nsim_fib_destroy(struct devlink *devlink, struct nsim_fib_data *data)
{
	devlink_resource_occ_get_unregister(devlink,
					    NSIM_RESOURCE_IPV6_FIB_RULES);
	devlink_resource_occ_get_unregister(devlink,
					    NSIM_RESOURCE_IPV6_FIB);
	devlink_resource_occ_get_unregister(devlink,
					    NSIM_RESOURCE_IPV4_FIB_RULES);
	devlink_resource_occ_get_unregister(devlink,
					    NSIM_RESOURCE_IPV4_FIB);
	unregister_fib_notifier(devlink_net(devlink), &data->fib_nb);
	rhashtable_free_and_destroy(&data->fib_rt_ht, nsim_fib_rt_free,
				    devlink);
	kfree(data);
}
