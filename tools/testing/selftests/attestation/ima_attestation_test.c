// SPDX-License-Identifier: GPL-2.0
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <linux/hash_info.h>

#include "../kselftest_harness.h"
#include "ctlv_attestation_sim_user_gen.h"

#define IMA_SECURITYFS "/sys/kernel/security/ima"
#define IMA_BINARY_SHA256 IMA_SECURITYFS "/binary_runtime_measurements_sha256"
#define IMA_BINARY_SHA384 IMA_SECURITYFS "/binary_runtime_measurements_sha384"
#define IMA_ASCII_SHA256 IMA_SECURITYFS "/ascii_runtime_measurements_sha256"
#define IMA_ASCII_SHA384 IMA_SECURITYFS "/ascii_runtime_measurements_sha384"
#define IMA_COUNT IMA_SECURITYFS "/runtime_measurements_count"
#define IMA_POLICY IMA_SECURITYFS "/policy"
#define READ_LIMIT (16 * 1024 * 1024)

static const char *error_text(const struct ctlv_error *error)
{
	static char text[512];

	return ctlv_error_str(error, text, sizeof(text));
}

static int read_all_fd(int fd, unsigned char **data, size_t *length)
{
	unsigned char *buffer = NULL;
	size_t capacity = 0;
	size_t used = 0;

	for (;;) {
		ssize_t ret;

		if (used == capacity) {
			size_t next = capacity ? capacity * 2 : 4096;
			void *resized;

			if (next > READ_LIMIT) {
				free(buffer);
				return -E2BIG;
			}
			resized = realloc(buffer, next + 1);
			if (!resized) {
				free(buffer);
				return -ENOMEM;
			}
			buffer = resized;
			capacity = next;
		}
		ret = read(fd, buffer + used, capacity - used);
		if (ret < 0) {
			ret = -errno;
			free(buffer);
			return ret;
		}
		if (!ret)
			break;
		used += ret;
	}
	if (!buffer) {
		buffer = calloc(1, 1);
		if (!buffer)
			return -ENOMEM;
	}
	buffer[used] = '\0';
	*data = buffer;
	*length = used;
	return 0;
}

static int read_all(const char *path, unsigned char **data, size_t *length)
{
	int fd;
	int ret;

	fd = open(path, O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	ret = read_all_fd(fd, data, length);
	close(fd);
	return ret;
}

static int read_count(unsigned long *count)
{
	unsigned char *text;
	size_t length;
	char *end;
	int ret;

	ret = read_all(IMA_COUNT, &text, &length);
	if (ret)
		return ret;
	errno = 0;
	*count = strtoul((char *)text, &end, 10);
	if (errno || end == (char *)text)
		ret = -EINVAL;
	free(text);
	return ret;
}

static int write_policy(const char *rule)
{
	size_t length = strlen(rule);
	ssize_t written;
	int saved_errno;
	int fd;

	fd = open(IMA_POLICY, O_WRONLY | O_CLOEXEC);
	if (fd < 0)
		return -errno;
	written = write(fd, rule, length);
	saved_errno = errno;
	close(fd);
	if (written < 0)
		return -saved_errno;
	return written == length ? 0 : -EIO;
}

static size_t substring_count(const char *text, const char *needle)
{
	size_t count = 0;
	size_t length = strlen(needle);

	while ((text = strstr(text, needle))) {
		count++;
		text += length;
	}
	return count;
}

static struct ctlv_dev *fixture_open(const char *name,
				     struct ctlv_error *error)
{
	unsigned int which;

	for (which = 0; which < 32; which++) {
		const struct ctlv_attr *info;
		const struct ctlv_attr *root;
		struct ctlv_reply *reply;
		struct ctlv_dev *dev;
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
		root = ctlv_reply_root(reply);
		info = ctlv_attestation_device_info_get(root);
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

static int log_get(struct ctlv_dev *dev, struct ctlv_reply **reply,
		   struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_log_get_request_new(&request);
	if (ret)
		return ret;
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_log_get(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static int mr_extend(struct ctlv_dev *dev, const unsigned char sha256[32],
		     const unsigned char sha384[48], const char *format,
		     const void *data, size_t data_length,
		     struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_extend_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_id_put(request, 1);
	ctlv_attestation_mr_value_start(request);
	ctlv_attestation_mr_hash_algo_put(request, HASH_ALGO_SHA256);
	ctlv_attestation_mr_data_put(request, sha256, 32);
	ctlv_nest_end(request);
	ctlv_attestation_mr_value_start(request);
	ctlv_attestation_mr_hash_algo_put(request, HASH_ALGO_SHA384);
	ctlv_attestation_mr_data_put(request, sha384, 48);
	ctlv_nest_end(request);
	ctlv_attestation_record_item_start(request);
	ctlv_attestation_record_format_put(request, format);
	ctlv_attestation_record_data_put(request, data, data_length);
	ctlv_nest_end(request);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_extend(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

static uint32_t decode_u32(const unsigned char *data, bool canonical)
{
	uint32_t value;

	memcpy(&value, data, sizeof(value));
	return canonical ? le32toh(value) : value;
}

static bool binary_has_record(const unsigned char *data, size_t length,
			      const unsigned char *digest,
			      size_t digest_length,
			      const unsigned char *payload,
			      size_t payload_length)
{
	size_t offset = 0;
	bool canonical = false;

	while (length - offset >= sizeof(uint32_t) + digest_length +
	       sizeof(uint32_t)) {
		const unsigned char *entry_digest;
		const unsigned char *entry_payload;
		uint32_t index;
		uint32_t name_length;
		uint32_t data_length;
		const char *name;

		index = decode_u32(data + offset, canonical);
		if (!offset && index != 1) {
			canonical = true;
			index = decode_u32(data + offset, true);
		}
		if (index != 1)
			return false;
		offset += sizeof(index);
		entry_digest = data + offset;
		offset += digest_length;
		name_length = decode_u32(data + offset, canonical);
		offset += sizeof(name_length);
		if (!name_length || name_length > 128 ||
		    name_length > length - offset)
			return false;
		name = (const char *)data + offset;
		offset += name_length;
		if (name_length == strlen("ima") &&
		    !memcmp(name, "ima", name_length))
			return false;
		if (length - offset < sizeof(data_length))
			return false;
		data_length = decode_u32(data + offset, canonical);
		offset += sizeof(data_length);
		if (data_length > length - offset)
			return false;
		entry_payload = data + offset;
		offset += data_length;
		if (name_length == strlen("ima-mr-extend") &&
		    !memcmp(name, "ima-mr-extend", name_length) &&
		    !memcmp(entry_digest, digest, digest_length) &&
		    data_length == payload_length &&
		    !memcmp(entry_payload, payload, payload_length))
			return true;
	}
	return false;
}

static size_t record_bytes(unsigned char *output, const char *format,
			   const void *data, size_t data_length)
{
	static const char prefix[] = "attestation-mr-record-v1";
	uint32_t length;
	size_t offset = 0;

	memcpy(output + offset, prefix, sizeof(prefix));
	offset += sizeof(prefix);
	length = htole32(strlen(format));
	memcpy(output + offset, &length, sizeof(length));
	offset += sizeof(length);
	memcpy(output + offset, format, strlen(format));
	offset += strlen(format);
	length = htole32(data_length);
	memcpy(output + offset, &length, sizeof(length));
	offset += sizeof(length);
	memcpy(output + offset, data, data_length);
	return offset + data_length;
}

FIXTURE(ima_attestation) {
	struct ctlv_dev *dev;
	struct ctlv_error error;
};

FIXTURE_SETUP(ima_attestation)
{
	const char *name = getenv("IMA_ATTESTATION_TEST_FIXTURE");

	if (access(IMA_ASCII_SHA256, R_OK) ||
	    access(IMA_ASCII_SHA384, R_OK))
		SKIP(return, "IMA attestation projection is unavailable");
	if (!name)
		name = "ima-test";
	self->dev = fixture_open(name, &self->error);
	if (!self->dev)
		SKIP(return, "early attestation fixture '%s' is unavailable",
		     name);
}

FIXTURE_TEARDOWN(ima_attestation)
{
	if (self->dev)
		ctlv_close(self->dev);
}

TEST_F(ima_attestation, target_and_initial_projection)
{
	const struct ctlv_attr *item;
	struct ctlv_reply *reply;
	unsigned char *sha256;
	unsigned char *sha384;
	unsigned long count;
	size_t sha256_length;
	size_t sha384_length;
	__u64 flags, unknown;
	__u64 id;
	bool found = false;
	bool got;

	ASSERT_EQ(0, ctlv_attestation_mr_dump(self->dev, NULL, &reply,
					      &self->error));
	ctlv_attestation_mr_for_each_item(item, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_id_get(item, &id));
		if (id != 1)
			continue;
		got = ctlv_attestation_mr_state_flags_get(item, &flags, &unknown);
		ASSERT_TRUE(got);
		EXPECT_EQ(0, unknown);
		EXPECT_TRUE(flags & CTLV_ATTESTATION_MR_STATE_FLAGS_IMA_ACTIVE);
		found = true;
	}
	EXPECT_TRUE(found);
	ctlv_reply_free(reply);

	EXPECT_EQ(-1, access(IMA_SECURITYFS "/ascii_runtime_measurements",
			     F_OK));
	EXPECT_EQ(ENOENT, errno);
	ASSERT_EQ(0, read_all(IMA_ASCII_SHA256, &sha256, &sha256_length));
	ASSERT_EQ(0, read_all(IMA_ASCII_SHA384, &sha384, &sha384_length));
	EXPECT_GT(sha256_length, 0);
	EXPECT_GT(sha384_length, 0);
	EXPECT_EQ(1, substring_count((char *)sha256, "boot_aggregate"));
	EXPECT_EQ(1, substring_count((char *)sha384, "boot_aggregate"));
	ASSERT_EQ(0, read_count(&count));
	EXPECT_GT(count, 0);
	free(sha384);
	free(sha256);
}

TEST_F(ima_attestation, journal_contains_ima_metadata)
{
	const struct ctlv_attr *entry;
	struct ctlv_reply *reply;
	unsigned int ima_entries = 0;

	ASSERT_EQ(0, log_get(self->dev, &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_attestation_log_for_each_entry(entry, ctlv_reply_root(reply)) {
		const struct ctlv_attr *ima;
		const char *name;

		ima = ctlv_attestation_log_ima_get(entry);
		if (!ima)
			continue;
		ASSERT_TRUE(ctlv_attestation_log_template_name_get(ima, &name));
		EXPECT_NE('\0', name[0]);
		ima_entries++;
	}
	EXPECT_GT(ima_entries, 0);
	ctlv_reply_free(reply);
}

TEST_F(ima_attestation, userspace_extension_is_projected)
{
	static const char format[] = "org.kernel.ima-selftest.v1";
	static const unsigned char record[] = "record";
	unsigned char sha256[32] = { 0x11 };
	unsigned char sha384[48] = { 0x22 };
	unsigned char payload[256];
	unsigned char *binary256;
	unsigned char *binary384;
	unsigned char *discard;
	struct ctlv_reply *reply;
	unsigned long before;
	unsigned long after;
	size_t binary256_length;
	size_t binary384_length;
	size_t discard_length;
	size_t payload_length;
	int fd256;
	int fd384;

	fd256 = open(IMA_BINARY_SHA256, O_RDONLY | O_CLOEXEC);
	ASSERT_GE(fd256, 0);
	fd384 = open(IMA_BINARY_SHA384, O_RDONLY | O_CLOEXEC);
	ASSERT_GE(fd384, 0);
	ASSERT_EQ(0, read_all_fd(fd256, &discard, &discard_length));
	free(discard);
	ASSERT_EQ(0, read_all_fd(fd384, &discard, &discard_length));
	free(discard);
	ASSERT_EQ(0, read_count(&before));

	ASSERT_EQ(0, mr_extend(self->dev, sha256, sha384, format,
			       record, sizeof(record) - 1,
			       &reply, &self->error)) {
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_reply_free(reply);
	ASSERT_EQ(0, read_all_fd(fd256, &binary256, &binary256_length));
	ASSERT_EQ(0, read_all_fd(fd384, &binary384, &binary384_length));
	close(fd384);
	close(fd256);

	payload_length = record_bytes(payload, format, record,
				      sizeof(record) - 1);
	EXPECT_TRUE(binary_has_record(binary256, binary256_length, sha256,
				      sizeof(sha256), payload,
				      payload_length));
	EXPECT_TRUE(binary_has_record(binary384, binary384_length, sha384,
				      sizeof(sha384), payload,
				      payload_length));
	ASSERT_EQ(0, read_count(&after));
	EXPECT_GE(after, before + 1);
	free(binary384);
	free(binary256);
}

TEST_F(ima_attestation, policy_assertions)
{
	char rule[160];

	if (access(IMA_POLICY, W_OK))
		SKIP(return, "IMA policy is not writable");

	EXPECT_EQ(-EINVAL,
		  write_policy("measure func=FILE_CHECK pcr=10\n"));
	EXPECT_EQ(-EINVAL,
		  write_policy("measure func=FILE_CHECK mr-id=1\n"));
	snprintf(rule, sizeof(rule),
		 "measure func=FILE_CHECK attestation=%s mr-id=2\n",
		 "ctlv-attestation-sim");
	EXPECT_EQ(-EINVAL, write_policy(rule));
	snprintf(rule, sizeof(rule),
		 "measure func=FILE_CHECK attestation-bus=%s mr-id=1\n",
		 "platform");
	EXPECT_EQ(-EINVAL, write_policy(rule));
	snprintf(rule, sizeof(rule),
		 "measure func=FILE_CHECK attestation=%s mr-id=1\n",
		 "ctlv-attestation-sim");
	ASSERT_EQ(0, write_policy(rule));
}

TEST_HARNESS_MAIN
