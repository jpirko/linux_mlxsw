#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="
	fw_crdump_test
"

NUM_NETIFS=0
source ../../../net/forwarding/lib.sh
source ../../../net/forwarding/devlink_lib.sh
source mlxsw_lib.sh

SP1_DEV_ID=cb84
FW_CR_SP1_DEV_ID_INDEX_L=237538
FW_CR_SP1_DEV_ID_INDEX_H=237539

fw_crdump_test()
{
	local dev_id_l
	local dev_id_h

	devlink health dump show $DEVLINK_DEV \
		reporter fw_crdump > /tmp/crdump

	dev_id_l=$(cat /tmp/crdump | jq -r ".[][$FW_CR_SP1_DEV_ID_INDEX_L]")
	dev_id_h=$(cat /tmp/crdump | jq -r ".[][$FW_CR_SP1_DEV_ID_INDEX_H]")

	RET=0
	check_err $? "Expected device id to be ($SP1_DEV_ID), but got ($dev_id_h$dev_id_l)"
	log_test "firmware crdump"
}

cleanup()
{
	devlink health dump clear $DEVLINK_DEV reporter fw_crdump
}

trap cleanup EXIT

tests_run

exit $EXIT_STATUS
