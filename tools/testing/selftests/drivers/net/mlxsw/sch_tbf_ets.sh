#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="
	ping_ipv4
	tbf_test
"
: ${QDISC:=ets strict}
source sch_tbf_core.sh

tbf_test_one()
{
	local bs=$1; shift

	tc qdisc replace dev $swp2 parent 10:8 handle 108: tbf \
	   rate 400Mbit burst $bs limit 1M
	tc qdisc replace dev $swp2 parent 10:7 handle 107: tbf \
	   rate 800Mbit burst $bs limit 1M

	do_tbf_test 10 400 $bs
	do_tbf_test 11 800 $bs
}

tbf_test()
{
	tc qdisc add dev $swp2 root handle 10: $QDISC 8 \
	   priomap 7 6 5 4 3 2 1 0
	tbf_test_one 128K
	tc qdisc del dev $swp2 root
}

trap cleanup EXIT

setup_prepare
setup_wait

bail_on_lldpad
tests_run

exit $EXIT_STATUS
