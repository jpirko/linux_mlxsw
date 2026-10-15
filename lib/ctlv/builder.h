/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_BUILDER_H
#define _CTLV_BUILDER_H

#include <linux/types.h>

struct ctlv_msg;
struct ctlv_op_ctx;
struct ctlv_schema_tree;

void ctlv_message_output_init(struct ctlv_msg *msg, struct ctlv_op_ctx *ctx, void *buf, size_t len,
			      const struct ctlv_schema_tree *tree);

#endif /* _CTLV_BUILDER_H */
