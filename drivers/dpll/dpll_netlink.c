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
	[DPLLA_FLAGS]		= { .type = NLA_U32 },
};

static const struct nla_policy dpll_genl_set_source_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_ID]	= { .type = NLA_U32 },
	[DPLLA_SOURCE_TYPE]	= { .type = NLA_U32 },
};

static const struct nla_policy dpll_genl_set_output_policy[] = {
	[DPLLA_DEVICE_ID]	= { .type = NLA_U32 },
	[DPLLA_OUTPUT_ID]	= { .type = NLA_U32 },
	[DPLLA_OUTPUT_TYPE]	= { .type = NLA_U32 },
};

struct param {
	struct netlink_callback *cb;
	struct dpll_device *dpll;
	struct nlattr **attrs;
	struct sk_buff *msg;
	int dpll_id;
	int dpll_source_id;
	int dpll_source_type;
	int dpll_output_id;
	int dpll_output_type;
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
	struct nlattr *src_attr;
	int i, ret = 0, type;

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
		nla_nest_end(msg, src_attr);
	}

	return ret;
}

static int __dpll_cmd_dump_outputs(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	struct nlattr *out_attr;
	int i, ret = 0, type;

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
		nla_nest_end(msg, out_attr);
	}

	return ret;
}

static int __dpll_cmd_dump_status(struct dpll_device *dpll,
					   struct sk_buff *msg)
{
	int ret;

	if (dpll->ops->get_status) {
		ret = dpll->ops->get_status(dpll);
		if (nla_put_u32(msg, DPLLA_STATUS, ret))
			return -EMSGSIZE;
	}

	if (dpll->ops->get_temp) {
		ret = dpll->ops->get_temp(dpll);
		if (nla_put_u32(msg, DPLLA_TEMP, ret))
			return -EMSGSIZE;
	}

	if (dpll->ops->get_lock_status) {
		ret = dpll->ops->get_lock_status(dpll);
		if (nla_put_u32(msg, DPLLA_LOCK_STATUS, ret))
			return -EMSGSIZE;
	}

	return 0;
}

static int dpll_device_dump_one(struct dpll_device *dpll, struct sk_buff *msg, int flags)
{
	struct nlattr *hdr;
	int ret;

	hdr = nla_nest_start(msg, DPLLA_DEVICE);
	if (!hdr)
		return -EMSGSIZE;

	mutex_lock(&dpll->lock);
	ret = __dpll_cmd_device_dump_one(dpll, msg);
	if (ret)
		goto out_cancel_nest;

	if (flags & DPLL_FLAG_SOURCES && dpll->ops->get_source_type) {
		ret = __dpll_cmd_dump_sources(dpll, msg);
		if (ret)
			goto out_cancel_nest;
	}

	if (flags & DPLL_FLAG_OUTPUTS && dpll->ops->get_output_type) {
		ret = __dpll_cmd_dump_outputs(dpll, msg);
		if (ret)
			goto out_cancel_nest;
	}

	if (flags & DPLL_FLAG_STATUS) {
		ret = __dpll_cmd_dump_status(dpll, msg);
		if (ret)
			goto out_cancel_nest;
	}

	mutex_unlock(&dpll->lock);
	nla_nest_end(msg, hdr);

	return 0;

out_cancel_nest:
	mutex_unlock(&dpll->lock);
	nla_nest_cancel(msg, hdr);

	return ret;
}

static int dpll_genl_cmd_set_source(struct param *p)
{
	const struct nlattr **attrs = p->attrs;
	struct dpll_device *dpll = p->dpll;
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

	return ret;
}

static int dpll_genl_cmd_set_output(struct param *p)
{
	const struct nlattr **attrs = p->attrs;
	struct dpll_device *dpll = p->dpll;
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

	return ret;
}

static int dpll_device_loop_cb(struct dpll_device *dpll, void *data)
{
	struct dpll_dump_ctx *ctx;
	struct param *p = (struct param *)data;

	ctx = dpll_dump_context(p->cb);

	ctx->pos_idx = dpll->id;

	return dpll_device_dump_one(dpll, p->msg, ctx->flags);
}

static int dpll_cmd_device_dump(struct param *p)
{
	struct dpll_dump_ctx *ctx = dpll_dump_context(p->cb);

	return for_each_dpll_device(ctx->pos_idx, dpll_device_loop_cb, p);
}

static int dpll_genl_cmd_device_get_id(struct param *p)
{
	struct dpll_device *dpll = p->dpll;
	int flags = 0;

	if (p->attrs[DPLLA_FLAGS])
		flags = nla_get_u32(p->attrs[DPLLA_FLAGS]);

	return dpll_device_dump_one(dpll, p->msg, flags);
}

static const cb_t cmd_doit_cb[] = {
	[DPLL_CMD_DEVICE_GET]		= dpll_genl_cmd_device_get_id,
	[DPLL_CMD_SET_SOURCE_TYPE]	= dpll_genl_cmd_set_source,
	[DPLL_CMD_SET_OUTPUT_TYPE]	= dpll_genl_cmd_set_output,
};

static const cb_t cmd_dump_cb[] = {
	[DPLL_CMD_DEVICE_GET]		= dpll_cmd_device_dump,
};

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

static int dpll_genl_cmd_dumpit(struct sk_buff *skb,
				   struct netlink_callback *cb)
{
	struct param p = { .cb = cb, .msg = skb };
	const struct genl_dumpit_info *info = genl_dumpit_info(cb);
	int cmd = info->op.cmd;
	int ret;
	void *hdr;

	hdr = genlmsg_put(skb, 0, 0, &dpll_gnl_family, 0, cmd);
	if (!hdr)
		return -EMSGSIZE;

	ret = cmd_dump_cb[cmd](&p);
	if (ret)
		goto out_cancel_msg;

	genlmsg_end(skb, hdr);

	return 0;

out_cancel_msg:
	genlmsg_cancel(skb, hdr);

	return ret;
}

static int dpll_genl_cmd_doit(struct sk_buff *skb,
				 struct genl_info *info)
{
	struct param p = { .attrs = info->attrs, .dpll = info->user_ptr[0] };
	int cmd = info->genlhdr->cmd;
	struct sk_buff *msg;
	void *hdr;
	int ret;

	msg = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!msg)
		return -ENOMEM;
	p.msg = msg;

	hdr = genlmsg_put_reply(msg, info, &dpll_gnl_family, 0, cmd);
	if (!hdr) {
		ret = -EMSGSIZE;
		goto out_free_msg;
	}

	ret = cmd_doit_cb[cmd](&p);
	if (ret)
		goto out_cancel_msg;

	genlmsg_end(msg, hdr);

	return genlmsg_reply(msg, info);

out_cancel_msg:
	genlmsg_cancel(msg, hdr);
out_free_msg:
	nlmsg_free(msg);

	return ret;
}

static int dpll_pre_doit(const struct genl_split_ops *ops, struct sk_buff *skb,
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
		.start	= dpll_genl_cmd_start,
		.dumpit	= dpll_genl_cmd_dumpit,
		.doit	= dpll_genl_cmd_doit,
		.policy	= dpll_genl_get_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_get_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_SOURCE_TYPE,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_doit,
		.policy	= dpll_genl_set_source_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_source_policy) - 1,
	},
	{
		.cmd	= DPLL_CMD_SET_OUTPUT_TYPE,
		.flags	= GENL_UNS_ADMIN_PERM,
		.doit	= dpll_genl_cmd_doit,
		.policy	= dpll_genl_set_output_policy,
		.maxattr = ARRAY_SIZE(dpll_genl_set_output_policy) - 1,
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
