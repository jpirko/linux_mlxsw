// SPDX-License-Identifier: GPL-2.0-only
#include <kunit/device.h>
#include <kunit/static_stub.h>
#include <kunit/test.h>
#include <linux/attestation/ima.h>
#include <linux/completion.h>
#include <linux/ctlv.h>
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
	struct ctlv_device *ctlvdev;
	struct device *parent;
	struct attestation *att;
	int read_count;
	int extend_count;
	int clear_count;
	int extend_error;
};

static const enum attestation_evidence_representation
attestation_test_representations[] = {
	ATTESTATION_EVIDENCE_REPRESENTATION_RAW,
};

static const struct attestation_evidence_supported_object
attestation_test_object = {
	.blob_class = ATTESTATION_EVIDENCE_BLOB_PRIMARY,
	.format = "attestation-kunit-v1",
	.representations = attestation_test_representations,
	.representations_count = ARRAY_SIZE(attestation_test_representations),
	.max_length = 64,
};

static const struct attestation_evidence_caps attestation_test_evidence_caps = {
	.max_challenge_length = 64,
	.max_total_output_length = 64,
	.objects = &attestation_test_object,
	.objects_count = 1,
};

static int
attestation_test_mr_read(struct attestation *att, unsigned int mr_id,
			 const struct ctlv_cursor *mr_options,
			 struct attestation_digest_set *values)
{
	struct kunit *test = kunit_get_current_test();
	struct attestation_test *fixture = test->priv;

	fixture->read_count++;
	if (mr_id != fixture->props.id ||
	    values->values_count != fixture->props.algorithms_count)
		return -EINVAL;
	memcpy(values->values, fixture->values,
	       values->values_count * sizeof(*values->values));
	return 0;
}

static int
attestation_test_mr_extend(struct attestation *att, unsigned int mr_id,
			   const struct ctlv_cursor *mr_options,
			   const struct attestation_digest_set *values)
{
	struct kunit *test = kunit_get_current_test();
	struct attestation_test *fixture = test->priv;

	fixture->extend_count++;
	if (fixture->extend_error)
		return fixture->extend_error;
	if (mr_id != fixture->props.id ||
	    values->values_count != fixture->props.algorithms_count)
		return -EINVAL;
	memcpy(fixture->values, values->values,
	       values->values_count * sizeof(*values->values));
	return 0;
}

static int
attestation_test_evidence_get(
	struct attestation *att,
	const struct attestation_evidence_request *request,
	struct attestation_evidence_result *result)
{
	return 0;
}

static int attestation_test_evidence_accept(struct attestation *att)
{
	return 0;
}

static void attestation_test_evidence_clear(struct attestation *att)
{
	struct kunit *test = kunit_get_current_test();
	struct attestation_test *fixture = test->priv;

	fixture->clear_count++;
}

static void attestation_test_destroy(void *data)
{
	struct attestation_test *fixture = data;

	if (fixture->ctlvdev)
		ctlv_destroy(fixture->ctlvdev);
	fixture->ctlvdev = NULL;
}

static void attestation_test_provider_init(struct attestation_test *fixture)
{
	fixture->algorithms[0] = HASH_ALGO_SHA256;
	fixture->props = (struct attestation_mr_props) {
		.id = 7,
		.name = "kunit-mr",
		.algorithms = fixture->algorithms,
		.algorithms_count = 1,
		.readable = true,
		.extendable = true,
	};
	fixture->values[0].algorithm = HASH_ALGO_SHA256;
	fixture->provider = (struct attestation_provider) {
		.evidence_get = attestation_test_evidence_get,
		.evidence_accept = attestation_test_evidence_accept,
		.evidence_accept_clear = attestation_test_evidence_clear,
		.mr_read = attestation_test_mr_read,
		.mr_extend = attestation_test_mr_extend,
		.evidence_caps = &attestation_test_evidence_caps,
		.mr_caps = &fixture->mr_caps,
		.mrs = &fixture->props,
		.mrs_count = 1,
	};
}

static int attestation_test_create(struct kunit *test,
				   struct attestation_test *fixture,
				   const char *name)
{
	attestation_test_provider_init(fixture);
	fixture->parent = kunit_device_register(test, name);
	if (IS_ERR(fixture->parent))
		return PTR_ERR(fixture->parent);
	fixture->ctlvdev = ctlv_create(
		fixture->parent, &ctlv_attestation_family,
		&fixture->provider);
	if (IS_ERR(fixture->ctlvdev)) {
		int ret = PTR_ERR(fixture->ctlvdev);

		fixture->ctlvdev = NULL;
		return ret;
	}
	fixture->att = attestation_test_find_device(fixture->ctlvdev);
	if (!fixture->att) {
		attestation_test_destroy(fixture);
		return -ENODEV;
	}
	return kunit_add_action_or_reset(
		test, attestation_test_destroy, fixture);
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
	struct ctlv_device *created;

	attestation_test_provider_init(fixture);
	fixture->parent = kunit_device_register(test, "attestation-contract");
	KUNIT_ASSERT_NOT_ERR_OR_NULL(test, fixture->parent);

	fixture->provider.evidence_accept_clear = NULL;
	created = ctlv_create(fixture->parent,
			      &ctlv_attestation_family,
			      &fixture->provider);
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);

	attestation_test_provider_init(fixture);
	fixture->provider.mr_read = NULL;
	created = ctlv_create(fixture->parent,
			      &ctlv_attestation_family,
			      &fixture->provider);
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);

	attestation_test_provider_init(fixture);
	fixture->provider.mrs = NULL;
	fixture->provider.mrs_count = 0;
	created = ctlv_create(fixture->parent,
			      &ctlv_attestation_family,
			      &fixture->provider);
	KUNIT_EXPECT_EQ(test, PTR_ERR(created), -EINVAL);
}

static void attestation_test_initial_journal(struct kunit *test)
{
	struct attestation_test *fixture = test->priv;
	struct attestation_mr_target *target;
	enum hash_algo *algorithms;
	size_t count;

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
	fixture->att->verdict_recorded = true;
	fixture->att->evidence_generation = 1;
	fixture->att->accepted_evidence_generation = 1;
	mutex_unlock(&fixture->att->lock);
	attestation_state_changed(fixture->att);
	KUNIT_EXPECT_EQ(test, fixture->att->device_generation, 1U);
	KUNIT_EXPECT_EQ(test, fixture->att->evidence_floor, 1U);
	KUNIT_EXPECT_FALSE(test, fixture->att->verdict_recorded);
	KUNIT_EXPECT_EQ(test, fixture->clear_count, 1);
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
attestation_test_event_nomem(struct ctlv_device *ctlvdev, u64 op_id,
			     struct ctlv_cursor **reply, gfp_t gfp)
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
	ctlv_destroy(race->fixture->ctlvdev);
	race->fixture->ctlvdev = NULL;
	current->kunit_test = NULL;
	complete(&race->done);
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

static struct kunit_case attestation_test_cases[] = {
	KUNIT_CASE(attestation_test_provider_contracts),
	KUNIT_CASE(attestation_test_initial_journal),
	KUNIT_CASE(attestation_test_target_extension),
	KUNIT_CASE(attestation_test_failed_extension),
	KUNIT_CASE(attestation_test_state_change),
	KUNIT_CASE(attestation_test_dynamic_topology),
	KUNIT_CASE(attestation_test_stale_target),
	KUNIT_CASE(attestation_test_generation_overflow),
	KUNIT_CASE(attestation_test_allocation_failure),
	KUNIT_CASE(attestation_test_event_allocation_failure),
	KUNIT_CASE(attestation_test_teardown_waits_for_srcu),
	KUNIT_CASE(attestation_test_kexec_roundtrip),
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
