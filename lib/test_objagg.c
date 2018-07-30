// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2018 Mellanox Technologies. All rights reserved */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/objagg.h>

struct key {
	unsigned int id;
};

#define BUF_LEN 128

struct world {
	unsigned int root_count;
	unsigned int delta_count;
	char next_root_buf[BUF_LEN];
};

struct root {
	struct key key;
	char buf[BUF_LEN];
};

struct delta {
	unsigned int key_id_diff;
};

#define MAX_KEY_ID_DIFF 5

static void *delta_create(void *priv, void *parent_obj, void *obj)
{
	struct key *parent_key = parent_obj;
	struct world *world = priv;
	struct key *key = obj;
	int diff = key->id - parent_key->id;
	struct delta *delta;

	if (diff < 0 || diff > MAX_KEY_ID_DIFF)
		return ERR_PTR(-EINVAL);

	delta = kzalloc(sizeof(*delta), GFP_KERNEL);
	if (!delta)
		return ERR_PTR(-ENOMEM);
	delta->key_id_diff = diff;
	world->delta_count++;
	return delta;
}

static void delta_destroy(void *priv, void *delta_priv)
{
	struct delta *delta = delta_priv;
	struct world *world = priv;

	world->delta_count--;
	kfree(delta);
}

static void *root_create(void *priv, void *obj)
{
	struct world *world = priv;
	struct key *key = obj;
	struct root *root;

	root = kzalloc(sizeof(*root), GFP_KERNEL);
	if (!root)
		return ERR_PTR(-ENOMEM);
	memcpy(&root->key, key, sizeof(root->key));
	memcpy(root->buf, world->next_root_buf, sizeof(root->buf));
	world->root_count++;
	return root;
}

static void root_destroy(void *priv, void *root_priv)
{
	struct root *root = root_priv;
	struct world *world = priv;

	world->root_count--;
	kfree(root);
}

static struct objagg_obj *test_nodelta_obj_get(struct world *world,
					       struct objagg *objagg,
					       unsigned int key_id,
					       bool should_create_root)
{
	unsigned int orig_root_count = world->root_count;
	struct objagg_obj *objagg_obj;
	const struct root *root;
	struct key key;
	int err;

	if (should_create_root)
		prandom_bytes(world->next_root_buf,
			      sizeof(world->next_root_buf));

	key.id = key_id;
	objagg_obj = objagg_obj_get(objagg, &key);
	if (IS_ERR(objagg_obj)) {
		pr_err("Key %u: Failed to get object.\n", key_id);
		return objagg_obj;
	}
	if (should_create_root) {
		if (world->root_count != orig_root_count + 1) {
			pr_err("Key %u: Root was not created\n", key_id);
			err = -EINVAL;
			goto err_check_root_count;
		}
	} else {
		if (world->root_count != orig_root_count) {
			pr_err("Key %u: Root was incorrectly created\n",
			       key_id);
			err = -EINVAL;
			goto err_check_root_count;
		}
	}
	root = objagg_obj_root_priv(objagg_obj);
	if (root->key.id != key_id) {
		pr_err("Key %u: Root has unexpected key id\n", key_id);
		err = -EINVAL;
		goto err_check_key_id;
	}
	if (should_create_root &&
	    memcmp(world->next_root_buf, root->buf, sizeof(root->buf))) {
		pr_err("Key %u: Buffer does not match the expected content\n",
		       key_id);
		err = -EINVAL;
		goto err_check_buf;
	}
	return objagg_obj;

err_check_buf:
err_check_key_id:
err_check_root_count:
	objagg_obj_put(objagg, objagg_obj);
	return ERR_PTR(err);
}

static int test_nodelta_obj_put(struct world *world,
				struct objagg *objagg,
				struct objagg_obj *objagg_obj,
				bool should_destroy_root)
{
	const struct root *root = objagg_obj_root_priv(objagg_obj);
	unsigned int orig_root_count = world->root_count;
	unsigned int key_id = root->key.id;

	objagg_obj_put(objagg, objagg_obj);

	if (should_destroy_root) {
		if (world->root_count != orig_root_count - 1) {
			pr_err("Key %u: Root was not destroyed\n", key_id);
			return -EINVAL;
		}
	} else {
		if (world->root_count != orig_root_count) {
			pr_err("Key %u: Root was incorrectly destroyed\n",
			       key_id);
			return -EINVAL;
		}
	}
	return 0;
}

#define NODELTA_KEYS 20

static void *delta_create_dummy(void *priv, void *parent_obj, void *obj)
{
	return ERR_PTR(-EINVAL);
}

static void delta_destroy_dummy(void *priv, void *delta_priv)
{
}

static const struct objagg_ops nodelta_ops = {
	.obj_size = sizeof(struct key),
	.delta_create = delta_create_dummy,
	.delta_destroy = delta_destroy_dummy,
	.root_create = root_create,
	.root_destroy = root_destroy,
};

static int test_nodelta(void)
{
	struct objagg_obj *objagg_objs[NODELTA_KEYS];
	struct objagg_obj *objagg_obj;
	struct world world = {};
	struct objagg *objagg;
	int i;
	int err;

	objagg = objagg_create(&nodelta_ops, &world);
	if (IS_ERR(objagg))
		return PTR_ERR(objagg);

	/* First round of gets, the root objects should be created */
	for (i = 0; i < NODELTA_KEYS; i++) {
		objagg_objs[i] = test_nodelta_obj_get(&world, objagg, i, true);
		if (IS_ERR(objagg_objs[i])) {
			err = PTR_ERR(objagg_objs[i]);
			goto err_obj_first_get;
		}
	}

	/* Do the second round of gets, all roots are already created,
	 * make sure that no new root is created
	 */
	for (i = 0; i < NODELTA_KEYS; i++) {
		objagg_obj = test_nodelta_obj_get(&world, objagg, i, false);
		if (IS_ERR(objagg_obj)) {
			err = PTR_ERR(objagg_obj);
			goto err_obj_second_get;
		}
	}

	for (i = NODELTA_KEYS - 1; i >= 0; i--) {
		err = test_nodelta_obj_put(&world, objagg,
					   objagg_objs[i], false);
		if (err)
			goto err_obj_first_put;
	}
	for (i = NODELTA_KEYS - 1; i >= 0; i--) {
		err = test_nodelta_obj_put(&world, objagg,
					   objagg_objs[i], true);
		if (err)
			goto err_obj_second_put;
	}
	objagg_destroy(objagg);
	return 0;

err_obj_second_get:
err_obj_first_put:
	for (i--; i >= 0; i--)
		objagg_obj_put(objagg, objagg_objs[i]);

	i = NODELTA_KEYS;
err_obj_first_get:
err_obj_second_put:
	for (i--; i >= 0; i--)
		objagg_obj_put(objagg, objagg_objs[i]);
	objagg_destroy(objagg);
	return err;
}

static const struct objagg_ops delta_ops = {
	.obj_size = sizeof(struct key),
	.delta_create = delta_create,
	.delta_destroy = delta_destroy,
	.root_create = root_create,
	.root_destroy = root_destroy,
};

enum action {
	ACTION_GET,
	ACTION_PUT,
};

enum expect_delta {
	EXPECT_DELTA_SAME,
	EXPECT_DELTA_INC,
	EXPECT_DELTA_DEC,
};

enum expect_root {
	EXPECT_ROOT_SAME,
	EXPECT_ROOT_INC,
	EXPECT_ROOT_DEC,
};

struct action_item {
	unsigned int key_id;
	enum action action;
	enum expect_delta expect_delta;
	enum expect_root expect_root;
};

static const struct action_item action_items[] = {
	{1, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_INC},	/* r: 1			d: */
	{7, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_INC},	/* r: 1, 7		d: */
	{3, ACTION_GET, EXPECT_DELTA_INC, EXPECT_ROOT_SAME},	/* r: 1, 7		d: 3^1 */
	{5, ACTION_GET, EXPECT_DELTA_INC, EXPECT_ROOT_SAME},	/* r: 1, 7		d: 3^1, 5^1 */
	{3, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 1, 7		d: 3^1, 3^1, 5^1 */
	{1, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 1, 1, 7		d: 3^1, 3^1, 5^1 */
	{30, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_INC},	/* r: 1, 1, 7, 30	d: 3^1, 3^1, 5^1 */
	{8, ACTION_GET, EXPECT_DELTA_INC, EXPECT_ROOT_SAME},	/* r: 1, 1, 7, 30	d: 3^1, 3^1, 5^1, 8^7 */
	{8, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 1, 1, 7, 30	d: 3^1, 3^1, 5^1, 8^7, 8^7 */
	{3, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 1, 1, 7, 30	d: 3^1, 5^1, 8^7, 8^7 */
	{3, ACTION_PUT, EXPECT_DELTA_DEC, EXPECT_ROOT_SAME},	/* r: 1, 1, 7, 30	d: 5^1, 8^7, 8^7 */
	{1, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 1, 7, 30		d: 5^1, 8^7, 8^7 */
	{1, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 7, 30		d: 5^1, 8^7, 8^7 */
	{5, ACTION_PUT, EXPECT_DELTA_DEC, EXPECT_ROOT_DEC},	/* r: 7, 30		d: 8^7, 8^7 */
	{5, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_INC},	/* r: 5, 7, 30		d: 8^7, 8^7 */
	{6, ACTION_GET, EXPECT_DELTA_INC, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5, 8^7, 8^7 */
	{8, ACTION_GET, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5, 8^7, 8^7, 8^7 */
	{8, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5, 8^7, 8^7 */
	{8, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5, 8^7 */
	{8, ACTION_PUT, EXPECT_DELTA_DEC, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5 */
	{8, ACTION_GET, EXPECT_DELTA_INC, EXPECT_ROOT_SAME},	/* r: 5, 7, 30		d: 6^5, 8^5 */
	{7, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_DEC},	/* r: 5, 30		d: 6^5, 8^5 */
	{30, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_DEC},	/* r: 5			d: 6^5, 8^5 */
	{5, ACTION_PUT, EXPECT_DELTA_SAME, EXPECT_ROOT_SAME},	/* r:			d: 6^5, 8^5 */
	{6, ACTION_PUT, EXPECT_DELTA_DEC, EXPECT_ROOT_SAME},	/* r:			d: 6^5 */
	{8, ACTION_PUT, EXPECT_DELTA_DEC, EXPECT_ROOT_DEC},	/* r:			d: */
};

static const unsigned int key_ids[] = {1, 7, 3, 5, 30, 8, 6};

static int key_id_index(unsigned int key_id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(key_ids); i++) {
		if (key_ids[i] == key_id)
			return i;
	}
	WARN_ON(1);
	return 0;
}

static int check_expect(struct world *world,
			const struct action_item *action_item,
			unsigned int orig_delta_count,
			unsigned int orig_root_count)
{
	unsigned int key_id = action_item->key_id;

	switch (action_item->expect_delta) {
	case EXPECT_DELTA_SAME:
		if (orig_delta_count != world->delta_count) {
			pr_err("Key %u: Delta count changed while expected to remain the same.\n",
			       key_id);
			return -EINVAL;
		}
		break;
	case EXPECT_DELTA_INC:
		if (WARN_ON(action_item->action == ACTION_PUT))
			return -EINVAL;
		if (orig_delta_count + 1 != world->delta_count) {
			pr_err("Key %u: Delta count was not incremented.\n",
			       key_id);
			return -EINVAL;
		}
		break;
	case EXPECT_DELTA_DEC:
		if (WARN_ON(action_item->action == ACTION_GET))
			return -EINVAL;
		if (orig_delta_count - 1 != world->delta_count) {
			pr_err("Key %u: Delta count was not decremented.\n",
			       key_id);
			return -EINVAL;
		}
		break;
	}

	switch (action_item->expect_root) {
	case EXPECT_ROOT_SAME:
		if (orig_root_count != world->root_count) {
			pr_err("Key %u: Root count changed while expected to remain the same.\n",
			       key_id);
			return -EINVAL;
		}
		break;
	case EXPECT_ROOT_INC:
		if (WARN_ON(action_item->action == ACTION_PUT))
			return -EINVAL;
		if (orig_root_count + 1 != world->root_count) {
			pr_err("Key %u: Root count was not incremented.\n",
			       key_id);
			return -EINVAL;
		}
		break;
	case EXPECT_ROOT_DEC:
		if (WARN_ON(action_item->action == ACTION_GET))
			return -EINVAL;
		if (orig_root_count - 1 != world->root_count) {
			pr_err("Key %u: Root count was not decremented.\n",
			       key_id);
			return -EINVAL;
		}
	}
	return 0;
}

static int test_delta_action_item(struct world *world,
				  struct objagg *objagg,
				  struct objagg_obj **objagg_objs,
				  unsigned int *key_refs,
				  const struct action_item *action_item,
				  bool inverse)
{
	unsigned int orig_delta_count = world->delta_count;
	unsigned int orig_root_count = world->root_count;
	unsigned int key_id = action_item->key_id;
	enum action action = action_item->action;
	struct objagg_obj *objagg_obj;
	struct key key;
	int err;

	if (inverse)
		action = action == ACTION_GET ? ACTION_PUT : ACTION_GET;

	switch (action) {
	case ACTION_GET:
		key.id = key_id;
		objagg_obj = objagg_obj_get(objagg, &key);
		if (IS_ERR(objagg_obj)) {
			pr_err("Key %u: Failed to get object.\n", key_id);
			return PTR_ERR(objagg_obj);
		}
		if (!key_refs[key_id_index(key_id)]++) {
			objagg_objs[key_id_index(key_id)] = objagg_obj;
		} else if (objagg_objs[key_id_index(key_id)] != objagg_obj &&
			   inverse) {
			pr_err("Key %u: God another object for the same key.\n",
			       key_id);
			err = -EINVAL;
			goto errout;
		}
		break;
	case ACTION_PUT:
		if (!key_refs[key_id_index(key_id)])
			break;
		objagg_obj = objagg_objs[key_id_index(key_id)];
		objagg_obj_put(objagg, objagg_obj);
		key_refs[key_id_index(key_id)]--;
		break;
	}

	if (inverse)
		return 0;
	err = check_expect(world, action_item,
			   orig_delta_count, orig_root_count);
	if (err)
		goto errout;
	return 0;

errout:
	/* This can only happen when action is not inversed.
	 * So in case of an error, cleanup by doing inverse action.
	 */
	test_delta_action_item(world, objagg, objagg_objs,
			       key_refs, action_item, true);
	return err;
}

static int test_delta(void)
{
	struct objagg_obj *objagg_objs[ARRAY_SIZE(key_ids)];
	unsigned int key_refs[ARRAY_SIZE(key_ids)] = {};
	struct world world = {};
	struct objagg *objagg;
	int i;
	int err;

	objagg = objagg_create(&delta_ops, &world);
	if (IS_ERR(objagg))
		return PTR_ERR(objagg);

	for (i = 0; i < ARRAY_SIZE(action_items); i++) {
		err = test_delta_action_item(&world, objagg, objagg_objs,
					     key_refs, &action_items[i], false);
		if (err)
			goto err_do_action_item;
	}

	objagg_destroy(objagg);
	return 0;

err_do_action_item:
	for (i--; i >= 0; i--)
		err = test_delta_action_item(&world, objagg, objagg_objs,
					     key_refs, &action_items[i], true);

	objagg_destroy(objagg);
	return err;
}

static int __init test_objagg_init(void)
{
	int err;

	err = test_nodelta();
	if (err)
		return err;
	return test_delta();
}

static void __exit test_objagg_exit(void)
{
}

module_init(test_objagg_init);
module_exit(test_objagg_exit);
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Jiri Pirko <jiri@mellanox.com>");
MODULE_DESCRIPTION("Test module for objagg");
