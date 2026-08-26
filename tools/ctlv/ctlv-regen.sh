#!/bin/bash
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#
# Reconcile the checked-in files against the independent output manifests.
#
#   -f  accepted for compatibility; reconciliation always renders everything
#   -c  do not write anything, fail if any output differs from what the
#       generator produces now
#   -p  search the given directory instead of the kernel tree

TOOL=$(dirname "$(realpath "$0")")/ctlvgen/cli.py
check=
search=

while getopts ':fcp:' option; do
	case "$option" in
	f)
		;;
	c)
		check=yes
		;;
	p)
		if [ -z "$OPTARG" ]; then
			echo "Option -p needs a nonempty directory" >&2
			exit 1
		fi
		search=$OPTARG
		;;
	:)
		echo "Option -$OPTARG needs an argument" >&2
		exit 1
		;;
	?)
		echo "Unrecognized option '-$OPTARG'" >&2
		exit 1
		;;
	esac
done
shift $((OPTIND - 1))
if [ "$#" -ne 0 ]; then
	echo "Unexpected argument '$1'" >&2
	exit 1
fi

KDIR=$(realpath "$(dirname "$0")/../..")
args=(--spec "$KDIR/Documentation/ctlv/specs/ctlv.yaml")
if [ -n "$search" ]; then
	args+=(--output-root "$search")
fi
if [ -n "$check" ]; then
	args+=(--outputs-check)
else
	args+=(--outputs-update)
fi
exec "$TOOL" "${args[@]}"
