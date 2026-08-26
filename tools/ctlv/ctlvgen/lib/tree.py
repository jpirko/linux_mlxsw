# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Whole-tree registry, specification and ABI coverage checks."""

from __future__ import annotations

from . import compat
from . import outputs
from . import spec
from .errors import CtlvError


def _stems(directory, exclude=()):
    return {path.stem for path in directory.glob('*.yaml')
            if path.name not in exclude}


def coverage_problems(paths, registry, snapshots=True,
                      allowed_missing_snapshots=()):
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


def check(paths):
    """Check all inputs whose expected set comes from the registry."""
    registry = spec.load_registry(paths)
    compat.registry_check(paths, registry)
    problems = coverage_problems(paths, registry)
    if problems:
        raise CtlvError('CTLV tree', '\n'.join(problems))
    outputs.load(paths)
    return registry


def check_for_update(paths):
    """Validate a registry update without requiring new snapshots yet."""
    registry = spec.load_registry(paths)
    compat.registry_check(paths, registry, exact=False)
    baseline = spec.load_registry(paths, paths.registry_abi)
    appended = {entry.name
                for entry in registry.entries[len(baseline.entries):]}
    problems = coverage_problems(paths, registry,
                                 allowed_missing_snapshots=appended)
    if problems:
        raise CtlvError('CTLV tree', '\n'.join(problems))
    outputs.load(paths)
    return registry


def finish_update(paths):
    """Require complete snapshots before advancing the registry baseline."""
    registry = check_for_update(paths)
    problems = coverage_problems(paths, registry)
    if problems:
        raise CtlvError('CTLV tree', '\n'.join(problems))
    return registry
