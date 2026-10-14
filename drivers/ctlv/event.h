/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _CTLV_EVENT_H
#define _CTLV_EVENT_H

#include <linux/gfp_types.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/types.h>
#include <linux/wait.h>

#include <uapi/linux/ctlv.h>

struct ctlv_device;
struct ctlv_op_entry;
struct file;
struct poll_table_struct;

struct ctlv_event_record {
	struct list_head node;
	u32 len;
	u32 room;
	bool reserved;
	struct ctlv_event_hdr hdr;
	u8 root[] __aligned(8);
};

struct ctlv_file {
	struct ctlv_device *ctlvdev;
	struct list_head node;

	struct list_head records;
	u32 queue_len;
	u32 queue_used;
	u64 sequence;
	unsigned long *subscribed;
	struct ctlv_event_record *overflow;
	struct ctlv_event_record *removed;
	bool overflowing;
	u64 first_dropped;
	u64 last_dropped;
	u64 dropped;
	bool reading_overflow;
	u64 first_reported;
	u64 last_reported;
	u64 dropped_reported;

	wait_queue_head_t readers;
	/* One reader of this description at a time, however many hold it. */
	struct mutex reading;
};

int ctlv_event_sizes(struct ctlv_device *ctlvdev);
void ctlv_event_device_init(struct ctlv_device *ctlvdev);
void ctlv_event_contexts_start(struct ctlv_device *ctlvdev);
void ctlv_event_contexts_stop(struct ctlv_device *ctlvdev);
void ctlv_event_contexts_wait(struct ctlv_device *ctlvdev);
void ctlv_event_device_dead(struct ctlv_device *ctlvdev);
int ctlv_event_file_init(struct ctlv_file *cfile);
void ctlv_event_file_exit(struct ctlv_file *cfile);
ssize_t ctlv_event_file_read(struct ctlv_file *cfile, char __user *buf,
			     size_t len, bool blocking);
__poll_t ctlv_event_file_poll(struct ctlv_file *cfile, struct file *filp,
			      struct poll_table_struct *wait);
void ctlv_event_op_changed(struct ctlv_device *ctlvdev,
			   const struct ctlv_op_entry *op, bool disabled,
			   u64 generation);

#endif /* _CTLV_EVENT_H */
