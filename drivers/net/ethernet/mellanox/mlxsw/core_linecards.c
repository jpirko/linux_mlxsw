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
#include <linux/vmalloc.h>

#include "core.h"
#include "../mlxfw/mlxfw.h"

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
mlxsw_linecard_types_lookup(struct mlxsw_linecards *linecards, u8 card_type)
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
		return ERR_PTR(err);
	mlxsw_reg_mddq_slot_name_unpack(mddq_pl, linecard->read_name);
	return linecard->read_name;
}

struct mlxsw_linecard_device_info {
	u16 fw_major;
	u16 fw_minor;
	u16 fw_sub_minor;
};

struct mlxsw_linecard_device {
	struct list_head list;
	u8 index;
	char component_name[16];
	struct mlxsw_linecard *linecard;
	struct devlink_linecard_device *devlink_device;
	struct mlxsw_linecard_device_info info;
};

static struct mlxsw_linecard_device *
mlxsw_linecard_device_lookup(struct mlxsw_linecard *linecard, u8 index)
{
	struct mlxsw_linecard_device *device;

	list_for_each_entry(device, &linecard->device_list, list)
		if (device->index == index)
			return device;
	return NULL;
}

struct mlxsw_linecard_device_fw_info {
	struct mlxfw_dev mlxfw_dev;
	struct mlxsw_core *mlxsw_core;
	struct mlxsw_linecard_device *device;
};

static const char *
mlxsw_linecard_device_fw_psid_get(struct mlxfw_dev *mlxfw_dev, u16 *psid_size)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mgir_pl;
	char *psid;
	int err;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_QUERY,
			    MLXSW_REG(mgir), &mgir_pl);

	mlxsw_reg_mgir_pack(mgir_pl);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
	if (err)
		return ERR_PTR(err);
	psid = kzalloc(MLXSW_REG_MGIR_FW_INFO_PSID_SIZE, GFP_KERNEL);
	if (!psid)
		return ERR_PTR(-ENOMEM);

	mlxsw_reg_mgir_fw_info_psid_memcpy_from(mgir_pl, psid);
	*psid_size = strlen(psid);
	return psid;
}

static void mlxsw_linecard_device_fw_psid_put(const char *psid)
{
	kfree(psid);
}

static int mlxsw_linecard_device_fw_component_query(struct mlxfw_dev *mlxfw_dev,
						    u16 component_index,
						    u32 *p_max_size,
						    u8 *p_align_bits,
						    u16 *p_max_write_size)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcqi_pl;
	int err;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_QUERY,
			    MLXSW_REG(mcqi), &mcqi_pl);

	mlxsw_reg_mcqi_pack(mcqi_pl, component_index);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
	if (err)
		return err;
	mlxsw_reg_mcqi_unpack(mcqi_pl, p_max_size, p_align_bits,
			      p_max_write_size);

	*p_align_bits = max_t(u8, *p_align_bits, 2);
	*p_max_write_size = min_t(u16, *p_max_write_size,
				  MLXSW_REG_MCDA_MAX_DATA_LEN);
	return 0;
}

static int mlxsw_linecard_device_fw_fsm_lock(struct mlxfw_dev *mlxfw_dev,
					     u32 *fwhandle)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	u8 control_state;
	char *mcc_pl;
	int err;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_QUERY,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, 0, 0, 0, 0);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
	if (err)
		return err;

	mlxsw_reg_mcc_unpack(mcc_pl, fwhandle, NULL, &control_state);
	if (control_state != MLXFW_FSM_STATE_IDLE)
		return -EBUSY;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_QUERY,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, MLXSW_REG_MCC_INSTRUCTION_LOCK_UPDATE_HANDLE,
			   0, *fwhandle, 0);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static int
mlxsw_linecard_device_fw_fsm_component_update(struct mlxfw_dev *mlxfw_dev,
					      u32 fwhandle,
					      u16 component_index,
					      u32 component_size)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcc_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, MLXSW_REG_MCC_INSTRUCTION_UPDATE_COMPONENT,
			   component_index, fwhandle, component_size);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static int
mlxsw_linecard_device_fw_fsm_block_download(struct mlxfw_dev *mlxfw_dev,
					    u32 fwhandle, u8 *data,
					    u16 size, u32 offset)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcda_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcda), &mcda_pl);
	mlxsw_reg_mcda_pack(mcda_pl, fwhandle, offset, size, data);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static int
mlxsw_linecard_device_fw_fsm_component_verify(struct mlxfw_dev *mlxfw_dev,
					      u32 fwhandle, u16 component_index)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcc_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, MLXSW_REG_MCC_INSTRUCTION_VERIFY_COMPONENT,
			   component_index, fwhandle, 0);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static int mlxsw_linecard_device_fw_fsm_activate(struct mlxfw_dev *mlxfw_dev,
						 u32 fwhandle)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcc_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, MLXSW_REG_MCC_INSTRUCTION_ACTIVATE,
			   0, fwhandle, 0);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static int
mlxsw_linecard_device_fw_fsm_query_state(struct mlxfw_dev *mlxfw_dev,
					 u32 fwhandle,
					 enum mlxfw_fsm_state *fsm_state,
					 enum mlxfw_fsm_state_err *fsm_state_err)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	u8 control_state;
	u8 error_code;
	char *mcc_pl;
	int err;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_QUERY,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, 0, 0, fwhandle, 0);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
	if (err)
		return err;

	mlxsw_reg_mcc_unpack(mcc_pl, NULL, &error_code, &control_state);
	*fsm_state = control_state;
	*fsm_state_err = min_t(enum mlxfw_fsm_state_err, error_code,
			       MLXFW_FSM_STATE_ERR_MAX);
	return 0;
}

static void mlxsw_linecard_device_fw_fsm_cancel(struct mlxfw_dev *mlxfw_dev,
						u32 fwhandle)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcc_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl, MLXSW_REG_MCC_INSTRUCTION_CANCEL,
			   0, fwhandle, 0);
	mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static void mlxsw_linecard_device_fw_fsm_release(struct mlxfw_dev *mlxfw_dev,
						 u32 fwhandle)
{
	struct mlxsw_linecard_device_fw_info *info =
		container_of(mlxfw_dev, struct mlxsw_linecard_device_fw_info,
			     mlxfw_dev);
	struct mlxsw_linecard_device *device = info->device;
	struct mlxsw_core *mlxsw_core = info->mlxsw_core;
	char mddt_pl[MLXSW_REG_MDDT_LEN];
	char *mcc_pl;

	mlxsw_reg_mddt_pack(mddt_pl, device->linecard->slot_index,
			    device->index,
			    MLXSW_REG_MDDT_METHOD_WRITE,
			    MLXSW_REG(mcc), &mcc_pl);
	mlxsw_reg_mcc_pack(mcc_pl,
			   MLXSW_REG_MCC_INSTRUCTION_RELEASE_UPDATE_HANDLE,
			   0, fwhandle, 0);
	mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddt), mddt_pl);
}

static const struct mlxfw_dev_ops mlxsw_linecard_device_dev_ops = {
	.psid_get		= mlxsw_linecard_device_fw_psid_get,
	.psid_put		= mlxsw_linecard_device_fw_psid_put,
	.component_query	= mlxsw_linecard_device_fw_component_query,
	.fsm_lock		= mlxsw_linecard_device_fw_fsm_lock,
	.fsm_component_update	= mlxsw_linecard_device_fw_fsm_component_update,
	.fsm_block_download	= mlxsw_linecard_device_fw_fsm_block_download,
	.fsm_component_verify	= mlxsw_linecard_device_fw_fsm_component_verify,
	.fsm_activate		= mlxsw_linecard_device_fw_fsm_activate,
	.fsm_query_state	= mlxsw_linecard_device_fw_fsm_query_state,
	.fsm_cancel		= mlxsw_linecard_device_fw_fsm_cancel,
	.fsm_release		= mlxsw_linecard_device_fw_fsm_release,
};

static int mlxsw_linecard_device_fw_flash(struct mlxsw_core *mlxsw_core,
					  const struct firmware *firmware,
					  struct mlxsw_linecard_device *device,
					  struct netlink_ext_ack *extack)
{
	struct mlxsw_linecard_device_fw_info info = {
		.mlxfw_dev = {
			.ops = &mlxsw_linecard_device_dev_ops,
			.devlink = priv_to_devlink(mlxsw_core),
		},
		.mlxsw_core = mlxsw_core,
		.device = device,
	};

	return mlxfw_firmware_flash(&info.mlxfw_dev, firmware, extack);
}

static int mlxsw_linecard_device_flash_cb(struct mlxsw_core *mlxsw_core,
					  const struct firmware *firmware,
					  struct netlink_ext_ack *extack, void *priv)
{
	struct mlxsw_linecard_device *device = priv;

	return mlxsw_linecard_device_fw_flash(mlxsw_core, firmware,
					      device, extack);
}

static int mlxsw_linecard_device_attach(struct mlxsw_core *mlxsw_core,
					struct mlxsw_linecard *linecard,
					u8 device_index, bool flash_owner)
{
	struct mlxsw_linecard_device *device;
	char *component_name = NULL;
	int err;

	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device)
		return -ENOMEM;
	device->index = device_index;
	device->linecard = linecard;

	if (flash_owner) {
		snprintf(device->component_name,
			 sizeof(device->component_name), "lc%u_dev%u",
			 linecard->slot_index, device->index);
		component_name = device->component_name;
		err = mlxsw_core_flash_component_register(mlxsw_core,
							  component_name,
							  mlxsw_linecard_device_flash_cb,
							  device);
		if (err)
			goto err_flash_component_register;
	}

	device->devlink_device = devlink_linecard_device_create(linecard->devlink_linecard,
								device_index,
								component_name,
								device);
	if (IS_ERR(device->devlink_device)) {
		err = PTR_ERR(device->devlink_device);
		goto err_devlink_linecard_device_attach;
	}

	list_add_tail(&device->list, &linecard->device_list);
	return 0;

err_devlink_linecard_device_attach:
	if (flash_owner)
		mlxsw_core_flash_component_unregister(mlxsw_core,
						      device->component_name);
err_flash_component_register:
	kfree(device);
	return err;
}

static void mlxsw_linecard_device_detach(struct mlxsw_core *mlxsw_core,
					 struct mlxsw_linecard *linecard,
					 struct mlxsw_linecard_device *device)
{
	list_del(&device->list);
	devlink_linecard_device_destroy(linecard->devlink_linecard,
					device->devlink_device);
	if (strlen(device->component_name))
		mlxsw_core_flash_component_unregister(mlxsw_core,
						      device->component_name);
	kfree(device);
}

static int mlxsw_linecard_devices_attach(struct mlxsw_core *mlxsw_core,
					 struct mlxsw_linecard *linecard)
{
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	bool flash_owner;
	bool data_valid;
	u8 device_index;
	u8 msg_seq = 0;
	int err;

	do {
		mlxsw_reg_mddq_device_info_pack(mddq_pl, linecard->slot_index,
						msg_seq);
		err =  mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
		if (err)
			return err;
		mlxsw_reg_mddq_device_info_unpack(mddq_pl, &msg_seq,
						  &data_valid, &flash_owner,
						  &device_index, NULL,
						  NULL, NULL);
		if (!data_valid)
			break;
		err = mlxsw_linecard_device_attach(mlxsw_core, linecard,
						   device_index, flash_owner);
		if (err)
			return err;
	} while (msg_seq);

	return 0;
}

static void mlxsw_linecard_devices_detach(struct mlxsw_core *mlxsw_core,
					  struct mlxsw_linecard *linecard)
{
	struct mlxsw_linecard_device *device, *tmp;

	list_for_each_entry_safe(device, tmp, &linecard->device_list, list)
		mlxsw_linecard_device_detach(mlxsw_core, linecard, device);
}

static void mlxsw_linecard_device_update(struct mlxsw_core *mlxsw_core,
					 struct mlxsw_linecard *linecard,
					 u8 device_index,
					 struct mlxsw_linecard_device_info *info)
{
	struct mlxsw_linecard_device *device;

	device = mlxsw_linecard_device_lookup(linecard, device_index);
	if (!device)
		return;
	device->info = *info;
}

static int mlxsw_linecard_devices_update(struct mlxsw_core *mlxsw_core,
					 struct mlxsw_linecard *linecard)
{
	struct mlxsw_linecard_device_info info;
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	bool data_valid;
	u8 device_index;
	u8 msg_seq = 0;
	int err;

	do {
		mlxsw_reg_mddq_device_info_pack(mddq_pl, linecard->slot_index,
						msg_seq);
		err =  mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
		if (err)
			return err;
		mlxsw_reg_mddq_device_info_unpack(mddq_pl, &msg_seq,
						  &data_valid, NULL,
						  &device_index,
						  &info.fw_major,
						  &info.fw_minor,
						  &info.fw_sub_minor);
		if (!data_valid)
			break;
		mlxsw_linecard_device_update(mlxsw_core, linecard,
					     device_index, &info);
	} while (msg_seq);

	return 0;
}

static int
mlxsw_linecard_device_info_get(struct devlink_linecard_device *devlink_linecard_device,
			       void *priv, struct devlink_info_req *req,
			       struct netlink_ext_ack *extack)
{
	struct mlxsw_linecard_device *device = priv;
	struct mlxsw_linecard_device_info *info;
	char buf[32];

	if (!device->linecard->active)
		return 0;

	info = &device->info;

	sprintf(buf, "%u.%u.%u", info->fw_major, info->fw_minor,
		info->fw_sub_minor);
	return devlink_info_version_running_put(req,
						DEVLINK_INFO_VERSION_GENERIC_FW,
						buf);
}

static void mlxsw_linecard_provision_fail(struct mlxsw_core *mlxsw_core,
					  struct mlxsw_linecard *linecard)
{
	linecard->provisioned = false;
	linecard->ready = false;
	linecard->active = false;
	mlxsw_linecard_devices_detach(mlxsw_core, linecard);
	devlink_linecard_provision_fail(linecard->devlink_linecard);
}

struct mlxsw_linecards_event_ops_item {
	struct list_head list;
	struct mlxsw_linecards_event_ops *event_ops;
	void *priv;
};

static int
mlxsw_linecard_provision_cbs_call(struct mlxsw_core *mlxsw_core,
				  struct mlxsw_linecards *linecards,
				  struct mlxsw_linecard *linecard)
{
	struct mlxsw_linecards_event_ops_item *item;
	int err;

	list_for_each_entry(item, &linecards->event_ops_list, list) {
		if (!item->event_ops->got_provisioned)
			continue;
		err = item->event_ops->got_provisioned(mlxsw_core,
						       linecard->slot_index,
						       linecard, item->priv);
		if (err)
			goto rollback;
	}
	return 0;

rollback:
	list_for_each_entry_continue_reverse(item, &linecards->event_ops_list,
					     list) {
		if (!item->event_ops->got_unprovisioned)
			continue;
		item->event_ops->got_unprovisioned(mlxsw_core,
						   linecard->slot_index,
						   linecard, item->priv);
	}
	return err;
}

static void
mlxsw_linecard_unprovision_cbs_call(struct mlxsw_core *mlxsw_core,
				    struct mlxsw_linecards *linecards,
				    struct mlxsw_linecard *linecard)
{
	struct mlxsw_linecards_event_ops_item *item;

	list_for_each_entry(item, &linecards->event_ops_list, list) {
		if (!item->event_ops->got_unprovisioned)
			continue;
		item->event_ops->got_unprovisioned(mlxsw_core,
						   linecard->slot_index,
						   linecard, item->priv);
	}
}

static int
mlxsw_linecard_provision_set(struct mlxsw_core *mlxsw_core,
			     struct mlxsw_linecards *linecards,
			     struct mlxsw_linecard *linecard, u8 card_type)
{
	const char *type = mlxsw_linecard_types_lookup(linecards, card_type);
	int err;

	if (!type) {
		type = mlxsw_linecard_type_name(linecard);
		if (IS_ERR(type)) {
			mlxsw_linecard_provision_fail(mlxsw_core, linecard);
			return PTR_ERR(type);
		}
	}
	err = mlxsw_linecard_devices_attach(mlxsw_core, linecard);
	if (err)
		return err;
	err = mlxsw_linecard_provision_cbs_call(mlxsw_core, linecards,
						linecard);
	if (err)
		goto err_cbs_call;
	linecard->provisioned = true;
	devlink_linecard_provision_set(linecard->devlink_linecard, type);
	return 0;

err_cbs_call:
	mlxsw_linecard_devices_detach(linecard->linecards->mlxsw_core,
				      linecard);
	return err;
}

static void mlxsw_linecard_provision_clear(struct mlxsw_core *mlxsw_core,
					   struct mlxsw_linecards *linecards,
					   struct mlxsw_linecard *linecard)
{
	linecard->provisioned = false;
	mlxsw_linecard_unprovision_cbs_call(mlxsw_core, linecards,
					    linecard);
	mlxsw_linecard_devices_detach(linecard->linecards->mlxsw_core,
				      linecard);
	devlink_linecard_provision_clear(linecard->devlink_linecard);
}

static int mlxsw_linecard_ready_set(struct mlxsw_core *mlxsw_core,
				    struct mlxsw_linecard *linecard)
{
	char mddc_pl[MLXSW_REG_MDDC_LEN];
	int err;

	mlxsw_reg_mddc_pack(mddc_pl, linecard->slot_index, false, true);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddc), mddc_pl);
	if (err)
		return err;
	linecard->ready = true;
	return 0;
}

static int mlxsw_linecard_ready_clear(struct mlxsw_core *mlxsw_core,
				      struct mlxsw_linecard *linecard)
{
	char mddc_pl[MLXSW_REG_MDDC_LEN];
	int err;

	mlxsw_reg_mddc_pack(mddc_pl, linecard->slot_index, false, false);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddc), mddc_pl);
	if (err)
		return err;
	linecard->ready = false;
	return 0;
}

static int mlxsw_linecard_active_set(struct mlxsw_core *mlxsw_core,
				     struct mlxsw_linecards *linecards,
				     struct mlxsw_linecard *linecard,
				     u16 hw_revision, u16 ini_version)
{
	struct mlxsw_linecards_event_ops_item *item;
	int err;

	err = mlxsw_linecard_devices_update(mlxsw_core, linecard);
	if (err)
		return err;

	linecard->active = true;
	linecard->hw_revision = hw_revision;
	linecard->ini_version = ini_version;
	list_for_each_entry(item, &linecards->event_ops_list, list) {
		if (!item->event_ops->got_active)
			continue;
		item->event_ops->got_active(mlxsw_core, linecard->slot_index,
					    linecard, item->priv);
	}
	devlink_linecard_activate(linecard->devlink_linecard);
	return 0;
}

static void mlxsw_linecard_active_clear(struct mlxsw_core *mlxsw_core,
					struct mlxsw_linecards *linecards,
					struct mlxsw_linecard *linecard)
{
	struct mlxsw_linecards_event_ops_item *item;

	linecard->active = false;
	list_for_each_entry(item, &linecards->event_ops_list, list) {
		if (!item->event_ops->got_inactive)
			continue;
		item->event_ops->got_inactive(mlxsw_core, linecard->slot_index,
					      linecard, item->priv);
	}
	devlink_linecard_deactivate(linecard->devlink_linecard);
}

static int mlxsw_linecard_status_process(struct mlxsw_core *mlxsw_core,
					 struct mlxsw_linecards *linecards,
					 struct mlxsw_linecard *linecard,
					 const char *mddq_pl,
					 bool process_provision_only)
{
	enum mlxsw_reg_mddq_slot_info_ready ready;
	bool provisioned, sr_valid, active;
	u16 ini_version, hw_revision;
	u8 slot_index, card_type;
	int err = 0;

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
		err = mlxsw_linecard_provision_set(mlxsw_core, linecards,
						   linecard, card_type);
		if (err)
			goto out;
	}

	if (!process_provision_only && !linecard->unprovision_done &&
	    ready == MLXSW_REG_MDDQ_SLOT_INFO_READY_READY && !linecard->ready) {
		err = mlxsw_linecard_ready_set(mlxsw_core, linecard);
		if (err)
			goto out;
	}

	if (!process_provision_only && !linecard->unprovision_done && active &&
	    linecard->active != active && linecard->ready) {
		err = mlxsw_linecard_active_set(mlxsw_core, linecards, linecard,
						hw_revision, ini_version);
		if (err)
			goto out;
	}

	if (!process_provision_only && !linecard->unprovision_done && !active &&
	    linecard->active != active)
		mlxsw_linecard_active_clear(mlxsw_core, linecards, linecard);

	if (!process_provision_only &&
	    ready != MLXSW_REG_MDDQ_SLOT_INFO_READY_READY &&
	    linecard->ready) {
		err = mlxsw_linecard_ready_clear(mlxsw_core, linecard);
		if (err)
			goto out;
	}

	if (!provisioned && linecard->provisioned != provisioned)
		mlxsw_linecard_provision_clear(mlxsw_core, linecards, linecard);

out:
	mutex_unlock(&linecard->lock);
	return err;
}

static int mlxsw_linecard_status_get_and_process(struct mlxsw_core *mlxsw_core,
						 struct mlxsw_linecards *linecards,
						 struct mlxsw_linecard *linecard,
						 bool process_provision_only)
{
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	int err;

	mlxsw_reg_mddq_slot_info_pack(mddq_pl, linecard->slot_index, false);
	err = mlxsw_reg_query(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
	if (err)
		return err;

	return mlxsw_linecard_status_process(mlxsw_core, linecards, linecard,
					     mddq_pl, process_provision_only);
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
mlxsw_linecard_ini_transfer(struct mlxsw_core *mlxsw_core,
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

static int mlxsw_linecard_bct_process(struct mlxsw_core *mlxsw_core,
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
	mlxsw_linecard_provision_fail(mlxsw_core, linecard);
	return err;
}

static int
mlxsw_linecard_ini_erase(struct mlxsw_core *mlxsw_core,
			 struct mlxsw_linecard *linecard,
			 struct netlink_ext_ack *extack)
{
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	enum mlxsw_reg_mbct_status status;
	int err;

	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_ERASE_INI_IMAGE, 0,
			    false, false, NULL);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct),
			      linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to erase linecard INI");
		return err;
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
	return 0;

fix_fsm_err_out:
	mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
	return -EINVAL;
}

static int
mlxsw_linecard_ini_activate(struct mlxsw_core *mlxsw_core,
			    struct mlxsw_linecard *linecard,
			    struct netlink_ext_ack *extack)
{
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	enum mlxsw_reg_mbct_status status;
	int err;

	mlxsw_reg_mbct_pack(linecard->mbct_pl, linecard->slot_index,
			    MLXSW_REG_MBCT_OP_ACTIVATE, 0,
			    false, true, NULL);
	err = mlxsw_reg_write(mlxsw_core, MLXSW_REG(mbct), linecard->mbct_pl);
	if (err) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to activate linecard INI");
		return err;
	}
	mlxsw_reg_mbct_unpack(linecard->mbct_pl, NULL, &status, &fsm_state);
	if (status == MLXSW_REG_MBCT_STATUS_ACTIVATION_FAILED) {
		NL_SET_ERR_MSG_MOD(extack, "Failed to activate linecard INI");
		goto fix_fsm_err_out;
	}

	return 0;

fix_fsm_err_out:
	mlxsw_linecard_fix_fsm_state(linecard, fsm_state);
	return -EINVAL;
}

#define MLXSW_LINECARD_INI_WAIT_RETRIES 10
#define MLXSW_LINECARD_INI_WAIT_MS 500

static int
mlxsw_linecard_ini_in_use_wait(struct mlxsw_core *mlxsw_core,
			       struct mlxsw_linecard *linecard,
			       struct netlink_ext_ack *extack)
{
	enum mlxsw_reg_mbct_fsm_state fsm_state;
	enum mlxsw_reg_mbct_status status;
	unsigned int ini_wait_retries = 0;
	int err;

query_ini_status:
	err = mlxsw_linecard_query_status(linecard, &status,
					  &fsm_state, extack);
	if (err)
		return err;

	switch (fsm_state) {
	case MLXSW_REG_MBCT_FSM_STATE_INI_IN_USE:
		if (ini_wait_retries++ > MLXSW_LINECARD_INI_WAIT_RETRIES) {
			NL_SET_ERR_MSG_MOD(extack, "Failed to wait for linecard INI to be not used");
			return -EINVAL;
		}
		mdelay(MLXSW_LINECARD_INI_WAIT_MS);
		goto query_ini_status;
	default:
		break;
	}
	return 0;
}

static bool mlxsw_linecard_port_selector(void *priv, u16 local_port)
{
	struct mlxsw_linecard *linecard = priv;
	struct mlxsw_core *mlxsw_core;

	mlxsw_core = linecard->linecards->mlxsw_core;
	return linecard == mlxsw_core_port_linecard_get(mlxsw_core, local_port);
}

static int mlxsw_linecard_provision(struct devlink_linecard *devlink_linecard,
				    void *priv, const char *type,
				    const void *type_priv,
				    struct netlink_ext_ack *extack)
{
	const struct mlxsw_linecard_ini_file *ini_file = type_priv;
	struct mlxsw_linecard *linecard = priv;
	struct mlxsw_core *mlxsw_core;
	int err;

	mutex_lock(&linecard->lock);

	linecard->unprovision_done = false;
	mlxsw_core = linecard->linecards->mlxsw_core;

	err = mlxsw_linecard_ini_erase(mlxsw_core, linecard, extack);
	if (err)
		goto err_out;

	err = mlxsw_linecard_ini_transfer(mlxsw_core, linecard,
					  ini_file, extack);
	if (err)
		goto err_out;

	err = mlxsw_linecard_ini_activate(mlxsw_core, linecard, extack);
	if (err)
		goto err_out;

	goto out;

err_out:
	mlxsw_linecard_provision_fail(mlxsw_core, linecard);
out:
	mutex_unlock(&linecard->lock);
	return err;
}

static int mlxsw_linecard_unprovision(struct devlink_linecard *devlink_linecard,
				      void *priv,
				      struct netlink_ext_ack *extack)
{
	struct mlxsw_linecard *linecard = priv;
	struct mlxsw_core *mlxsw_core;
	int err;

	mutex_lock(&linecard->lock);

	mlxsw_core = linecard->linecards->mlxsw_core;

	mlxsw_core_ports_remove_selected(mlxsw_core,
					 mlxsw_linecard_port_selector,
					 linecard);

	err = mlxsw_linecard_ini_in_use_wait(mlxsw_core, linecard, extack);
	if (err)
		goto err_out;

	err = mlxsw_linecard_ini_erase(mlxsw_core, linecard, extack);
	if (err)
		goto err_out;

	linecard->unprovision_done = true;
	goto out;

err_out:
	mlxsw_linecard_provision_fail(mlxsw_core, linecard);
out:
	mutex_unlock(&linecard->lock);
	return err;
}

struct mlxsw_linecard_status_event {
	struct mlxsw_core *mlxsw_core;
	char mddq_pl[MLXSW_REG_MDDQ_LEN];
	struct work_struct work;
};

static void mlxsw_linecard_status_event_work(struct work_struct *work)
{
	struct mlxsw_linecard_status_event *event;
	struct mlxsw_linecards *linecards;
	struct mlxsw_core *mlxsw_core;

	event = container_of(work, struct mlxsw_linecard_status_event, work);
	mlxsw_core = event->mlxsw_core;
	linecards = mlxsw_core_linecards(mlxsw_core);
	mlxsw_linecard_status_process(mlxsw_core, linecards, NULL,
				      event->mddq_pl, false);
	kfree(event);
}

static void
mlxsw_linecard_status_listener_func(const struct mlxsw_reg_info *reg,
				       char *mddq_pl, void *priv)
{
	struct mlxsw_linecard_status_event *event;
	struct mlxsw_core *mlxsw_core = priv;

	event = kmalloc(sizeof(*event), GFP_ATOMIC);
	if (!event)
		return;
	event->mlxsw_core = mlxsw_core;
	memcpy(event->mddq_pl, mddq_pl, sizeof(event->mddq_pl));
	INIT_WORK(&event->work, mlxsw_linecard_status_event_work);
	mlxsw_core_schedule_work(&event->work);
}

struct mlxsw_linecard_bct_event {
	struct mlxsw_core *mlxsw_core;
	char mbct_pl[MLXSW_REG_MBCT_LEN];
	struct work_struct work;
};

static void mlxsw_linecard_bct_event_work(struct work_struct *work)
{
	struct mlxsw_linecard_bct_event *event;
	struct mlxsw_core *mlxsw_core;

	event = container_of(work, struct mlxsw_linecard_bct_event, work);
	mlxsw_core = event->mlxsw_core;
	mlxsw_linecard_bct_process(mlxsw_core, event->mbct_pl);
	kfree(event);
}

static void
mlxsw_linecard_bct_listener_func(const struct mlxsw_reg_info *reg,
				    char *mbct_pl, void *priv)
{
	struct mlxsw_linecard_bct_event *event;
	struct mlxsw_core *mlxsw_core = priv;

	event = kmalloc(sizeof(*event), GFP_ATOMIC);
	if (!event)
		return;
	event->mlxsw_core = mlxsw_core;
	memcpy(event->mbct_pl, mbct_pl, sizeof(event->mbct_pl));
	INIT_WORK(&event->work, mlxsw_linecard_bct_event_work);
	mlxsw_core_schedule_work(&event->work);
}

static const struct mlxsw_listener mlxsw_linecard_listener[] = {
	MLXSW_EVENTL(mlxsw_linecard_status_listener_func, DSDSC, DSDSC),
	MLXSW_EVENTL(mlxsw_linecard_bct_listener_func, BCTOE, BCTOE),
};

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

static int
mlxsw_linecard_info_get(struct devlink_linecard *devlink_linecard, void *priv,
			struct devlink_info_req *req,
			struct netlink_ext_ack *extack)
{
	struct mlxsw_linecard *linecard = priv;
	char buf[32];
	int err;

	if (!linecard->active)
		return 0;

	sprintf(buf, "%d", linecard->hw_revision);
	err = devlink_info_version_fixed_put(req, "hw.revision", buf);
	if (err)
		return err;

	sprintf(buf, "%d", linecard->ini_version);
	return devlink_info_version_running_put(req, "ini.version", buf);
}

static const struct devlink_linecard_ops mlxsw_linecard_ops = {
	.provision = mlxsw_linecard_provision,
	.unprovision = mlxsw_linecard_unprovision,
	.types_count = mlxsw_linecard_types_count,
	.types_get = mlxsw_linecard_types_get,
	.info_get = mlxsw_linecard_info_get,
	.device_info_get = mlxsw_linecard_device_info_get,
};

static int mlxsw_linecard_event_delivery_set(struct mlxsw_core *mlxsw_core,
					     struct mlxsw_linecard *linecard,
					     bool enable)
{
	char mddq_pl[MLXSW_REG_MDDQ_LEN];

	mlxsw_reg_mddq_slot_info_pack(mddq_pl, linecard->slot_index, enable);
	return mlxsw_reg_write(mlxsw_core, MLXSW_REG(mddq), mddq_pl);
}

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
	INIT_LIST_HEAD(&linecard->device_list);

	devlink_linecard = devlink_linecard_create(priv_to_devlink(mlxsw_core),
						   slot_index, &mlxsw_linecard_ops,
						   linecard);
	if (IS_ERR(devlink_linecard)) {
		err = PTR_ERR(devlink_linecard);
		goto err_devlink_linecard_create;
	}
	linecard->devlink_linecard = devlink_linecard;

	err = mlxsw_linecard_status_get_and_process(mlxsw_core, linecards,
						    linecard, true);
	if (err)
		goto err_status_get_and_process;

	err = mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, true);
	if (err)
		goto err_event_delivery_set;

	err = mlxsw_linecard_status_get_and_process(mlxsw_core, linecards,
						    linecard, false);
	if (err)
		goto err_status_get_and_process_2;

	return 0;

err_status_get_and_process_2:
	mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, false);
err_event_delivery_set:
err_status_get_and_process:
	devlink_linecard_destroy(linecard->devlink_linecard);
err_devlink_linecard_create:
	mutex_destroy(&linecard->lock);
	return err;
}

static void mlxsw_linecard_fini(struct mlxsw_core *mlxsw_core,
				struct mlxsw_linecards *linecards,
				u8 slot_index)
{
	struct mlxsw_linecard *linecard;

	linecard = mlxsw_linecard_get(linecards, slot_index);
	mlxsw_linecard_event_delivery_set(mlxsw_core, linecard, false);
	/* Make sure all scheduled events are processed */
	mlxsw_core_flush_owq();
	if (linecard->active)
		mlxsw_linecard_active_clear(mlxsw_core, linecards, linecard);
	mlxsw_linecard_devices_detach(mlxsw_core, linecard);
	devlink_linecard_destroy(linecard->devlink_linecard);
	mutex_destroy(&linecard->lock);
}

/*       LINECARDS INI BUNDLE FILE
 *  +----------------------------------+
 *  |        MAGIC ("NVLCINI+")        |
 *  +----------------------------------+     +------------------+
 *  |  INI 0                           +---> | __le16 size      |
 *  +----------------------------------+     | u8 __dontcare[7] |
 *  |  INI 1                           |     | u8 type;         |
 *  +----------------------------------+     | u8 name[20]      |
 *  |  ...                             |     | ...              |
 *  +----------------------------------+     +------------------+
 *  |  INI N                           |
 *  +----------------------------------+
 */

#define MLXSW_LINECARDS_INI_BUNDLE_MAGIC "NVLCINI+"

static int
mlxsw_linecard_types_file_validate(struct mlxsw_linecards *linecards,
				   struct mlxsw_linecard_types_info *types_info)
{
	size_t magic_size = strlen(MLXSW_LINECARDS_INI_BUNDLE_MAGIC);
	struct mlxsw_linecard_ini_file *ini_file;
	size_t size = types_info->data_size;
	const u8 *data = types_info->data;
	unsigned int count = 0;
	u16 ini_file_size;

	if (size < magic_size) {
		dev_warn(linecards->bus_info->dev, "Invalid linecards INIs file size, smaller than magic size\n");
		return -EINVAL;
	}
	if (memcmp(data, MLXSW_LINECARDS_INI_BUNDLE_MAGIC, magic_size)) {
		dev_warn(linecards->bus_info->dev, "Invalid linecards INIs file magic pattern\n");
		return -EINVAL;
	}

	data += magic_size;
	size -= magic_size;

	while (size > 0) {
		if (size < sizeof(*ini_file)) {
			dev_warn(linecards->bus_info->dev, "Linecards INIs file contains INI which is smaller than bare minimal\n");
			return -EINVAL;
		}
		ini_file = (struct mlxsw_linecard_ini_file *) data;
		ini_file_size = le16_to_cpu(ini_file->size);
		if (ini_file_size + sizeof(__le16) > size) {
			dev_warn(linecards->bus_info->dev, "Linecards INIs file appears to be truncated\n");
			return -EINVAL;
		}
		if (ini_file_size % 4) {
			dev_warn(linecards->bus_info->dev, "Linecards INIs file contains INI with invalid size\n");
			return -EINVAL;
		}
		data += ini_file_size + sizeof(__le16);
		size -= ini_file_size + sizeof(__le16);
		count++;
	}
	if (!count) {
		dev_warn(linecards->bus_info->dev, "Linecards INIs file does not contain any INI\n");
		return -EINVAL;
	}
	types_info->count = count;
	return 0;
}

static void
mlxsw_linecard_types_file_parse(struct mlxsw_linecard_types_info *types_info)
{
	size_t magic_size = strlen(MLXSW_LINECARDS_INI_BUNDLE_MAGIC);
	size_t size = types_info->data_size - magic_size;
	const u8 *data = types_info->data + magic_size;
	struct mlxsw_linecard_ini_file *ini_file;
	unsigned int count = 0;
	u16 ini_file_size;
	int i;

	while (size) {
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
}

static int mlxsw_linecard_types_init(struct mlxsw_core *mlxsw_core,
				     struct mlxsw_linecards *linecards)
{
	struct mlxsw_linecard_types_info *types_info;
	const char *lc_ini_bundle_filename;
	const struct firmware *firmware;
	int err;

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

	types_info = kzalloc(sizeof(*types_info), GFP_KERNEL);
	if (!types_info)
		return -ENOMEM;
	linecards->types_info = types_info;

	types_info->data_size = firmware->size;
	types_info->data = vmalloc(types_info->data_size);
	if (!types_info->data) {
		err = -ENOMEM;
		goto err_data_alloc;
	}
	memcpy(types_info->data, firmware->data, types_info->data_size);
	release_firmware(firmware);

	err = mlxsw_linecard_types_file_validate(linecards, types_info);
	if (err) {
		err = 0;
		goto err_type_file_file_validate;
	}

	types_info->ini_files = kmalloc_array(types_info->count,
					      sizeof(struct mlxsw_linecard_ini_file),
					      GFP_KERNEL);
	if (!types_info->ini_files) {
		err = -ENOMEM;
		goto err_ini_files_alloc;
	}

	mlxsw_linecard_types_file_parse(types_info);

	return 0;

err_ini_files_alloc:
err_type_file_file_validate:
	vfree(types_info->data);
err_data_alloc:
	kfree(types_info);
	return err;
}

static void mlxsw_linecard_types_fini(struct mlxsw_linecards *linecards)
{
	struct mlxsw_linecard_types_info *types_info = linecards->types_info;

	kfree(types_info->ini_files);
	vfree(types_info->data);
	kfree(types_info);
}

int mlxsw_linecards_init(struct mlxsw_core *mlxsw_core,
			 const struct mlxsw_bus_info *bus_info)
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
	if (!slot_count)
		return 0;

	linecards = vzalloc(struct_size(linecards, linecards, slot_count));
	if (!linecards)
		return -ENOMEM;
	linecards->count = slot_count;
	linecards->mlxsw_core = mlxsw_core;
	linecards->bus_info = bus_info;
	INIT_LIST_HEAD(&linecards->event_ops_list);

	err = mlxsw_linecard_types_init(mlxsw_core, linecards);
	if (err)
		goto err_types_init;

	err = mlxsw_core_traps_register(mlxsw_core, mlxsw_linecard_listener,
					ARRAY_SIZE(mlxsw_linecard_listener),
					mlxsw_core);
	if (err)
		goto err_traps_register;

	mlxsw_core_linecards_set(mlxsw_core, linecards);

	for (i = 0; i < linecards->count; i++) {
		err = mlxsw_linecard_init(mlxsw_core, linecards, i + 1);
		if (err)
			goto err_linecard_init;
	}

	return 0;

err_linecard_init:
	for (i--; i >= 0; i--)
		mlxsw_linecard_fini(mlxsw_core, linecards, i + 1);
	mlxsw_core_traps_unregister(mlxsw_core, mlxsw_linecard_listener,
				    ARRAY_SIZE(mlxsw_linecard_listener),
				    mlxsw_core);
err_traps_register:
	mlxsw_linecard_types_fini(linecards);
err_types_init:
	vfree(linecards);

	return err;
}

void mlxsw_linecards_fini(struct mlxsw_core *mlxsw_core)
{
	struct mlxsw_linecards *linecards = mlxsw_core_linecards(mlxsw_core);
	int i;

	if (!linecards)
		return;
	WARN_ON(!list_empty(&linecards->event_ops_list));
	for (i = 0; i < linecards->count; i++)
		mlxsw_linecard_fini(mlxsw_core, linecards, i + 1);
	mlxsw_core_traps_unregister(mlxsw_core, mlxsw_linecard_listener,
				    ARRAY_SIZE(mlxsw_linecard_listener),
				    mlxsw_core);
	mlxsw_linecard_types_fini(linecards);
	vfree(linecards);
}

int mlxsw_linecards_event_ops_register(struct mlxsw_core *mlxsw_core,
				       struct mlxsw_linecards_event_ops *ops,
				       void *priv)
{
	struct mlxsw_linecards *linecards = mlxsw_core_linecards(mlxsw_core);
	struct mlxsw_linecards_event_ops_item *item;

	item = kzalloc(sizeof(*item), GFP_KERNEL);
	if (!item)
		return -ENOMEM;
	item->event_ops = ops;
	item->priv = priv;
	list_add_tail(&item->list, &linecards->event_ops_list);
	return 0;
}
EXPORT_SYMBOL(mlxsw_linecards_event_ops_register);

void mlxsw_linecards_event_ops_unregister(struct mlxsw_core *mlxsw_core,
					  struct mlxsw_linecards_event_ops *ops,
					  void *priv)
{
	struct mlxsw_linecards *linecards = mlxsw_core_linecards(mlxsw_core);
	struct mlxsw_linecards_event_ops_item *item, *tmp;

	list_for_each_entry_safe(item, tmp, &linecards->event_ops_list, list) {
		if (item->event_ops == ops && item->priv == priv) {
			list_del(&item->list);
			kfree(item);
		}
	}
}
EXPORT_SYMBOL(mlxsw_linecards_event_ops_unregister);
