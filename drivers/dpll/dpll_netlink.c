// SPDX-License-Identifier: GPL-2.0
/*
 * Generic netlink for DPLL management framework
 *
 * Copyright (c) 2021 Meta Platforms, Inc. and affiliates
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/genetlink.h>
#include "dpll_core.h"

#include <uapi/linux/dpll.h>

static const struct genl_multicast_group dpll_genl_mcgrps[] = {
	{ .name = DPLL_CONFIG_DEVICE_GROUP_NAME, },
	{ .name = DPLL_CONFIG_SOURCE_GROUP_NAME, },
	{ .name = DPLL_CONFIG_OUTPUT_GROUP_NAME, },
	{ .name = DPLL_MONITOR_GROUP_NAME,  },
};

static const struct nla_policy dpll_genl_get_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_DEVICE_NAME]	= { .type = NLA_STRING,
				    .len = DPLL_NAME_LENGTH },
	[DPLLA_DEVICE_SRC_SELECT_MODE] = { .type = NLA_U32 },
	[DPLLA_FLAGS]		= { .type = NLA_U32 },
};

static const struct nla_policy dpll_genl_set_source_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_TYPE]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_NAME]	= { .type = NLA_STRING,
				    .len = DPLL_NAME_LENGTH },
};

static const struct nla_policy dpll_genl_set_output_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_OUTPUT_ID]	= { .type = NLA_U32 },
	[DPLLA_OUTPUT_TYPE]	= { .type = NLA_U32 },
	[DPLLA_OUTPUT_NAME]	= { .type = NLA_STRING,
				    .len = DPLL_NAME_LENGTH },
};

static const struct nla_policy dpll_genl_set_src_select_mode_policy[] = {
	[DPLLA_DEVICE_ID]		  = { .type = NLA_U32 },
	[DPLLA_DEVICE_SRC_SELECT_MODE] = { .type = NLA_U32 },
};

static const struct nla_policy dpll_genl_set_source_prio_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_PRIO]	= { .type = NLA_U32 },
};

struct param {
	struct netlink_callback *cb;
	struct dpll_device *dpll;
	struct sk_buff *msg;
	int dpll_id;
	int dpll_src_select_mode;
	int dpll_source_id;
	int dpll_source_type;
	int dpll_source_prio;
	int dpll_output_id;
	int dpll_output_type;
	int dpll_status;
	int dpll_event_group;
	const char *dpll_name;
};

struct dpll_dump_ctx {
	struct dpll_device *dev;
	int flags;
	int pos_idx;
	int pos_src_idx;
	int pos_out_idx;
};

typedef int (*cb_t)(struct param *);

static struct genl_family dpll_gnl_family;

static struct dpll_dump_ctx *dpll_dump_context(struct netlink_callback *cb)
{
	return (struct dpll_dump_ctx *)cb->ctx;
}

static int __dpll_cmd_device_dump_one(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	if (nla_put_u32(msg, DPLLA_DEVICE_ID, dpll->id))
		return -EMSGSIZE;

	if (nla_put_string(msg, DPLLA_DEVICE_NAME, dev_name(&dpll->dev)))
		return -EMSGSIZE;

	return 0;
}

static int __dpll_cmd_dump_sources(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	int i, ret = 0, type, prio;
	struct nlattr *src_attr;
	const char *name;

	for (i = 0; i < dpll->sources_count; i++) {
		src_attr = nla_nest_start(msg, DPLLA_SOURCE);
		if (!src_attr) {
			ret = -EMSGSIZE;
			break;
		}
		type = dpll->ops->get_source_type(dpll, i);
		if (nla_put_u32(msg, DPLLA_SOURCE_ID, i) ||
		    nla_put_u32(msg, DPLLA_SOURCE_TYPE, type)) {
			nla_nest_cancel(msg, src_attr);
			ret = -EMSGSIZE;
			break;
		}
		if (dpll->ops->get_source_supported) {
			for (type = 0; type <= DPLL_TYPE_MAX; type++) {
				ret = dpll->ops->get_source_supported(dpll, i, type);
				if (ret && nla_put_u32(msg, DPLLA_SOURCE_SUPPORTED, type)) {
					ret = -EMSGSIZE;
					break;
				}
			}
			ret = 0;
		}
		if (dpll->ops->get_source_prio) {
			prio = dpll->ops->get_source_prio(dpll, i);
			if (nla_put_u32(msg, DPLLA_SOURCE_PRIO, prio)) {
				nla_nest_cancel(msg, src_attr);
				ret = -EMSGSIZE;
				break;
			}
		}
		if (dpll->ops->get_source_name) {
			name = dpll->ops->get_source_name(dpll, i);
			if (name && nla_put_string(msg, DPLLA_SOURCE_NAME,
						   name)) {
				nla_nest_cancel(msg, src_attr);
				ret = -EMSGSIZE;
				break;
			}
		}
		nla_nest_end(msg, src_attr);
	}

	return ret;
}

static int __dpll_cmd_dump_outputs(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	struct nlattr *out_attr;
	int i, ret = 0, type;
	const char *name;

	for (i = 0; i < dpll->outputs_count; i++) {
		out_attr = nla_nest_start(msg, DPLLA_OUTPUT);
		if (!out_attr) {
			ret = -EMSGSIZE;
			break;
		}
		type = dpll->ops->get_output_type(dpll, i);
		if (nla_put_u32(msg, DPLLA_OUTPUT_ID, i) ||
		    nla_put_u32(msg, DPLLA_OUTPUT_TYPE, type)) {
			nla_nest_cancel(msg, out_attr);
			ret = -EMSGSIZE;
			break;
		}
		if (dpll->ops->get_output_supported) {
			for (type = 0; type <= DPLL_TYPE_MAX; type++) {
				ret = dpll->ops->get_output_supported(dpll, i, type);
				if (ret && nla_put_u32(msg, DPLLA_OUTPUT_SUPPORTED, type)) {
					ret = -EMSGSIZE;
					break;
				}
			}
			ret = 0;
		}
		if (dpll->ops->get_output_name) {
			name = dpll->ops->get_output_name(dpll, i);
			if (name && nla_put_string(msg, DPLLA_OUTPUT_NAME,
						   name)) {
				nla_nest_cancel(msg, out_attr);
				ret = -EMSGSIZE;
				break;
			}
		}
		nla_nest_end(msg, out_attr);
	}

	return ret;
}

static int __dpll_cmd_dump_status(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	struct dpll_device_ops *ops = dpll->ops;
	int ret, type, attr;

	if (ops->get_status) {
		ret = ops->get_status(dpll);
		if (nla_put_u32(msg, DPLLA_STATUS, ret))
			return -EMSGSIZE;
	}

	if (ops->get_temp) {
		ret = ops->get_temp(dpll);
		if (nla_put_u32(msg, DPLLA_TEMP, ret))
			return -EMSGSIZE;
	}

	if (ops->get_lock_status) {
		ret = ops->get_lock_status(dpll);
		if (nla_put_u32(msg, DPLLA_LOCK_STATUS, ret))
			return -EMSGSIZE;
	}

	if (ops->get_source_select_mode) {
		ret = ops->get_source_select_mode(dpll);
		if (nla_put_u32(msg, DPLLA_DEVICE_SRC_SELECT_MODE, ret))
			return -EMSGSIZE;
	} else {
		if (nla_put_u32(msg, DPLLA_DEVICE_SRC_SELECT_MODE,
				DPLL_SRC_SELECT_FORCED))
			return -EMSGSIZE;
	}

	if (ops->get_source_select_mode_supported) {
		attr = DPLLA_DEVICE_SRC_SELECT_MODE_SUPPORTED;
		for (type = 0; type <= DPLL_SRC_SELECT_MAX; type++) {
			ret = ops->get_source_select_mode_supported(dpll,
								    type);
			if (ret && nla_put_u32(msg, attr, type)) {
				return -EMSGSIZE;
			}
		}
	} else {
		if (nla_put_u32(msg, DPLLA_DEVICE_SRC_SELECT_MODE_SUPPORTED,
				DPLL_SRC_SELECT_FORCED))
			return -EMSGSIZE;
	}

	return 0;
}

static int
dpll_device_dump_one(struct dpll_device *dpll, struct sk_buff *msg,
		     u32 portid, u32 seq, int flags)
{
	struct nlattr *hdr;
	int ret;

	hdr = genlmsg_put(msg, portid, seq, &dpll_gnl_family, 0,
			  DPLL_CMD_DEVICE_GET);
	if (!hdr)
		return -EMSGSIZE;

	mutex_lock(&dpll->lock);
	ret = __dpll_cmd_device_dump_one(dpll, msg);
	if (ret)
		goto out_unlock;

	if (flags & DPLL_FLAG_SOURCES && dpll->ops->get_source_type) {
		ret = __dpll_cmd_dump_sources(dpll, msg);
		if (ret)
			goto out_unlock;
	}

	if (flags & DPLL_FLAG_OUTPUTS && dpll->ops->get_output_type) {
		ret = __dpll_cmd_dump_outputs(dpll, msg);
		if (ret)
			goto out_unlock;
	}

	if (flags & DPLL_FLAG_STATUS) {
		ret = __dpll_cmd_dump_status(dpll, msg);
		if (ret)
			goto out_unlock;
	}
	mutex_unlock(&dpll->lock);
	genlmsg_end(msg, hdr);

	return 0;

out_unlock:
	mutex_unlock(&dpll->lock);
	genlmsg_cancel(msg, hdr);

	return ret;
}

static int dpll_genl_cmd_set_source(struct sk_buff *skb, struct genl_info *info)
{
	struct dpll_device *dpll = info->user_ptr[0];
	struct nlattr **attrs = info->attrs;
	int ret = 0, src_id, type;

	if (!attrs[DPLLA_SOURCE_ID] ||
	    !attrs[DPLLA_SOURCE_TYPE])
		return -EINVAL;

	if (!dpll->ops->set_source_type)
		return -EOPNOTSUPP;

	src_id = nla_get_u32(attrs[DPLLA_SOURCE_ID]);
	type = nla_get_u32(attrs[DPLLA_SOURCE_TYPE]);

	mutex_lock(&dpll->lock);
	ret = dpll->ops->set_source_type(dpll, src_id, type);
	mutex_unlock(&dpll->lock);

	if (!ret)
		dpll_notify_source_change(dpll->id, src_id, type);

	return ret;
}

static int dpll_genl_cmd_set_output(struct sk_buff *skb, struct genl_info *info)
{
	struct dpll_device *dpll = info->user_ptr[0];
	struct nlattr **attrs = info->attrs;
	int ret = 0, out_id, type;

	if (!attrs[DPLLA_OUTPUT_ID] ||
	    !attrs[DPLLA_OUTPUT_TYPE])
		return -EINVAL;

	if (!dpll->ops->set_output_type)
		return -EOPNOTSUPP;

	out_id = nla_get_u32(attrs[DPLLA_OUTPUT_ID]);
	type = nla_get_u32(attrs[DPLLA_OUTPUT_TYPE]);

	mutex_lock(&dpll->lock);
	ret = dpll->ops->set_output_type(dpll, out_id, type);
	mutex_unlock(&dpll->lock);

	if (!ret)
		dpll_notify_output_change(dpll->id, out_id, type);

	return ret;
}

static int dpll_genl_cmd_set_source_prio(struct sk_buff *skb, struct genl_info *info)
{
	struct dpll_device *dpll = info->user_ptr[0];
	struct nlattr **attrs = info->attrs;
	int ret = 0, src_id, prio;

	if (!attrs[DPLLA_SOURCE_ID] ||
	    !attrs[DPLLA_SOURCE_PRIO])
		return -EINVAL;

	if (!dpll->ops->set_source_prio)
		return -EOPNOTSUPP;

	src_id = nla_get_u32(attrs[DPLLA_SOURCE_ID]);
	prio = nla_get_u32(attrs[DPLLA_SOURCE_PRIO]);

	mutex_lock(&dpll->lock);
	ret = dpll->ops->set_source_prio(dpll, src_id, prio);
	mutex_unlock(&dpll->lock);

	if (!ret)
		dpll_notify_source_prio_change(dpll->id, src_id, prio);

	return ret;
}

static int dpll_genl_cmd_set_select_mode(struct sk_buff *skb, struct genl_info *info)
{
	struct dpll_device *dpll = info->user_ptr[0];
	struct nlattr **attrs = info->attrs;
	int ret = 0, mode;

	if (!attrs[DPLLA_DEVICE_SRC_SELECT_MODE])
		return -EINVAL;

	if (!dpll->ops->set_source_select_mode)
		return -EOPNOTSUPP;

	mode = nla_get_u32(attrs[DPLLA_DEVICE_SRC_SELECT_MODE]);

	mutex_lock(&dpll->lock);
	ret = dpll->ops->set_source_select_mode(dpll, mode);
	mutex_unlock(&dpll->lock);

	if (!ret)
		dpll_notify_source_select_mode_change(dpll->id, mode);

	return ret;
}

static int dpll_device_loop_cb(struct dpll_device *dpll, void *data)
{
	struct dpll_dump_ctx *ctx;
	struct param *p = (struct param *)data;

	ctx = dpll_dump_context(p->cb);

	ctx->pos_idx = dpll->id;

	return dpll_device_dump_one(dpll, p->msg, 0, 0, ctx->flags);
}

static int
dpll_cmd_device_dump(struct sk_buff *skb, struct netlink_callback *cb)
{
	struct dpll_dump_ctx *ctx = dpll_dump_context(cb);
	struct param p = { .cb = cb, .msg = skb };

	return for_each_dpll_device(ctx->pos_idx, dpll_device_loop_cb, &p);
}

static int
dpll_genl_cmd_device_get_id(struct sk_buff *skb, struct genl_info *info)
{
	struct dpll_device *dpll = info->user_ptr[0];
	struct nlattr **attrs = info->attrs;
	struct sk_buff *msg;
	int flags = 0;
	int ret;

	if (attrs[DPLLA_FLAGS])
		flags = nla_get_u32(attrs[DPLLA_FLAGS]);

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;

	ret = dpll_device_dump_one(dpll, msg, info->snd_portid, info->snd_seq,
				   flags);
	if (ret)
		goto out_free_msg;

	return genlmsg_reply(msg, info);

out_free_msg:
	nlmsg_free(msg);
	return ret;

}

static int dpll_genl_cmd_start(struct netlink_callback *cb)
{
	const struct genl_dumpit_info *info = genl_dumpit_info(cb);
	struct dpll_dump_ctx *ctx = dpll_dump_context(cb);

	ctx->dev = NULL;
	if (info->attrs[DPLLA_FLAGS])
		ctx->flags = nla_get_u32(info->attrs[DPLLA_FLAGS]);
	else
		ctx->flags = 0;
	ctx->pos_idx = 0;
	ctx->pos_src_idx = 0;
	ctx->pos_out_idx = 0;
	return 0;
}

static int dpll_pre_doit(const struct genl_ops *ops, struct sk_buff *skb,
						 struct genl_info *info)
{
	struct dpll_device *dpll_id = NULL, *dpll_name = NULL;

	if (!info->attrs[DPLLA_DEVICE_ID] &&
	    !info->attrs[DPLLA_DEVICE_NAME])
		return -EINVAL;

	if (info->attrs[DPLLA_DEVICE_ID]) {
		u32 id = nla_get_u32(info->attrs[DPLLA_DEVICE_ID]);

		dpll_id = dpll_device_get_by_id(id);
		if (!dpll_id)
			return -ENODEV;
		info->user_ptr[0] = dpll_id;
	}
	if (info->attrs[DPLLA_DEVICE_NAME]) {
		const char *name = nla_data(info->attrs[DPLLA_DEVICE_NAME]);

		dpll_name = dpll_device_get_by_name(name);
		if (!dpll_name)
			return -ENODEV;

		if (dpll_id && dpll_name != dpll_id)
			return -EINVAL;
		info->user_ptr[0] = dpll_name;
	}

	return 0;
}

static const struct genl_ops dpll_genl_ops[] = {
	{
		.cmd	= DPLL_CMD_DEVICE_GET,
		.flags  = GENL_UNS_ADMIN_PERM,
		.start	= dpll_genl_cmd_start,
		.dumpit	= dpll_cmd_device_dump,
		.doit	= dpll_genl_cmd_device_get_id,
		.policy	= dpll_genl_get_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_get_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_SOURCE_TYPE,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_set_source,
		.policy	= dpll_genl_set_source_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_source_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_OUTPUT_TYPE,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_set_output,
		.policy	= dpll_genl_set_output_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_output_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_SRC_SELECT_MODE,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_set_select_mode,
		.policy	= dpll_genl_set_src_select_mode_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_src_select_mode_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_SOURCE_PRIO,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_set_source_prio,
		.policy	= dpll_genl_set_source_prio_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_source_prio_policy) - 1,
	},
};

static struct genl_family dpll_gnl_family __ro_after_init = {
	.hdrsize	= 0,
	.name		= DPLL_FAMILY_NAME,
	.version	= DPLL_VERSION,
	.ops		= dpll_genl_ops,
	.n_ops		= ARRAY_SIZE(dpll_genl_ops),
	.mcgrps		= dpll_genl_mcgrps,
	.n_mcgrps	= ARRAY_SIZE(dpll_genl_mcgrps),
	.pre_doit	= dpll_pre_doit,
};

static int dpll_event_device_create(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
	    nla_put_string(p->msg, DPLLA_DEVICE_NAME, p->dpll_name))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_device_delete(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_status(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
		nla_put_u32(p->msg, DPLLA_LOCK_STATUS, p->dpll_status))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_source_change(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
	    nla_put_u32(p->msg, DPLLA_SOURCE_ID, p->dpll_source_id) ||
		nla_put_u32(p->msg, DPLLA_SOURCE_TYPE, p->dpll_source_type))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_output_change(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
	    nla_put_u32(p->msg, DPLLA_OUTPUT_ID, p->dpll_output_id) ||
		nla_put_u32(p->msg, DPLLA_OUTPUT_TYPE, p->dpll_output_type))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_source_prio(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
	    nla_put_u32(p->msg, DPLLA_SOURCE_ID, p->dpll_source_id) ||
	    nla_put_u32(p->msg, DPLLA_SOURCE_PRIO, p->dpll_source_prio))
		return -EMSGSIZE;

	return 0;
}

static int dpll_event_select_mode(struct param *p)
{
	if (nla_put_u32(p->msg, DPLLA_DEVICE_ID, p->dpll_id) ||
	    nla_put_u32(p->msg, DPLLA_DEVICE_SRC_SELECT_MODE,
		    p->dpll_src_select_mode))
		return -EMSGSIZE;

	return 0;
}

static const cb_t event_cb[] = {
	[DPLL_EVENT_DEVICE_CREATE]	= dpll_event_device_create,
	[DPLL_EVENT_DEVICE_DELETE]	= dpll_event_device_delete,
	[DPLL_EVENT_STATUS_LOCKED]	= dpll_event_status,
	[DPLL_EVENT_STATUS_UNLOCKED]	= dpll_event_status,
	[DPLL_EVENT_SOURCE_CHANGE]	= dpll_event_source_change,
	[DPLL_EVENT_OUTPUT_CHANGE]	= dpll_event_output_change,
	[DPLL_EVENT_SOURCE_PRIO]        = dpll_event_source_prio,
	[DPLL_EVENT_SELECT_MODE]        = dpll_event_select_mode,
};

/*
 * Generic netlink DPLL event encoding
 */
static int dpll_send_event(enum dpll_genl_event event,
				   struct param *p)
{
	struct sk_buff *msg;
	int ret = -EMSGSIZE;
	void *hdr;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;
	p->msg = msg;

	hdr = genlmsg_put(msg, 0, 0, &dpll_gnl_family, 0, event);
	if (!hdr)
		goto out_free_msg;

	ret = event_cb[event](p);
	if (ret)
		goto out_cancel_msg;

	genlmsg_end(msg, hdr);

	genlmsg_multicast(&dpll_gnl_family, msg, 0, p->dpll_event_group, GFP_KERNEL);

	return 0;

out_cancel_msg:
	genlmsg_cancel(msg, hdr);
out_free_msg:
	nlmsg_free(msg);

	return ret;
}

int dpll_notify_device_create(int dpll_id, const char *name)
{
	struct param p = { .dpll_id = dpll_id, .dpll_name = name,
			   .dpll_event_group = 0 };

	return dpll_send_event(DPLL_EVENT_DEVICE_CREATE, &p);
}

int dpll_notify_device_delete(int dpll_id)
{
	struct param p = { .dpll_id = dpll_id, .dpll_event_group = 0 };

	return dpll_send_event(DPLL_EVENT_DEVICE_DELETE, &p);
}

int dpll_notify_status_locked(int dpll_id)
{
	struct param p = { .dpll_id = dpll_id, .dpll_status = 1,
			   .dpll_event_group = 3 };

	return dpll_send_event(DPLL_EVENT_STATUS_LOCKED, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_status_locked);

int dpll_notify_status_unlocked(int dpll_id)
{
	struct param p = { .dpll_id = dpll_id, .dpll_status = 0,
			   .dpll_event_group = 3 };

	return dpll_send_event(DPLL_EVENT_STATUS_UNLOCKED, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_status_unlocked);

int dpll_notify_source_change(int dpll_id, int source_id, int source_type)
{
	struct param p =  { .dpll_id = dpll_id, .dpll_source_id = source_id,
			    .dpll_source_type = source_type, .dpll_event_group = 1 };

	return dpll_send_event(DPLL_EVENT_SOURCE_CHANGE, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_source_change);

int dpll_notify_output_change(int dpll_id, int output_id, int output_type)
{
	struct param p =  { .dpll_id = dpll_id, .dpll_output_id = output_id,
			    .dpll_output_type = output_type, .dpll_event_group = 2 };

	return dpll_send_event(DPLL_EVENT_OUTPUT_CHANGE, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_output_change);

int dpll_notify_source_select_mode_change(int dpll_id, int new_mode)
{
	struct param p =  { .dpll_id = dpll_id,
			    .dpll_src_select_mode = new_mode,
			    .dpll_event_group = 0 };

	return dpll_send_event(DPLL_EVENT_SELECT_MODE, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_source_select_mode_change);

int dpll_notify_source_prio_change(int dpll_id, int source_id, int prio)
{
	struct param p =  { .dpll_id = dpll_id, .dpll_source_id = source_id,
			    .dpll_source_prio = prio,
			    .dpll_event_group = 1 };

	return dpll_send_event(DPLL_EVENT_SOURCE_PRIO, &p);
}
EXPORT_SYMBOL_GPL(dpll_notify_source_prio_change);

int __init dpll_netlink_init(void)
{
	return genl_register_family(&dpll_gnl_family);
}

void dpll_netlink_finish(void)
{
	genl_unregister_family(&dpll_gnl_family);
}

void __exit dpll_netlink_fini(void)
{
	dpll_netlink_finish();
}
