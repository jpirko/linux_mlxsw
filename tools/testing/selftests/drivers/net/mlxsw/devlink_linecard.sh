#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# In addition to the common variables, user might use:
# LC_SLOT - If not set, all probed line cards are going to be tested,
#	    with an exception of the "activation_16x100G_test".
#	    It set, only the selected line card is going to be used
#	    for tests, including "activation_16x100G_test".

lib_dir=$(dirname $0)/../../../net/forwarding

ALL_TESTS="
	unprovision_test
	provision_test
	activation_16x100G_test
"

NUM_NETIFS=0

source $lib_dir/lib.sh
source $lib_dir/devlink_lib.sh

until_lc_state_is()
{
	local state=$1; shift
	local current=$("$@")

	echo "$current"
	[ "$current" == "$state" ]
}

until_lc_state_is_not()
{
	! until_lc_state_is "$@"
}

lc_state_get()
{
	local lc=$1

	devlink lc show $DEVLINK_DEV lc $lc -j | jq -e -r ".[][][].state"
}

lc_wait_until_state_changes()
{
	local lc=$1
	local state=$2
	local timeout=$3 # ms

	busywait "$timeout" until_lc_state_is_not "$state" lc_state_get "$lc"
}

lc_wait_until_state_becomes()
{
	local lc=$1
	local state=$2
	local timeout=$3 # ms

	busywait "$timeout" until_lc_state_is "$state" lc_state_get "$lc"
}

until_lc_port_count_is()
{
	local port_count=$1; shift
	local current=$("$@")

	echo "$current"
	[ $current == $port_count ]
}

lc_port_count_get()
{
	local lc=$1

	devlink port -j | jq -e -r ".[][] | select(.lc==$lc) | .port" | wc -l
}

lc_wait_until_port_count_is()
{
	local lc=$1
	local port_count=$2
	local timeout=$3 # ms

	busywait "$timeout" until_lc_port_count_is "$port_count" lc_port_count_get "$lc"
}

PROV_UNPROV_TIMEOUT=8000 # ms
POST_PROV_ACT_TIMEOUT=2000 # ms
PROV_PORTS_INSTANTIATION_TIMEOUT=15000 # ms

unprovision_one()
{
	local lc=$1
	local state

	state=$(lc_state_get $lc)
	check_err $? "Failed to get state of linecard $lc"
	if [[ "$state" == "unprovisioned" ]]; then
		return
	fi

	log_info "Unprovisioning linecard $lc"

	devlink lc set $DEVLINK_DEV lc $lc notype
	check_err $? "Failed to trigger linecard $lc unprovisioning"

	state=$(lc_wait_until_state_changes $lc "unprovisioning" \
		$PROV_UNPROV_TIMEOUT)
	check_err $? "Failed to unprovision linecard $lc (timeout)"

	[ "$state" == "unprovisioned" ]
	check_err $? "Failed to unprovision linecard $lc (state=$state)"
}

provision_one()
{
	local lc=$1
	local type=$2
	local state

	log_info "Provisioning linecard $lc"

	devlink lc set $DEVLINK_DEV lc $lc type $type
	check_err $? "Failed trigger linecard $lc provisioning"

	state=$(lc_wait_until_state_changes $lc "provisioning" \
		$PROV_UNPROV_TIMEOUT)
	check_err $? "Failed to provision linecard $lc (timeout)"

	[ "$state" == "provisioned" ] || [ "$state" == "active" ]
	check_err $? "Failed to provision linecard $lc (state=$state)"

	provisioned_type=$(devlink lc show $DEVLINK_DEV lc $lc -j | jq -e -r ".[][][].type")
	[ "$provisioned_type" == "$type" ]
	check_err $? "Wrong provision type returned for linecard $lc (got \"$provisioned_type\", expected \"$type\")"

	# Wait for possible activation to make sure the state
	# won't change after return from this function.
	state=$(lc_wait_until_state_becomes $lc "active" \
		$POST_PROV_ACT_TIMEOUT)
}

unprovision_test()
{
	RET=0
	local lc

	lc=$LC_SLOT
	unprovision_one $lc
	log_test "Unprovision"
}

LC_16X100G_TYPE="16x100G"
LC_16X100G_PORT_COUNT=16
LC_16X100G_DEVICE_COUNT=4

supported_types_check()
{
	local lc=$1
	local supported_types_count
	local type_index
	local lc_16x100_found=false

	supported_types_count=$(devlink lc show $DEVLINK_DEV lc $lc -j | \
				jq -e -r ".[][][].supported_types | length")
	[ $supported_types_count != 0 ]
	check_err $? "No supported types found for linecard $lc"
	for (( type_index=0; type_index<$supported_types_count; type_index++ ))
	do
		type=$(devlink lc show $DEVLINK_DEV lc $lc -j | \
		       jq -e -r ".[][][].supported_types[$type_index]")
		if [[ "$type" == "$LC_16X100G_TYPE" ]]; then
			lc_16x100_found=true
			break
		fi
	done
	[ $lc_16x100_found = true ]
	check_err $? "16X100G not found between supported types of linecard $lc"
}

lc_info_check()
{
	local lc=$1
	local fixed_hw_revision
	local running_ini_version

	fixed_hw_revision=$(devlink lc -v info $DEVLINK_DEV lc $lc -j | \
			    jq -e -r '.[][][].versions.fixed."hw.revision"')
	check_err $? "Failed to get linecard $lc fixed.hw.revision"
	log_info "Linecard $lc fixed.hw.revision: \"$fixed_hw_revision\""
	running_ini_version=$(devlink lc -v info $DEVLINK_DEV lc $lc -j | \
			      jq -e -r '.[][][].versions.running."ini.version"')
	check_err $? "Failed to get linecard $lc running.ini.version"
	log_info "Linecard $lc running.ini.version: \"$running_ini_version\""
}

lc_devices_check()
{
	local lc=$1
	local expected_device_count=$2
	local device_count
	local device
	local flashable
	local component
	local componentstr

	device_count=$(devlink lc show $DEVLINK_DEV lc $lc -j | \
		       jq -e -r ".[][][].devices |length")
	check_err $? "Failed to get linecard $lc device count"
	[ $device_count != 0 ]
	check_err $? "No device found on linecard $lc"
	[ $device_count == $expected_device_count ]
	check_err $? "Unexpected device count on linecard $lc (got $expected_device_count, expected $device_count)"
	for (( device=0; device<device_count; device++ ))
	do
		flashable=$(devlink lc -v show $DEVLINK_DEV lc $lc -j -p | \
			    jq -r ".[][][].devices[$device].flashable")
		[[ "$flashable" == "null" ]]
		check_fail $? "Failed to get linecard $lc device $device flashable flag"
		if [[ "$flashable" == "true" ]]; then
			component=$(devlink lc -v show $DEVLINK_DEV lc $lc -j -p | \
				    jq -e -r ".[][][].devices[$device].component")
			check_err $? "Failed to get linecard $lc device $device component name"
			componentstr=", component \"$component\""
		elif [[ "$flashable" == "false" ]]; then
			componentstr=""
			[ $device == 0 ]
			check_fail $? "Device 0 on linecard $lc is not flashable"
		fi
		log_info "Linecard $lc device $device flashable: \"$flashable\"$componentstr"
	done
}

ports_check()
{
	local lc=$1
	local expected_port_count=$2
	local port_count

	port_count=$(lc_wait_until_port_count_is $lc $expected_port_count \
		$PROV_PORTS_INSTANTIATION_TIMEOUT)
	[ $port_count != 0 ]
	check_err $? "No port associated with linecard $lc"
	[ $port_count == $expected_port_count ]
	check_err $? "Unexpected port count linecard $lc (got $port_count, expected $expected_port_count)"
}

provision_test()
{
	RET=0
	local lc
	local type
	local state

	lc=$LC_SLOT
	supported_types_check $lc
	state=$(lc_state_get $lc)
	check_err $? "Failed to get state of linecard $lc"
	if [[ "$state" != "unprovisioned" ]]; then
		unprovision_one $lc
	fi
	provision_one $lc $LC_16X100G_TYPE
	lc_devices_check $lc $LC_16X100G_DEVICE_COUNT
	lc_info_check $lc
	ports_check $lc $LC_16X100G_PORT_COUNT
	log_test "Provision"
}

ACTIVATION_TIMEOUT=20000 # ms

interface_check()
{
	ip link set $h1 up
	ip link set $h2 up
	ifaces_upped=true
	setup_wait
}

lc_devices_info_check()
{
	local lc=$1
	local expected_device_count=$2
	local device_count
	local device
	local running_device_fw

	device_count=$(devlink lc info $DEVLINK_DEV lc $lc -j | \
		       jq -e -r ".[][][].devices |length")
	check_err $? "Failed to get linecard $lc device count"
	for (( device=0; device<device_count; device++ ))
	do
		running_device_fw=$(devlink lc -v info $DEVLINK_DEV lc $lc -j | \
				    jq -e -r ".[][][].devices[$device].versions.running.fw")
		check_err $? "Failed to get linecard $lc device $device running fw version"
		log_info "Linecard $lc device $device running.fw: \"$running_device_fw\""
		fixed_device_fw_psid=$(devlink lc -v info $DEVLINK_DEV lc $lc -j | \
				       jq -e -r ".[][][].devices[$device].versions.fixed" | \
				       jq -e -r '."fw.psid"')
		check_err $? "Failed to get linecard $lc device $device fixed fw PSID"
		log_info "Linecard $lc device $device fixed.fw.psid: \"$fixed_device_fw_psid\""
	done
}

activation_16x100G_test()
{
	RET=0
	local lc
	local type
	local state

	lc=$LC_SLOT
	type=$LC_16X100G_TYPE

	unprovision_one $lc
	provision_one $lc $type
	state=$(lc_wait_until_state_becomes $lc "active" \
		$ACTIVATION_TIMEOUT)
	check_err $? "Failed to get linecard $lc activated (timeout)"

	lc_devices_info_check $lc $LC_16X100G_DEVICE_COUNT

	interface_check

	log_test "Activation 16x100G"
}

setup_prepare()
{
	local lc_num=$(devlink lc show -j | jq -e -r ".[][\"$DEVLINK_DEV\"] |length")
	if [[ $? -ne 0 ]] || [[ $lc_num -eq 0 ]]; then
		echo "SKIP: No linecard support found"
		exit $ksft_skip
	fi

	if [ -z "$LC_SLOT" ]; then
		echo "SKIP: \"LC_SLOT\" variable not provided"
		exit $ksft_skip
	fi

	# Interfaces are not present during the script start,
	# that's why we define NUM_NETIFS here so dummy
	# implicit veth pairs are not created.
	NUM_NETIFS=2
	h1=${NETIFS[p1]}
	h2=${NETIFS[p2]}
	ifaces_upped=false
}

cleanup()
{
	if [ "$ifaces_upped" = true ] ; then
		ip link set $h1 down
		ip link set $h2 down
	fi
}

trap cleanup EXIT

setup_prepare

tests_run

exit $EXIT_STATUS
