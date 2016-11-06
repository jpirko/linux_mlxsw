/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * include/linux/actuator.h - Generic actuator support
 * Copyright (c) 2018 Jiri Pirko <jiri@resnulli.us>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _ACTUATOR_H_
#define _ACTUATOR_H_

#include <linux/types.h>
#include <linux/bitops.h>
#include <uapi/linux/actuator.h>

struct actuator;

typedef u32 actuator_vector_items;

#define __ACTUATOR_VECTOR_BIT(bit) (1 << (bit))
#define __ACTUATOR_VECTOR(name) \
	__ACTUATOR_VECTOR_BIT(ACTUATOR_ATTR_VECTOR_##name)

#define ACTUATOR_VECTOR_MOVE		BIT(0)
#define ACTUATOR_VECTOR_STOP		BIT(1)
#define ACTUATOR_VECTOR_VALUE		BIT(2)
#define ACTUATOR_VECTOR_INC_VALUE	BIT(3)
#define ACTUATOR_VECTOR_DIR		BIT(4)
#define ACTUATOR_VECTOR_SPEED		BIT(5)

struct actuator_vector;

bool actuator_vector_move_get(struct actuator_vector *vector);
void actuator_vector_move_set(struct actuator_vector *vector);
bool actuator_vector_stop_get(struct actuator_vector *vector);
void actuator_vector_stop_set(struct actuator_vector *vector);
bool actuator_vector_value_present(struct actuator_vector *vector);
u32 actuator_vector_value_get(struct actuator_vector *vector);
void actuator_vector_value_set(struct actuator_vector *vector, u32 value);
bool actuator_vector_inc_value_present(struct actuator_vector *vector);
u32 actuator_vector_inc_value_get(struct actuator_vector *vector);
void actuator_vector_inc_value_set(struct actuator_vector *vector,
				   u32 inc_value);
bool actuator_vector_dir_present(struct actuator_vector *vector);
enum actuator_dir actuator_vector_dir_get(struct actuator_vector *vector);
void actuator_vector_dir_set(struct actuator_vector *vector,
			     enum actuator_dir dir);
bool actuator_vector_speed_present(struct actuator_vector *vector);
u32 actuator_vector_speed_get(struct actuator_vector *vector);
void actuator_vector_speed_set(struct actuator_vector *vector, u32 speed);

struct actuator_vector_set_op {
	actuator_vector_items items;
	int (*op)(struct actuator *act, struct actuator_vector *vector);
};

#define ACTUATOR_VECTOR_SET_OP(_items, _op)	\
	{					\
		.items = _items,		\
		.op = _op,			\
	}

struct actuator_ops {
	size_t priv_size;
	const char *driver_name;
	enum actuator_type type;
	enum actuator_units units;
	int (*vector_get)(struct actuator *act,
			  struct actuator_vector *vector);
	const struct actuator_vector_set_op *vector_set_ops;
	unsigned int vector_set_ops_count;
};

void actuator_notify_vector(struct actuator *act,
			    struct actuator_vector *vector);
void *actuator_priv(struct actuator *act);
struct actuator_vector *actuator_clean_tmp_vector(struct actuator *act);
struct actuator *actuator_alloc(const struct actuator_ops *aco);
void actuator_free(struct actuator *act);
int actuator_register(struct actuator *act, struct device *dev,
		      unsigned int index);
void actuator_unregister(struct actuator *act);

#endif
