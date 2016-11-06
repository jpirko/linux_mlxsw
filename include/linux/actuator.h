/*
 * include/linux/actuator.h - Generic actuator support
 * Copyright (c) 2016-2017 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _ACTUATOR_H_
#define _ACTUATOR_H_

#include <linux/types.h>
#include <uapi/linux/actuator.h>

struct actuator;

struct actuator_status {
	u32 value;
	enum actuator_move_state move_state;
};

struct actuator_ops {
	size_t priv_size;
	const char *driver_name;
	enum actuator_type type;
	enum actuator_units units;
	int (*status)(struct actuator *act, struct actuator_status *status);
	int (*move)(struct actuator *act, u32 value);
	int (*stop)(struct actuator *act);
};

void actuator_notify_status(struct actuator *act,
			    struct actuator_status *status);
void *actuator_priv(struct actuator *act);
struct actuator *actuator_alloc(const struct actuator_ops *aco);
void actuator_free(struct actuator *act);
int actuator_register(struct actuator *act, struct device *dev,
		      unsigned int index);
void actuator_unregister(struct actuator *act);

#endif
