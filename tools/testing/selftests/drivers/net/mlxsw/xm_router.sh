#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

ALL_TESTS="xlt_cache_16_test \
	   no_cache_16_test \
	   xlt_cache_2x24_test \
	   no_cache_2x24_test \
	   "

NUM_NETIFS=4
lib_dir=$(dirname $0)/../../../net/forwarding
source $lib_dir/lib.sh
source $lib_dir/devlink_lib.sh
source $lib_dir/tc_common.sh

h1_create()
{
	simple_if_init $h1 192.0.2.2/24
}

h1_destroy()
{
	simple_if_fini $h1 192.0.2.2/24
}

h2_create()
{
	simple_if_init $h2 198.51.100.2/24
	tc qdisc add dev $h2 clsact
}

h2_destroy()
{
	tc qdisc del dev $h2 clsact
	simple_if_fini $h2 198.51.100.2/24
}

router_create()
{
	vrf_create_nodflt vrf-router
	ip link set dev vrf-router up

	ip link set dev $rp1 master vrf-router
	ip link set dev $rp2 master vrf-router

	ip link set dev $rp1 up
	ip link set dev $rp2 up

	ip address add 192.0.2.1/24 dev $rp1
	ip address add 2001:db8:1::1/64 dev $rp1

	ip address add 198.51.100.1/24 dev $rp2
	ip address add 2001:db8:2::1/64 dev $rp2

	ip neigh add 198.51.100.2 lladdr $h2mac dev $rp2
}

router_destroy()
{
	ip address del 2001:db8:2::1/64 dev $rp2
	ip address del 198.51.100.1/24 dev $rp2

	ip address del 2001:db8:1::1/64 dev $rp1
	ip address del 192.0.2.1/24 dev $rp1

	ip link set dev $rp2 down
	ip link set dev $rp1 down

	ip link set dev $rp2 nomaster
	ip link set dev $rp1 nomaster

	vrf_destroy_nodflt vrf-router
}

xlt_cache_16_test()
{
	local unexpected_metric
	RET=0

	# Disable the XLT cache to make sure there are no leftovers.
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value false
	# Enable the XLT cache (enabled by default).
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value true
	check_err $? "Failed to enable XLT cache"

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		skip_sw dst_ip 203.0.113.1 action drop

	# First packet should be dropped (no route),
	# cache miss with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "First packet arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1")
	check_err $? "First packet - unexpected value of metric $unexpected_metric"

	# Insert test /16 prefix - only M cache should be used.
	ip route add vrf vrf-router 203.0.0.0/16 nexthop via 198.51.100.2

	# Second packet - prefix should be learned.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_err $? "Second packet didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1" "xm_cache_learned_ipv4 1")
	check_err $? "Second packet - unexpected value of metric $unexpected_metric"

	# Third packet - prefix should be cached.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 2
	check_err $? "Third packet didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_hit_ipv4_m 1")
	check_err $? "Third packet - unexpected value of metric $unexpected_metric"

	# Remove test /16 prefix.
	ip route del vrf vrf-router 203.0.0.0/16 nexthop via 198.51.100.2

	# Fourth packet should be dropped (route was removed),
	# cache miss with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 3
	check_fail $? "Fourth packet arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1")
	check_err $? "Fourth packet - unexpected value of metric $unexpected_metric"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower

	log_test "XLT cache /16 prefix"
}

no_cache_16_test()
{
	local unexpected_metric
	RET=0

	# Disable the XLT cache. The counters should not move in the whole test.
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value false
	check_err $? "Failed to disable XLT cache"

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		skip_sw dst_ip 203.0.113.1 action drop

	# First packet should be dropped (no route).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "First packet arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "First packet - unexpected value of metric $unexpected_metric"

	# Insert test /16 prefix.
	ip route add vrf vrf-router 203.0.0.0/16 nexthop via 198.51.100.2

	# Second packet.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_err $? "Second packet didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Second packet - unexpected value of metric $unexpected_metric"

	# Third packet.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 2
	check_err $? "Third packet didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Third packet - unexpected value of metric $unexpected_metric"

	# Remove test /16 prefix.
	ip route del vrf vrf-router 203.0.0.0/16 nexthop via 198.51.100.2

	# Fourth packet should be dropped (route was removed).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 3
	check_fail $? "Fourth packet arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Fourth packet - unexpected value of metric $unexpected_metric"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower

	log_test "No XLT cache /16 prefix"
}

xlt_cache_2x24_test()
{
	local unexpected_metric
	RET=0

	# Disable the XLT cache to make sure there are no leftovers.
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value false
	# Enable the XLT cache.
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value true
	check_err $? "Failed to enable XLT cache"

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		skip_sw dst_ip 203.0.113.1 action drop # Destination A
	tc filter add dev $h2 ingress protocol ip pref 1 handle 102 flower \
		skip_sw dst_ip 203.0.114.1 action drop # Destination B

	# First packet to destination A should be dropped (no route),
	# cache miss with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "First packet to destination A arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1")
	check_err $? "First packet to destination A - unexpected value of metric $unexpected_metric"

	# Insert test /24 prefix for destination A - m/ml cache should be used.
	ip route add vrf vrf-router 203.0.113.0/24 nexthop via 198.51.100.2

	# Second packet to destination A - prefix should be learned.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_err $? "Second packet to destination A didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1" "xm_cache_learned_ipv4 1")
	check_err $? "Second packet to destination A - unexpected value of metric $unexpected_metric"

	# First packet to destination B should be dropped (no route),
	# cache miss in ML with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 1
	check_fail $? "First packet to destination B arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_ml 1")
	check_err $? "First packet to destination B - unexpected value of metric $unexpected_metric"

	# Third packet to destination A - prefix should be cached.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 2
	check_err $? "Third packet to destination A didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_hit_ipv4_ml 1")
	check_err $? "Third packet to destination A - unexpected value of metric $unexpected_metric"

	# Insert test /24 prefix for destination B - m/ml cache should be used.
	ip route add vrf vrf-router 203.0.114.0/24 nexthop via 198.51.100.2

	# Second packet to destination B - prefix should be learned.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "Second packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_ml 1" "xm_cache_learned_ipv4 1")
	check_err $? "Second packet to destination B - unexpected value of metric $unexpected_metric"

	# Third packet to destination B - prefix should be cached.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 2
	check_err $? "Third packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_hit_ipv4_ml 1")
	check_err $? "Third packet to destination B - unexpected value of metric $unexpected_metric"

	# Remove test /24 prefix to destination A.
	ip route del vrf vrf-router 203.0.113.0/24 nexthop via 198.51.100.2

	# Fourth packet should be dropped (route was removed),
	# cache miss with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 3
	check_fail $? "Fourth packet to destination A arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_ml 1")
	check_err $? "Fourth packet to destination A - unexpected value of metric $unexpected_metric"

	# Fourth packet to destination B - prefix should be still cached.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 3
	check_err $? "Fourth packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check "xm_cache_hit_ipv4_ml 1")
	check_err $? "Fourth packet to destination B - unexpected value of metric $unexpected_metric"

	# Remove test /24 prefix to destination B.
	ip route del vrf vrf-router 203.0.114.0/24 nexthop via 198.51.100.2

	# Fifth packet should be dropped (route was removed),
	# cache miss with no learn.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 4
	check_fail $? "Fifth packet arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check "xm_cache_miss_ipv4_m 1")
	check_err $? "Fifth packet - unexpected value of metric $unexpected_metric"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 102 flower
	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower

	log_test "XLT cache two /24 prefixes"
}

no_cache_2x24_test()
{
	local unexpected_metric
	RET=0

	# Disable the XLT cache. The counters should not move in the whole test.
	devlink dev param set $DEVLINK_DEV name router_xm_cache_enable cmode runtime value false
	check_err $? "Failed to disable XLT cache"

	tc filter add dev $h2 ingress protocol ip pref 1 handle 101 flower \
		skip_sw dst_ip 203.0.113.1 action drop # Destination A
	tc filter add dev $h2 ingress protocol ip pref 1 handle 102 flower \
		skip_sw dst_ip 203.0.114.1 action drop # Destination B

	# First packet to destination A should be dropped (no route).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_fail $? "First packet to destination A arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "First packet to destination A - unexpected value of metric $unexpected_metric"

	# Insert test /24 prefix for destination A.
	ip route add vrf vrf-router 203.0.113.0/24 nexthop via 198.51.100.2

	# Second packet to destination A.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 1
	check_err $? "Second packet to destination A didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Second packet to destination A - unexpected value of metric $unexpected_metric"

	# First packet to destination B should be dropped (no route).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 1
	check_fail $? "First packet to destination B arrived on nexthop which should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "First packet to destination B - unexpected value of metric $unexpected_metric"

	# Third packet to destination A..
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 2
	check_err $? "Third packet to destination A didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Third packet to destination A - unexpected value of metric $unexpected_metric"

	# Insert test /24 prefix for destination B.
	ip route add vrf vrf-router 203.0.114.0/24 nexthop via 198.51.100.2

	# Second packet to destination B.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 1
	check_err $? "Second packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Second packet to destination B - unexpected value of metric $unexpected_metric"

	# Third packet to destination B.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 2
	check_err $? "Third packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Third packet to destination B - unexpected value of metric $unexpected_metric"

	# Remove test /24 prefix to destination A.
	ip route del vrf vrf-router 203.0.113.0/24 nexthop via 198.51.100.2

	# Fourth packet should be dropped (route was removed).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.113.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 101 3
	check_fail $? "Fourth packet to destination A arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Fourth packet to destination A - unexpected value of metric $unexpected_metric"

	# Fourth packet to destination B.
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 3
	check_err $? "Fourth packet to destination B didn't arrive on nexthop"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Fourth packet to destination B - unexpected value of metric $unexpected_metric"

	# Remove test /24 prefix to destination B.
	ip route del vrf vrf-router 203.0.114.0/24 nexthop via 198.51.100.2

	# Fifth packet should be dropped (route was removed).
	devlink_metric_save
	$MZ $h1 -c 1 -p 64 -a $h1mac -b $rp1mac -A 192.0.2.2 -B 203.0.114.1 -t ip -q
	tc_check_packets "dev $h2 ingress" 102 4
	check_fail $? "Fifth packet arrived on nexthop which it should not"
	unexpected_metric=$(devlink_metric_check)
	check_err $? "Fifth packet - unexpected value of metric $unexpected_metric"

	tc filter del dev $h2 ingress protocol ip pref 1 handle 102 flower
	tc filter del dev $h2 ingress protocol ip pref 1 handle 101 flower

	log_test "No XLT cache two /24 prefixes"
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	rp1=${NETIFS[p2]}

	rp2=${NETIFS[p3]}
	h2=${NETIFS[p4]}

	h1mac=$(mac_get $h1)
	rp1mac=$(mac_get $rp1)
	rp2mac=$(mac_get $rp2)
	h2mac=$(mac_get $h2)

	vrf_prepare

	h1_create
	h2_create
	router_create
}

cleanup()
{
	pre_cleanup

	router_destroy
	h2_destroy
	h1_destroy

	vrf_cleanup
}

devlink dev param show $DEVLINK_DEV name router_xm_ipv4_enable >/dev/null 2>&1
if [ $? -eq 1 ]
then
	echo "SKIP: Test needs Spectrum board equipped with XM to run"
	exit 1
fi

check_devlink_metric_support

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
