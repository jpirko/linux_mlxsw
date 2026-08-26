#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# Reports the ctlv_compat32 leg either way: a machine without a 32-bit
# toolchain is the common case, and a run that quietly omitted the compat path
# would read like a run that passed it.

TESTDIR=$(dirname "$(realpath "$0")")
BIN=$TESTDIR/ctlv_compat32

# shellcheck source=../kselftest/ktap_helpers.sh
source "$TESTDIR/../kselftest/ktap_helpers.sh"

ktap_print_header
ktap_set_plan 1

if [ ! -x "$BIN" ]; then
	ktap_print_msg "no 32-bit build: the toolchain here makes none"
	ktap_test_skip "a_32_bit_caller_sees_the_same_device"
	ktap_finished
fi

output=$("$BIN" 2>&1)
status=$?

while IFS= read -r line; do
	ktap_print_msg "$line"
done <<< "$output"

case $status in
0)	ktap_test_pass "a_32_bit_caller_sees_the_same_device" ;;
"$KSFT_SKIP")
	ktap_test_skip "a_32_bit_caller_sees_the_same_device" ;;
*)	ktap_test_fail "a_32_bit_caller_sees_the_same_device" ;;
esac

ktap_finished
