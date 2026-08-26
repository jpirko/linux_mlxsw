#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# shellcheck disable=SC2317,SC2329

TESTDIR=$(dirname "$(realpath "$0")")
KDIR=$(realpath "$TESTDIR/../../../..")
TOOLDIR=$KDIR/tools/ctlv
CTLVCTL=$TOOLDIR/ctlvctl/ctlvctl
SPECS=$KDIR/Documentation/ctlv/specs
DEVICE_ADD=/sys/kernel/debug/ctlv-test/device_add
DEVICE_REMOVE=/sys/kernel/debug/ctlv-test/device_remove
# The C harness confines itself to slots 0 through 6.
INSTANCE=7

# shellcheck source=tools/testing/selftests/kselftest/ktap_helpers.sh
source "$TESTDIR/../kselftest/ktap_helpers.sh"

skip_all()
{
	ktap_skip_all "$@"
	exit "$KSFT_SKIP"
}

require()
{
	[ -d "$TOOLDIR" ] || skip_all "no CTLV source tree"
	command -v make >/dev/null || skip_all "no make"
	command -v python3 >/dev/null || skip_all "no python3"
	command -v pkg-config >/dev/null || skip_all "no pkg-config"
	command -v timeout >/dev/null || skip_all "no timeout"
	pkg-config --atleast-version=0.14 json-c ||
		skip_all "no json-c 0.14 or newer"
	python3 -c 'import yaml, jsonschema' 2>/dev/null ||
		skip_all "no ctlvgen Python dependencies"

	if [ ! -d /sys/module/ctlv_test ]; then
		modprobe ctlv-test 2>/dev/null ||
			insmod "$KDIR/drivers/ctlv/ctlv-test.ko" 2>/dev/null ||
			skip_all "no ctlv-test module"
	fi
	[ -e "$DEVICE_ADD" ] ||
		mount -t debugfs none /sys/kernel/debug 2>/dev/null
	[ -w "$DEVICE_ADD" ] && [ -w "$DEVICE_REMOVE" ] ||
		skip_all "no writable CTLV test debugfs"
}

build()
{
	local output

	if output=$(make -s -C "$TOOLDIR" programs 2>&1); then
		return 0
	fi
	while IFS= read -r line; do
		ktap_print_msg "$line"
	done <<< "$output"
	return 1
}

device_get()
{
	"$CTLVCTL" list |
		awk -F'	' '$2 == "ctlv/ctlv-test" { print $1; exit }'
}

device_add()
{
	echo "$INSTANCE 0" > "$DEVICE_ADD" || return
	CREATED=1
}

cleanup()
{
	[ -z "$CREATED" ] || echo "$INSTANCE" > "$DEVICE_REMOVE"
}

names_test()
{
	local output

	output=$("$CTLVCTL" --spec-dir "$SPECS" ops "$DEV") || return
	grep -q '^probe-get' <<< "$output" || return
	if grep -q '0x0003000000000002' <<< "$output"; then
		return 1
	fi
}

ids_test()
{
	"$CTLVCTL" --spec-dir "$SPECS" -n ops "$DEV" |
		grep -q 'probe-get.*0x0003000000000002'
}

json_test()
{
	"$CTLVCTL" --spec-dir "$SPECS" --output-json \
		query "$DEV" probe-get token=7 |
		python3 -c 'import json, sys
reply = json.load(sys.stdin)
assert reply["op"] == "probe-get"
assert reply["reply"]["probe.token"] == 7
'
}

recursive_test()
{
	timeout 90 "$CTLVCTL" --spec-dir "$SPECS" --output-json \
		query "$DEV" op-schema-get op.id=0x0003000000000008 |
		python3 -c 'import json, sys
reply = json.load(sys.stdin)
assert "schema.request" in reply["reply"]
'
}

kind_test()
{
	local output

	if output=$("$CTLVCTL" --spec-dir "$SPECS" \
		    exec "$DEV" series-dump 2>&1); then
		return 1
	fi
	grep -q 'is a query; use "query"' <<< "$output"
}

report()
{
	local name=$1

	shift
	if "$@"; then
		ktap_test_pass "$name"
	else
		ktap_test_fail "$name"
	fi
}

CREATED=
DEV=

require
ktap_print_header
ktap_set_plan 6

report ctlvctl_builds build
if [ ! -x "$CTLVCTL" ]; then
	for name in names ids json recursive kind; do
		ktap_test_fail "ctlvctl_${name}"
	done
	ktap_finished
	exit "$KSFT_FAIL"
fi

DEV=$(device_get)
if [ -z "$DEV" ]; then
	device_add
	DEV=$(device_get)
fi
trap cleanup EXIT

report ctlvctl_names_operations names_test
report ctlvctl_shows_ids_on_request ids_test
report ctlvctl_builds_and_prints_json json_test
report ctlvctl_prints_recursive_replies recursive_test
report ctlvctl_rejects_the_wrong_command kind_test

ktap_finished
