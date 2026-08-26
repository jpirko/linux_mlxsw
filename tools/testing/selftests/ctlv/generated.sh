#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
#
# The generator against the tree it is checked in with: specifications
# validate, no UAPI snapshot moved unreviewed, no generated file is stale. A
# selftest rather than a build rule so that whatever runs the CTLV tests
# reaches it too. Nothing here speaks to a device.

TESTDIR=$(dirname "$(realpath "$0")")
KDIR=$(realpath "$TESTDIR/../../../..")
TOOLDIR=$KDIR/tools/ctlv

# shellcheck source=../kselftest/ktap_helpers.sh
source "$TESTDIR/../kselftest/ktap_helpers.sh"

skip_all()
{
	ktap_skip_all "$@"
	exit "$KSFT_SKIP"
}

# An installed test without the tree it came from, or an image without what the
# generator imports, is a skip rather than a failure.
require()
{
	[ -d "$TOOLDIR" ] || skip_all "no $TOOLDIR, no tree to check"
	command -v git >/dev/null || skip_all "no git"
	command -v python3 >/dev/null || skip_all "no python3"
	python3 -c 'import yaml' 2>/dev/null || skip_all "no PyYAML"
	python3 -c 'import jsonschema' 2>/dev/null || skip_all "no jsonschema"
	git -C "$KDIR" rev-parse --git-dir >/dev/null 2>&1 ||
		skip_all "not a git tree, cannot tell what is generated"
}

# One make target, reported as one test. What it printed is kept as diagnostics
# when it fails, because for the stale check that output is the diff.
check()
{
	local target=$1 name=$2 output

	if output=$(make -s -C "$TOOLDIR" "$target" 2>&1); then
		ktap_test_pass "$name"
		return
	fi
	while IFS= read -r line; do
		ktap_print_msg "$line"
	done <<< "$output"
	ktap_test_fail "$name"
}

ktap_print_header
require
ktap_set_plan 3

check validate every_specification_validates
check abi-check no_unreviewed_uapi_change
check check-regen no_stale_generated_file

ktap_finished
