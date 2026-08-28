/* SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause) */
/*
 * Generated from Documentation/ctlv/specs/ctlv-test-child.yaml by
 * tools/ctlv/ctlv-regen.sh. Do not edit.
 */
/* CTLV-GEN user header */

/* Userspace binding of the ctlv-test-child CTLV family. */

#ifndef _CTLV_CTLV_TEST_CHILD_USER_GEN_H
#define _CTLV_CTLV_TEST_CHILD_USER_GEN_H

#include <ctlv.h>

#include <ctlv/ctlv_test_child_gen.h>

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
	return ctlv_action(dev, CTLV_TEST_CHILD_OP_EXTRA_NOOP, request, NULL, error);
}

/* Building a request. ctlv_msg_err() reports the first failure. */

/* The one member the child adds, wherever the extension point is. */
static inline int ctlv_test_child_extension_point_extra_put(struct ctlv_msg *msg, __u64 value)
{
	return ctlv_put_u64(msg, CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA, value);
}

/* Reading a reply or an event. A getter answers whether it could read. */

/* The one member the child adds, wherever the extension point is. */
static inline bool ctlv_test_child_extension_point_extra_present(const struct ctlv_hdr *record)
{
	return ctlv_hdr_find(record, CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA);
}

static inline bool ctlv_test_child_extension_point_extra_get(const struct ctlv_hdr *record,
							     __u64 *value)
{
	const struct ctlv_hdr *hdr;

	hdr = ctlv_hdr_find(record, CTLV_TEST_CHILD_T_EXTENSION_POINT_EXTRA);
	return hdr && !ctlv_get_u64(hdr, value);
}

#endif /* _CTLV_CTLV_TEST_CHILD_USER_GEN_H */
