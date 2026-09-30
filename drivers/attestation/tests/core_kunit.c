// SPDX-License-Identifier: GPL-2.0-only
#include <kunit/device.h>
#include <kunit/static_stub.h>
#include <kunit/test.h>
#include <linux/attestation/ima.h>
#include <linux/cleanup.h>
#include <linux/completion.h>
#include <linux/ctlv.h>
#include <linux/ctlv_gen.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/jiffies.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/srcu.h>

#include "../core/internal.h"

struct attestation_test {
	struct attestation_provider provider;
	struct attestation_mr_caps mr_caps;
	struct attestation_mr_props props;
	enum hash_algo algorithms[2];
	struct attestation_digest values[2];
	struct device *parent;
	struct attestation_dev *att;
	bool registered;
	int read_count;
	int extend_count;
	int extend_error;
	bool all_banks_only;
	bool change_in_extend;
};

/* What a provider driver allocates around its attestation device. */
struct attestation_test_dev {
	struct attestation_dev att;
	struct attestation_test *fixture;
};

static struct attestation_test *attestation_test_from(struct attestation_dev *att)
{
	return container_of(att, struct attestation_test_dev, att)->fixture;
}

static const struct attestation_evidence_format attestation_test_formats[] = {
	{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_QUOTE },
};

static const struct attestation_evidence_challenge_length
attestation_test_challenge_lengths[] = {
	{ .min = 1, .max = 64 },
};

static const struct attestation_evidence_supported_object
attestation_test_object = {
	.evidence_class = ATTESTATION_EVIDENCE_SIGNED,
	.formats = attestation_test_formats,
	.formats_count = ARRAY_SIZE(attestation_test_formats),
	.challenge_lengths = attestation_test_challenge_lengths,
	.challenge_lengths_count = ARRAY_SIZE(attestation_test_challenge_lengths),
	.max_length = 64,
};

static const struct attestation_evidence_caps attestation_test_evidence_caps = {
	.objects = &attestation_test_object,
	.objects_count = 1,
};

static int
attestation_test_mr_read(struct attestation_dev *att, unsigned int mr_id,
			 const struct ctlv_cursor *mr_options,
			 struct attestation_digest_set *values)
{
	struct attestation_test *fixture = attestation_test_from(att);

	fixture->read_count++;
	if (mr_id != fixture->props.id ||
	    values->values_count != fixture->props.algorithms_count)
		return -EINVAL;
	memcpy(values->values, fixture->values,
	       values->values_count * sizeof(*values->values));
	return 0;
}

static int attestation_test_bank(struct attestation_test *fixture,
				 enum hash_algo algorithm)
{
	size_t bank;

	for (bank = 0; bank < fixture->props.algorithms_count; bank++)
		if (fixture->algorithms[bank] == algorithm)
			return bank;
	return -EINVAL;
}

static int
attestation_test_mr_extend(struct attestation_dev *att, unsigned int mr_id,
			   const struct ctlv_cursor *mr_options,
			   const struct attestation_digest_set *values)
{
	struct attestation_test *fixture = attestation_test_from(att);
	size_t index;
	int bank;

	fixture->extend_count++;
	if (fixture->change_in_extend)
		attestation_state_changed_locked(att);
	if (fixture->extend_error)
		return fixture->extend_error;
	if (mr_id != fixture->props.id)
		return -EINVAL;
	if (fixture->all_banks_only &&
	    values->values_count != fixture->props.algorithms_count)
		return -EOPNOTSUPP;
	for (index = 0; index < values->values_count; index++) {
		bank = attestation_test_bank(fixture,
					     values->values[index].algorithm);
		if (bank < 0)
			return bank;
		fixture->values[bank] = values->values[index];
	}
	return 0;
}

static int
attestation_test_evidence_get(
	struct attestation_dev *att,
	const struct attestation_evidence_request *request,
	struct attestation_evidence_result *result)
{
	return 0;
}

static int attestation_test_evidence_accept(struct attestation_dev *att)
{
	return 0;
}

static void attestation_test_destroy(void *data)
{
	struct attestation_test *fixture = data;

	if (fixture->registered)
		attestation_unregister(fixture->att);
	fixture->registered = false;
	attestation_put(fixture->att);
	fixture->att = NULL;
}

static void attestation_test_provider_init(struct attestation_test *fixture)
{
	fixture->algorithms[0] = HASH_ALGO_SHA256;
	fixture->props = (struct attestation_mr_props) {
		.id = 7,
		.name = "kunit-mr",
		.algorithms = fixture->algorithms,
		.algorithms_count = 1,
		.extendable = true,
	};
	fixture->values[0].algorithm = HASH_ALGO_SHA256;
	fixture->provider = (struct attestation_provider) {
		.evidence_get = attestation_test_evidence_get,
		.evidence_accept = attestation_test_evidence_accept,
		.mr_read = attestation_test_mr_read,
		.mr_extend = attestation_test_mr_extend,
		.evidence_caps = &attestation_test_evidence_caps,
		.mr_caps = &fixture->mr_caps,
		.mrs = &fixture->props,
		.mrs_count = 1,
	};
}

static struct attestation_test_dev *
attestation_test_alloc(struct attestation_test *fixture,
		       const struct ctlv_family *family)
{
	struct attestation_test_dev *tdev;

	tdev = attestation_alloc(fixture->parent, &fixture->provider, family,
				 struct attestation_test_dev, att);
	if (!IS_ERR(tdev))
		tdev->fixture = fixture;
	return tdev;
}

static int attestation_test_register(struct kunit *test,
				     struct attestation_test *fixture,
				     const char *name)
{
	struct attestation_test_dev *tdev;
	int ret;

	fixture->parent = kunit_device_register(test, name);
	if (IS_ERR(fixture->parent))
		return PTR_ERR(fixture->parent);
	tdev = attestation_test_alloc(fixture, &ctlv_attestation_family);
	if (IS_ERR(tdev))
		return PTR_ERR(tdev);
	ret = attestation_register(&tdev->att);
	if (ret) {
		attestation_put(&tdev->att);
		return ret;
	}
	fixture->att = &tdev->att;
	fixture->registered = true;
	return kunit_add_action_or_reset(
		test, attestation_test_destroy, fixture);
}

static int attestation_test_create(struct kunit *test,
				   struct attestation_test *fixture,
				   const char *name)
{
	attestation_test_provider_init(fixture);
	return attestation_test_register(test, fixture, name);
}

/* The error a provider's device is refused with, if it is. */
static long attestation_test_alloc_error(struct attestation_test *fixture,
					 const struct ctlv_family *family)
{
	struct attestation_test_dev *tdev;

	tdev = attestation_test_alloc(fixture, family);
	if (IS_ERR(tdev))
		return PTR_ERR(tdev);
	attestation_put(&tdev->att);
	return 0;
}

static int attestation_test_init(struct kunit *test)
{
	struct attestation_test *fixture;

	fixture = kunit_kzalloc(test, sizeof(*fixture), GFP_KERNEL);
	if (!fixture)
		return -ENOMEM;
	test->priv = fixture;
	return 0;
}

static void attestation_test_provider_contracts(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_test_dev *tdev;

	attestation_test_provider_init(fixture);
	fixture->parent = kunit_device_register(test, "attestation-contract");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fixture->parent);
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_family),
			-EINVAL);

	fixture->provider.evidence_get = NULL;
	fixture->provider.evidence_caps = NULL;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);

	/* Required with MRs, even before there is one. */
	attestation_test_provider_init(fixture);
	fixture->provider.mr_read = NULL;
	fixture->provider.mrs = NULL;
	fixture->provider.mrs_count = 0;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);

	attestation_test_provider_init(fixture);
	fixture->algorithms[1] = HASH_ALGO_SHA256;
	fixture->props.algorithms_count = 2;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);

	/* Refused only once the driver had its chance to add dynamic MRs. */
	attestation_test_provider_init(fixture);
	fixture->provider.mrs = NULL;
	fixture->provider.mrs_count = 0;
	tdev = attestation_test_alloc(fixture, &ctlv_attestation_family);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, tdev);
	KUNIT_EXPECT_EQ(test, attestation_register(&tdev->att), -EINVAL);
	attestation_put(&tdev->att);
}

static void attestation_test_challenge_contracts(struct kunit *test)
{
	static const struct {
		struct attestation_evidence_challenge_length lengths[3];
		size_t count;
		int error;
	} cases[] = {
		{ { { 32, 32 }, { 48, 64 }, { 128, 128 } }, 3, 0 },
		{ { { 64, 32 } }, 1, -EINVAL },
		{ { { 64, 64 }, { 32, 32 } }, 2, -EINVAL },
		{ { { 1, 32 }, { 32, 64 } }, 2, -EINVAL },
		/* A signed object takes a challenge, never empty nor over 4096. */
		{ { { 0, 32 } }, 1, -EINVAL },
		{ { {} }, 0, -EINVAL },
		{ { { 4096, 4097 } }, 1, -EINVAL },
	};
	struct attestation_test *fixture = test->priv;
	struct attestation_evidence_supported_object object;
	struct attestation_evidence_caps caps;
	size_t index;
	long ret;

	attestation_test_provider_init(fixture);
	fixture->parent = kunit_device_register(test, "attestation-challenge");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fixture->parent);
	object = attestation_test_object;
	caps = attestation_test_evidence_caps;
	caps.objects = &object;
	fixture->provider.evidence_caps = &caps;
	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		object.challenge_lengths = cases[index].lengths;
		object.challenge_lengths_count = cases[index].count;
		ret = attestation_test_alloc_error(fixture, &ctlv_attestation_family);
		KUNIT_EXPECT_EQ_MSG(test, ret, cases[index].error, "case %zu", index);
	}

	/* No other object takes one. */
	object.evidence_class = ATTESTATION_EVIDENCE_EVENT_LOG;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);
	object.challenge_lengths_count = 0;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family), 0);
}

static void attestation_test_object_contracts(struct kunit *test)
{
	static const struct attestation_evidence_format standard[] = {
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_SPDM_CERT_CHAIN },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_QUOTE },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EVENT_LOG },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EK_CERT },
	};
	static const struct attestation_evidence_format repeated[] = {
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_QUOTE },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_QUOTE },
	};
	/* ctlv-attestation gives evidence-vendor-format no value. */
	static const struct attestation_evidence_format unknown[] = {
		{ .value = 0 },
		{ .value = CTLV_ATTESTATION_EVIDENCE_FORMAT_TPM_EK_CERT + 1 },
		{ .vendor = true, .value = 1 },
	};
	static const struct {
		const struct attestation_evidence_format *formats;
		size_t count;
		int error;
	} cases[] = {
		{ standard, 4, 0 },
		{ standard, 5, -EINVAL },
		{ standard, 0, -EINVAL },
		{ repeated, 2, -EINVAL },
		{ &unknown[0], 1, -EINVAL },
		{ &unknown[1], 1, -EINVAL },
		{ &unknown[2], 1, -EINVAL },
	};
	struct attestation_evidence_supported_object *objects;
	struct attestation_test *fixture = test->priv;
	struct attestation_evidence_caps caps;
	size_t index;
	long ret;

	attestation_test_provider_init(fixture);
	fixture->parent = kunit_device_register(test, "attestation-objects");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fixture->parent);
	objects = kunit_kcalloc(test, 17, sizeof(*objects), GFP_KERNEL);
	KUNIT_ASSERT_NOT_NULL(test, objects);
	for (index = 0; index < 17; index++)
		objects[index] = attestation_test_object;
	caps = attestation_test_evidence_caps;
	caps.objects = objects;
	fixture->provider.evidence_caps = &caps;

	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		objects[0].formats = cases[index].formats;
		objects[0].formats_count = cases[index].count;
		ret = attestation_test_alloc_error(fixture, &ctlv_attestation_family);
		KUNIT_EXPECT_EQ_MSG(test, ret, cases[index].error, "case %zu", index);
	}

	objects[0] = attestation_test_object;
	objects[0].challenge_lengths_count = 0;
	objects[0].evidence_class = 0;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);
	objects[0].evidence_class = ATTESTATION_EVIDENCE_AUXILIARY + 1;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);

	objects[0] = attestation_test_object;
	objects[0].has_tcb_digest = true;
	objects[0].tcb_digest_algorithm = HASH_ALGO__LAST;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);
	objects[0].tcb_digest_algorithm = HASH_ALGO_SHA384;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family), 0);

	/* Sixteen are as many as the IDs number. */
	caps.objects_count = 16;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family), 0);
	caps.objects_count = 17;
	KUNIT_EXPECT_EQ(test, attestation_test_alloc_error(fixture, &ctlv_attestation_family),
			-EINVAL);
}

static void attestation_test_initial_journal(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_mr_target *target;
	enum hash_algo *algorithms;
	char *family_name;
	char *device_name;
	char *bus_name;
	char **family = &family_name;
	char **device = &device_name;
	char **bus = &bus_name;
	size_t count;
	int ret;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-initial"), 0);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 1U);
	target = attestation_mr_target_get(
		"ctlv-attestation", NULL, NULL, fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	KUNIT_EXPECT_EQ(test,
			attestation_mr_target_start_generation(target), 1U);
	algorithms = attestation_mr_target_algorithms(target, &count);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, algorithms);
	KUNIT_EXPECT_EQ(test, count, 1);
	KUNIT_EXPECT_EQ(test, algorithms[0], HASH_ALGO_SHA256);
	KUNIT_EXPECT_EQ(test,
			attestation_mr_target_handover_restored(target), 0);
	if (dev_bus_name(fixture->parent)) {
		ret = attestation_mr_target_identity(target, family, bus,
						     device);
		KUNIT_ASSERT_EQ(test, ret, 0);
		KUNIT_EXPECT_STREQ(test, family_name, "ctlv-attestation");
		KUNIT_EXPECT_STREQ(test, bus_name,
				   dev_bus_name(fixture->parent));
		KUNIT_EXPECT_STREQ(test, device_name,
				   dev_name(fixture->parent));
		kfree(family_name);
		kfree(bus_name);
		kfree(device_name);
	}
	kfree(algorithms);
	attestation_mr_target_put(target);
}

static void attestation_test_target_extension(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest value = {
		.algorithm = HASH_ALGO_SHA256,
		.data = { 0xa5 },
	};
	struct attestation_digest_set values = {
		.values_count = 1,
		.values = &value,
	};
	const struct attestation_mr_log_entry *entry;
	struct attestation_mr_target *target;
	unsigned int generation = 0;
	const u8 *template;
	size_t length;
	int srcu_index;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-extend"), 0);
	target = attestation_mr_target_get(
		"ctlv-attestation", NULL, NULL, fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	KUNIT_ASSERT_EQ(test, attestation_mr_target_extend(
				target, &values, "ima-ng",
				(const u8 *)"template", 8, false), 0);
	KUNIT_EXPECT_EQ(test, fixture->extend_count, 1);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 3U);

	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	attestation_mr_log_for_each_entry_srcu(
		entry, target, generation, fixture->att->log->generation) {
		generation = attestation_mr_log_entry_generation(entry);
		if (generation != 3)
			continue;
		KUNIT_EXPECT_EQ(test,
				attestation_mr_log_entry_event(entry),
			CTLV_ATTESTATION_MR_LOG_EVENT_EXTEND);
		template = attestation_mr_log_entry_template_data(
			entry, &length);
		KUNIT_EXPECT_EQ(test, length, 8);
		KUNIT_EXPECT_MEMEQ(test, template, "template", 8);
	}
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	KUNIT_EXPECT_EQ(test, generation, 3U);
	attestation_mr_target_put(target);
}

static void attestation_test_failed_extension(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest value = {
		.algorithm = HASH_ALGO_SHA256,
	};
	struct attestation_digest_set values = {
		.values_count = 1,
		.values = &value,
	};
	struct attestation_mr_target *target;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-failure"), 0);
	target = attestation_mr_target_get(
		"ctlv-attestation", NULL, NULL, fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	fixture->extend_error = -EIO;
	KUNIT_EXPECT_EQ(test, attestation_mr_target_extend(
			target, &values, "ima-ng", NULL, 0, false), -EIO);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 2U);
	attestation_mr_target_put(target);
}

static void attestation_test_state_change(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-state"), 0);
	mutex_lock(&fixture->att->lock);
	fixture->att->evidence_generation = 1;
	mutex_unlock(&fixture->att->lock);
	attestation_state_changed(fixture->att);
	KUNIT_EXPECT_EQ(test, fixture->att->device_generation, 1U);
	KUNIT_EXPECT_EQ(test, fixture->att->evidence_floor, 1U);
}

/* The extend callback runs under the instance mutex and reports from there. */
static void attestation_test_state_change_locked(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest value = {
		.algorithm = HASH_ALGO_SHA256,
	};
	struct attestation_digest_set values = {
		.values_count = 1,
		.values = &value,
	};
	struct attestation_mr_log_entry *entry;
	struct attestation_mr_target *target;
	int ret;

	ret = attestation_test_create(test, fixture, "attestation-state-locked");
	KUNIT_ASSERT_EQ(test, ret, 0);
	target = attestation_mr_target_get("ctlv-attestation", NULL, NULL, fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	fixture->change_in_extend = true;
	ret = attestation_mr_target_extend(target, &values, "ima-ng", NULL, 0, false);
	KUNIT_EXPECT_EQ(test, ret, 0);
	attestation_mr_target_put(target);
	KUNIT_EXPECT_EQ(test, fixture->att->device_generation, 1U);
	entry = xa_load(&fixture->att->log->entries, 3);
	KUNIT_ASSERT_NOT_NULL(test, entry);
	KUNIT_EXPECT_EQ(test, entry->device_generation, 1U);
}

static void attestation_test_dynamic_topology(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_mr_props props;
	struct attestation_mr *mr;

	fixture->mr_caps.supports_dynamic_topology = true;
	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-topology"), 0);
	props = fixture->props;
	props.id = 9;
	props.name = "dynamic";
	mr = attestation_mr_create(fixture->att, &props);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mr);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 2U);
	attestation_mr_destroy(mr);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 3U);
}

static void attestation_test_stale_target(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest value = {
		.algorithm = HASH_ALGO_SHA256,
	};
	struct attestation_digest_set values = {
		.values_count = 1,
		.values = &value,
	};
	struct attestation_mr_props props;
	struct attestation_mr_target *target;
	struct attestation_mr *mr;

	fixture->mr_caps.supports_dynamic_topology = true;
	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-stale"), 0);
	props = fixture->props;
	props.id = 9;
	props.name = "dynamic";
	mr = attestation_mr_create(fixture->att, &props);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, mr);
	target = attestation_mr_target_get(
		"ctlv-attestation", NULL, NULL, props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	attestation_mr_destroy(mr);
	KUNIT_EXPECT_EQ(test, attestation_mr_target_extend(
			target, &values, "ima-ng", NULL, 0, false), -ENODEV);
	attestation_mr_target_put(target);
}

static void attestation_test_generation_overflow(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_mr_props props;
	struct attestation_mr_target *target;
	struct attestation_mr *mr;

	fixture->mr_caps.supports_dynamic_topology = true;
	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-overflow"), 0);
	props = fixture->props;
	mutex_lock(&fixture->att->lock);
	fixture->att->log->generation = UINT_MAX;
	mutex_unlock(&fixture->att->lock);
	props.id = 9;
	props.name = "overflow";
	mr = attestation_mr_create(fixture->att, &props);
	KUNIT_EXPECT_EQ(test, PTR_ERR(mr), -EOVERFLOW);
	target = attestation_mr_target_get(
		"ctlv-attestation", NULL, NULL, fixture->props.id);
	KUNIT_EXPECT_EQ(test, PTR_ERR(target), -ENODEV);
}

static struct attestation_mr_log_entry_info *
attestation_test_info_nomem(const struct attestation_mr *mr)
{
	return ERR_PTR(-ENOMEM);
}

static void attestation_test_allocation_failure(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_mr_props props;
	struct attestation_mr *mr;

	fixture->mr_caps.supports_dynamic_topology = true;
	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-nomem"), 0);
	props = fixture->props;
	props.id = 9;
	props.name = "allocation-failure";
	kunit_activate_static_stub(test, attestation_mr_info_alloc,
				   attestation_test_info_nomem);
	mr = attestation_mr_create(fixture->att, &props);
	kunit_deactivate_static_stub(test, attestation_mr_info_alloc);
	KUNIT_EXPECT_EQ(test, PTR_ERR(mr), -ENOMEM);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 1U);
	KUNIT_EXPECT_NULL(test, xa_load(&fixture->att->mrs, props.id));
}

static struct ctlv_op_ctx *
attestation_test_event_nomem(struct ctlv *ctlv, u64 op_id,
			     struct ctlv_cursor **payload, gfp_t gfp)
{
	return ERR_PTR(-ENOMEM);
}

static void attestation_test_event_allocation_failure(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-event-nomem"), 0);
	kunit_activate_static_stub(test, ctlv_event_create,
				   attestation_test_event_nomem);
	attestation_state_changed(fixture->att);
	kunit_deactivate_static_stub(test, ctlv_event_create);
	KUNIT_EXPECT_EQ(test, fixture->att->device_generation, 1U);
}

struct attestation_teardown_race {
	struct attestation_test *fixture;
	struct kunit *test;
	struct completion done;
};

static int attestation_test_destroy_thread(void *arg)
{
	struct attestation_teardown_race *race = arg;

	current->kunit_test = race->test;
	attestation_unregister(race->fixture->att);
	race->fixture->registered = false;
	current->kunit_test = NULL;
	complete(&race->done);
	/* kthread_stop() still needs this task. */
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (kthread_should_stop())
			break;
		schedule();
	}
	__set_current_state(TASK_RUNNING);
	return 0;
}

static void attestation_test_teardown_waits_for_srcu(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_teardown_race race = {
		.fixture = fixture,
		.test = test,
	};
	struct task_struct *task;
	int srcu_index;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-teardown"), 0);
	init_completion(&race.done);
	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	task = kthread_run(attestation_test_destroy_thread, &race,
			   "attestation-destroy");
	if (IS_ERR(task)) {
		srcu_read_unlock(&attestation_instances_srcu, srcu_index);
		KUNIT_FAIL(test, "failed to start destroy thread");
		return;
	}
	KUNIT_EXPECT_EQ(test, wait_for_completion_timeout(&race.done, 1), 0UL);
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	KUNIT_EXPECT_NE(test, wait_for_completion_timeout(&race.done, HZ), 0UL);
	kthread_stop(task);
}

static void attestation_test_kexec_roundtrip(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;

	KUNIT_ASSERT_EQ(test, attestation_test_create(
				test, fixture, "attestation-kexec"), 0);
	if (!dev_bus_name(fixture->parent))
		kunit_skip(test, "KUnit parent has no stable bus name");
	KUNIT_EXPECT_EQ(test,
			attestation_kexec_test_roundtrip(fixture->att), 0);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 1U);
	KUNIT_EXPECT_GT(test, fixture->read_count, 0);
}

static void attestation_test_kexec_ima_roundtrip(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest value = {
		.algorithm = HASH_ALGO_SHA256,
		.data = { 0xa5 },
	};
	struct attestation_digest_set values = {
		.values_count = 1,
		.values = &value,
	};
	const struct attestation_mr_log_entry *entry;
	struct attestation_mr_target *target;
	unsigned int generation = 0;
	unsigned int end_generation;
	const char *family = "ctlv-attestation";
	const char *template_name;
	const u8 *template_data;
	size_t length;
	int srcu_index;
	int ret;
	bool found = false;
	bool violation;

	ret = attestation_test_create(test, fixture,
				      "attestation-kexec-ima");
	KUNIT_ASSERT_EQ(test, ret, 0);
	if (!dev_bus_name(fixture->parent))
		kunit_skip(test, "KUnit parent has no stable bus name");
	target = attestation_mr_target_get(family, NULL, NULL,
					   fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	ret = attestation_mr_target_extend(target, &values, "ima-ng",
					   (const u8 *)"kexec-template",
					   14, true);
	KUNIT_ASSERT_EQ(test, ret, 0);
	KUNIT_ASSERT_EQ(test,
			attestation_kexec_test_roundtrip(fixture->att), 0);

	srcu_index = srcu_read_lock(&attestation_instances_srcu);
	end_generation = fixture->att->log->generation;
	entry = attestation_mr_log_iter_seek(target, generation,
					     end_generation);
	while (!IS_ERR_OR_NULL(entry)) {
		generation = attestation_mr_log_entry_generation(entry);
		template_name =
			attestation_mr_log_entry_template_name(entry);
		if (template_name) {
			KUNIT_EXPECT_STREQ(test, template_name, "ima-ng");
			violation =
				attestation_mr_log_entry_violation(entry);
			KUNIT_EXPECT_TRUE(test, violation);
			template_data =
				attestation_mr_log_entry_template_data(entry,
								       &length);
			KUNIT_EXPECT_EQ(test, length, 14);
			KUNIT_EXPECT_MEMEQ(test, template_data,
					   "kexec-template", 14);
			found = true;
		}
		entry = attestation_mr_log_iter_next(entry, end_generation);
	}
	srcu_read_unlock(&attestation_instances_srcu, srcu_index);
	KUNIT_EXPECT_TRUE(test, found);
	attestation_mr_target_put(target);
}

static void attestation_test_bank_order(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest values[] = {
		{ .algorithm = HASH_ALGO_SHA384, .data = { 0x38 } },
		{ .algorithm = HASH_ALGO_SHA256, .data = { 0x25 } },
	};
	struct attestation_digest_set set = {
		.values_count = ARRAY_SIZE(values),
		.values = values,
	};
	struct attestation_mr_target *target;
	enum hash_algo *algorithms;
	size_t count;
	int ret;

	attestation_test_provider_init(fixture);
	fixture->algorithms[0] = HASH_ALGO_SHA384;
	fixture->algorithms[1] = HASH_ALGO_SHA256;
	fixture->props.algorithms_count = 2;
	fixture->values[0].algorithm = HASH_ALGO_SHA384;
	fixture->values[1].algorithm = HASH_ALGO_SHA256;
	ret = attestation_test_register(test, fixture, "attestation-order");
	KUNIT_ASSERT_EQ(test, ret, 0);
	if (!dev_bus_name(fixture->parent))
		kunit_skip(test, "KUnit parent has no stable bus name");
	target = attestation_mr_target_get("ctlv-attestation", NULL, NULL,
					   fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	algorithms = attestation_mr_target_algorithms(target, &count);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, algorithms);
	KUNIT_EXPECT_EQ(test, count, 2);
	KUNIT_EXPECT_EQ(test, algorithms[0], HASH_ALGO_SHA384);
	kfree(algorithms);
	ret = attestation_mr_target_extend(target, &set, "ima-ng", NULL, 0,
					   false);
	KUNIT_EXPECT_EQ(test, ret, 0);
	attestation_mr_target_put(target);
	KUNIT_EXPECT_MEMEQ(test, fixture->values, values, sizeof(values));
	KUNIT_EXPECT_EQ(test,
			attestation_kexec_test_roundtrip(fixture->att), 0);
}

/* What mr-extend does once it has decoded the request. */
static int attestation_test_extend(struct attestation_test *fixture,
				   const struct attestation_digest_set *values)
{
	struct attestation_dev *att = fixture->att;
	const struct attestation_mr_log_entry *entry;
	struct attestation_mr_change *change;
	struct attestation_mr *mr;
	int ret;

	guard(mutex)(&att->lock);
	mr = xa_load(&att->mrs, fixture->props.id);
	ret = attestation_mr_values_validate(mr->props, values, false);
	if (ret)
		return ret;
	change = attestation_mr_change_prepare(att, mr, values, NULL, NULL, 0);
	if (IS_ERR(change))
		return PTR_ERR(change);
	entry = attestation_mr_extend_locked(att, change, NULL);
	attestation_mr_change_free(change);
	return PTR_ERR_OR_ZERO(entry);
}

static size_t attestation_test_entry_values(struct attestation_test *fixture,
					    unsigned int generation)
{
	struct attestation_mr_log_entry *entry;

	entry = xa_load(&fixture->att->log->entries, generation);
	if (!entry || entry->type != ATTESTATION_MR_LOG_ENTRY_EXTEND)
		return 0;
	return container_of(entry, struct attestation_mr_log_entry_extend,
			    entry)->values.values_count;
}

static void attestation_test_bank_subsets(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_digest values[] = {
		{ .algorithm = HASH_ALGO_SHA256, .data = { 0x25 } },
		{ .algorithm = HASH_ALGO_SHA384, .data = { 0x38 } },
		{ .algorithm = HASH_ALGO_SHA256 },
		{ .algorithm = HASH_ALGO_SHA256 },
		{ .algorithm = HASH_ALGO_SHA512 },
	};
	/* Sets of @count values from @first on. */
	static const struct {
		size_t first, count;
		bool all_banks;
		int error;
	} cases[] = {
		{ 0, 2, true, 0 },
		{ 0, 1, false, 0 },
		{ 1, 1, false, 0 },
		{ 1, 1, true, -EINVAL },
		{ 0, 0, false, -EINVAL },
		{ 1, 2, false, -EINVAL },
		{ 2, 2, false, -EINVAL },
		{ 4, 1, false, -EINVAL },
	};
	struct attestation_digest_set set = { .values = &values[1] };
	const struct attestation_mr_props *props = &fixture->props;
	struct attestation_digest before[2];
	struct attestation_mr_target *target;
	size_t index;
	int ret;

	attestation_test_provider_init(fixture);
	fixture->algorithms[1] = HASH_ALGO_SHA384;
	fixture->props.algorithms_count = 2;
	fixture->values[1].algorithm = HASH_ALGO_SHA384;
	ret = attestation_test_register(test, fixture, "attestation-subsets");
	KUNIT_ASSERT_EQ(test, ret, 0);

	for (index = 0; index < ARRAY_SIZE(cases); index++) {
		struct attestation_digest_set check = {
			.values = &values[cases[index].first],
			.values_count = cases[index].count,
		};

		ret = attestation_mr_values_validate(props, &check,
						     cases[index].all_banks);
		KUNIT_EXPECT_EQ_MSG(test, ret, cases[index].error, "case %zu",
				    index);
	}

	set.values_count = 1;
	KUNIT_EXPECT_EQ(test, attestation_test_extend(fixture, &set), 0);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 2U);
	KUNIT_EXPECT_EQ(test, attestation_test_entry_values(fixture, 2), 1U);
	KUNIT_EXPECT_EQ(test, fixture->values[0].data[0], 0);
	KUNIT_EXPECT_EQ(test, fixture->values[1].data[0], 0x38);

	fixture->all_banks_only = true;
	memcpy(before, fixture->values, sizeof(before));
	KUNIT_EXPECT_EQ(test, attestation_test_extend(fixture, &set),
			-EOPNOTSUPP);
	KUNIT_EXPECT_EQ(test, fixture->extend_count, 2);
	KUNIT_EXPECT_EQ(test, fixture->att->log->generation, 2U);
	KUNIT_EXPECT_MEMEQ(test, fixture->values, before, sizeof(before));

	/* A handover keeps the entry a subset. */
	if (dev_bus_name(fixture->parent)) {
		ret = attestation_kexec_test_roundtrip(fixture->att);
		KUNIT_EXPECT_EQ(test, ret, 0);
		KUNIT_EXPECT_EQ(test, attestation_test_entry_values(fixture, 2),
				1U);
	}

	target = attestation_mr_target_get("ctlv-attestation", NULL, NULL,
					   fixture->props.id);
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, target);
	ret = attestation_mr_target_extend(target, &set, "ima-ng", NULL, 0,
					   false);
	KUNIT_EXPECT_EQ(test, ret, -EINVAL);
	attestation_mr_target_put(target);
}

static struct kunit_case attestation_test_cases[] = {
	KUNIT_CASE(attestation_test_provider_contracts),
	KUNIT_CASE(attestation_test_challenge_contracts),
	KUNIT_CASE(attestation_test_object_contracts),
	KUNIT_CASE(attestation_test_initial_journal),
	KUNIT_CASE(attestation_test_target_extension),
	KUNIT_CASE(attestation_test_failed_extension),
	KUNIT_CASE(attestation_test_state_change),
	KUNIT_CASE(attestation_test_state_change_locked),
	KUNIT_CASE(attestation_test_dynamic_topology),
	KUNIT_CASE(attestation_test_stale_target),
	KUNIT_CASE(attestation_test_generation_overflow),
	KUNIT_CASE(attestation_test_allocation_failure),
	KUNIT_CASE(attestation_test_event_allocation_failure),
	KUNIT_CASE(attestation_test_teardown_waits_for_srcu),
	KUNIT_CASE(attestation_test_kexec_roundtrip),
	KUNIT_CASE(attestation_test_kexec_ima_roundtrip),
	KUNIT_CASE(attestation_test_bank_order),
	KUNIT_CASE(attestation_test_bank_subsets),
	{}
};

static struct kunit_suite attestation_test_suite = {
	.name = "attestation",
	.init = attestation_test_init,
	.test_cases = attestation_test_cases,
};

kunit_test_suite(attestation_test_suite);

MODULE_DESCRIPTION("Attestation core KUnit tests");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("ATTESTATION");
MODULE_IMPORT_NS("CTLV");
MODULE_IMPORT_NS("EXPORTED_FOR_KUNIT_TESTING");
