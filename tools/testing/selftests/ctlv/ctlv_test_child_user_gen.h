/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/* Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from: */
/*	Documentation/ctlv/specs/ctlv-test-child.yaml */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-test-child CTLV family. */

#ifndef _CTLV_CTLV_TEST_CHILD_USER_GEN_H
#define _CTLV_CTLV_TEST_CHILD_USER_GEN_H

#include <ctlv.h>

#include <linux/ctlv_test_child_gen.h>

#include "ctlv_test_user_gen.h"

/* What this family's specification says, for holding a device against it. */
extern const struct ctlv_family_spec ctlv_test_child_spec;

/* Executing an op. A query is reassembled into the one reply it fills. */

#define CTLV_TEST_CHILD_OP_EXTRA_NOOP_REQUEST_MAX_LEN 16

static inline int ctlv_test_child_extra_noop_request_new(struct ctlv_msg **request)
{
	const __u32 len = CTLV_TEST_CHILD_OP_EXTRA_NOOP_REQUEST_MAX_LEN;

	return ctlv_msg_new(len, request);
}

static inline int ctlv_test_child_extra_noop(struct ctlv_dev *dev, struct ctlv_msg *request,
					     struct ctlv_error *error)
{
	return ctlv_action_sized(dev, CTLV_TEST_CHILD_OP_EXTRA_NOOP, request, 0, NULL, error);
}

/* Building a request. ctlv_msg_err() reports the first failure. */

static inline int ctlv_test_child_child_extra_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA, value);
}

/* Reading a reply or a record. A getter answers whether it could read. */

static inline bool ctlv_test_child_child_extra_present(const struct ctlv_attr *nest)
{
	return ctlv_attr_find(nest, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA);
}

static inline bool ctlv_test_child_child_extra_get(const struct ctlv_attr *nest, __u64 *value)
{
	const struct ctlv_attr *attr;

	attr = ctlv_attr_find(nest, CTLV_TEST_CHILD_ATTR_CHILD_EXTRA);
	return attr && !ctlv_get_u64(attr, value);
}

#endif /* _CTLV_CTLV_TEST_CHILD_USER_GEN_H */
