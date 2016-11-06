/*
 * drivers/actuator/actuator.c - Generic actuator support
 * Copyright (c) 2016-2017 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <net/genetlink.h>
#include <net/netlink.h>
#include <uapi/linux/actuator.h>
#include <linux/actuator.h>

static LIST_HEAD(actuator_list);
static DEFINE_MUTEX(actuator_mutex);

struct actuator {
	struct list_head list;
	struct device *dev;
	unsigned int index;
	const struct actuator_ops *aco;
	unsigned long priv[0];
};

void *actuator_priv(struct actuator *act)
{
	return act->priv;
}
EXPORT_SYMBOL_GPL(actuator_priv);

static struct genl_family actuator_nl_family;

enum actuator_multicast_groups {
	ACTUATOR_MCGRP_CONFIG,
};

static const struct genl_multicast_group actuator_nl_mcgrps[] = {
	[ACTUATOR_MCGRP_CONFIG] = { .name = ACTUATOR_GENL_MCGRP_CONFIG_NAME },
};

static int actuator_nl_put_handle(struct sk_buff *msg, struct actuator *act)
{
	if (nla_put_string(msg, ACTUATOR_ATTR_BUS_NAME, act->dev->bus->name))
		return -EMSGSIZE;
	if (nla_put_string(msg, ACTUATOR_ATTR_DEV_NAME, dev_name(act->dev)))
		return -EMSGSIZE;
	if (nla_put_u32(msg, ACTUATOR_ATTR_INDEX, act->index))
		return -EMSGSIZE;
	return 0;
}

static int actuator_nl_fill(struct sk_buff *msg, struct actuator *act,
			    enum actuator_command cmd,
			    struct actuator_status *status, bool fill_status,
			    u32 portid, u32 seq, int flags)
{
	void *hdr;

	if (!status && fill_status) {
		struct actuator_status tmp_status;
		int err;

		err = act->aco->status(act, &tmp_status);
		if (err)
			return err;
		status = &tmp_status;
	}

	hdr = genlmsg_put(msg, portid, seq, &actuator_nl_family, flags, cmd);
	if (!hdr)
		return -EMSGSIZE;

	if (actuator_nl_put_handle(msg, act))
		goto nla_put_failure;
	if (nla_put_string(msg, ACTUATOR_ATTR_DRIVER_NAME,
			   act->aco->driver_name))
		goto nla_put_failure;
	if (nla_put_u8(msg, ACTUATOR_ATTR_TYPE, act->aco->type))
		goto nla_put_failure;
	if (nla_put_u8(msg, ACTUATOR_ATTR_UNITS, act->aco->units))
		goto nla_put_failure;
	if (fill_status) {
		if (nla_put_u32(msg, ACTUATOR_ATTR_VALUE, status->value))
			goto nla_put_failure;
		if (nla_put_u8(msg, ACTUATOR_ATTR_MOVE_STATE,
			       status->move_state))
			goto nla_put_failure;
	}

	genlmsg_end(msg, hdr);
	return 0;

nla_put_failure:
	genlmsg_cancel(msg, hdr);
	return -EMSGSIZE;
}

static void actuator_notify(struct actuator *act, enum actuator_command cmd,
			    struct actuator_status *status, bool fill_status)
{
	struct sk_buff *msg;
	int err;

	WARN_ON(cmd != ACTUATOR_CMD_GET &&
		cmd != ACTUATOR_CMD_NEW &&
		cmd != ACTUATOR_CMD_DEL);

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg) {
		dev_err(act->dev, "Failed to allocate actuator notification message\n");
		return;
	}

	err = actuator_nl_fill(msg, act, cmd, status, fill_status, 0, 0, 0);
	if (err) {
		dev_err(act->dev, "Failed to fill-up actuator notification message\n");
		nlmsg_free(msg);
		return;
	}

	genlmsg_multicast(&actuator_nl_family, msg, 0,
			  ACTUATOR_MCGRP_CONFIG, GFP_KERNEL);
}

/**
 *	actuator_notify_status - Send actuator notifications
 *
 *	@act: actuator
 *	@status: status
 *
 *	Called by the driver to notify about async status change.
 */
void actuator_notify_status(struct actuator *act,
			    struct actuator_status *status)
{
	actuator_notify(act, ACTUATOR_CMD_GET, status, true);
}
EXPORT_SYMBOL_GPL(actuator_notify_status);

static void actuator_notify_new(struct actuator *act)
{
	actuator_notify(act, ACTUATOR_CMD_NEW, NULL, true);
}

static void actuator_notify_del(struct actuator *act)
{
	actuator_notify(act, ACTUATOR_CMD_DEL, NULL, false);
}

static int actuator_nl_cmd_get_doit(struct sk_buff *skb, struct genl_info *info)
{
	struct actuator *act = info->user_ptr[0];
	struct sk_buff *msg;
	int err;

	msg = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	err = actuator_nl_fill(msg, act, ACTUATOR_CMD_GET, NULL, true,
			       info->snd_portid, info->snd_seq, 0);
	if (err) {
		nlmsg_free(msg);
		return err;
	}

	return genlmsg_reply(msg, info);
}

static int actuator_nl_cmd_get_dumpit(struct sk_buff *msg,
				      struct netlink_callback *cb)
{
	struct actuator *act;
	int start = cb->args[0];
	int idx = 0;
	int err;

	mutex_lock(&actuator_mutex);
	list_for_each_entry(act, &actuator_list, list) {
		if (idx < start) {
			idx++;
			continue;
		}
		err = actuator_nl_fill(msg, act, ACTUATOR_CMD_GET, NULL, true,
				       NETLINK_CB(cb->skb).portid,
				       cb->nlh->nlmsg_seq, NLM_F_MULTI);
		if (err)
			goto out;
		idx++;
	}
out:
	mutex_unlock(&actuator_mutex);

	cb->args[0] = idx;
	return msg->len;
}

static int actuator_nl_cmd_move_doit(struct sk_buff *skb,
				     struct genl_info *info)
{
	struct actuator *act = info->user_ptr[0];
	u32 value;

	if (!info->attrs[ACTUATOR_ATTR_VALUE])
		return -EINVAL;

	value = nla_get_u32(info->attrs[ACTUATOR_ATTR_VALUE]);
	if (act->aco->move)
		return act->aco->move(act, value);
	return -EOPNOTSUPP;
}

static int actuator_nl_cmd_stop_doit(struct sk_buff *skb,
				     struct genl_info *info)
{
	struct actuator *act = info->user_ptr[0];

	if (act->aco->stop)
		return act->aco->stop(act);
	return -EOPNOTSUPP;
}

static struct actuator *actuator_get_from_attrs(struct net *net,
						struct nlattr **attrs)
{
	struct actuator *act;
	char *busname;
	char *devname;
	u32 index;

	if (!attrs[ACTUATOR_ATTR_BUS_NAME] ||
	    !attrs[ACTUATOR_ATTR_DEV_NAME] ||
	    !attrs[ACTUATOR_ATTR_INDEX])
		return ERR_PTR(-EINVAL);

	busname = nla_data(attrs[ACTUATOR_ATTR_BUS_NAME]);
	devname = nla_data(attrs[ACTUATOR_ATTR_DEV_NAME]);
	index = nla_get_u32(attrs[ACTUATOR_ATTR_INDEX]);

	list_for_each_entry(act, &actuator_list, list) {
		if (strcmp(act->dev->bus->name, busname) == 0 &&
		    strcmp(dev_name(act->dev), devname) == 0 &&
		    act->index == index)
			return act;
	}

	return ERR_PTR(-ENODEV);
}

static struct actuator *actuator_get_from_info(struct genl_info *info)
{
	return actuator_get_from_attrs(genl_info_net(info), info->attrs);
}

static int actuator_nl_pre_doit(const struct genl_ops *ops,
				struct sk_buff *skb, struct genl_info *info)
{
	struct actuator *act;

	mutex_lock(&actuator_mutex);
	act = actuator_get_from_info(info);
	if (IS_ERR(act)) {
		mutex_unlock(&actuator_mutex);
		return PTR_ERR(act);
	}
	info->user_ptr[0] = act;
	return 0;
}

static void actuator_nl_post_doit(const struct genl_ops *ops,
				  struct sk_buff *skb, struct genl_info *info)
{
	mutex_unlock(&actuator_mutex);
}

static const struct nla_policy actuator_nl_policy[ACTUATOR_ATTR_MAX + 1] = {
	[ACTUATOR_ATTR_BUS_NAME] = { .type = NLA_NUL_STRING },
	[ACTUATOR_ATTR_DEV_NAME] = { .type = NLA_NUL_STRING },
	[ACTUATOR_ATTR_INDEX] = { .type = NLA_U32 },
	[ACTUATOR_ATTR_VALUE] = { .type = NLA_U32 },
};

static const struct genl_ops actuator_nl_ops[] = {
	{
		.cmd = ACTUATOR_CMD_GET,
		.doit = actuator_nl_cmd_get_doit,
		.dumpit = actuator_nl_cmd_get_dumpit,
		.policy = actuator_nl_policy,
		/* can be retrieved by unprivileged users */
	},
	{
		.cmd = ACTUATOR_CMD_MOVE,
		.doit = actuator_nl_cmd_move_doit,
		.policy = actuator_nl_policy,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = ACTUATOR_CMD_STOP,
		.doit = actuator_nl_cmd_stop_doit,
		.policy = actuator_nl_policy,
		.flags = GENL_ADMIN_PERM,
	},
};

static struct genl_family actuator_nl_family __ro_after_init = {
	.name		= ACTUATOR_GENL_NAME,
	.version	= ACTUATOR_GENL_VERSION,
	.maxattr	= ACTUATOR_ATTR_MAX,
	.pre_doit	= actuator_nl_pre_doit,
	.post_doit	= actuator_nl_post_doit,
	.module		= THIS_MODULE,
	.ops		= actuator_nl_ops,
	.n_ops		= ARRAY_SIZE(actuator_nl_ops),
	.mcgrps		= actuator_nl_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(actuator_nl_mcgrps),
};

/**
 *	actuator_alloc - Allocate new actuator instance resources
 *
 *	@aco: actuator instance ops
 *
 *	Allocate new actuator instance resources.
 */
struct actuator *actuator_alloc(const struct actuator_ops *aco)
{
	struct actuator *act;

	WARN_ON(!aco->status || !aco->driver_name ||
		aco->type == ACTUATOR_TYPE_UNSPEC ||
		aco->units == ACTUATOR_UNITS_UNSPEC);
	act = kzalloc(sizeof(struct actuator) + aco->priv_size, GFP_KERNEL);
	if (!act)
		return NULL;
	act->aco = aco;
	return act;
}
EXPORT_SYMBOL_GPL(actuator_alloc);

/**
 *	actuator_free - Free actuator instance resources
 *
 *	@act: actuator
 */
void actuator_free(struct actuator *act)
{
	kfree(act);
}
EXPORT_SYMBOL_GPL(actuator_free);

/**
 *	actuator_register - Register actuator instance
 *
 *	@act: actuator
 *	@dev: parent device
 *	@index: index within the parent device
 */
int actuator_register(struct actuator *act, struct device *dev,
		      unsigned int index)
{
	act->dev = dev;
	act->index = index;
	mutex_lock(&actuator_mutex);
	list_add_tail(&act->list, &actuator_list);
	mutex_unlock(&actuator_mutex);
	actuator_notify_new(act);
	return 0;
}
EXPORT_SYMBOL_GPL(actuator_register);

/**
 *	actuator_unregister - Unregister actuator instance
 *
 *	@act: actuator
 */
void actuator_unregister(struct actuator *act)
{
	mutex_lock(&actuator_mutex);
	list_del(&act->list);
	mutex_unlock(&actuator_mutex);
	actuator_notify_del(act);
}
EXPORT_SYMBOL_GPL(actuator_unregister);

static int __init actuator_init(void)
{
	return genl_register_family(&actuator_nl_family);
}

static void __exit actuator_exit(void)
{
	genl_unregister_family(&actuator_nl_family);
}

module_init(actuator_init);
module_exit(actuator_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiri Pirko <jiri@resnulli.us>");
MODULE_DESCRIPTION("Generic actuator support");
MODULE_ALIAS_GENL_FAMILY(ACTUATOR_GENL_NAME);
