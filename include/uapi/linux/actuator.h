/*
 * include/uapi/linux/actuator.h - Generic actuator support
 * Copyright (c) 2017 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _UAPI_LINUX_ACTUATOR_H_
#define _UAPI_LINUX_ACTUATOR_H_

#define ACTUATOR_GENL_NAME "actuator"
#define ACTUATOR_GENL_VERSION 0x1
#define ACTUATOR_GENL_MCGRP_CONFIG_NAME "config"

enum actuator_command {
	/* don't change the order or add anything between, this is ABI! */
	ACTUATOR_CMD_UNSPEC,

	ACTUATOR_CMD_GET,		/* can dump */
	ACTUATOR_CMD_NEW,
	ACTUATOR_CMD_DEL,

	ACTUATOR_CMD_MOVE,
	ACTUATOR_CMD_STOP,

	/* add new commands above here */
	__ACTUATOR_CMD_MAX,
	ACTUATOR_CMD_MAX = __ACTUATOR_CMD_MAX - 1
};

enum actuator_type {
	ACTUATOR_TYPE_UNSPEC,
	ACTUATOR_TYPE_LINEAR,
};

enum actuator_units {
	ACTUATOR_UNITS_UNSPEC,
	ACTUATOR_UNITS_UM, /* micrometres */
};

enum actuator_move_state {
	ACTUATOR_MOVE_STATE_UNSPEC,
	ACTUATOR_MOVE_STATE_STOPPED,
	ACTUATOR_MOVE_STATE_POSITIVE,
	ACTUATOR_MOVE_STATE_NEGATIVE,
};

enum actuator_attr {
	/* don't change the order or add anything between, this is ABI! */
	ACTUATOR_ATTR_UNSPEC,

	/* bus name + dev name + index together are a handle
	 * for actuator entity
	 */
	ACTUATOR_ATTR_BUS_NAME,		/* string */
	ACTUATOR_ATTR_DEV_NAME,		/* string */
	ACTUATOR_ATTR_INDEX,		/* u32 */

	ACTUATOR_ATTR_DRIVER_NAME,	/* string */
	ACTUATOR_ATTR_TYPE,		/* u8, enum actuator_type */
	ACTUATOR_ATTR_UNITS,		/* u8, enum actuator_units */
	ACTUATOR_ATTR_VALUE,		/* u32, in units */
	ACTUATOR_ATTR_MOVE_STATE,	/* u8, enum actuator_move_state */

	/* add new attributes above here, update the policy in actuator.c */

	__ACTUATOR_ATTR_MAX,
	ACTUATOR_ATTR_MAX = __ACTUATOR_ATTR_MAX - 1
};

#endif /* _UAPI_LINUX_ACTUATOR_H_ */
