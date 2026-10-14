/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Character device nodes of CTLV instances, in a subsystem's own class.
 */
#ifndef _LINUX_CTLV_DEVICE_H
#define _LINUX_CTLV_DEVICE_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/idr.h>
#include <linux/types.h>

struct ctlv;
struct module;

/**
 * struct ctlv_device_class - The class of a subsystem's nodes.
 * @class: the class, whose dev_release frees a node's structure; CTLV sets
 *	its dev_uevent
 * @max_devices: how many nodes the class may have at once
 */
struct ctlv_device_class {
	struct class class;
	unsigned int max_devices;
	/* private: */
	struct module *owner;
	dev_t devt;
	struct ida ids;
};

/**
 * struct ctlv_device - The node of one instance.
 * @dev: the node's device, named for its class and a number of its own
 *
 * Embedded beside the instance in the subsystem's device structure.
 */
struct ctlv_device {
	struct device dev;
	/* private: */
	struct cdev cdev;
	struct ctlv *ctlv;
	struct ctlv_device_class *cls;
};

int __ctlv_device_class_register(struct ctlv_device_class *cls, struct module *owner);
#define ctlv_device_class_register(cls) __ctlv_device_class_register(cls, THIS_MODULE)
void ctlv_device_class_unregister(struct ctlv_device_class *cls);

int ctlv_device_init(struct ctlv_device *ctlvdev, struct ctlv_device_class *cls,
		     struct device *parent, struct ctlv *ctlv);
void ctlv_device_fini(struct ctlv_device *ctlvdev);
int ctlv_device_add(struct ctlv_device *ctlvdev);
void ctlv_device_del(struct ctlv_device *ctlvdev);

#endif /* _LINUX_CTLV_DEVICE_H */
