/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_BLOB_H
#define _CTLV_BLOB_H

#include <linux/types.h>

struct ctlv_blob;
struct ctlv_op_ctx;

void ctlv_blob_context_init(struct ctlv_op_ctx *ctx);
void ctlv_blob_context_track(struct ctlv_blob *blob,
			     struct ctlv_op_ctx *ctx);
int ctlv_blob_context_finish(struct ctlv_op_ctx *ctx);

#endif /* _CTLV_BLOB_H */
