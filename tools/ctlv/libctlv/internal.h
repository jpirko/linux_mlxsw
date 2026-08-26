/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (c) 2026, NVIDIA CORPORATION & AFFILIATES
 *
 * What libctlv's own files share and nothing outside it sees.
 */
#ifndef _CTLV_INTERNAL_H
#define _CTLV_INTERNAL_H

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ctlv.h"

#define CTLV_ARRAY_SIZE(_a) (sizeof(_a) / sizeof((_a)[0]))

/*
 * How many times a query is started over when the answer changed underneath
 * it. More than this is a device changing faster than it can be described.
 */
#define QUERY_RESTARTS 8

/* The root is at depth zero, hence one more than the levels below it. */
#define CTLV_DEPTH_MAX (CTLV_MAX_NEST_DEPTH + 1)

/*
 * Reassembly needs a bound of its own, a query reply not being held to the
 * inline limit. The wire cannot describe more anyway: a nest's length is u32.
 */
#define CTLV_REPLY_LEN_MAX (64U << 20)

/**
 * struct ctlv_msg - One message being written.
 * @buf: the bytes, grown as needed and aligned as a root must be
 * @capacity: how much @buf holds
 * @max: how far @capacity may grow before appending fails
 * @len: how much of it is written
 * @open: offsets of the nest headers not yet ended, the root first
 * @depth: how many of those there are
 * @err: the first failure since the last reset
 *
 * An attr is tracked by offset because a nest's length is only known once its
 * children are written, and because growing @buf moves it.
 */
struct ctlv_msg {
	__u8 *buf;
	__u32 capacity;
	__u32 max;
	__u32 len;
	__u32 open[CTLV_DEPTH_MAX];
	unsigned int depth;
	int err;
};

int ctlv_msg_new_max(__u32 capacity, __u32 max, struct ctlv_msg **msgp);
int ctlv_msg_put(struct ctlv_msg *msg, __u64 attr_id, __u32 flags,
		 const void *payload, __u32 len);
/* Appends a copy of one attr, header and payload, without its flags. */
int ctlv_msg_copy(struct ctlv_msg *msg, const struct ctlv_attr *attr);

/**
 * struct ctlv_dev - One open device.
 * @fd: the character device
 * @name: what it is called under /dev and /sys/class/ctlv
 * @chain_len: how many families it answers
 * @chain_id: their registry IDs, the root first
 * @chain_name: their names, in the same order
 * @sizes: one ops dump, kept for the message lengths it reports
 *
 * @sizes is filled the first time an op runs without its lengths named, and
 * only its length members are read: they cannot go out of date.
 */
struct ctlv_dev {
	int fd;
	char *name;
	unsigned int chain_len;
	__u16 *chain_id;
	char **chain_name;
	struct ctlv_ops *sizes;
};

/**
 * struct ctlv_reply - One complete answer.
 * @msg: the reassembled message, whose root is the reply's
 * @generation: what the device reported the answer belongs to
 * @fragments: how many ioctls it took
 */
struct ctlv_reply {
	struct ctlv_msg *msg;
	__u64 generation;
	unsigned int fragments;
};

/**
 * struct ctlv_event - One record, as one read() returned it.
 * @buf: the record header followed by the one root that belongs to it
 * @len: how long the record is
 */
struct ctlv_event {
	__u8 *buf;
	__u32 len;
};

/**
 * struct ctlv_schema - One op's trees, and the nodes they are built from.
 * @node: every node of both trees, so that freeing one frees all of them
 * @n_nodes: how many
 * @request: the root of the request tree, or NULL
 * @reply: the root of the reply tree, or NULL
 */
struct ctlv_schema {
	struct ctlv_schema_node *node;
	unsigned int n_nodes;
	const struct ctlv_schema_node *request;
	const struct ctlv_schema_node *reply;
};

void ctlv_error_clear(struct ctlv_error *error);
void ctlv_error_set(struct ctlv_error *error, int err);
/* Reads whatever error information a failed op wrote into the reply buffer. */
void ctlv_error_parse(struct ctlv_error *error, int err, const void *buf,
		      __u32 len);

int ctlv_chain_load(struct ctlv_dev *dev);
int ctlv_schema_parse(const struct ctlv_attr *root, struct ctlv_schema **sp);

#endif /* _CTLV_INTERNAL_H */
