#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

# This test uses standard topology for testing gretap. See
# mirror_gre_topo_lib.sh for more details.
#
# Test for "tc action mirred egress mirror" when the underlay route points at a
# bridge device with vlan filtering (802.1q).

ALL_TESTS="
	test_gretap
	test_ip6gretap
"

NUM_NETIFS=6
source lib.sh
source mirror_lib.sh
source mirror_gre_lib.sh
source mirror_gre_topo_lib.sh

setup_prepare()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	swp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	swp3=${NETIFS[p5]}
	h3=${NETIFS[p6]}

	vrf_prepare
	mirror_gre_topo_create

	ip link set dev $swp3 master br1
	bridge vlan add dev br1 vid 555 pvid untagged self
	ip address add dev br1 192.0.2.129/32
	ip address add dev br1 2001:db8:2::1/128

	ip -4 route add 192.0.2.130/32 dev br1
	ip -6 route add 2001:db8:2::2/128 dev br1

	vlan_create $h3 555 v$h3 192.0.2.130/28 2001:db8:2::2/64
	bridge vlan add dev $swp3 vid 555
}

cleanup()
{
	pre_cleanup

	ip link set dev $swp3 nomaster
	vlan_destroy $h3 555

	mirror_gre_topo_destroy
	vrf_cleanup
}

test_gretap()
{
	full_test_span_gre_dir gt4 ingress 8 0 "mirror to gretap"
	full_test_span_gre_dir gt4 egress 0 8 "mirror to gretap"
}

test_ip6gretap()
{
	full_test_span_gre_dir gt6 ingress 8 0 "mirror to ip6gretap"
	full_test_span_gre_dir gt6 egress 0 8 "mirror to ip6gretap"
}

tests()
{
	slow_path_trap_install $swp1 ingress
	slow_path_trap_install $swp1 egress

	tests_run

	slow_path_trap_uninstall $swp1 egress
	slow_path_trap_uninstall $swp1 ingress
}

trap cleanup EXIT

setup_prepare
setup_wait

tcflags="skip_hw"
tests

if ! tc_offload_check; then
	echo "WARN: Could not test offloaded functionality"
else
	tcflags="skip_sw"
	tests
fi

exit $EXIT_STATUS
