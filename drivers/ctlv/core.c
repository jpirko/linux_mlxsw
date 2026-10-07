// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 */
#define pr_fmt(fmt) "ctlv: " fmt
#include <linux/align.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/build_bug.h>
#include <linux/cache.h>
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
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#include <uapi/linux/ctlv.h>

#include "blob.h"
#include "builder.h"
#include "device.h"
#include "event.h"
#include "internal.h"
#include "schema.h"

enum {
	CTLV_MAX_DEVICES = 4096,
};

/*
 * One layout on every ABI, including compat. Alignment is asserted too because
 * a 32-bit ABI aligns a plain __u64 to four bytes.
 */
static_assert(sizeof(struct ctlv_attr) == 16);
static_assert(__alignof__(struct ctlv_attr) == CTLV_ALIGNTO);

static_assert(sizeof(struct ctlv_op_exec) == 64);
static_assert(__alignof__(struct ctlv_op_exec) == CTLV_ALIGNTO);

static_assert(sizeof(struct ctlv_event_hdr) == 24);
static_assert(__alignof__(struct ctlv_event_hdr) == CTLV_ALIGNTO);

static dev_t ctlv_device_number;
static DEFINE_IDA(ctlv_device_ids);

/* Every family priv has core-owned links immediately before it. */
#define CTLV_PRIV_ALIGN SMP_CACHE_BYTES

struct ctlv_priv_container {
	struct ctlv_priv_container *parent;
	struct ctlv_priv_container *child;
	u8 priv[] __aligned(CTLV_PRIV_ALIGN);
};

static inline const struct ctlv_priv_container *ctlv_priv_container(const void *priv)
{
	return container_of_const(priv, struct ctlv_priv_container, priv);
}

struct ctlv_family_priv {
	const struct ctlv_family *family;
	void *priv;
	size_t priv_offset;
	const void *init_info;
	bool initialized;
};

/* Claims persist across unload so one ID cannot later name another family. */
struct ctlv_family_claim {
	struct list_head entry;
	char *name;
	u16 family_id;
};

static LIST_HEAD(ctlv_family_claims);
static DEFINE_MUTEX(ctlv_family_claims_lock);

static int ctlv_family_claim_add(const struct ctlv_family_schema *schema)
{
	struct ctlv_family_claim *claim;

	guard(mutex)
		(&ctlv_family_claims_lock);
	list_for_each_entry(claim, &ctlv_family_claims, entry) {
		bool same_id = claim->family_id == schema->family_id;
		bool same_name = !strcmp(claim->name, schema->name);

		if (same_id && same_name)
			return 0;
		if (same_id || same_name) {
			pr_err("family %s wants ID %u, which belongs to %s\n", schema->name,
			       schema->family_id, claim->name);
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
	list_add_tail(&claim->entry, &ctlv_family_claims);
	return 0;
}

static void ctlv_family_claims_free(void)
{
	struct ctlv_family_claim *claim, *next;

	list_for_each_entry_safe(claim, next, &ctlv_family_claims, entry) {
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

static int ctlv_op_bad(const struct ctlv_family *family, u64 op_id, const char *what)
{
	pr_err("family %s: op %#018llx %s\n", ctlv_family_name(family), op_id, what);
	return -EINVAL;
}

/*
 * A family is named its parent's whole name plus a suffix, so one name states
 * the chain it belongs to and every family of this core is ctlv-something.
 */
static int ctlv_family_chain_check(const struct ctlv_family *family)
{
	const struct ctlv_family *child;

	for (child = family; child != &ctlv_family; child = child->parent) {
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
		if (!len || strncmp(child->schema->name, parent->schema->name, len) ||
		    child->schema->name[len] != '-')
			return ctlv_family_bad(family, "is not named after the family it inherits");
	}
	return 0;
}

/* One descriptor of a chain: what it answers with, and what it fills. */
static int ctlv_family_descriptor_check(const struct ctlv_family *family)
{
	const struct ctlv_family_schema *schema = family->schema;
	u32 index;

	/*
	 * A family that declares no op of its own binds nothing: what it
	 * inherited is answered where it was declared.
	 */
	if (!schema || !schema->name || !*schema->name || (family->ops_count && !family->ops))
		return ctlv_family_bad(family, "descriptor is incomplete");
	/* Nothing in common means an empty pair, not an absent one. */
	if (!family->pre || !family->post)
		return ctlv_family_bad(family, "has no hook pair");
	if (!family->priv_size || !family->init || !family->fini)
		return ctlv_family_bad(family, "has no priv lifecycle");
	/* Every device answers the root family already, as the core. */
	if (schema->family_id == CTLV_FAMILY_ROOT && family != &ctlv_family)
		return ctlv_family_bad(family, "claims the root family ID");
	/* An extension point is answered by whichever family filled it. */
	if (family->stub_ops_count != schema->stub_fill_schemas_count)
		return ctlv_family_bad(family, "answers other extension points than it fills");
	for (index = 0; index < family->stub_ops_count; index++) {
		const struct ctlv_stub_fill_schema *fill_schema = &schema->stub_fill_schemas[index];
		const struct ctlv_stub_ops *stub_ops = &family->stub_ops[index];

		if (stub_ops->family_id == fill_schema->family_id &&
		    stub_ops->set_id == fill_schema->set_id)
			continue;
		return ctlv_family_bad(family, "answers an extension point it does not fill");
	}
	return 0;
}

/* Every descriptor binds its local schema and supplies a complete lifecycle. */
static int ctlv_family_descriptors_check(const struct ctlv_family *family)
{
	const struct ctlv_family *descriptor;

	for (descriptor = family; descriptor; descriptor = descriptor->parent) {
		int ret;

		ret = ctlv_family_descriptor_check(descriptor);
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
	ret = ctlv_family_chain_check(family);
	if (ret)
		return ret;
	return ctlv_family_descriptors_check(family);
}

static struct ctlv_family_priv *ctlv_family_priv_find(const struct ctlv_device *ctlvdev,
						      const struct ctlv_family *family)
{
	u32 index;

	for (index = 0; index < ctlvdev->privs_count; index++)
		if (ctlvdev->privs[index].family == family)
			return &ctlvdev->privs[index];
	return NULL;
}

static void ctlv_family_privs_free(struct ctlv_device *ctlvdev)
{
	kvfree(ctlvdev->priv_block);
	ctlvdev->priv_block = NULL;
	kfree(ctlvdev->privs);
	ctlvdev->privs = NULL;
	ctlvdev->privs_count = 0;
}

static int ctlv_family_privs_alloc(struct ctlv_device *ctlvdev, const struct ctlv_family *family)
{
	struct ctlv_priv_container *parent = NULL;
	void *aligned;
	size_t alloc_len;
	size_t len = 0;
	u32 index;

	ctlvdev->privs_count = ctlv_family_levels(family);
	ctlvdev->privs = kcalloc(ctlvdev->privs_count, sizeof(*ctlvdev->privs), GFP_KERNEL);
	if (!ctlvdev->privs)
		return -ENOMEM;

	for (index = 0; index < ctlvdev->privs_count; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];
		size_t size, aligned_len;

		family_priv->family =
			ctlv_family_ancestor(family, ctlvdev->privs_count - index - 1);
		size = max_t(size_t, family_priv->family->priv_size(), 1);
		if (check_add_overflow(len, offsetof(struct ctlv_priv_container, priv),
				       &aligned_len) ||
		    check_add_overflow(aligned_len, CTLV_PRIV_ALIGN - 1, &aligned_len))
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
		ctlv_family_privs_free(ctlvdev);
		return -ENOMEM;
	}
	aligned = PTR_ALIGN(ctlvdev->priv_block, CTLV_PRIV_ALIGN);

	for (index = 0; index < ctlvdev->privs_count; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];
		struct ctlv_priv_container *container;
		void *priv;

		priv = (u8 *)aligned + family_priv->priv_offset;
		family_priv->priv = priv;
		container = container_of_const(priv, struct ctlv_priv_container, priv);
		container->parent = parent;
		container->child = NULL;
		if (parent)
			parent->child = container;
		parent = container;
	}
	return 0;

overflow:
	ctlv_family_privs_free(ctlvdev);
	return -EOVERFLOW;
}

static int ctlv_family_privs_pre_init(struct ctlv_device *ctlvdev, const void *init_info)
{
	u32 index;

	for (index = ctlvdev->privs_count; index; index--) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index - 1];

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

static void ctlv_family_privs_pre_fini(struct ctlv_device *ctlvdev)
{
	u32 index;

	lockdep_assert_not_held(&ctlvdev->registration_lock);
	lockdep_assert_not_held(&ctlvdev->transition_lock);
	lockdep_assert_not_held(&ctlvdev->events_lock);

	for (index = 0; index < ctlvdev->privs_count; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];

		if (!family_priv->initialized || !family_priv->family->pre_fini)
			continue;
		family_priv->family->pre_fini(ctlvdev, family_priv->priv);
	}
}

static void ctlv_family_privs_fini_only(struct ctlv_device *ctlvdev)
{
	u32 index;

	for (index = ctlvdev->privs_count; index; index--) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index - 1];

		if (!family_priv->initialized)
			continue;
		family_priv->family->fini(ctlvdev, family_priv->priv);
		family_priv->initialized = false;
	}
}

static int ctlv_family_privs_init(struct ctlv_device *ctlvdev)
{
	u32 index;
	int ret;

	for (index = 0; index < ctlvdev->privs_count; index++) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index];

		ret = family_priv->family->init(ctlvdev, family_priv->init_info, family_priv->priv);
		family_priv->init_info = NULL;
		if (ret) {
			ctlv_family_privs_fini_only(ctlvdev);
			return ret;
		}
		family_priv->initialized = true;
	}
	return 0;
}

static int ctlv_family_privs_post_init(struct ctlv_device *ctlvdev)
{
	u32 index;
	int ret;

	for (index = ctlvdev->privs_count; index; index--) {
		struct ctlv_family_priv *family_priv = &ctlvdev->privs[index - 1];

		if (!family_priv->family->post_init)
			continue;
		ret = family_priv->family->post_init(ctlvdev, family_priv->priv);
		if (ret)
			return ret;
	}
	return 0;
}

static void ctlv_device_fini(struct ctlv_device *ctlvdev)
{
	ctlv_family_privs_pre_fini(ctlvdev);

	/* No provider can remain after pre_fini has drained its consumers. */
	scoped_guard(spinlock, &ctlvdev->transition_lock) {
		ctlvdev->destroying = true;
		ctlv_event_contexts_stop(ctlvdev);
	}
	ctlv_event_contexts_wait(ctlvdev);
	ctlv_event_device_dead(ctlvdev);

	ctlv_family_privs_fini_only(ctlvdev);
}

/* What one walk of the chain has accounted for so far. */
struct ctlv_op_resolution {
	struct ctlv_device *ctlvdev;
	const struct ctlv_family *target;
	const struct ctlv_family *family;
	u32 bound;
	u32 dynamic;
	u32 events;
	u64 previous;
};

/* Every binding of one family answers an op, so a spare one answers nothing. */
static int ctlv_op_resolution_family_bound(const struct ctlv_op_resolution *state)
{
	const struct ctlv_family *family = state->family;

	if (!family || state->bound == family->ops_count)
		return 0;
	/* The core's own table is this core's bug, not a family's. */
	if (WARN_ON_ONCE(family == &ctlv_family))
		return -EINVAL;
	return ctlv_family_bad(family, "binds more than its schema asks for");
}

/*
 * The ops of one family are contiguous in the walk, so the family before this
 * one is complete as soon as another family's op arrives.
 */
static int ctlv_op_resolution_family_enter(struct ctlv_op_resolution *state,
					   const struct ctlv_family *family)
{
	int ret;

	if (state->family == family)
		return 0;
	ret = ctlv_op_resolution_family_bound(state);
	if (ret)
		return ret;
	state->family = family;
	state->bound = 0;
	return 0;
}

static int ctlv_op_resolution_add(struct ctlv_op_resolution *state, const struct ctlv_family *owner,
				  const struct ctlv_op_schema *op, struct ctlv_op_entry *entry)
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
	if (!CTLV_OP_NUMBER(op->op_id) || CTLV_OP_FAMILY(op->op_id) != owner->schema->family_id)
		return ctlv_op_bad(target, op->op_id, "belongs to another family");
	if (op->op_id <= state->previous)
		return ctlv_op_bad(target, op->op_id, "is out of order");
	state->previous = op->op_id;

	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
	case CTLV_OP_TYPE_QUERY:
		if (op->fd_access != CTLV_OP_FD_ACCESS_READ &&
		    op->fd_access != CTLV_OP_FD_ACCESS_READ_WRITE)
			return ctlv_op_bad(family, op->op_id, "has an unknown fd access");
		/* The family that declared the op is the one that answers it. */
		if (state->bound >= family->ops_count)
			return ctlv_op_bad(family, op->op_id, "has nothing bound to it");
		entry->binding = &family->ops[state->bound++];
		/* Required even for an op this device disables or withholds. */
		if (!entry->binding->action)
			return ctlv_op_bad(family, op->op_id, "has no callback");
		/* An optional op arrives absent, for the opt-in to clear. */
		entry->absent = op->optional;
		/* An op has a whole pair of hooks of its own, or none. */
		if (!entry->binding->pre != !entry->binding->post)
			return ctlv_op_bad(family, op->op_id, "has half a hook pair");
		/* A pair is on no wire, so this is the only check of it. */
		if (!entry->binding->pre != !op->pre_post)
			return ctlv_op_bad(family, op->op_id, "disagrees about its hook pair");
		break;
	case CTLV_OP_TYPE_EVENT:
		if (op->fd_access)
			return ctlv_op_bad(target, op->op_id, "is an event with fd access");
		if (op->optional)
			return ctlv_op_bad(target, op->op_id, "is an event a device could omit");
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
			return ctlv_op_bad(target, op->op_id, "is an event a device could disable");
		/*
		 * Numbered from zero as the walk meets them, per device: two
		 * families of one chain need not have the same ops.
		 */
		entry->state_index = state->dynamic++;
		break;
	default:
		return ctlv_op_bad(target, op->op_id, "has an unknown availability");
	}

	/*
	 * Once per device: what an inherited op may carry depends on the
	 * extension points this family filled and on nothing that changes later.
	 */
	ret = ctlv_schema_op_sizes(target, op, &entry->sizes);
	if (ret)
		return ctlv_op_bad(target, op->op_id, "needs messages this core cannot carry");
	return 0;
}

static int ctlv_op_resolution_init(struct ctlv_device *ctlvdev, const struct ctlv_family *family)
{
	struct ctlv_op_resolution state = {
		.ctlvdev = ctlvdev,
		.target = family,
	};
	const struct ctlv_family *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;
	u32 ops_count = 0;
	int ret;

	ret = ctlv_family_check(family);
	if (ret)
		return ret;
	ret = ctlv_family_privs_alloc(ctlvdev, family);
	if (ret)
		return ret;

	ctlv_schema_cursor_init(&cursor, family);
	while (ctlv_schema_next_op(&cursor, &owner))
		ops_count++;
	if (!ops_count)
		return ctlv_family_bad(family, "has an effective schema with no op");

	ctlvdev->ops = kcalloc(ops_count, sizeof(*ctlvdev->ops), GFP_KERNEL);
	if (!ctlvdev->ops)
		return -ENOMEM;
	ctlvdev->ops_count = ops_count;

	ops_count = 0;
	ctlv_schema_cursor_init(&cursor, family);
	while ((op = ctlv_schema_next_op(&cursor, &owner))) {
		ret = ctlv_op_resolution_family_enter(&state, owner);
		if (ret)
			return ret;
		ret = ctlv_op_resolution_add(&state, owner, op, &ctlvdev->ops[ops_count++]);
		if (ret)
			return ret;
	}
	/* The device family, no op of another having ended its table. */
	ret = ctlv_op_resolution_family_bound(&state);
	if (ret)
		return ret;
	ctlvdev->dynamic_ops_count = state.dynamic;
	ctlvdev->events_count = state.events;
	return 0;
}

void *ctlv_priv_parent(const void *priv)
{
	const struct ctlv_priv_container *container = ctlv_priv_container(priv);

	return container->parent ? container->parent->priv : NULL;
}
EXPORT_SYMBOL_NS_GPL(ctlv_priv_parent, "CTLV");

void *ctlv_priv_child(const void *priv)
{
	const struct ctlv_priv_container *container = ctlv_priv_container(priv);

	return container->child ? container->child->priv : NULL;
}
EXPORT_SYMBOL_NS_GPL(ctlv_priv_child, "CTLV");

const struct ctlv_family *
ctlv_device_family(const struct ctlv_device *ctlvdev)
{
	return READ_ONCE(ctlvdev->family);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_family, "CTLV");

struct device *ctlv_device_parent(const struct ctlv_device *ctlvdev)
{
	return ctlvdev->dev.parent;
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_parent, "CTLV");

/*
 * Dispatch reads the availability bit under no lock, so an op already admitted
 * is neither cancelled nor waited for by a disable.
 */

struct ctlv_op_entry *ctlv_op_find(struct ctlv_device *ctlvdev, u64 op_id)
{
	u32 index;

	/* The index ascends by op ID, so a miss stops early. */
	for (index = 0; index < ctlvdev->ops_count; index++) {
		if (ctlvdev->ops[index].schema->op_id == op_id)
			return &ctlvdev->ops[index];
		if (ctlvdev->ops[index].schema->op_id > op_id)
			break;
	}
	return NULL;
}

static bool ctlv_op_disabled(struct ctlv_device *ctlvdev, const struct ctlv_op_entry *op)
{
	if (op->state_index == CTLV_OP_STATE_NONE)
		return false;
	/* Acquire, so what the family prepared before enabling is visible. */
	return test_bit_acquire(op->state_index, ctlvdev->disabled);
}

static void ctlv_op_publish_all(struct ctlv_device *ctlvdev)
{
	u32 published = 0;
	u32 index;

	/* Against a family still opting in as this is called. */
	guard(spinlock)
		(&ctlvdev->transition_lock);
	for (index = 0; index < ctlvdev->ops_count; index++) {
		if (ctlvdev->ops[index].absent)
			continue;
		ctlvdev->ops[published++] = ctlvdev->ops[index];
	}
	ctlvdev->ops_count = published;
	ctlvdev->published = true;
}

void ctlv_device_op_provide(struct ctlv_device *ctlvdev, u64 op_id)
{
	struct ctlv_op_entry *op;

	guard(spinlock)
		(&ctlvdev->transition_lock);
	if (WARN_ON_ONCE(ctlvdev->published || ctlvdev->destroying))
		return;
	op = ctlv_op_find(ctlvdev, op_id);
	/* An event is produced rather than answered, so there is none. */
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(!op->binding))
		return;
	op->absent = false;
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_provide, "CTLV");

void ctlv_device_op_state_set(struct ctlv_device *ctlvdev, u64 op_id, bool disabled)
{
	const struct ctlv_op_entry *op;

	/*
	 * Also serializes lookup against publication, which compacts the
	 * array, and against teardown stopping state changes.
	 */
	guard(spinlock)
		(&ctlvdev->transition_lock);
	if (ctlvdev->destroying)
		return;
	op = ctlv_op_find(ctlvdev, op_id);
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(op->state_index == CTLV_OP_STATE_NONE))
		return;
	/* An op this device never took on has no state to change. */
	if (WARN_ON_ONCE(op->absent))
		return;
	/*
	 * Fully ordered, releasing what the family prepared to the acquire in
	 * ctlv_op_disabled(). The old state makes a repeat change nothing.
	 */
	if (disabled ? test_and_set_bit(op->state_index, ctlvdev->disabled) :
		       !test_and_clear_bit(op->state_index, ctlvdev->disabled))
		return;
	if (!ctlvdev->published)
		return;
	ctlvdev->state_generation++;
	/* Still holding the serializer, so records reach queues in order. */
	ctlv_event_op_changed(ctlvdev, op, disabled, ctlvdev->state_generation);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_state_set, "CTLV");

static int ctlv_op_check_mode(const struct file *filp, const struct ctlv_op_entry *op)
{
	if (op->schema->type == CTLV_OP_TYPE_EVENT)
		return -EOPNOTSUPP;

	switch (op->schema->fd_access) {
	case CTLV_OP_FD_ACCESS_READ:
		return filp->f_mode & FMODE_READ ? 0 : -EACCES;
	case CTLV_OP_FD_ACCESS_READ_WRITE:
		return (filp->f_mode & (FMODE_READ | FMODE_WRITE)) == (FMODE_READ | FMODE_WRITE) ?
			       0 :
			       -EACCES;
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
 * request_len is the copy boundary; root length frames the message. Ignored
 * unknown attrs may exceed the schema maximum up to the transport limit.
 */
static int ctlv_request_frame(struct ctlv_op_ctx *ctx, const struct ctlv_op_exec *exec,
			      const struct ctlv_attr *root)
{
	if (root->len < sizeof(*root) || root->len > exec->request_len ||
	    root->attr_id != CTLV_ATTR_ROOT || root->flags != CTLV_ATTR_F_NESTED) {
		ctx->error.reason = CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		return -EINVAL;
	}
	return 0;
}

static int ctlv_request_validate(struct ctlv_op_ctx *ctx, const struct ctlv_op_exec *exec,
				 struct ctlv_attr *root)
{
	int ret;

	ret = ctlv_request_frame(ctx, exec, root);
	if (ret)
		return ret;
	if (!ctx->op->schema->request.node_schemas)
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
static u32 ctlv_reply_capacity(const struct ctlv_op_entry *op, const struct ctlv_op_exec *exec)
{
	if (!op->schema->reply.node_schemas)
		return 0;
	if (op->schema->type == CTLV_OP_TYPE_QUERY)
		return exec->reply_len;
	return op->sizes.action_reply_max_len;
}

/* Actions and final query fragments must be complete schema-valid replies. */
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

/* A successful query callback must end exactly one fragment. */
static int ctlv_query_fragment_result(struct ctlv_op_ctx *ctx)
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
		ctlv_message_root(&ctx->request_root, &ctx->request);
		request = &ctx->request_root;
	}
	if (ctx->output.root)
		reply = &ctx->output_root;

	ctlv_blob_context_init(ctx);
	if (op->schema->type == CTLV_OP_TYPE_ACTION) {
		ret = op->binding->action(op->priv, ctx, request, reply);
	} else {
		ret = op->binding->query(op->priv, ctx, request, reply);
		if (!ret)
			ret = ctlv_query_fragment_result(ctx);
	}
	blob_ret = ctlv_blob_context_finish(ctx);
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

	if (CTLV_OP_FAMILY(ctx->op->schema->op_id) == CTLV_FAMILY_ROOT) {
		family = &ctlv_family;
		family_priv = ctx->ctlvdev->privs[0].priv;
	} else {
		family = ctx->ctlvdev->family;
		family_priv = ctx->ctlvdev->privs[ctx->ctlvdev->privs_count - 1].priv;
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

/* Error information replaces the reply atomically; errno remains authoritative. */
static void ctlv_error_info(struct ctlv_op_ctx *ctx, u32 room, void **reply, u32 *reply_len)
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
	ctlv_message_output_init(&ctx->output, ctx, buf, CTLV_ERROR_INFO_MAX_LEN,
				 &ctlv_error_info_schema);
	ctlv_message_root(root, &ctx->output);
	ctx->deepest = root;

	if (error->reason)
		ctlv_put_u64(root, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM, error->reason);
	if (error->message[0])
		ctlv_error_message_put(root, error->message);
	if (error->reason == CTLV_VALIDATION_REASON_STALE_GENERATION) {
		/* The one failure that is about no part of any message. */
		ctlv_put_u64(root, CTLV_ATTR_ERROR_EXPECTED_GENERATION, CTLV_SCHEMA_UINT,
			     error->expected_generation);
		ctlv_put_u64(root, CTLV_ATTR_ERROR_CURRENT_GENERATION, CTLV_SCHEMA_UINT,
			     error->current_generation);
	} else if (error->reason) {
		ctlv_put_u64(root, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET, CTLV_SCHEMA_UINT,
			     error->bad_attr_offset);
		ctlv_put_u64(root, CTLV_ATTR_ERROR_NEST_OFFSET, CTLV_SCHEMA_UINT,
			     error->nest_offset);
		/* No attr is numbered zero, so a zero names nothing. */
		if (error->missing_attr_id)
			ctlv_put_u64(root, CTLV_ATTR_ERROR_MISSING_ATTR_ID, CTLV_SCHEMA_UINT,
				     error->missing_attr_id);
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
static int ctlv_op_exec(struct ctlv_file *cfile, struct file *filp, struct ctlv_op_exec *exec,
			void **reply, u32 *reply_len)
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
	/* And no longer than a message of this core may be. */
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
	if (copy_from_user(snapshot, u64_to_user_ptr(exec->request), exec->request_len))
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
		ctlv_message_output_init(&ctx.output, &ctx, out, capacity, &op->schema->reply);
		ctlv_message_root(&ctx.output_root, &ctx.output);
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
	ctlv_message_index_free(&ctx.request);
	return ret;
}

/* What one family of this device's chain does with one extension point. */
static const struct ctlv_stub_ops *ctlv_stub_ops_find(const struct ctlv_device *ctlvdev,
						      const struct ctlv_stub_schema *stub_schema,
						      void **priv)
{
	const struct ctlv_family *family;
	u32 index;

	/* From the device family towards the core, where the fill was found. */
	for (family = ctlvdev->family; family; family = family->parent)
		for (index = 0; index < family->stub_ops_count; index++)
			if (family->stub_ops[index].family_id == stub_schema->family_id &&
			    family->stub_ops[index].set_id == stub_schema->set_id) {
				struct ctlv_family_priv *family_priv;

				family_priv = ctlv_family_priv_find(ctlvdev, family);
				if (WARN_ON_ONCE(!family_priv))
					return NULL;
				*priv = family_priv->priv;
				return &family->stub_ops[index];
			}
	return NULL;
}

int ctlv_stub_fill(struct ctlv_op_ctx *ctx, const struct ctlv_cursor *request,
		   struct ctlv_cursor *nest)
{
	const struct ctlv_schema_tree *tree;
	u32 index;

	if (WARN_ON_ONCE(!nest->tree))
		return -EINVAL;
	tree = nest->tree;
	for (index = 0; index < tree->stub_schemas_count; index++) {
		const struct ctlv_stub_schema *stub_schema = &tree->stub_schemas[index];
		const struct ctlv_cursor *filled_request = request;
		const struct ctlv_stub_fill_schema *fill_schema;
		const struct ctlv_stub_ops *stub_ops;
		struct ctlv_cursor filled;
		void *priv;
		int ret;

		if (stub_schema->node != nest->node)
			continue;
		stub_ops = ctlv_stub_ops_find(ctx->ctlvdev, stub_schema, &priv);
		if (!stub_ops || !stub_ops->fill)
			continue;
		if (request) {
			fill_schema = ctlv_schema_fill(ctx->family, stub_schema->family_id,
						       stub_schema->set_id);
			if (WARN_ON_ONCE(!fill_schema))
				return -EINVAL;
			filled = *request;
			if (ctlv_message_input_rebind(&filled, &fill_schema->members, 0))
				filled_request = &filled;
			else
				filled_request = NULL;
		}
		ret = stub_ops->fill(priv, ctx, filled_request, nest);
		if (ret)
			return ret;
	}
	return 0;
}
EXPORT_SYMBOL_NS_GPL(ctlv_stub_fill, "CTLV");

/*
 * The core answers the root family through the same generated descriptor a
 * family creates with, so it needs the same callbacks.
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
	if (WARN_ON_ONCE(init_info) || WARN_ON_ONCE(ctlv_priv_parent(priv)))
		return -EINVAL;
	return 0;
}

void ctlv_fini(struct ctlv_device *ctlvdev, void *priv)
{
}

static long ctlv_file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
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
	/* Do not make destruction wait for a faulting userspace copy. */
	if (reply_len && copy_to_user(u64_to_user_ptr(exec.reply), reply, reply_len))
		return ret ?: -EFAULT;
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
static ssize_t ctlv_file_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	struct ctlv_file *cfile = filp->private_data;

	return ctlv_event_file_read(cfile, buf, len, !(filp->f_flags & O_NONBLOCK));
}

static __poll_t ctlv_file_poll(struct file *filp, struct poll_table_struct *wait)
{
	struct ctlv_file *cfile = filp->private_data;

	return ctlv_event_file_poll(cfile, filp, wait);
}

static bool ctlv_file_open_allowed(struct ctlv_device *ctlvdev)
{
	lockdep_assert_held_read(&ctlvdev->registration_lock);
	return ctlvdev->registered && ctlvdev->family;
}

static void ctlv_file_open_pinned(struct ctlv_device *ctlvdev)
{
}

static int ctlv_file_open(struct inode *inode, struct file *filp)
{
	struct ctlv_device *ctlvdev = container_of(inode->i_cdev, struct ctlv_device, cdev);
	struct ctlv_file *cfile;
	int ret;

	/*
	 * A failed create can withdraw the cdev while an open is waiting for
	 * the lifecycle lock, so admission itself first pins core storage.
	 */
	get_device(&ctlvdev->dev);
	ctlv_file_open_pinned(ctlvdev);
	scoped_guard(rwsem_read, &ctlvdev->registration_lock) {
		if (!ctlv_file_open_allowed(ctlvdev)) {
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
		ret = ctlv_event_file_init(cfile);
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

static int ctlv_file_release(struct inode *inode, struct file *filp)
{
	struct ctlv_file *cfile = filp->private_data;
	struct ctlv_device *ctlvdev = cfile->ctlvdev;

	ctlv_event_file_exit(cfile);
	kfree(cfile);
	put_device(&ctlvdev->dev);
	return 0;
}

static const struct file_operations ctlv_file_operations = {
	.owner = THIS_MODULE,
	.open = ctlv_file_open,
	.release = ctlv_file_release,
	.read = ctlv_file_read,
	.poll = ctlv_file_poll,
	.unlocked_ioctl = ctlv_file_ioctl,
	/* Every ioctl field is fixed width, so one entry point serves both. */
	.compat_ioctl = compat_ptr_ioctl,
};

/* Uevent identity is available to rules before the device can be opened. */
static int ctlv_device_uevent_families(struct kobj_uevent_env *env,
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

		ret = add_uevent_var(env, "CTLV_COMPATIBLE_%u=%s", level, ancestor->schema->name);
		if (ret)
			return ret;
	}
	return add_uevent_var(env, "CTLV_COMPATIBLE_N=%u", levels);
}

static int ctlv_device_uevent_check(const struct ctlv_family *family)
{
	struct kobj_uevent_env *env;
	int ret;

	env = kzalloc_obj(*env, GFP_KERNEL);
	if (!env)
		return -ENOMEM;
	ret = ctlv_device_uevent_families(env, family);
	if (!ret && (env->buflen > CTLV_UEVENT_MAX_LEN || env->envp_idx > CTLV_UEVENT_MAX_VARS)) {
		pr_err("family %s: its chain of names needs %d bytes in %d properties of a uevent, more than the %d in %d it may take\n",
		       family->schema->name, env->buflen, env->envp_idx, CTLV_UEVENT_MAX_LEN,
		       CTLV_UEVENT_MAX_VARS);
		ret = -E2BIG;
	}
	kfree(env);
	return ret;
}

/* Read unlocked: a uevent may be asked for under the registration lock. */
static int ctlv_device_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
	const struct ctlv_device *ctlvdev = container_of_const(dev, struct ctlv_device, dev);
	const struct ctlv_family *family = READ_ONCE(ctlvdev->family);

	if (!family)
		return 0;
	return ctlv_device_uevent_families(env, family);
}

static void ctlv_device_release(struct device *dev)
{
	struct ctlv_device *ctlvdev = container_of(dev, struct ctlv_device, dev);

	ida_free(&ctlv_device_ids, ctlvdev->dev.devt - ctlv_device_number);
	bitmap_free(ctlvdev->disabled);
	ctlv_family_privs_free(ctlvdev);
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
}

static struct class ctlv_device_class = {
	.name = "ctlv",
	.dev_release = ctlv_device_release,
	.dev_uevent = ctlv_device_uevent,
};

static int ctlv_device_add(struct ctlv_device *ctlvdev)
{
	lockdep_assert_held_write(&ctlvdev->registration_lock);

	return cdev_device_add(&ctlvdev->cdev, &ctlvdev->dev);
}

static void ctlv_device_del(struct ctlv_device *ctlvdev)
{
	cdev_device_del(&ctlvdev->cdev, &ctlvdev->dev);
}

struct ctlv_device *ctlv_create(struct device *parent_dev, const struct ctlv_family *family,
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
	ctlv_event_device_init(ctlvdev);

	/* Before anything reads the schema through this device. */
	ret = ctlv_op_resolution_init(ctlvdev, family);
	if (ret)
		goto err_free;
	/* What this family's events need of every queue opened on it. */
	ret = ctlv_event_sizes(ctlvdev);
	if (ret)
		goto err_free;
	/* Before a minor is spent on a device that could not be announced. */
	ret = ctlv_device_uevent_check(family);
	if (ret)
		goto err_free;
	if (ctlvdev->dynamic_ops_count) {
		ctlvdev->disabled = bitmap_zalloc(ctlvdev->dynamic_ops_count, GFP_KERNEL);
		if (!ctlvdev->disabled) {
			ret = -ENOMEM;
			goto err_free;
		}
	}
	devnum = ida_alloc_max(&ctlv_device_ids, CTLV_MAX_DEVICES - 1, GFP_KERNEL);
	if (devnum < 0) {
		ret = devnum;
		goto err_free;
	}

	ctlvdev->dev.devt = ctlv_device_number + devnum;
	ctlvdev->dev.class = &ctlv_device_class;
	ctlvdev->dev.parent = parent_dev;
	device_initialize(&ctlvdev->dev);
	/*
	 * The core owns the file operations, so an open holds the core module
	 * and not the family's, which may unload while readers remain.
	 */
	cdev_init(&ctlvdev->cdev, &ctlv_file_operations);
	ctlvdev->cdev.owner = THIS_MODULE;

	ret = dev_set_name(&ctlvdev->dev, "ctlv%d", devnum);
	if (ret)
		goto err_put;
	ret = ctlv_family_claim_add(family->schema);
	if (ret)
		goto err_put;
	ret = ctlv_family_privs_pre_init(ctlvdev, init_info);
	if (ret)
		goto err_put;
	ret = ctlv_family_privs_init(ctlvdev);
	if (ret)
		goto err_put;
	ctlv_op_publish_all(ctlvdev);
	ctlv_event_contexts_start(ctlvdev);
	ret = ctlv_family_privs_post_init(ctlvdev);
	if (ret)
		goto err_fini;
	scoped_guard(rwsem_write, &ctlvdev->registration_lock) {
		ret = ctlv_device_add(ctlvdev);
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
	ctlv_family_privs_free(ctlvdev);
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(ctlv_create, "CTLV");

void ctlv_destroy(struct ctlv_device *ctlvdev)
{
	u32 index;

	/* No new opens, no discovery entry, and no node to open. */
	ctlv_device_del(ctlvdev);

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
	for (index = 0; index < ctlvdev->ops_count; index++) {
		ctlvdev->ops[index].binding = NULL;
		ctlvdev->ops[index].priv = NULL;
	}
	ctlv_family_privs_free(ctlvdev);
	put_device(&ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_destroy, "CTLV");

static int __init ctlv_module_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&ctlv_device_number, 0, CTLV_MAX_DEVICES, "ctlv");
	if (ret)
		return ret;

	ret = class_register(&ctlv_device_class);
	if (ret)
		goto err_unregister_chrdev_region;

	return 0;

err_unregister_chrdev_region:
	unregister_chrdev_region(ctlv_device_number, CTLV_MAX_DEVICES);
	return ret;
}

static void __exit ctlv_module_exit(void)
{
	class_unregister(&ctlv_device_class);
	unregister_chrdev_region(ctlv_device_number, CTLV_MAX_DEVICES);
	ctlv_family_claims_free();
	ida_destroy(&ctlv_device_ids);
}

subsys_initcall(ctlv_module_init);
module_exit(ctlv_module_exit);
MODULE_DESCRIPTION("CTLV character device TLV UAPI core");
MODULE_LICENSE("GPL");
/* A blob attr may name a dma-buf, so the core resolves one itself. */
MODULE_IMPORT_NS("DMA_BUF");
