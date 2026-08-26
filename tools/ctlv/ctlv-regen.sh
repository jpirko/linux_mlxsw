#!/bin/bash
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#
# Regenerate the checked-in CTLV generated files. Every generated file names
# its own specification and generation mode, so the parameters are recovered
# from the file itself.
#
#   -f  regenerate even when the output is newer than its specification
#   -c  do not write anything, fail if any output differs from what the
#       generator produces now
#   -p  search the given directory instead of the kernel tree

TOOL=$(dirname $(realpath $0))/ctlvgen/cli.py

# The one path that cannot be derived from the specification being generated:
# one meta-schema serves the whole tree.
SCHEMA=Documentation/ctlv/ctlv-schema.yaml

force=
check=
search=

while [ ! -z "$1" ]; do
  case "$1" in
    -f ) force=yes; shift ;;
    -c ) check=yes; shift ;;
    -p ) search=$2; shift 2 ;;
    * )  echo "Unrecognized option '$1'"; exit 1 ;;
  esac
done

KDIR=$(dirname $(dirname $(dirname $(realpath $0))))
pushd ${search:-$KDIR} >>/dev/null

rc=0

# --untracked so that a newly added generated file is not silently skipped.
# The pathspec keeps documentation that quotes the marker out of the way. A
# generated page carries the marker in its own comment form, so either is read.
MARKER='CTLV-GEN \(kernel\|family\|uapi\|user\|doc\)'
files=$(git grep --untracked --files-with-matches \
	"^\(/\*\|\.\.\) $MARKER" -- '*.c' '*.h' '*.rst')
for f in $files; do
    # params:     0        1       2      3
    #         $YAML CTLV-GEN kernel $mode
    params=( $(git grep --untracked --no-line-number -B1 -h \
	       "\(/\*\|\.\.\) CTLV-GEN" $f | \
	       sed -e 's@/\*\(.*\)\*/@\1@' -e 's@^\.\.[ \t]*@@') )

    if [ -n "$check" ]; then
	tmp=$(mktemp)
	$TOOL --mode ${params[2]} --${params[3]} \
	      --spec $KDIR/${params[0]} --schema $KDIR/$SCHEMA -o $tmp || exit 1
	if ! diff -u $f $tmp ; then
	    echo -e "\tSTALE $f"
	    rc=1
	fi
	rm -f $tmp
	continue
    fi

    if [ $f -nt ${params[0]} -a -z "$force" ]; then
	echo -e "\tSKIP $f"
	continue
    fi

    echo -e "\tGEN ${params[2]}\t$f"
    $TOOL --cmp-out --mode ${params[2]} --${params[3]} \
	  --spec $KDIR/${params[0]} --schema $KDIR/$SCHEMA -o $f || exit 1
done

popd >>/dev/null
exit $rc
