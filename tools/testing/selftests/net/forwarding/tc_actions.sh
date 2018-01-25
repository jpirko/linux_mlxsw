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
	simple_if_init $h2 192.0.2.2/24
	tc qdisc add dev $h2 clsact
}

h2_fini()
{
	tc qdisc del dev $h2 clsact
	simple_if_fini $h2 192.0.2.2/24
}

switch_init()
{
	simple_if_init $swp1 192.0.2.2/24
	tc qdisc add dev $swp1 clsact

	simple_if_init $swp2 192.0.2.1/24
}

switch_fini()
{
	simple_if_fini $swp2 192.0.2.1/24

	tc qdisc del dev $swp1 clsact
	simple_if_fini $swp1 192.0.2.2/24
}

mirred_egress_redirect_test()
{
	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		$tcflags dst_ip 192.0.2.2 action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "matched without redirect rule inserted"

	tc filter add dev $swp1 ingress protocol ip pref 1 handle 101 flower \
		$tcflags dst_ip 192.0.2.2 action mirred egress redirect dev $swp2

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_err $? "did not match incoming redirected packet"

	tc filter del dev $swp1 ingress protocol ip pref 1 handle 101 flower
	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower

	log_test "mirred egress redirect ($tcflags)"
}

gact_drop_and_ok_test()
{
	RET=0

	tc filter add dev $swp1 ingress protocol ip pref 2 handle 102 flower \
		skip_hw dst_ip 192.0.2.2 action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $swp1 ingress" 102 1
	check_err $? "packet was not dropped"

	tc filter add dev $swp1 ingress protocol ip pref 1 handle 101 flower \
		$tcflags dst_ip 192.0.2.2 action ok

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $swp1 ingress" 101 1
	check_err $? "did not see trapped packet"

	tc filter del dev $swp1 ingress protocol ip pref 2 handle 102 flower
	tc filter del dev $swp1 ingress protocol ip pref 1 handle 101 flower

	log_test "gact drop and ok ($tcflags)"
}

gact_trap_test()
{
	RET=0

	tc filter add dev $swp1 ingress protocol ip pref 1 handle 101 flower \
		skip_hw dst_ip 192.0.2.2 action drop
	tc filter add dev $swp1 ingress protocol ip pref 3 handle 103 flower \
		$tcflags dst_ip 192.0.2.2 action mirred egress redirect dev $swp2

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $swp1 ingress" 101 1
	check_fail $? "saw packet without trap rule inserted"

	tc filter add dev $swp1 ingress protocol ip pref 2 handle 102 flower \
		$tcflags dst_ip 192.0.2.2 action trap

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $swp1 ingress" 102 1
	check_err $? "packet was not trapped"

	tc_check_packets "dev $swp1 ingress" 101 1
	check_err $? "did not see trapped packet"

	tc filter del dev $swp1 ingress protocol ip pref 3 handle 103 flower
	tc filter del dev $swp1 ingress protocol ip pref 2 handle 102 flower
	tc filter del dev $swp1 ingress protocol ip pref 1 handle 101 flower

	log_test "trap ($tcflags)"
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

	swp1origmac=$(mac_get $swp1)
	swp2origmac=$(mac_get $swp2)
	ip link set $swp1 address $h2mac
	ip link set $swp2 address $h1mac

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
	ip link set $swp1 address $swp1origmac
}

trap cleanup EXIT

setup_prepare
setup_wait

gact_drop_and_ok_test
mirred_egress_redirect_test

tc_offload_check
if [[ $? -ne 0 ]]; then
	log_info "Could not test offloaded functionality"
else
	tcflags="skip_sw"
	gact_drop_and_ok_test
	mirred_egress_redirect_test
	gact_trap_test
fi

exit $EXIT_STATUS
