// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/hash_info.h>

#include "../kselftest_harness.h"
#include "ctlv_attestation_sim_user_gen.h"

#define DEBUGFS_ROOT "/sys/kernel/debug/attestation-sim"
#define EVENT_WAIT_MS 5000
#define EVIDENCE_MAX 4096
/* Past the TCG_PCR_EVENT header of a crypto-agile log's first event. */
#define TCG_SPEC_ID_SIGNATURE_OFFSET 32

/* The simulator's objects, by ID. */
enum { SIM_REPORT, SIM_TOKEN, SIM_LOG, SIM_MANIFEST, SIM_OBJECTS };

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
		const struct ctlv_hdr *info;
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
		info = ctlv_attestation_device_get_info_get(
			ctlv_reply_root(reply));
		if (info)
			ctlv_attestation_sim_device_info_name_get(info, &found);
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
	const struct ctlv_hdr *caps, *evidence, *mr, *info;
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
	caps = ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_device_get_caps_evidence_get(caps);
	mr = ctlv_attestation_device_get_caps_mr_get(caps);
	ASSERT_NE(NULL, evidence);
	ASSERT_NE(NULL, mr);
	EXPECT_TRUE(ctlv_attestation_device_get_evidence_flags_get(
		evidence, &flags, &unknown));
	EXPECT_EQ(0, unknown);
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_CAPS_MASK, flags);
	EXPECT_TRUE(ctlv_attestation_device_get_mr_flags_get(mr, &flags,
							     &unknown));
	EXPECT_EQ(CTLV_ATTESTATION_MR_DEVICE_CAPS_MASK, flags);
	info = ctlv_attestation_device_get_info_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, info);
	EXPECT_TRUE(ctlv_attestation_sim_device_info_name_get(info, &name));
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
	ctlv_attestation_mr_read_id_put(request, id);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_read(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static int mr_get(struct ctlv_dev *dev, unsigned int id,
		  struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_get_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_get_id_put(request, id);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_get(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

/* The item of an mr-get reply that describes one MR, or NULL. */
static const struct ctlv_hdr *mr_only(const struct ctlv_reply *reply)
{
	const struct ctlv_hdr *item, *only = NULL;
	unsigned int count = 0;

	ctlv_attestation_mr_get_items_for_each(item, ctlv_reply_root(reply)) {
		only = item;
		count++;
	}
	return count == 1 ? only : NULL;
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
	const struct ctlv_hdr *item, *value, *entry;
	struct ctlv_reply *reply;
	const char *name;
	const void *digest;
	__u64 id, algorithm, event;
	__u32 length;
	unsigned int count;

	ASSERT_EQ(0, ctlv_attestation_mr_get(
			  self->dev, NULL, &reply, &self->error));
	count = 0;
	ctlv_attestation_mr_get_items_for_each(item, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_get_items_id_get(item, &id));
		EXPECT_EQ(count, id);
		count++;
	}
	EXPECT_EQ(2, count);
	EXPECT_EQ(2, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, mr_get(self->dev, 0, &reply, &self->error));
	item = mr_only(reply);
	ASSERT_NE(NULL, item);
	EXPECT_TRUE(ctlv_attestation_mr_desc_name_get(item, &name));
	EXPECT_STREQ("sim-read-only", name);
	ctlv_reply_free(reply);
	EXPECT_EQ(-ENOENT, mr_get(self->dev, 2, &reply, &self->error));

	ASSERT_EQ(0, mr_read(self->dev, 1, &reply, &self->error));
	count = 0;
	ctlv_attestation_mr_values_for_each(value, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_value_hash_algo_get(
			value, &algorithm, NULL));
		ASSERT_TRUE(ctlv_attestation_mr_value_data_get(
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
	ctlv_attestation_mr_log_get_entries_for_each(entry,
						     ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_log_entry_event_get(
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
	ctlv_attestation_mr_extend_id_put(request, id);
	if (sha256) {
		ctlv_attestation_mr_values_start(request);
		ctlv_attestation_mr_value_hash_algo_put(request,
							HASH_ALGO_SHA256);
		ctlv_attestation_mr_value_data_put(request, sha256, 32);
		ctlv_record_end(request);
	}
	if (sha384) {
		ctlv_attestation_mr_values_start(request);
		ctlv_attestation_mr_value_hash_algo_put(request,
							HASH_ALGO_SHA384);
		ctlv_attestation_mr_value_data_put(request, sha384, 48);
		ctlv_record_end(request);
	}
	if (format) {
		ctlv_attestation_mr_extend_record_start(request);
		ctlv_attestation_mr_extend_record_format_put(request, format);
		ctlv_attestation_mr_extend_record_data_put(request, data,
							   data_length);
		ctlv_record_end(request);
	}
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_extend(
			dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static bool digest_is(const struct ctlv_hdr *parent, __u64 wanted,
		      const unsigned char *expected, __u32 expected_len)
{
	const struct ctlv_hdr *value;
	const void *digest;
	__u64 algorithm;
	__u32 length;

	ctlv_attestation_mr_values_for_each(value, parent) {
		if (!ctlv_attestation_mr_value_hash_algo_get(value, &algorithm,
							     NULL) ||
		    algorithm != wanted)
			continue;
		return ctlv_attestation_mr_value_data_get(value, &digest,
							  &length) &&
		       length == expected_len &&
		       !memcmp(digest, expected, length);
	}
	return false;
}

TEST_F(attestation, extension_record_log_and_event)
{
	static const unsigned char sha256[32] = { 1 };
	static const unsigned char sha384[48] = { 2 };
	static const char record[] = "services-started";
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	const struct ctlv_hdr *entry, *record_tlv;
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
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA256,
			      sha256, sizeof(sha256)));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA384,
			      sha384, sizeof(sha384)));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	ASSERT_TRUE(ctlv_attestation_mr_log_entry_appended_is(event));
	entry = ctlv_attestation_mr_log_entry_appended_entry_get(
		ctlv_event_root(event));
	ASSERT_NE(NULL, entry);
	record_tlv = ctlv_attestation_mr_log_entry_record_get(entry);
	ASSERT_NE(NULL, record_tlv);
	EXPECT_TRUE(ctlv_attestation_mr_log_entry_record_format_get(
		record_tlv, &format));
	EXPECT_STREQ("org.example.boot-phase.v1", format);
	EXPECT_TRUE(ctlv_attestation_mr_log_entry_record_data_get(
		record_tlv, &data, &length));
	EXPECT_EQ(sizeof(record) - 1, length);
	EXPECT_EQ(0, memcmp(record, data, length));
	ctlv_event_free(event);

	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(3, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
}

/* From the memfd when fd is not negative, else from data. */
static int mr_extend_raw(struct ctlv_dev *dev, unsigned int id,
			 const void *data, size_t length, int fd,
			 const unsigned char *sha256,
			 struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_extend_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_extend_id_put(request, id);
	if (fd < 0)
		ctlv_attestation_mr_extend_raw_data_put_va(request, data,
							   length);
	else
		ctlv_attestation_mr_extend_raw_data_put_memfd(request, fd, 0,
							      length);
	if (sha256) {
		ctlv_attestation_mr_values_start(request);
		ctlv_attestation_mr_value_hash_algo_put(request,
							HASH_ALGO_SHA256);
		ctlv_attestation_mr_value_data_put(request, sha256, 32);
		ctlv_record_end(request);
	}
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_extend(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(attestation, extension_by_data)
{
	static const unsigned char abc_sha256[32] = {
		0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
		0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
		0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
		0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
	};
	static const unsigned char abc_sha384[48] = {
		0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b,
		0xb5, 0xa0, 0x3d, 0x69, 0x9a, 0xc6, 0x50, 0x07,
		0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
		0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed,
		0x80, 0x86, 0x07, 0x2b, 0xa1, 0xe7, 0xcc, 0x23,
		0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7,
	};
	static const unsigned char million_sha256[32] = {
		0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92,
		0x81, 0xa1, 0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67,
		0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97, 0x20, 0x0e,
		0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0,
	};
	static const unsigned char million_sha384[48] = {
		0x9d, 0x0e, 0x18, 0x09, 0x71, 0x64, 0x74, 0xcb,
		0x08, 0x6e, 0x83, 0x4e, 0x31, 0x0a, 0x4a, 0x1c,
		0xed, 0x14, 0x9e, 0x9c, 0x00, 0xf2, 0x48, 0x52,
		0x79, 0x72, 0xce, 0xc5, 0x70, 0x4c, 0x2a, 0x5b,
		0x07, 0xb8, 0xb3, 0xdc, 0x38, 0xec, 0xc4, 0xeb,
		0xae, 0x97, 0xdd, 0xd8, 0x7f, 0x3d, 0x89, 0x85,
	};
	static const unsigned char sha256[32] = { 5 };
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	const struct ctlv_hdr *entry;
	struct ctlv_event *event;
	struct ctlv_reply *reply;
	char abc[] = "abc";
	char block[1000];
	int fd, i;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, mr_extend_raw(self->dev, 1, abc, 3, -1, NULL,
				   &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA256,
			      abc_sha256, sizeof(abc_sha256)));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA384,
			      abc_sha384, sizeof(abc_sha384)));
	ctlv_reply_free(reply);
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	entry = ctlv_attestation_mr_log_entry_appended_entry_get(
		ctlv_event_root(event));
	ASSERT_NE(NULL, entry);
	EXPECT_TRUE(digest_is(entry, HASH_ALGO_SHA256, abc_sha256,
			      sizeof(abc_sha256)));
	EXPECT_TRUE(digest_is(entry, HASH_ALGO_SHA384, abc_sha384,
			      sizeof(abc_sha384)));
	ctlv_event_free(event);

	fd = memfd_create("attestation-test", MFD_CLOEXEC | MFD_ALLOW_SEALING);
	ASSERT_LE(0, fd);
	memset(block, 'a', sizeof(block));
	for (i = 0; i < 1000; i++)
		ASSERT_EQ((ssize_t)sizeof(block),
			  write(fd, block, sizeof(block)));
	ASSERT_EQ(0, fcntl(fd, F_ADD_SEALS, F_SEAL_SHRINK));
	ASSERT_EQ(0, mr_extend_raw(self->dev, 1, NULL, 1000000, fd, NULL,
				   &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	close(fd);
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA256,
			      million_sha256, sizeof(million_sha256)));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA384,
			      million_sha384, sizeof(million_sha384)));
	ctlv_reply_free(reply);
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	entry = ctlv_attestation_mr_log_entry_appended_entry_get(
		ctlv_event_root(event));
	ASSERT_NE(NULL, entry);
	EXPECT_TRUE(digest_is(entry, HASH_ALGO_SHA256, million_sha256,
			      sizeof(million_sha256)));
	EXPECT_TRUE(digest_is(entry, HASH_ALGO_SHA384, million_sha384,
			      sizeof(million_sha384)));
	ctlv_event_free(event);

	EXPECT_EQ(-EINVAL, mr_extend_raw(self->dev, 1, abc, 3, -1, sha256,
					 &reply, &self->error));
	EXPECT_EQ(-EINVAL, mr_extend(self->dev, 1, NULL, NULL, NULL, NULL, 0,
				     &reply, &self->error));
	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(4, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, fixture_file(self->name, "mr-add", "9 sha256 sm3"));
	EXPECT_EQ(-EOPNOTSUPP, mr_extend_raw(self->dev, 9, abc, 3, -1, NULL,
					     &reply, &self->error));
}

static bool values_in_order(const struct ctlv_hdr *parent,
			    const __u64 *algorithms, unsigned int count)
{
	const struct ctlv_hdr *value;
	unsigned int index = 0;
	__u64 algorithm;

	ctlv_attestation_mr_values_for_each(value, parent) {
		if (index == count ||
		    !ctlv_attestation_mr_value_hash_algo_get(value, &algorithm,
							     NULL) ||
		    algorithm != algorithms[index++])
			return false;
	}
	return index == count;
}

TEST_F(attestation, bank_order)
{
	static const __u64 banks[] = { HASH_ALGO_SHA384, HASH_ALGO_SHA256 };
	static const unsigned char sha256[32] = { 6 };
	static const unsigned char sha384[48] = { 7 };
	struct ctlv_msg *request;
	struct ctlv_reply *reply;
	int i;

	EXPECT_EQ(-EINVAL, fixture_file(self->name, "mr-add",
					"9 sha256 sha256"));
	ASSERT_EQ(0, fixture_file(self->name, "mr-add", "9 sha384 sha256"));

	/* mr_extend() puts SHA-256 first. */
	ASSERT_EQ(0, mr_extend(self->dev, 9, sha256, sha384, NULL, NULL, 0,
			       &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_TRUE(values_in_order(ctlv_reply_root(reply), banks,
				    ARRAY_SIZE(banks)));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA256,
			      sha256, sizeof(sha256)));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA384,
			      sha384, sizeof(sha384)));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, mr_read(self->dev, 9, &reply, &self->error));
	EXPECT_TRUE(values_in_order(ctlv_reply_root(reply), banks,
				    ARRAY_SIZE(banks)));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, ctlv_attestation_mr_extend_request_new(&request));
	ctlv_attestation_mr_extend_id_put(request, 9);
	for (i = 0; i < 2; i++) {
		ctlv_attestation_mr_values_start(request);
		ctlv_attestation_mr_value_hash_algo_put(request,
							HASH_ALGO_SHA256);
		ctlv_attestation_mr_value_data_put(request, sha256, 32);
		ctlv_record_end(request);
	}
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(-EINVAL, ctlv_attestation_mr_extend(self->dev, request,
						      &reply, &self->error));
	ctlv_msg_free(request);
}

/* Each extend takes one of MR 1's banks and leaves the other as it was. */
TEST_F(attestation, bank_subsets)
{
	static const __u64 sha256_bank[] = { HASH_ALGO_SHA256 };
	static const __u64 sha384_bank[] = { HASH_ALGO_SHA384 };
	static const unsigned char sha256[32] = { 0x11 };
	static const unsigned char sha384[48] = { 0x22 };
	/* Each bank's hash of its zeroes and then sha256 or sha384. */
	static const unsigned char after256[32] = {
		0x96, 0x72, 0x93, 0xee, 0x9d, 0x7b, 0xa6, 0x79,
		0xc3, 0xef, 0x07, 0x6b, 0xef, 0x13, 0x9e, 0x2c,
		0xeb, 0x96, 0xd4, 0x5d, 0x19, 0xa6, 0x24, 0xcc,
		0x59, 0xbb, 0x5a, 0x3c, 0x16, 0x49, 0xce, 0x38,
	};
	static const unsigned char after384[48] = {
		0x92, 0xe1, 0xe2, 0xec, 0xd4, 0xd8, 0x81, 0xb9,
		0x81, 0x5c, 0x5f, 0xf0, 0xf4, 0xe0, 0x8e, 0x74,
		0x57, 0x26, 0x56, 0x7a, 0x10, 0xed, 0xf9, 0x32,
		0x15, 0x0c, 0xd3, 0x73, 0x4f, 0x2b, 0x84, 0x82,
		0x64, 0x89, 0x9a, 0xbe, 0xd5, 0x0c, 0xd1, 0xf7,
		0xde, 0x41, 0xbf, 0x8d, 0xc2, 0x4d, 0xab, 0xd6,
	};
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	const struct ctlv_hdr *root, *entry;
	struct ctlv_event *event;
	struct ctlv_reply *reply;
	__u64 generation = 0;
	const __u64 *bank;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, mr_extend(self->dev, 1, sha256, NULL, NULL, NULL, 0,
			       &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	root = ctlv_reply_root(reply);
	EXPECT_TRUE(ctlv_attestation_mr_generation_get(root, &generation));
	EXPECT_EQ(3, generation);
	EXPECT_TRUE(values_in_order(root, sha256_bank, 1));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	root = ctlv_event_root(event);
	entry = ctlv_attestation_mr_log_entry_appended_entry_get(root);
	ASSERT_NE(NULL, entry);
	EXPECT_TRUE(values_in_order(entry, sha256_bank, 1));
	EXPECT_TRUE(digest_is(entry, HASH_ALGO_SHA256, sha256,
			      sizeof(sha256)));
	ctlv_event_free(event);

	ASSERT_EQ(0, mr_extend(self->dev, 1, NULL, sha384, NULL, NULL, 0,
			       &reply, &self->error));
	EXPECT_TRUE(values_in_order(ctlv_reply_root(reply), sha384_bank, 1));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, mr_read(self->dev, 1, &reply, &self->error));
	root = ctlv_reply_root(reply);
	EXPECT_TRUE(digest_is(root, HASH_ALGO_SHA256, after256,
			      sizeof(after256)));
	EXPECT_TRUE(digest_is(root, HASH_ALGO_SHA384, after384,
			      sizeof(after384)));
	ctlv_reply_free(reply);

	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error));
	EXPECT_EQ(4, ctlv_reply_generation(reply));
	ctlv_attestation_mr_log_get_entries_for_each(entry,
						     ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_generation_get(entry,
							       &generation));
		if (generation < 3)
			continue;
		bank = generation == 3 ? sha256_bank : sha384_bank;
		EXPECT_TRUE(values_in_order(entry, bank, 1));
	}
	ctlv_reply_free(reply);
}

/* MR 0 allows nothing, and is read all the same. */
TEST_F(attestation, read_without_caps)
{
	static const __u64 sha384_bank[] = { HASH_ALGO_SHA384 };
	static const unsigned char zero[48];
	__u64 caps = 1, unknown = 1;
	const struct ctlv_hdr *item;
	struct ctlv_reply *reply;

	ASSERT_EQ(0, mr_get(self->dev, 0, &reply, &self->error));
	item = mr_only(reply);
	ASSERT_NE(NULL, item);
	EXPECT_TRUE(ctlv_attestation_mr_desc_caps_get(item, &caps, &unknown));
	EXPECT_EQ(0, caps);
	EXPECT_EQ(0, unknown);
	ctlv_reply_free(reply);

	ASSERT_EQ(0, mr_read(self->dev, 0, &reply, &self->error));
	EXPECT_TRUE(values_in_order(ctlv_reply_root(reply), sha384_bank, 1));
	EXPECT_TRUE(digest_is(ctlv_reply_root(reply), HASH_ALGO_SHA384, zero,
			      sizeof(zero)));
	ctlv_reply_free(reply);
}

static void evidence_object_put(struct ctlv_msg *request, __u64 id,
				void *output, size_t output_length)
{
	ctlv_attestation_evidence_get_objects_start(request);
	ctlv_attestation_evidence_get_objects_id_put(request, id);
	ctlv_attestation_evidence_get_objects_data_put_va(request, output,
							  output_length);
	ctlv_record_end(request);
}

/* Asks for the objects @ids, each into a blob @output_length long. */
static int evidence_ids_get(struct ctlv_dev *dev, const void *challenge,
			    __u32 challenge_length, const __u64 *ids,
			    size_t count, size_t output_length,
			    struct ctlv_reply **reply, struct ctlv_error *error)
{
	static union {
		__u64 alignment;
		unsigned char data[SIM_OBJECTS][EVIDENCE_MAX];
	} blobs;
	struct ctlv_msg *request;
	size_t index;
	int ret;

	if (count > SIM_OBJECTS || output_length > EVIDENCE_MAX)
		return -E2BIG;
	ret = ctlv_attestation_evidence_get_request_new(&request);
	if (ret)
		return ret;
	if (challenge)
		ctlv_attestation_evidence_get_challenge_put(request, challenge,
							    challenge_length);
	for (index = 0; index < count; index++)
		evidence_object_put(request, ids[index], blobs.data[index],
				    output_length);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_get(
			dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

/* Asks for the report alone. */
static int evidence_get(struct ctlv_dev *dev, const void *challenge,
			__u32 challenge_length, size_t output_length,
			struct ctlv_reply **reply, struct ctlv_error *error)
{
	static const __u64 report[] = { SIM_REPORT };

	return evidence_ids_get(dev, challenge, challenge_length, report, 1,
				output_length, reply, error);
}

/* New evidence, and the device and evidence generations it was given. */
static int evidence_collect(struct ctlv_dev *dev, __u64 *device_generation,
			    __u64 *evidence_generation,
			    struct ctlv_error *error)
{
	static const unsigned char challenge[] = "freshness";
	struct ctlv_reply *reply;
	int ret;

	ret = evidence_get(dev, challenge, sizeof(challenge) - 1, EVIDENCE_MAX,
			   &reply, error);
	if (ret)
		return ret;
	if (!ctlv_attestation_device_generation_get(ctlv_reply_root(reply),
						    device_generation) ||
	    !ctlv_attestation_evidence_get_evidence_generation_get(
		    ctlv_reply_root(reply), evidence_generation))
		ret = -EPROTO;
	ctlv_reply_free(reply);
	return ret;
}

static int evidence_accept(struct ctlv_dev *dev, __u64 device_generation,
			   __u64 evidence_generation, __u64 *new_generation,
			   struct ctlv_error *error)
{
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_evidence_accept_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_device_generation_put(request, device_generation);
	ctlv_attestation_evidence_accept_evidence_generation_put(
		request, evidence_generation);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_accept(dev, request, &reply,
						       error);
	ctlv_msg_free(request);
	if (ret)
		return ret;
	if (!ctlv_attestation_device_generation_get(ctlv_reply_root(reply),
						    new_generation))
		ret = -EPROTO;
	ctlv_reply_free(reply);
	return ret;
}

/* The next event is @op_id, its evidence generation 0 when it has none. */
static void expect_state_event(struct __test_metadata *_metadata,
			       struct ctlv_dev *dev, __u64 op_id,
			       __u64 device_generation,
			       __u64 evidence_generation)
{
	struct ctlv_event *event;
	__u64 device = 0, evidence = 0;

	ASSERT_EQ(0, ctlv_event_wait(dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(dev, &event));
	EXPECT_EQ(op_id, ctlv_event_op_id(event));
	EXPECT_TRUE(ctlv_attestation_device_generation_get(
		ctlv_event_root(event), &device));
	if (ctlv_attestation_evidence_state_changed_is(event))
		ctlv_attestation_evidence_state_changed_evidence_generation_get(
			ctlv_event_root(event), &evidence);
	EXPECT_EQ(device_generation, device);
	EXPECT_EQ(evidence_generation, evidence);
	ctlv_event_free(event);
}

/* How often the provider was entered through @callback, or -1. */
static int callback_count(const char *name, const char *callback)
{
	char status[512], key[64];
	const char *line;
	int count;

	if (fixture_read(name, "status", status, sizeof(status)))
		return -1;
	snprintf(key, sizeof(key), "\n%s ", callback);
	line = strstr(status, key);
	if (!line || sscanf(line + strlen(key), "%d", &count) != 1)
		return -1;
	return count;
}

/* Exactly one of the two lists holds @format; @vendor says which. */
static const struct {
	__u64 class;
	bool vendor;
	__u64 format;
	__u64 max_length;
	bool instance;
	bool tcb_digest;
} sim_objects[] = {
	[SIM_REPORT] = { CTLV_ATTESTATION_EVIDENCE_CLASS_SIGNED, true,
			 CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_REPORT,
			 EVIDENCE_MAX, false, true },
	[SIM_TOKEN] = { CTLV_ATTESTATION_EVIDENCE_CLASS_SIGNED, true,
			CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_TOKEN,
			1024, false, false },
	[SIM_LOG] = { CTLV_ATTESTATION_EVIDENCE_CLASS_EVENT_LOG, false,
		      CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL, 65, false, false },
	[SIM_MANIFEST] = { CTLV_ATTESTATION_EVIDENCE_CLASS_AUXILIARY, true,
			   CTLV_ATTESTATION_SIM_EVIDENCE_VENDOR_FORMAT_MANIFEST,
			   1024, true, false },
};

TEST_F(attestation, evidence_objects_listed)
{
	const struct ctlv_hdr *caps, *evidence, *object, *hdr;
	unsigned int count = 0, formats, vendor_formats;
	struct ctlv_reply *reply;
	__u64 id, value;
	bool known;

	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error));
	caps = ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_device_get_caps_evidence_get(caps);
	ASSERT_NE(NULL, evidence);
	ctlv_attestation_device_get_evidence_objects_for_each(object,
							      evidence) {
		ASSERT_TRUE(ctlv_attestation_device_get_objects_id_get(object,
								       &id));
		ASSERT_EQ(count, id);
		ASSERT_GT(SIM_OBJECTS, id);
		ASSERT_TRUE(ctlv_attestation_device_get_objects_class_get(
			object, &value, &known));
		EXPECT_EQ(sim_objects[id].class, value);
		formats = 0;
		ctlv_attestation_device_get_objects_formats_for_each(hdr,
								     object) {
			EXPECT_TRUE(ctlv_attestation_device_get_objects_formats_get(
				hdr, &value, &known));
			EXPECT_EQ(sim_objects[id].format, value);
			formats++;
		}
		vendor_formats = 0;
		ctlv_attestation_device_get_objects_vendor_formats_for_each(
			hdr, object) {
			EXPECT_TRUE(ctlv_attestation_device_get_objects_vendor_formats_get(
				hdr, &value));
			EXPECT_EQ(sim_objects[id].format, value);
			vendor_formats++;
		}
		EXPECT_EQ(!sim_objects[id].vendor, formats);
		EXPECT_EQ(sim_objects[id].vendor, vendor_formats);
		ASSERT_TRUE(ctlv_attestation_device_get_objects_max_length_get(
			object, &value));
		EXPECT_EQ(sim_objects[id].max_length, value);
		EXPECT_EQ(sim_objects[id].instance,
			  ctlv_attestation_device_get_objects_instance_get(
				  object, &value));
		if (sim_objects[id].instance)
			EXPECT_EQ(1, value);
		EXPECT_EQ(sim_objects[id].tcb_digest,
			  ctlv_attestation_device_get_objects_tcb_digest_algo_get(
				  object, &value, &known));
		if (sim_objects[id].tcb_digest)
			EXPECT_EQ(HASH_ALGO_SHA384, value);
		count++;
	}
	EXPECT_EQ(SIM_OBJECTS, count);
	ctlv_reply_free(reply);
}

TEST_F(attestation, evidence_objects_and_options)
{
	static union {
		__u64 alignment;
		unsigned char data[SIM_OBJECTS][EVIDENCE_MAX];
	} blobs;
	/* The one length both signed objects take. */
	static const unsigned char challenge[32] = "options";
	static const unsigned char guid[16] = { 1, 2, 3, 4 };
	const struct ctlv_hdr *result, *digest;
	__u64 lengths[SIM_OBJECTS] = {};
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	unsigned int count = 0;
	__u64 id, value = 0;
	const void *data;
	__u32 length;
	bool known;

	ASSERT_EQ(0, ctlv_attestation_evidence_get_request_new(&request));
	ctlv_attestation_evidence_get_challenge_put(request, challenge,
						    sizeof(challenge));
	for (id = 0; id < SIM_OBJECTS; id++)
		evidence_object_put(request, id, blobs.data[id], EVIDENCE_MAX);
	ctlv_attestation_evidence_get_selection_start(request);
	ctlv_attestation_evidence_get_selection_id_put(request, 1);
	ctlv_attestation_evidence_get_selection_hash_algo_put(request,
							      HASH_ALGO_SHA256);
	ctlv_record_end(request);
	ctlv_attestation_evidence_get_privlevel_put(request, 7);
	ctlv_attestation_evidence_get_service_provider_put(
		request, "example-service");
	ctlv_attestation_evidence_get_service_guid_put(
		request, guid, sizeof(guid));
	ctlv_attestation_evidence_get_service_manifest_version_put(request, 9);
	ASSERT_EQ(0, ctlv_msg_err(request));
	ASSERT_EQ(0, ctlv_attestation_evidence_get(
			  self->dev, request, &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_msg_free(request);
	ctlv_attestation_evidence_get_results_for_each(result,
						       ctlv_reply_root(reply)) {
		ASSERT_GT(SIM_OBJECTS, count);
		if (sim_objects[count].vendor) {
			EXPECT_FALSE(ctlv_attestation_evidence_get_results_format_present(
				result));
			EXPECT_TRUE(ctlv_attestation_evidence_get_results_vendor_format_get(
				result, &value));
		} else {
			EXPECT_FALSE(ctlv_attestation_evidence_get_results_vendor_format_present(
				result));
			EXPECT_TRUE(ctlv_attestation_evidence_get_results_format_get(
				result, &value, &known));
		}
		EXPECT_EQ(sim_objects[count].format, value);
		ASSERT_TRUE(ctlv_attestation_evidence_get_results_length_get(
			result, &lengths[count]));
		EXPECT_GT(lengths[count], 0);
		digest = ctlv_attestation_evidence_get_results_tcb_digest_get(
			result);
		EXPECT_EQ(sim_objects[count].tcb_digest, !!digest);
		if (digest) {
			EXPECT_TRUE(ctlv_attestation_evidence_get_tcb_digest_hash_algo_get(
				digest, &value, &known));
			EXPECT_EQ(HASH_ALGO_SHA384, value);
			EXPECT_TRUE(ctlv_attestation_evidence_get_tcb_digest_data_get(
				digest, &data, &length));
			EXPECT_EQ(48, length);
		}
		count++;
	}
	EXPECT_EQ(SIM_OBJECTS, count);
	EXPECT_EQ(0, memcmp(blobs.data[SIM_REPORT], "SIM-REPORT-v1", 13));
	EXPECT_EQ(0, memcmp(blobs.data[SIM_TOKEN], "SIM-TOKEN-v1", 12));
	EXPECT_EQ(0, memcmp(blobs.data[SIM_TOKEN] + 12, challenge,
			    sizeof(challenge)));
	EXPECT_EQ(0, memcmp(blobs.data[SIM_LOG] + TCG_SPEC_ID_SIGNATURE_OFFSET,
			    "Spec ID Event03", 16));
	EXPECT_NE(NULL, memmem(blobs.data[SIM_MANIFEST], lengths[SIM_MANIFEST],
			       self->name, strlen(self->name)));
	ctlv_reply_free(reply);
}

/* The length of the one object an evidence-get reply reports. */
static __u64 evidence_length(struct ctlv_reply *reply)
{
	const struct ctlv_hdr *root = ctlv_reply_root(reply), *hdr;
	__u64 length = 0;

	ctlv_attestation_evidence_get_results_for_each(hdr, root)
		ctlv_attestation_evidence_get_results_length_get(hdr, &length);
	ctlv_reply_free(reply);
	return length;
}

/* Any blob the object fits in takes it, and a shorter one says what fits. */
TEST_F(attestation, evidence_blob_lengths)
{
	static const unsigned char challenge[] = "blob lengths";
	struct ctlv_reply *reply;
	__u64 length;

	ASSERT_EQ(0, evidence_get(self->dev, challenge, sizeof(challenge) - 1,
				  EVIDENCE_MAX, &reply, &self->error));
	length = evidence_length(reply);
	ASSERT_GT(length, 1);
	ASSERT_GT(EVIDENCE_MAX, length);

	EXPECT_EQ(-EMSGSIZE,
		  evidence_get(self->dev, challenge, sizeof(challenge) - 1,
			       length - 1, &reply, &self->error));
	EXPECT_EQ(CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR,
		  self->error.reason);
	ASSERT_TRUE(self->error.present & CTLV_ERROR_HAS_REQUIRED_LENGTH);
	EXPECT_EQ(length, self->error.required_length);

	ASSERT_EQ(0, evidence_get(self->dev, challenge, sizeof(challenge) - 1,
				  length, &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_EQ(length, evidence_length(reply));
}

/* An ID the device does not list is unknown, and an object is named once. */
TEST_F(attestation, evidence_object_ids)
{
	static const __u64 unknown[] = { SIM_OBJECTS }, beyond[] = { 16 };
	static const __u64 twice[] = { SIM_REPORT, SIM_REPORT };
	static const unsigned char challenge[32] = "ids";
	struct ctlv_reply *reply;

	EXPECT_EQ(-ENOENT, evidence_ids_get(self->dev, NULL, 0, unknown, 1,
					    EVIDENCE_MAX, &reply,
					    &self->error));
	EXPECT_EQ(-EINVAL, evidence_ids_get(self->dev, NULL, 0, beyond, 1,
					    EVIDENCE_MAX, &reply,
					    &self->error));
	EXPECT_EQ(-EINVAL, evidence_ids_get(self->dev, challenge,
					    sizeof(challenge), twice, 2,
					    EVIDENCE_MAX, &reply,
					    &self->error));
}

/* An entry's length, min-length and max-length, all ones where absent. */
static void challenge_length_get(const struct ctlv_hdr *entry, __u64 values[3])
{
	__u64 length, min, max;

	if (!ctlv_attestation_device_get_challenge_lengths_length_get(entry,
								      &length))
		length = -1;
	if (!ctlv_attestation_device_get_challenge_lengths_min_length_get(entry,
									  &min))
		min = -1;
	if (!ctlv_attestation_device_get_challenge_lengths_max_length_get(entry,
									  &max))
		max = -1;
	values[0] = length;
	values[1] = min;
	values[2] = max;
}

/* A challenge goes with signed objects only, and has to suit each named. */
TEST_F(attestation, challenge_lengths)
{
	static const unsigned char challenge[129] = "lengths";
	/* An object's ID, then its entry's length, min-length and max-length. */
	static const __u64 entries[][4] = {
		{ SIM_REPORT, -1, 1, 64 },
		{ SIM_REPORT, 128, -1, -1 },
		{ SIM_TOKEN, 32, -1, -1 },
	};
	static const __u64 report[] = { SIM_REPORT };
	static const __u64 both[] = { SIM_REPORT, SIM_TOKEN };
	static const __u64 log[] = { SIM_LOG };
	static const struct {
		const __u64 *ids;
		size_t count;
		bool omit;
		__u32 length;
		int error;
	} requests[] = {
		{ report, 1, true, 0, -EINVAL },
		{ report, 1, false, 0, -EINVAL },
		{ report, 1, false, 1, 0 },
		{ report, 1, false, 64, 0 },
		{ report, 1, false, 65, -EINVAL },
		{ report, 1, false, 127, -EINVAL },
		{ report, 1, false, 128, 0 },
		{ report, 1, false, 129, -EINVAL },
		{ both, 2, false, 32, 0 },
		{ both, 2, false, 64, -EINVAL },
		{ log, 1, true, 0, 0 },
		{ log, 1, false, 32, -EINVAL },
	};
	const struct ctlv_hdr *caps, *evidence, *object, *entry;
	struct ctlv_reply *reply;
	unsigned int count = 0;
	__u64 values[4];
	size_t index;
	int ret;

	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error));
	caps = ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_device_get_caps_evidence_get(caps);
	ASSERT_NE(NULL, evidence);
	ctlv_attestation_device_get_evidence_objects_for_each(object,
							      evidence) {
		ASSERT_TRUE(ctlv_attestation_device_get_objects_id_get(
			object, &values[0]));
		ctlv_attestation_device_get_objects_challenge_lengths_for_each(
			entry, object) {
			ASSERT_GT(ARRAY_SIZE(entries), count);
			challenge_length_get(entry, &values[1]);
			EXPECT_EQ(0, memcmp(entries[count], values,
					    sizeof(values)));
			count++;
		}
	}
	EXPECT_EQ(ARRAY_SIZE(entries), count);
	ctlv_reply_free(reply);

	for (index = 0; index < ARRAY_SIZE(requests); index++) {
		ret = evidence_ids_get(self->dev,
				       requests[index].omit ? NULL : challenge,
				       requests[index].length,
				       requests[index].ids,
				       requests[index].count, EVIDENCE_MAX,
				       &reply, &self->error);
		EXPECT_EQ(requests[index].error, ret) {
			TH_LOG("request %zu: %s", index,
			       error_text(&self->error));
		}
		if (!ret)
			ctlv_reply_free(reply);
	}
}

static int devsec_state(struct ctlv_dev *dev, __u64 *state,
			__u64 *device_generation, struct ctlv_error *error)
{
	struct ctlv_reply *reply;
	bool known;
	int ret;

	ret = ctlv_attestation_devsec_state_get(dev, NULL, &reply, error);
	if (ret)
		return ret;
	if (!ctlv_attestation_devsec_state_get_tdisp_state_get(
		    ctlv_reply_root(reply), state, &known) || !known ||
	    !ctlv_attestation_device_generation_get(ctlv_reply_root(reply),
						    device_generation))
		ret = -EPROTO;
	ctlv_reply_free(reply);
	return ret;
}

TEST_F(attestation, evidence_accept_runs_device)
{
	static const unsigned char sha256[32] = { 11 };
	static const unsigned char sha384[48] = { 12 };
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED,
		CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED,
	};
	__u64 device = 0, evidence = 0, stale = 0, later = 0;
	__u64 generation = 0, state = 0;
	struct ctlv_reply *reply;
	int calls;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	/* Nothing runs an unlocked device. */
	EXPECT_EQ(-EOPNOTSUPP, evidence_accept(self->dev, 0, 1, &generation,
					       &self->error));
	ASSERT_EQ(0, fixture_file(self->name, "state", "LOCKED"));
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED, 1, 0);

	/* The evidence reports MRs extended since. */
	ASSERT_EQ(0, evidence_collect(self->dev, &device, &stale,
				      &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED, 1, 1);
	ASSERT_EQ(0, mr_extend(self->dev, 1, sha256, sha384, NULL, NULL, 0,
			       &reply, &self->error));
	ctlv_reply_free(reply);
	EXPECT_EQ(-ESTALE, evidence_accept(self->dev, device, stale,
					   &generation, &self->error));

	/* Evidence collected later leaves this one current. */
	ASSERT_EQ(0, evidence_collect(self->dev, &device, &evidence,
				      &self->error));
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED, 1, 2);
	ASSERT_EQ(0, evidence_collect(self->dev, &device, &later,
				      &self->error));
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED, 1, 3);

	/* Failing before the device moved leaves it locked and unchanged. */
	ASSERT_EQ(0, fixture_file(self->name, "fail-next",
				  "evidence_accept EIO"));
	EXPECT_EQ(-EIO, evidence_accept(self->dev, device, evidence,
					&generation, &self->error));
	ASSERT_EQ(0, devsec_state(self->dev, &state, &generation,
				  &self->error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_LOCKED, state);
	EXPECT_EQ(1, generation);

	ASSERT_EQ(0, evidence_accept(self->dev, device, evidence, &generation,
				     &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_EQ(2, generation);
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED, 2, 0);
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_EVIDENCE_STATE_CHANGED, 2, 0);
	ASSERT_EQ(0, devsec_state(self->dev, &state, &generation,
				  &self->error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_RUN, state);
	EXPECT_EQ(2, generation);
	EXPECT_EQ(-EOPNOTSUPP, evidence_accept(self->dev, device, evidence,
					       &generation, &self->error));

	/* Locked again, no earlier evidence reaches the provider. */
	ASSERT_EQ(0, fixture_file(self->name, "state", "LOCKED"));
	expect_state_event(_metadata, self->dev,
			   CTLV_ATTESTATION_OP_DEVICE_STATE_CHANGED, 3, 0);
	calls = callback_count(self->name, "evidence_accept");
	ASSERT_LE(0, calls);
	EXPECT_EQ(-ESTALE, evidence_accept(self->dev, device, later,
					   &generation, &self->error));
	EXPECT_EQ(-ESTALE, evidence_accept(self->dev, 3, later, &generation,
					   &self->error));
	EXPECT_EQ(calls, callback_count(self->name, "evidence_accept"));
}

TEST_F(attestation, fixture_isolation)
{
	char other_name[32];
	struct ctlv_error other_error = {};
	struct ctlv_dev *other;
	__u64 state, generation;

	snprintf(other_name, sizeof(other_name), "other-%d", getpid());
	ASSERT_EQ(0, fixture_control("create", other_name));
	other = fixture_open(other_name, &other_error);
	ASSERT_NE(NULL, other);
	ASSERT_EQ(0, fixture_file(self->name, "state", "LOCKED"));
	ASSERT_EQ(0, fixture_file(other_name, "state", "ERROR"));
	ASSERT_EQ(0, devsec_state(self->dev, &state, &generation,
				  &self->error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_LOCKED, state);
	EXPECT_EQ(1, generation);
	ASSERT_EQ(0, devsec_state(other, &state, &generation, &other_error));
	EXPECT_EQ(CTLV_ATTESTATION_TDISP_STATE_ERROR, state);
	EXPECT_EQ(1, generation);
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
	struct ctlv_queue queue;
	bool found = false;
	unsigned int index, extensions;

	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, ctlv_queue_get(self->dev, &queue, &self->error));
	extensions = queue.len / sizeof(record) + 16;
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

/* The shortest reply buffer @op_id takes on @dev, zero when it cannot say. */
static __u32 reply_min_len(struct ctlv_dev *dev, __u64 op_id)
{
	const struct ctlv_op_info *info;
	struct ctlv_ops *ops;
	__u32 len = 0;

	if (ctlv_ops_get(dev, &ops, NULL))
		return 0;
	info = ctlv_ops_find(ops, op_id);
	if (info)
		len = info->query_reply_min_len;
	ctlv_ops_free(ops);
	return len;
}

/* A log larger than a fragment arrives whole, each entry once and in order. */
TEST_F(attestation, log_across_fragments)
{
	const __u32 len = reply_min_len(self->dev,
					CTLV_ATTESTATION_OP_MR_LOG_GET);
	static unsigned char record[4096];
	unsigned int index, extensions = 2 * len / sizeof(record);
	unsigned char sha256[32] = { 9 };
	unsigned char sha384[48] = { 10 };
	const struct ctlv_hdr *entry;
	struct ctlv_msg *request;
	struct ctlv_reply *reply;
	__u64 generation, expected = 1;

	ASSERT_LT(0, len);
	for (index = 0; index < extensions; index++) {
		ASSERT_EQ(0, mr_extend(self->dev, 1, sha256, sha384,
				       "org.example.large-record.v1", record,
				       sizeof(record), &reply, &self->error));
		ctlv_reply_free(reply);
	}
	ASSERT_EQ(0, ctlv_attestation_mr_log_get_request_new(&request));
	ASSERT_EQ(0, ctlv_query_sized(self->dev, CTLV_ATTESTATION_OP_MR_LOG_GET,
				      request, len, &reply, &self->error));
	ctlv_msg_free(request);
	ctlv_attestation_mr_log_get_entries_for_each(entry,
						     ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_generation_get(entry,
							       &generation));
		EXPECT_EQ(expected, generation);
		expected++;
	}
	EXPECT_EQ(2 + extensions + 1, expected);
	ctlv_reply_free(reply);
}

/* Any fragment that fits the largest MR fits every MR, one at a time. */
TEST_F(attestation, mr_get_across_fragments)
{
	const struct ctlv_hdr *item;
	struct ctlv_reply *reply;
	__u32 largest = 0, total = 0, len, min_len;
	unsigned int count = 0;
	__u64 id, expected;

	min_len = reply_min_len(self->dev, CTLV_ATTESTATION_OP_MR_GET);
	ASSERT_LT(0, min_len);
	ASSERT_EQ(0, ctlv_attestation_mr_get(self->dev, NULL, &reply,
					     &self->error));
	ctlv_attestation_mr_get_items_for_each(item, ctlv_reply_root(reply)) {
		if (CTLV_ALIGN(item->len) > largest)
			largest = CTLV_ALIGN(item->len);
		total += CTLV_ALIGN(item->len);
		count++;
	}
	ctlv_reply_free(reply);
	len = sizeof(struct ctlv_hdr) + largest;
	if (len < min_len)
		len = min_len;
	for (; len < sizeof(struct ctlv_hdr) + total; len += CTLV_ALIGNTO) {
		ASSERT_EQ(0, ctlv_query_sized(self->dev,
					      CTLV_ATTESTATION_OP_MR_GET, NULL,
					      len, &reply, &self->error));
		expected = 0;
		ctlv_attestation_mr_get_items_for_each(
			item, ctlv_reply_root(reply)) {
			ASSERT_TRUE(ctlv_attestation_mr_get_items_id_get(item,
									 &id));
			EXPECT_EQ(expected, id);
			expected++;
		}
		EXPECT_EQ(count, expected);
		ctlv_reply_free(reply);
	}
}

TEST_F(attestation, dynamic_topology)
{
	const struct ctlv_hdr *item;
	struct ctlv_reply *reply;
	unsigned int count = 0;

	ASSERT_EQ(0, fixture_file(self->name, "mr-add", "9 sha256"));
	ASSERT_EQ(0, ctlv_attestation_mr_get(
			  self->dev, NULL, &reply, &self->error));
	ctlv_attestation_mr_get_items_for_each(item, ctlv_reply_root(reply))
		count++;
	EXPECT_EQ(3, count);
	EXPECT_EQ(3, ctlv_reply_generation(reply));
	ctlv_reply_free(reply);
	ASSERT_EQ(0, fixture_file(self->name, "mr-remove", "9"));
	ASSERT_EQ(0, ctlv_attestation_mr_get(
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
