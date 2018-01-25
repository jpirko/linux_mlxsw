#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

NUM_NETIFS=4
source lib.sh
source tc_common.sh

tcflags="skip_hw"

h1_init()
{
	simple_if_init $h1 192.0.2.1/24
}

h1_fini()
{
	simple_if_fini $h1 192.0.2.1/24
}

h2_init()
{
	simple_if_init $h2 192.0.2.1/24
}

h2_fini()
{
	simple_if_fini $h2 192.0.2.1/24
}

switch_init()
{
	simple_if_init $swp1 192.0.2.2/24
	tc qdisc add dev $swp1 ingress_block 22 egress_block 23 clsact

	simple_if_init $swp2 192.0.2.2/24
	tc qdisc add dev $swp2 ingress_block 22 egress_block 23 clsact
}

switch_fini()
{
	tc qdisc del dev $swp2 clsact
	simple_if_fini $swp2 192.0.2.2/24

	tc qdisc del dev $swp1 clsact
	simple_if_fini $swp1 192.0.2.2/24
}

shared_block_test()
{
	RET=0

	tc filter add block 22 protocol ip pref 1 handle 101 flower \
		$tcflags dst_ip 192.0.2.2 action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $swmac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "block 22" 101 1
	check_err $? "did not match first incoming packet on a block"

	mausezahn $h2 -c 1 -p 64 -a $h2mac -b $swmac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "block 22" 101 2
	check_err $? "did not match second incoming packet on a block"

	tc filter del block 22 protocol ip pref 1 handle 101 flower

	log_test "shared block ($tcflags)"
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	swp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	h1mac=$(mac_get $h1)
	h2mac=$(mac_get $h2)

	if [[ "${OPTIONS[noprepare]}" == "yes" ]]; then
		echo "INFO: Not doing setup prepare"
		return 0
	fi

	swmac=$(mac_get $swp1)
	swp2origmac=$(mac_get $swp2)
	ip link set $swp2 address $swmac

	vrf_prepare

	h1_init
	h2_init
	switch_init
}

cleanup()
{
	if [[ "${OPTIONS[nocleanup]}" == "yes" ]]; then
		echo "INFO: Not doing cleanup"
		return 0
	fi

	switch_fini
	h2_fini
	h1_fini

	vrf_cleanup

	ip link set $swp2 address $swp2origmac
}

trap cleanup EXIT

setup_prepare
setup_wait

shared_block_test

tc_offload_check
if [[ $? -ne 0 ]]; then
	log_info "Could not test offloaded functionality"
else
	tcflags="skip_sw"
	shared_block_test
fi

exit $EXIT_STATUS
