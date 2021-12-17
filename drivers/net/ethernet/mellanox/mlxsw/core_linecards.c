// SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0
/* Copyright (c) 2021 NVIDIA Corporation and Mellanox Technologies. All rights reserved */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/workqueue.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "core.h"

struct mlxsw_linecard_ini_file {
	__le16 size;
	union {
		u8 data[0];
		struct {
			u8 __dontcare[7];
			u8 type;
			u8 name[20];
		} format;
	};
};

struct mlxsw_linecard_types_info {
	struct mlxsw_linecard_ini_file **ini_files;
	unsigned int count;
	size_t data_size;
	char *data;
};

static const char *
mlxsw_linecard_types_lookup(struct mlxsw_linecards *linecards,
			    enum mlxsw_reg_mddq_card_type card_type)
{
	struct mlxsw_linecard_types_info *types_info;
	struct mlxsw_linecard_ini_file *ini_file;
	int i;

	types_info = linecards->types_info;
	for (i = 0; i < types_info->count; i++) {
		ini_file = linecards->types_info->ini_files[i];
		if (ini_file->format.type == card_type)
			return ini_file->format.name;
	}
	return NULL;
}

static const char *mlxsw_linecard_type_name(struct mlxsw_linecard *linecard)
{
	struct mlxsw_core *mlxsw_core = linecard->linecards->mlxsw_core;
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	int err;

	mlxsw_reg_mddq_slot_name_pack(mddq_pl, linecard->slot_index);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
	if (err)
		return NULL;
	mlxsw_reg_mddq_slot_name_unpack(mddq_pl, linecard->read_name);
	return linecard->read_name;
}

static void mlxsw_linecard_provision_fail(struct mlxsw_linecard *linecard)
{
	linecard->provisioned = false;
	devlink_linecard_provision_fail(linecard->devlink_linecard);
}

static int
mlxsw_linecard_provision_set(struct mlxsw_linecards *linecards,
			     struct mlxsw_linecard *linecard,
			     enum mlxsw_reg_mddq_card_type card_type)
{
	const char *type = mlxsw_linecard_types_lookup(linecards, card_type);

	if (!type)
		type = mlxsw_linecard_type_name(linecard);
	if (!type) {
		mlxsw_linecard_provision_fail(linecard);
		return -EINVAL;
	}
	linecard->provisioned = true;
	devlink_linecard_provision_set(linecard->devlink_linecard, type);
	return 0;
}

static void mlxsw_linecard_provision_clear(struct mlxsw_linecard *linecard)
{
	linecard->provisioned = false;
	devlink_linecard_provision_clear(linecard->devlink_linecard);
}

static int __mlxsw_linecard_status_process(struct mlxsw_core *mlxsw_core,
					   struct mlxsw_linecards *linecards,
					   struct mlxsw_linecard *linecard,
					   const char *mddq_pl)
{
	enum mlxsw_reg_mddq_card_type card_type;
	enum mlxsw_reg_mddq_ready ready;
	bool provisioned;
	u16 ini_version;
	u16 hw_revision;
	bool sr_valid;
	u8 slot_index;
	int err = 0;
	bool active;

	mlxsw_reg_mddq_slot_info_unpack(mddq_pl, &slot_index, &provisioned,
					&sr_valid, &ready, &active,
					&hw_revision, &ini_version,
					&card_type);

	if (linecard) {
		if (slot_index != linecard->slot_index)
			return -EINVAL;
	} else {
		if (slot_index > linecards->count)
			return -EINVAL;
		linecard = mlxsw_linecard_get(linecards, slot_index);
	}

	mutex_lock(&linecard->lock);

	if (provisioned && linecard->provisioned != provisioned) {
		err = mlxsw_linecard_provision_set(linecards, linecard,
						   card_type);
		if (err)
			goto out;
	}

	if (!provisioned && linecard->provisioned != provisioned)
		mlxsw_linecard_provision_clear(linecard);

out:
	mutex_unlock(&linecard->lock);
	return err;
}

int mlxsw_linecard_status_process(struct mlxsw_core *mlxsw_core,
				  const char *mddq_pl)
{
	struct mlxsw_linecards *linecards = mlxsw_core_linecards(mlxsw_core);

	return __mlxsw_linecard_status_process(mlxsw_core, linecards, NULL,
					       mddq_pl);
}
EXPORT_SYMBOL(mlxsw_linecard_status_process);

static int mlxsw_linecard_status_get_and_process(struct mlxsw_core *mlxsw_core,
						 struct mlxsw_linecards *linecards,
						 struct mlxsw_linecard *linecard)
{
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	int err;

	mlxsw_reg_mddq_slot_info_pack(mddq_pl, linecard->slot_index, false);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
	if (err)
		return err;

	return __mlxsw_linecard_status_process(mlxsw_core, linecards, linecard,
					       mddq_pl);
}

static int __mlxsw_linecard_fix_fsm_state(struct mlxsw_linecard *linecard)
{
	dev_info(linecard->linecards->bus_info->dev, "linecard %u: Clearing FSM state error",
		 linecard->slot_index);
	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_CLEAR_ERRORS, 0,
			    false, false, NULL);
	return mlxsw_reg_write(linecard->linecards->mlxsw_core,
			       MLXSW_REG(mbct), linecard->mbct_pl);
}

static int mlxsw_linecard_fix_fsm_state(struct mlxsw_linecard *linecard,
					enum mlxsw_reg_mbct_fsm_state fsm_state)
{
	if (fsm_state != MLXSW_REG_MBCT_FSM_STATE_ERROR)
		return 0;
	return __mlxsw_linecard_fix_fsm_state(linecard);
}

static int mlxsw_linecard_query_status(struct mlxsw_linecard *linecard,
				       enum mlxsw_reg_mbct_status *status,
				       enum mlxsw_reg_mbct_fsm_state *fsm_state,
				       struct netlink_ext_ack *extack)
{
	bool second_try = false;
	int err;

another_try:
	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_QUERY_STATUS, 0,
			    false, false, NULL);
	err = mlxsw_reg_query(linecard->linecards->mlxsw_core, MLXSW_REG(mbct),
			      linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to query linecard INI status");
		return err;
	}
	mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL, status, fsm_state);
	if (!second_try &&
	    (*status == MLXSW_REG_MBCT_STATUS_ILLEGAL_OPERATION ||
	     *fsm_state == MLXSW_REG_MBCT_FSM_STATE_ERROR)) {
		err = __mlxsw_linecard_fix_fsm_state(linecard);
		if (!err) {
			second_try = true;
			goto another_try;
		}
	}
	return err;
}

static int
mlxsw_linecard_provision_data(struct mlxsw_core *mlxsw_core,
			      struct mlxsw_linecards *linecards,
			      struct mlxsw_linecard *linecard,
			      const struct mlxsw_linecard_ini_file *ini_file,
			      struct netlink_ext_ack *extack)
{
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	enum mlxsw_reg_mbct_status status;
	size_t size_left;
	const u8 *data;
	int err;

	size_left = le16_to_cpu(ini_file->size);
	data = ini_file->data;
	while (size_left) {
		size_t data_size = MLXSW_REG_MBCT_DATA_LEN;
		bool is_last = false;

		if (size_left <= MLXSW_REG_MBCT_DATA_LEN) {
			data_size = size_left;
			is_last = true;
		}

		mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
				    MLXSW_REG_MBCT_OP_DATA_TRANSFER, data_size,
				    is_last, false, data);
		err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct),
				      linecard->mbct_pl);
		if (err) {
			NL_SET_ERR_MSG_MOD(extack, "Failed to transfer linecard INI data");
			return err;
		}
		mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL,
				      &status, &fsm_state);
		if ((!is_last && status != MLXSW_REG_MBCT_STATUS_PART_DATA) ||
		    (is_last && status != MLXSW_REG_MBCT_STATUS_LAST_DATA)) {
			NL_SET_ERR_MSG_MOD(extack, "Failed to transfer linecard INI data");
			mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
			return -EINVAL;
		}
		size_left -= data_size;
		data += data_size;
	}

	return 0;
}

int mlxsw_linecard_bct_process(struct mlxsw_core *mlxsw_core,
			       const char *mbct_pl)
{
	struct mlxsw_linecards *linecards = mlxsw_core_linecards(mlxsw_core);
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	enum mlxsw_reg_mbct_status status;
	struct mlxsw_linecard *linecard;
	u8 slot_index;
	int err;

	mlxsw_reg_mbct_unpack(mbct_pl, &slot_index, &status, &fsm_state);
	if (slot_index > linecards->count)
		return -EINVAL;
	linecard = mlxsw_linecard_get(linecards, slot_index);
	if (status == MLXSW_REG_MBCT_STATUS_ACTIVATION_FAILED) {
		dev_err(linecards->bus_info->dev, "linecard %u: Failed to activate INI",
			linecard->slot_index);
		err = -EINVAL;
		goto fix_fsm_err_out;
	}
	return 0;

fix_fsm_err_out:
	mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
	mlxsw_linecard_provision_fail(linecard);
	return err;
}
EXPORT_SYMBOL(mlxsw_linecard_bct_process);

static int mlxsw_linecard_provision(struct devlink_linecard *devlink_linecard,
				    void *priv, const char *type,
				    const void *type_priv,
				    struct netlink_ext_ack *extack)
{
	const struct mlxsw_linecard_ini_file *ini_file = type_priv;
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	struct mlxsw_linecard *linecard = priv;
	struct mlxsw_linecards *linecards;
	enum mlxsw_reg_mbct_status status;
	struct mlxsw_core *mlxsw_core;
	int err;

	mutex_lock(&linecard->lock);

	linecards = linecard->linecards;
	mlxsw_core = linecards->mlxsw_core;
	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_ERASE_INI_IMAGE, 0,
			    false, false, NULL);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct), linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI");
		goto err_out;
	}
	mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL, &status, &fsm_state);
	if (status == MLXSW_REG_MBCT_STATUS_ERASE_FAILED) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI");
		err = -EINVAL;
		goto fix_fsm_err_out;
	}

	err = mlxsw_linecard_provision_data(mlxsw_core, linecards,
					    linecard, ini_file, extack);
	if (err)
		goto err_out;

	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_ACTIVATE, 0,
			    false, true, NULL);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct), linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to activate linecard INI");
		goto err_out;
	}
	mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL, &status, &fsm_state);
	if (status == MLXSW_REG_MBCT_STATUS_ACTIVATION_FAILED) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to activate linecard INI");
		goto fix_fsm_err_out;
	}

	goto out;

fix_fsm_err_out:
	mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
err_out:
	mlxsw_linecard_provision_fail(linecard);
out:
	mutex_unlock(&linecard->lock);
	return err;
}

#define MLXSW_LINECARD_INI_WAIT_RETRIES 10
#define MLXSW_LINECARD_INI_WAIT_MS 500

static int mlxsw_linecard_unprovision(struct devlink_linecard *devlink_linecard,
				      void *priv,
				      struct netlink_ext_ack *extack)
{
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	struct mlxsw_linecard *linecard = priv;
	struct mlxsw_linecards *linecards;
	enum mlxsw_reg_mbct_status status;
	unsigned int ini_wait_retries = 0;
	struct mlxsw_core *mlxsw_core;
	int err;

	mutex_lock(&linecard->lock);

	linecards = linecard->linecards;
	mlxsw_core = linecard->linecards->mlxsw_core;

query_ini_status:
	err = mlxsw_linecard_query_status(linecard, &status,
					  &fsm_state, extack);
	if (err)
		goto err_out;

	switch (fsm_state) {
	case MLXSW_REG_MBCT_FSM_STATE_INI_IN_USE:
		if (ini_wait_retries++ > MLXSW_LINECARD_INI_WAIT_RETRIES) {
			NL_SET_ERR_MSG_MOD(extack, "Failed to wait for linecard INI to be not used");
			goto err_out;
		}
		mdelay(MLXSW_LINECARD_INI_WAIT_MS);
		goto query_ini_status;
	default:
		break;
	}

	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_ERASE_INI_IMAGE, 0,
			    false, false, NULL);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct),
			      linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI");
		goto err_out;
	}
	mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL, &status, &fsm_state);
	switch (status) {
	case MLXSW_REG_MBCT_STATUS_ERASE_COMPLETE:
		break;
	default:
		/* Should not happen */
		fallthrough;
	case MLXSW_REG_MBCT_STATUS_ERASE_FAILED:
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI");
		goto fix_fsm_err_out;
	case MLXSW_REG_MBCT_STATUS_ERROR_INI_IN_USE:
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI while being used");
		goto fix_fsm_err_out;
	}
	goto out;

fix_fsm_err_out:
	mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
err_out:
	mlxsw_linecard_provision_fail(linecard);
out:
	mutex_unlock(&linecard->lock);
	return err;
}

static unsigned int
mlxsw_linecard_types_count(struct devlink_linecard *devlink_linecard,
			   void *priv)
{
	struct mlxsw_linecard *linecard = priv;

	return linecard->linecards->types_info->count;
}

static void mlxsw_linecard_types_get(struct devlink_linecard *devlink_linecard,
				     void *priv, unsigned int index,
				     const char **type, const void **type_priv)
{
	struct mlxsw_linecard_types_info *types_info;
	struct mlxsw_linecard_ini_file *ini_file;
	struct mlxsw_linecard *linecard = priv;

	types_info = linecard->linecards->types_info;
	ini_file = types_info->ini_files[index];
	*type = ini_file->format.name;
	*type_priv = ini_file;
}

static const struct devlink_linecard_ops mlxsw_linecard_ops = {
	.provision = mlxsw_linecard_provision,
	.unprovision = mlxsw_linecard_unprovision,
	.types_count = mlxsw_linecard_types_count,
	.types_get = mlxsw_linecard_types_get,
};

static int mlxsw_linecard_init(struct mlxsw_core *mlxsw_core,
			       struct mlxsw_linecards *linecards,
			       u8 slot_index)
{
	struct devlink_linecard *devlink_linecard;
	struct mlxsw_linecard *linecard;
	int err;

	linecard = mlxsw_linecard_get(linecards, slot_index);
	linecard->slot_index = slot_index;
	linecard->linecards = linecards;
	mutex_init(&linecard->lock);

	devlink_linecard = devlink_linecard_create(priv_to_devlink(mlxsw_core),
						   slot_index, &mlxsw_linecard_ops,
						   linecard);
	if (IS_ERR(devlink_linecard))
		return PTR_ERR(devlink_linecard);
	linecard->devlink_linecard = devlink_linecard;

	err = mlxsw_linecard_status_get_and_process(mlxsw_core, linecards,
						    linecard);
	if (err)
		goto err_status_get_and_process;

	return 0;

err_status_get_and_process:
	devlink_linecard_destroy(linecard->devlink_linecard);
	return err;
}

static int mlxsw_linecard_event_delivery_set(struct mlxsw_core *mlxsw_core,
					     struct mlxsw_linecard *linecard,
					     bool enable)
{
	char mddq_pl[MLXSW_REG_MDDQ_LEN];

	mlxsw_reg_mddq_slot_info_pack(mddq_pl, linecard->slot_index, enable);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
}

static int mlxsw_linecard_post_init(struct mlxsw_core *mlxsw_core,
				    struct mlxsw_linecards *linecards,
				    u8 slot_index)
{
	struct mlxsw_linecard *linecard;
	int err;

	linecard = mlxsw_linecard_get(linecards, slot_index);
	linecard->slot_index = slot_index;

	err = mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, true);
	if (err)
		return err;

	err = mlxsw_linecard_status_get_and_process(mlxsw_core, linecards,
						    linecard);
	if (err)
		goto err_status_get_and_process;

	return 0;

err_status_get_and_process:
	mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, false);
	return err;
}

static void mlxsw_linecard_pre_fini(struct mlxsw_core *mlxsw_core,
				    struct mlxsw_linecards *linecards,
				    u8 slot_index)
{
	struct mlxsw_linecard *linecard;

	linecard = mlxsw_linecard_get(linecards, slot_index);
	mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, false);
}

static void mlxsw_linecard_fini(struct mlxsw_core *mlxsw_core,
				struct mlxsw_linecards *linecards,
				u8 slot_index)
{
	struct mlxsw_linecard *linecard;

	linecard = mlxsw_linecard_get(linecards, slot_index);
	devlink_linecard_destroy(linecard->devlink_linecard);
	mutex_destroy(&linecard->lock);
}

#define MLXSW_LINECARDS_INI_BUNDLE_MAGIC "NVLCINI+"

static int mlxsw_linecard_types_init(struct mlxsw_core *mlxsw_core,
				     struct mlxsw_linecards *linecards)
{
	struct mlxsw_linecard_types_info *types_info;
	struct mlxsw_linecard_ini_file *ini_file;
	const char *lc_ini_bundle_filename;
	const struct firmware *firmware;
	unsigned int count;
	u16 ini_file_size;
	size_t magic_size;
	const u8 *data;
	size_t size;
	int err;

	types_info = kzalloc(sizeof(*types_info), GFP_KERNEL);
	if (!types_info)
		return -ENOMEM;
	linecards->types_info = types_info;

	lc_ini_bundle_filename = mlxsw_core_lc_ini_bundle_filename(mlxsw_core);
	if (!lc_ini_bundle_filename)
		return 0;

	err = request_firmware_direct(&firmware, lc_ini_bundle_filename,
				      linecards->bus_info->dev);
	if (err) {
		dev_warn(linecards->bus_info->dev, "Could not request linecards INI file \"%s\", provisioning will not be possible\n",
			 lc_ini_bundle_filename);
		return 0;
	}

	types_info->data_size = firmware->size;
	types_info->data = kmemdup(firmware->data, firmware->size, GFP_KERNEL);
	release_firmware(firmware);
	if (!types_info->data) {
		err = -ENOMEM;
		goto free_types_info;
	}

	data = types_info->data;
	size = types_info->data_size;
	magic_size = strlen(MLXSW_LINECARDS_INI_BUNDLE_MAGIC);

	if (size < magic_size ||
	    memcmp(data, MLXSW_LINECARDS_INI_BUNDLE_MAGIC, magic_size))
		goto incorrect_inis_file_format;
	data += magic_size;
	size -= magic_size;
	count = 0;

	while (size > 0) {
		if (size < sizeof(*ini_file))
			goto incorrect_inis_file_format;
		ini_file = (struct mlxsw_linecard_ini_file *) data;
		ini_file_size = le16_to_cpu(ini_file->size);
		if (ini_file_size > size || ini_file_size % 4)
			goto incorrect_inis_file_format;
		data += ini_file_size + sizeof(__le16);
		size -= ini_file_size + sizeof(__le16);
		count++;
	}
	if (size)
		goto incorrect_inis_file_format;

	types_info->ini_files = kmalloc_array(count, sizeof(ini_file),
					      GFP_KERNEL);
	if (!types_info->ini_files) {
		err = -ENOMEM;
		goto free_types_info;
	}

	data = types_info->data + magic_size;
	size = types_info->data_size - magic_size;
	count = 0;

	while (size) {
		int i;

		ini_file = (struct mlxsw_linecard_ini_file *) data;
		ini_file_size = le16_to_cpu(ini_file->size);
		for (i = 0; i < ini_file_size / 4; i++) {
			u32 *val = &((u32 *) ini_file->data)[i];

			*val = swab32(*val);
		}
		types_info->ini_files[count] = ini_file;
		data += ini_file_size + sizeof(__le16);
		size -= ini_file_size + sizeof(__le16);
		count++;
	}

	types_info->count = count;
	return 0;

incorrect_inis_file_format:
	dev_warn(linecards->bus_info->dev, "Incorrect linecards INIs file format, provisioning will not be possible\n");
	return 0;

free_types_info:
	kfree(types_info);
	return err;
}

static void mlxsw_linecard_types_fini(struct mlxsw_linecards *linecards)
{
	struct mlxsw_linecard_types_info *types_info = linecards->types_info;

	kfree(types_info->ini_files);
	kfree(types_info->data);
	kfree(types_info);
}

int mlxsw_linecards_init(struct mlxsw_core *mlxsw_core,
			 const struct mlxsw_bus_info *bus_info,
			 struct mlxsw_linecards **p_linecards)
{
	char mgpir_pl[MLXSW_REG_MGPIR_LEN];
	struct mlxsw_linecards *linecards;
	u8 slot_count;
	int err;
	int i;

	mlxsw_reg_mgpir_pack(mgpir_pl, 0);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mgpir), mgpir_pl);
	if (err)
		return err;

	mlxsw_reg_mgpir_unpack(mgpir_pl, NULL, NULL, NULL,
			       NULL, &slot_count);
	if (!slot_count) {
		*p_linecards = NULL;
		return 0;
	}

	linecards = kzalloc(struct_size(linecards, linecards, slot_count),
			    GFP_KERNEL);
	if (!linecards)
		return -ENOMEM;
	linecards->count = slot_count;
	linecards->mlxsw_core = mlxsw_core;
	linecards->bus_info = bus_info;

	linecards->wq = alloc_workqueue("mlxsw_linecards", 0, 0);
	if (!linecards->wq) {
		err = ENOMEM;
		goto err_wq_alloc;
	}

	err = mlxsw_linecard_types_init(mlxsw_core, linecards);
	if (err)
		goto err_types_init;

	for (i = 0; i < linecards->count; i++) {
		err = mlxsw_linecard_init(mlxsw_core, linecards, i + 1);
		if (err)
			goto err_linecard_init;
	}

	*p_linecards = linecards;

	return 0;

err_linecard_init:
	for (i--; i >= 0; i--)
		mlxsw_linecard_fini(mlxsw_core, linecards, i + 1);
err_types_init:
	destroy_workqueue(linecards->wq);
err_wq_alloc:
	kfree(linecards);

	return err;
}

int mlxsw_linecards_post_init(struct mlxsw_core *mlxsw_core,
			      struct mlxsw_linecards *linecards)
{
	int err;
	int i;

	if (!linecards)
		return 0;

	for (i = 0; i < linecards->count; i++) {
		err = mlxsw_linecard_post_init(mlxsw_core, linecards, i + 1);
		if (err)
			goto err_linecard_post_init;
	}
	return 0;

err_linecard_post_init:
	for (i--; i >= 0; i--)
		mlxsw_linecard_pre_fini(mlxsw_core, linecards, i + 1);

	return err;
}

void mlxsw_linecards_pre_fini(struct mlxsw_core *mlxsw_core,
			      struct mlxsw_linecards *linecards)
{
	int i;

	if (!linecards)
		return;
	for (i = 0; i < linecards->count; i++)
		mlxsw_linecard_pre_fini(mlxsw_core, linecards, i + 1);
	/* Make sure all scheduled events are processed */
	mlxsw_core_flush_owq();
}

void mlxsw_linecards_fini(struct mlxsw_core *mlxsw_core,
			  struct mlxsw_linecards *linecards)
{
	int i;

	if (!linecards)
		return;
	for (i = 0; i < linecards->count; i++)
		mlxsw_linecard_fini(mlxsw_core, linecards, i + 1);
	mlxsw_linecard_types_fini(linecards);
	destroy_workqueue(linecards->wq);
	kfree(linecards);
}
