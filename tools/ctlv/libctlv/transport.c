// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * Executing one op, and putting a fragmented answer back together. An action
 * is one ioctl, a query as many as its answer takes; the boundaries, the
 * re-emitted paths and the cursor are this file's business. A device also
 * writes error information into the reply buffer describing the first thing it
 * refused, which is what turns an errno into which attr and why.
 */

#include <stdio.h>
#include <sys/ioctl.h>

#include "internal.h"

void ctlv_error_clear(struct ctlv_error *error)
{
	if (error)
		memset(error, 0, sizeof(*error));
}

void ctlv_error_set(struct ctlv_error *error, int err)
{
	if (!error)
		return;
	memset(error, 0, sizeof(*error));
	error->err = err;
}

static void error_u64(struct ctlv_error *error, const struct ctlv_attr *root,
		      __u64 attr_id, __u32 bit, __u64 *value)
{
	const struct ctlv_attr *attr = ctlv_attr_find(root, attr_id);

	if (!attr || ctlv_get_u64(attr, value))
		return;
	error->present |= bit;
}

/*
 * What a device wrote about the failure. A buffer holding nothing, or nothing
 * this reader can parse, leaves the errno as the whole of the report.
 */
void ctlv_error_parse(struct ctlv_error *error, int err, const void *buf,
		      __u32 len)
{
	const struct ctlv_attr *root = buf;
	const struct ctlv_attr *attr;
	__u64 value;

	if (!error)
		return;
	memset(error, 0, sizeof(*error));
	error->err = err;
	if (!buf || ctlv_msg_check(buf, len))
		return;

	attr = ctlv_attr_find(root, CTLV_ATTR_ERROR_REASON);
	if (attr && !ctlv_get_enum(attr, CTLV_VALIDATION_REASON_STALE_GENERATION,
				   &value, NULL)) {
		error->reason = value;
		error->present |= CTLV_ERROR_HAS_REASON;
	}
	attr = ctlv_attr_find(root, CTLV_ATTR_ERROR_MESSAGE);
	if (attr) {
		const char *message;

		if (!ctlv_get_str(attr, &message)) {
			snprintf(error->message, sizeof(error->message), "%s",
				 message);
			error->present |= CTLV_ERROR_HAS_MESSAGE;
		}
	}
	error_u64(error, root, CTLV_ATTR_ERROR_BAD_ATTR_OFFSET,
		  CTLV_ERROR_HAS_BAD_ATTR_OFFSET, &error->bad_attr_offset);
	error_u64(error, root, CTLV_ATTR_ERROR_NEST_OFFSET,
		  CTLV_ERROR_HAS_NEST_OFFSET, &error->nest_offset);
	error_u64(error, root, CTLV_ATTR_ERROR_MISSING_ATTR_ID,
		  CTLV_ERROR_HAS_MISSING_ATTR_ID, &error->missing_attr_id);
	error_u64(error, root, CTLV_ATTR_ERROR_EXPECTED_GENERATION,
		  CTLV_ERROR_HAS_EXPECTED_GENERATION,
		  &error->expected_generation);
	error_u64(error, root, CTLV_ATTR_ERROR_CURRENT_GENERATION,
		  CTLV_ERROR_HAS_CURRENT_GENERATION,
		  &error->current_generation);
}

static const char *const reasons[] = {
	[CTLV_VALIDATION_REASON_MALFORMED_MESSAGE] = "malformed message",
	[CTLV_VALIDATION_REASON_UNKNOWN_ATTR] = "unknown attr",
	[CTLV_VALIDATION_REASON_UNKNOWN_FLAG] = "unknown flag",
	[CTLV_VALIDATION_REASON_NESTING_TOO_DEEP] = "nesting too deep",
	[CTLV_VALIDATION_REASON_MISSING_ATTR] = "missing attr",
	[CTLV_VALIDATION_REASON_TOO_MANY_OCCURRENCES] = "too many occurrences",
	[CTLV_VALIDATION_REASON_VALUE_OUT_OF_RANGE] = "value out of range",
	[CTLV_VALIDATION_REASON_BAD_LENGTH] = "bad length",
	[CTLV_VALIDATION_REASON_BAD_STRING] = "bad string",
	[CTLV_VALIDATION_REASON_BAD_BLOB_DESCRIPTOR] = "bad blob descriptor",
	[CTLV_VALIDATION_REASON_STALE_GENERATION] = "stale generation",
};

const char *ctlv_error_str(const struct ctlv_error *error, char *buf,
			   size_t len)
{
	const char *reason = NULL;
	size_t used;

	if (error->present & CTLV_ERROR_HAS_REASON &&
	    error->reason < CTLV_ARRAY_SIZE(reasons))
		reason = reasons[error->reason];
	used = snprintf(buf, len, "%s", strerror(-error->err));
	if (reason && used < len)
		used += snprintf(buf + used, len - used, ": %s", reason);
	if (error->present & CTLV_ERROR_HAS_MESSAGE && used < len)
		used += snprintf(buf + used, len - used, ": %s",
				 error->message);
	if (error->present & CTLV_ERROR_HAS_MISSING_ATTR_ID && used < len)
		used += snprintf(buf + used, len - used, " (attr %#llx)",
				 (unsigned long long)error->missing_attr_id);
	if (error->present & CTLV_ERROR_HAS_BAD_ATTR_OFFSET && used < len)
		snprintf(buf + used, len - used, " (at offset %llu)",
			 (unsigned long long)error->bad_attr_offset);
	return buf;
}

static int exec(struct ctlv_dev *dev, struct ctlv_op_exec *arg)
{
	if (ioctl(dev->fd, CTLV_IOC_OP_EXEC, arg))
		return -errno;
	return 0;
}

static int reply_new(__u32 capacity, struct ctlv_reply **replyp)
{
	struct ctlv_reply *reply;
	int ret;

	reply = calloc(1, sizeof(*reply));
	if (!reply)
		return -ENOMEM;
	ret = ctlv_msg_new_max(capacity, CTLV_REPLY_LEN_MAX, &reply->msg);
	if (ret) {
		free(reply);
		return ret;
	}
	*replyp = reply;
	return 0;
}

void ctlv_reply_free(struct ctlv_reply *reply)
{
	if (!reply)
		return;
	ctlv_msg_free(reply->msg);
	free(reply);
}

const struct ctlv_attr *ctlv_reply_root(const struct ctlv_reply *reply)
{
	return ctlv_msg_root(reply->msg);
}

__u64 ctlv_reply_generation(const struct ctlv_reply *reply)
{
	return reply->generation;
}

unsigned int ctlv_reply_fragments(const struct ctlv_reply *reply)
{
	return reply->fragments;
}

/* The request bytes an op takes, an op that takes none included. */
static int request_of(struct ctlv_msg *request, __u64 *ptr, __u32 *len)
{
	int ret;

	if (!request) {
		*ptr = 0;
		*len = 0;
		return 0;
	}
	ret = ctlv_msg_done(request);
	if (ret)
		return ret;
	*ptr = (__u64)(uintptr_t)request->buf;
	*len = request->len;
	return 0;
}

int ctlv_action_sized(struct ctlv_dev *dev, __u64 op_id,
		      struct ctlv_msg *request, __u32 reply_len,
		      struct ctlv_reply **replyp, struct ctlv_error *error)
{
	struct ctlv_op_exec arg = { .op_id = op_id };
	struct ctlv_reply *reply = NULL;
	int ret;

	ctlv_error_clear(error);
	ret = request_of(request, &arg.request, &arg.request_len);
	if (ret) {
		ctlv_error_set(error, ret);
		return ret;
	}

	/*
	 * Never less than a refusal's error information: an action cannot be
	 * retried into a larger buffer the way a query can, it has happened.
	 */
	if (reply_len < CTLV_MAX_ERROR_LEN)
		reply_len = CTLV_MAX_ERROR_LEN;
	ret = reply_new(reply_len, &reply);
	if (ret) {
		ctlv_error_set(error, ret);
		return ret;
	}
	/* A zeroed root says nothing was written, a real one being longer. */
	memset(reply->msg->buf, 0, sizeof(struct ctlv_attr));
	arg.reply = (__u64)(uintptr_t)reply->msg->buf;
	arg.reply_len = reply_len;

	ret = exec(dev, &arg);
	if (ret) {
		ctlv_error_parse(error, ret, reply->msg->buf, reply_len);
		ctlv_reply_free(reply);
		return ret;
	}

	reply->fragments = 1;
	reply->msg->len = ctlv_msg_root(reply->msg)->len;
	if (!reply->msg->len) {
		/* Acknowledged, and there is nothing to read. */
		ctlv_reply_free(reply);
		reply = NULL;
	} else {
		ret = ctlv_msg_check(reply->msg->buf, reply_len);
		if (ret) {
			ctlv_error_set(error, ret);
			ctlv_reply_free(reply);
			return ret;
		}
	}
	if (replyp)
		*replyp = reply;
	else
		ctlv_reply_free(reply);
	return 0;
}

/* How many nests a fragment left open, which the next one re-emits. */
static unsigned int frag_open(const struct ctlv_attr *nest)
{
	const struct ctlv_attr *child, *last = NULL;

	ctlv_for_each(child, nest)
		last = child;
	if (!last || !(last->flags & CTLV_ATTR_F_FRAG_MORE))
		return 0;
	return 1 + frag_open(last);
}

/*
 * One fragment, appended to the reply being assembled. @skip levels of
 * re-emitted path are descended into rather than appended, being still open in
 * @out; a nest left marked open stays open for the next fragment.
 */
static int splice(struct ctlv_msg *out, const struct ctlv_attr *nest,
		  unsigned int skip)
{
	const struct ctlv_attr *child;
	bool first = true;
	int ret;

	ctlv_for_each(child, nest) {
		bool more = child->flags & CTLV_ATTR_F_FRAG_MORE;

		if (first && skip) {
			ret = splice(out, child, skip - 1);
			if (!ret && !more)
				ret = ctlv_nest_end(out);
		} else if (more) {
			ret = ctlv_nest_start(out, child->attr_id);
			if (!ret)
				ret = splice(out, child, 0);
		} else {
			/* Complete: nothing inside waits for a continuation. */
			ret = ctlv_msg_copy(out, child);
		}
		if (ret)
			return ret;
		first = false;
	}
	return 0;
}

int ctlv_query_sized(struct ctlv_dev *dev, __u64 op_id,
		     struct ctlv_msg *request, __u32 buf_len,
		     struct ctlv_reply **replyp, struct ctlv_error *error)
{
	struct ctlv_op_exec arg = { .op_id = op_id };
	struct ctlv_reply *reply = NULL;
	unsigned int open = 0;
	void *buf;
	int ret;

	ctlv_error_clear(error);
	if (!buf_len)
		buf_len = CTLV_DEFAULT_QUERY_REPLY_LEN;
	ret = request_of(request, &arg.request, &arg.request_len);
	if (ret)
		goto out_err;

	buf = malloc(buf_len);
	if (!buf) {
		ret = -ENOMEM;
		goto out_err;
	}
	arg.reply = (__u64)(uintptr_t)buf;
	arg.reply_len = buf_len;

	ret = reply_new(buf_len, &reply);
	if (ret)
		goto out_free_buf;

	do {
		const struct ctlv_attr *root = buf;

		ret = exec(dev, &arg);
		if (ret) {
			ctlv_error_parse(error, ret, buf, buf_len);
			goto out_free_reply;
		}
		ret = ctlv_msg_check(buf, buf_len);
		if (ret)
			goto out_set_err;

		/*
		 * The root's mark and the cursor both say whether there is
		 * more, and disagreeing is not something to reassemble.
		 */
		if (!(root->flags & CTLV_ATTR_F_FRAG_MORE) != !arg.cursor) {
			ret = -EPROTO;
			goto out_set_err;
		}

		ret = splice(reply->msg, root, open);
		if (ret)
			goto out_set_err;
		open = frag_open(root);
		if (!arg.cursor && open) {
			ret = -EPROTO;
			goto out_set_err;
		}

		reply->fragments++;
		reply->generation = arg.generation;
	} while (arg.cursor);

	/* The answer having ended, the root is the only nest still open. */
	if (reply->msg->depth != 1) {
		ret = -EPROTO;
		goto out_set_err;
	}
	ret = ctlv_msg_done(reply->msg);
	if (ret)
		goto out_set_err;
	/* The reassembled tree is a message in its own right, or nothing is. */
	ret = ctlv_msg_check(reply->msg->buf, reply->msg->len);
	if (ret)
		goto out_set_err;

	free(buf);
	if (replyp)
		*replyp = reply;
	else
		ctlv_reply_free(reply);
	return 0;

out_set_err:
	ctlv_error_set(error, ret);
out_free_reply:
	ctlv_reply_free(reply);
out_free_buf:
	free(buf);
	return ret;
out_err:
	ctlv_error_set(error, ret);
	return ret;
}
