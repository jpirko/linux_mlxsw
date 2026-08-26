// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 */
#define pr_fmt(fmt) "ctlv: " fmt
#include <kunit/static_stub.h>
#include <kunit/visibility.h>
#include <linux/align.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/build_bug.h>
#include <linux/cleanup.h>
#include <linux/container_of.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kdev_t.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/lockdep.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/overflow.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/stdarg.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <uapi/linux/ctlv.h>

#include "blob.h"
#include "ctlv_private.h"

enum {
	CTLV_MAX_DEVICES = 4096,
};

static_assert(CTLV_MAX_DEVICES < (1U << MINORBITS));
static_assert(!(CTLV_PRIV_ALIGN & (CTLV_PRIV_ALIGN - 1)));
static_assert(__alignof__(void *) <= CTLV_PRIV_ALIGN);

/*
 * One layout on every ABI, including compat. Alignment is asserted too because
 * a 32-bit ABI aligns a plain __u64 to four bytes.
 */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(__alignof__(struct ctlv_attr) == CTLV_ALIGNTO);
static_assert(offsetof(struct ctlv_attr, len) == 0);
static_assert(offsetof(struct ctlv_attr, flags) == 4);
static_assert(offsetof(struct ctlv_attr, attr_id) == 8);

static_assert(sizeof(struct ctlv_op_exec) == 64);
static_assert(__alignof__(struct ctlv_op_exec) == CTLV_ALIGNTO);
static_assert(offsetof(struct ctlv_op_exec, op_id) == 0);
static_assert(offsetof(struct ctlv_op_exec, request) == 8);
static_assert(offsetof(struct ctlv_op_exec, reply) == 16);
static_assert(offsetof(struct ctlv_op_exec, request_len) == 24);
static_assert(offsetof(struct ctlv_op_exec, reply_len) == 28);
static_assert(offsetof(struct ctlv_op_exec, generation) == 32);
static_assert(offsetof(struct ctlv_op_exec, cursor) == 40);
static_assert(offsetof(struct ctlv_op_exec, reserved) == 48);

static_assert(sizeof(struct ctlv_event_hdr) == 24);
static_assert(__alignof__(struct ctlv_event_hdr) == CTLV_ALIGNTO);
static_assert(offsetof(struct ctlv_event_hdr, length) == 0);
static_assert(offsetof(struct ctlv_event_hdr, flags) == 4);
static_assert(offsetof(struct ctlv_event_hdr, op_id) == 8);
static_assert(offsetof(struct ctlv_event_hdr, sequence) == 16);

/* Generated metadata checks these bounds without including this header. */
static_assert(UEVENT_BUFFER_SIZE / 2 == CTLV_UEVENT_MAX_LEN);
static_assert(UEVENT_NUM_ENVP / 2 == CTLV_UEVENT_MAX_VARS);

/* One op entry holds one callback, whichever of the two names it is read by. */
static_assert(sizeof(ctlv_action_fn) == sizeof(ctlv_query_fn));

static dev_t ctlv_devt;
static DEFINE_IDA(ctlv_ida);

/*
 * A family ID is a position in Documentation/ctlv/families.yaml. The generator
 * polices it in this tree but not against a module built elsewhere, so the
 * first device of a family records the name it claimed its ID under. The claim
 * outlives that module and copies the name.
 */
struct ctlv_claim {
	struct list_head entry;
	char *name;
	u16 family_id;
};

static LIST_HEAD(ctlv_claims);
static DEFINE_MUTEX(ctlv_claims_lock);

static int ctlv_claim_family(const struct ctlv_family_schema *schema)
{
	struct ctlv_claim *claim;

	guard(mutex)(&ctlv_claims_lock);
	list_for_each_entry(claim, &ctlv_claims, entry) {
		bool same_id = claim->family_id == schema->family_id;
		bool same_name = !strcmp(claim->name, schema->name);

		if (same_id && same_name)
			return 0;
		if (same_id || same_name) {
			pr_err("family %s wants ID %u, which belongs to %s\n",
			       schema->name, schema->family_id, claim->name);
			return -EEXIST;
		}
	}

	claim = kzalloc_obj(*claim, GFP_KERNEL);
	if (!claim)
		return -ENOMEM;
	claim->name = kstrdup(schema->name, GFP_KERNEL);
	if (!claim->name) {
		kfree(claim);
		return -ENOMEM;
	}
	claim->family_id = schema->family_id;
	list_add_tail(&claim->entry, &ctlv_claims);
	return 0;
}

static void ctlv_claims_free(void)
{
	struct ctlv_claim *claim, *next;

	list_for_each_entry_safe(claim, next, &ctlv_claims, entry) {
		list_del(&claim->entry);
		kfree(claim->name);
		kfree(claim);
	}
}

/*
 * Everything resolved below is generated, so a failure here is a bug, checked
 * anyway because an out-of-tree family comes from another generator.
 */

static const char *ctlv_family_name(const struct ctlv_family *family)
{
	if (!family->schema || !family->schema->name)
		return "(unnamed)";
	return family->schema->name;
}

static int ctlv_family_bad(const struct ctlv_family *family, const char *what)
{
	pr_err("family %s: %s\n", ctlv_family_name(family), what);
	return -EINVAL;
}

static int ctlv_op_bad(const struct ctlv_family *family, u64 op_id,
		       const char *what)
{
	pr_err("family %s: op %#018llx %s\n", ctlv_family_name(family), op_id,
	       what);
	return -EINVAL;
}

/*
 * A family is named its parent's whole name plus a suffix, so one name states
 * the chain it belongs to and every family of this framework is ctlv-something.
 */
static int ctlv_chain_check(const struct ctlv_family *family)
{
	const struct ctlv_family *child;

	for (child = family; child != &ctlv_ctlv_family;
	     child = child->parent) {
		const struct ctlv_family *parent;
		size_t len;

		if (!child)
			return ctlv_family_bad(family, "heads no chain of the core's");
		parent = child->parent;
		if (!child->schema || !parent || !parent->schema)
			return ctlv_family_bad(child, "does not name a complete parent descriptor");
		if (!child->schema->name || !parent->schema->name)
			return ctlv_family_bad(family, "inherits a family with no name");
		if (parent->schema->family_id >= child->schema->family_id)
			return ctlv_family_bad(child, "does not follow its parent in the registry");
		len = strlen(parent->schema->name);
		if (!len || strncmp(child->schema->name, parent->schema->name,
				    len) ||
		    child->schema->name[len] != '-')
			return ctlv_family_bad(family, "is not named after the family it inherits");
	}
	return 0;
}

/* One descriptor of a chain: what it answers with, and what it fills. */
static int ctlv_descriptor_check(const struct ctlv_family *family)
{
	const struct ctlv_family_schema *schema = family->schema;
	u32 index;

	/*
	 * A family that declares no op of its own binds nothing: what it
	 * inherited is answered where it was declared.
	 */
	if (!schema || !schema->name || !*schema->name ||
	    (family->n_ops && !family->ops))
		return ctlv_family_bad(family, "descriptor is incomplete");
	/* Nothing in common means an empty pair, not an absent one. */
	if (!family->pre || !family->post)
		return ctlv_family_bad(family, "has no hook pair");
	if (!family->priv_size || !family->init || !family->fini)
		return ctlv_family_bad(family, "has no priv lifecycle");
	/* Every device answers the root family already, as the framework. */
	if (schema->family_id == CTLV_FAMILY_ID &&
	    family != &ctlv_ctlv_family)
		return ctlv_family_bad(family, "claims the root family ID");
	/* An extension point is answered by whichever family filled it. */
	if (family->n_stubs != schema->n_fills)
		return ctlv_family_bad(family, "answers other extension points than it fills");
	for (index = 0; index < family->n_stubs; index++) {
		const struct ctlv_stub_ops *stub = &family->stubs[index];
		const struct ctlv_stub_fill *fill = &schema->fills[index];

		if (stub->family_id == fill->family_id &&
		    stub->set_id == fill->set_id)
			continue;
		return ctlv_family_bad(family, "answers an extension point it does not fill");
	}
	return 0;
}

/* Every descriptor binds its local schema and supplies a complete lifecycle. */
static int ctlv_descriptors_check(const struct ctlv_family *family)
{
	const struct ctlv_family *descriptor;

	for (descriptor = family; descriptor; descriptor = descriptor->parent) {
		int ret;

		ret = ctlv_descriptor_check(descriptor);
		if (ret)
			return ret;
	}
	return 0;
}

/* The graph a device answers over, and the descriptors standing on it. */
static int ctlv_family_check(const struct ctlv_family *family)
{
	int ret;

	if (!family->schema)
		return ctlv_family_bad(family, "descriptor is incomplete");
	ret = ctlv_chain_check(family);
	if (ret)
		return ret;
	return ctlv_descriptors_check(family);
}

static struct ctlv_family_priv *
ctlv_family_priv_find(const struct ctlv_device *ctlvdev,
		      const struct ctlv_family *family)
{
	u32 index;

	for (index = 0; index < ctlvdev->n_privs; index++)
		if (ctlvdev->privs[index].family == family)
			return &ctlvdev->privs[index];
	return NULL;
}

static void ctlv_privs_free(struct ctlv_device *ctlvdev)
{
	kvfree(ctlvdev->priv_block);
	ctlvdev->priv_block = NULL;
	kfree(ctlvdev->privs);
	ctlvdev->privs = NULL;
	ctlvdev->n_privs = 0;
}

static int ctlv_privs_alloc(struct ctlv_device *ctlvdev,
			    const struct ctlv_family *family)
{
	struct ctlv_priv_container *parent = NULL;
	void *aligned;
	size_t alloc_len;
	size_t len = 0;
	u32 index;

	ctlvdev->n_privs = ctlv_family_levels(family);
	ctlvdev->privs = kcalloc(ctlvdev->n_privs, sizeof(*ctlvdev->privs),
				 GFP_KERNEL);
	if (!ctlvdev->privs)
		return -ENOMEM;

	for (index = 0; index < ctlvdev->n_privs; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];
		size_t size, aligned_len;

		family_priv->family =
			ctlv_family_ancestor(family,
					     ctlvdev->n_privs - index - 1);
		size = max_t(size_t, family_priv->family->priv_size(), 1);
		if (check_add_overflow(len,
				       offsetof(struct ctlv_priv_container, priv),
				       &aligned_len) ||
		    check_add_overflow(aligned_len, CTLV_PRIV_ALIGN - 1,
				       &aligned_len))
			goto overflow;
		aligned_len = ALIGN_DOWN(aligned_len, CTLV_PRIV_ALIGN);
		family_priv->priv_offset = aligned_len;
		if (check_add_overflow(aligned_len, size, &len))
			goto overflow;
	}
	if (check_add_overflow(len, CTLV_PRIV_ALIGN - 1, &alloc_len))
		goto overflow;
	ctlvdev->priv_block = kvzalloc(alloc_len, GFP_KERNEL);
	if (!ctlvdev->priv_block) {
		ctlv_privs_free(ctlvdev);
		return -ENOMEM;
	}
	aligned = PTR_ALIGN(ctlvdev->priv_block, CTLV_PRIV_ALIGN);

	for (index = 0; index < ctlvdev->n_privs; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];
		struct ctlv_priv_container *container;
		void *priv;

		priv = (u8 *)aligned + family_priv->priv_offset;
		family_priv->priv = priv;
		container = container_of_const(priv, struct ctlv_priv_container,
					       priv);
		container->parent = parent;
		container->child = NULL;
		if (parent)
			parent->child = container;
		parent = container;
	}
	return 0;

overflow:
	ctlv_privs_free(ctlvdev);
	return -EOVERFLOW;
}

static int ctlv_privs_pre_init(struct ctlv_device *ctlvdev,
			       const void *init_info)
{
	u32 index;

	for (index = ctlvdev->n_privs; index; index--) {
		struct ctlv_family_priv *family_priv =
			&ctlvdev->privs[index - 1];

		family_priv->init_info = init_info;
		if (!family_priv->family->pre_init) {
			init_info = NULL;
			continue;
		}
		init_info = family_priv->family->pre_init(init_info);
		if (IS_ERR(init_info))
			return PTR_ERR(init_info);
	}
	if (WARN_ON_ONCE(init_info))
		return -EINVAL;
	return 0;
}

static void ctlv_privs_pre_fini(struct ctlv_device *ctlvdev)
{
	u32 index;

	lockdep_assert_not_held(&ctlvdev->registration_lock);
	lockdep_assert_not_held(&ctlvdev->transition_lock);
	lockdep_assert_not_held(&ctlvdev->events_lock);

	for (index = 0; index < ctlvdev->n_privs; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];

		if (!family_priv->initialized ||
		    !family_priv->family->pre_fini)
			continue;
		family_priv->family->pre_fini(ctlvdev, family_priv->priv);
	}
}

static void ctlv_privs_fini_only(struct ctlv_device *ctlvdev)
{
	u32 index;

	for (index = ctlvdev->n_privs; index; index--) {
		struct ctlv_family_priv *family_priv =
			&ctlvdev->privs[index - 1];

		if (!family_priv->initialized)
			continue;
		family_priv->family->fini(ctlvdev, family_priv->priv);
		family_priv->initialized = false;
	}
}

static int ctlv_privs_init(struct ctlv_device *ctlvdev)
{
	u32 index;
	int ret;

	for (index = 0; index < ctlvdev->n_privs; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];

		ret = family_priv->family->init(ctlvdev, family_priv->init_info,
						family_priv->priv);
		family_priv->init_info = NULL;
		if (ret) {
			ctlv_privs_fini_only(ctlvdev);
			return ret;
		}
		family_priv->initialized = true;
	}
	return 0;
}

static int ctlv_privs_post_init(struct ctlv_device *ctlvdev)
{
	u32 index;
	int ret;

	for (index = ctlvdev->n_privs; index; index--) {
		struct ctlv_family_priv *family_priv =
			&ctlvdev->privs[index - 1];

		if (!family_priv->family->post_init)
			continue;
		ret = family_priv->family->post_init(ctlvdev,
						     family_priv->priv);
		if (ret)
			return ret;
	}
	return 0;
}

static void ctlv_device_fini(struct ctlv_device *ctlvdev)
{
	ctlv_privs_pre_fini(ctlvdev);

	/* No provider can remain after pre_fini has drained its consumers. */
	scoped_guard(spinlock, &ctlvdev->transition_lock) {
		ctlvdev->destroying = true;
		ctlv_event_contexts_stop(ctlvdev);
	}
	ctlv_event_contexts_wait(ctlvdev);
	ctlv_events_device_dead(ctlvdev);

	ctlv_privs_fini_only(ctlvdev);
}

/* What one walk of the chain has accounted for so far. */
struct ctlv_resolve {
	struct ctlv_device *ctlvdev;
	const struct ctlv_family *target;
	const struct ctlv_family *family;
	u32 bound;
	u32 dynamic;
	u32 events;
	u64 previous;
};

/* Every binding of one family answers an op, so a spare one answers nothing. */
static int ctlv_family_bound(const struct ctlv_resolve *state)
{
	const struct ctlv_family *family = state->family;

	if (!family || state->bound == family->n_ops)
		return 0;
	/* The framework's own table is this framework's bug, not a family's. */
	if (WARN_ON_ONCE(family == &ctlv_ctlv_family))
		return -EINVAL;
	return ctlv_family_bad(family, "binds more than its schema asks for");
}

/*
 * The ops of one family are contiguous in the walk, so the family before this
 * one is complete as soon as another family's op arrives.
 */
static int ctlv_family_enter(struct ctlv_resolve *state,
			     const struct ctlv_family *family)
{
	int ret;

	if (state->family == family)
		return 0;
	ret = ctlv_family_bound(state);
	if (ret)
		return ret;
	state->family = family;
	state->bound = 0;
	return 0;
}

static int ctlv_op_resolve(struct ctlv_resolve *state,
			   const struct ctlv_family *owner,
			   const struct ctlv_op_schema *op,
			   struct ctlv_op_entry *entry)
{
	const struct ctlv_family *target = state->target;
	const struct ctlv_family *family = state->family;
	struct ctlv_family_priv *family_priv;
	int ret;

	family_priv = ctlv_family_priv_find(state->ctlvdev, family);
	if (WARN_ON_ONCE(!family_priv))
		return -EINVAL;
	entry->schema = op;
	entry->priv = family_priv->priv;
	entry->state_index = CTLV_OP_STATE_NONE;

	/*
	 * An op ID is its family's ID and a number within it, so a walk from
	 * the oldest ancestor down ascends, which covers uniqueness too.
	 */
	if (!CTLV_OP_ID_NUMBER(op->op_id) ||
	    CTLV_OP_ID_FAMILY(op->op_id) != owner->schema->family_id)
		return ctlv_op_bad(target, op->op_id,
				   "belongs to another family");
	if (op->op_id <= state->previous)
		return ctlv_op_bad(target, op->op_id, "is out of order");
	state->previous = op->op_id;

	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
	case CTLV_OP_TYPE_QUERY:
		if (op->fd_access != CTLV_OP_FD_ACCESS_READ &&
		    op->fd_access != CTLV_OP_FD_ACCESS_READ_WRITE)
			return ctlv_op_bad(family, op->op_id,
					   "has an unknown fd access");
		/* The family that declared the op is the one that answers it. */
		if (state->bound >= family->n_ops)
			return ctlv_op_bad(family, op->op_id,
					   "has nothing bound to it");
		entry->binding = &family->ops[state->bound++];
		/* Required even for an op this device disables or withholds. */
		if (!entry->binding->action)
			return ctlv_op_bad(family, op->op_id, "has no callback");
		/* An optional op arrives absent, for the opt-in to clear. */
		entry->absent = op->optional;
		/* An op has a whole pair of hooks of its own, or none. */
		if (!entry->binding->pre != !entry->binding->post)
			return ctlv_op_bad(family, op->op_id,
					   "has half a hook pair");
		/* A pair is on no wire, so this is the only check of it. */
		if (!entry->binding->pre != !op->pre_post)
			return ctlv_op_bad(family, op->op_id,
					   "disagrees about its hook pair");
		break;
	case CTLV_OP_TYPE_EVENT:
		if (op->fd_access)
			return ctlv_op_bad(target, op->op_id,
					   "is an event with fd access");
		if (op->optional)
			return ctlv_op_bad(target, op->op_id,
					   "is an event a device could omit");
		/*
		 * Produced rather than answered, so nothing to bind: an event
		 * gets a subscription bit, numbered as the walk meets it.
		 */
		entry->event_index = state->events++;
		break;
	default:
		return ctlv_op_bad(target, op->op_id, "has an unknown type");
	}

	switch (op->availability) {
	case CTLV_OP_AVAILABILITY_FIXED:
		break;
	case CTLV_OP_AVAILABILITY_DYNAMIC:
		if (op->type == CTLV_OP_TYPE_EVENT)
			return ctlv_op_bad(target, op->op_id,
					   "is an event a device could disable");
		/*
		 * Numbered from zero as the walk meets them, per device: two
		 * families of one chain need not have the same ops.
		 */
		entry->state_index = state->dynamic++;
		break;
	default:
		return ctlv_op_bad(target, op->op_id,
				   "has an unknown availability");
	}

	/*
	 * Once per device: what an inherited op may carry depends on the
	 * extension points this family filled and on nothing that changes later.
	 */
	ret = ctlv_schema_op_sizes(target, op, &entry->sizes);
	if (ret)
		return ctlv_op_bad(target, op->op_id,
				   "needs messages this framework cannot carry");
	return 0;
}

/**
 * ctlv_ops_resolve - Build one device's index into its family's schema graph.
 * @ctlvdev: device being allocated
 * @family: generated descriptor it will answer as
 *
 * On success @ctlvdev owns an entry per effective op and one priv per family.
 * The caller frees both on failure.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_ops_resolve(struct ctlv_device *ctlvdev,
		     const struct ctlv_family *family)
{
	struct ctlv_resolve state = {
		.ctlvdev = ctlvdev,
		.target = family,
	};
	const struct ctlv_family *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;
	u32 n_ops = 0;
	int ret;

	ret = ctlv_family_check(family);
	if (ret)
		return ret;
	ret = ctlv_privs_alloc(ctlvdev, family);
	if (ret)
		return ret;

	ctlv_schema_cursor_init(&cursor, family);
	while (ctlv_schema_next_op(&cursor, &owner))
		n_ops++;
	if (!n_ops)
		return ctlv_family_bad(family, "has an effective schema with no op");

	ctlvdev->ops = kcalloc(n_ops, sizeof(*ctlvdev->ops), GFP_KERNEL);
	if (!ctlvdev->ops)
		return -ENOMEM;
	ctlvdev->n_ops = n_ops;

	n_ops = 0;
	ctlv_schema_cursor_init(&cursor, family);
	while ((op = ctlv_schema_next_op(&cursor, &owner))) {
		ret = ctlv_family_enter(&state, owner);
		if (ret)
			return ret;
		ret = ctlv_op_resolve(&state, owner, op, &ctlvdev->ops[n_ops++]);
		if (ret)
			return ret;
	}
	/* The device family, no op of another having ended its table. */
	ret = ctlv_family_bound(&state);
	if (ret)
		return ret;
	ctlvdev->n_dynamic_ops = state.dynamic;
	ctlvdev->n_events = state.events;
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_ops_resolve);

/**
 * ctlv_family_is_a - Whether one generated family inherits another.
 * @family: descriptor a driver is about to create a device with
 * @ancestor: descriptor to look for, @family itself counting as one
 *
 * Ancestry is immutable generated metadata, so this needs no device, invokes
 * no callback and takes no lock.
 *
 * Return: true when @family is @ancestor or inherits it.
 */
bool ctlv_family_is_a(const struct ctlv_family *family,
		      const struct ctlv_family *ancestor)
{
	const struct ctlv_family *fast = family;

	while (family) {
		if (family == ancestor)
			return true;
		family = family->parent;
		if (fast && fast->parent)
			fast = fast->parent->parent;
		else
			fast = NULL;
		if (family && family == fast)
			return false;
	}
	return false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_family_is_a, "CTLV");

/**
 * ctlv_device_family - Family a device was created as.
 * @ctlvdev: device to identify
 *
 * The immutable leaf descriptor remains available through pre_fini and fini.
 * The pointer carries no reference and must not outlive ctlv_destroy().
 *
 * Return: the leaf family descriptor, or NULL for kernel misuse
 */
const struct ctlv_family *
ctlv_device_family(const struct ctlv_device *ctlvdev)
{
	if (WARN_ON_ONCE(!ctlvdev))
		return NULL;
	return READ_ONCE(ctlvdev->family);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_family, "CTLV");

/**
 * ctlv_device_parent - Device-model parent supplied to ctlv_create().
 * @ctlvdev: device to identify
 *
 * The returned pointer carries no reference.
 *
 * Return: the parent device, or NULL for kernel misuse
 */
struct device *ctlv_device_parent(const struct ctlv_device *ctlvdev)
{
	if (WARN_ON_ONCE(!ctlvdev))
		return NULL;
	return ctlvdev->dev.parent;
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_parent, "CTLV");

/*
 * Dispatch reads the availability bit under no lock, so an op already admitted
 * is neither cancelled nor waited for by a disable.
 */

/**
 * ctlv_op_find - Look one op up in what a device resolved.
 * @ctlvdev: device to look in
 * @op_id: complete ID of the op, which names the family that declared it
 *
 * Return: the entry, or NULL when this device has no such op. An unselected
 * optional op is still found until publication drops it.
 */
struct ctlv_op_entry *ctlv_op_find(struct ctlv_device *ctlvdev, u64 op_id)
{
	u32 index;

	KUNIT_STATIC_STUB_REDIRECT(ctlv_op_find, ctlvdev, op_id);

	/* The index ascends by op ID, so a miss stops early. */
	for (index = 0; index < ctlvdev->n_ops; index++) {
		if (ctlvdev->ops[index].schema->op_id == op_id)
			return &ctlvdev->ops[index];
		if (ctlvdev->ops[index].schema->op_id > op_id)
			break;
	}
	return NULL;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_op_find);

static bool ctlv_op_disabled(struct ctlv_device *ctlvdev,
			     const struct ctlv_op_entry *op)
{
	if (op->state_index == CTLV_OP_STATE_NONE)
		return false;
	/* Acquire, so what the family prepared before enabling is visible. */
	return test_bit_acquire(op->state_index, ctlvdev->disabled);
}

/**
 * ctlv_ops_publish - Freeze what one device answers.
 * @ctlvdev: device being initialized before publication
 *
 * Drops the optional ops the family did not opt in to, once, and keeps the
 * ascending order and the availability numbering of what is left.
 */
void ctlv_ops_publish(struct ctlv_device *ctlvdev)
{
	u32 published = 0;
	u32 index;

	/* Against a family still opting in as this is called. */
	guard(spinlock)(&ctlvdev->transition_lock);
	for (index = 0; index < ctlvdev->n_ops; index++) {
		if (ctlvdev->ops[index].absent)
			continue;
		ctlvdev->ops[published++] = ctlvdev->ops[index];
	}
	ctlvdev->n_ops = published;
	ctlvdev->published = true;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_ops_publish);

/**
 * ctlv_device_op_provide - Give one device an op its specification made optional.
 * @ctlvdev: device to widen, which nothing may have observed yet
 * @op_id: complete ID of the op, which names the family that declared it
 *
 * Called only by the generated per-op helpers, which exist for optional ops
 * alone, so a family cannot name an op that was never its to withhold. An op
 * left alone is absent rather than disabled: publication leaves it out of the
 * index and it stays out for the life of the device.
 *
 * Idempotent, and cannot fail. An op this device does not have, and a device
 * already published, both warn and change nothing.
 */
void ctlv_device_op_provide(struct ctlv_device *ctlvdev, u64 op_id)
{
	struct ctlv_op_entry *op;

	guard(spinlock)(&ctlvdev->transition_lock);
	if (WARN_ON_ONCE(ctlvdev->published || ctlvdev->destroying))
		return;
	op = ctlv_op_find(ctlvdev, op_id);
	/* An event is produced rather than answered, so there is none. */
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(!op->binding))
		return;
	op->absent = false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_provide, "CTLV");

/**
 * ctlv_device_op_state_set - Enable or disable one dynamically available op.
 * @ctlvdev: device to change
 * @op_id: complete ID of the op, which names the family that declared it
 * @disabled: the state to put the op into
 *
 * Called only by the generated per-op helpers, which is what keeps a family
 * from naming an op its specification did not mark dynamically available.
 *
 * Idempotent, and cannot fail: an op this device does not have, and one it
 * took away, both warn and change nothing.
 *
 * Valid during family init and from inside an op callback. A change that races
 * destruction is dropped.
 */
void ctlv_device_op_state_set(struct ctlv_device *ctlvdev, u64 op_id,
			      bool disabled)
{
	const struct ctlv_op_entry *op;

	/*
	 * Also serializes lookup against publication, which compacts the
	 * array, and against teardown stopping state changes.
	 */
	guard(spinlock)(&ctlvdev->transition_lock);
	if (ctlvdev->destroying)
		return;
	op = ctlv_op_find(ctlvdev, op_id);
	if (WARN_ON_ONCE(!op) ||
	    WARN_ON_ONCE(op->state_index == CTLV_OP_STATE_NONE))
		return;
	/* An op this device never took on has no state to change. */
	if (WARN_ON_ONCE(op->absent))
		return;
	/*
	 * Fully ordered, releasing what the family prepared to the acquire in
	 * ctlv_op_disabled(). The old state makes a repeat change nothing.
	 */
	if (disabled ? test_and_set_bit(op->state_index, ctlvdev->disabled)
		     : !test_and_clear_bit(op->state_index, ctlvdev->disabled))
		return;
	if (!ctlvdev->published)
		return;
	ctlvdev->state_generation++;
	/* Still holding the serializer, so records reach queues in order. */
	ctlv_events_op_changed(ctlvdev, op, disabled, ctlvdev->state_generation);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_state_set, "CTLV");

static int ctlv_op_check_mode(const struct file *filp,
			      const struct ctlv_op_entry *op)
{
	if (op->schema->type == CTLV_OP_TYPE_EVENT)
		return -EOPNOTSUPP;

	switch (op->schema->fd_access) {
	case CTLV_OP_FD_ACCESS_READ:
		return filp->f_mode & FMODE_READ ? 0 : -EACCES;
	case CTLV_OP_FD_ACCESS_READ_WRITE:
		return (filp->f_mode & (FMODE_READ | FMODE_WRITE)) ==
		       (FMODE_READ | FMODE_WRITE) ? 0 : -EACCES;
	default:
		/* Registration rejects this before the device is visible. */
		return -EACCES;
	}
}

static u32 ctlv_reply_needed(const struct ctlv_op_entry *op)
{
	if (op->schema->type == CTLV_OP_TYPE_QUERY)
		return op->sizes.query_reply_min_len;
	return op->sizes.action_reply_max_len;
}

/*
 * The root's own length is the message; what the caller declared readable is
 * only the copy boundary. A root refused here has no offset yet, so the
 * failure is the message as a whole. It carries no flag but NESTED: what
 * frames a request is not something its sender may ask to have ignored.
 * Schema limits bound known attrs; ignored unknown attrs may use the remaining
 * space up to the transport limit.
 */
static int ctlv_request_frame(struct ctlv_op_ctx *ctx,
			      const struct ctlv_op_exec *exec,
			      const struct ctlv_attr *root)
{
	if (root->len < sizeof(*root) || root->len > exec->request_len ||
	    root->attr_id != CTLV_ATTR_ID_ROOT ||
	    root->flags != CTLV_ATTR_F_NESTED) {
		ctx->error.reason = CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		return -EINVAL;
	}
	return 0;
}

static int ctlv_request_validate(struct ctlv_op_ctx *ctx,
				 const struct ctlv_op_exec *exec,
				 struct ctlv_attr *root)
{
	int ret;

	ret = ctlv_request_frame(ctx, exec, root);
	if (ret)
		return ret;
	if (!ctx->op->schema->request.nodes)
		return 0;

	ctx->request = (struct ctlv_msg){
		.root = root,
		.len = exec->request_len,
		.tree = &ctx->op->schema->request,
		.family = ctx->family,
		.request = true,
	};
	return ctlv_schema_validate(&ctx->request, &ctx->error);
}

/*
 * An action is given exactly the maximum its schema allows, so running out of
 * room is this kernel's mistake. A query is given whatever the caller offered.
 */
static u32 ctlv_reply_capacity(const struct ctlv_op_entry *op,
			       const struct ctlv_op_exec *exec)
{
	if (!op->schema->reply.nodes)
		return 0;
	if (op->schema->type == CTLV_OP_TYPE_QUERY)
		return exec->reply_len;
	return op->sizes.action_reply_max_len;
}

/*
 * A bounded reply has to be complete, closed and a message of its schema, all
 * of it this kernel's own doing. A fragment cannot be, an attr the schema
 * requires may be in an earlier one, but a query answered without a boundary
 * is complete and is checked like any other.
 */
static int ctlv_reply_finish(struct ctlv_op_ctx *ctx)
{
	struct ctlv_error refused = {};

	if (!ctx->output.root)
		return 0;
	/* Whatever poisoned it has warned about it already. */
	if (ctx->output.poison)
		return ctx->output.poison;
	/* A query was held to its open nests by the ending it came to. */
	if (ctx->op->schema->type == CTLV_OP_TYPE_QUERY) {
		if (ctx->frag.cursor || ctx->frag.next_cursor)
			return 0;
	} else if (WARN_ON_ONCE(ctx->deepest != &ctx->output_root)) {
		/* A cursor of a returned frame is reset, never followed. */
		ctx->deepest = &ctx->output_root;
		return -EINVAL;
	}
	if (WARN_ON_ONCE(ctlv_schema_validate(&ctx->output, &refused)))
		return -EINVAL;
	return 0;
}

/*
 * Only the callback can say where it stopped, so returning success without
 * ending the fragment is not an answer, and ignoring what the ending returned
 * does not deliver it.
 */
static int ctlv_frag_result(struct ctlv_op_ctx *ctx)
{
	if (WARN_ON_ONCE(!ctx->frag.answered))
		return -EINVAL;
	return ctx->frag.result;
}

static int ctlv_op_call(struct ctlv_op_ctx *ctx)
{
	const struct ctlv_op_entry *op = ctx->op;
	struct ctlv_cursor *request = NULL;
	struct ctlv_cursor *reply = NULL;
	int blob_ret;
	int ret;

	if (ctx->request.root) {
		ctlv_msg_root(&ctx->request_root, &ctx->request);
		request = &ctx->request_root;
	}
	if (ctx->output.root)
		reply = &ctx->output_root;

	ctlv_blobs_init(ctx);
	if (op->schema->type == CTLV_OP_TYPE_ACTION) {
		ret = op->binding->action(op->priv, ctx, request, reply);
	} else {
		ret = op->binding->query(op->priv, ctx, request, reply);
		if (!ret)
			ret = ctlv_frag_result(ctx);
	}
	blob_ret = ctlv_blobs_finish(ctx);
	if (!ret)
		ret = blob_ret;
	if (ret)
		return ret;
	return ctlv_reply_finish(ctx);
}

/*
 * The device family's hooks around the declaring op's, except that a core op
 * uses the core family. The availability check is innermost.
 */
static int ctlv_op_dispatch(struct ctlv_op_ctx *ctx)
{
	const struct ctlv_op *op = ctx->op->binding;
	const struct ctlv_family *family;
	void *family_priv;
	int ret;

	if (CTLV_OP_ID_FAMILY(ctx->op->schema->op_id) == CTLV_FAMILY_ID) {
		family = &ctlv_ctlv_family;
		family_priv = ctx->ctlvdev->privs[0].priv;
	} else {
		family = ctx->ctlvdev->family;
		family_priv =
			ctx->ctlvdev->privs[ctx->ctlvdev->n_privs - 1].priv;
	}
	ret = family->pre(family_priv, ctx);
	if (ret)
		return ret;

	if (op->pre) {
		ret = op->pre(ctx->op->priv, ctx);
		if (ret)
			goto out;
	}

	/* The only authoritative check: introspection is advisory. */
	if (ctlv_op_disabled(ctx->ctlvdev, ctx->op))
		ret = -EOPNOTSUPP;
	else
		ret = ctlv_op_call(ctx);

	if (op->post)
		op->post(ctx->op->priv, ctx);
out:
	family->post(family_priv, ctx);
	return ret;
}

/*
 * Error information goes where a reply would have gone, whole or not at all:
 * too little room for it and the caller gets the errno alone. Only a failure
 * this core recorded is described.
 */
static void ctlv_error_info(struct ctlv_op_ctx *ctx, u32 room, void **reply,
			    u32 *reply_len)
{
	const struct ctlv_error *error = &ctx->error;
	struct ctlv_cursor *root = &ctx->output_root;
	void *buf;

	if ((!error->reason && !error->message[0]) || !room)
		return;
	buf = kvmalloc(CTLV_ERROR_INFO_MAX_LEN, GFP_KERNEL_ACCOUNT);
	if (!buf)
		return;

	/* The same builder over another schema, and a buffer of its own. */
	ctlv_msg_output(&ctx->output, ctx, buf, CTLV_ERROR_INFO_MAX_LEN,
			&ctlv_error_info_schema);
	ctlv_msg_root(root, &ctx->output);
	ctx->deepest = root;

	if (error->reason)
		ctlv_put_u64(root, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM,
			     error->reason);
	if (error->message[0])
		ctlv_error_message_put(root, error->message);
	if (error->reason == CTLV_VALIDATION_REASON_STALE_GENERATION) {
		/* The one failure that is about no part of any message. */
		ctlv_put_u64(root, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
			     CTLV_SCHEMA_UINT, error->expected_generation);
		ctlv_put_u64(root, CTLV_ATTR_ERROR_CURRENT_GENERATION,
			     CTLV_SCHEMA_UINT, error->current_generation);
	} else if (error->reason) {
		ctlv_put_u64(root, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET,
			     CTLV_SCHEMA_UINT, error->bad_attr_offset);
		ctlv_put_u64(root, CTLV_ATTR_ERROR_NEST_OFFSET,
			     CTLV_SCHEMA_UINT, error->nest_offset);
		/* No attr is numbered zero, so a zero names nothing. */
		if (error->missing_attr_id)
			ctlv_put_u64(root, CTLV_ATTR_ERROR_MISSING_ATTR_ID,
				     CTLV_SCHEMA_UINT, error->missing_attr_id);
	}

	if (ctx->output.poison || ctx->output.root->len > room) {
		kvfree(buf);
		return;
	}
	*reply = buf;
	*reply_len = ctx->output.root->len;
}

/*
 * Everything that needs the device to still have a family. @reply is built
 * here and becomes the caller's to free, to copy out once the family is gone.
 */
static int ctlv_op_exec(struct ctlv_file *cfile, struct file *filp,
			struct ctlv_op_exec *exec, void **reply,
			u32 *reply_len)
{
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	const struct ctlv_op_entry *op;
	struct ctlv_op_ctx ctx = {};
	u32 capacity;
	int ret;

	/* No family numbers an op zero, so a zero here is an unset field. */
	if (!exec->op_id)
		return -EINVAL;
	op = ctlv_op_find(ctlvdev, exec->op_id);
	if (!op)
		return -EOPNOTSUPP;
	ret = ctlv_op_check_mode(filp, op);
	if (ret)
		return ret;
	/*
	 * Only a query resumes. A cursor says which fragment this is, so a
	 * generation without one continues what was never started.
	 */
	if (op->schema->type == CTLV_OP_TYPE_QUERY) {
		if (!exec->cursor && exec->generation)
			return -EINVAL;
		ctx.frag.generation = exec->generation;
		ctx.frag.cursor = exec->cursor;
	} else if (exec->generation || exec->cursor) {
		return -EINVAL;
	}

	/* Before the callback, so a buffer too small mutates nothing. */
	if (exec->reply_len < ctlv_reply_needed(op))
		return -EMSGSIZE;
	/* And no longer than a message of this framework may be. */
	if (exec->reply_len > CTLV_MAX_INLINE_MESSAGE_LEN)
		return -EINVAL;

	ctx.ctlvdev = ctlvdev;
	ctx.family = ctlvdev->family;
	ctx.file = cfile;
	/* Whose memory a blob descriptor of this request describes. */
	ctx.submitter = current;
	ctx.op = op;

	void *snapshot __free(kvfree) = NULL;
	void *out __free(kvfree) = NULL;

	if (exec->request_len < sizeof(struct ctlv_attr) ||
	    exec->request_len > CTLV_MAX_INLINE_MESSAGE_LEN)
		return -EINVAL;
	/* Everything after this reads the snapshot, never userspace. */
	snapshot = kvmalloc(exec->request_len, GFP_KERNEL_ACCOUNT);
	if (!snapshot)
		return -ENOMEM;
	if (copy_from_user(snapshot, u64_to_user_ptr(exec->request),
			   exec->request_len))
		return -EFAULT;
	/* Before any hook, so that no family code sees an unchecked attr. */
	ret = ctlv_request_validate(&ctx, exec, snapshot);
	if (ret)
		goto out_error_info;

	capacity = ctlv_reply_capacity(op, exec);
	if (capacity) {
		/*
		 * Not zeroed: a byte is copied out only once the root's
		 * length covers it, padding included.
		 */
		out = kvmalloc(capacity, GFP_KERNEL_ACCOUNT);
		if (!out) {
			ret = -ENOMEM;
			goto out_request;
		}
		ctlv_msg_output(&ctx.output, &ctx, out, capacity,
				&op->schema->reply);
		ctlv_msg_root(&ctx.output_root, &ctx.output);
		ctx.deepest = &ctx.output_root;
	}

	ret = ctlv_op_dispatch(&ctx);
	if (ret)
		goto out_error_info;

	/* Where the caller carries on from, zero for anything but a query. */
	exec->generation = ctx.frag.reported;
	exec->cursor = ctx.frag.next_cursor;
	*reply_len = ctx.output.root ? ctx.output.root->len : 0;
	*reply = no_free_ptr(out);
	ret = 0;
	goto out_request;

out_error_info:
	ctlv_error_info(&ctx, exec->reply_len, reply, reply_len);
out_request:
	ctlv_msg_index_free(&ctx.request);
	return ret;
}

/* What one family of this device's chain does with one extension point. */
static const struct ctlv_stub_ops *
ctlv_stub_answers(const struct ctlv_device *ctlvdev,
		  const struct ctlv_stub_ref *ref, void **priv)
{
	const struct ctlv_family *family;
	u32 index;

	/* From the device family towards the core, where the fill was found. */
	for (family = ctlvdev->family; family; family = family->parent)
		for (index = 0; index < family->n_stubs; index++)
			if (family->stubs[index].family_id == ref->family_id &&
			    family->stubs[index].set_id == ref->set_id) {
				struct ctlv_family_priv *family_priv;

				family_priv =
					ctlv_family_priv_find(ctlvdev, family);
				if (WARN_ON_ONCE(!family_priv))
					return NULL;
				*priv = family_priv->priv;
				return &family->stubs[index];
			}
	return NULL;
}

/**
 * ctlv_stub_fill - Let whoever filled one extension point write its members.
 * @ctx: context of the op being executed, or of the event being built
 * @request: the same nest of the request being answered, or NULL
 * @nest: cursor of the open nest the extension point stands for
 *
 * Called by the implementation that declared the extension point, once per nest
 * it opened for one, so that a message of an inherited op carries what a family
 * inheriting it put there.
 *
 * Return: 0, or what a fill refused with.
 */
int ctlv_stub_fill(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		   struct ctlv_cursor *nest)
{
	const struct ctlv_schema_tree *tree;
	u32 index;

	if (WARN_ON_ONCE(!nest || !nest->tree))
		return -EINVAL;
	tree = nest->tree;
	for (index = 0; index < tree->n_stubs; index++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[index];
		const struct ctlv_cursor *filled_request = request;
		const struct ctlv_stub_fill *fill;
		const struct ctlv_stub_ops *stub;
		struct ctlv_cursor filled;
		void *priv;
		int ret;

		if (ref->node != nest->node)
			continue;
		stub = ctlv_stub_answers(ctx->ctlvdev, ref, &priv);
		if (!stub || !stub->fill)
			continue;
		if (request) {
			fill = ctlv_schema_fill(ctx->family, ref->family_id,
						ref->set_id);
			if (WARN_ON_ONCE(!fill))
				return -EINVAL;
			filled = *request;
			if (ctlv_input_rebind(&filled, &fill->members, 0))
				filled_request = &filled;
			else
				filled_request = NULL;
		}
		ret = stub->fill(priv, ctx, filled_request, nest);
		if (ret)
			return ret;
	}
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_stub_fill, "CTLV");

/**
 * ctlv_stub_read - Let whoever filled one extension point read its members.
 * @ctx: context of the op being executed
 * @nest: cursor of the nest the extension point stands for, which may address
 *	nothing: an extension point the caller did not send is nothing to read
 *
 * The other direction of ctlv_stub_fill(), for an implementation that hands the
 * members it cannot know to the family that declared them.
 */
void ctlv_stub_read(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *nest)
{
	const struct ctlv_schema_tree *tree;
	u32 index;

	if (WARN_ON_ONCE(!nest || !nest->tree))
		return;
	if (!nest->attr)
		return;
	tree = nest->tree;
	for (index = 0; index < tree->n_stubs; index++) {
		const struct ctlv_stub_ref *ref = &tree->stubs[index];
		const struct ctlv_stub_fill *fill;
		const struct ctlv_stub_ops *stub;
		struct ctlv_cursor filled;
		void *priv;

		if (ref->node != nest->node)
			continue;
		stub = ctlv_stub_answers(ctx->ctlvdev, ref, &priv);
		if (!stub || !stub->read)
			continue;
		fill = ctlv_schema_fill(ctx->family, ref->family_id,
					ref->set_id);
		if (WARN_ON_ONCE(!fill))
			continue;
		filled = *nest;
		if (!ctlv_input_rebind(&filled, &fill->members, 0))
			continue;
		stub->read(priv, ctx, &filled);
	}
}
EXPORT_SYMBOL_NS_GPL(ctlv_stub_read, "CTLV");

static bool ctlv_op_error_args(const struct ctlv_op_ctx *ctx, int error,
			       const char *message)
{
	return !WARN_ON_ONCE(error >= 0 || error < -MAX_ERRNO) &&
	       !WARN_ON_ONCE(!ctx || !message);
}

/**
 * ctlv_op_error - Return an errno with a family diagnostic.
 * @ctx: context of the operation being executed
 * @error: negative errno to return
 * @message: diagnostic to copy
 *
 * The first message recorded during an execution wins. Longer messages are
 * truncated to fit error.message.
 *
 * Return: @error, or -EINVAL for an invalid errno
 */
int ctlv_op_error(struct ctlv_op_ctx *ctx, int error, const char *message)
{
	if (!ctlv_op_error_args(ctx, error, message))
		return error < 0 && error >= -MAX_ERRNO ? error : -EINVAL;
	if (!ctx->error.message[0])
		strscpy(ctx->error.message, message,
			sizeof(ctx->error.message));
	return error;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_error, "CTLV");

/**
 * ctlv_op_errorf - Return an errno with a formatted family diagnostic.
 * @ctx: context of the operation being executed
 * @error: negative errno to return
 * @fmt: printf-style diagnostic
 *
 * The first message recorded during an execution wins.
 *
 * Return: @error, or -EINVAL for an invalid errno
 */
int ctlv_op_errorf(struct ctlv_op_ctx *ctx, int error, const char *fmt, ...)
{
	va_list args;

	if (!ctlv_op_error_args(ctx, error, fmt))
		return error < 0 && error >= -MAX_ERRNO ? error : -EINVAL;
	if (ctx->error.message[0])
		return error;
	va_start(args, fmt);
	vscnprintf(ctx->error.message, sizeof(ctx->error.message), fmt, args);
	va_end(args);
	return error;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_errorf, "CTLV");

/* Continuation is a query's, so asking about one anywhere else is a bug. */
static bool ctlv_op_is_query(const struct ctlv_op_ctx *ctx)
{
	return !WARN_ON_ONCE(ctx->op->schema->type != CTLV_OP_TYPE_QUERY);
}

/**
 * ctlv_op_query_generation - Generation the caller is continuing from.
 * @ctx: context of the query being executed
 *
 * Return: what the previous fragment reported, or zero for the first
 * invocation of a logical reply.
 */
u64 ctlv_op_query_generation(const struct ctlv_op_ctx *ctx)
{
	if (!ctlv_op_is_query(ctx))
		return 0;
	return ctx->frag.generation;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_generation, "CTLV");

/**
 * ctlv_op_query_cursor - Where the caller is continuing from.
 * @ctx: context of the query being executed
 *
 * Return: the cursor the previous fragment ended with, or zero to start the
 * logical reply from its beginning.
 */
u64 ctlv_op_query_cursor(const struct ctlv_op_ctx *ctx)
{
	if (!ctlv_op_is_query(ctx))
		return 0;
	return ctx->frag.cursor;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_cursor, "CTLV");

/*
 * What both endings check: a query, one ending, and a generation that has not
 * moved under a continuation, which no fragment can be delivered across.
 */
static int ctlv_frag_end(struct ctlv_op_ctx *ctx, u64 generation)
{
	if (!ctlv_op_is_query(ctx))
		return -EINVAL;
	if (WARN_ON_ONCE(ctx->frag.answered))
		return -EINVAL;
	ctx->frag.answered = true;
	/* Whatever the builder gave up on has warned about it already. */
	if (ctx->output.poison)
		return ctx->output.poison;
	if (ctx->frag.cursor && generation != ctx->frag.generation) {
		ctx->error.reason = CTLV_VALIDATION_REASON_STALE_GENERATION;
		ctx->error.expected_generation = ctx->frag.generation;
		ctx->error.current_generation = generation;
		return -ESTALE;
	}
	ctx->frag.reported = generation;
	return 0;
}

/**
 * ctlv_op_query_more - End a fragment at the boundary the callback reached.
 * @ctx: context of the query being executed
 * @generation: generation this fragment was written at
 * @next_cursor: where a continuation of it resumes, which must not be zero
 *
 * Marks the root and every nest the callback left open, so every open nest's
 * cursor has to be alive here. The fragment must have completed a logical
 * leaf. A @next_cursor that is zero, or the one this invocation started from,
 * is a reply that never ends and is refused.
 *
 * Return: zero, or the negative errno that is the result of the op.
 */
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor)
{
	int ret = ctlv_frag_end(ctx, generation);

	if (!ret && WARN_ON_ONCE(!ctx->frag.progress || !next_cursor ||
				 next_cursor == ctx->frag.cursor))
		ret = -EINVAL;
	if (!ret) {
		ctlv_out_frag_mark(ctx);
		ctx->frag.next_cursor = next_cursor;
	}
	ctx->frag.result = ret;
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_more, "CTLV");

/**
 * ctlv_op_query_done - End a fragment that completes the logical reply.
 * @ctx: context of the query being executed
 * @generation: generation this fragment was written at
 *
 * The reply carries no boundary mark, so every nest of it has to be closed.
 *
 * Return: zero, or the negative errno that is the result of the op.
 */
int ctlv_op_query_done(struct ctlv_op_ctx *ctx, u64 generation)
{
	int ret = ctlv_frag_end(ctx, generation);

	if (!ret && WARN_ON_ONCE(ctx->deepest != &ctx->output_root)) {
		/* A cursor of a frame that has returned is not followed. */
		ctx->deepest = &ctx->output_root;
		ret = -EINVAL;
	}
	ctx->frag.result = ret;
	return ret;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_query_done, "CTLV");

/*
 * The core answers the root family through the same generated descriptor a
 * driver creates with, so it needs the same callbacks.
 */
int ctlv_op_pre(void *priv, struct ctlv_op_ctx *ctx)
{
	return 0;
}

void ctlv_op_post(void *priv, struct ctlv_op_ctx *ctx)
{
}

size_t ctlv_priv_size(void)
{
	return 0;
}

int ctlv_init(struct ctlv_device *ctlvdev, const void *init_info, void *priv)
{
	if (WARN_ON_ONCE(init_info) ||
	    WARN_ON_ONCE(ctlv_priv_parent(priv)))
		return -EINVAL;
	return 0;
}

void ctlv_fini(struct ctlv_device *ctlvdev, void *priv)
{
}

static long ctlv_fops_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	struct ctlv_op_exec __user *uexec = (struct ctlv_op_exec __user *)arg;
	struct ctlv_file *cfile = filp->private_data;
	struct ctlv_device *ctlvdev = cfile->ctlvdev;
	struct ctlv_op_exec exec;
	u32 reply_len = 0;
	int ret;

	if (cmd != CTLV_IOC_OP_EXEC)
		return -ENOIOCTLCMD;
	if (copy_from_user(&exec, uexec, sizeof(exec)))
		return -EFAULT;
	/* Reserved fields are what a compatible growth of this ioctl uses. */
	if (exec.reserved[0] || exec.reserved[1])
		return -EINVAL;

	void *reply __free(kvfree) = NULL;

	scoped_guard(rwsem_read, &ctlvdev->registration_lock) {
		if (!ctlvdev->registered || !ctlvdev->family)
			return -ENODEV;
		ret = ctlv_op_exec(cfile, filp, &exec, &reply, &reply_len);
	}
	/*
	 * Outside the registration lock, so a copy that faults is not one more
	 * thing destruction waits for. An action that returned
	 * successfully has committed and is not rolled back for it.
	 */
	if (reply_len &&
	    copy_to_user(u64_to_user_ptr(exec.reply), reply, reply_len))
		return ret ? : -EFAULT;
	/*
	 * The only fields the kernel writes back, a field at a time so that
	 * what the caller stated about its own buffers stays the caller's.
	 */
	if (!ret && (put_user(exec.generation, &uexec->generation) ||
		     put_user(exec.cursor, &uexec->cursor)))
		return -EFAULT;
	return ret;
}

/*
 * One record per read, so a reader never gets a partial event. The offset says
 * nothing about a queue and is neither read nor advanced.
 */
static ssize_t ctlv_fops_read(struct file *filp, char __user *buf, size_t len,
			      loff_t *ppos)
{
	struct ctlv_file *cfile = filp->private_data;

	return ctlv_events_read(cfile, buf, len,
				!(filp->f_flags & O_NONBLOCK));
}

static __poll_t ctlv_fops_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct ctlv_file *cfile = filp->private_data;

	return ctlv_events_poll(cfile, filp, wait);
}

static bool ctlv_fops_open_allowed(struct ctlv_device *ctlvdev)
{
	lockdep_assert_held_read(&ctlvdev->registration_lock);
	return ctlvdev->registered && ctlvdev->family;
}

VISIBLE_IF_KUNIT
void ctlv_fops_open_pinned(struct ctlv_device *ctlvdev)
{
	KUNIT_STATIC_STUB_REDIRECT(ctlv_fops_open_pinned, ctlvdev);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_fops_open_pinned);

VISIBLE_IF_KUNIT
int ctlv_fops_open(struct inode *inode, struct file *filp)
{
	struct ctlv_device *ctlvdev =
		container_of(inode->i_cdev, struct ctlv_device, cdev);
	struct ctlv_file *cfile;
	int ret;

	/*
	 * A failed create can withdraw the cdev while an open is waiting for
	 * the lifecycle lock, so admission itself first pins core storage.
	 */
	get_device(&ctlvdev->dev);
	ctlv_fops_open_pinned(ctlvdev);
	scoped_guard(rwsem_read, &ctlvdev->registration_lock) {
		if (!ctlv_fops_open_allowed(ctlvdev)) {
			ret = -ENODEV;
			break;
		}
		if (!(filp->f_mode & FMODE_READ)) {
			ret = -EACCES;
			break;
		}

		cfile = kzalloc_obj(*cfile, GFP_KERNEL_ACCOUNT);
		if (!cfile) {
			ret = -ENOMEM;
			break;
		}
		cfile->ctlvdev = ctlvdev;
		/*
		 * Commits this description to the device. An open that failed
		 * after this would have to take it back.
		 */
		ret = ctlv_events_file_init(cfile);
		if (ret) {
			kfree(cfile);
			break;
		}
		/* This description keeps the reference taken on entry. */
		filp->private_data = cfile;
		return 0;
	}

	put_device(&ctlvdev->dev);
	return ret;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_fops_open);

static int ctlv_fops_release(struct inode *inode, struct file *filp)
{
	struct ctlv_file *cfile = filp->private_data;
	struct ctlv_device *ctlvdev = cfile->ctlvdev;

	ctlv_events_file_exit(cfile);
	kfree(cfile);
	put_device(&ctlvdev->dev);
	return 0;
}

static const struct file_operations ctlv_fops = {
	.owner = THIS_MODULE,
	.open = ctlv_fops_open,
	.release = ctlv_fops_release,
	.read = ctlv_fops_read,
	.poll = ctlv_fops_poll,
	.unlocked_ioctl = ctlv_fops_ioctl,
	/* Every ioctl field is fixed width, so one entry point serves both. */
	.compat_ioctl = compat_ptr_ioctl,
};

/*
 * Identity is in the uevent, not in sysfs: a rule deciding what to do with a
 * new node cannot read an attribute of it. The chain is numbered from its
 * oldest family, so a rule survives a descendant being added below it.
 */
static int ctlv_uevent_families(struct kobj_uevent_env *env,
				const struct ctlv_family *family)
{
	u32 levels = ctlv_family_levels(family);
	u32 level;
	int ret;

	ret = add_uevent_var(env, "CTLV_FAMILY=%s", family->schema->name);
	if (ret)
		return ret;
	for (level = 0; level < levels; level++) {
		const struct ctlv_family *ancestor =
			ctlv_family_ancestor(family, levels - 1 - level);

		ret = add_uevent_var(env, "CTLV_COMPATIBLE_%u=%s", level,
				    ancestor->schema->name);
		if (ret)
			return ret;
	}
	return add_uevent_var(env, "CTLV_COMPATIBLE_N=%u", levels);
}

/**
 * ctlv_uevent_check - Whether one chain's names fit the uevents it needs.
 * @family: family a device would answer as
 *
 * A chain that does not fit is refused rather than announced short, since a
 * rule matching a truncated chain matches the wrong device.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_uevent_check(const struct ctlv_family *family)
{
	struct kobj_uevent_env *env;
	int ret;

	env = kzalloc_obj(*env, GFP_KERNEL);
	if (!env)
		return -ENOMEM;
	ret = ctlv_uevent_families(env, family);
	if (!ret && (env->buflen > CTLV_UEVENT_MAX_LEN ||
		     env->envp_idx > CTLV_UEVENT_MAX_VARS)) {
		pr_err("family %s: its chain of names needs %d bytes in %d properties of a uevent, more than the %d in %d it may take\n",
		       family->schema->name, env->buflen, env->envp_idx,
		       CTLV_UEVENT_MAX_LEN, CTLV_UEVENT_MAX_VARS);
		ret = -E2BIG;
	}
	kfree(env);
	return ret;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_uevent_check);

/* Read unlocked: a uevent may be asked for under the registration lock. */
static int ctlv_dev_uevent(const struct device *dev,
			   struct kobj_uevent_env *env)
{
	const struct ctlv_device *ctlvdev =
		container_of_const(dev, struct ctlv_device, dev);
	const struct ctlv_family *family = READ_ONCE(ctlvdev->family);

	if (!family)
		return 0;
	return ctlv_uevent_families(env, family);
}

static void ctlv_device_release(struct device *dev)
{
	struct ctlv_device *ctlvdev =
		container_of(dev, struct ctlv_device, dev);

	ida_free(&ctlv_ida, ctlvdev->dev.devt - ctlv_devt);
	bitmap_free(ctlvdev->disabled);
	ctlv_privs_free(ctlvdev);
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
}

static struct class ctlv_class = {
	.name = "ctlv",
	.dev_release = ctlv_device_release,
	.dev_uevent = ctlv_dev_uevent,
};

VISIBLE_IF_KUNIT
int ctlv_cdev_device_add(struct ctlv_device *ctlvdev)
{
	lockdep_assert_held_write(&ctlvdev->registration_lock);
	KUNIT_STATIC_STUB_REDIRECT(ctlv_cdev_device_add, ctlvdev);

	return cdev_device_add(&ctlvdev->cdev, &ctlvdev->dev);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_cdev_device_add);

VISIBLE_IF_KUNIT
void ctlv_cdev_device_del(struct ctlv_device *ctlvdev)
{
	KUNIT_STATIC_STUB_REDIRECT(ctlv_cdev_device_del, ctlvdev);

	cdev_device_del(&ctlvdev->cdev, &ctlvdev->dev);
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_cdev_device_del);

/**
 * ctlv_create - Create and publish one CTLV device.
 * @parent_dev: device-model parent, the device this one belongs to
 * @family: generated descriptor of the family the device answers as
 * @init_info: immutable initialization information for @family
 *
 * Runs pre-init from @family to the root, initializes every core-allocated
 * family priv from the root to @family, then freezes ops and starts events
 * before post-init may expose the provider to kernel consumers. Userspace
 * publication follows only after all callbacks succeed.
 *
 * Return: the live device, or an ERR_PTR.
 */
struct ctlv_device *ctlv_create(struct device *parent_dev,
				const struct ctlv_family *family,
				const void *init_info)
{
	struct ctlv_device *ctlvdev;
	int devnum, ret;

	if (!parent_dev || !family)
		return ERR_PTR(-EINVAL);

	ctlvdev = kzalloc_obj(*ctlvdev, GFP_KERNEL);
	if (!ctlvdev)
		return ERR_PTR(-ENOMEM);
	ctlvdev->family = family;
	init_rwsem(&ctlvdev->registration_lock);
	spin_lock_init(&ctlvdev->transition_lock);
	ctlv_events_device_init(ctlvdev);

	/* Before anything reads the schema through this device. */
	ret = ctlv_ops_resolve(ctlvdev, family);
	if (ret)
		goto err_free;
	/* What this family's events need of every queue opened on it. */
	ret = ctlv_events_sizes(ctlvdev);
	if (ret)
		goto err_free;
	/* Before a minor is spent on a device that could not be announced. */
	ret = ctlv_uevent_check(family);
	if (ret)
		goto err_free;
	if (ctlvdev->n_dynamic_ops) {
		ctlvdev->disabled = bitmap_zalloc(ctlvdev->n_dynamic_ops,
						  GFP_KERNEL);
		if (!ctlvdev->disabled) {
			ret = -ENOMEM;
			goto err_free;
		}
	}
	devnum = ida_alloc_max(&ctlv_ida, CTLV_MAX_DEVICES - 1, GFP_KERNEL);
	if (devnum < 0) {
		ret = devnum;
		goto err_free;
	}

	ctlvdev->dev.devt = ctlv_devt + devnum;
	ctlvdev->dev.class = &ctlv_class;
	ctlvdev->dev.parent = parent_dev;
	device_initialize(&ctlvdev->dev);
	/*
	 * The core owns the file operations, so an open holds the core module
	 * and not the family's, which may unload while readers remain.
	 */
	cdev_init(&ctlvdev->cdev, &ctlv_fops);
	ctlvdev->cdev.owner = THIS_MODULE;

	ret = dev_set_name(&ctlvdev->dev, "ctlv%d", devnum);
	if (ret)
		goto err_put;
	ret = ctlv_claim_family(family->schema);
	if (ret)
		goto err_put;
	ret = ctlv_privs_pre_init(ctlvdev, init_info);
	if (ret)
		goto err_put;
	ret = ctlv_privs_init(ctlvdev);
	if (ret)
		goto err_put;
	ctlv_ops_publish(ctlvdev);
	ctlv_event_contexts_start(ctlvdev);
	ret = ctlv_privs_post_init(ctlvdev);
	if (ret)
		goto err_fini;
	scoped_guard(rwsem_write, &ctlvdev->registration_lock) {
		ret = ctlv_cdev_device_add(ctlvdev);
		if (!ret)
			ctlvdev->registered = true;
	}
	if (ret)
		goto err_fini;
	return ctlvdev;

err_fini:
	ctlv_device_fini(ctlvdev);
err_put:
	put_device(&ctlvdev->dev);
	return ERR_PTR(ret);
err_free:
	bitmap_free(ctlvdev->disabled);
	ctlv_privs_free(ctlvdev);
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(ctlv_create, "CTLV");

/**
 * ctlv_destroy - Destroy one live CTLV device.
 * @ctlvdev: device returned by ctlv_create()
 *
 * Removes the device, drains family callbacks, finalizes every family priv,
 * and consumes the creator's reference. Open files retain only core storage.
 */
void ctlv_destroy(struct ctlv_device *ctlvdev)
{
	u32 index;

	/* No new opens, no discovery entry, and no node to open. */
	ctlv_cdev_device_del(ctlvdev);

	/*
	 * The write side waits for admitted callbacks while their events still
	 * work. Once held, registration state keeps every later ioctl out.
	 */
	scoped_guard(rwsem_write, &ctlvdev->registration_lock)
		ctlvdev->registered = false;

	ctlv_device_fini(ctlvdev);

	/* No callback can reach what the family owns after finalization. */
	scoped_guard(rwsem_write, &ctlvdev->registration_lock)
		WRITE_ONCE(ctlvdev->family, NULL);
	for (index = 0; index < ctlvdev->n_ops; index++) {
		ctlvdev->ops[index].binding = NULL;
		ctlvdev->ops[index].priv = NULL;
	}
	ctlv_privs_free(ctlvdev);
	put_device(&ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_destroy, "CTLV");

static int __init ctlv_module_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&ctlv_devt, 0, CTLV_MAX_DEVICES, "ctlv");
	if (ret)
		return ret;

	ret = class_register(&ctlv_class);
	if (ret)
		goto err_unregister_chrdev_region;

	return 0;

err_unregister_chrdev_region:
	unregister_chrdev_region(ctlv_devt, CTLV_MAX_DEVICES);
	return ret;
}

static void __exit ctlv_module_exit(void)
{
	class_unregister(&ctlv_class);
	unregister_chrdev_region(ctlv_devt, CTLV_MAX_DEVICES);
	ctlv_claims_free();
	ida_destroy(&ctlv_ida);
}

subsys_initcall(ctlv_module_init);
module_exit(ctlv_module_exit);
MODULE_DESCRIPTION("CTLV character device TLV UAPI framework");
MODULE_LICENSE("GPL");
/* A blob attr may name a dma-buf, so the core resolves one itself. */
MODULE_IMPORT_NS("DMA_BUF");
