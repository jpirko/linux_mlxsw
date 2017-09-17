/*
 * drivers/actuator/lac.c - Actuonix Linear Actuator Control Board support
 * Copyright (c) 2017 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/usb.h>
#include <linux/actuator.h>
#include "../../drivers/hid/hid-ids.h"

static const char lac_driver_name[] = "lac";

static const struct usb_device_id lac_id_table[] = {
	{ USB_DEVICE(USB_VENDOR_ID_MICROCHIP, 0xfc5f) },
	{ },
};

struct lac_msg {
	u8 cmd;
	__le16 value;
} __packed;

struct lac {
	struct mutex lock;
	struct actuator *act;
	struct usb_device *udev;
	struct lac_msg msg;
};

enum lac_msg_cmd {
	LAC_MSG_CMD_VALUE = 0x20,
	LAC_MSG_CMD_SPEED = 0x21,
};

#define LAC_USB_MSG_TIMEOUT 300

static int lac_usb_msg(struct lac *lac, void *data, int len, unsigned int pipe)
{
	int actual_length;
	int err;

	err = usb_bulk_msg(lac->udev, pipe, data, len,
			   &actual_length, LAC_USB_MSG_TIMEOUT);
	if (err)
		return err;
	if (actual_length != len)
		return -EINVAL;
	return 0;
}

static int lac_usb_msg_in(struct lac *lac, void *data, int len)
{
	return lac_usb_msg(lac, data, len, usb_rcvbulkpipe(lac->udev, 1));
}

static int lac_usb_msg_out(struct lac *lac, void *data, int len)
{
	return lac_usb_msg(lac, data, len, usb_sndbulkpipe(lac->udev, 1));
}

static int lac_actuator_status(struct actuator *act,
			       struct actuator_status *act_status)
{
	struct lac *lac = actuator_priv(act);

	return 0;
}

static int lac_actuator_move(struct actuator *act, u32 value)
{
	struct lac *lac = actuator_priv(act);
	int err;

	lac->msg.cmd = LAC_MSG_CMD_VALUE;
	lac->msg.value = value;
	mutex_lock(&lac->lock);
	err = lac_usb_msg_out(lac, &lac->msg, sizeof(lac->msg));
	mutex_unlock(&lac->lock);
	return err;
}

static int lac_actuator_stop(struct actuator *act)
{
	return 0;
}

static int lac_init(struct lac *lac)
{
	return 0;
}

static const struct actuator_ops lac_actuator_ops = {
	.priv_size = sizeof(struct lac),
	.driver_name = lac_driver_name,
	.type = ACTUATOR_TYPE_LINEAR,
	.units = ACTUATOR_UNITS_UM,
	.status = lac_actuator_status,
	.move = lac_actuator_move,
	.stop = lac_actuator_stop,
};

static int lac_probe(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct actuator *act;
	struct lac *lac;
	int err;

	act = actuator_alloc(&lac_actuator_ops);
	if (!act)
		return -ENOMEM;
	lac = actuator_priv(act);
	lac->act = act;
	lac->udev = udev;
	mutex_init(&lac->lock);

	err = lac_init(lac);
	if (err) {
		dev_err(&lac->udev->dev, "Initialization failed\n");
		goto err_init;
	}

	err = actuator_register(act, &udev->dev, 0);
	if (err)
		goto err_actuator_register;

	usb_set_intfdata(interface, act);
	return 0;

err_actuator_register:
err_init:
	actuator_free(act);
	return err;
}

static void lac_disconnect(struct usb_interface *interface)
{
	struct actuator *act = usb_get_intfdata(interface);
	struct lac *lac = actuator_priv(act);

	actuator_unregister(act);
	actuator_free(act);
}

static struct usb_driver lac_driver = {
	.name = lac_driver_name,
	.probe = lac_probe,
	.disconnect = lac_disconnect,
	.id_table = lac_id_table,
};

module_usb_driver(lac_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiri Pirko <jiri@resnulli.us>");
MODULE_DESCRIPTION("Actuonix Linear Actuator Control Board support");
MODULE_DEVICE_TABLE(usb, lac_id_table);
