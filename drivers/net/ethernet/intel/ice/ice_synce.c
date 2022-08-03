// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2022, Intel Corporation. */

#include "ice.h"
#include "ice_lib.h"
#include "ice_trace.h"
#include <linux/dpll.h>
#include <uapi/linux/dpll.h>

#define SYNCE_CGU_STATE_ACQ_ERR_THRESHOLD	50

/**
 * ice_synce_set_freq
 * @pf: Board private structure
 * @id: pin index
 * @freq: frequency
 * @input: input or output pin
 *
 * Set frequency of given pin.
 * Return:
 * * 0 - OK
 * * negative - error
 */
static int ice_synce_set_freq(struct ice_pf *pf, u8 id, u32 freq, bool input)
{
	u8 flags, src_sel = 0;
	s32 phase = 0;
	int ret;

	if (input) {
		flags = ICE_AQC_SET_CGU_IN_CFG_FLG1_UPDATE_FREQ;
		ret = ice_aq_set_input_pin_cfg(&pf->hw, id, flags,
					       pf->synce.inputs[id].flags,
					       freq, phase);
	} else {
		flags = pf->synce.outputs[id].flags |
			ICE_AQC_SET_CGU_OUT_CFG_UPDATE_FREQ;
		ret = ice_aq_set_output_pin_cfg(&pf->hw, id, flags, src_sel,
						freq, phase);
	}

	return ret;
}

/**
 * ice_synce_get_status
 * @dpll: registered dpll pointer
 * @id: input pin index
 *
 * dpll subsystem callback.
 * Check current status of a dpll.
 * Return:
 * * 0 - OK
 * * other - error state
 */
static int ice_synce_get_status(struct dpll_device *dpll)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	mutex_lock(&pf->synce.lock);
	ret = pf->synce.ref_state;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_get_lock_status
 * @dpll: registered dpll pointer
 * @id: input pin index
 *
 * dpll subsystem callback.
 * Check current lock status of a dpll.
 * Return:
 * * 1 - locked
 * * 0 - not locked
 */
static int ice_synce_get_lock_status(struct dpll_device *dpll)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	mutex_lock(&pf->synce.lock);
	/* Once dpll interface provides defines for dpll lock states,
	 * change return values to proper ones, instead of 0 or 1.
	 */
	ret = pf->synce.dpll_state == ICE_CGU_STATE_LOCKED ||
	      pf->synce.dpll_state == ICE_CGU_STATE_LOCKED_HO_ACQ ?
	      1 : 0;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_get_source_type
 * @dpll: registered dpll pointer
 * @id: input pin index
 *
 * dpll subsystem callback.
 * Check current type of a given pin.
 * Return: current type value of a pin.
 */
static int ice_synce_get_source_type(struct dpll_device *dpll, int id)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	if (id < 0 || id >= pf->synce.num_inputs)
		return DPLL_TYPE_NONE;
	mutex_lock(&pf->synce.lock);
	ret = pf->synce.inputs[id].type;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_set_source_type
 * @dpll: registered dpll pointer
 * @id: output pin index
 *
 * dpll subsystem callback.
 * Set type of a given source pin.
 * Return:
 * * 0 - success,
 * * negative - failure.
 */
static int ice_synce_set_source_type(struct dpll_device *dpll, int id, int type)
{
	struct ice_pf *pf = dpll_priv(dpll);
	struct ice_synce *se = &pf->synce;
	struct ice_synce_pin *inputs;
	int ret;

	if (id < 0 || id >= se->num_inputs)
		return -EINVAL;
	inputs = se->inputs;

	if (type == inputs[id].type)
		return 0;

	if (inputs[id].num_types == 1)
		return -EPERM;

	if (!(inputs[id].types_supported & BIT(type)))
		return -EINVAL;

	mutex_lock(&pf->synce.lock);
	if (type == DPLL_TYPE_EXT_1PPS)
		ret = ice_synce_set_freq(pf, (u8)id, 1, true);
	else if (type == DPLL_TYPE_EXT_10MHZ)
		ret = ice_synce_set_freq(pf, (u8)id, 10000000, true);
	else
		ret = -EINVAL;

	if (!ret)
		inputs[id].type = type;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_get_source_supported
 * @dpll: registered dpll pointer
 * @id: input pin index
 * @type: type
 *
 * dpll subsystem callback.
 * Check if given type is supported on a given pin.
 * Return:
 * * true - if supported
 * * false - if not supported
 */
static int
ice_synce_get_source_supported(struct dpll_device *dpll, int id, int type)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	mutex_lock(&pf->synce.lock);
	ret = !!(pf->synce.inputs[id].types_supported & BIT(type));
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_get_output_type
 * @dpll: registered dpll pointer
 * @id: output pin index
 *
 * dpll subsystem callback.
 * Check current type of a given pin.
 * Return: current type value of a pin.
 */
static int ice_synce_get_output_type(struct dpll_device *dpll, int id)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	if (id < 0 || id >= pf->synce.num_outputs)
		return DPLL_TYPE_NONE;
	mutex_lock(&pf->synce.lock);
	ret = pf->synce.outputs[id].type;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_set_output_type
 * @dpll: registered dpll pointer
 * @id: output pin index
 *
 * dpll subsystem callback.
 * Set type of a given output pin.
 * Return:
 * * 0 - success,
 * * negative - failure.
 */
static int ice_synce_set_output_type(struct dpll_device *dpll, int id, int type)
{
	struct ice_pf *pf = dpll_priv(dpll);
	struct ice_synce *se = &pf->synce;
	struct ice_synce_pin *outputs;
	int ret;

	if (id < 0 || id >= se->num_outputs)
		return -EINVAL;
	outputs = se->outputs;

	if (type == outputs[id].type)
		return 0;

	if (outputs[id].num_types == 1)
		return -EPERM;

	if (!(outputs[id].types_supported & BIT(type)))
		return -EINVAL;

	mutex_lock(&pf->synce.lock);
	if (type == DPLL_TYPE_EXT_1PPS)
		ret = ice_synce_set_freq(pf, (u8)id, 1, false);
	else if (type == DPLL_TYPE_EXT_10MHZ)
		ret = ice_synce_set_freq(pf, (u8)id, 10000000, false);
	else
		ret = -EINVAL;

	if (!ret)
		outputs[id].type = type;
	mutex_unlock(&pf->synce.lock);

	return ret;
}

/**
 * ice_synce_get_output_supported
 * @dpll: registered dpll pointer
 * @id: output pin index
 * @type: id of type
 *
 * dpll subsystem callback.
 * Check if given type is supported on a given pin.
 * Return:
 * * true - if supported
 * * false - if not supported
 */
static int
ice_synce_get_output_supported(struct dpll_device *dpll, int id, int type)
{
	struct ice_pf *pf = dpll_priv(dpll);
	int ret;

	mutex_lock(&pf->synce.lock);
	ret = !!(pf->synce.outputs[id].types_supported & BIT(type));
	mutex_unlock(&pf->synce.lock);

	return ret;
}

static struct dpll_device_ops ice_synce_dpll_ops = {
	.get_status = ice_synce_get_status,
	.get_lock_status = ice_synce_get_lock_status,
	.get_source_type = ice_synce_get_source_type,
	.set_source_type = ice_synce_set_source_type,
	.get_source_supported = ice_synce_get_source_supported,
	.get_output_type = ice_synce_get_output_type,
	.set_output_type = ice_synce_set_output_type,
	.get_output_supported = ice_synce_get_output_supported,
};

/**
 * ice_synce_get_pin_freq
 * @hw: Board private structure
 * @pin: pin index
 * @freq: pin frequency
 * @input: input or output pin
 *
 * Get pin frequency.
 * * 0 - success
 * * negative - failure
 */
static int ice_synce_get_pin_freq(struct ice_hw *hw, u8 pin, u32 *freq,
				  bool input)
{
	int ret;

	if (input)
		ret = ice_aq_get_input_pin_cfg(hw, pin, NULL, NULL, NULL, NULL,
					       freq, NULL);
	else
		ret = ice_aq_get_output_pin_cfg(hw, pin, NULL, NULL, freq,
						NULL);
	if (ret) {
		dev_err(ice_pf_to_dev((struct ice_pf *)hw->back),
			"err:%d %s failed to read %s pin cfg on pin:%u\n",
			ret, ice_aq_str(hw->adminq.sq_last_status),
			input ? "input" : "output", pin);
	}

	return ret;
}

/**
 * ice_synce_pin_freq_to_type
 * @hw: Board private structure
 * @pin: pin index
 * @input: input or output pin
 *
 * Get pin frequency and translate it to a type.
 * Return: type of a pin based on frequency.
 */
static u32 ice_synce_pin_freq_to_type(struct ice_hw *hw, u8 pin, bool input)
{
	u32 freq;
	int ret;

	ret = ice_synce_get_pin_freq(hw, pin, &freq, input);
	if (ret)
		return DPLL_TYPE_NONE;

	if (freq == 1)
		return DPLL_TYPE_EXT_1PPS;
	else if (freq == 10000000)
		return DPLL_TYPE_EXT_10MHZ;

	return DPLL_TYPE_NONE;
}

/**
 * ice_synce_type_bit_to_type
 * @type_mask: bitmask with one bit set
 *
 * Translate bitmask to a dpll interface pin type;
 * Return: type of a pin
 */
static int ice_synce_type_bit_to_type(u8 type_mask)
{
	int type = -1;

	while (type_mask) {
		type++;
		type_mask = type_mask >> 1;
	}

	if (type < 0 || type > DPLL_TYPE_MAX)
		return DPLL_TYPE_NONE;

	return type;
}

/**
 * ice_synce_init_pin_type
 * @hw: Board private structure
 * @input: input or output pins
 * @num_pins: numer of pins to init
 * @pins: pointer to pins
 *
 * Init input or output pin type.
 */
static void ice_synce_init_pin_types(struct ice_hw *hw, bool input, u8 pin_idx,
				     struct ice_synce_pin *pin)
{
	u32 ts;

	if (pin->num_types == 0) {
		pin->type = DPLL_TYPE_NONE;
		pin->types_supported = 0;
	} else {
		ts = ice_cgu_get_pin_types_supported(hw, pin_idx, input);
		pin->types_supported = ts;
		if (pin->num_types == 1) {
			pin->type = ice_synce_type_bit_to_type(ts);
		} else {
			pin->type = ice_synce_pin_freq_to_type(hw, pin_idx,
							       input);
		}
	}
}

/**
 * ice_synce_release_info
 * @pf: Board private structure
 *
 * Release memory allocated by ice_synce_init_info.
 */
static void ice_synce_release_info(struct ice_pf *pf)
{
	struct ice_synce *se = &pf->synce;

	kfree(se->inputs);
	se->inputs = NULL;
	kfree(se->outputs);
	se->outputs = NULL;
}

/**
 * ice_synce_init_pins
 * @hw: Board private structure
 * @input: input or output pins
 * @num_pins: number of pins to be initialized
 * @pins: pointer to the pins array
 * @dpll_idx: index of dpll for getting priorities
 *
 * Init info about input or output pins, cache them in pins struct.
 */
static int ice_synce_init_pins(struct ice_hw *hw, bool input, int num_pins,
			       struct ice_synce_pin *pins, u8 dpll_idx)
{
	int ret;
	u8 i;

	for (i = 0; i < num_pins; i++) {
		pins[i].num_types =
			ice_cgu_get_pin_num_types_supported(hw, i, input);
		ice_synce_init_pin_types(hw, input, i, &pins[i]);
		if (input) {
			ret = ice_aq_get_cgu_ref_prio(hw, dpll_idx, i,
						      &pins[i].prio);
			if (ret)
				return ret;
			ret = ice_aq_get_input_pin_cfg(hw, i, NULL, NULL,
						       NULL, &pins[i].flags,
						       NULL, NULL);
			if (ret)
				return ret;
		} else {
			ret = ice_aq_get_output_pin_cfg(hw, i, &pins[i].flags,
							NULL, NULL, NULL);
		}
	}

	return 0;
}

/**
 * ice_synce_init_info
 * @pf: Board private structure
 *
 * Acquire (from HW) and set basic dpll information (on pf->synce struct).
 * Return:
 * * 0 - success
 * * negative - AQ error
 */
static int ice_synce_init_info(struct ice_pf *pf)
{
	struct ice_aqc_get_cgu_abilities abilities;
	struct ice_synce *se = &pf->synce;
	struct ice_hw *hw = &pf->hw;
	int ret, alloc_size;

	ret = ice_aq_get_cgu_abilities(hw, &abilities);
	if (ret) {
		dev_err(ice_pf_to_dev(pf),
			"err:%d %s failed to read cgu abilities\n",
			ret, ice_aq_str(hw->adminq.sq_last_status));
		return ret;
	}
	se->dpll_idx = abilities.synce_dpll_idx;
	se->num_inputs = abilities.num_inputs;
	alloc_size = sizeof(*se->inputs) * se->num_inputs;
	se->inputs = kmalloc(alloc_size, GFP_KERNEL);
	if (!se->inputs)
		return -ENOMEM;
	ret = ice_synce_init_pins(hw, true, se->num_inputs, se->inputs,
				  se->dpll_idx);
	if (ret)
		goto release_info;
	se->num_outputs = abilities.num_outputs;
	alloc_size = sizeof(*se->outputs) * se->num_outputs;
	se->outputs = kmalloc(alloc_size, GFP_KERNEL);
	if (!se->outputs)
		goto release_info;
	ret = ice_synce_init_pins(hw, false, se->num_outputs, se->outputs,
				  se->dpll_idx);
	if (ret)
		goto release_info;
	dev_info(ice_pf_to_dev(pf), "SyncE dpll init: inputs:%u, outputs:%u\n",
		 abilities.num_inputs, abilities.num_outputs);

	return 0;

release_info:
	ice_synce_release_info(pf);
	return ret;
}

/**
 * ice_synce_init_dpll
 * @pf: Board private structure
 * @last_state: last known state of DPLL
 *
 * Allocate and register dpll in dpll subsystem.
 * Return:
 * * 0 - success
 * * -ENOMEM - allocation fails
 */
static int ice_synce_init_dpll(struct ice_pf *pf)
{
	struct device *dev = ice_pf_to_dev(pf);
	struct ice_synce *se = &pf->synce;
	char name[DPLL_NAME_LENGTH];

	snprintf(name, DPLL_NAME_LENGTH, "%s-SyncE-%s",
		 dev_driver_string(dev), dev_name(dev));

	se->dpll = dpll_device_alloc(&ice_synce_dpll_ops, name, se->num_inputs,
				     se->num_outputs, pf);
	if (!se->dpll) {
		dev_err(ice_pf_to_dev(pf), "dpll_device_alloc failed\n");
		return -ENOMEM;
	}
	dpll_device_register(se->dpll);

	return 0;
}

/**
 * ice_synce_update_dpll_state
 * @pf: Board private structure
 * @last_state: last known state of DPLL
 *
 * Poll current state of dpll from hw and update pf->synce struct.
 * Return:
 * * 0 - success
 * * negative - AQ failure
 */
static int ice_synce_update_dpll_state(struct ice_pf *pf,
				       enum ice_cgu_state last_state)
{
	struct ice_synce *se = &pf->synce;
	int ret;

	ret = ice_get_cgu_state(&pf->hw, se->dpll_idx, last_state,
				&se->current_source, &se->ref_state,
				&se->eec_mode, &se->phase_offset,
				&se->dpll_state);
	if (ret)
		dev_err(ice_pf_to_dev(pf), "update dpll state failed, ret=%d %s\n",
			ret, ice_aq_str(pf->hw.adminq.sq_last_status));

	return ret;
}

/**
 * ice_synce_periodic_work - SyncE periodic worker.
 * @work: kthread_work
 *
 * SyncE periodic worker is responsible for polling state of dpll.
 */
static void ice_synce_periodic_work(struct kthread_work *work)
{
	struct ice_synce *synce = container_of(work, struct ice_synce,
					       work.work);
	struct ice_pf *pf = container_of(synce, struct ice_pf, synce);
	int ret;

	mutex_lock(&pf->synce.lock);
	ret = ice_synce_update_dpll_state(pf, synce->dpll_state);
	if (ret) {
		synce->cgu_state_acq_err_num++;
		/* stop rescheduling this worker */
		if (synce->cgu_state_acq_err_num >
		    SYNCE_CGU_STATE_ACQ_ERR_THRESHOLD) {
			dev_err(ice_pf_to_dev(pf),
				"SyncE periodic work disabled\n");
			return;
		}
		if (synce->last_dpll_state != synce->dpll_state) {
			synce->last_dpll_state = synce->dpll_state;
			/* Need to notify dpll user here by dpll interface once
			 * it is provided there
			 */
		}
	}
	mutex_unlock(&pf->synce.lock);
	/* Run twice a second or reschedule if update failed */
	kthread_queue_delayed_work(synce->kworker, &synce->work,
				   ret ? msecs_to_jiffies(10) :
				   msecs_to_jiffies(500));
}

/**
 * ice_synce_init - Initialize SyncE periodic worker.
 * @pf: Board private structure
 *
 * Create and start SyncE periodic worker.
 * Return:
 * * 0 - success
 * * negative - create worker failure
 */
static int ice_synce_init_dpll_worker(struct ice_pf *pf)
{
	struct ice_synce *se = &pf->synce;
	struct kthread_worker *kworker;

	ice_synce_update_dpll_state(pf, ICE_CGU_STATE_UNKNOWN);
	kthread_init_delayed_work(&se->work, ice_synce_periodic_work);
	kworker = kthread_create_worker(0, "ice-SyncE-%s",
					dev_name(ice_pf_to_dev(pf)));
	if (IS_ERR(kworker))
		return PTR_ERR(kworker);
	se->kworker = kworker;
	se->cgu_state_acq_err_num = 0;
	kthread_queue_delayed_work(se->kworker, &se->work, 0);

	return 0;
}

/**
 * ice_synce_init - Initialize SyncE support
 * @pf: Board private structure
 *
 * Set up the device as owner of SyncE feature.
 * Register in dpll subsystem, and allow userpsace to obtain state of PLL
 * and handle its configuration requests.
 * Return:
 * * 0 - success
 * * negative - init failure
 */
int ice_synce_init(struct ice_pf *pf)
{
	int err;

	mutex_init(&pf->synce.lock);
	mutex_lock(&pf->synce.lock);

	err = ice_synce_init_info(pf);
	if (err)
		goto unlock;
	err = ice_synce_init_dpll_worker(pf);
	if (err)
		goto free_info;
	err = ice_synce_init_dpll(pf);
	if (err)
		goto free_info;
	dev_dbg(ice_pf_to_dev(pf), "SyncE init successful\n");
	mutex_unlock(&pf->synce.lock);

	return err;
free_info:
	ice_synce_release_info(pf);
unlock:
	mutex_unlock(&pf->synce.lock);
	return err;
}

/**
 * ice_synce_release - Disable the driver/HW support for SyncE and unregister
 * the dpll.
 * @pf: Board private structure
 *
 * This function handles the cleanup work required from the initialization by
 * clearing out the important information and unregistering the dpll.
 */
void ice_synce_release(struct ice_pf *pf)
{
	struct ice_synce *se = &pf->synce;

	mutex_lock(&se->lock);
	ice_synce_release_info(pf);
	if (se->dpll) {
		dpll_device_unregister(se->dpll);
		dpll_device_free(se->dpll);
		dev_dbg(ice_pf_to_dev(pf), "SyncE dpll removed\n");
	}
	kthread_cancel_delayed_work_sync(&se->work);
	if (se->kworker) {
		kthread_destroy_worker(se->kworker);
		se->kworker = NULL;
		dev_dbg(ice_pf_to_dev(pf), "SyncE worker removed\n");
	}
	mutex_unlock(&se->lock);
	mutex_destroy(&se->lock);
}

