/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_LINUX_DPLL_H
#define _UAPI_LINUX_DPLL_H

#define DPLL_NAME_LENGTH	20

/* Adding event notification support elements */
#define DPLL_FAMILY_NAME		"dpll"
#define DPLL_VERSION			0x01
#define DPLL_CONFIG_DEVICE_GROUP_NAME  "config"
#define DPLL_CONFIG_SOURCE_GROUP_NAME  "source"
#define DPLL_CONFIG_OUTPUT_GROUP_NAME  "output"
#define DPLL_MONITOR_GROUP_NAME        "monitor"

#define DPLL_FLAG_SOURCES	1
#define DPLL_FLAG_OUTPUTS	2
#define DPLL_FLAG_STATUS	4

/* Attributes of dpll_genl_family */
enum dpll_genl_attr {
	DPLLA_UNSPEC,
	DPLLA_DEVICE_ID,
	DPLLA_DEVICE_NAME,
	DPLLA_SOURCE,
	DPLLA_SOURCE_ID,
	DPLLA_SOURCE_TYPE,
	DPLLA_SOURCE_SUPPORTED,
	DPLLA_OUTPUT,
	DPLLA_OUTPUT_ID,
	DPLLA_OUTPUT_TYPE,
	DPLLA_OUTPUT_SUPPORTED,
	DPLLA_STATUS,
	DPLLA_TEMP,
	DPLLA_LOCK_STATUS,
	DPLLA_FLAGS,

	__DPLLA_MAX,
};
#define DPLLA_MAX (__DPLLA_MAX - 1)

/* DPLL status provides information of device status */
enum dpll_genl_status {
	DPLL_STATUS_NONE,
	DPLL_STATUS_CALIBRATING,
	DPLL_STATUS_LOCKED,

	__DPLL_STATUS_MAX,
};
#define DPLL_STATUS_MAX (__DPLL_STATUS_MAX - 1)

/* DPLL signal types used as source or as output */
enum dpll_genl_signal_type {
	DPLL_TYPE_EXT_1PPS,
	DPLL_TYPE_EXT_10MHZ,
	DPLL_TYPE_SYNCE_ETH_PORT,
	DPLL_TYPE_INT_OSCILLATOR,
	DPLL_TYPE_GNSS,

	__DPLL_TYPE_MAX,
};
#define DPLL_TYPE_MAX (__DPLL_TYPE_MAX - 1)

/* DPLL lock status provides information of source used to lock the device */
enum dpll_genl_lock_status {
	DPLL_LOCK_STATUS_UNLOCKED,
	DPLL_LOCK_STATUS_EXT_1PPS,
	DPLL_LOCK_STATUS_EXT_10MHZ,
	DPLL_LOCK_STATUS_SYNCE,
	DPLL_LOCK_STATUS_INT_OSCILLATOR,
	DPLL_LOCK_STATUS_GNSS,

	__DPLL_LOCK_STATUS_MAX,
};
#define DPLL_LOCK_STATUS_MAX (__DPLL_LOCK_STATUS_MAX - 1)

/* Events of dpll_genl_family */
enum dpll_genl_event {
	DPLL_EVENT_UNSPEC,
	DPLL_EVENT_DEVICE_CREATE,		/* DPLL device creation */
	DPLL_EVENT_DEVICE_DELETE,		/* DPLL device deletion */
	DPLL_EVENT_STATUS_LOCKED,		/* DPLL device locked to source */
	DPLL_EVENT_STATUS_UNLOCKED,	/* DPLL device freerun */
	DPLL_EVENT_SOURCE_CHANGE,		/* DPLL device source changed */
	DPLL_EVENT_OUTPUT_CHANGE,		/* DPLL device output changed */

	__DPLL_EVENT_MAX,
};
#define DPLL_EVENT_MAX (__DPLL_EVENT_MAX - 1)

/* Commands supported by the dpll_genl_family */
enum dpll_genl_cmd {
	DPLL_CMD_UNSPEC,
	DPLL_CMD_DEVICE_GET,	/* List of DPLL devices id */
	DPLL_CMD_SET_SOURCE_TYPE,	/* Set the DPLL device source type */
	DPLL_CMD_SET_OUTPUT_TYPE,	/* Set the DPLL device output type */

	__DPLL_CMD_MAX,
};
#define DPLL_CMD_MAX (__DPLL_CMD_MAX - 1)

#endif /* _UAPI_LINUX_DPLL_H */
