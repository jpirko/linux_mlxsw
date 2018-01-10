#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

NUM_NETIFS=2
source lib.sh
source tc_common.sh

tcflags="skip_hw"

h1_init()
{
	simple_if_init $h1 192.0.2.1/24 198.51.100.1/24
}

h1_fini()
{
	simple_if_fini $h1 192.0.2.1/24 198.51.100.1/24
}

h2_init()
{
	simple_if_init $h2 192.0.2.2/24 198.51.100.2/24
	tc qdisc add dev $h2 clsact
}

h2_fini()
{
	tc qdisc del dev $h2 clsact
	simple_if_fini $h2 192.0.2.2/24 198.51.100.2/24
}

match_dst_mac_test()
{
	local dummy_mac=de:ad:be:ef:aa:aa

	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		$tcflags dst_mac $dummy_mac action drop
	tc filter add dev $h2 ingress protocol ip pref 2 handle 102 flower \
		$tcflags dst_mac $h2mac action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "matched on a wrong filter"

	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "did not match on correct filter"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower
	tc filter del dev $h2 ingress protocol ip pref 2 handle 102 flower

	log_test "dst_mac match ($tcflags)"
}

match_src_mac_test()
{
	local dummy_mac=de:ad:be:ef:aa:aa

	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		$tcflags src_mac $dummy_mac action drop
	tc filter add dev $h2 ingress protocol ip pref 2 handle 102 flower \
		$tcflags src_mac $h1mac action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "matched on a wrong filter"

	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "did not match on correct filter"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower
	tc filter del dev $h2 ingress protocol ip pref 2 handle 102 flower

	log_test "src_mac match ($tcflags)"
}

match_dst_ip_test()
{
	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		$tcflags dst_ip 198.51.100.2 action drop
	tc filter add dev $h2 ingress protocol ip pref 2 handle 102 flower \
		$tcflags dst_ip 192.0.2.2 action drop
	tc filter add dev $h2 ingress protocol ip pref 3 handle 103 flower \
		$tcflags dst_ip 192.0.2.0/24 action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "matched on a wrong filter"

	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "did not match on correct filter"

	tc filter del dev $h2 ingress protocol ip pref 2 handle 102 flower

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 103 1
	check_err $? "did not match on correct filter with mask"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower
	tc filter del dev $h2 ingress protocol ip pref 3 handle 103 flower

	log_test "dst_ip match ($tcflags)"
}

match_src_ip_test()
{
	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		$tcflags src_ip 198.51.100.1 action drop
	tc filter add dev $h2 ingress protocol ip pref 2 handle 102 flower \
		$tcflags src_ip 192.0.2.1 action drop
	tc filter add dev $h2 ingress protocol ip pref 3 handle 103 flower \
		$tcflags src_ip 192.0.2.0/24 action drop

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "matched on a wrong filter"

	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "did not match on correct filter"

	tc filter del dev $h2 ingress protocol ip pref 2 handle 102 flower

	mausezahn $h1 -c 1 -p 64 -a $h1mac -b $h2mac -A 192.0.2.1 -B 192.0.2.2 \
		-t ip -q

	tc_check_packets "dev $h2 ingress" 103 1
	check_err $? "did not match on correct filter with mask"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower
	tc filter del dev $h2 ingress protocol ip pref 3 handle 103 flower

	log_test "src_ip match ($tcflags)"
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	h2=${NETIFS[p2]}
	h1mac=$(mac_get $h1)
	h2mac=$(mac_get $h2)

	if [[ "${OPTIONS[noprepare]}" == "yes" ]]; then
		echo "INFO: Not doing setup prepare"
		return 0
	fi

	vrf_prepare

	h1_init
	h2_init
}

cleanup()
{
	if [[ "${OPTIONS[nocleanup]}" == "yes" ]]; then
		echo "INFO: Not doing cleanup"
		return 0
	fi

	h2_fini
	h1_fini

	vrf_cleanup
}

trap cleanup EXIT

setup_prepare
setup_wait

match_dst_mac_test
match_src_mac_test
match_dst_ip_test
match_src_ip_test

tc_offload_check
if [[ $? -ne 0 ]]; then
	log_info "Could not test offloaded functionality"
else
	tcflags="skip_sw"
	match_dst_mac_test
	match_src_mac_test
	match_dst_ip_test
	match_src_ip_test
fi

exit $EXIT_STATUS
