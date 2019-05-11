/*
 * Copyright (c) 2018 Cumulus Networks. All rights reserved.
 * Copyright (c) 2018 David Ahern <dsa@cumulusnetworks.com>
 * Copyright (c) 2019 Mellanox Technologies. All rights reserved.
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

#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/etherdevice.h>
#include <linux/inet.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/workqueue.h>
#include <linux/random.h>
#include <linux/rtnetlink.h>
#include <net/devlink.h>
#include <net/ip.h>
#include <uapi/linux/devlink.h>
#include <uapi/linux/ip.h>
#include <uapi/linux/udp.h>

#include "netdevsim.h"

static struct dentry *nsim_dev_ddir;

static int nsim_dev_debugfs_init(struct nsim_dev *nsim_dev)
{
	char dev_ddir_name[16];

	sprintf(dev_ddir_name, DRV_NAME "%u", nsim_dev->nsim_bus_dev->dev.id);
	nsim_dev->ddir = debugfs_create_dir(dev_ddir_name, nsim_dev_ddir);
	if (IS_ERR_OR_NULL(nsim_dev->ddir))
		return PTR_ERR_OR_ZERO(nsim_dev->ddir) ?: -EINVAL;
	nsim_dev->ports_ddir = debugfs_create_dir("ports", nsim_dev->ddir);
	if (IS_ERR_OR_NULL(nsim_dev->ports_ddir))
		return PTR_ERR_OR_ZERO(nsim_dev->ports_ddir) ?: -EINVAL;
	return 0;
}

static void nsim_dev_debugfs_exit(struct nsim_dev *nsim_dev)
{
	debugfs_remove_recursive(nsim_dev->ports_ddir);
	debugfs_remove_recursive(nsim_dev->ddir);
}

static int nsim_dev_port_debugfs_init(struct nsim_dev *nsim_dev,
				      struct nsim_dev_port *nsim_dev_port)
{
	char port_ddir_name[16];
	char dev_link_name[32];

	sprintf(port_ddir_name, "%u", nsim_dev_port->port_index);
	nsim_dev_port->ddir = debugfs_create_dir(port_ddir_name,
						 nsim_dev->ports_ddir);
	if (IS_ERR_OR_NULL(nsim_dev_port->ddir))
		return -ENOMEM;

	sprintf(dev_link_name, "../../../" DRV_NAME "%u",
		nsim_dev->nsim_bus_dev->dev.id);
	debugfs_create_symlink("dev", nsim_dev_port->ddir, dev_link_name);

	return 0;
}

static void nsim_dev_port_debugfs_exit(struct nsim_dev_port *nsim_dev_port)
{
	debugfs_remove_recursive(nsim_dev_port->ddir);
}

static u64 nsim_dev_ipv4_fib_resource_occ_get(void *priv)
{
	struct nsim_dev *nsim_dev = priv;

	return nsim_fib_get_val(nsim_dev->fib_data,
				NSIM_RESOURCE_IPV4_FIB, false);
}

static u64 nsim_dev_ipv4_fib_rules_res_occ_get(void *priv)
{
	struct nsim_dev *nsim_dev = priv;

	return nsim_fib_get_val(nsim_dev->fib_data,
				NSIM_RESOURCE_IPV4_FIB_RULES, false);
}

static u64 nsim_dev_ipv6_fib_resource_occ_get(void *priv)
{
	struct nsim_dev *nsim_dev = priv;

	return nsim_fib_get_val(nsim_dev->fib_data,
				NSIM_RESOURCE_IPV6_FIB, false);
}

static u64 nsim_dev_ipv6_fib_rules_res_occ_get(void *priv)
{
	struct nsim_dev *nsim_dev = priv;

	return nsim_fib_get_val(nsim_dev->fib_data,
				NSIM_RESOURCE_IPV6_FIB_RULES, false);
}

static int nsim_dev_resources_register(struct devlink *devlink)
{
	struct nsim_dev *nsim_dev = devlink_priv(devlink);
	struct devlink_resource_size_params params = {
		.size_max = (u64)-1,
		.size_granularity = 1,
		.unit = DEVLINK_RESOURCE_UNIT_ENTRY
	};
	int err;
	u64 n;

	/* Resources for IPv4 */
	err = devlink_resource_register(devlink, "IPv4", (u64)-1,
					NSIM_RESOURCE_IPV4,
					DEVLINK_RESOURCE_ID_PARENT_TOP,
					&params);
	if (err) {
		pr_err("Failed to register IPv4 top resource\n");
		goto out;
	}

	n = nsim_fib_get_val(nsim_dev->fib_data,
			     NSIM_RESOURCE_IPV4_FIB, true);
	err = devlink_resource_register(devlink, "fib", n,
					NSIM_RESOURCE_IPV4_FIB,
					NSIM_RESOURCE_IPV4, &params);
	if (err) {
		pr_err("Failed to register IPv4 FIB resource\n");
		return err;
	}

	n = nsim_fib_get_val(nsim_dev->fib_data,
			     NSIM_RESOURCE_IPV4_FIB_RULES, true);
	err = devlink_resource_register(devlink, "fib-rules", n,
					NSIM_RESOURCE_IPV4_FIB_RULES,
					NSIM_RESOURCE_IPV4, &params);
	if (err) {
		pr_err("Failed to register IPv4 FIB rules resource\n");
		return err;
	}

	/* Resources for IPv6 */
	err = devlink_resource_register(devlink, "IPv6", (u64)-1,
					NSIM_RESOURCE_IPV6,
					DEVLINK_RESOURCE_ID_PARENT_TOP,
					&params);
	if (err) {
		pr_err("Failed to register IPv6 top resource\n");
		goto out;
	}

	n = nsim_fib_get_val(nsim_dev->fib_data,
			     NSIM_RESOURCE_IPV6_FIB, true);
	err = devlink_resource_register(devlink, "fib", n,
					NSIM_RESOURCE_IPV6_FIB,
					NSIM_RESOURCE_IPV6, &params);
	if (err) {
		pr_err("Failed to register IPv6 FIB resource\n");
		return err;
	}

	n = nsim_fib_get_val(nsim_dev->fib_data,
			     NSIM_RESOURCE_IPV6_FIB_RULES, true);
	err = devlink_resource_register(devlink, "fib-rules", n,
					NSIM_RESOURCE_IPV6_FIB_RULES,
					NSIM_RESOURCE_IPV6, &params);
	if (err) {
		pr_err("Failed to register IPv6 FIB rules resource\n");
		return err;
	}

	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV4_FIB,
					  nsim_dev_ipv4_fib_resource_occ_get,
					  nsim_dev);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV4_FIB_RULES,
					  nsim_dev_ipv4_fib_rules_res_occ_get,
					  nsim_dev);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV6_FIB,
					  nsim_dev_ipv6_fib_resource_occ_get,
					  nsim_dev);
	devlink_resource_occ_get_register(devlink,
					  NSIM_RESOURCE_IPV6_FIB_RULES,
					  nsim_dev_ipv6_fib_rules_res_occ_get,
					  nsim_dev);
out:
	return err;
}

/* All driver-specific traps must be documented in
 * Documentation/networking/devlink-trap-netdevsim.rst
 */
enum {
	NSIM_TRAP_ID_BASE = DEVLINK_TRAP_GENERIC_ID_MAX,
	NSIM_TRAP_ID_FID_MISS_EXCEPTION,
};

#define NSIM_TRAP_NAME_FID_MISS_EXCEPTION "fid_miss_exception"

#define NSIM_GENERIC_TRAP(_id, _init_action, _type, _group_id)		      \
	{								      \
		.metadata_cap = {					      \
			.port_type = DEVLINK_PORT_TYPE_ETH,		      \
			.timestamp = 1,					      \
			.in_port = 1,					      \
		},							      \
		.action_set = nsim_dev_trap_action_set,			      \
		.init_action = DEVLINK_TRAP_ACTION_##_init_action,	      \
		.type = DEVLINK_TRAP_TYPE_##_type,			      \
		.generic = true,					      \
		.name = DEVLINK_TRAP_GENERIC_NAME_##_id,		      \
		.group_id = NSIM_TRAP_GROUP_ID_##_group_id,		      \
		.id = DEVLINK_TRAP_GENERIC_ID_##_id,			      \
	}

#define NSIM_DRIVER_TRAP(_id, _init_action, _type, _group_id)		      \
	{								      \
		.metadata_cap = {					      \
			.port_type = DEVLINK_PORT_TYPE_ETH,		      \
			.timestamp = 1,					      \
			.in_port = 1,					      \
		},							      \
		.action_set = nsim_dev_trap_action_set,			      \
		.init_action = DEVLINK_TRAP_ACTION_##_init_action,	      \
		.type = DEVLINK_TRAP_TYPE_##_type,			      \
		.generic = false,					      \
		.name = NSIM_TRAP_NAME_##_id,				      \
		.group_id = NSIM_TRAP_GROUP_ID_##_group_id,		      \
		.id = NSIM_TRAP_ID_##_id,				      \
	}

#define NSIM_TRAP_L2(_id)						      \
	NSIM_GENERIC_TRAP(_id, DROP, DROP, L2_DROPS)
#define NSIM_TRAP_L2_DRIVER_EXCEPTION(_id)				      \
	NSIM_DRIVER_TRAP(_id, TRAP, EXCEPTION, L2_DROPS)
#define NSIM_TRAP_L3_DROP(_id)						      \
	NSIM_GENERIC_TRAP(_id, DROP, DROP, L3_DROPS)
#define NSIM_TRAP_L3_EXCEPTION(_id)					      \
	NSIM_GENERIC_TRAP(_id, TRAP, EXCEPTION, L3_DROPS)
#define NSIM_TRAP_BUFFER(_id)						      \
	NSIM_GENERIC_TRAP(_id, DROP, DROP, BUFFER_DROPS)

/* All trap groups must be documented in
 * Documentation/networking/devlink-trap-netdevsim.rst
 */
enum {
	NSIM_TRAP_GROUP_ID_L2_DROPS,
	NSIM_TRAP_GROUP_ID_L3_DROPS,
	NSIM_TRAP_GROUP_ID_BUFFER_DROPS,
};

#define NSIM_TRAP_GROUP_NAME_L2_DROPS			"l2_drops"
#define NSIM_TRAP_GROUP_NAME_L3_DROPS			"l3_drops"
#define NSIM_TRAP_GROUP_NAME_BUFFER_DROPS		"buffer_drops"

#define NSIM_TRAP_GROUP(_id)						      \
	{								      \
		.id = NSIM_TRAP_GROUP_ID_##_id,				      \
		.name = NSIM_TRAP_GROUP_NAME_##_id,			      \
	}

static int nsim_dev_trap_action_set(struct devlink *devlink, u16 id,
				    enum devlink_trap_action action,
				    struct netlink_ext_ack *extack);

const static struct devlink_trap_group nsim_trap_groups_arr[] = {
	NSIM_TRAP_GROUP(L2_DROPS),
	NSIM_TRAP_GROUP(L3_DROPS),
	NSIM_TRAP_GROUP(BUFFER_DROPS),
};

const static struct devlink_trap nsim_traps_arr[] = {
	NSIM_TRAP_L2(INGRESS_SMAC_MC_DROP),
	NSIM_TRAP_L2(INGRESS_VLAN_TAG_ALLOW_DROP),
	NSIM_TRAP_L2(INGRESS_VLAN_FILTER_DROP),
	NSIM_TRAP_L2(INGRESS_STP_FILTER_DROP),
	NSIM_TRAP_L2(EMPTY_TX_LIST_DROP),
	NSIM_TRAP_L2(LOOPBACK_FILTER_DROP),
	NSIM_TRAP_L2_DRIVER_EXCEPTION(FID_MISS_EXCEPTION),
	NSIM_TRAP_L3_DROP(BLACKHOLE_ROUTE_DROP),
	NSIM_TRAP_L3_EXCEPTION(TTL_ERROR_EXCEPTION),
	NSIM_TRAP_BUFFER(TAIL_DROP),
	NSIM_TRAP_BUFFER(EARLY_DROP),
};

#define NSIM_TRAP_REPORT_INTERVAL_MS	1000

static int nsim_dev_trap_action_set(struct devlink *devlink, u16 id,
				    enum devlink_trap_action action,
				    struct netlink_ext_ack *extack)
{
	struct nsim_dev *nsim_dev = devlink_priv(devlink);
	size_t arr_size = ARRAY_SIZE(nsim_traps_arr);
	int i;

	for (i = 0; i < arr_size; i++) {
		if (nsim_traps_arr[i].id == id)
			break;
	}

	if (WARN_ON_ONCE(i == arr_size))
		return -EINVAL;

	nsim_dev->traps_state_arr[i] = action == DEVLINK_TRAP_ACTION_TRAP;

	return 0;
}

static struct sk_buff *nsim_dev_trap_skb_build(void)
{
	int tot_len, data_len = 100;
	struct sk_buff *skb;
	struct udphdr *udph;
	struct ethhdr *eth;
	struct iphdr *iph;

	skb = alloc_skb(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		return NULL;
	tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + data_len;

	eth = skb_put(skb, sizeof(struct ethhdr));
	eth_random_addr(eth->h_dest);
	eth_random_addr(eth->h_source);
	eth->h_proto = htons(ETH_P_IP);
	skb->protocol = htons(ETH_P_IP);

	iph = skb_put(skb, sizeof(struct iphdr));
	iph->protocol = IPPROTO_UDP;
	iph->saddr = in_aton("192.0.2.1");
	iph->daddr = in_aton("198.51.100.1");
	iph->version = 0x4;
	iph->frag_off = 0;
	iph->ihl = 0x5;
	iph->tot_len = htons(tot_len);
	ip_send_check(iph);

	udph = skb_put_zero(skb, sizeof(struct udphdr) + data_len);
	get_random_bytes(&udph->source, sizeof(u16));
	get_random_bytes(&udph->dest, sizeof(u16));
	udph->len = htons(sizeof(struct udphdr) + data_len);

	return skb;
}

static void nsim_dev_trap_report(struct nsim_dev_port *nsim_dev_port)
{
	struct nsim_dev *nsim_dev = nsim_dev_port->ns->nsim_dev;
	struct devlink *devlink = priv_to_devlink(nsim_dev);
	int i;

	for (i = 0; i < ARRAY_SIZE(nsim_traps_arr); i++) {
		struct devlink_trap_metadata metadata = {
			.in_devlink_port = &nsim_dev_port->devlink_port,
			.timestamp = jiffies_to_nsecs(jiffies),
		};
		struct sk_buff *skb;

		if (nsim_dev->traps_state_arr[i] == false)
			continue;

		skb = nsim_dev_trap_skb_build();
		if (!skb)
			continue;
		skb->dev = nsim_dev_port->ns->netdev;

		devlink_trap_report(devlink, &nsim_traps_arr[i], skb,
				    &metadata);
		consume_skb(skb);
	}
}

static void nsim_dev_trap_report_work(struct work_struct *work)
{
	struct nsim_dev *nsim_dev = container_of(work, struct nsim_dev,
						 trap_report_dw.work);
	struct nsim_dev_port *nsim_dev_port;

	/* For each running port and enabled packet trap, generate a UDP
	 * packet with a random 5-tuple and report it.
	 */
	mutex_lock(&nsim_dev->port_list_lock);
	list_for_each_entry(nsim_dev_port, &nsim_dev->port_list, list) {
		if (!netif_running(nsim_dev_port->ns->netdev))
			continue;

		nsim_dev_trap_report(nsim_dev_port);
	}
	mutex_unlock(&nsim_dev->port_list_lock);

	schedule_delayed_work(&nsim_dev->trap_report_dw,
			      msecs_to_jiffies(NSIM_TRAP_REPORT_INTERVAL_MS));
}

static int nsim_dev_traps_init(struct devlink *devlink)
{
	struct nsim_dev *nsim_dev = devlink_priv(devlink);
	int i, err;

	nsim_dev->traps_state_arr = kcalloc(ARRAY_SIZE(nsim_traps_arr),
					    sizeof(u8), GFP_KERNEL);
	if (!nsim_dev->traps_state_arr)
		return -ENOMEM;

	for (i = 0; i < ARRAY_SIZE(nsim_traps_arr); i++) {
		if (nsim_traps_arr[i].init_action != DEVLINK_TRAP_ACTION_DROP)
			nsim_dev->traps_state_arr[i] = true;
	}

	err = devlink_trap_groups_register(devlink, nsim_trap_groups_arr,
					   ARRAY_SIZE(nsim_trap_groups_arr));
	if (err)
		goto err_free_traps_arr;

	err = devlink_traps_register(devlink, nsim_traps_arr,
				     ARRAY_SIZE(nsim_traps_arr));
	if (err)
		goto err_trap_groups_unregister;

	INIT_DELAYED_WORK(&nsim_dev->trap_report_dw, nsim_dev_trap_report_work);
	schedule_delayed_work(&nsim_dev->trap_report_dw,
			      msecs_to_jiffies(NSIM_TRAP_REPORT_INTERVAL_MS));

	return 0;

err_trap_groups_unregister:
	devlink_trap_groups_unregister(devlink, nsim_trap_groups_arr,
				       ARRAY_SIZE(nsim_trap_groups_arr));
err_free_traps_arr:
	kfree(nsim_dev->traps_state_arr);
	return err;
}

static void nsim_dev_traps_exit(struct devlink *devlink)
{
	struct nsim_dev *nsim_dev = devlink_priv(devlink);

	cancel_delayed_work_sync(&nsim_dev->trap_report_dw);
	devlink_traps_unregister(devlink, nsim_traps_arr,
				 ARRAY_SIZE(nsim_traps_arr));
	devlink_trap_groups_unregister(devlink, nsim_trap_groups_arr,
				       ARRAY_SIZE(nsim_trap_groups_arr));
	kfree(nsim_dev->traps_state_arr);
}

static int nsim_dev_reload(struct devlink *devlink,
			   struct netlink_ext_ack *extack)
{
	struct nsim_dev *nsim_dev = devlink_priv(devlink);
	enum nsim_resource_id res_ids[] = {
		NSIM_RESOURCE_IPV4_FIB, NSIM_RESOURCE_IPV4_FIB_RULES,
		NSIM_RESOURCE_IPV6_FIB, NSIM_RESOURCE_IPV6_FIB_RULES
	};
	int i;

	for (i = 0; i < ARRAY_SIZE(res_ids); ++i) {
		int err;
		u64 val;

		err = devlink_resource_size_get(devlink, res_ids[i], &val);
		if (!err) {
			err = nsim_fib_set_max(nsim_dev->fib_data,
					       res_ids[i], val, extack);
			if (err)
				return err;
		}
	}

	return 0;
}

static const struct devlink_ops nsim_dev_devlink_ops = {
	.reload = nsim_dev_reload,
};

static struct nsim_dev *
nsim_dev_create(struct nsim_bus_dev *nsim_bus_dev, unsigned int port_count)
{
	struct nsim_dev *nsim_dev;
	struct devlink *devlink;
	int err;

	devlink = devlink_alloc(&nsim_dev_devlink_ops, sizeof(*nsim_dev));
	if (!devlink)
		return ERR_PTR(-ENOMEM);
	nsim_dev = devlink_priv(devlink);
	nsim_dev->nsim_bus_dev = nsim_bus_dev;
	nsim_dev->switch_id.id_len = sizeof(nsim_dev->switch_id.id);
	get_random_bytes(nsim_dev->switch_id.id, nsim_dev->switch_id.id_len);
	INIT_LIST_HEAD(&nsim_dev->port_list);
	mutex_init(&nsim_dev->port_list_lock);

	nsim_dev->fib_data = nsim_fib_create();
	if (IS_ERR(nsim_dev->fib_data)) {
		err = PTR_ERR(nsim_dev->fib_data);
		goto err_devlink_free;
	}

	err = nsim_dev_resources_register(devlink);
	if (err)
		goto err_fib_destroy;

	err = devlink_register(devlink, &nsim_bus_dev->dev);
	if (err)
		goto err_resources_unregister;

	err = nsim_dev_traps_init(devlink);
	if (err)
		goto err_dl_unregister;

	err = nsim_dev_debugfs_init(nsim_dev);
	if (err)
		goto err_traps_exit;

	err = nsim_bpf_dev_init(nsim_dev);
	if (err)
		goto err_debugfs_exit;

	return nsim_dev;

err_debugfs_exit:
	nsim_dev_debugfs_exit(nsim_dev);
err_traps_exit:
	nsim_dev_traps_exit(devlink);
err_dl_unregister:
	devlink_unregister(devlink);
err_resources_unregister:
	devlink_resources_unregister(devlink, NULL);
err_fib_destroy:
	nsim_fib_destroy(nsim_dev->fib_data);
err_devlink_free:
	devlink_free(devlink);
	return ERR_PTR(err);
}

static void nsim_dev_destroy(struct nsim_dev *nsim_dev)
{
	struct devlink *devlink = priv_to_devlink(nsim_dev);

	nsim_bpf_dev_exit(nsim_dev);
	nsim_dev_debugfs_exit(nsim_dev);
	nsim_dev_traps_exit(devlink);
	devlink_unregister(devlink);
	devlink_resources_unregister(devlink, NULL);
	nsim_fib_destroy(nsim_dev->fib_data);
	mutex_destroy(&nsim_dev->port_list_lock);
	devlink_free(devlink);
}

static int __nsim_dev_port_add(struct nsim_dev *nsim_dev,
			       unsigned int port_index)
{
	struct nsim_dev_port *nsim_dev_port;
	struct devlink_port *devlink_port;
	int err;

	nsim_dev_port = kzalloc(sizeof(*nsim_dev_port), GFP_KERNEL);
	if (!nsim_dev_port)
		return -ENOMEM;
	nsim_dev_port->port_index = port_index;

	devlink_port = &nsim_dev_port->devlink_port;
	devlink_port_attrs_set(devlink_port, DEVLINK_PORT_FLAVOUR_PHYSICAL,
			       port_index + 1, 0, 0,
			       nsim_dev->switch_id.id,
			       nsim_dev->switch_id.id_len);
	err = devlink_port_register(priv_to_devlink(nsim_dev), devlink_port,
				    port_index);
	if (err)
		goto err_port_free;

	err = nsim_dev_port_debugfs_init(nsim_dev, nsim_dev_port);
	if (err)
		goto err_dl_port_unregister;

	nsim_dev_port->ns = nsim_create(nsim_dev, nsim_dev_port);
	if (IS_ERR(nsim_dev_port->ns)) {
		err = PTR_ERR(nsim_dev_port->ns);
		goto err_port_debugfs_exit;
	}

	devlink_port_type_eth_set(devlink_port, nsim_dev_port->ns->netdev);
	list_add(&nsim_dev_port->list, &nsim_dev->port_list);

	return 0;

err_port_debugfs_exit:
	nsim_dev_port_debugfs_exit(nsim_dev_port);
err_dl_port_unregister:
	devlink_port_unregister(devlink_port);
err_port_free:
	kfree(nsim_dev_port);
	return err;
}

static void __nsim_dev_port_del(struct nsim_dev_port *nsim_dev_port)
{
	struct devlink_port *devlink_port = &nsim_dev_port->devlink_port;

	list_del(&nsim_dev_port->list);
	devlink_port_type_clear(devlink_port);
	nsim_destroy(nsim_dev_port->ns);
	nsim_dev_port_debugfs_exit(nsim_dev_port);
	devlink_port_unregister(devlink_port);
	kfree(nsim_dev_port);
}

static void nsim_dev_port_del_all(struct nsim_dev *nsim_dev)
{
	struct nsim_dev_port *nsim_dev_port, *tmp;

	mutex_lock(&nsim_dev->port_list_lock);
	list_for_each_entry_safe(nsim_dev_port, tmp,
				 &nsim_dev->port_list, list)
		__nsim_dev_port_del(nsim_dev_port);
	mutex_unlock(&nsim_dev->port_list_lock);
}

int nsim_dev_probe(struct nsim_bus_dev *nsim_bus_dev)
{
	struct nsim_dev *nsim_dev;
	int i;
	int err;

	nsim_dev = nsim_dev_create(nsim_bus_dev, nsim_bus_dev->port_count);
	if (IS_ERR(nsim_dev))
		return PTR_ERR(nsim_dev);
	dev_set_drvdata(&nsim_bus_dev->dev, nsim_dev);

	for (i = 0; i < nsim_bus_dev->port_count; i++) {
		err = __nsim_dev_port_add(nsim_dev, i);
		if (err)
			goto err_port_del_all;
	}
	return 0;

err_port_del_all:
	nsim_dev_port_del_all(nsim_dev);
	nsim_dev_destroy(nsim_dev);
	return err;
}

void nsim_dev_remove(struct nsim_bus_dev *nsim_bus_dev)
{
	struct nsim_dev *nsim_dev = dev_get_drvdata(&nsim_bus_dev->dev);

	nsim_dev_port_del_all(nsim_dev);
	nsim_dev_destroy(nsim_dev);
}

static struct nsim_dev_port *
__nsim_dev_port_lookup(struct nsim_dev *nsim_dev, unsigned int port_index)
{
	struct nsim_dev_port *nsim_dev_port;

	list_for_each_entry(nsim_dev_port, &nsim_dev->port_list, list)
		if (nsim_dev_port->port_index == port_index)
			return nsim_dev_port;
	return NULL;
}

int nsim_dev_port_add(struct nsim_bus_dev *nsim_bus_dev,
		      unsigned int port_index)
{
	struct nsim_dev *nsim_dev = dev_get_drvdata(&nsim_bus_dev->dev);
	int err;

	mutex_lock(&nsim_dev->port_list_lock);
	if (__nsim_dev_port_lookup(nsim_dev, port_index))
		err = -EEXIST;
	else
		err = __nsim_dev_port_add(nsim_dev, port_index);
	mutex_unlock(&nsim_dev->port_list_lock);
	return err;
}

int nsim_dev_port_del(struct nsim_bus_dev *nsim_bus_dev,
		      unsigned int port_index)
{
	struct nsim_dev *nsim_dev = dev_get_drvdata(&nsim_bus_dev->dev);
	struct nsim_dev_port *nsim_dev_port;
	int err = 0;

	mutex_lock(&nsim_dev->port_list_lock);
	nsim_dev_port = __nsim_dev_port_lookup(nsim_dev, port_index);
	if (!nsim_dev_port)
		err = -ENOENT;
	else
		__nsim_dev_port_del(nsim_dev_port);
	mutex_unlock(&nsim_dev->port_list_lock);
	return err;
}

int nsim_dev_init(void)
{
	nsim_dev_ddir = debugfs_create_dir(DRV_NAME, NULL);
	if (IS_ERR_OR_NULL(nsim_dev_ddir))
		return -ENOMEM;
	return 0;
}

void nsim_dev_exit(void)
{
	debugfs_remove_recursive(nsim_dev_ddir);
}
