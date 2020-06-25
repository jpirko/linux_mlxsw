#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="
	ping_ipv4
	fifo_mirror_test
	fifo_trap_test
"
: ${QDISC:=ets}
source sch_red_core.sh

_1MB=$((1000 * 1000))
iPOOL=1
ePOOL=5
INGRESS_TC=0
BACKLOG=$_1MB
RED_MIN=100000 # Set RED min higher than the queue limit.

fifo_setup_prepare()
{
	setup_prepare

	devlink_pool_size_thtype_save $iPOOL
	devlink_pool_size_thtype_save $ePOOL
	devlink_port_pool_th_save $swp2 $iPOOL
	devlink_port_pool_th_save $swp3 $ePOOL
	devlink_port_pool_th_save $swp5 $iPOOL

	devlink_tc_bind_pool_th_save $swp2 $INGRESS_TC ingress
	devlink_tc_bind_pool_th_save $swp5 $INGRESS_TC ingress
	for tc in {0..7}; do
		devlink_tc_bind_pool_th_save $swp3 $tc egress
	done

	devlink_pool_size_thtype_change $iPOOL static $_1MB
	devlink_pool_size_thtype_change $ePOOL static $_1MB
	devlink_port_pool_th_change $swp2 $iPOOL $_1MB
	devlink_port_pool_th_change $swp3 $ePOOL $_1MB
	devlink_port_pool_th_change $swp5 $iPOOL $_1MB
	devlink_tc_bind_pool_th_change $swp2 $INGRESS_TC ingress $iPOOL $_1MB
	devlink_tc_bind_pool_th_change $swp5 $INGRESS_TC ingress $iPOOL $_1MB
	for tc in {0..7}; do
		devlink_tc_bind_pool_th_change $swp3 $tc egress $ePOOL $_1MB
	done
}

fifo_cleanup()
{
	cleanup

	devlink_pool_size_thtype_restore $iPOOL
	devlink_pool_size_thtype_restore $ePOOL
	devlink_port_pool_th_restore $swp2 $iPOOL
	devlink_port_pool_th_restore $swp3 $ePOOL
	devlink_port_pool_th_restore $swp5 $iPOOL
	devlink_tc_bind_pool_th_restore $swp2 $INGRESS_TC ingress
	devlink_tc_bind_pool_th_restore $swp5 $INGRESS_TC ingress
	for tc in {0..7}; do
		devlink_tc_bind_pool_th_restore $swp3 $tc egress
	done
}

install_qdisc()
{
	local -a args=("$@")

	tc qdisc add dev $swp3 root handle 10: $QDISC \
	   bands 8 priomap 7 6 5 4 3 2 1 0
	tc qdisc add dev $swp3 parent 10:8 handle 108: bfifo \
	   limit 1000000 "${args[@]}"
	tc qdisc add dev $swp3 parent 10:7 handle 107: bfifo \
	   limit 1000000  "${args[@]}"
	sleep 1
}

uninstall_qdisc()
{
	tc qdisc del dev $swp3 parent 10:7
	tc qdisc del dev $swp3 parent 10:8
	tc qdisc del dev $swp3 root
}

fifo_mirror_test()
{
	install_qdisc qevent tail_drop block 10

	do_drop_mirror_test 10 $BACKLOG tail_drop
	do_drop_mirror_test 11 $BACKLOG tail_drop

	uninstall_qdisc
}

fifo_trap_test()
{
	install_qdisc qevent tail_drop block 10

	do_drop_trap_test 10 $BACKLOG tail_drop
	do_drop_trap_test 11 $BACKLOG tail_drop

	uninstall_qdisc
}

trap fifo_cleanup EXIT

fifo_setup_prepare
setup_wait

bail_on_lldpad
tests_run

exit $EXIT_STATUS
