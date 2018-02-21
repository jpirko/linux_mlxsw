#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

NUM_NETIFS=4
source lib.sh

h1_create()
{
	vrf_create "vrf-h1" 1
	ip link set dev $h1 master vrf-h1

	ip link set dev vrf-h1 up
	ip link set dev $h1 up

	ip address add 192.0.2.1/24 dev $h1
	ip address add 2001:db8:1::1/64 dev $h1
}

h1_destroy()
{
	ip address del 2001:db8:1::1/64 dev $h1
	ip address del 192.0.2.1/24 dev $h1

	ip link set dev $h1 down
	vrf_destroy "vrf-h1" 1
}

h2_create()
{
	vrf_create "vrf-h2" 2
	ip link set dev $h2 master vrf-h2

	ip link set dev vrf-h2 up
	ip link set dev $h2 up

	ip address add 192.0.2.2/24 dev $h2
	ip address add 2001:db8:1::2/64 dev $h2
}

h2_destroy()
{
	ip address del 2001:db8:1::2/64 dev $h2
	ip address del 192.0.2.2/24 dev $h2

	ip link set dev $h2 down
	vrf_destroy "vrf-h2" 2
}

switch_create()
{
	# 10 Seconds ageing time.
	ip link add dev br0 type bridge vlan_filtering 1 ageing_time 1000 \
		mcast_snooping 0

	ip link set dev $swp1 master br0
	ip link set dev $swp2 master br0

	ip link set dev br0 up
	ip link set dev $swp1 up
	ip link set dev $swp2 up
}

switch_destroy()
{
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

trap cleanup EXIT

setup_prepare
setup_wait

ping_test "vrf-h1" 192.0.2.2
ping6_test "vrf-h1" 2001:db8:1::2
learning_test "br0" $swp1 $h1 $h2
flood_test $swp2 $h1 $h2

exit $EXIT_STATUS
