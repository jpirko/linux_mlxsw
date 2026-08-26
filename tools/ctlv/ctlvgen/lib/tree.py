# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Whole-tree registry, specification and ABI checks, and regeneration."""

from __future__ import annotations

import pathlib
import subprocess
from collections.abc import Collection

from . import compat
from . import outputs
from . import semantics
from . import spec
from .errors import CtlvError


def _stems(directory: pathlib.Path, exclude: tuple[str, ...] = ()) -> set[str]:
    return {path.stem for path in directory.glob('*.yaml')
            if path.name not in exclude}


def coverage_problems(
        paths: spec.Paths, registry: spec.Registry, snapshots: bool = True,
        allowed_missing_snapshots: Collection[str] = ()) -> list[str]:
    expected = {entry.name for entry in registry.entries}
    problems = []

    actual_specs = _stems(paths.spec_dir)
    for name in sorted(expected - actual_specs):
        problems.append(f'missing specification {name}.yaml')
    for name in sorted(actual_specs - expected):
        problems.append(f'unexpected specification {name}.yaml')

    if snapshots:
        actual_abi = _stems(paths.abi_dir,
                            exclude=(paths.registry_abi.name,))
        missing = expected - actual_abi - set(allowed_missing_snapshots)
        for name in sorted(missing):
            problems.append(f'missing ABI snapshot {name}.yaml')
        for name in sorted(actual_abi - expected):
            problems.append(f'unexpected ABI snapshot {name}.yaml')
    return problems


def _relative(paths: spec.Paths,
              path: pathlib.Path) -> pathlib.PurePosixPath:
    return pathlib.PurePosixPath(path.relative_to(paths.kernel_dir).as_posix())


def _families(paths: spec.Paths,
              registry: spec.Registry) -> tuple[semantics.Family, ...]:
    return tuple(semantics.load(spec.Paths(
        paths.spec_dir / f'{entry.name}.yaml', schema=paths.schema,
        registry=paths.registry)) for entry in registry.entries)


def _snapshots(paths: spec.Paths, registry: spec.Registry,
               families: tuple[semantics.Family, ...]
               ) -> dict[pathlib.PurePosixPath, str]:
    texts = {_relative(paths, paths.registry_abi):
             compat.registry_render(registry)}
    for family in families:
        texts[_relative(paths, paths.abi(family.name))] = compat.render(family)
    return texts


def _git(root: pathlib.Path, *args: str) -> subprocess.CompletedProcess[str]:
    return subprocess.run(['git', *args], cwd=root, capture_output=True,
                          text=True, check=False)


def _baseline(root: pathlib.Path, relative: pathlib.PurePosixPath,
              committed: bool) -> tuple[str, str | None]:
    """Where the ABI to keep is read from, and its text, or None for none.

    Regeneration keeps what HEAD has, so that what it wrote but nobody
    committed can still change. A check, or a tree without git, keeps the file.
    """
    if committed and (root / '.git').exists():
        where = f'HEAD:./{relative}'
        found = _git(root, 'rev-parse', '--verify', '--quiet', where)
        if found.returncode == 1:
            return where, None
        if found.returncode == 0:
            found = _git(root, 'cat-file', 'blob', found.stdout.strip())
        if found.returncode:
            raise CtlvError(str(root), found.stderr.strip())
        return where, found.stdout
    path = root.joinpath(*relative.parts)
    if not path.is_file():
        return str(path), None
    return str(path), path.read_text(encoding='utf-8')


def _breaks(paths: spec.Paths, root: pathlib.Path, registry: spec.Registry,
            families: tuple[semantics.Family, ...],
            committed: bool) -> list[str]:
    problems = []
    where, text = _baseline(root, _relative(paths, paths.registry_abi),
                            committed)
    if text is not None:
        baseline = spec.load_registry(paths, where, text)
        problems += [f'registry: {problem}' for problem in
                     compat.registry_problems(baseline, registry)]
    for family in families:
        where, text = _baseline(root, _relative(paths, paths.abi(family.name)),
                                committed)
        if text is not None:
            problems += [f'{family.name}: {problem}' for problem in
                         compat.breaks(family, where, text)]
    return problems


def _root(paths: spec.Paths,
          output_root: pathlib.Path | str | None) -> pathlib.Path:
    return pathlib.Path(paths.kernel_dir if output_root is None
                        else output_root)


def check(paths: spec.Paths,
          output_root: pathlib.Path | str | None = None) -> None:
    """Fail on anything regeneration would change or refuse."""
    registry = spec.load_registry(paths)
    problems = coverage_problems(paths, registry)
    if problems:
        raise CtlvError('CTLV tree', '\n'.join(problems))
    families = _families(paths, registry)
    problems = _breaks(paths, _root(paths, output_root), registry, families,
                       committed=False)
    if problems:
        raise CtlvError('the ABI snapshots', 'the specifications break them' +
                        ''.join(f'\n  {problem}' for problem in problems))
    outputs.reconcile(paths, output_root, check=True,
                      extra=_snapshots(paths, registry, families))


def update(paths: spec.Paths, output_root: pathlib.Path | str | None = None,
           abi_break: bool = False) -> tuple[str, ...]:
    """Regenerate every output and ABI snapshot, and say what changed.

    A change that breaks the committed ABI is refused unless abi_break.
    """
    registry = spec.load_registry(paths)
    problems = coverage_problems(
        paths, registry,
        allowed_missing_snapshots=[entry.name for entry in registry.entries])
    if problems:
        raise CtlvError('CTLV tree', '\n'.join(problems))
    families = _families(paths, registry)
    problems = _breaks(paths, _root(paths, output_root), registry, families,
                       committed=True)
    if problems and not abi_break:
        raise CtlvError('the committed ABI', 'the specifications break it' +
                        ''.join(f'\n  {problem}' for problem in problems) +
                        '\nregen with ABI_BREAK=1 writes it anyway')
    changed = outputs.reconcile(paths, output_root,
                                extra=_snapshots(paths, registry, families))
    return tuple(f'ABI break, {problem}' for problem in problems) + changed
