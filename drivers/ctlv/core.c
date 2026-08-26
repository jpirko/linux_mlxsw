// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 */
#define pr_fmt(fmt) "ctlv: " fmt
#include <kunit/visibility.h>
#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/build_bug.h>
#include <linux/cleanup.h>
#include <linux/container_of.h>
#include <linux/ctlv.h>
#include <linux/ctlv_schema_gen.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kdev_t.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#include <uapi/linux/ctlv.h>
#include <uapi/linux/ctlv_gen.h>

#include "ctlv_family_gen.h"
#include "ctlv_private.h"

enum {
	CTLV_MAX_DEVICES = 4096,
};

static_assert(CTLV_MAX_DEVICES < (1U << MINORBITS));

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
	const struct ctlv_family_schema *schema = family->schema;
	u32 levels = ctlv_schema_levels(schema);

	if (ctlv_schema_ancestor(schema, levels - 1) != &ctlv_ctlv_schema)
		return ctlv_family_bad(family, "heads no chain of the core's");

	for (; levels > 1; levels--) {
		const struct ctlv_family_schema *child =
			ctlv_schema_ancestor(schema, levels - 2);
		const struct ctlv_family_schema *parent = child->parent;
		size_t len;

		if (!child->name || !parent->name)
			return ctlv_family_bad(family, "inherits a family with no name");
		len = strlen(parent->name);
		if (!len || strncmp(child->name, parent->name, len) ||
		    child->name[len] != '-')
			return ctlv_family_bad(family, "is not named after the family it inherits");
	}
	return 0;
}

static int ctlv_family_check(const struct ctlv_family *family)
{
	const struct ctlv_family_schema *schema = family->schema;

	if (!schema || !schema->name || !*schema->name || !family->ops ||
	    !family->n_ops)
		return ctlv_family_bad(family, "descriptor is incomplete");
	/* Nothing in common means an empty pair, not an absent one. */
	if (!family->pre || !family->post)
		return ctlv_family_bad(family, "has no hook pair");
	/* Every device answers the root family already, as the framework. */
	if (schema->family_id == CTLV_FAMILY_ID)
		return ctlv_family_bad(family, "claims the root family ID");
	return ctlv_chain_check(family);
}

/* What one walk of the chain has accounted for so far. */
struct ctlv_resolve {
	const struct ctlv_family *family;
	u32 bound;
	u32 core_bound;
	u32 dynamic;
	u32 events;
	u64 previous;
};

static int ctlv_op_resolve(struct ctlv_resolve *state,
			   const struct ctlv_family_schema *owner,
			   const struct ctlv_op_schema *op,
			   struct ctlv_op_entry *entry)
{
	const struct ctlv_family *family = state->family;
	const struct ctlv_family *answers;
	u32 *bound;
	int ret;

	entry->schema = op;
	entry->state_index = CTLV_OP_STATE_NONE;
	/*
	 * The two tables partition the effective ops: the framework answers
	 * the root family's, the registered family everything else.
	 */
	if (owner == &ctlv_ctlv_schema) {
		answers = &ctlv_ctlv_family;
		bound = &state->core_bound;
	} else {
		answers = family;
		bound = &state->bound;
	}

	/*
	 * An op ID is its family's ID and a number within it, so a walk from
	 * the oldest ancestor down ascends, which covers uniqueness too.
	 */
	if (!CTLV_OP_ID_NUMBER(op->op_id) ||
	    CTLV_OP_ID_FAMILY(op->op_id) != owner->family_id)
		return ctlv_op_bad(family, op->op_id, "belongs to another family");
	if (op->op_id <= state->previous)
		return ctlv_op_bad(family, op->op_id, "is out of order");
	state->previous = op->op_id;

	switch (op->type) {
	case CTLV_OP_TYPE_ACTION:
	case CTLV_OP_TYPE_QUERY:
		if (*bound >= answers->n_ops)
			return ctlv_op_bad(family, op->op_id, "has nothing bound to it");
		entry->answers = answers;
		entry->binding = &answers->ops[(*bound)++];
		/* Required even for an op this device disables or removes. */
		if (!entry->binding->action)
			return ctlv_op_bad(family, op->op_id, "has no callback");
		/* An op has a whole pair of hooks of its own, or none. */
		if (!entry->binding->pre != !entry->binding->post)
			return ctlv_op_bad(family, op->op_id, "has half a hook pair");
		/* A pair is on no wire, so this is the only check of it. */
		if (!entry->binding->pre != !op->pre_post)
			return ctlv_op_bad(family, op->op_id, "disagrees about its hook pair");
		break;
	case CTLV_OP_TYPE_EVENT:
		/*
		 * Produced rather than answered, so nothing to bind: an event
		 * gets a subscription bit, numbered as the walk meets it.
		 */
		entry->event_index = state->events++;
		break;
	default:
		return ctlv_op_bad(family, op->op_id, "has an unknown type");
	}

	switch (op->availability) {
	case CTLV_OP_AVAILABILITY_FIXED:
		break;
	case CTLV_OP_AVAILABILITY_DYNAMIC:
		if (op->type == CTLV_OP_TYPE_EVENT)
			return ctlv_op_bad(family, op->op_id, "is an event a device could disable");
		/*
		 * Numbered from zero as the walk meets them, per device: two
		 * leaves of one chain need not have the same ops.
		 */
		entry->state_index = state->dynamic++;
		break;
	default:
		return ctlv_op_bad(family, op->op_id, "has an unknown availability");
	}

	/*
	 * Once per device: what an inherited op may carry depends on the
	 * extension points this leaf filled and on nothing that changes later.
	 */
	ret = ctlv_schema_op_sizes(family->schema, op, &entry->sizes);
	if (ret)
		return ctlv_op_bad(family, op->op_id, "needs messages this framework cannot carry");
	return 0;
}

/**
 * ctlv_ops_resolve - Build one device's index into its family's schema graph.
 * @ctlvdev: device being allocated
 * @family: generated descriptor it will answer as
 *
 * On success @ctlvdev owns an entry per effective op, ascending by op ID. The
 * caller frees what this allocated whether or not it succeeded.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_ops_resolve(struct ctlv_device *ctlvdev,
		     const struct ctlv_family *family)
{
	struct ctlv_resolve state = { .family = family };
	const struct ctlv_family_schema *owner;
	const struct ctlv_op_schema *op;
	struct ctlv_schema_cursor cursor;
	u32 n_ops = 0;
	int ret;

	ret = ctlv_family_check(family);
	if (ret)
		return ret;

	ctlv_schema_cursor_init(&cursor, family->schema);
	while (ctlv_schema_next_op(&cursor, &owner))
		n_ops++;
	if (!n_ops)
		return ctlv_family_bad(family, "has an effective schema with no op");

	ctlvdev->ops = kcalloc(n_ops, sizeof(*ctlvdev->ops), GFP_KERNEL);
	if (!ctlvdev->ops)
		return -ENOMEM;
	ctlvdev->n_ops = n_ops;

	n_ops = 0;
	ctlv_schema_cursor_init(&cursor, family->schema);
	while ((op = ctlv_schema_next_op(&cursor, &owner))) {
		ret = ctlv_op_resolve(&state, owner, op, &ctlvdev->ops[n_ops++]);
		if (ret)
			return ret;
	}
	/* Every binding answers an op, so a spare one answers nothing. */
	if (state.bound != family->n_ops)
		return ctlv_family_bad(family, "binds more than its schema asks for");
	/*
	 * The framework's own table too, so a core op with no implementation
	 * is caught by the first device allocated.
	 */
	if (WARN_ON_ONCE(state.core_bound != ctlv_ctlv_family.n_ops))
		return -EINVAL;
	ctlvdev->n_dynamic_ops = state.dynamic;
	ctlvdev->n_events = state.events;
	return 0;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_ops_resolve);

/**
 * ctlv_family_is_a - Whether one generated family inherits another's schema.
 * @family: descriptor a driver is about to register
 * @ancestor: schema to look for, @family's own schema counting as one
 *
 * Ancestry is immutable generated metadata, so this needs no device, invokes
 * no callback and takes no lock.
 *
 * Return: true when @family is @ancestor or inherits it.
 */
bool ctlv_family_is_a(const struct ctlv_family *family,
		      const struct ctlv_family_schema *ancestor)
{
	if (!family || !family->schema || !ancestor)
		return false;
	return ctlv_schema_is_a(family->schema, ancestor);
}
EXPORT_SYMBOL_NS_GPL(ctlv_family_is_a, "CTLV");

/*
 * Dispatch reads the availability bit under no lock, so an op already admitted
 * is neither cancelled nor waited for by a disable.
 */

/**
 * ctlv_op_find - Look one op up in what a device resolved.
 * @ctlvdev: device to look in
 * @op_id: complete ID of the op, which names the family that declared it
 *
 * Return: the entry, or NULL when this device has no such op. An op the family
 * removed is still found until publication drops it.
 */
struct ctlv_op_entry *ctlv_op_find(struct ctlv_device *ctlvdev, u64 op_id)
{
	u32 index;

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
 * @ctlvdev: device on its way to being registered
 *
 * Drops the ops the family took away, once, and keeps the ascending order and
 * the availability numbering of what is left.
 */
void ctlv_ops_publish(struct ctlv_device *ctlvdev)
{
	u32 published = 0;
	u32 index;

	/* Against a family still taking ops away as this is called. */
	guard(spinlock)(&ctlvdev->transition_lock);
	ctlvdev->published = true;
	for (index = 0; index < ctlvdev->n_ops; index++) {
		if (ctlvdev->ops[index].removed)
			continue;
		ctlvdev->ops[published++] = ctlvdev->ops[index];
	}
	ctlvdev->n_ops = published;
}
EXPORT_SYMBOL_IF_KUNIT(ctlv_ops_publish);

/**
 * ctlv_device_op_remove - Take one op away from one device for good.
 * @ctlvdev: device to narrow, which nothing may have observed yet
 * @op_id: complete ID of the op, which names the family that declared it
 *
 * Called only by the generated per-op helpers. The op is absent rather than
 * disabled: publication leaves it out of the index and it stays out for the
 * life of the device.
 *
 * Idempotent, and cannot fail. An op this device does not have, and a device
 * already published, both warn and change nothing.
 */
void ctlv_device_op_remove(struct ctlv_device *ctlvdev, u64 op_id)
{
	struct ctlv_op_entry *op = ctlv_op_find(ctlvdev, op_id);

	/* An event is produced rather than answered, so there is none. */
	if (WARN_ON_ONCE(!op) || WARN_ON_ONCE(!op->binding))
		return;

	guard(spinlock)(&ctlvdev->transition_lock);
	if (WARN_ON_ONCE(ctlvdev->published))
		return;
	op->removed = true;
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_remove, "CTLV");

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
 * Valid from allocation onwards, including from inside an op's own callback. A
 * change that races unregistration is dropped.
 */
void ctlv_device_op_state_set(struct ctlv_device *ctlvdev, u64 op_id,
			      bool disabled)
{
	const struct ctlv_op_entry *op = ctlv_op_find(ctlvdev, op_id);

	if (WARN_ON_ONCE(!op) ||
	    WARN_ON_ONCE(op->state_index == CTLV_OP_STATE_NONE))
		return;
	/*
	 * Read without the registration lock: an op changing availability
	 * holds it already, and this is device state, not the family's.
	 */
	if (!READ_ONCE(ctlvdev->family))
		return;

	/*
	 * Orders writers against each other and against the generation. It is
	 * deliberately not an admission lock.
	 */
	guard(spinlock)(&ctlvdev->transition_lock);
	/* An op given up before publication is not one to give back. */
	if (WARN_ON_ONCE(op->removed))
		return;
	/*
	 * Fully ordered, releasing what the family prepared to the acquire in
	 * ctlv_op_disabled(). The old state makes a repeat change nothing.
	 */
	if (disabled ? test_and_set_bit(op->state_index, ctlvdev->disabled)
		     : !test_and_clear_bit(op->state_index, ctlvdev->disabled))
		return;
	ctlvdev->state_generation++;
	/* Still holding the serializer, so records reach queues in order. */
	ctlv_events_op_changed(ctlvdev, op, disabled, ctlvdev->state_generation);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_op_state_set, "CTLV");

static int ctlv_op_check_mode(const struct file *filp,
			      const struct ctlv_op_entry *op)
{
	switch (op->schema->type) {
	case CTLV_OP_TYPE_ACTION:
		/* An action may mutate, so it needs the mode that says so. */
		return filp->f_mode & FMODE_WRITE ? 0 : -EACCES;
	case CTLV_OP_TYPE_QUERY:
		return filp->f_mode & FMODE_READ ? 0 : -EACCES;
	default:
		/* An event is delivered to a reader, never executed. */
		return -EOPNOTSUPP;
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
 * failure is the message as a whole.
 */
static int ctlv_request_frame(struct ctlv_op_ctx *ctx,
			      const struct ctlv_op_exec *exec,
			      const struct ctlv_attr *root)
{
	if (root->len > ctx->op->sizes.request_max_len)
		return -EMSGSIZE;
	if (root->len < sizeof(*root) || root->len > exec->request_len ||
	    root->attr_id != CTLV_ATTR_ID_ROOT ||
	    root->flags != CTLV_ATTR_F_NESTED) {
		ctx->error.reason = CTLV_VALIDATION_REASON_MALFORMED_MESSAGE;
		return -EINVAL;
	}
	return 0;
}

/*
 * A caller that sent nothing is answered as if it had sent an empty root, so
 * that a missing required attr does the same thing either way.
 */
static int ctlv_request_validate(struct ctlv_op_ctx *ctx,
				 const struct ctlv_op_exec *exec,
				 struct ctlv_attr *root, struct ctlv_attr *empty)
{
	size_t len = sizeof(*empty);
	int ret;

	if (root) {
		/* Which also refuses a request to an op that takes none. */
		ret = ctlv_request_frame(ctx, exec, root);
		if (ret)
			return ret;
		len = exec->request_len;
	} else {
		*empty = (struct ctlv_attr){
			.len = sizeof(*empty),
			.flags = CTLV_ATTR_F_NESTED,
			.attr_id = CTLV_ATTR_ID_ROOT,
		};
		root = empty;
	}
	if (!ctx->op->schema->request.nodes)
		return 0;

	ctx->request = (struct ctlv_msg){
		.root = root,
		.len = len,
		.tree = &ctx->op->schema->request,
		.leaf = ctx->leaf,
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
	int ret;

	if (ctx->request.root) {
		ctlv_msg_root(&ctx->request_root, &ctx->request);
		request = &ctx->request_root;
	}
	if (ctx->output.root)
		reply = &ctx->output_root;

	if (op->schema->type == CTLV_OP_TYPE_ACTION) {
		ret = op->binding->action(ctx, request, reply);
	} else {
		ret = op->binding->query(ctx, request, reply);
		if (!ret)
			ret = ctlv_frag_result(ctx);
	}
	if (ret)
		return ret;
	return ctlv_reply_finish(ctx);
}

/*
 * The answering family's hooks around the op's around the callback, with the
 * availability check innermost. A pre that fails is the result of the op, and
 * everything whose pre succeeded is unwound whatever the result. Nothing here
 * runs with the transition serializer held, so a hook may change availability.
 */
static int ctlv_op_dispatch(struct ctlv_op_ctx *ctx)
{
	const struct ctlv_family *answers = ctx->op->answers;
	const struct ctlv_op *op = ctx->op->binding;
	int ret;

	ret = answers->pre(ctx);
	if (ret)
		return ret;

	if (op->pre) {
		ret = op->pre(ctx);
		if (ret)
			goto out_family;
	}

	/* The only authoritative check: introspection is advisory. */
	if (ctlv_op_disabled(ctx->ctlvdev, ctx->op))
		ret = -EOPNOTSUPP;
	else
		ret = ctlv_op_call(ctx);

	if (op->post)
		op->post(ctx);
out_family:
	answers->post(ctx);
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

	if (!error->reason || !room)
		return;
	buf = kvmalloc(CTLV_ERROR_INFO_MAX_LEN, GFP_KERNEL_ACCOUNT);
	if (!buf)
		return;

	/* The same builder over another schema, and a buffer of its own. */
	ctlv_msg_output(&ctx->output, ctx, buf, CTLV_ERROR_INFO_MAX_LEN,
			&ctlv_error_info_schema);
	ctlv_msg_root(root, &ctx->output);
	ctx->deepest = root;

	ctlv_put_u64(root, CTLV_ATTR_ERROR_REASON, CTLV_SCHEMA_ENUM,
		     error->reason);
	if (error->reason == CTLV_VALIDATION_REASON_STALE_GENERATION) {
		/* The one failure that is about no part of any message. */
		ctlv_put_u64(root, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
			     CTLV_SCHEMA_UINT, error->expected_generation);
		ctlv_put_u64(root, CTLV_ATTR_ERROR_CURRENT_GENERATION,
			     CTLV_SCHEMA_UINT, error->current_generation);
	} else {
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
	struct ctlv_attr empty;
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
	ctx.leaf = ctlvdev->family->schema;
	ctx.file = cfile;
	/* Whose memory a blob descriptor of this request describes. */
	ctx.submitter = current;
	ctx.op = op;

	void *snapshot __free(kvfree) = NULL;
	void *out __free(kvfree) = NULL;

	if (exec->request_len) {
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
	}
	/* Before any hook, so that no family code sees an unchecked attr. */
	ret = ctlv_request_validate(&ctx, exec, snapshot, &empty);
	if (ret)
		goto out_error_info;

	capacity = ctlv_reply_capacity(op, exec);
	if (capacity) {
		/*
		 * Not zeroed: a byte is copied out only once the root's
		 * length covers it, padding included.
		 */
		out = kvmalloc(capacity, GFP_KERNEL_ACCOUNT);
		if (!out)
			return -ENOMEM;
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
	return 0;

out_error_info:
	ctlv_error_info(&ctx, exec->reply_len, reply, reply_len);
	return ret;
}

/**
 * ctlv_op_priv - The family-private pointer of the device being operated on.
 * @ctx: context of the op being executed
 *
 * Valid for as long as the callback that was given @ctx has not returned.
 */
void *ctlv_op_priv(struct ctlv_op_ctx *ctx)
{
	return ctx->ctlvdev->priv;
}
EXPORT_SYMBOL_NS_GPL(ctlv_op_priv, "CTLV");

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
 * cursor has to be alive here. A @next_cursor that is zero, or the one this
 * invocation started from, is a reply that never ends and is refused.
 *
 * Return: zero, or the negative errno that is the result of the op.
 */
int ctlv_op_query_more(struct ctlv_op_ctx *ctx, u64 generation,
		       u64 next_cursor)
{
	int ret = ctlv_frag_end(ctx, generation);

	if (!ret && WARN_ON_ONCE(!next_cursor ||
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
 * driver registers, so it needs a hook pair; its ops have nothing in common.
 */
int ctlv_op_pre(struct ctlv_op_ctx *ctx)
{
	return 0;
}

void ctlv_op_post(struct ctlv_op_ctx *ctx)
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
		if (!ctlvdev->family)
			return -ENODEV;
		ret = ctlv_op_exec(cfile, filp, &exec, &reply, &reply_len);
	}
	/*
	 * Outside the registration lock, so a copy that faults is not one more
	 * thing an unregistration waits for. An action that returned
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

static int ctlv_fops_open(struct inode *inode, struct file *filp)
{
	struct ctlv_device *ctlvdev =
		container_of(inode->i_cdev, struct ctlv_device, cdev);
	struct ctlv_file *cfile;
	int ret;

	guard(rwsem_read)(&ctlvdev->registration_lock);
	if (!ctlvdev->family)
		return -ENODEV;

	cfile = kzalloc_obj(*cfile, GFP_KERNEL_ACCOUNT);
	if (!cfile)
		return -ENOMEM;
	cfile->ctlvdev = ctlvdev;
	/*
	 * Which commits this description to the device: an open that failed
	 * after this would have to take it back.
	 */
	ret = ctlv_events_file_init(cfile);
	if (ret) {
		kfree(cfile);
		return ret;
	}
	/* An open file description keeps the instance alive by itself. */
	get_device(&ctlvdev->dev);
	filp->private_data = cfile;
	return 0;
}

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
				const struct ctlv_family_schema *leaf)
{
	u32 levels = ctlv_schema_levels(leaf);
	u32 level;
	int ret;

	ret = add_uevent_var(env, "CTLV_FAMILY=%s", leaf->name);
	if (ret)
		return ret;
	for (level = 0; level < levels; level++) {
		const struct ctlv_family_schema *schema =
			ctlv_schema_ancestor(leaf, levels - 1 - level);

		ret = add_uevent_var(env, "CTLV_COMPATIBLE_%u=%s", level,
				     schema->name);
		if (ret)
			return ret;
	}
	return add_uevent_var(env, "CTLV_COMPATIBLE_N=%u", levels);
}

/**
 * ctlv_uevent_check - Whether one chain's names fit the uevents it needs.
 * @leaf: family a device would answer as
 *
 * A chain that does not fit is refused rather than announced short, since a
 * rule matching a truncated chain matches the wrong device.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_uevent_check(const struct ctlv_family_schema *leaf)
{
	struct kobj_uevent_env *env;
	int ret;

	env = kzalloc_obj(*env, GFP_KERNEL);
	if (!env)
		return -ENOMEM;
	ret = ctlv_uevent_families(env, leaf);
	if (!ret && (env->buflen > CTLV_UEVENT_MAX_LEN ||
		     env->envp_idx > CTLV_UEVENT_MAX_VARS)) {
		pr_err("family %s: its chain of names needs %d bytes in %d properties of a uevent, more than the %d in %d it may take\n",
		       leaf->name, env->buflen, env->envp_idx,
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
	return ctlv_uevent_families(env, family->schema);
}

static void ctlv_device_release(struct device *dev)
{
	struct ctlv_device *ctlvdev =
		container_of(dev, struct ctlv_device, dev);

	ida_free(&ctlv_ida, ctlvdev->dev.devt - ctlv_devt);
	bitmap_free(ctlvdev->disabled);
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
}

static struct class ctlv_class = {
	.name = "ctlv",
	.dev_release = ctlv_device_release,
	.dev_uevent = ctlv_dev_uevent,
};

/**
 * ctlv_device_alloc - Allocate one unpublished CTLV device.
 * @parent_dev: device-model parent, the device this one belongs to
 * @family: generated descriptor of the effective family to answer as
 * @priv: family-private pointer, returned to callbacks by ctlv_op_priv()
 *
 * Returns one device answering every op of @family, all enabled and visible to
 * nothing, for the family to configure before registering it. ctlv_device_put()
 * releases the returned reference however registration goes.
 *
 * Return: the device, or an ERR_PTR.
 */
struct ctlv_device *ctlv_device_alloc(struct device *parent_dev,
				      const struct ctlv_family *family,
				      void *priv)
{
	struct ctlv_device *ctlvdev;
	int devnum, ret;

	if (!parent_dev || !family)
		return ERR_PTR(-EINVAL);

	ctlvdev = kzalloc_obj(*ctlvdev, GFP_KERNEL);
	if (!ctlvdev)
		return ERR_PTR(-ENOMEM);
	/* Before anything reads the schema through this device. */
	ret = ctlv_ops_resolve(ctlvdev, family);
	if (ret)
		goto err_free_ops;
	/* What this family's events need of every queue opened on it. */
	ret = ctlv_events_sizes(ctlvdev);
	if (ret)
		goto err_free_ops;
	/* Before a minor is spent on a device that could not be announced. */
	ret = ctlv_uevent_check(family->schema);
	if (ret)
		goto err_free_ops;
	if (ctlvdev->n_dynamic_ops) {
		ctlvdev->disabled = bitmap_zalloc(ctlvdev->n_dynamic_ops,
						  GFP_KERNEL);
		if (!ctlvdev->disabled) {
			ret = -ENOMEM;
			goto err_free_ops;
		}
	}
	devnum = ida_alloc_max(&ctlv_ida, CTLV_MAX_DEVICES - 1, GFP_KERNEL);
	if (devnum < 0) {
		ret = devnum;
		goto err_free_bitmap;
	}

	ctlvdev->family = family;
	ctlvdev->priv = priv;
	init_rwsem(&ctlvdev->registration_lock);
	spin_lock_init(&ctlvdev->transition_lock);
	ctlv_events_device_init(ctlvdev);

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
	if (ret) {
		put_device(&ctlvdev->dev);
		return ERR_PTR(ret);
	}
	return ctlvdev;

err_free_bitmap:
	bitmap_free(ctlvdev->disabled);
err_free_ops:
	kfree(ctlvdev->ops);
	kfree(ctlvdev);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_alloc, "CTLV");

/**
 * ctlv_device_register - Publish one allocated CTLV device.
 * @ctlvdev: device from ctlv_device_alloc(), fully configured
 *
 * On success the device is in sysfs and /dev and its family's callbacks may
 * run. What the device answers is frozen whether or not the rest succeeds, so
 * a failure is released with ctlv_device_put() rather than configured again.
 *
 * Return: 0, or a negative errno.
 */
int ctlv_device_register(struct ctlv_device *ctlvdev)
{
	int ret;

	if (WARN_ON_ONCE(!ctlvdev->family))
		return -ENODEV;
	ret = ctlv_claim_family(ctlvdev->family->schema);
	if (ret)
		return ret;
	ctlv_ops_publish(ctlvdev);
	return cdev_device_add(&ctlvdev->cdev, &ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_register, "CTLV");

/**
 * ctlv_device_unregister - Take one CTLV device away from userspace.
 * @ctlvdev: registered device
 *
 * No family callback runs after this returns. It may return while file
 * descriptors are still open: those answer -ENODEV out of core-owned memory,
 * so the family's module may unload while they do.
 *
 * Does not release the caller's reference; ctlv_device_put() does that.
 */
void ctlv_device_unregister(struct ctlv_device *ctlvdev)
{
	/* No new opens, no discovery entry, and no node to open. */
	cdev_device_del(&ctlvdev->cdev, &ctlvdev->dev);

	/*
	 * The write side waits for the callbacks already admitted, and
	 * clearing what the family owns is what guarantees there are no more.
	 */
	guard(rwsem_write)(&ctlvdev->registration_lock);
	/* Written once, because the availability setter reads it unlocked. */
	WRITE_ONCE(ctlvdev->family, NULL);
	ctlvdev->priv = NULL;
	/* After the family is gone, so that nothing is queued behind this. */
	ctlv_events_device_dead(ctlvdev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_unregister, "CTLV");

/**
 * ctlv_device_put - Release the caller's reference to one CTLV device.
 * @ctlvdev: allocated device
 *
 * Required after a failed registration and a successful unregistration alike.
 * Open descriptions hold their own references, so this may not be the last.
 */
void ctlv_device_put(struct ctlv_device *ctlvdev)
{
	put_device(&ctlvdev->dev);
}
EXPORT_SYMBOL_NS_GPL(ctlv_device_put, "CTLV");

static int __init ctlv_init(void)
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

static void __exit ctlv_exit(void)
{
	class_unregister(&ctlv_class);
	unregister_chrdev_region(ctlv_devt, CTLV_MAX_DEVICES);
	ctlv_claims_free();
	ida_destroy(&ctlv_ida);
}

subsys_initcall(ctlv_init);
module_exit(ctlv_exit);
MODULE_DESCRIPTION("CTLV character device TLV UAPI framework");
MODULE_LICENSE("GPL");
/* A blob attr may name a dma-buf, so the core resolves one itself. */
MODULE_IMPORT_NS("DMA_BUF");
