/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_DEVICE_H
#define _CTLV_DEVICE_H

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/rwsem.h>
#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/wait.h>

struct ctlv_event_record;
struct ctlv_family;
struct ctlv_family_priv;
struct ctlv_op_entry;

struct ctlv_device {
	struct device dev;
	struct cdev cdev;

	struct rw_semaphore registration_lock;
	bool registered;
	const struct ctlv_family *family;
	struct ctlv_family_priv *privs;
	u32 privs_count;
	void *priv_block;

	struct ctlv_op_entry *ops;
	u32 ops_count;

	/* Op publication and state writers, never the dispatch path. */
	spinlock_t transition_lock;
	bool published;
	bool destroying;
	u64 state_generation;
	u32 dynamic_ops_count;
	unsigned long *disabled;

	/* Everything about the queues, and nothing family code runs under. */
	spinlock_t events_lock;
	struct list_head files;
	bool dead_announced;
	wait_queue_head_t event_context_wait;
	u32 active_event_contexts;
	bool events_live;
	u32 events_count;
	const struct ctlv_op_entry *overflow_op;
	const struct ctlv_op_entry *removed_op;
	u32 queue_min;
	u32 queue_reserved;
	u32 queue_default;
};

#endif /* _CTLV_DEVICE_H */
