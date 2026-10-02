# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Paths and helpers shared by the CTLV schema tool's tests.

A registered family under Documentation/ctlv/ has a permanent family ID, an
ABI and so a checked-in snapshot. Everything else is a fixture: specs/
exercises the schema language, bad-specs/ must be rejected, both draw their
family IDs from the fixture registry beside them, and neither has an ABI to
snapshot.
"""

from __future__ import annotations

import pathlib

from ctlvgen.lib import cgen
from ctlvgen.lib import semantics
from ctlvgen.lib import spec

TESTS_DIR: pathlib.Path = pathlib.Path(__file__).resolve().parent
TOOL_DIR: pathlib.Path = TESTS_DIR.parent
KERNEL_DIR: pathlib.Path = TOOL_DIR.parents[1]

DOC_DIR: pathlib.Path = KERNEL_DIR / 'Documentation' / 'ctlv'
SPEC_DIR: pathlib.Path = DOC_DIR / 'specs'
ABI_DIR: pathlib.Path = DOC_DIR / 'abi'
SCHEMA: pathlib.Path = DOC_DIR / 'ctlv-schema.yaml'
UAPI_HEADER: pathlib.Path = KERNEL_DIR / 'include' / 'uapi' / 'ctlv' / 'ctlv.h'
FAMILY_HEADER: pathlib.Path = (KERNEL_DIR / 'include' / 'linux' /
                               'ctlv_family.h')
CORE_SOURCE: pathlib.Path = KERNEL_DIR / 'lib' / 'ctlv' / 'core.c'

FIXTURE_SPEC_DIR: pathlib.Path = TESTS_DIR / 'specs'
BAD_SPEC_DIR: pathlib.Path = TESTS_DIR / 'bad-specs'
FIXTURE_REGISTRY: pathlib.Path = TESTS_DIR / 'families.yaml'

# Every registered family, in registry order.
FAMILIES: tuple[str, ...] = ('ctlv', 'ctlv-attestation',
                             'ctlv-attestation-tdx', 'ctlv-test',
                             'ctlv-test-child', 'ctlv-attestation-sim')

# The fixture families, in fixture registry order: a parent holding one of
# every shape the language allows, and a child that inherits it.
FIXTURE_FAMILIES: tuple[str, ...] = ('ctlv-fixture', 'ctlv-fixture-child')

ALL_FAMILIES: tuple[str, ...] = FAMILIES + FIXTURE_FAMILIES


def load(name: str) -> semantics.Family:
    """Load one registered family."""
    return semantics.load(spec.Paths(SPEC_DIR / f'{name}.yaml'))


def load_family(name: str) -> semantics.Family:
    """Load one fixture family."""
    return load_fixture(FIXTURE_SPEC_DIR / f'{name}.yaml')


def load_any(name: str) -> semantics.Family:
    """Load one family, registered or fixture, by name."""
    if name in FAMILIES:
        return load(name)
    return load_family(name)


def load_fixture(path: pathlib.Path) -> semantics.Family:
    """Load a specification from the test corpus."""
    return semantics.load(spec.Paths(path, schema=SCHEMA))


def validate_fixture(path: pathlib.Path) -> semantics.Family:
    """Load a fixture and name its declarations, as --validate does.

    Two declarations spelling the same C name is caught by the generators, so a
    specification is only fully checked once its names have been built.
    """
    family = load_fixture(path)
    cgen.symbols(family)
    return family


def op(family: semantics.Family, name: str) -> semantics.Op:
    """Return one op of a family by name."""
    for candidate in family.ops:
        if candidate.name == name:
            return candidate
    raise KeyError(f'{family.name} has no op {name}')


def request(family: semantics.Family, name: str) -> semantics.Node:
    """Return the request tree of one op, which has to have one."""
    tree = op(family, name).request
    assert tree is not None
    return tree


def reply(family: semantics.Family, name: str) -> semantics.Node:
    """Return the reply tree of one op, which has to have one."""
    tree = op(family, name).reply
    assert tree is not None
    return tree


def child_of(node: semantics.Node, full_name: str) -> semantics.Node:
    """Return one child of a node by the full name of its field."""
    for child in node.children():
        if child.child_field.full_name == full_name:
            return child
    raise KeyError(f'no child {full_name}')
