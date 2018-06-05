// SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note
/*
 * include/uapi/linux/ethlink.h - Ethernet netdevice Netlink interface
 *
 * Copyright (c) 2018 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2018 Jiri Pirko <jiri@mellanox.com>
 */

#ifndef _UAPI_LINUX_ETHLINK_H_
#define _UAPI_LINUX_ETHLINK_H_

#define ETHLINK_GENL_NAME "ethlink"
#define ETHLINK_GENL_VERSION 0x1
#define ETHLINK_GENL_MCGRP_CONFIG_NAME "config"

enum ethlink_command {
	/* don't change the order or add anything between, this is ABI! */
	ETHLINK_CMD_UNSPEC,

	ETHLINK_CMD_GET,		/* can dump */
	ETHLINK_CMD_NEW,
	ETHLINK_CMD_DEL,


	/* add new commands above here */
	__ETHLINK_CMD_MAX,
	ETHLINK_CMD_MAX = __ETHLINK_CMD_MAX - 1
};

enum ethlink_attr {
	/* don't change the order or add anything between, this is ABI! */
	ETHLINK_ATTR_UNSPEC,

	/* Either ifindex or ifname are used as a handle. If both attributes
	 * exist, ifname is ignored.
	 */
	ETHLINK_ATTR_IFINDEX,			/* u32 */
	ETHLINK_ATTR_IFNAME,			/* string */

	/* add new attributes above here, update the policy in ethlink.c */

	__ETHLINK_ATTR_MAX,
	ETHLINK_ATTR_MAX = __ETHLINK_ATTR_MAX - 1
};

#endif /* _UAPI_LINUX_ETHLINK_H_ */
