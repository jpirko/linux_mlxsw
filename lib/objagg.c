// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2018 Mellanox Technologies. All rights reserved */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/refcount.h>
#include <linux/list.h>
#include <linux/rhashtable.h>
#include <linux/objagg.h>

struct objagg {
	const struct objagg_ops *ops;
	void *priv;
	struct rhashtable obj_ht;
	struct rhashtable_params ht_params;
	struct list_head obj_list;
};

struct objagg_obj {
	struct rhash_head ht_node; /* member of objagg->obj_ht */
	struct list_head list; /* member of objagg->obj_list */
	struct objagg_obj *parent; /* if the object is nested, this
				    * holds pointer to parent, otherwise NULL
				    */
	union {
		void *delta_priv; /* user delta private */
		void *root_priv; /* user root private */
	};
	refcount_t refcount; /* counts number of users of this object
			      * including nested objects.
			      */
	unsigned long obj[0];
};

static bool objagg_obj_is_root(const struct objagg_obj *objagg_obj)
{
	/* Nesting is not supported, so we can use ->parent
	 * to figure out if the object is root.
	 */
	return !objagg_obj->parent;
}

const void *objagg_obj_root_priv(const struct objagg_obj *objagg_obj)
{
	if (objagg_obj_is_root(objagg_obj))
		return objagg_obj->root_priv;
	WARN_ON(!objagg_obj_is_root(objagg_obj->parent));
	return objagg_obj->parent->root_priv;
}
EXPORT_SYMBOL(objagg_obj_root_priv);

const void *objagg_obj_delta_priv(const struct objagg_obj *objagg_obj)
{
	if (objagg_obj_is_root(objagg_obj))
		return NULL;
	return objagg_obj->delta_priv;
}
EXPORT_SYMBOL(objagg_obj_delta_priv);

const void *objagg_obj_raw(const struct objagg_obj *objagg_obj)
{
	return objagg_obj->obj;
}
EXPORT_SYMBOL(objagg_obj_raw);

static struct objagg_obj *objagg_obj_lookup(struct objagg *objagg, void *obj)
{
	return rhashtable_lookup_fast(&objagg->obj_ht, obj, objagg->ht_params);
}

static int objagg_obj_parent_assign(struct objagg *objagg,
				    struct objagg_obj *objagg_obj)
{
	struct objagg_obj *objagg_obj_cur;
	void *delta_priv;

	list_for_each_entry(objagg_obj_cur, &objagg->obj_list, list) {
		/* Nesting is not supported. In case the object
		 * is not root, it cannot be assigned as parent.
		 */
		if (!objagg_obj_is_root(objagg_obj_cur))
			continue;
		delta_priv = objagg->ops->delta_create(objagg->priv,
						       objagg_obj_cur->obj,
						       objagg_obj->obj);
		if (!IS_ERR(delta_priv)) {
			/* User returned a delta private, that means that
			 * our object can be aggregated into the cursor.
			 */
			objagg_obj->parent = objagg_obj_cur;
			objagg_obj->delta_priv = delta_priv;
			refcount_inc(&objagg_obj->parent->refcount);
			return 0;
		}
	}
	return -ENOENT;
}

static void objagg_obj_parent_unassign(struct objagg *objagg,
				       struct objagg_obj *objagg_obj)
{
	objagg->ops->delta_destroy(objagg->priv, objagg_obj->delta_priv);
	objagg_obj_put(objagg, objagg_obj->parent);
}

static int objagg_obj_root_create(struct objagg *objagg,
				  struct objagg_obj *objagg_obj)
{
	objagg_obj->root_priv = objagg->ops->root_create(objagg->priv,
							 objagg_obj->obj);
	return PTR_ERR_OR_ZERO(objagg_obj->root_priv);
}

static void objagg_obj_root_destroy(struct objagg *objagg,
				    struct objagg_obj *objagg_obj)
{
	objagg->ops->root_destroy(objagg->priv, objagg_obj->root_priv);
}

static int objagg_obj_init(struct objagg *objagg,
			   struct objagg_obj *objagg_obj)
{
	int err;

	/* Try to find if the object can be aggregated under an existing one. */
	err = objagg_obj_parent_assign(objagg, objagg_obj);
	if (!err)
		return 0;
	/* If aggregation is not possible, make the object a root. */
	return objagg_obj_root_create(objagg, objagg_obj);
}

static void objagg_obj_fini(struct objagg *objagg,
			    struct objagg_obj *objagg_obj)
{
	if (!objagg_obj_is_root(objagg_obj))
		objagg_obj_parent_unassign(objagg, objagg_obj);
	else
		objagg_obj_root_destroy(objagg, objagg_obj);
}

struct objagg_obj *objagg_obj_get(struct objagg *objagg, void *obj)
{
	struct objagg_obj *objagg_obj;
	int err;

	/* First, try to find the object exactly as user passed that,
	 * perhaps it is already in use.
	 */
	objagg_obj = objagg_obj_lookup(objagg, obj);
	if (objagg_obj) {
		refcount_inc(&objagg_obj->refcount);
		return objagg_obj;
	}

	objagg_obj = kzalloc(sizeof(*objagg_obj) + objagg->ops->obj_size,
			     GFP_KERNEL);
	if (!objagg_obj)
		return ERR_PTR(-ENOMEM);
	refcount_set(&objagg_obj->refcount, 1);
	memcpy(objagg_obj->obj, obj, objagg->ops->obj_size);

	err = objagg_obj_init(objagg, objagg_obj);
	if (err)
		goto err_obj_init;

	err = rhashtable_insert_fast(&objagg->obj_ht, &objagg_obj->ht_node,
				     objagg->ht_params);
	if (err)
		goto err_ht_insert;
	list_add(&objagg_obj->list, &objagg->obj_list);

	return objagg_obj;

err_ht_insert:
	objagg_obj_fini(objagg, objagg_obj);
err_obj_init:
	kfree(objagg_obj);
	return ERR_PTR(err);
}
EXPORT_SYMBOL(objagg_obj_get);

void objagg_obj_put(struct objagg *objagg, struct objagg_obj *objagg_obj)
{
	if (!refcount_dec_and_test(&objagg_obj->refcount))
		return;
	list_del(&objagg_obj->list);
	rhashtable_remove_fast(&objagg->obj_ht, &objagg_obj->ht_node,
			       objagg->ht_params);
	objagg_obj_fini(objagg, objagg_obj);
	kfree(objagg_obj);
}
EXPORT_SYMBOL(objagg_obj_put);

struct objagg *objagg_create(const struct objagg_ops *ops, void *priv)
{
	struct objagg *objagg;
	int err;

	if (WARN_ON(!ops || !ops->root_create || !ops->root_destroy ||
		    !ops->delta_create || !ops->delta_destroy))
		return ERR_PTR(-EINVAL);
	objagg = kzalloc(sizeof(*objagg), GFP_KERNEL);
	if (!objagg)
		return ERR_PTR(-ENOMEM);
	objagg->ops = ops;
	objagg->priv = priv;
	INIT_LIST_HEAD(&objagg->obj_list);

	objagg->ht_params.key_len = ops->obj_size;
	objagg->ht_params.key_offset = offsetof(struct objagg_obj, obj);
	objagg->ht_params.head_offset = offsetof(struct objagg_obj, ht_node);

	err = rhashtable_init(&objagg->obj_ht, &objagg->ht_params);
	if (err)
		goto err_rhashtable_init;

	return objagg;

err_rhashtable_init:
	kfree(objagg);
	return ERR_PTR(err);
}
EXPORT_SYMBOL(objagg_create);

void objagg_destroy(struct objagg *objagg)
{
	WARN_ON(!list_empty(&objagg->obj_list));
	rhashtable_destroy(&objagg->obj_ht);
	kfree(objagg);
}
EXPORT_SYMBOL(objagg_destroy);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jiri Pirko <jiri@mellanox.com>");
MODULE_DESCRIPTION("Object aggregation manager");
