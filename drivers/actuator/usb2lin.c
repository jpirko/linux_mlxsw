/*
 * drivers/actuator/usb2lin.c - Linak USB2LIN cable support
 * Copyright (c) 2016-2017 Jiri Pirko <jiri@resnulli.us>
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

static const char u2l_driver_name[] = "usb2lin";

static const struct usb_device_id u2l_id_table[] = {
	{ USB_DEVICE(USB_VENDOR_ID_LINAK, USB_DEVICE_ID_LINAK_USB2LIN) },
	{ },
};

struct u2l_status {
	__le16 header;
	__le16 move_status;
#define U2L_HEIGHT_MULTIPLIER 100
	__le16 height;
#define U2L_STATUS_MOVE_FLAGS_BLOCKED BIT(1)
#define U2L_STATUS_MOVE_FLAGS_DIR BIT(7)
	u8 move_flags;
	u8 move_magic; /* not 0 when moving :O */
	u8 unknown1[12];
#define U2L_STATUS_TARGET_HEIGHT_MAGIC_STOPPED 0x8001
	__le16 target_height;
	u8 unknown2[42];
} __packed;

struct u2l_move {
	u8 header;
#define U2L_MOVE_TARGET_HEIGHT_MAGIC_UP 0x8000
#define U2L_MOVE_TARGET_HEIGHT_MAGIC_DOWN 0x7fff
	__le16 target_height[4]; /* unaligned access */
	u8 pad[59]; /* zero padding */
} __packed;

struct u2l_init {
	__le32 header;
	u8 pad[60]; /* zero padding */
} __packed;

struct u2l {
	struct mutex lock;
	struct actuator *act;
	struct usb_device *udev;
	struct u2l_status last_status;
	struct u2l_init init;
	struct delayed_work dw;
	struct {
		bool in_progress;
		bool started;
		unsigned long start_jiffies;
		u16 target_height;
	} move;
	union {
		struct u2l_move status;
		struct u2l_move move;
	} msg;
};

struct u2l_usbmsg_info {
	u8 request;
	u8 requesttype;
	u16 value;
};

enum u2l_usbmsg_type {
	U2L_USBMSG_HELLO,
	U2L_USBMSG_INIT,
	U2L_USBMSG_STATUS,
	U2L_USBMSG_MOVE,
};

#define U2L_USBMSG_REQUEST_TYPE_IN 0xa1
#define U2L_USBMSG_REQUEST_TYPE_OUT 0x21

static const struct u2l_usbmsg_info u2l_ubsmsg_infos[] = {
	[U2L_USBMSG_HELLO] = {
		.request = 0x0a,
		.requesttype = U2L_USBMSG_REQUEST_TYPE_OUT,
	},
	[U2L_USBMSG_INIT] = {
		.request = 0x09,
		.requesttype = U2L_USBMSG_REQUEST_TYPE_OUT,
		.value = 0x0303,
	},
	[U2L_USBMSG_STATUS] = {
		.request = 0x01,
		.requesttype = U2L_USBMSG_REQUEST_TYPE_IN,
		.value = 0x0304,
	},
	[U2L_USBMSG_MOVE] = {
		.request = 0x09,
		.requesttype = U2L_USBMSG_REQUEST_TYPE_OUT,
		.value = 0x0305,
	},
};

#define U2L_USB_CONTROL_MSG_TIMEOUT 300

static int u2l_usb_control_msg(struct u2l *u2l, enum u2l_usbmsg_type type,
			       void *data, u16 size)
{
	const struct u2l_usbmsg_info *info = &u2l_ubsmsg_infos[type];
	unsigned int pipe;
	int err;

	pipe = info->requesttype == U2L_USBMSG_REQUEST_TYPE_IN ?
	       usb_rcvctrlpipe(u2l->udev, 0) :
	       usb_sndctrlpipe(u2l->udev, 0);

	err = usb_control_msg(u2l->udev, pipe, info->request, info->requesttype,
			      info->value, 0, data, size,
			      U2L_USB_CONTROL_MSG_TIMEOUT);
	return err < 0 ? err : 0;
}

static int u2l_cmd_hello(struct u2l *u2l)
{
	return u2l_usb_control_msg(u2l, U2L_USBMSG_HELLO, NULL, 0);
}

#define U2L_USBMSG_INIT_HEADER_MAGIC 0xfb000403

static int u2l_cmd_init(struct u2l *u2l)
{
	u2l->init.header = cpu_to_le32(U2L_USBMSG_INIT_HEADER_MAGIC);
	return u2l_usb_control_msg(u2l, U2L_USBMSG_INIT,
				   &u2l->init, sizeof(u2l->init));
}

static int u2l_cmd_status(struct u2l *u2l, struct u2l_status *status)
{
	int err;

	err = u2l_usb_control_msg(u2l, U2L_USBMSG_STATUS,
				  &u2l->msg.status, sizeof(*status));
	if (err)
		return err;
	memcpy(status, &u2l->msg.status, sizeof(*status));
	dev_dbg(&u2l->udev->dev, "move_status %04x, height %04x (%u), move_flags %02x (%s%s), move_magic %02x, target_height %04x (%u)\n",
		status->move_status, status->height, status->height,
		status->move_flags,
		status->move_flags & U2L_STATUS_MOVE_FLAGS_BLOCKED ?
				     "blocked " : "",
		status->move_flags & U2L_STATUS_MOVE_FLAGS_DIR ?
				     "down" : "up",
		status->move_magic,
		status->target_height, status->target_height);
	return 0;
}

static void u2l_cmd_last_status_save(struct u2l *u2l, struct u2l_status *status)
{
	memcpy(&u2l->last_status, status, sizeof(u2l->last_status));
}

#define U2L_USBMSG_MOVE_HEADER_MAGIC 0x05

static int u2l_cmd_move(struct u2l *u2l, u16 target_height)
{
	__le16 tmp;
	int i;

	u2l->msg.move.header = U2L_USBMSG_MOVE_HEADER_MAGIC;
	tmp = cpu_to_le16(target_height);
	for (i = 0; i < 4; i++)
		memcpy(&u2l->msg.move.target_height[i], &tmp, sizeof(tmp));

	return u2l_usb_control_msg(u2l, U2L_USBMSG_MOVE,
				   &u2l->msg.move, sizeof(u2l->move));
}

static void u2l_actuator_status_fill(struct u2l *u2l,
				     struct actuator_status *act_status,
				     struct u2l_status *status)
{
	act_status->value = le16_to_cpu(status->height) * U2L_HEIGHT_MULTIPLIER;
	if (!status->move_magic) {
		act_status->move_state = ACTUATOR_MOVE_STATE_STOPPED;
	} else {
		act_status->move_state =
			status->move_flags & U2L_STATUS_MOVE_FLAGS_DIR ?
			ACTUATOR_MOVE_STATE_NEGATIVE :
			ACTUATOR_MOVE_STATE_POSITIVE;
	}
}

static int u2l_actuator_status(struct actuator *act,
			       struct actuator_status *act_status)
{
	struct u2l *u2l = actuator_priv(act);
	struct u2l_status status;
	int err;

	mutex_lock(&u2l->lock);
	err = u2l_cmd_status(u2l, &status);
	mutex_unlock(&u2l->lock);
	if (err)
		return err;
	u2l_actuator_status_fill(u2l, act_status, &status);
	return 0;
}

#define U2L_MOVE_START_DELAY 500 /* ms */

static void u2l_check_move(struct u2l *u2l, struct u2l_status *status)
{
	if (!u2l->move.in_progress)
		return;

	if (!u2l->move.started) {
		if (status->move_magic)
			u2l->move.started = true;
		else if (jiffies_to_msecs(jiffies - u2l->move.start_jiffies) >
			 U2L_MOVE_START_DELAY) {
			goto finish; /* Move start timed-out */
		}
	} else if (!status->move_magic) {
		goto finish; /* We are at the destination or limit */
	}

	u2l_cmd_move(u2l, u2l->move.target_height);
	return;
finish:
	u2l->move.in_progress = false;
}

static void u2l_check_notify(struct u2l *u2l, struct u2l_status *status)
{
	struct actuator_status act_status;

	if (!!status->move_magic != !!u2l->last_status.move_magic) {
		u2l_actuator_status_fill(u2l, &act_status, status);
		actuator_notify_status(u2l->act, &act_status);
	}
}

#define U2L_WORK_DELAY 100 /* ms */

static void u2l_schedule_work(struct u2l *u2l)
{
	schedule_delayed_work(&u2l->dw, U2L_WORK_DELAY);
}

static void u2l_work(struct work_struct *work)
{
	struct u2l_status status;
	struct u2l *u2l;
	int err;

	u2l = container_of(work, struct u2l, dw.work);

	mutex_lock(&u2l->lock);
	err = u2l_cmd_status(u2l, &status);
	if (err)
		goto unlock;
	u2l_check_move(u2l, &status);
	u2l_check_notify(u2l, &status);
	u2l_cmd_last_status_save(u2l, &status);
	u2l_schedule_work(u2l);
unlock:
	mutex_unlock(&u2l->lock);
}

static int u2l_actuator_move(struct actuator *act, u32 value)
{
	struct u2l *u2l = actuator_priv(act);
	int err;

	value /= U2L_HEIGHT_MULTIPLIER;
	if (value < 0 || value > 0xFFFF)
		return -EINVAL;

	mutex_lock(&u2l->lock);

	u2l->move.in_progress = true;
	u2l->move.started = false;
	u2l->move.start_jiffies = jiffies;
	u2l->move.target_height = value;

	/* Do move cmd twice, first one would wake up the control in case
	 * it sleeps. Unfortunately there is no known way to find out.
	 */
	err = u2l_cmd_move(u2l, u2l->move.target_height);
	if (err)
		goto unlock;
	err = u2l_cmd_move(u2l, u2l->move.target_height);
	if (err)
		goto unlock;

unlock:
	mutex_unlock(&u2l->lock);
	return err;
}

static int u2l_actuator_stop(struct actuator *act)
{
	struct u2l *u2l = actuator_priv(act);
	struct u2l_status status;
	u16 wakeup_target_height;
	int err;

	mutex_lock(&u2l->lock);

	err = u2l_cmd_status(u2l, &status);
	if (err)
		goto unlock;
	if (le16_to_cpu(status.target_height) ==
	    U2L_STATUS_TARGET_HEIGHT_MAGIC_STOPPED)
		goto unlock;

	wakeup_target_height =
		status.move_flags & U2L_STATUS_MOVE_FLAGS_DIR ?
		U2L_MOVE_TARGET_HEIGHT_MAGIC_UP :
		U2L_MOVE_TARGET_HEIGHT_MAGIC_DOWN;

	err = u2l_cmd_move(u2l, wakeup_target_height);

	u2l->move.in_progress = false;

unlock:
	mutex_unlock(&u2l->lock);
	return err;
}

static int u2l_init(struct u2l *u2l)
{
	struct u2l_status status;
	u16 current_height;
	int err;

	err = u2l_cmd_hello(u2l);
	if (err)
		return err;

	err = u2l_cmd_status(u2l, &status);
	if (err)
		return err;
	current_height = le16_to_cpu(status.height);
	u2l_cmd_last_status_save(u2l, &status);

	err = u2l_cmd_init(u2l);
	if (err)
		return err;

	return u2l_cmd_move(u2l, current_height);
}

static const struct actuator_ops u2l_actuator_ops = {
	.priv_size = sizeof(struct u2l),
	.driver_name = u2l_driver_name,
	.type = ACTUATOR_TYPE_LINEAR,
	.units = ACTUATOR_UNITS_UM,
	.status = u2l_actuator_status,
	.move = u2l_actuator_move,
	.stop = u2l_actuator_stop,
};

static int u2l_probe(struct usb_interface *interface,
		     const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct actuator *act;
	struct u2l *u2l;
	int err;

	act = actuator_alloc(&u2l_actuator_ops);
	if (!act)
		return -ENOMEM;
	u2l = actuator_priv(act);
	u2l->act = act;
	u2l->udev = udev;
	mutex_init(&u2l->lock);
	INIT_DELAYED_WORK(&u2l->dw, u2l_work);

	err = u2l_init(u2l);
	if (err) {
		dev_err(&u2l->udev->dev, "Initialization failed\n");
		goto err_init;
	}

	err = actuator_register(act, &udev->dev, 0);
	if (err)
		goto err_actuator_register;

	usb_set_intfdata(interface, act);
	u2l_schedule_work(u2l);
	return 0;

err_actuator_register:
err_init:
	actuator_free(act);
	return err;
}

static void u2l_disconnect(struct usb_interface *interface)
{
	struct actuator *act = usb_get_intfdata(interface);
	struct u2l *u2l = actuator_priv(act);

	cancel_delayed_work_sync(&u2l->dw);
	actuator_unregister(act);
	actuator_free(act);
}

static struct usb_driver u2l_driver = {
	.name = u2l_driver_name,
	.probe = u2l_probe,
	.disconnect = u2l_disconnect,
	.id_table = u2l_id_table,
};

module_usb_driver(u2l_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiri Pirko <jiri@resnulli.us>");
MODULE_DESCRIPTION("Linak USB2LIN cable support");
MODULE_DEVICE_TABLE(usb, u2l_id_table);
