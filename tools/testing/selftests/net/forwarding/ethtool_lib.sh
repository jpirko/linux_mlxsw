#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

declare -A speed_values

speed_values=(	[10baseT/Half]=0x001
		[10baseT/Full]=0x002
		[100baseT/Half]=0x004
		[100baseT/Full]=0x008
		[1000baseT/Half]=0x010
		[1000baseT/Full]=0x020
		[1000baseKX/Full]=0x20000
		[1000baseX/Full]=0x20000000000
		[2500baseT/Full]=0x800000000000
		[2500baseX/Full]=0x8000
		[5000baseT/Full]=0x1000000000000
		[10000baseT/Full]=0x1000
		[10000baseKX4/Full]=0x40000
		[10000baseKR/Full]=0x80000
		[10000baseCR/Full]=0x40000000000
		[10000baseSR/Full]=0x80000000000
		[10000baseLR/Full]=0x100000000000
		[10000baseLRM/Full]=0x200000000000
		[10000baseER/Full]=0x400000000000
		[20000baseMLD2/Full]=0x200000
		[20000baseKR2/Full]=0x400000
		[25000baseCR/Full]=0x80000000
		[25000baseKR/Full]=0x100000000
		[25000baseSR/Full]=0x200000000
		[40000baseKR4/Full]=0x800000
		[40000baseCR4/Full]=0x1000000
		[40000baseSR4/Full]=0x2000000
		[40000baseLR4/Full]=0x4000000
		[50000baseCR2/Full]=0x400000000
		[40000baseSR4/Full]=0x2000000
		[40000baseLR4/Full]=0x4000000
		[50000baseCR2/Full]=0x400000000
		[50000baseKR2/Full]=0x800000000
		[50000baseSR2/Full]=0x10000000000
		[56000baseKR4/Full]=0x8000000
		[56000baseCR4/Full]=0x10000000
		[56000baseSR4/Full]=0x20000000
		[56000baseLR4/Full]=0x40000000
		[100000baseKR4/Full]=0x1000000000
		[100000baseSR4/Full]=0x2000000000
		[100000baseCR4/Full]=0x4000000000
		[100000baseLR4_ER4/Full]=0x8000000000)

ethtool_set()
{
	local cmd="$@"
	local out=$(ethtool -s $cmd 2>&1 | wc -l)
	check_err $out "error in configuration. $cmd"
}

speeds_get()
{
	local dev=$1; shift
	local with_mode=$1; shift

	local speeds_str=$(ethtool "$dev" | \
		# Snip everything before the link modes section.
		sed -n '/Supported link modes:/,$p' | \
		# Quit processing the rest at the start of the next section.
		# When checking, skip the header of this section (hence the 2,).
		sed -n '2,${/^[\t][^ \t]/q};p' | \
		# Drop the section header of the current section.
		cut -d':' -f2)

	local -a speeds_arr=($speeds_str)
	if [[ $with_mode -eq 0 ]]; then
		for ((i=0; i<${#speeds_arr[@]}; i++)); do
			speeds_arr[$i]=${speeds_arr[$i]%base*}
		done
	fi
	echo ${speeds_arr[@]}
}

common_speeds_get()
{
	dev1=$1; shift
	dev2=$1; shift
	with_mode=$1; shift

	local -a dev1_speeds=($(speeds_get $dev1 $with_mode))
	local -a dev2_speeds=($(speeds_get $dev2 $with_mode))

	comm -12 \
		<(printf '%s\n' "${dev1_speeds[@]}" | sort -u) \
		<(printf '%s\n' "${dev2_speeds[@]}" | sort -u)
}
