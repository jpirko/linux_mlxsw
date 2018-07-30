/* SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0 */
/* Copyright (c) 2018 Mellanox Technologies. All rights reserved */

#ifndef _OBJAGG_H
#define _OBJAGG_H

struct objagg_ops {
	size_t obj_size;
	void * (*delta_create)(void *priv, void *parent_obj, void *obj);
	void (*delta_destroy)(void *priv, void *delta_priv);
	void * (*root_create)(void *priv, void *obj);
	void (*root_destroy)(void *priv, void *root_priv);
};

struct objagg;
struct objagg_obj;

const void *objagg_obj_root_priv(const struct objagg_obj *objagg_obj);
const void *objagg_obj_delta_priv(const struct objagg_obj *objagg_obj);
const void *objagg_obj_raw(const struct objagg_obj *objagg_obj);

struct objagg_obj *objagg_obj_get(struct objagg *objagg, void *obj);
void objagg_obj_put(struct objagg *objagg, struct objagg_obj *objagg_obj);
struct objagg *objagg_create(const struct objagg_ops *ops, void *priv);
void objagg_destroy(struct objagg *objagg);

#endif
