#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="
	ping_ipv4
	xxx
"
NUM_NETIFS=6
CHECK_TC="yes"
lib_dir=$(dirname $0)/../../../net/forwarding
source $lib_dir/lib.sh
source $lib_dir/devlink_lib.sh

host_create()
{
	local dev=$1; shift
	local ip10=$1; shift
	local ip11=$1; shift

	simple_if_init $dev
	mtu_set $dev 9900

	vlan_create $dev 10 v$dev $ip10
	ip link set dev $dev.10 type vlan egress 0:0

	vlan_create $dev 11 v$dev $ip11
	ip link set dev $dev.11 type vlan egress 0:1
}

host_destroy()
{
	local dev=$1; shift

	vlan_destroy $dev 11
	vlan_destroy $dev 10
	mtu_restore $dev
	simple_if_fini $dev
}

h1_create()
{
	host_create $h1 192.0.2.1/28 192.0.2.17/28
}

h1_destroy()
{
	host_destroy $h1
}

h2_create()
{
	host_create $h2 192.0.2.3/28 192.0.2.19/28
}

h2_destroy()
{
	host_destroy $h2
}

h3_create()
{
	host_create $h3 192.0.2.4/28 192.0.2.20/28
}

h3_destroy()
{
	host_destroy $h3
}

switch_create()
{
	local intf
	local vlan
	local i

	ip link add dev br2_10 type bridge vlan_filtering 0
	ip link add dev br2_11 type bridge vlan_filtering 0

	for intf in $swp1 $swp2 $swp3; do
		ip link set dev $intf up
		mtu_set $intf 9900
		ethtool -s $intf speed 1000 autoneg off

		for vlan in 10 11; do
			vlan_create $intf $vlan
			ip link set dev $intf.$vlan master br2_$vlan
			ip link set dev $intf.$vlan up
		done
	done
	for intf in $swp1 $swp2; do
		for vlan in 10 11; do
			ip link set dev $intf.$vlan type vlan ingress 0:0 1:1
		done
	done

	ip link set dev br2_10 up
	ip link set dev br2_11 up
	bridge fdb add dev $swp3.10 $h3_mac master
	bridge fdb add dev $swp3.11 $h3_mac master

	tc qdisc add dev $swp3 root handle 10: ets \
	   bands 8 priomap 7 6 5 4 3 2 1 0

	tc qdisc add dev $swp3 parent 10:8 handle 108: red ecn \
	   limit 1000000 min 300000 max 300001 probability 1.0 \
	   avpkt 8000 burst 38

	tc qdisc add dev $swp3 parent 10:7 handle 107: red ecn \
	   limit 1000000 min 500000 max 500001 probability 1.0 \
	   avpkt 8000 burst 63
}

switch_destroy()
{
	local intf
	local vlan

	tc qdisc del dev $swp3 root

	ip link set dev br2_11 down
	ip link set dev br2_10 down

	for intf in $swp1 $swp2 $swp3; do
		for vlan in 11 10; do
			ip link set dev $intf.$vlan down
			ip link set dev $intf.$vlan nomaster
			vlan_destroy $intf $vlan
		done

		ethtool -s $intf autoneg on
		mtu_restore $intf
		ip link set dev $intf down
	done

	ip link del dev br2_10
	ip link del dev br2_11
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	swp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	swp3=${NETIFS[p5]}
	h3=${NETIFS[p6]}

	h3_mac=$(mac_get $h3)

	vrf_prepare

	h1_create
	h2_create
	h3_create
	switch_create
}

cleanup()
{
	pre_cleanup

	switch_destroy
	h3_destroy
	h2_destroy
	h1_destroy

	vrf_cleanup
}

ping_ipv4()
{
	ping_test $h2.10 192.0.2.4 " 10"
	ping_test $h2.11 192.0.2.20 " 11"
}

xxx()
{
	read -p Ready.
}

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
