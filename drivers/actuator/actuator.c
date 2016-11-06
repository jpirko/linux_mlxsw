/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * drivers/actuator/actuator.c - Generic actuator support
 * Copyright (c) 2018 Jiri Pirko <jiri@resnulli.us>
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

struct actuator_vector {
	actuator_vector_items items;
	union {
		u32 value;
		u32 inc_value;
	};
	enum actuator_dir dir;
	u32 speed;
};

bool actuator_vector_move_get(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_MOVE;
}
EXPORT_SYMBOL_GPL(actuator_vector_move_get);

void actuator_vector_move_set(struct actuator_vector *vector)
{
	vector->items |= ACTUATOR_VECTOR_MOVE;
}
EXPORT_SYMBOL_GPL(actuator_vector_move_set);

bool actuator_vector_stop_get(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_STOP;
}
EXPORT_SYMBOL_GPL(actuator_vector_stop_get);

void actuator_vector_stop_set(struct actuator_vector *vector)
{
	vector->items |= ACTUATOR_VECTOR_STOP;
}
EXPORT_SYMBOL_GPL(actuator_vector_stop_set);

bool actuator_vector_value_present(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_VALUE;
}
EXPORT_SYMBOL_GPL(actuator_vector_value_present);

u32 actuator_vector_value_get(struct actuator_vector *vector)
{
	WARN_ON(!actuator_vector_value_present(vector));
	return vector->value;
}
EXPORT_SYMBOL_GPL(actuator_vector_value_get);

void actuator_vector_value_set(struct actuator_vector *vector, u32 value)
{
	vector->value = value;
	vector->items |= ACTUATOR_VECTOR_VALUE;
}
EXPORT_SYMBOL_GPL(actuator_vector_value_set);

bool actuator_vector_inc_value_present(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_INC_VALUE;
}
EXPORT_SYMBOL_GPL(actuator_vector_inc_value_present);

u32 actuator_vector_inc_value_get(struct actuator_vector *vector)
{
	WARN_ON(!actuator_vector_inc_value_present(vector));
	return vector->inc_value;
}
EXPORT_SYMBOL_GPL(actuator_vector_inc_value_get);

void actuator_vector_inc_value_set(struct actuator_vector *vector,
				   u32 inc_value)
{
	vector->inc_value = inc_value;
	vector->items |= ACTUATOR_VECTOR_INC_VALUE;
}
EXPORT_SYMBOL_GPL(actuator_vector_inc_value_set);

bool actuator_vector_dir_present(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_DIR;
}
EXPORT_SYMBOL_GPL(actuator_vector_dir_present);

enum actuator_dir actuator_vector_dir_get(struct actuator_vector *vector)
{
	WARN_ON(!actuator_vector_dir_present(vector));
	return vector->dir;
}
EXPORT_SYMBOL_GPL(actuator_vector_dir_get);

void actuator_vector_dir_set(struct actuator_vector *vector,
			     enum actuator_dir dir)
{
	vector->dir = dir;
	vector->items |= ACTUATOR_VECTOR_DIR;
}
EXPORT_SYMBOL_GPL(actuator_vector_dir_set);

bool actuator_vector_speed_present(struct actuator_vector *vector)
{
	return vector->items & ACTUATOR_VECTOR_SPEED;
}
EXPORT_SYMBOL_GPL(actuator_vector_speed_present);

u32 actuator_vector_speed_get(struct actuator_vector *vector)
{
	WARN_ON(!actuator_vector_speed_present(vector));
	return vector->speed;
}
EXPORT_SYMBOL_GPL(actuator_vector_speed_get);

void actuator_vector_speed_set(struct actuator_vector *vector, u32 speed)
{
	vector->speed = speed;
	vector->items |= ACTUATOR_VECTOR_SPEED;
}
EXPORT_SYMBOL_GPL(actuator_vector_speed_set);

struct actuator {
	struct list_head list;
	struct device *dev;
	unsigned int index;
	const struct actuator_ops *aco;
	struct actuator_vector tmp_vector;
	unsigned long priv[0];
};

void *actuator_priv(struct actuator *act)
{
	return act->priv;
}
EXPORT_SYMBOL_GPL(actuator_priv);

struct actuator_vector *actuator_clean_tmp_vector(struct actuator *act)
{
	memset(&act->tmp_vector, 0, sizeof(act->tmp_vector));
	return &act->tmp_vector;
}
EXPORT_SYMBOL_GPL(actuator_clean_tmp_vector);

static struct genl_family actuator_nl_family;

enum actuator_multicast_groups {
	ACTUATOR_MCGRP_CONFIG,
};

static const struct genl_multicast_group actuator_nl_mcgrps[] = {
	[ACTUATOR_MCGRP_CONFIG] = { .name = ACTUATOR_GENL_MCGRP_CONFIG_NAME },
};

static int actuator_nl_put_vector(struct sk_buff *msg,
				  struct actuator_vector *vector)
{
	struct nlattr *vector_attr;

	vector_attr = nla_nest_start(msg, ACTUATOR_ATTR_VECTOR);
	if (!vector_attr)
		return -EMSGSIZE;

	if (actuator_vector_move_get(vector) &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_MOVE))
		return -EMSGSIZE;
	if (actuator_vector_stop_get(vector) &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_STOP))
		return -EMSGSIZE;
	if (actuator_vector_value_present(vector) &&
	    nla_put_u32(msg, ACTUATOR_ATTR_VECTOR_VALUE,
		        actuator_vector_value_get(vector)))
		return -EMSGSIZE;
	if (actuator_vector_inc_value_present(vector) &&
	    nla_put_u32(msg, ACTUATOR_ATTR_VECTOR_INC_VALUE,
		        actuator_vector_inc_value_get(vector)))
		return -EMSGSIZE;
	if (actuator_vector_dir_present(vector) &&
	    nla_put_u8(msg, ACTUATOR_ATTR_VECTOR_DIR,
		       actuator_vector_dir_get(vector)))
		return -EMSGSIZE;
	if (actuator_vector_speed_present(vector) &&
	    nla_put_u32(msg, ACTUATOR_ATTR_VECTOR_SPEED,
		        actuator_vector_speed_get(vector)))
		return -EMSGSIZE;

	nla_nest_end(msg, vector_attr);
	return 0;
}

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

static int actuator_nl_put_vector_template(struct sk_buff *msg,
					   actuator_vector_items items)
{
	struct nlattr *template_attr;

	template_attr = nla_nest_start(msg, ACTUATOR_ATTR_VECTOR);
	if (!template_attr)
		return -EMSGSIZE;

	if (items & ACTUATOR_VECTOR_MOVE &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_MOVE))
		return -EMSGSIZE;
	if (items & ACTUATOR_VECTOR_STOP &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_STOP))
		return -EMSGSIZE;
	if (items & ACTUATOR_VECTOR_VALUE &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_VALUE))
		return -EMSGSIZE;
	if (items & ACTUATOR_VECTOR_INC_VALUE &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_INC_VALUE))
		return -EMSGSIZE;
	if (items & ACTUATOR_VECTOR_DIR &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_DIR))
		return -EMSGSIZE;
	if (items & ACTUATOR_VECTOR_SPEED &&
	    nla_put_flag(msg, ACTUATOR_ATTR_VECTOR_SPEED))
		return -EMSGSIZE;

	nla_nest_end(msg, template_attr);
	return 0;
}

static int actuator_nl_put_vector_templates(struct sk_buff *msg,
					    struct actuator *act)
{
	struct nlattr *templates_attr;
	int i;
	int err;

	templates_attr = nla_nest_start(msg, ACTUATOR_ATTR_VECTOR_TEMPLATES);
	if (!templates_attr)
		return -EMSGSIZE;

	for (i = 0; i < act->aco->vector_set_ops_count; i++) {
		const struct actuator_vector_set_op *vector_set_op;

		vector_set_op = &act->aco->vector_set_ops[i];
		err = actuator_nl_put_vector_template(msg, vector_set_op->items);
		if (err)
			return -EMSGSIZE;
	}

	nla_nest_end(msg, templates_attr);
	return 0;
}

static int actuator_nl_fill(struct sk_buff *msg, struct actuator *act,
			    enum actuator_command cmd,
			    struct actuator_vector *vector, bool fill_vector,
			    u32 portid, u32 seq, int flags)
{
	void *hdr;

	if (!vector && fill_vector) {
		struct actuator_vector tmp_vector = {0};
		int err;

		err = act->aco->vector_get(act, &tmp_vector);
		if (err)
			return err;
		vector = &tmp_vector;
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
	if (actuator_nl_put_vector_templates(msg, act))
		goto nla_put_failure;
	if (fill_vector && actuator_nl_put_vector(msg, vector))
		goto nla_put_failure;

	genlmsg_end(msg, hdr);
	return 0;

nla_put_failure:
	genlmsg_cancel(msg, hdr);
	return -EMSGSIZE;
}

static void actuator_notify(struct actuator *act, enum actuator_command cmd,
			    struct actuator_vector *vector, bool fill_vector)
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

	err = actuator_nl_fill(msg, act, cmd, vector, fill_vector, 0, 0, 0);
	if (err) {
		dev_err(act->dev, "Failed to fill-up actuator notification message\n");
		nlmsg_free(msg);
		return;
	}

	genlmsg_multicast(&actuator_nl_family, msg, 0,
			  ACTUATOR_MCGRP_CONFIG, GFP_KERNEL);
}

/**
 *	actuator_notify_vector - Send actuator notifications
 *
 *	@act: actuator
 *	@vector: vector
 *
 *	Called by the driver to notify about async vector change.
 */
void actuator_notify_vector(struct actuator *act,
			    struct actuator_vector *vector)
{
	actuator_notify(act, ACTUATOR_CMD_GET, vector, true);
}
EXPORT_SYMBOL_GPL(actuator_notify_vector);

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
	if (!msg) {
		NL_SET_ERR_MSG(info->extack, "Failed to allocate memory for reply message");
		return -ENOMEM;
	}

	err = actuator_nl_fill(msg, act, ACTUATOR_CMD_GET, NULL, true,
			       info->snd_portid, info->snd_seq, 0);
	if (err) {
		NL_SET_ERR_MSG(info->extack, "Failed to fill-up reply message");
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

static const struct nla_policy
actuator_vector_nl_policy[ACTUATOR_ATTR_VECTOR_MAX + 1] = {
	[ACTUATOR_ATTR_VECTOR_MOVE] = { .type = NLA_FLAG },
	[ACTUATOR_ATTR_VECTOR_STOP] = { .type = NLA_FLAG },
	[ACTUATOR_ATTR_VECTOR_VALUE] = { .type = NLA_U32 },
	[ACTUATOR_ATTR_VECTOR_INC_VALUE] = { .type = NLA_U32 },
	[ACTUATOR_ATTR_VECTOR_DIR] = { .type = NLA_U8 },
	[ACTUATOR_ATTR_VECTOR_SPEED] = { .type = NLA_U32 },
};

static int actuator_vector_parse(struct actuator_vector *vector,
				 struct nlattr **vector_attrs,
				 struct netlink_ext_ack *extack)
{
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_VALUE] &&
	    vector_attrs[ACTUATOR_ATTR_VECTOR_INC_VALUE]) {
		NL_SET_ERR_MSG(extack, "Set message contains both value and incremental value attributes");
		return -EINVAL;
	}
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_MOVE])
		actuator_vector_move_set(vector);
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_STOP])
		actuator_vector_stop_set(vector);
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_VALUE]) {
		u32 value;

		value = nla_get_u32(vector_attrs[ACTUATOR_ATTR_VECTOR_VALUE]);
		actuator_vector_value_set(vector, value);
	}
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_INC_VALUE]) {
		u32 inc_value;

		inc_value = nla_get_u32(vector_attrs[ACTUATOR_ATTR_VECTOR_INC_VALUE]);
		actuator_vector_inc_value_set(vector, inc_value);
	}
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_DIR]) {
		u8 dir;

		dir = nla_get_u8(vector_attrs[ACTUATOR_ATTR_VECTOR_DIR]);
		if (dir != ACTUATOR_DIR_POSITIVE &&
		    dir != ACTUATOR_DIR_NEGATIVE) {
			NL_SET_ERR_MSG(extack, "Invalid direction attribute value");
			return -EINVAL;
		}
		actuator_vector_dir_set(vector, dir);
	}
	if (vector_attrs[ACTUATOR_ATTR_VECTOR_SPEED]) {
		u32 speed;

		speed = nla_get_u32(vector_attrs[ACTUATOR_ATTR_VECTOR_SPEED]);
		actuator_vector_speed_set(vector, speed);
	}
	return 0;
}

static const struct actuator_vector_set_op *
actuator_vector_set_op_find(struct actuator *act,
			    struct actuator_vector *vector)
{
	int i;

	for (i = 0; i < act->aco->vector_set_ops_count; i++) {
		if (act->aco->vector_set_ops[i].items == vector->items)
			return &act->aco->vector_set_ops[i];
	}
	return NULL;
}

static int actuator_nl_cmd_set_doit(struct sk_buff *skb,
				    struct genl_info *info)
{
	struct nlattr *vector_attrs[ACTUATOR_ATTR_VECTOR_MAX + 1];
	const struct actuator_vector_set_op *vector_set_op;
	struct actuator *act = info->user_ptr[0];
	struct actuator_vector vector = {0};
	int err;

	if (!info->attrs[ACTUATOR_ATTR_VECTOR]) {
		NL_SET_ERR_MSG(info->extack, "Set message does not contain vector attribute");
		return -EINVAL;
	}
	err = nla_parse_nested(vector_attrs, ACTUATOR_ATTR_VECTOR_MAX,
			       info->attrs[ACTUATOR_ATTR_VECTOR],
			       actuator_vector_nl_policy, info->extack);
	if (err) {
		NL_SET_ERR_MSG(info->extack, "Failed to parse vector attribute");
		return -EINVAL;
	}

	err = actuator_vector_parse(&vector, vector_attrs, info->extack);
	if (err)
		return err;

	vector_set_op = actuator_vector_set_op_find(act, &vector);
	if (!vector_set_op) {
		NL_SET_ERR_MSG(info->extack, "Device does not support this set operation for specified vector");
		return -EOPNOTSUPP;
	}

	err = vector_set_op->op(act, &vector);
	if (err) {
		NL_SET_ERR_MSG(info->extack, "Device failed to set vector");
		return err;
	}
	return 0;
}

static struct actuator *actuator_get_from_attrs(struct net *net,
						struct nlattr **attrs,
						struct netlink_ext_ack *extack)
{
	struct actuator *act;
	char *busname;
	char *devname;
	u32 index;

	if (!attrs[ACTUATOR_ATTR_BUS_NAME] ||
	    !attrs[ACTUATOR_ATTR_DEV_NAME] ||
	    !attrs[ACTUATOR_ATTR_INDEX]) {
		NL_SET_ERR_MSG(extack, "Failed to get actuator handle from user");
		return ERR_PTR(-EINVAL);
	}

	busname = nla_data(attrs[ACTUATOR_ATTR_BUS_NAME]);
	devname = nla_data(attrs[ACTUATOR_ATTR_DEV_NAME]);
	index = nla_get_u32(attrs[ACTUATOR_ATTR_INDEX]);

	list_for_each_entry(act, &actuator_list, list) {
		if (strcmp(act->dev->bus->name, busname) == 0 &&
		    strcmp(dev_name(act->dev), devname) == 0 &&
		    act->index == index)
			return act;
	}

	NL_SET_ERR_MSG(extack, "Failed to find requested actuator device");
	return ERR_PTR(-ENODEV);
}

static struct actuator *actuator_get_from_info(struct genl_info *info)
{
	return actuator_get_from_attrs(genl_info_net(info),
				       info->attrs, info->extack);
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
	[ACTUATOR_ATTR_VECTOR] = { .type = NLA_NESTED },
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
		.cmd = ACTUATOR_CMD_SET,
		.doit = actuator_nl_cmd_set_doit,
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

	WARN_ON(!aco->vector_get || !aco->driver_name ||
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
