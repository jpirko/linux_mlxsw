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
/* Past a DCAP Quote's 48-byte header, at offset 520 of its TD report body. */
#define TDX_QUOTE_REPORTDATA_OFFSET (48 + 520)
#define TDX_QUOTE_MAX_LEN ((128 * 1024) - 24)
/* Past the TCG_PCR_EVENT header of a crypto-agile log's first event. */
#define TCG_SPEC_ID_SIGNATURE_OFFSET 32

/* The provider's objects, by ID. */
enum { TDX_QUOTE, TDX_LOG };

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
	const struct ctlv_hdr *caps, *evidence, *mr, *info;
	const void *value;
	struct ctlv_reply *reply;
	__u64 flags, unknown;
	__u32 length;
	bool got;

	ASSERT_EQ(3, ctlv_chain_len(self->dev));
	EXPECT_TRUE(ctlv_dev_inherits(self->dev, "ctlv-attestation"));
	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error))
	{
		TH_LOG("%s", error_text(&self->error));
	}
	caps =
		ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	evidence = ctlv_attestation_device_get_caps_evidence_get(caps);
	mr = ctlv_attestation_device_get_caps_mr_get(caps);
	ASSERT_NE(NULL, evidence);
	ASSERT_NE(NULL, mr);
	got = ctlv_attestation_device_get_evidence_flags_get(evidence, &flags,
							     &unknown);
	ASSERT_TRUE(got);
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_CAPS_MR_SELECTION, flags);
	EXPECT_EQ(0, unknown);
	got = ctlv_attestation_device_get_mr_flags_get(mr, &flags, &unknown);
	ASSERT_TRUE(got);
	EXPECT_EQ(CTLV_ATTESTATION_MR_DEVICE_CAPS_EXTEND, flags);
	EXPECT_EQ(0, unknown);
	info = ctlv_attestation_device_get_info_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, info);
	got =
		ctlv_attestation_tdx_device_info_mrconfigid_get(info, &value,
								&length);
	ASSERT_TRUE(got);
	EXPECT_EQ(48, length);
	got = ctlv_attestation_tdx_device_info_mrowner_get(info, &value,
							   &length);
	ASSERT_TRUE(got);
	EXPECT_EQ(48, length);
	ASSERT_TRUE(ctlv_attestation_tdx_device_info_mrownerconfig_get(info, &value, &length));
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
	ctlv_attestation_mr_read_id_put(request, id);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_mr_read(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

TEST_F(tdx_attestation, measurement_registers)
{
	const struct ctlv_hdr *item, *algorithm, *value;
	struct ctlv_reply *reply;
	const void *digest;
	__u64 id, hash;
	__u32 length;
	unsigned int count = 0;
	bool got;

	ASSERT_EQ(0, ctlv_attestation_mr_get(self->dev, NULL, &reply,
					     &self->error));
	ctlv_attestation_mr_get_items_for_each(item, ctlv_reply_root(reply)) {
		ASSERT_TRUE(ctlv_attestation_mr_get_items_id_get(item, &id));
		EXPECT_EQ(count, id);
		algorithm = NULL;
		ctlv_attestation_mr_desc_supported_hash_algorithms_for_each(algorithm, item) {
			break;
		}
		ASSERT_NE(NULL, algorithm);
		got = ctlv_attestation_mr_desc_supported_hash_algorithms_get(algorithm,
									     &hash, NULL);
		ASSERT_TRUE(got);
		EXPECT_EQ(HASH_ALGO_SHA384, hash);
		count++;
	}
	EXPECT_EQ(5, count);
	ctlv_reply_free(reply);

	for (id = 0; id < 5; id++) {
		ASSERT_EQ(0, tdx_mr_read(self->dev, id, &reply, &self->error));
		value = NULL;
		ctlv_attestation_mr_values_for_each(value,
						    ctlv_reply_root(reply)) {
			break;
		}
		ASSERT_NE(NULL, value);
		got = ctlv_attestation_mr_value_data_get(value, &digest,
							 &length);
		ASSERT_TRUE(got);
		EXPECT_EQ(48, length);
		ctlv_reply_free(reply);
	}
}

static void tdx_object_put(struct ctlv_msg *request, __u64 id, void *output,
			   size_t output_length)
{
	ctlv_attestation_evidence_get_objects_start(request);
	ctlv_attestation_evidence_get_objects_id_put(request, id);
	ctlv_attestation_evidence_get_objects_data_put_va(request, output,
							  output_length);
	ctlv_record_end(request);
}

static int tdx_evidence_get(struct ctlv_dev *dev, __u64 id,
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
	if (challenge)
		ctlv_attestation_evidence_get_challenge_put(request, challenge,
							    challenge_length);
	tdx_object_put(request, id, output, output_length);
	ret = ctlv_msg_err(request);
	if (!ret)
		ret = ctlv_attestation_evidence_get(dev, request, reply, error);
	ctlv_msg_free(request);
	return ret;
}

/* The Quote has either format, and takes REPORTDATA's length alone. */
TEST_F(tdx_attestation, quote_object)
{
	static const __u64 formats[] = {
		CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE,
		CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE_DICE,
	};
	const struct ctlv_hdr *caps, *object = NULL, *hdr;
	unsigned int count = 0, lengths = 0;
	struct ctlv_reply *reply;
	__u64 value;
	bool known;

	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error));
	caps = ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	caps = ctlv_attestation_device_get_caps_evidence_get(caps);
	ASSERT_NE(NULL, caps);
	ctlv_attestation_device_get_evidence_objects_for_each(object, caps)
		break;
	ASSERT_NE(NULL, object);
	ASSERT_TRUE(ctlv_attestation_device_get_objects_id_get(object, &value));
	EXPECT_EQ(TDX_QUOTE, value);
	ASSERT_TRUE(ctlv_attestation_device_get_objects_class_get(object, &value, &known));
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_CLASS_SIGNED, value);
	ctlv_attestation_device_get_objects_formats_for_each(hdr, object)
		count++;
	EXPECT_EQ(0, count);
	ctlv_attestation_device_get_objects_vendor_formats_for_each(hdr, object) {
		ASSERT_GT(ARRAY_SIZE(formats), count);
		ASSERT_TRUE(ctlv_attestation_device_get_objects_vendor_formats_get(hdr, &value));
		EXPECT_EQ(formats[count], value);
		count++;
	}
	EXPECT_EQ(ARRAY_SIZE(formats), count);
	ctlv_attestation_device_get_objects_challenge_lengths_for_each(hdr, object) {
		ASSERT_TRUE(ctlv_attestation_device_get_challenge_lengths_length_get(hdr,
										     &value));
		EXPECT_EQ(TDX_REPORTDATA_LEN, value);
		lengths++;
	}
	EXPECT_EQ(1, lengths);
	ctlv_reply_free(reply);
}

TEST_F(tdx_attestation, quote_when_service_is_available)
{
	unsigned char challenge[TDX_REPORTDATA_LEN];
	const struct ctlv_hdr *root, *result = NULL;
	struct ctlv_reply *reply;
	__u64 format = 0, length = 0;
	unsigned int index;
	void *quote;
	int ret;

	for (index = 0; index < sizeof(challenge); index++)
		challenge[index] = index;
	ASSERT_EQ(0, posix_memalign(&quote, 8, TDX_QUOTE_MAX_LEN));
	ret = tdx_evidence_get(self->dev, TDX_QUOTE, challenge,
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
	root = ctlv_reply_root(reply);
	EXPECT_TRUE(ctlv_attestation_mr_generation_present(root));
	EXPECT_FALSE(ctlv_attestation_evidence_get_evidence_generation_present(root));
	ctlv_attestation_evidence_get_results_for_each(result, root)
		break;
	ASSERT_NE(NULL, result);
	EXPECT_FALSE(ctlv_attestation_evidence_get_results_format_present(result));
	ASSERT_TRUE(ctlv_attestation_evidence_get_results_vendor_format_get(result, &format));
	ASSERT_TRUE(ctlv_attestation_evidence_get_results_length_get(result, &length));
	if (format == CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE) {
		ASSERT_GE(length, TDX_QUOTE_REPORTDATA_OFFSET + sizeof(challenge));
		EXPECT_EQ(0, memcmp((unsigned char *)quote + TDX_QUOTE_REPORTDATA_OFFSET,
				    challenge, sizeof(challenge)));
	} else {
		EXPECT_EQ(CTLV_ATTESTATION_TDX_EVIDENCE_VENDOR_FORMAT_QUOTE_DICE, format);
	}
	ctlv_reply_free(reply);
	free(quote);
}

/* The log is read as it stands, so it takes no challenge. */
TEST_F(tdx_attestation, ccel_log_alone)
{
	const struct ctlv_hdr *caps, *object, *hdr, *result = NULL;
	__u64 id, class, format = 0, max_length = 0, length = 0;
	unsigned int lengths = 0;
	struct ctlv_reply *reply;
	unsigned char *log;
	bool known;

	ASSERT_EQ(0, ctlv_attestation_device_get(self->dev, NULL, &reply,
						 &self->error));
	caps = ctlv_attestation_device_get_caps_get(ctlv_reply_root(reply));
	ASSERT_NE(NULL, caps);
	caps = ctlv_attestation_device_get_caps_evidence_get(caps);
	ASSERT_NE(NULL, caps);
	ctlv_attestation_device_get_evidence_objects_for_each(object, caps) {
		if (!ctlv_attestation_device_get_objects_class_get(object, &class, &known) ||
		    class != CTLV_ATTESTATION_EVIDENCE_CLASS_EVENT_LOG)
			continue;
		ASSERT_TRUE(ctlv_attestation_device_get_objects_id_get(object, &id));
		EXPECT_EQ(TDX_LOG, id);
		ctlv_attestation_device_get_objects_challenge_lengths_for_each(hdr, object)
			lengths++;
		EXPECT_EQ(0, lengths);
		ASSERT_TRUE(ctlv_attestation_device_get_objects_max_length_get(object,
									       &max_length));
	}
	ctlv_reply_free(reply);
	if (!max_length)
		SKIP(return, "no CCEL table");
	ASSERT_GE(max_length, TCG_SPEC_ID_SIGNATURE_OFFSET + 16);

	log = malloc(max_length);
	ASSERT_NE(NULL, log);
	ASSERT_EQ(0, tdx_evidence_get(self->dev, TDX_LOG, NULL, 0, log,
				      max_length, &reply, &self->error))
	{
		TH_LOG("%s", error_text(&self->error));
	}
	ctlv_attestation_evidence_get_results_for_each(result, ctlv_reply_root(reply))
		break;
	ASSERT_NE(NULL, result);
	ASSERT_TRUE(ctlv_attestation_evidence_get_results_format_get(result, &format, &known));
	EXPECT_EQ(CTLV_ATTESTATION_EVIDENCE_FORMAT_CCEL, format);
	ASSERT_TRUE(ctlv_attestation_evidence_get_results_length_get(result, &length));
	EXPECT_EQ(max_length, length);
	EXPECT_EQ(0, memcmp(log + TCG_SPEC_ID_SIGNATURE_OFFSET,
			    "Spec ID Event03", 16));
	ctlv_reply_free(reply);
	free(log);
}

TEST_F(tdx_attestation, common_validation_precedes_native_calls)
{
	unsigned char challenge[TDX_REPORTDATA_LEN] = {};
	struct ctlv_reply *reply;
	struct ctlv_msg *request;
	void *quote;

	ASSERT_EQ(0, posix_memalign(&quote, 8, TDX_QUOTE_MAX_LEN));
	EXPECT_EQ(-EINVAL,
		  tdx_evidence_get(self->dev, TDX_QUOTE, challenge,
				   sizeof(challenge) - 1, quote,
				   TDX_QUOTE_MAX_LEN, &reply, &self->error));
	EXPECT_EQ(-EINVAL,
		  tdx_evidence_get(self->dev, TDX_QUOTE, NULL, 0, quote,
				   TDX_QUOTE_MAX_LEN, &reply, &self->error));
	EXPECT_EQ(-ENOENT,
		  tdx_evidence_get(self->dev, TDX_LOG + 1, NULL, 0, quote,
				   TDX_QUOTE_MAX_LEN, &reply, &self->error));

	ASSERT_EQ(0, ctlv_attestation_evidence_get_request_new(&request));
	ctlv_attestation_evidence_get_challenge_put(request, challenge,
						    sizeof(challenge));
	tdx_object_put(request, TDX_QUOTE, quote, TDX_QUOTE_MAX_LEN);
	ctlv_attestation_evidence_get_selection_start(request);
	ctlv_attestation_evidence_get_selection_id_put(request, 99);
	ctlv_attestation_evidence_get_selection_hash_algo_put(request,
							      HASH_ALGO_SHA384);
	ctlv_record_end(request);
	ASSERT_EQ(0, ctlv_msg_err(request));
	EXPECT_EQ(-ENOENT, ctlv_attestation_evidence_get(self->dev, request,
							 &reply, &self->error));
	ctlv_msg_free(request);
	free(quote);
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
	ctlv_attestation_mr_extend_id_put(request, id);
	ctlv_attestation_mr_values_start(request);
	ctlv_attestation_mr_value_hash_algo_put(request, HASH_ALGO_SHA384);
	ctlv_attestation_mr_value_data_put(request, digest, 48);
	ctlv_record_end(request);
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
