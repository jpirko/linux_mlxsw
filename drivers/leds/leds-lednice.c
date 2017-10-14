/*
 * drivers/leds/leds-lednice.c - Lednice generic USB led device support
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
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/usb.h>
#include <linux/leds.h>
#include "../../drivers/hid/hid-ids.h"

static const char lednice_driver_name[] = "lednice";

static const struct usb_device_id lednice_id_table[] = {
	{ USB_DEVICE(0xDEAD, 0xBEEF) },
	{ },
};

enum lednice_usb_cmd {
	LEDNICE_USB_CMD_GET_INFO,
	LEDNICE_USB_CMD_GET_LED_INFO,
	LEDNICE_USB_CMD_SET_LED_BRIGHTNESS,
	LEDNICE_USB_CMD_GET_LED_BRIGHTNESS,
};

#define LEDNICE_USB_MSG_NAME_SIZE 16

struct lednice_info_msg {
	__le16 led_count;
	u8 dev_name[LEDNICE_USB_MSG_NAME_SIZE];
} __packed;

struct lednice_led_info_msg {
	u8 led_name[LEDNICE_USB_MSG_NAME_SIZE];
	u8 led_subname[LEDNICE_USB_MSG_NAME_SIZE];
	u8 max_brightness;
} __packed;

struct lednice_led_brightness_msg {
	u8 brightness;
} __packed;

/* USB communication with the device is done only by sending and receiving
 * control urb messages.
 *
 * LEDNICE_USB_CMD_GET_INFO - get information about led device
 *   IN:
 *     value 0
 *     index 0
 *   OUT:
 *     struct lednice_info_msg
 *
 * LEDNICE_USB_CMD_GET_LED_INFO - get information about specific led
 *   IN:
 *     value 0
 *     index LED_ID (0 .. lednice_info_msg.led_count - 1)
 *   OUT:
 *     struct lednice_led_info_msg
 *
 * LEDNICE_USB_CMD_SET_LED_BRIGHTNESS - set specific led brightness
 *   IN:
 *     value BRIGHTNESS (0 .. 255)
 *     index LED_ID (0 .. lednice_info_msg.led_count - 1)
 *   OUT:
 *     NULL
 *
 * LEDNICE_USB_CMD_GET_LED_BRIGHTNESS - get specific led brightness
 *   IN:
 *     value 0
 *     index LED_ID (0 .. lednice_info_msg.led_count - 1)
 *   OUT: struct lednice_led_info_msg
 *     struct lednice_led_brightness_msg
 */

struct lednice_led;

struct lednice {
	struct mutex lock;
	struct usb_device *udev;
	struct lednice_info_msg info;
	struct lednice_led *leds;
};

struct lednice_led {
	struct lednice *lednice;
	u16 id;
	struct led_classdev led_cdev;
	struct lednice_led_info_msg info;
};

static void lednice_fix_name(u8 *name)
{
	int i;

	name[LEDNICE_USB_MSG_NAME_SIZE - 1] = 0;
	for (i = 0; i < strlen(name); i++) {
		if (!isalnum(name[i]))
			name[i] = 0;
	}
}

#define LEDNICE_USB_CONTROL_MSG_TIMEOUT 300

static int lednice_usb_read(struct lednice *lednice, enum lednice_usb_cmd cmd,
			     __u16 value, __u16 index, void *data, __u16 size)
{
	void *dmadata = kmalloc(size, GFP_KERNEL);
	int ret;

	if (!dmadata)
		return -ENOMEM;
	mutex_lock(&lednice->lock);
	ret = usb_control_msg(lednice->udev, usb_rcvctrlpipe(lednice->udev, 0),
			      cmd, USB_TYPE_VENDOR | USB_RECIP_INTERFACE |
			      USB_DIR_IN, value, index, dmadata, size,
			      LEDNICE_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&lednice->lock);
	memcpy(data, dmadata, size);
	kfree(dmadata);
	return ret < 0 ? ret : 0;
}

static int lednice_usb_write(struct lednice *lednice, enum lednice_usb_cmd cmd,
			     __u16 value, __u16 index, void *data, __u16 size)
{
	void *dmadata = kmemdup(data, size, GFP_KERNEL);
	int ret;

	if (!dmadata)
		return -ENOMEM;
	mutex_lock(&lednice->lock);
	ret = usb_control_msg(lednice->udev, usb_sndctrlpipe(lednice->udev, 0),
			      cmd, USB_TYPE_VENDOR | USB_RECIP_INTERFACE,
			      value, index, dmadata, size,
			      LEDNICE_USB_CONTROL_MSG_TIMEOUT);
	mutex_unlock(&lednice->lock);
	kfree(dmadata);
	return ret < 0 ? ret : 0;
}

static int lednice_led_brightness_set(struct led_classdev *led_cdev,
				      enum led_brightness brightness)
{
	struct lednice_led *lednice_led =
			container_of(led_cdev, struct lednice_led, led_cdev);

	return lednice_usb_write(lednice_led->lednice,
				 LEDNICE_USB_CMD_SET_LED_BRIGHTNESS,
				 brightness, lednice_led->id, NULL, 0);
}

static enum led_brightness
lednice_led_brightness_get(struct led_classdev *led_cdev)
{
	struct lednice_led_brightness_msg brightness_msg;
	struct lednice_led *lednice_led =
			container_of(led_cdev, struct lednice_led, led_cdev);
	int err;

	err = lednice_usb_read(lednice_led->lednice,
			       LEDNICE_USB_CMD_GET_LED_BRIGHTNESS,
			       0, lednice_led->id,
			       &brightness_msg, sizeof(brightness_msg));
	if (err)
		return err;
	return brightness_msg.brightness;
}

static char *lednice_led_name_generate(struct lednice_led *lednice_led)
{
	char *names[] = {
		lednice_led->lednice->info.dev_name,
		lednice_led->info.led_name,
		lednice_led->info.led_subname,
		NULL,
	};
	char *genname;
	int items = 0;
	int len = 0;
	char *name;
	char *pos;
	int i;

	i = 0;
	while ((name = names[i++])) {
		if (strlen(name)) {
			len += strlen(name);
			items++;
		}
	}
	if (!items)
		return kstrdup(lednice_driver_name, GFP_KERNEL);
	genname = kmalloc(len + items, GFP_KERNEL);
	if (!genname)
		return NULL;

	pos = genname;
	i = 0;
	while ((name = names[i++])) {
		if (strlen(name))
			pos += sprintf(pos, "%s%s",
				       pos != genname ? ":" : "", name);
	}

	return genname;
}

static int lednice_led_init(struct lednice *lednice, u16 led_id)
{
	struct lednice_led *lednice_led = &lednice->leds[led_id];
	int err;

	lednice_led->lednice = lednice;
	lednice_led->id = led_id;

	err = lednice_usb_read(lednice, LEDNICE_USB_CMD_GET_LED_INFO, 0, led_id,
			       &lednice_led->info, sizeof(lednice_led->info));
	if (err)
		return err;
	lednice_fix_name(lednice_led->info.led_name);
	lednice_fix_name(lednice_led->info.led_subname);

	lednice_led->led_cdev.name = lednice_led_name_generate(lednice_led);
	if (!lednice_led->led_cdev.name)
		return -ENOMEM;
	lednice_led->led_cdev.max_brightness = lednice_led->info.max_brightness;
	lednice_led->led_cdev.flags = LED_HW_PLUGGABLE;
	lednice_led->led_cdev.brightness_set_blocking = lednice_led_brightness_set;
	lednice_led->led_cdev.brightness_get = lednice_led_brightness_get;

	err = led_classdev_register(&lednice->udev->dev, &lednice_led->led_cdev);
	if (err)
		goto err_led_classdev_register;
	return 0;

err_led_classdev_register:
	kfree(lednice_led->led_cdev.name);
	return err;
}

static void lednice_led_fini(struct lednice *lednice, u16 led_id)
{
	struct lednice_led *lednice_led = &lednice->leds[led_id];

	led_classdev_unregister(&lednice_led->led_cdev);
	kfree(lednice_led->led_cdev.name);
}

static int lednice_init(struct lednice *lednice)
{
	int count;
	int err;
	int i;

	err = lednice_usb_read(lednice, LEDNICE_USB_CMD_GET_INFO, 0, 0,
			       &lednice->info, sizeof(lednice->info));
	if (err)
		return err;
	lednice_fix_name(lednice->info.dev_name);
	count = lednice->info.led_count;
	lednice->leds = kzalloc(sizeof(struct lednice_led) * count, GFP_KERNEL);
	if (!lednice->leds)
		return -ENOMEM;
	for (i = 0; i < count; i++) {
		err = lednice_led_init(lednice, i);
		if (err)
			goto err_lednice_led_init;
	}
	return err;

err_lednice_led_init:
	for (i--; i >= 0; i--)
		lednice_led_fini(lednice, i);
	kfree(lednice->leds);
	return err;
}

static void lednice_fini(struct lednice *lednice)
{
	int i;

	for (i = 0; i < lednice->info.led_count; i++)
		lednice_led_fini(lednice, i);
	kfree(lednice->leds);
}

static int lednice_probe(struct usb_interface *interface,
			 const struct usb_device_id *id)
{
	struct usb_device *udev = interface_to_usbdev(interface);
	struct lednice *lednice;
	int err;

	lednice = kzalloc(sizeof(*lednice), GFP_KERNEL);
	if (!lednice)
		return -ENOMEM;
	lednice->udev = udev;
	mutex_init(&lednice->lock);

	err = lednice_init(lednice);
	if (err)
		goto err_get_info;

	usb_set_intfdata(interface, lednice);
	return 0;

err_get_info:
	kfree(lednice);
	return err;
}

static void lednice_disconnect(struct usb_interface *interface)
{
	struct lednice *lednice = usb_get_intfdata(interface);

	lednice_fini(lednice);
	kfree(lednice);
}

static struct usb_driver lednice_driver = {
	.name = lednice_driver_name,
	.probe = lednice_probe,
	.disconnect = lednice_disconnect,
	.id_table = lednice_id_table,
};

module_usb_driver(lednice_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Jiri Pirko <jiri@resnulli.us>");
MODULE_DESCRIPTION("Lednice generic USB led device support");
MODULE_DEVICE_TABLE(usb, lednice_id_table);
