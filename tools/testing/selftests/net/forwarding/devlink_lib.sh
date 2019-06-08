#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

##############################################################################
# Defines

DEVLINK_DEV=$(devlink port show "${NETIFS[p1]}" -j \
		     | jq -r '.port | keys[]' | cut -d/ -f-2)
if [ -z "$DEVLINK_DEV" ]; then
	echo "SKIP: ${NETIFS[p1]} has no devlink device registered for it"
	exit 1
fi
if [[ "$(echo $DEVLINK_DEV | grep -c pci)" -eq 0 ]]; then
	echo "SKIP: devlink device's bus is not PCI"
	exit 1
fi

DEVLINK_VIDDID=$(lspci -s $(echo $DEVLINK_DEV | cut -d"/" -f2) \
		 -n | cut -d" " -f3)

##############################################################################
# Sanity checks

devlink help 2>&1 | grep resource &> /dev/null
if [ $? -ne 0 ]; then
	echo "SKIP: iproute2 too old, missing devlink resource support"
	exit 1
fi

devlink help 2>&1 | grep "trap" &> /dev/null
if [ $? -ne 0 ]; then
	echo "SKIP: iproute2 too old, missing devlink-trap"
	exit $KSFT_SKIP
fi

##############################################################################
# Devlink helpers

devlink_resource_names_to_path()
{
	local resource
	local path=""

	for resource in "${@}"; do
		if [ "$path" == "" ]; then
			path="$resource"
		else
			path="${path}/$resource"
		fi
	done

	echo "$path"
}

devlink_resource_get()
{
	local name=$1
	local resource_name=.[][\"$DEVLINK_DEV\"]

	resource_name="$resource_name | .[] | select (.name == \"$name\")"

	shift
	for resource in "${@}"; do
		resource_name="${resource_name} | .[\"resources\"][] | \
			       select (.name == \"$resource\")"
	done

	devlink -j resource show "$DEVLINK_DEV" | jq "$resource_name"
}

devlink_resource_size_get()
{
	local size=$(devlink_resource_get "$@" | jq '.["size_new"]')

	if [ "$size" == "null" ]; then
		devlink_resource_get "$@" | jq '.["size"]'
	else
		echo "$size"
	fi
}

devlink_resource_size_set()
{
	local new_size=$1
	local path

	shift
	path=$(devlink_resource_names_to_path "$@")
	devlink resource set "$DEVLINK_DEV" path "$path" size "$new_size"
	check_err $? "Failed setting path $path to size $size"
}

devlink_reload()
{
	local still_pending

	devlink dev reload "$DEVLINK_DEV" &> /dev/null
	check_err $? "Failed reload"

	still_pending=$(devlink resource show "$DEVLINK_DEV" | \
			grep -c "size_new")
	check_err $still_pending "Failed reload - There are still unset sizes"
}

declare -A DEVLINK_ORIG

devlink_port_pool_threshold()
{
	local port=$1; shift
	local pool=$1; shift

	devlink sb port pool show $port pool $pool -j \
		| jq '.port_pool."'"$port"'"[].threshold'
}

devlink_port_pool_th_set()
{
	local port=$1; shift
	local pool=$1; shift
	local th=$1; shift
	local key="port_pool($port,$pool).threshold"

	DEVLINK_ORIG[$key]=$(devlink_port_pool_threshold $port $pool)
	devlink sb port pool set $port pool $pool th $th
}

devlink_port_pool_th_restore()
{
	local port=$1; shift
	local pool=$1; shift
	local key="port_pool($port,$pool).threshold"

	devlink sb port pool set $port pool $pool th ${DEVLINK_ORIG[$key]}
}

devlink_pool_size_thtype()
{
	local pool=$1; shift

	devlink sb pool show "$DEVLINK_DEV" pool $pool -j \
	    | jq -r '.pool[][] | (.size, .thtype)'
}

devlink_pool_size_thtype_set()
{
	local pool=$1; shift
	local thtype=$1; shift
	local size=$1; shift
	local key="pool($pool).size_thtype"

	DEVLINK_ORIG[$key]=$(devlink_pool_size_thtype $pool)
	devlink sb pool set "$DEVLINK_DEV" pool $pool size $size thtype $thtype
}

devlink_pool_size_thtype_restore()
{
	local pool=$1; shift
	local key="pool($pool).size_thtype"
	local -a orig=(${DEVLINK_ORIG[$key]})

	devlink sb pool set "$DEVLINK_DEV" pool $pool \
		size ${orig[0]} thtype ${orig[1]}
}

devlink_tc_bind_pool_th()
{
	local port=$1; shift
	local tc=$1; shift
	local dir=$1; shift

	devlink sb tc bind show $port tc $tc type $dir -j \
	    | jq -r '.tc_bind[][] | (.pool, .threshold)'
}

devlink_tc_bind_pool_th_set()
{
	local port=$1; shift
	local tc=$1; shift
	local dir=$1; shift
	local pool=$1; shift
	local th=$1; shift
	local key="tc_bind($port,$dir,$tc).pool_th"

	DEVLINK_ORIG[$key]=$(devlink_tc_bind_pool_th $port $tc $dir)
	devlink sb tc bind set $port tc $tc type $dir pool $pool th $th
}

devlink_tc_bind_pool_th_restore()
{
	local port=$1; shift
	local tc=$1; shift
	local dir=$1; shift
	local key="tc_bind($port,$dir,$tc).pool_th"
	local -a orig=(${DEVLINK_ORIG[$key]})

	devlink sb tc bind set $port tc $tc type $dir \
		pool ${orig[0]} th ${orig[1]}
}

devlink_trap_report_set()
{
	local trap_name=$1; shift
	local report=$1; shift

	# Pipe output to /dev/null to avoid expected warnings.
	devlink trap set $DEVLINK_DEV trap $trap_name report $report \
		&> /dev/null
}

devlink_trap_action_set()
{
	local trap_name=$1; shift
	local action=$1; shift

	# Pipe output to /dev/null to avoid expected warnings.
	devlink trap set $DEVLINK_DEV trap $trap_name \
		action $action &> /dev/null
}

devlink_trap_rx_packets_get()
{
	local trap_name=$1; shift

	devlink -js trap show $DEVLINK_DEV trap $trap_name \
		| jq '.[][][]["stats"]["rx"]["packets"]'
}

devlink_trap_rx_bytes_get()
{
	local trap_name=$1; shift

	devlink -js trap show $DEVLINK_DEV trap $trap_name \
		| jq '.[][][]["stats"]["rx"]["bytes"]'
}

devlink_trap_stats_idle_test()
{
	local trap_name=$1; shift
	local t0_packets t0_bytes
	local t1_packets t1_bytes

	t0_packets=$(devlink_trap_rx_packets_get $trap_name)
	t0_bytes=$(devlink_trap_rx_bytes_get $trap_name)

	sleep 1

	t1_packets=$(devlink_trap_rx_packets_get $trap_name)
	t1_bytes=$(devlink_trap_rx_bytes_get $trap_name)

	if [[ $t0_packets -eq $t1_packets && $t0_bytes -eq $t1_bytes ]]; then
		return 0
	else
		return 1
	fi
}

devlink_trap_group_rx_packets_get()
{
	local group_name=$1; shift

	devlink -js trap group show $DEVLINK_DEV group $group_name \
		| jq '.[][][]["stats"]["rx"]["packets"]'
}

devlink_trap_group_rx_bytes_get()
{
	local group_name=$1; shift

	devlink -js trap group show $DEVLINK_DEV group $group_name \
		| jq '.[][][]["stats"]["rx"]["bytes"]'
}

devlink_trap_group_stats_idle_test()
{
	local group_name=$1; shift
	local t0_packets t0_bytes
	local t1_packets t1_bytes

	t0_packets=$(devlink_trap_group_rx_packets_get $group_name)
	t0_bytes=$(devlink_trap_group_rx_bytes_get $group_name)

	sleep 1

	t1_packets=$(devlink_trap_group_rx_packets_get $group_name)
	t1_bytes=$(devlink_trap_group_rx_bytes_get $group_name)

	if [[ $t0_packets -eq $t1_packets && $t0_bytes -eq $t1_bytes ]]; then
		return 0
	else
		return 1
	fi
}

devlink_trap_mon_input_port_test()
{
	local trap_name=$1; shift
	local input_port=$1; shift
	local devlink_pid
	local tmp_dir
	local rc

	tmp_dir="$(mktemp -d)"

	devlink_trap_report_set $trap_name "true"
	timeout 1 devlink -v mon trap-report &> ${tmp_dir}/reports.txt
	devlink_trap_report_set $trap_name "false"

	grep -e "$trap_name" ${tmp_dir}/reports.txt -A 2 \
		| grep -q -e "$input_port"

	# Return false if no reports were found.
	rc=$?

	rm -rf $tmp_dir

	return $rc
}
