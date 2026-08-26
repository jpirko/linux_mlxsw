#!/bin/bash
# SPDX-License-Identifier: GPL-2.0
# shellcheck disable=SC2317,SC2329

TESTDIR=$(dirname "$(realpath "$0")")
KDIR=$(realpath "$TESTDIR/../../../..")
CTLVCTL=$TESTDIR/ctlvctl
# The build writes each family's manifest beside ctlvctl.
SPECS=$TESTDIR
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
	[ -x "$CTLVCTL" ] || skip_all "no ctlvctl"
	if "$CTLVCTL" --output-json 2>&1 | grep -q 'require json-c'; then
		skip_all "ctlvctl built without json-c 0.14 or newer"
	fi
	command -v timeout >/dev/null || skip_all "no timeout"

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
	local output

	output=$("$CTLVCTL" --spec-dir "$SPECS" --output-json \
		 exec "$DEV" probe-get token=7) || return
	grep -q '"op":"probe-get"' <<< "$output" &&
		grep -q '"reply":{.*"token-info.token":7[,}]' <<< "$output"
}

recursive_test()
{
	local output

	output=$(timeout 90 "$CTLVCTL" --spec-dir "$SPECS" --output-json \
		 exec "$DEV" op-schema-get id=0x0003000000000008) || return
	grep -q '"reply":{.*"op-schema-get request":' <<< "$output"
}

blob_test()
{
	local dir output ret=1

	dir=$(mktemp -d) || return
	printf abc > "$dir/source"
	printf xyz > "$dir/scratch"
	cat > "$dir/request" <<-EOF
	{"source": {"file": "$dir/source"},
	 "target": {"file": "$dir/target", "length": 5},
	 "scratch": {"file": "$dir/scratch"},
	 "fill": 7}
	EOF
	# FNV-1a of abc and then xyz, and the fill over all eight bytes.
	if output=$("$CTLVCTL" --spec-dir "$SPECS" --output-json \
		    exec "$DEV" transfer --json "@$dir/request") &&
	   grep -q '"transfer sum":18136983334979290878[,}]' <<< "$output" &&
	   grep -q '"transfer written":8[,}]' <<< "$output" &&
	   cmp -s "$dir/target" <(printf '\a\a\a\a\a') &&
	   cmp -s "$dir/scratch" <(printf '\a\a\a'); then
		ret=0
	fi
	rm -r "$dir"
	return "$ret"
}

op_type_test()
{
	local output

	"$CTLVCTL" --spec-dir "$SPECS" exec "$DEV" series-get totals=1 \
		>/dev/null || return
	if output=$("$CTLVCTL" --spec-dir "$SPECS" \
		    exec "$DEV" pinged 2>&1); then
		return 1
	fi
	grep -q 'is an event; use "monitor"' <<< "$output"
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
report ctlvctl_moves_blobs_through_files blob_test
report ctlvctl_execs_queries_but_not_events op_type_test

ktap_finished
