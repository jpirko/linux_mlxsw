# SPDX-License-Identifier: GPL-2.0
source ../mirror_gre_scale.sh

mirror_gre_get_target()
{
	local should_fail=$1; shift

	if ((! should_fail)); then
		echo 8
	else
		echo 9
	fi
}
