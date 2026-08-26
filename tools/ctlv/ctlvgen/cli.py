#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Command line front end of the CTLV schema tool."""

from __future__ import annotations

import argparse
import pathlib
import sys
from collections.abc import Callable

# So that this file works both as a module of the ctlvgen package and as the
# script the Makefile and ctlv-regen.sh run. The imports below have to follow.
sys.path.append(pathlib.Path(__file__).resolve().parents[1].as_posix())

from ctlvgen.lib import cgen  # noqa: E402
from ctlvgen.lib import gen_kernel  # noqa: E402
from ctlvgen.lib import gen_uapi  # noqa: E402
from ctlvgen.lib import semantics  # noqa: E402
from ctlvgen.lib import spec  # noqa: E402
from ctlvgen.lib import tree  # noqa: E402
from ctlvgen.lib.errors import CtlvError  # noqa: E402


# What each --mode and file generates.
GENERATORS: dict[tuple[str, str], Callable[..., str]] = {
    ('uapi', 'header'): gen_uapi.render,
    ('kernel', 'header'): gen_kernel.render_header,
    ('kernel', 'source'): gen_kernel.render_source,
}


def main() -> int:
    parser = argparse.ArgumentParser(description='CTLV schema tool')
    parser.add_argument('--spec', type=str, required=True,
                        help='path to a CTLV family specification')
    parser.add_argument('--schema', type=str,
                        help='path to the meta-schema, derived from --spec '
                             'when not given')
    parser.add_argument('--registry', type=str,
                        help='path to the family registry, derived from '
                             '--spec when not given')
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--validate', action='store_true',
                        help='validate the specification and exit')
    action.add_argument('--outputs-check', action='store_true',
                        help='check every file in the output manifests and '
                             'every ABI snapshot')
    action.add_argument('--outputs-update', action='store_true',
                        help='regenerate every file in the output manifests '
                             'and every ABI snapshot')
    action.add_argument('--mode',
                        choices=('uapi', 'kernel'),
                        help='generate the files of this mode')
    parser.add_argument('--schema-only', action='store_true',
                        help='omit the family API from kernel output')
    files = parser.add_mutually_exclusive_group()
    files.add_argument('--header', action='store_true',
                       help='generate the header of --mode')
    files.add_argument('--source', action='store_true',
                       help='generate the source of --mode')
    parser.add_argument('-o', '--output', type=str,
                        help='write to this file instead of standard output')
    parser.add_argument('--cmp-out', action='store_true',
                        help='leave the output file alone when its contents '
                             'would not change')
    parser.add_argument('--output-root', type=str,
                        help='alternate destination root for generated files')
    parser.add_argument('--abi-break', action='store_true',
                        help='let --outputs-update break the committed ABI')
    args = parser.parse_args()

    file = next((name for name in ('header', 'source')
                 if getattr(args, name)), None)
    if args.mode and not file:
        parser.error('--mode needs --header or --source')
    if file and not args.mode:
        parser.error('--header and --source need --mode')
    if args.mode and (args.mode, file) not in GENERATORS:
        parser.error(f'--mode {args.mode} cannot generate --{file}')
    if args.schema_only and args.mode != 'kernel':
        parser.error('--schema-only needs --mode kernel')
    output_action = args.outputs_check or args.outputs_update
    if args.output is not None and not args.mode:
        parser.error('-o/--output needs --mode')
    if args.cmp_out and (not args.mode or args.output is None):
        parser.error('--cmp-out needs --mode and --output')
    if args.output_root is not None:
        if not output_action:
            parser.error('--output-root needs an outputs action')
        if not args.output_root:
            parser.error('--output-root cannot be empty')
    if args.abi_break and not args.outputs_update:
        parser.error('--abi-break needs --outputs-update')

    paths = spec.Paths(args.spec, schema=args.schema, registry=args.registry)
    try:
        if args.outputs_check:
            tree.check(paths, args.output_root)
            return 0
        elif args.outputs_update:
            for item in tree.update(paths, args.output_root, args.abi_break):
                print(f'ctlvgen: {item}', file=sys.stderr)
            return 0
        family = semantics.load(paths)
        if args.validate:
            cgen.symbols(family)
        else:
            assert file is not None
            _generate(args, file, family)
    except CtlvError as exc:
        print(f'ctlvgen: {exc}', file=sys.stderr)
        return 1
    return 0


def _generate(args: argparse.Namespace, file: str,
              family: semantics.Family) -> None:
    render = GENERATORS[(args.mode, file)]
    if args.mode == 'kernel':
        text = render(family, family_api=not args.schema_only)
    else:
        text = render(family)
    if args.output is None:
        sys.stdout.write(text)
        return
    path = pathlib.Path(args.output)
    # Leaving an unchanged file untouched keeps its timestamp, which is what
    # ctlv-regen.sh compares against the specification.
    if args.cmp_out and path.exists() and \
            path.read_text(encoding='utf-8') == text:
        return
    path.write_text(text, encoding='utf-8')


if __name__ == '__main__':
    sys.exit(main())
