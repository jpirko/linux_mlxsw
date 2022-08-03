/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright (C) 2022, Intel Corporation. */

#ifndef _ICE_SYNCE_H_
#define _ICE_SYNCE_H_

#include "ice.h"

#define ICE_SYNCE_PRIO_MAX	0xF

/** ice_synce_pin - store info about pins
 * @types_supported: bitmask of enabled types
 * @type: type currently set for a pin
 * @num_types: number of types supported
 * @flags: pin flags returned from HW
 * @prio: priority of a pin on the dpll
 */
struct ice_synce_pin {
	int types_supported;
	u8 type;
	u8 num_types;
	u8 flags;
	u8 prio;
};

/** ice_synce - store info required for SyncE and CGU DPLL control
 * @dpll: pointer to dpll dev
 * @dpll_idx: index of dpll on the NIC
 * @current_source: source currently selected
 * @ref_state: state of dpll reference signals
 * @eec_mode: eec_mode dpll is configured for
 * @phase_offset: phase delay of a dpll
 * @dpll_state: current dpll sync state
 * @last_dpll_state: last dpll sync state
 * @num_inputs: number of input pins available on dpll
 * @inputs: input pins pointer
 * @num_outputs: number of output pins available on dpll
 * @cgu_state_acq_err_num: number of errors returned during periodic work
 * @kworker: periodic worker
 * @lock: locks access to configuration of a dpll
 * @work: periodic work
 */
struct ice_synce {
	struct dpll_device *dpll;
	int dpll_idx;
	u8 current_source;
	u8 ref_state;
	u8 eec_mode;
	s64 phase_offset;
	enum ice_cgu_state dpll_state;
	enum ice_cgu_state last_dpll_state;
	int num_inputs;
	struct ice_synce_pin *inputs;
	int num_outputs;
	struct ice_synce_pin *outputs;
	int cgu_state_acq_err_num;
	struct kthread_worker *kworker;
	struct mutex lock;
	struct kthread_delayed_work work;
};

int ice_synce_init(struct ice_pf *pf);

void ice_synce_release(struct ice_pf *pf);

#endif
