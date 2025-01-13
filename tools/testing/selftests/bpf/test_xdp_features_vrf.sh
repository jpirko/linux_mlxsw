#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

h1_ip=2001:db8:1::1
swp1_ip=2001:db8:1::2

# +--------------------+
# | H1                 |
# |                    |
# |              $h1 + |
# | 2001:db8:1::1/64 | |
# |                  | |
# +------------------|-+
#                    |
# +------------------|-+
# | SW               | |
# |            $swp1 + |
# | 2001:db8:1::2/64   |
# |                    |
# +--------------------+

ALL_TESTS="
	xdp_aborted_test
	xdp_drop_test
	xdp_pass_test
	xdp_tx_test
	xdp_redirect_test
"

NUM_NETIFS=2
lib_dir=$(dirname $0)/../net/forwarding
source $lib_dir/lib.sh

h1_create()
{
	simple_if_init $h1 $h1_ip/64
}

h1_destroy()
{
	simple_if_fini $h1 $h1_ip/64
}

switch_create()
{
	ip link set dev $swp1 up
	ip address add $swp1_ip/64 dev $swp1
}

switch_destroy()
{
	ip address del $swp1_ip/64 dev $swp1
	ip link set dev $swp1 down
}

setup()
{
	h1=${NETIFS[p1]}
	swp1=${NETIFS[p2]}

	vrf_prepare

	h1_create
	switch_create
}

cleanup()
{
	switch_destroy
	h1_destroy

	vrf_cleanup
}

grep_xdp_features()
{
	ss -tlp | grep -q "xdp_features"
}

xdp_features()
{
	local feature=$1; shift

	local flags="-D $swp1_ip -T $h1_ip"

	# DUT #
	./xdp_features -f $feature $flags $swp1 &
	busywait "$TIMEOUT" grep_xdp_features

	# Tester #
	ip vrf exec v$h1 ./xdp_features -f $feature $flags -t -C $swp1_ip $h1
	check_err $? "$feature failed"

	busywait "$TIMEOUT" not grep_xdp_features
}

xdp_aborted_test()
{
	RET=0

	xdp_features XDP_ABORTED
	log_test "XDP ABORTED"
}

xdp_drop_test()
{
	RET=0

	xdp_features XDP_DROP
	log_test "XDP DROP"
}

xdp_pass_test()
{
	RET=0

	xdp_features XDP_PASS
	log_test "XDP PASS"
}

xdp_tx_test()
{
	RET=0

	xdp_features XDP_TX
	log_test "XDP TX"
}

xdp_redirect_test()
{
	RET=0

	xdp_features XDP_REDIRECT
	log_test "XDP REDIRECT"
}

trap cleanup EXIT

setup
setup_wait

tests_run

exit $EXIT_STATUS
