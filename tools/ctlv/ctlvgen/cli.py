#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Command line front end of the CTLV schema tool."""

import argparse
import pathlib
import sys

# So that this file works both as a module of the ctlvgen package and as the
# script the Makefile and ctlv-regen.sh run. The imports below have to follow.
sys.path.append(pathlib.Path(__file__).resolve().parent.as_posix())

from lib import cgen  # noqa: E402
from lib import compat  # noqa: E402
from lib import dump  # noqa: E402
from lib import gen_doc  # noqa: E402
from lib import gen_kernel  # noqa: E402
from lib import gen_runtime  # noqa: E402
from lib import gen_uapi  # noqa: E402
from lib import gen_user  # noqa: E402
from lib import outputs  # noqa: E402
from lib import semantics  # noqa: E402
from lib import spec  # noqa: E402
from lib import tree  # noqa: E402
from lib.errors import CtlvError  # noqa: E402


# What each non-kernel --mode and file kind generates.
GENERATORS = {
    ('uapi', 'header'): gen_uapi.render,
    ('user', 'header'): gen_user.render_header,
    ('user', 'source'): gen_user.render_source,
    ('doc', 'rst'): gen_doc.render,
}


def main():
    parser = argparse.ArgumentParser(description='CTLV schema tool')
    parser.add_argument('--spec', type=str, required=True,
                        help='path to a CTLV family specification')
    parser.add_argument('--schema', type=str,
                        help='path to the meta-schema, derived from --spec '
                             'when not given')
    parser.add_argument('--registry', type=str,
                        help='path to the family registry, derived from '
                             '--spec when not given')
    parser.add_argument('--abi', type=str,
                        help='directory of the ABI snapshots, derived from '
                             '--spec when not given')
    action = parser.add_mutually_exclusive_group(required=True)
    action.add_argument('--validate', action='store_true',
                        help='validate the specification and exit')
    action.add_argument('--dump', action='store_true',
                        help='print the resolved schema in text form')
    action.add_argument('--runtime-json', action='store_true',
                        help='print the resolved runtime manifest as JSON')
    action.add_argument('--abi-check', action='store_true',
                        help='check the specification against its ABI '
                             'snapshot')
    action.add_argument('--abi-update', action='store_true',
                        help='rewrite the ABI snapshot from the '
                             'specification')
    action.add_argument('--tree-check', action='store_true',
                        help='check registry, spec, snapshot and output '
                             'coverage')
    action.add_argument('--tree-check-update', action='store_true',
                        help='check whole-tree inputs before an ABI update')
    action.add_argument('--registry-abi-update', action='store_true',
                        help='update the ordered family registry baseline')
    action.add_argument('--outputs-check', action='store_true',
                        help='check every file in the output manifests')
    action.add_argument('--outputs-update', action='store_true',
                        help='regenerate every file in the output manifests')
    action.add_argument('--mode',
                        choices=('uapi', 'kernel', 'user', 'doc'),
                        help='generate the files of this mode')
    parser.add_argument('--schema-only', action='store_true',
                        help='omit the family API from kernel output')
    kind = parser.add_mutually_exclusive_group()
    kind.add_argument('--header', action='store_true',
                      help='generate the header of --mode')
    kind.add_argument('--source', action='store_true',
                      help='generate the source of --mode')
    kind.add_argument('--rst', action='store_true',
                      help='generate the reStructuredText of --mode')
    parser.add_argument('-o', '--output', type=str,
                        help='write to this file instead of standard output')
    parser.add_argument('--cmp-out', action='store_true',
                        help='leave the output file alone when its contents '
                             'would not change')
    parser.add_argument('--output-root', type=str,
                        help='alternate destination root for generated files')
    args = parser.parse_args()

    kinds = (args.header, args.source, args.rst)
    if args.mode and not any(kinds):
        parser.error('--mode needs --header, --source or --rst')
    if any(kinds) and not args.mode:
        parser.error('--header, --source and --rst need --mode')
    if args.mode == 'kernel' and args.rst:
        parser.error('--mode kernel needs --header or --source')
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

    paths = spec.Paths(args.spec, schema=args.schema, registry=args.registry,
                       abi=args.abi)
    try:
        if args.tree_check:
            tree.check(paths)
            return 0
        elif args.tree_check_update:
            tree.check_for_update(paths)
            return 0
        elif args.registry_abi_update:
            registry = tree.finish_update(paths)
            paths.registry_abi.write_text(
                compat.registry_render(registry), encoding='utf-8')
            print(f'ctlvgen: wrote {paths.registry_abi}', file=sys.stderr)
            return 0
        elif args.outputs_check:
            tree.check(paths)
            outputs.reconcile(paths, args.output_root, check=True)
            return 0
        elif args.outputs_update:
            tree.check_for_update(paths)
            changed = outputs.reconcile(paths, args.output_root)
            for item in changed:
                print(f'ctlvgen: {item}', file=sys.stderr)
            return 0
        family = semantics.load(paths)
        if args.validate:
            cgen.symbols(family)
            # The root family has no binding: libctlv is that binding.
            if family.name != spec.CORE_FAMILY_NAME:
                cgen.user_symbols(family)
        elif args.dump:
            sys.stdout.write(dump.render(family))
        elif args.runtime_json:
            sys.stdout.write(gen_runtime.render(family))
        elif args.abi_check:
            compat.registry_check(paths, spec.load_registry(paths))
            compat.check(family, paths.abi(family.name))
        elif args.abi_update:
            _abi_update(paths, family)
        else:
            _generate(args, family)
    except CtlvError as exc:
        print(f'ctlvgen: {exc}', file=sys.stderr)
        return 1
    return 0


def _abi_update(paths, family):
    path = paths.abi(family.name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(compat.render(family), encoding='utf-8')
    print(f'ctlvgen: wrote {path}', file=sys.stderr)


def _generate(args, family):
    if args.header:
        kind = 'header'
    elif args.source:
        kind = 'source'
    else:
        kind = 'rst'
    if args.mode == 'kernel':
        renderer = (gen_kernel.render_header if args.header
                    else gen_kernel.render_source)
        text = renderer(family, family_api=not args.schema_only)
    else:
        text = GENERATORS[(args.mode, kind)](family)
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
