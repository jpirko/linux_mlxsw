#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Test devlink-trap L2 drops functionality over mlxsw. Each registered L2 drop
# packet trap is tested to make sure it is triggered under the right
# conditions.

lib_dir=$(dirname $0)/../../../net/forwarding

ALL_TESTS="
	ingress_smac_mc_drop_test
	ingress_vlan_tag_allow_drop_test
	ingress_vlan_filter_drop_test
	ingress_stp_filter_drop_test
	uc_empty_tx_list_drop_test
	mc_empty_tx_list_drop_test
	uc_loopback_filter_drop_test
"
NUM_NETIFS=4
source $lib_dir/tc_common.sh
source $lib_dir/lib.sh
source $lib_dir/devlink_lib.sh

h1_create()
{
	simple_if_init $h1
}

h1_destroy()
{
	simple_if_fini $h1
}

h2_create()
{
	simple_if_init $h2
}

h2_destroy()
{
	simple_if_fini $h2
}

switch_create()
{
	ip link add dev br0 type bridge vlan_filtering 1 mcast_snooping 0

	ip link set dev $swp1 master br0
	ip link set dev $swp2 master br0

	ip link set dev br0 up
	ip link set dev $swp1 up
	ip link set dev $swp2 up

	tc qdisc add dev $swp2 clsact
}

switch_destroy()
{
	tc qdisc del dev $swp2 clsact

	ip link set dev $swp2 down
	ip link set dev $swp1 down

	ip link del dev br0
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	swp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	vrf_prepare

	h1_create
	h2_create

	switch_create
}

cleanup()
{
	pre_cleanup

	switch_destroy

	h2_destroy
	h1_destroy

	vrf_cleanup
}

l2_drops_test()
{
	local trap_name=$1; shift
	local group_name=$1; shift

	# This is the common part of all the tests. It checks that stats are
	# initially idle, then non-idle after changing the trap action and
	# finally idle again. It also makes sure the packets are dropped and
	# never forwarded.
	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle with initial drop action"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with initial drop action"

	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_fail $? "Trap stats idle after setting action to trap"
	devlink_trap_group_stats_idle_test $group_name
	check_fail $? "Trap group stats idle after setting action to trap"

	devlink_trap_mon_input_port_test $trap_name $swp1
	check_err $? "Port $swp1 not reported as input port"

	devlink_trap_action_set $trap_name "drop"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle after setting action to drop"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle after setting action to drop"

	tc_check_packets "dev $swp2 egress" 101 0
	check_err $? "Packets were not dropped"
}

l2_drops_cleanup()
{
	local mz_pid=$1; shift

	kill $mz_pid && wait $mz_pid &> /dev/null
	tc filter del dev $swp2 egress protocol ip pref 1 handle 101 flower
}

ingress_smac_mc_drop_test()
{
	local trap_name="ingress_smac_mc_drop"
	local smac=01:02:03:04:05:06
	local group_name="l2_drops"
	local mz_pid

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower src_mac $smac action drop

	$MZ $h1 -c 0 -p 100 -a $smac -b bcast -t ip -d 1msec -q &
	mz_pid=$!

	RET=0

	l2_drops_test $trap_name $group_name

	log_test "Ingress multicast source MAC"

	l2_drops_cleanup $mz_pid
}

__ingress_vlan_tag_allow_drop_test()
{
	local trap_name="ingress_vlan_tag_allow_drop"
	local dmac=de:ad:be:ef:13:37
	local group_name="l2_drops"
	local opt=$1; shift
	local mz_pid

	# Remove PVID flag. This should prevent untagged and prio-tagged
	# packets from entering the bridge.
	bridge vlan add vid 1 dev $swp1 untagged master

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 "$opt" -c 0 -p 100 -a own -b $dmac -t ip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Add PVID and make sure packets are no longer dropped.
	bridge vlan add vid 1 dev $swp1 pvid untagged master
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	l2_drops_cleanup $mz_pid
}

ingress_vlan_tag_allow_drop_untagged_test()
{
	RET=0

	__ingress_vlan_tag_allow_drop_test

	log_test "Ingress VLAN tag allow - untagged packets"
}

ingress_vlan_tag_allow_drop_vid_0_test()
{
	RET=0

	__ingress_vlan_tag_allow_drop_test "-Q 0"

	log_test "Ingress VLAN tag allow - prio-tagged packets"
}

ingress_vlan_tag_allow_drop_test()
{
	ingress_vlan_tag_allow_drop_untagged_test
	ingress_vlan_tag_allow_drop_vid_0_test
}

ingress_vlan_filter_drop_test()
{
	local trap_name="ingress_vlan_filter_drop"
	local dmac=de:ad:be:ef:13:37
	local group_name="l2_drops"
	local mz_pid
	local vid=10

	bridge vlan add vid $vid dev $swp2 master
	# During initialization the firmware enables all the VLAN filters and
	# the driver does not turn them off since the traffic will be discarded
	# by the STP filter whose default is DISCARD state. Add the VID on the
	# ingress bridge port and then remove it to make sure it is not member
	# in the VLAN.
	bridge vlan add vid $vid dev $swp1 master
	bridge vlan del vid $vid dev $swp1 master

	RET=0

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 -Q $vid -c 0 -p 100 -a own -b $dmac -t ip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Add the VLAN on the bridge port and make sure packets are no longer
	# dropped.
	bridge vlan add vid $vid dev $swp1 master
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	log_test "Ingress VLAN filter"

	l2_drops_cleanup $mz_pid

	bridge vlan del vid $vid dev $swp1 master
	bridge vlan del vid $vid dev $swp2 master
}

__ingress_stp_filter_drop_test()
{
	local trap_name="ingress_stp_filter_drop"
	local dmac=de:ad:be:ef:13:37
	local group_name="l2_drops"
	local state=$1; shift
	local mz_pid
	local vid=20

	bridge vlan add vid $vid dev $swp2 master
	bridge vlan add vid $vid dev $swp1 master
	ip link set dev $swp1 type bridge_slave state $state

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 -Q $vid -c 0 -p 100 -a own -b $dmac -t ip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Change STP state to forwarding and make sure packets are no longer
	# dropped.
	ip link set dev $swp1 type bridge_slave state 3
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	l2_drops_cleanup $mz_pid

	bridge vlan del vid $vid dev $swp1 master
	bridge vlan del vid $vid dev $swp2 master
}

ingress_stp_filter_drop_listening_test()
{
	local state=$1; shift

	RET=0

	__ingress_stp_filter_drop_test $state

	log_test "Ingress STP filter - listening state"
}

ingress_stp_filter_drop_learning_test()
{
	local state=$1; shift

	RET=0

	__ingress_stp_filter_drop_test $state

	log_test "Ingress STP filter - learning state"
}

ingress_stp_filter_drop_test()
{
	ingress_stp_filter_drop_listening_test 1
	ingress_stp_filter_drop_learning_test 2
}

uc_empty_tx_list_drop_test()
{
	local trap_name="uc_empty_tx_list_drop"
	local dmac=de:ad:be:ef:13:37
	local group_name="l2_drops"
	local mz_pid

	# Disable unicast flooding on both ports, so that packets cannot egress
	# any port.
	ip link set dev $swp1 type bridge_slave flood off
	ip link set dev $swp2 type bridge_slave flood off

	RET=0

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 -c 0 -p 100 -a own -b $dmac -t ip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Allow packets to be flooded to one port.
	ip link set dev $swp2 type bridge_slave flood on
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	log_test "Unicast empty Tx"

	l2_drops_cleanup $mz_pid

	ip link set dev $swp1 type bridge_slave flood on
}

mc_empty_tx_list_drop_test()
{
	local trap_name="mc_empty_tx_list_drop"
	local dmac=01:00:5e:00:00:01
	local group_name="l2_drops"
	local dip=239.0.0.1
	local mz_pid

	# Disable multicast flooding on both ports, so that packets cannot
	# egress any port. We also need to flush IP addresses from the bridge
	# in order to prevent packets from being flooded to the router port.
	ip link set dev $swp1 type bridge_slave mcast_flood off
	ip link set dev $swp2 type bridge_slave mcast_flood off
	ip address flush dev br0

	RET=0

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 -c 0 -p 100 -a own -b $dmac -t ip -B $dip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Allow packets to be flooded to one port.
	ip link set dev $swp2 type bridge_slave mcast_flood on
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	log_test "Multicast empty Tx"

	l2_drops_cleanup $mz_pid

	ip link set dev $swp1 type bridge_slave mcast_flood on
}

uc_loopback_filter_drop_test()
{
	local trap_name="uc_loopback_filter_drop"
	local dmac=de:ad:be:ef:13:37
	local group_name="l2_drops"
	local mz_pid

	# Make sure packets can only egress the input port.
	ip link set dev $swp2 type bridge_slave flood off

	RET=0

	tc filter add dev $swp2 egress protocol ip pref 1 handle 101 \
		flower dst_mac $dmac action drop

	$MZ $h1 -c 0 -p 100 -a own -b $dmac -t ip -d 1msec -q &
	mz_pid=$!

	l2_drops_test $trap_name $group_name

	# Allow packets to be flooded.
	ip link set dev $swp2 type bridge_slave flood on
	devlink_trap_action_set $trap_name "trap"

	devlink_trap_stats_idle_test $trap_name
	check_err $? "Trap stats not idle when packets should not be dropped"
	devlink_trap_group_stats_idle_test $group_name
	check_err $? "Trap group stats not idle with when packets should not be dropped"

	tc_check_packets "dev $swp2 egress" 101 0
	check_fail $? "Packets not forwarded when should"

	devlink_trap_action_set $trap_name "drop"

	log_test "Unicast loopback filter"

	l2_drops_cleanup $mz_pid
}

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
