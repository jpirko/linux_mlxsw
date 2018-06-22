#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="template_create_destroy template_filter_fits \
	   template_create_nonempty template_destroy_nonempty"
NUM_NETIFS=2
source tc_common.sh
source lib.sh

h1_create()
{
	simple_if_init $h1 192.0.2.1/24
}

h1_destroy()
{
	simple_if_fini $h1 192.0.2.1/24
}

h2_create()
{
	simple_if_init $h2 192.0.2.2/24
	tc qdisc add dev $h2 clsact
}

h2_destroy()
{
	tc qdisc del dev $h2 clsact
	simple_if_fini $h2 192.0.2.2/24
}

template_create_destroy()
{
	RET=0

	tc chaintemplate add dev $h2 ingress protocol ip \
		flower dst_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF
	check_err $? "Failed to create template for default chain"

	tc chaintemplate add dev $h2 ingress chain 1 protocol ip \
		flower dst_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF
	check_err $? "Failed to create template for chain 1"

	tc chaintemplate del dev $h2 ingress
	check_err $? "Failed to destroy template for default chain"

	tc chaintemplate del dev $h2 ingress chain 1
	check_err $? "Failed to destroy template for chain 1"

	log_test "template create destroy"
}

template_filter_fits()
{
	RET=0

	tc chaintemplate add dev $h2 ingress protocol ip \
		flower dst_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF &> /dev/null
	tc chaintemplate add dev $h2 ingress chain 1 protocol ip \
		flower src_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF &> /dev/null

	tc filter add dev $h2 ingress protocol ip pref 1 handle 1101 \
		flower dst_mac $h2mac action drop
	check_err $? "Failed to insert filter which fits template"

	tc filter add dev $h2 ingress protocol ip pref 1 handle 1102 \
		flower src_mac $h2mac action drop &> /dev/null
	check_fail $? "Incorrectly succeded to insert filter which does not template"

	tc filter add dev $h2 ingress chain 1 protocol ip pref 1 handle 1101 \
		flower src_mac $h2mac action drop
	check_err $? "Failed to insert filter which fits template"

	tc filter add dev $h2 ingress chain 1protocol ip pref 1 handle 1102 \
		flower dst_mac $h2mac action drop &> /dev/null
	check_fail $? "Incorrectly succeded to insert filter which does not template"

	tc filter del dev $h2 ingress chain 1 protocol ip pref 1 handle 1102 \
		flower &> /dev/null
	tc filter del dev $h2 ingress chain 1 protocol ip pref 1 handle 1101 \
		flower &> /dev/null

	tc filter del dev $h2 ingress protocol ip pref 1 handle 1102 \
		flower &> /dev/null
	tc filter del dev $h2 ingress protocol ip pref 1 handle 1101 \
		flower &> /dev/null

	tc chaintemplate del dev $h2 ingress chain 1
	tc chaintemplate del dev $h2 ingress

	log_test "template filter fits"
}

template_create_nonempty()
{
	RET=0

	tc filter add dev $h2 ingress protocol ip pref 1 handle 1101 \
		flower dst_mac $h2mac action drop
	tc chaintemplate add dev $h2 ingress protocol ip \
		flower dst_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF &> /dev/null
	check_fail $? "Incorrectly succeded to create template for non-empty chain"

	tc chaintemplate del dev $h2 ingress &> /dev/null
	tc filter del dev $h2 ingress protocol ip pref 1 handle 1101 flower

	log_test "template create non-empty"
}

template_destroy_nonempty()
{
	RET=0

	tc chaintemplate add dev $h2 ingress protocol ip \
		flower dst_mac 00:00:00:00:00:00/FF:FF:FF:FF:FF:FF
	tc filter add dev $h2 ingress protocol ip pref 1 handle 1101 \
		flower dst_mac $h2mac action drop

	tc chaintemplate del dev $h2 ingress &> /dev/null
	check_fail $? "Incorrectly succeded to destroy template for non-empty chain"
	tc filter del dev $h2 ingress protocol ip pref 1 handle 1101 flower
	tc chaintemplate del dev $h2 ingress &> /dev/null
	check_err $? "Failed to destroy template for empty chain"

	log_test "template destroy non-empty"
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	h2=${NETIFS[p2]}
	h1mac=$(mac_get $h1)
	h2mac=$(mac_get $h2)

	vrf_prepare

	h1_create
	h2_create
}

cleanup()
{
	pre_cleanup

	h2_destroy
	h1_destroy

	vrf_cleanup
}

check_tc_chaintemplate_support

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
