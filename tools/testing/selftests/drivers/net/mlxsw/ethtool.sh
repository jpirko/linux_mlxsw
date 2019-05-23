#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
lib_dir=$(dirname $0)/../../../net/forwarding

ALL_TESTS="
	same_speeds_autoneg_off
	different_speeds_autoneg_off
	combination_of_neg_on_and_off
	advertise_subset_of_speeds
	check_highest_speed_is_chosen
	different_speeds_autoneg_on
"
NUM_NETIFS=2
source $lib_dir/lib.sh
source $lib_dir/ethtool_lib.sh

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
}

h2_destroy()
{
	simple_if_fini $h2 192.0.2.2/24
}

setup_prepare()
{
	h1=${NETIFS[p1]}
	h2=${NETIFS[p2]}

	h1_create
	h2_create
}

cleanup()
{
	pre_cleanup

	h2_destroy
	h1_destroy
}

different_speeds_get()
{
	local dev1=$1; shift
	local dev2=$1; shift
	local with_mode=$1; shift

	local -a speeds_arr

	speeds_arr=($(common_speeds_get $dev1 $dev2 $with_mode))

	if [[ ${#speeds_arr[@]} < 2 ]]; then
		check_err 1 "cannot check different speeds. There are not enough speeds"
	fi

	echo ${speeds_arr[0]} ${speeds_arr[1]}
}

same_speeds_autoneg_off()
{
	# Check that when each of the reported speeds
	# is forced, the link comes up and is operational.
	local -a speeds_arr=($(common_speeds_get $h1 $h2 0))
	for speed in "${speeds_arr[@]}"; do
		# Skip 56G because this speed isn't supported with autoneg off.
		if [[ $speed == 56000 ]]; then
			continue
		fi

		RET=0
		ethtool_set $h1 speed $speed autoneg off
		ethtool_set $h2 speed $speed autoneg off

		setup_wait_dev_with_timeout $h1
		setup_wait_dev_with_timeout $h2
		ping_do $h1 192.0.2.2
		check_err $? " speed $speed autoneg off"
		log_test "force of same speed ($speed) autoneg off"
	done

	ethtool -s $h2 autoneg on
	ethtool -s $h1 autoneg on
}

different_speeds_autoneg_off()
{
	# Test that when we force different speeds,
	# links aren't up and ping fails.
	RET=0

	local -a speeds_arr=($(different_speeds_get $h1 $h2 0))
	local speed1=${speeds_arr[0]}
	local speed2=${speeds_arr[1]}

	ethtool_set $h1 speed $speed1 autoneg off
	ethtool_set $h2 speed $speed2 autoneg off

	setup_wait_dev_with_timeout $h1
	setup_wait_dev_with_timeout $h2
	ping_do $h1 192.0.2.2
	check_fail $? "ping with different speeds"

	log_test "force of different speeds autoneg off"

	ethtool -s $h2 autoneg on
	ethtool -s $h1 autoneg on
}

combination_of_neg_on_and_off()
{
	# Test that when one dev is forced to a speed supported
	# by both endpoints and the other dev is configured to autoneg on,
	# the links are up and ping succeeds.
	local -a speeds_arr=($(common_speeds_get $h1 $h2 0))

	for speed in "${speeds_arr[@]}"; do
		# Skip 56G because this speed isn't supported with autoneg off.
		if [[ $speed == 56000 ]]; then
			continue
		fi

		RET=0
		ethtool_set $h1 speed $speed autoneg off

		setup_wait_dev_with_timeout $h1
		setup_wait_dev_with_timeout $h2
		ping_do $h1 192.0.2.2
		check_err $? "h1-speed=$speed autoneg off, h2 autoneg on"
		log_test "one side with autoneg off (speed = $speed) and another with autoneg on"
	done

	ethtool -s $h1 autoneg on
}

subset_of_common_speeds_get()
{
	local dev1=$1; shift
	local dev2=$1; shift

	local -a speeds_arr=($(common_speeds_get $dev1 $dev2 0))
	local speed_to_advertise=0
	local speed_to_remove=${speeds_arr[0]}
	# If speed_to_remove=x we don't want to remove also speeds
	# that start with x000.. so add 'base' to limit the speed.
	speed_to_remove+='base'

	local -a speeds_mode_arr=($(common_speeds_get $dev1 $dev2 1))

	for speed in ${speeds_mode_arr[@]}; do
		if [[ $speed != $speed_to_remove* ]]; then
			speed_to_advertise=$(($speed_to_advertise | \
				${speed_values[$speed]}))
		fi

	done
	# Convert to hex base
	printf "%#x" "$speed_to_advertise"
}

speed_to_advertise_get()
{
	# The function returns the hex number that is appropriate to
	# all modes of the parameter - speed_without_mode
	local speed_without_mode=$1; shift
	local supported_speeds=("$@"); shift
	local speed_to_advertise=0

	# If speed_without_mode=x we don't want to match also speeds
	# that start with x000.. so add 'base' to limit the speed.
	speed_without_mode+='base'
	for speed in ${supported_speeds[@]}; do
		if [[ $speed == $speed_without_mode* ]]; then
			speed_to_advertise=$(($speed_to_advertise | \
				${speed_values[$speed]}))
		fi

	done

	# Convert to hex base
	printf "%#x" "$speed_to_advertise"
}
advertise_subset_of_speeds()
{
	# Test that when one dev advertises a subset of speeds
	# and another advertises a specific speed (but all modes of this speed),
	# the links are up and ping success.
	RET=0

	local speed_1_to_advertise=$(subset_of_common_speeds_get $h1 $h2)
	ethtool_set $h1 advertise $speed_1_to_advertise

	if [ $RET != 0 ]; then
		log_test "advertise subset of speeds"
		return
	fi

	local -a speeds_arr_without_mode=($(common_speeds_get $h1 $h2 0))
	# Check only speeds that h1 advertised. remove the first speed.
	unset speeds_arr_without_mode[0]
	local -a speeds_arr_with_mode=($(common_speeds_get $h1 $h2 1))

	for speed_value in ${speeds_arr_without_mode[@]}; do
		RET=0
		local speed_2_to_advertise=$(speed_to_advertise_get $speed_value \
			"${speeds_arr_with_mode[@]}")
		ethtool_set $h2 advertise $speed_2_to_advertise

		setup_wait_dev_with_timeout $h1
		setup_wait_dev_with_timeout $h2
		ping_do $h1 192.0.2.2
		check_err $? "h1=$speed_1_to_advertise, h2=$speed_2_to_advertise ($speed_value)"

		log_test "advertise subset of speeds (h1=$speed_1_to_advertise, h2=$speed_2_to_advertise)"
	done

	ethtool -s $h2 autoneg on
	ethtool -s $h1 autoneg on
}

check_highest_speed_is_chosen()
{
	# Test that when one dev advertise subset of speeds,
	# the other chooses the highest speed.
	# This test checks configuration without traffic.
	RET=0

	local max_speed
	local chosen_speed
	local speed_to_advertise=$(subset_of_common_speeds_get $h1 $h2)

	ethtool_set $h1 advertise $speed_to_advertise

	if [ $RET != 0 ]; then
		log_test "check highest speed."
		return
	fi

	local -a speeds_arr=($(common_speeds_get $h1 $h2 0))
	# Remove the first speed, h1 does not advertise this speed.
	unset speeds_arr[0]

	max_speed=${speeds_arr[0]}
	for current in ${speeds_arr[@]}; do
		if [[ $current -gt $max_speed ]]; then
			max_speed=$current
		fi
	done

	setup_wait_dev_with_timeout $h1
	setup_wait_dev_with_timeout $h2
	chosen_speed=$(ethtool $h1 | grep 'Speed:')
	chosen_speed=${chosen_speed%"Mb/s"*}
	chosen_speed=${chosen_speed#*"Speed: "}
	((chosen_speed == max_speed))
	check_err $? "h1 advertise $speed_to_advertise, h2 sync to speed $chosen_speed"

	log_test "check highest speed"

	ethtool -s $h2 autoneg on
	ethtool -s $h1 autoneg on
}

different_speeds_autoneg_on()
{
	# Test that when we configure links to advertise different speeds,
	# link aren't up and ping fails.
	RET=0

	local -a speeds=($(different_speeds_get $h1 $h2 1))
	local speed1=${speeds[0]}
	local speed2=${speeds[1]}

	ethtool_set $h1 advertise ${speed_values[$speed1]}
	ethtool_set $h2 advertise ${speed_values[$speed2]}

	if (($RET)); then
		setup_wait_dev_with_timeout $h1
		setup_wait_dev_with_timeout $h2
		ping_do $h1 192.0.2.2
		check_fail $? "ping with different speeds autoneg on"
	fi

	log_test "advertise different speeds autoneg on"

	ethtool -s $h2 autoneg on
	ethtool -s $h1 autoneg on
}

trap cleanup EXIT

setup_prepare
setup_wait

tests_run

exit $EXIT_STATUS
