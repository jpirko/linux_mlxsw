// SPDX-License-Identifier: GPL-2.0
/*
 * include/net/ethlink.h - Ethernet netdevice Netlink interface
 *
 * Copyright (c) 2018 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2018 Jiri Pirko <jiri@mellanox.com>
 */
#ifndef _NET_ETHLINK_H_
#define _NET_ETHLINK_H_

#include <uapi/linux/ethlink.h>

struct ethlink_ops {
};

void ethlink_notify_new(struct net_device *dev);
void ethlink_notify_del(struct net_device *dev);

#endif /* _NET_ETHLINK_H_ */
