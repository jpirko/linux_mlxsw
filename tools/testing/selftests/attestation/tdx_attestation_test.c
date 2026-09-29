// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <linux/hash_info.h>

#include "../kselftest_harness.h"
#include "ctlv_attestation_tdx_user_gen.h"

#define EVENT_WAIT_MS 5000
#define TDX_REPORTDATA_LEN 64
#define TDX_REPORT_LEN 1024
#define TDX_REPORTDATA_OFFSET 128
#define TDX_QUOTE_MAX_LEN ((128 * 1024) - 24)

static const char *error_text(const struct ctlv_error *error)
{
	static char text[512];

	return ctlv_error_str(error, text, sizeof(text));
}

FIXTURE(tdx_attestation)
{
	struct ctlv_dev *dev;
	struct ctlv_error error;
};

FIXTURE_SETUP(tdx_attestation)
{
	int ret;

	ret = ctlv_open("ctlv-attestation-tdx", 0, O_RDWR, &self->dev);
	if (ret)
		SKIP(return, "no TDX attestation device: %s", strerror(-ret));
}

FIXTURE_TEARDOWN(tdx_attestation)
{
	if (self->dev)
		ctlv_close(self->dev);
}

TEST_F(tdx_attestation, discovery_and_construction_fields)
{
	const struct ctlv_attr *caps, *evidence, *mr, *info;
	const void *value;
	struct ctlv_reply *reply;
	__u64 flags, unknown;
	__u32 length;
	bool got;

	ASSERT_EQ(4, ctlv_chain_len(self->dev));
	EXPECT_TRUE(ctlv_dev_inherits(self->dev, "ctlv-attestation"));
	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error))
	{
		TH_LOG("%s", error_text(&self->error));
	}
	caps =
		ctlv_attestation_device_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_cap_evidence_get(caps);
	mr = ctlv_attestation_cap_mr_get(caps);
	ASSERT_NE(NULL, evidence);
	ASSERT_NE(NULL, mr);
	got = ctlv_attestation_cap_evidence_flags_get(evidence, &flags,
						      &unknown);
	ASSERT_TRUE(got);
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION, flags);
	EXPECT_EQ(0, unknown);
	got = ctlv_attestation_cap_mr_flags_get(mr, &flags, &unknown);
	ASSERT_TRUE(got);
	EXPECT_EQ(CTLV_ATTESTATION_MR_DEVICE_CAPS_READ |
			  CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND,
		  flags);
	EXPECT_EQ(0, unknown);
	info = ctlv_attestation_device_info_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, info);
	got =
		ctlv_attestation_tdx_info_mrconfigid_get(info, &value, &length);
	ASSERT_TRUE(got);
	EXPECT_EQ(48, length);
	got = ctlv_attestation_tdx_info_mrowner_get(info, &value, &length);
	ASSERT_TRUE(got);
	EXPECT_EQ(48, length);
	ASSERT_TRUE(ctlv_attestation_tdx_info_mrownerconfig_get(info, &value,
								&length));
	EXPECT_EQ(48, length);
	ctlv_reply_free(reply);
}

static int tdx_mr_read(struct ctlv_dev *dev, unsigned int id,
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

TEST_F(tdx_attestation, measurement_registers)
{
	const struct ctlv_attr *item, *algorithm, *value;
	struct ctlv_reply *reply;
	const void *digest;
	__u64 id, hash;
	__u32 length;
	unsigned int count = 0;
	bool got;

	ASSERT_EQ(0, ctlv_attestation_mr_dump(self->dev, NULL, &reply,
					      &self->error));
	ctlv_attestation_mr_for_each_item(item, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_id_get(item, &id));
		EXPECT_EQ(count, id);
		algorithm = NULL;
		ctlv_attestation_mr_for_each_supported_hash_algo(algorithm, item) {
			break;
		}
		ASSERT_NE(NULL, algorithm);
		got = ctlv_attestation_mr_supported_hash_algo_get(algorithm,
								  &hash);
		ASSERT_TRUE(got);
		EXPECT_EQ(HASH_ALGO_SHA384, hash);
		count++;
	}
	EXPECT_EQ(5, count);
	ctlv_reply_free(reply);

	for (id = 0; id < 5; id++) {
		ASSERT_EQ(0, tdx_mr_read(self->dev, id, &reply, &self->error));
		value = NULL;
		ctlv_attestation_mr_for_each_value(value, ctlv_reply_root(reply)) {
			break;
		}
		ASSERT_NE(NULL, value);
		got = ctlv_attestation_mr_data_get(value, &digest, &length);
		ASSERT_TRUE(got);
		EXPECT_EQ(48, length);
		ctlv_reply_free(reply);
	}
}

static int tdx_evidence_get(struct ctlv_dev *dev, const char *format,
			    const unsigned char *challenge,
			    size_t challenge_length, void *output,
			    size_t output_length, struct ctlv_reply **reply,
			    struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_evidence_get_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_evidence_challenge_put(request, challenge,
						challenge_length);
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(request,
						 CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY);
	ctlv_attestation_evidence_format_put(request, format);
	ctlv_attestation_evidence_representation_put(request,
						     CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
	ctlv_attestation_evidence_data_put_va(request, output, output_length);
	ctlv_nest_end(request);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_get(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(tdx_attestation, tdreport_challenge_binding)
{
	static union {
		__u64 alignment;
		unsigned char bytes[TDX_REPORT_LEN];
	} report;
	unsigned char challenge[TDX_REPORTDATA_LEN];
	struct ctlv_reply *reply;
	unsigned int index;
	bool got;
	int ret;

	for (index = 0; index < sizeof(challenge); index++)
		challenge[index] = index;
	ret = tdx_evidence_get(self->dev, "tdx-tdreport-1.0", challenge,
			       sizeof(challenge), report.bytes,
			       sizeof(report.bytes), &reply, &self->error);
	ASSERT_EQ(0, ret) {
		TH_LOG("%s", error_text(&self->error));
	}
	EXPECT_EQ(0, memcmp(report.bytes + TDX_REPORTDATA_OFFSET, challenge,
			    sizeof(challenge)));
	got = ctlv_attestation_mr_generation_present(ctlv_reply_root(reply));
	EXPECT_TRUE(got);
	got = ctlv_attestation_evidence_generation_present(ctlv_reply_root(reply));
	EXPECT_FALSE(got);
	ctlv_reply_free(reply);
}

TEST_F(tdx_attestation, quote_when_service_is_available)
{
	unsigned char challenge[TDX_REPORTDATA_LEN] = {};
	struct ctlv_reply *reply;
	void *quote;
	int ret;

	ASSERT_EQ(0, posix_memalign(&quote, 8, TDX_QUOTE_MAX_LEN));
	ret = tdx_evidence_get(self->dev, "tdx-quote-1.0", challenge,
			       sizeof(challenge), quote, TDX_QUOTE_MAX_LEN,
			       &reply, &self->error);
	if (ret == -EIO || ret == -ETIMEDOUT || ret == -EOPNOTSUPP) {
		free(quote);
		SKIP(return, "TDX Quote service unavailable: %s", strerror(-ret));
	}
	ASSERT_EQ(0, ret)
	{
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_reply_free(reply);
	free(quote);
}

TEST_F(tdx_attestation, common_validation_precedes_native_calls)
{
	static union {
		__u64 alignment;
		unsigned char bytes[TDX_REPORT_LEN];
	} report;
	unsigned char challenge[TDX_REPORTDATA_LEN] = {};
	struct ctlv_reply *reply;
	struct ctlv_msg *request;

	EXPECT_EQ(-EINVAL,
		  tdx_evidence_get(self->dev, "tdx-tdreport-1.0", challenge,
				   sizeof(challenge) - 1, report.bytes,
				   sizeof(report.bytes), &reply, &self->error));

	ASSERT_EQ(0, ctlv_attestation_evidence_get_request_new(&request));
	ctlv_attestation_evidence_challenge_put(request, challenge,
						sizeof(challenge));
	ctlv_attestation_evidence_object_start(request);
	ctlv_attestation_evidence_blob_class_put(request,
						 CTLV_ATTESTATION_EVIDENCE_BLOB_CLASS_PRIMARY);
	ctlv_attestation_evidence_format_put(request, "tdx-tdreport-1.0");
	ctlv_attestation_evidence_representation_put(request,
						     CTLV_ATTESTATION_EVIDENCE_REPRESENTATION_RAW);
	ctlv_attestation_evidence_data_put_va(request, report.bytes,
					      sizeof(report.bytes));
	ctlv_nest_end(request);
	ctlv_attestation_evidence_selection_start(request);
	ctlv_attestation_mr_id_put(request, 99);
	ctlv_attestation_evidence_hash_algo_put(request, HASH_ALGO_SHA384);
	ctlv_nest_end(request);
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(-ENOENT, ctlv_attestation_evidence_get(self->dev, request,
							 &reply, &self->error));
	ctlv_msg_free(request);
}

static int tdx_rtmr_extend(struct ctlv_dev *dev, unsigned int id,
			   const unsigned char digest[48],
			   struct ctlv_reply **reply, struct ctlv_error *error)
{
	struct ctlv_msg *request;
	int ret;

	ret = ctlv_attestation_mr_extend_request_new(&request);
	if (ret)
		return ret;
	ctlv_attestation_mr_id_put(request, id);
	ctlv_attestation_mr_value_start(request);
	ctlv_attestation_mr_hash_algo_put(request, HASH_ALGO_SHA384);
	ctlv_attestation_mr_data_put(request, digest, 48);
	ctlv_nest_end(request);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_extend(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(tdx_attestation, destructive_rtmr_extension_is_opt_in)
{
	const __u64 subscriptions[] = {
		CTLV_ATTESTATION_OP_MR_LOG_ENTRY_APPENDED,
	};
	unsigned char digest[48] = { 0xa5 };
	const char *selected = getenv("TDX_ATTESTATION_TEST_RTMR");
	struct ctlv_event *event;
	struct ctlv_reply *reply;
	char *end;
	unsigned long value;
	unsigned int rtmr;

	if (!selected)
		SKIP(return,
		     "set TDX_ATTESTATION_TEST_RTMR=0..3 in a disposable guest");
	errno = 0;
	value = strtoul(selected, &end, 10);
	ASSERT_EQ(0, errno);
	ASSERT_EQ('\0', *end);
	rtmr = value;
	ASSERT_LE(rtmr, 3);
	ASSERT_EQ(0, ctlv_subscribe(self->dev, subscriptions,
				    ARRAY_SIZE(subscriptions), &self->error));
	ASSERT_EQ(0, tdx_rtmr_extend(self->dev, rtmr + 1, digest, &reply,
				     &self->error));
	ctlv_reply_free(reply);
	ASSERT_EQ(0, ctlv_event_wait(self->dev, EVENT_WAIT_MS));
	ASSERT_EQ(0, ctlv_event_read(self->dev, &event));
	EXPECT_TRUE(ctlv_attestation_mr_log_entry_appended_is(event));
	ctlv_event_free(event);
}

TEST_HARNESS_MAIN
