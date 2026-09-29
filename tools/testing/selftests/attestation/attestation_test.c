// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/hash_info.h>

#include "../kselftest_harness.h"
#include "ctlv_attestation_sim_user_gen.h"

#define DEBUGFS_ROOT "/sys/kernel/debug/attestation-sim"
#define EVENT_WAIT_MS 5000
#define EVIDENCE_MAX 4096

static const char *error_text(const struct ctlv_error *error)
{
	static char text[512];

	return ctlv_error_str(error, text, sizeof(text));
}

static int write_text(const char *path, const char *text)
{
	ssize_t length = strlen(text);
	int fd;

	fd = open(path, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	if (write(fd, text, length) != length) {
		int ret = errno ? -errno : -EIO;

		close(fd);
		return ret;
	}
	close(fd);
	return 0;
}

static int read_text(const char *path, char *text, size_t size)
{
	ssize_t length;
	int fd;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	length = read(fd, text, size - 1);
	if (length < 0) {
		int ret = -errno;

		close(fd);
		return ret;
	}
	text[length] = '\0';
	close(fd);
	return 0;
}

static int fixture_control(const char *control, const char *name)
{
	char path[256];

	snprintf(path, sizeof(path), "%s/%s", DEBUGFS_ROOT, control);
	return write_text(path, name);
}

static int fixture_file(const char *name, const char *file,
			const char *value)
{
	char path[256];

	snprintf(path, sizeof(path), "%s/%s/%s",
		 DEBUGFS_ROOT, name, file);
	return write_text(path, value);
}

static int fixture_read(const char *name, const char *file,
			char *value, size_t size)
{
	char path[256];

	snprintf(path, sizeof(path), "%s/%s/%s",
		 DEBUGFS_ROOT, name, file);
	return read_text(path, value, size);
}

static struct ctlv_dev *fixture_open(const char *name,
				     struct ctlv_error *error)
{
	struct ctlv_dev *dev;
	unsigned int which;

	for (which = 0; which < 32; which++) {
		const struct ctlv_attr *info;
		struct ctlv_reply *reply;
		const char *found = NULL;
		int ret;

		ret = ctlv_open("ctlv-attestation-sim", which, O_RDWR, &dev);
		if (ret)
			return NULL;
		ret = ctlv_attestation_device_get(dev, NULL, &reply, error);
		if (ret) {
			ctlv_close(dev);
			return NULL;
		}
		info = ctlv_attestation_device_info_get(
			ctlv_reply_root(reply));
		if (info)
			ctlv_attestation_sim_sim_name_get(info, &found);
		if (found && !strcmp(found, name)) {
			ctlv_reply_free(reply);
			return dev;
		}
		ctlv_reply_free(reply);
		ctlv_close(dev);
	}
	return NULL;
}

FIXTURE(attestation) {
	char name[32];
	struct ctlv_dev *dev;
	struct ctlv_error error;
	bool destroyed;
};

FIXTURE_SETUP(attestation)
{
	if (access(DEBUGFS_ROOT "/create", W_OK))
		SKIP(return, "attestation simulator is unavailable");
	snprintf(self->name, sizeof(self->name), "test-%d", getpid());
	ASSERT_EQ(0, fixture_control("create", self->name));
	self->dev = fixture_open(self->name, &self->error);
	ASSERT_NE(NULL, self->dev) {
		TH_LOG("%s", error_text(&self->error));
	}
}

FIXTURE_TEARDOWN(attestation)
{
	if (self->dev)
		ctlv_close(self->dev);
	if (!self->destroyed)
		fixture_control("destroy", self->name);
}

TEST_F(attestation, discovery_and_caps)
{
	const struct ctlv_attr *caps, *evidence, *mr, *devsec, *info;
	struct ctlv_reply *reply;
	const char *name = NULL;
	__u64 generation = 1;
	__u64 flags = 0, unknown = 0;

	ASSERT_EQ(3, ctlv_chain_len(self->dev));
	EXPECT_STREQ("ctlv-attestation-sim", ctlv_dev_family(self->dev));
	EXPECT_TRUE(ctlv_dev_inherits(self->dev, "ctlv-attestation"));
	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error))
	{
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_TRUE(ctlv_attestation_device_generation_get(
		ctlv_reply_root(reply), &generation));
	EXPECT_EQ(0, generation);
	caps = ctlv_attestation_device_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_cap_evidence_get(caps);
	mr = ctlv_attestation_cap_mr_get(caps);
	devsec = ctlv_attestation_cap_devsec_get(caps);
	ASSERT_NE(NULL, evidence);
	ASSERT_NE(NULL, mr);
	ASSERT_NE(NULL, devsec);
	EXPECT_TRUE(ctlv_attestation_cap_evidence_flags_get(evidence, &flags,
							    &unknown));
	EXPECT_EQ(0, unknown);
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_CAPS_MASK, flags);
	EXPECT_TRUE(ctlv_attestation_cap_mr_flags_get(mr, &flags, &unknown));
	EXPECT_EQ(CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK, flags);
	info = ctlv_attestation_device_info_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, info);
	EXPECT_TRUE(ctlv_attestation_sim_sim_name_get(info, &name));
	EXPECT_STREQ(self->name, name);
	ctlv_reply_free(reply);
}

static int mr_read(struct ctlv_dev *dev, unsigned int id,
		   struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_read_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_id_put(request, id);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_read(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static int log_get(struct ctlv_dev *dev, struct ctlv_reply **reply,
		   struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_log_get_request_new(&request);
	if (ret)
		return ret;
	ret = ctlv_attestation_mr_log_get(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(attestation, fixed_measurement_registers_and_initial_log)
{
	const struct ctlv_attr *item, *value, *entry;
	struct ctlv_msg *request;
	struct ctlv_reply *reply;
	const char *name;
	const void *digest;
	__u64 id, algorithm, event;
	__u32 length;
	unsigned int count;

	ASSERT_EQ(0, ctlv_attestation_mr_dump(
			  self->dev, NULL, &reply, &self->error));
	count = 0;
	ctlv_attestation_mr_for_each_item(item, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_id_get(item, &id));
		EXPECT_EQ(count, id);
		count++;
	}
	EXPECT_EQ(2, count);
	EXPECT_EQ(2, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, ctlv_attestation_mr_get_request_new(&request));
	ctlv_attestation_mr_id_put(request, 0);
	ASSERT_EQ(0, ctlv_attestation_mr_get(
			  self->dev, request, &reply, &self->error));
	ctlv_msg_free(request);
	EXPECT_TRUE(ctlv_attestation_mr_name_get(
		ctlv_reply_root(reply), &name));
	EXPECT_STREQ("sim-read-only", name);
	ctlv_reply_free(reply);

	ASSERT_EQ(0, mr_read(self->dev, 1, &reply, &self->error));
	count = 0;
	ctlv_attestation_mr_for_each_value(value, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_hash_algo_get(
			value, &algorithm));
		ASSERT_TRUE(ctlv_attestation_mr_data_get(
			value, &digest, &length));
		EXPECT_TRUE(algorithm == HASH_ALGO_SHA256 ||
			    algorithm == HASH_ALGO_SHA384);
		EXPECT_EQ(0, memcmp(digest, (unsigned char[64]) {}, length));
		count++;
	}
	EXPECT_EQ(2, count);
	ctlv_reply_free(reply);

	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	count = 0;
	ctlv_attestation_log_for_each_entry(entry, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_log_event_get(
			entry, &event, NULL));
		EXPECT_EQ(CTLV_ATTESTATION_MR_LOG_EVENT_INFO, event);
		count++;
	}
	EXPECT_EQ(2, count);
	ctlv_reply_free(reply);
}

static int mr_extend(struct ctlv_dev *dev, unsigned int id,
		     const unsigned char *sha256,
		     const unsigned char *sha384,
		     const char *format, const void *data, size_t data_length,
		     struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_extend_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_id_put(request, id);
	if (sha256) {
		ctlv_attestation_mr_value_start(request);
		ctlv_attestation_mr_hash_algo_put(request, HASH_ALGO_SHA256);
		ctlv_attestation_mr_data_put(request, sha256, 32);
		ctlv_nest_end(request);
	}
	if (sha384) {
		ctlv_attestation_mr_value_start(request);
		ctlv_attestation_mr_hash_algo_put(request, HASH_ALGO_SHA384);
		ctlv_attestation_mr_data_put(request, sha384, 48);
		ctlv_nest_end(request);
	}
	if (format) {
		ctlv_attestation_record_item_start(request);
		ctlv_attestation_record_format_put(request, format);
		ctlv_attestation_record_data_put(request, data, data_length);
		ctlv_nest_end(request);
	}
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_extend(
			dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(attestation, extension_record_log_and_event)
{
	static const unsigned char sha256[32] = { 1 };
	static const unsigned char sha384[48] = { 2 };
	static const char record[] = "services-started";
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	const struct ctlv_attr *entry, *record_nest;
	struct ctlv_event *event;
	struct ctlv_reply *reply;
	const char *format = NULL;
	const void *data;
	__u64 generation = 0;
	__u32 length;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, mr_extend(self->dev, 1, sha256, sha384,
			       "org.example.boot-phase.v1",
			       record, sizeof(record) - 1,
			       &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_TRUE(ctlv_attestation_mr_generation_get(
		ctlv_reply_root(reply), &generation));
	EXPECT_EQ(3, generation);
	ctlv_reply_free(reply);

	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	ASSERT_TRUE(ctlv_attestation_mr_log_entry_appended_is(event));
	entry = ctlv_attestation_notification_entry_get(
		ctlv_event_root(event));
	ASSERT_NE(NULL, entry);
	record_nest = ctlv_attestation_record_item_get(entry);
	ASSERT_NE(NULL, record_nest);
	EXPECT_TRUE(ctlv_attestation_record_format_get(
		record_nest, &format));
	EXPECT_STREQ("org.example.boot-phase.v1", format);
	EXPECT_TRUE(ctlv_attestation_record_data_get(
		record_nest, &data, &length));
	EXPECT_EQ(sizeof(record) - 1, length);
	EXPECT_EQ(0, memcmp(record, data, length));
	ctlv_event_free(event);

	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(3, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
}

static int evidence_get(struct ctlv_dev *dev, void *output, size_t output_length,
			struct ctlv_reply **reply, struct ctlv_error *error)
{
	static const unsigned char challenge[] = "freshness";
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_evidence_get_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_evidence_challenge_put(
		request, challenge, sizeof(challenge) - 1);
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(
		request, CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY);
	ctlv_attestation_evidence_format_put(
		request, "attestation-sim-primary-v1");
	ctlv_attestation_evidence_representation_put(
		request, CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
	ctlv_attestation_evidence_data_put_va(
		request, output, output_length);
	ctlv_nest_end(request);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_get(
			dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static int evidence_accept(struct ctlv_dev *dev, __u64 device_generation,
			   __u64 evidence_generation,
			   struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_evidence_accept_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_device_generation_put(request, device_generation);
	ctlv_attestation_evidence_generation_put(
		request, evidence_generation);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_accept(dev, request, error);
	ctlv_msg_free(request);
	return ret;
}

static void expect_event(struct __test_metadata *_metadata,
			 struct ctlv_dev *dev, __u64 op_id)
{
	struct ctlv_event *event;

	ASSERT_EQ(0, ctlv_event_wait(dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(dev, &event));
	EXPECT_EQ(op_id, ctlv_event_op_id(event));
	ctlv_event_free(event);
}

TEST_F(attestation, evidence_objects_representations_and_options)
{
	static union {
		__u64 alignment;
		unsigned char data[EVIDENCE_MAX];
	} primary;
	static union {
		__u64 alignment;
		unsigned char data[1024];
	} auxiliary, manifest;
	static const unsigned char challenge[] = "options";
	static const unsigned char guid[16] = { 1, 2, 3, 4 };
	const struct ctlv_attr *length_attr;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	__u64 length;
	unsigned int lengths = 0;

	ASSERT_EQ(0, ctlv_attestation_evidence_get_request_new(&request));
	ctlv_attestation_evidence_challenge_put(
		request, challenge, sizeof(challenge) - 1);
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(
		request, CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY);
	ctlv_attestation_evidence_format_put(
		request, "attestation-sim-primary-v1");
	ctlv_attestation_evidence_representation_put(
		request, CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_TCB_DIGEST);
	ctlv_attestation_evidence_hash_algo_put(request, HASH_ALGO_SHA256);
	ctlv_attestation_evidence_data_put_va(
		request, primary.data, sizeof(primary.data));
	ctlv_nest_end(request);
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(
		request, CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_AUXILIARY);
	ctlv_attestation_evidence_format_put(
		request, "attestation-sim-aux-v1");
	ctlv_attestation_evidence_representation_put(
		request, CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
	ctlv_attestation_evidence_data_put_va(
		request, auxiliary.data, sizeof(auxiliary.data));
	ctlv_nest_end(request);
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(
		request, CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_MANIFEST);
	ctlv_attestation_evidence_blob_instance_put(request, 1);
	ctlv_attestation_evidence_format_put(
		request, "attestation-sim-manifest-v1");
	ctlv_attestation_evidence_representation_put(
		request, CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
	ctlv_attestation_evidence_data_put_va(
		request, manifest.data, sizeof(manifest.data));
	ctlv_nest_end(request);
	ctlv_attestation_evidence_selection_start(request);
	ctlv_attestation_mr_id_put(request, 1);
	ctlv_attestation_evidence_hash_algo_put(request, HASH_ALGO_SHA256);
	ctlv_nest_end(request);
	ctlv_attestation_evidence_privlevel_put(request, 7);
	ctlv_attestation_evidence_service_provider_put(
		request, "example-service");
	ctlv_attestation_evidence_service_guid_put(
		request, guid, sizeof(guid));
	ctlv_attestation_evidence_service_manifest_version_put(request, 9);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_attestation_evidence_get(
			  self->dev, request, &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_msg_free(request);
	ctlv_attestation_evidence_for_each_object_length(
		length_attr, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_evidence_object_length_get(
			length_attr, &length));
		EXPECT_GT(length, 0);
		lengths++;
	}
	EXPECT_EQ(3, lengths);
	EXPECT_NE(0, memcmp(primary.data, (unsigned char[32]) {}, 32));
	EXPECT_NE(NULL, memmem(auxiliary.data, sizeof(auxiliary.data),
			       "SIM-AUX-v1", 10));
	EXPECT_NE(NULL, memmem(manifest.data, sizeof(manifest.data),
			       self->name, strlen(self->name)));
	ctlv_reply_free(reply);
}

TEST_F(attestation, evidence_verdict_and_run)
{
	static union {
		__u64 alignment;
		unsigned char data[EVIDENCE_MAX];
	} output;
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED,
		CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED,
	};
	__u64 evidence_generation = 0, device_generation = 0, state = 0;
	bool known;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, fixture_file(self->name, "state", "LOCKED NONE"));
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED);
	ASSERT_EQ(0, evidence_get(self->dev, output.data, sizeof(output.data),
				  &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_TRUE(ctlv_attestation_device_generation_get(
		ctlv_reply_root(reply), &device_generation));
	EXPECT_TRUE(ctlv_attestation_evidence_generation_get(
		ctlv_reply_root(reply), &evidence_generation));
	EXPECT_EQ(1, device_generation);
	EXPECT_EQ(1, evidence_generation);
	ctlv_reply_free(reply);
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);
	ASSERT_EQ(0, evidence_accept(self->dev, device_generation,
				     evidence_generation, &self->error));
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);

	ASSERT_EQ(0,
		  ctlv_attestation_evidence_accept_clear_request_new(&request));
	ASSERT_EQ(0, ctlv_attestation_evidence_accept_clear(
			  self->dev, request, &self->error));
	ctlv_msg_free(request);
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);
	ASSERT_EQ(0, evidence_accept(self->dev, device_generation,
				     evidence_generation, &self->error));
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);

	ASSERT_EQ(0, ctlv_attestation_devsec_run_request_new(&request));
	ctlv_attestation_device_generation_put(request, device_generation);
	ctlv_attestation_evidence_generation_put(
		request, evidence_generation);
	ASSERT_EQ(0, ctlv_attestation_devsec_run(
			  self->dev, request, &reply, &self->error));
	ctlv_msg_free(request);
	EXPECT_TRUE(ctlv_attestation_device_generation_get(
		ctlv_reply_root(reply), &device_generation));
	EXPECT_EQ(2, device_generation);
	ctlv_reply_free(reply);
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED);
	expect_event(_metadata, self->dev,
		     CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED);

	ASSERT_EQ(0, ctlv_attestation_devsec_state_get(
			  self->dev, NULL, &reply, &self->error));
	EXPECT_TRUE(ctlv_attestation_devsec_tdisp_state_get(
		ctlv_reply_root(reply), &state, &known));
	EXPECT_TRUE(known);
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_RUN, state);
	ctlv_reply_free(reply);
}

static int devsec_state(struct ctlv_dev *dev, __u64 *state,
			struct ctlv_error *error)
{
	struct ctlv_reply *reply;
	bool known;
	int ret;

	ret = ctlv_attestation_devsec_state_get(dev, NULL, &reply, error);
	if (ret)
		return ret;
	if (!ctlv_attestation_devsec_tdisp_state_get(
		    ctlv_reply_root(reply), state, &known) || !known)
		ret = -EPROTO;
	ctlv_reply_free(reply);
	return ret;
}

TEST_F(attestation, fixture_isolation)
{
	char other_name[32];
	struct ctlv_error other_error = {};
	struct ctlv_dev *other;
	__u64 state;

	snprintf(other_name, sizeof(other_name), "other-%d", getpid());
	ASSERT_EQ(0, fixture_control("create", other_name));
	other = fixture_open(other_name, &other_error);
	ASSERT_NE(NULL, other);
	ASSERT_EQ(0, fixture_file(self->name, "state", "LOCKED NONE"));
	ASSERT_EQ(0, fixture_file(other_name, "state", "ERROR TCB"));
	ASSERT_EQ(0, devsec_state(self->dev, &state, &self->error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_LOCKED, state);
	ASSERT_EQ(0, devsec_state(other, &state, &other_error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_ERROR, state);
	ctlv_close(other);
	EXPECT_EQ(0, fixture_control("destroy", other_name));
}

TEST_F(attestation, provider_failure_is_transactional)
{
	static const unsigned char sha256[32] = { 3 };
	static const unsigned char sha384[48] = { 4 };
	struct ctlv_reply *reply;

	ASSERT_EQ(0, fixture_file(self->name, "fail-next", "mr_extend EIO"));
	EXPECT_EQ(-EIO, mr_extend(self->dev, 1, sha256, sha384,
				  NULL, NULL, 0, &reply, &self->error));
	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(2, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
}

TEST_F(attestation, event_loss_and_journal_recovery)
{
	static unsigned char record[4096];
	unsigned char sha256[32] = { 7 };
	unsigned char sha384[48] = { 8 };
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	struct ctlv_overflow overflow;
	struct ctlv_event *event;
	struct ctlv_reply *reply;
	bool found = false;
	__u64 queue_len, queue_used;
	unsigned int index, extensions;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, ctlv_queue_get(
			  self->dev, &queue_len, &queue_used, &self->error));
	extensions = queue_len / sizeof(record) + 16;
	for (index = 0; index < extensions; index++) {
		sha256[0] = index;
		sha384[0] = index;
		ASSERT_EQ(0, mr_extend(
				  self->dev, 1, sha256, sha384,
				  "org.example.large-record.v1",
				  record, sizeof(record), &reply,
				  &self->error));
		ctlv_reply_free(reply);
	}
	while (!ctlv_event_wait(self->dev, 100)) {
		ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
		if (!ctlv_event_overflow(event, &overflow)) {
			found = true;
			EXPECT_GT(overflow.dropped_count, 0);
			EXPECT_LE(overflow.first_sequence,
				  overflow.last_sequence);
		}
		ctlv_event_free(event);
	}
	EXPECT_TRUE(found);
	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(2 + extensions, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
}

TEST_F(attestation, dynamic_topology)
{
	const struct ctlv_attr *item;
	struct ctlv_reply *reply;
	unsigned int count = 0;

	ASSERT_EQ(0, fixture_file(self->name, "mr-add", "9 sha256"));
	ASSERT_EQ(0, ctlv_attestation_mr_dump(
			  self->dev, NULL, &reply, &self->error));
	ctlv_attestation_mr_for_each_item(item, ctlv_reply_root(reply))
		count++;
	EXPECT_EQ(3, count);
	EXPECT_EQ(3, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
	ASSERT_EQ(0, fixture_file(self->name, "mr-remove", "9"));
	ASSERT_EQ(0, ctlv_attestation_mr_dump(
			  self->dev, NULL, &reply, &self->error));
	EXPECT_EQ(4, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
}

struct paused_extend {
	struct ctlv_dev *dev;
	struct ctlv_error error;
	int result;
};

static void *paused_extend_thread(void *arg)
{
	static const unsigned char sha256[32] = { 5 };
	static const unsigned char sha384[48] = { 6 };
	struct paused_extend *paused = arg;
	struct ctlv_reply *reply = NULL;

	paused->result = mr_extend(paused->dev, 1, sha256, sha384,
				   NULL, NULL, 0, &reply, &paused->error);
	if (reply)
		ctlv_reply_free(reply);
	return NULL;
}

TEST_F(attestation, paused_callback_and_removal)
{
	struct paused_extend paused = {
		.dev = self->dev,
	};
	char status[512];
	pthread_t thread;
	unsigned int tries;

	ASSERT_EQ(0, fixture_file(self->name, "pause-next", "mr_extend"));
	ASSERT_EQ(0, pthread_create(
			  &thread, NULL, paused_extend_thread, &paused));
	for (tries = 0; tries < 500; tries++) {
		ASSERT_EQ(0, fixture_read(
				  self->name, "status", status, sizeof(status)));
		if (strstr(status, "paused mr_extend"))
			break;
		usleep(1000);
	}
	ASSERT_LT(tries, 500);
	ASSERT_EQ(0, fixture_file(self->name, "resume", "1"));
	ASSERT_EQ(0, pthread_join(thread, NULL));
	EXPECT_EQ(0, paused.result) {
		TH_LOG("%s", error_text(&paused.error));
	}
}

TEST_F(attestation, terminal_removal)
{
	struct ctlv_event *event;

	ASSERT_EQ(0, fixture_control("destroy", self->name));
	self->destroyed = true;
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	EXPECT_EQ(CTLV_OP_DEVICE_REMOVED, ctlv_event_op_id(event));
	ctlv_event_free(event);
	EXPECT_EQ(-ENODEV, ctlv_attestation_device_get(
				  self->dev, NULL, NULL, &self->error));
}

TEST_HARNESS_MAIN
