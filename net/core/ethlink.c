// SPDX-License-Identifier: GPL-2.0
/*
 * net/core/ethlink.c - Ethernet netdevice Netlink interface
 *
 * Copyright (c) 2018 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2018 Jiri Pirko <jiri@mellanox.com>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/netdevice.h>
#include <net/netlink.h>
#include <net/genetlink.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <net/ethlink.h>

static struct net_device *
ethlink_netdev_get_from_attrs(struct net *net, struct nlattr **attrs,
			      struct netlink_ext_ack *extack)
{
	struct net_device *dev;

	if (attrs[ETHLINK_ATTR_IFINDEX]) {
		u32 ifindex = nla_get_u32(attrs[ETHLINK_ATTR_IFINDEX]);

		dev = __dev_get_by_index(net, ifindex);
		if (!dev) {
			NL_SET_ERR_MSG_MOD(extack, "Device not found by ifindex");
			return ERR_PTR(-ENODEV);
		}
	} else if (attrs[ETHLINK_ATTR_IFNAME]) {
		const char *ifname = nla_data(attrs[ETHLINK_ATTR_IFNAME]);

		dev = __dev_get_by_name(net, ifname);
		if (!dev) {
			NL_SET_ERR_MSG_MOD(extack, "Device not found by ifname");
			return ERR_PTR(-ENODEV);
		}
	} else {
		NL_SET_ERR_MSG_MOD(extack, "Either ifindex or ifname is required");
		return ERR_PTR(-EINVAL);
	}
	if (!dev->ethlink_ops) {
		NL_SET_ERR_MSG_MOD(extack, "Device does not support ethlink");
		return ERR_PTR(-EOPNOTSUPP);
	}
	return dev;
}

static struct net_device *ethlink_netdev_get_from_info(struct genl_info *info)
{
	return ethlink_netdev_get_from_attrs(genl_info_net(info), info->attrs,
					     info->extack);
}

static int ethlink_nl_pre_doit(const struct genl_ops *ops,
			       struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *dev;

	rtnl_lock();
	dev = ethlink_netdev_get_from_info(info);
	if (IS_ERR(dev)) {
		rtnl_unlock();
		return PTR_ERR(dev);
	}
	info->user_ptr[0] = dev;
	return 0;
}

static void ethlink_nl_post_doit(const struct genl_ops *ops,
				 struct sk_buff *skb, struct genl_info *info)
{
	rtnl_unlock();
}

static struct genl_family ethlink_nl_family;

enum ethlink_multicast_groups {
	ETHLINK_MCGRP_CONFIG,
};

static const struct genl_multicast_group ethlink_nl_mcgrps[] = {
	[ETHLINK_MCGRP_CONFIG] = { .name = ETHLINK_GENL_MCGRP_CONFIG_NAME },
};

static int ethlink_nl_put_handle(struct sk_buff *msg, struct net_device *dev)
{
	if (nla_put_u32(msg, ETHLINK_ATTR_IFINDEX, dev->ifindex) ||
	    nla_put_string(msg, ETHLINK_ATTR_IFNAME, dev->name))
		return -EMSGSIZE;
	return 0;
}

static int ethlink_nl_fill(struct sk_buff *msg, struct net_device *dev,
			   enum ethlink_command cmd, u32 portid,
			   u32 seq, int flags)
{
	void *hdr;

	hdr = genlmsg_put(msg, portid, seq, &ethlink_nl_family, flags, cmd);
	if (!hdr)
		return -EMSGSIZE;

	if (ethlink_nl_put_handle(msg, dev))
		goto nla_put_failure;

	genlmsg_end(msg, hdr);
	return 0;

nla_put_failure:
	genlmsg_cancel(msg, hdr);
	return -EMSGSIZE;
}

static void ethlink_notify(struct net_device *dev, enum ethlink_command cmd)
{
	struct sk_buff *msg;
	int err;

	WARN_ON(cmd != ETHLINK_CMD_NEW && cmd != ETHLINK_CMD_DEL);

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return;

	err = ethlink_nl_fill(msg, dev, cmd, 0, 0, 0);
	if (err) {
		nlmsg_free(msg);
		return;
	}

	genlmsg_multicast_netns(&ethlink_nl_family, dev_net(dev),
				msg, 0, ETHLINK_MCGRP_CONFIG, GFP_KERNEL);
}

void ethlink_notify_new(struct net_device *dev)
{
	if (dev->ethlink_ops)
		ethlink_notify(dev, ETHLINK_CMD_NEW);
}

void ethlink_notify_del(struct net_device *dev)
{
	if (dev->ethlink_ops)
		ethlink_notify(dev, ETHLINK_CMD_DEL);
}

static int ethlink_nl_cmd_get_doit(struct sk_buff *skb, struct genl_info *info)
{
	struct net_device *dev = info->user_ptr[0];
	struct sk_buff *msg;
	int err;

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	err = ethlink_nl_fill(msg, dev, ETHLINK_CMD_NEW,
			      info->snd_portid, info->snd_seq, 0);
	if (err) {
		nlmsg_free(msg);
		return err;
	}

	return genlmsg_reply(msg, info);
}

static int ethlink_nl_cmd_get_dumpit(struct sk_buff *msg,
				     struct netlink_callback *cb)
{
	struct net_device *dev;
	int start = cb->args[0];
	int idx = 0;
	int err;

	rtnl_lock();
	for_each_netdev(sock_net(msg->sk), dev) {
		if (!dev->ethlink_ops)
			continue;
		if (idx < start) {
			idx++;
			continue;
		}
		err = ethlink_nl_fill(msg, dev, ETHLINK_CMD_NEW,
				      NETLINK_CB(cb->skb).portid,
				      cb->nlh->nlmsg_seq, NLM_F_MULTI);
		if (err)
			goto out;
		idx++;
	}
out:
	rtnl_unlock();

	cb->args[0] = idx;
	return msg->len;
}

static const struct nla_policy ethlink_nl_policy[ETHLINK_ATTR_MAX + 1] = {
	[ETHLINK_ATTR_IFINDEX] = { .type = NLA_U32 },
	[ETHLINK_ATTR_IFNAME] = { .type = NLA_NUL_STRING },
};

static const struct genl_ops ethlink_nl_ops[] = {
	{
		.cmd = ETHLINK_CMD_GET,
		.doit = ethlink_nl_cmd_get_doit,
		.dumpit = ethlink_nl_cmd_get_dumpit,
		.policy = ethlink_nl_policy,
		/* can be retrieved by unprivileged users */
	},
};

static struct genl_family ethlink_nl_family __ro_after_init = {
	.name		= ETHLINK_GENL_NAME,
	.version	= ETHLINK_GENL_VERSION,
	.maxattr	= ETHLINK_ATTR_MAX,
	.netnsok	= true,
	.pre_doit	= ethlink_nl_pre_doit,
	.post_doit	= ethlink_nl_post_doit,
	.module		= THIS_MODULE,
	.ops		= ethlink_nl_ops,
	.n_ops		= ARRAY_SIZE(ethlink_nl_ops),
	.mcgrps		= ethlink_nl_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(ethlink_nl_mcgrps),
};

static int __init ethlink_module_init(void)
{
	return genl_register_family(&ethlink_nl_family);
}

static void __exit ethlink_module_exit(void)
{
	genl_unregister_family(&ethlink_nl_family);
}

module_init(ethlink_module_init);
module_exit(ethlink_module_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiri Pirko <jiri@mellanox.com>");
MODULE_DESCRIPTION("Ethernet netdevice Netlink interface");
MODULE_ALIAS_GENL_FAMILY(ETHLINK_GENL_NAME);
