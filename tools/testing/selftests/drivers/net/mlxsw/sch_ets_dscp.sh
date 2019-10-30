#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

# +--------------------+ +-------------------+ +------------------------+
# | H1                 | | H2                | | H3                     |
# |    + $h1.10        | |   + $h2.11        | |   + $h3.13             |
# |    | 192.0.2.1/28  | |   | 192.0.2.17/28 | |   | 192.0.2.33/28      |
# |    |               | |   |               | |   |                    |
# |    + $h1           | |   + $h2           | |   + $h3                |
# +----|---------------+ +---|---------------+ +---|--------------------+
#      |                     |                     |
# +----|---------------------|---------------------|--------------------+
# | SW |                     |                     |                    |
# |  +-|---------------------|---------------------|-----------------+  |
# |  | + $swp1               + $swp2               + $swp3           |  |
# |  |   DSCP 10->prio 0       DSCP 10->prio 1       DSCP 10->prio 2 |  |
# |  |                                                               |  |
# |  |                   BR (802.1Q)                                 |  |
# |  |                                                               |  |
# |  |                       + $swp2                                 |  |
# |  |                       | 1Gbps (ethtool or HTB qdisc)          |  |
# |  |                       | qdisc ets quanta $W0 $W1 $W2          |  |
# |  |                       |           priomap 0 1 2               |  |
# |  |                       |                                       |  |
# |  +-----------------------|---------------------------------------+  |
# +--------------------------|------------------------------------------+
#                            |
# +--------------------------|------------------------------------------+
# | H4                       + $h4                                      |
# |     ____________________/|\____________________                     |
# |    /                     |                     \                    |
# |    + $h4.10              + $h4.11              + $h4.13             |
# |      192.0.2.2/28          192.0.2.18/28         192.0.2.34/28      |
# +---------------------------------------------------------------------+

NUM_NETIFS=8
CHECK_TC="yes"

lib_dir=$(dirname $0)/../../../net/forwarding
source $lib_dir/lib.sh
source $lib_dir/sch_ets_tests.sh
source $lib_dir/devlink_lib.sh

ALL_TESTS="
	ping_ipv4
	$ALL_TESTS_STRICT
	$ALL_TESTS_MIXED
	$ALL_TESTS_DWRR
"

PARENT=root
TOS=8

__mlnx_qos()
{
	local out

	if ! out=$(mlnx_qos "$@"); then
		echo $out
	fi
}

sip()
{
	echo 192.0.2.$((16 * $1 + 1))
}

dip()
{
	echo 192.0.2.$((16 * $1 + 2))
}

ets_start_traffic()
{
	local i=$1; shift

	local dst_mac=$(mac_get $h4)
	local dev

	case $i in
	    0) dev=$h1.10;;
	    1) dev=$h2.11;;
	    2) dev=$h3.12;;
	esac

	start_traffic $dev $(sip $i) $(dip $i) $dst_mac \
		      $(printf 'tos=%#x' $((TOS << 2)))
}

get_stats()
{
	local band=$1; shift

	tc_rule_stats_get $h4 100$band
}

host_create()
{
	local dev=$1; shift
	local i=$1; shift

	simple_if_init $dev
	mtu_set $dev 9900
	vlan_create $dev 1$i v$dev $(sip $i)/28
	ethtool -s $dev speed 1000 autoneg off
}

host_destroy()
{
	local dev=$1; shift
	local i=$1; shift

	ethtool -s $dev autoneg on
	vlan_destroy $dev 1$i
	mtu_restore $dev
	simple_if_fini $dev
}

h1_create()
{
	host_create $h1 0
}

h1_destroy()
{
	host_destroy $h1 0
}

h2_create()
{
	host_create $h2 1
}

h2_destroy()
{
	host_destroy $h2 1
}

h3_create()
{
	host_create $h3 2
}

h3_destroy()
{
	host_destroy $h3 2
}

h4_create()
{
	local i

	simple_if_init $h4
	mtu_set $h4 9900
	for i in {0..2}; do
		vlan_create $h4 1$i v$h4 $(dip $i)/28
	done
	ethtool -s $h4 speed 1000 autoneg off

	tc qdisc add dev $h4 clsact
	for i in {0..2}; do
		tc filter add dev $h4 ingress protocol 802.1q pref 100$i \
		   flower skip_sw vlan_id 1$i action pass
	done
}

h4_destroy()
{
	local i

	tc qdisc del dev $h4 clsact

	ethtool -s $h4 autoneg on
	for i in {0..2}; do
		vlan_destroy $h4 1$i
	done
	mtu_restore $h4
	simple_if_fini $h4
}

switch_create()
{
	local dev

	ip link add dev br type bridge vlan_filtering 1

	for dev in $swp1 $swp2 $swp3 $swp4; do
		mtu_set $dev 9900
		ip link set dev $dev master br
	done

	bridge vlan add dev $swp1 vid 10
	bridge vlan add dev $swp2 vid 11
	bridge vlan add dev $swp3 vid 12
	bridge vlan add dev $swp4 vid 10
	bridge vlan add dev $swp4 vid 11
	bridge vlan add dev $swp4 vid 12

	for dev in $swp1 $swp2 $swp3 $swp4; do
		ip link set dev $dev up
		ethtool -s $dev speed 1000 autoneg off
	done

	ip link set dev br up

	__mlnx_qos -i $swp1 --dscp2prio set,$TOS,0
	__mlnx_qos -i $swp2 --dscp2prio set,$TOS,1
	__mlnx_qos -i $swp3 --dscp2prio set,$TOS,2

	# Set the ingress quota high and use the three egress TCs to limit the
	# amount of traffic that is admitted to the shared buffers. This makes
	# sure that there is always enough traffic of all types to select from
	# for the ETS process.

	for dev in $swp1 $swp2 $swp3; do
		devlink_port_pool_th_set $dev 0 12
		devlink_tc_bind_pool_th_set $dev 0 ingress 0 12
	done

	devlink_port_pool_th_set $swp4 4 12
	devlink_tc_bind_pool_th_set $swp4 7 egress 4 5
	devlink_tc_bind_pool_th_set $swp4 6 egress 4 5
	devlink_tc_bind_pool_th_set $swp4 5 egress 4 5
}

switch_destroy()
{
	local dev

	tc qdisc del dev $swp4 root

	devlink_tc_bind_pool_th_restore $swp4 5 egress
	devlink_tc_bind_pool_th_restore $swp4 6 egress
	devlink_tc_bind_pool_th_restore $swp4 7 egress
	devlink_port_pool_th_restore $swp4 4

	for dev in $swp3 $swp2 $swp1; do
		devlink_tc_bind_pool_th_restore $dev 0 ingress
		devlink_port_pool_th_restore $dev 0
	done

	__mlnx_qos -i $swp3 --dscp2prio del,$TOS,2
	__mlnx_qos -i $swp2 --dscp2prio del,$TOS,1
	__mlnx_qos -i $swp1 --dscp2prio del,$TOS,0

	ip link set dev br down

	for dev in $swp1 $swp2 $swp3 $swp4; do
		ethtool -s $dev autoneg on
		ip link set dev $dev down
		ip link set dev $dev nomaster
		mtu_restore $dev
	done

	ip link del dev br
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	swp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	swp3=${NETIFS[p5]}
	h3=${NETIFS[p6]}

	swp4=${NETIFS[p7]}
	h4=${NETIFS[p8]}

	put=$swp4
	hut=$h4

	vrf_prepare

	h1_create
	h2_create
	h3_create
	h4_create
	switch_create
}

cleanup()
{
	pre_cleanup

	switch_destroy
	h4_destroy
	h3_destroy
	h2_destroy
	h1_destroy

	vrf_cleanup
}

ping_ipv4()
{
	ping_test $h1.10 $(dip 0) " vlan 10"
	ping_test $h2.11 $(dip 1) " vlan 11"
	ping_test $h3.12 $(dip 2) " vlan 12"
}

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
