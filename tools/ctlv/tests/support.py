# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Paths and helpers shared by the CTLV schema tool's tests.

A registered family under Documentation/ctlv/ has a permanent family ID, a
wire ABI and so a checked-in snapshot. Everything else is a fixture: specs/
exercises the schema language, bad-specs/ must be rejected, both draw their
family IDs from the fixture registry beside them, and neither has an ABI to
snapshot.
"""

import pathlib

from ctlvgen.lib import cgen
from ctlvgen.lib import semantics
from ctlvgen.lib import spec

TESTS_DIR = pathlib.Path(__file__).resolve().parent
TOOL_DIR = TESTS_DIR.parent
KERNEL_DIR = TOOL_DIR.parents[1]

DOC_DIR = KERNEL_DIR / 'Documentation' / 'ctlv'
SPEC_DIR = DOC_DIR / 'specs'
ABI_DIR = DOC_DIR / 'abi'
SCHEMA = DOC_DIR / 'ctlv-schema.yaml'
UAPI_HEADER = KERNEL_DIR / 'include' / 'uapi' / 'linux' / 'ctlv.h'
CORE_SOURCE = KERNEL_DIR / 'drivers' / 'ctlv' / 'core.c'

FIXTURE_SPEC_DIR = TESTS_DIR / 'specs'
BAD_SPEC_DIR = TESTS_DIR / 'bad-specs'
FIXTURE_REGISTRY = TESTS_DIR / 'families.yaml'

# Every registered family, in registry order.
FAMILIES = ('ctlv', 'ctlv-attestation', 'ctlv-attestation-tdx',
            'ctlv-test', 'ctlv-test-child', 'ctlv-attestation-sim')

# The fixture families, in fixture registry order: a parent holding one of
# every shape the language allows, and a child that inherits it.
FIXTURE_FAMILIES = ('ctlv-fixture', 'ctlv-fixture-child')

ALL_FAMILIES = FAMILIES + FIXTURE_FAMILIES


def load(name):
    """Load one registered family."""
    return semantics.load(spec.Paths(SPEC_DIR / f'{name}.yaml'))


def load_family(name):
    """Load one fixture family."""
    return load_fixture(FIXTURE_SPEC_DIR / f'{name}.yaml')


def load_any(name):
    """Load one family, registered or fixture, by name."""
    if name in FAMILIES:
        return load(name)
    return load_family(name)


def load_fixture(path):
    """Load a specification from the test corpus."""
    return semantics.load(spec.Paths(path, schema=SCHEMA))


def validate_fixture(path):
    """Load a fixture and name its declarations, as --validate does.

    Two declarations spelling the same C name is caught by the generators, so a
    specification is only fully checked once its names have been built.
    """
    family = load_fixture(path)
    cgen.symbols(family)
    return family


def op(family, name):
    """Return one op of a family by name."""
    for candidate in family.ops:
        if candidate.name == name:
            return candidate
    raise KeyError(f'{family.name} has no op {name}')


def placement(node, full_name):
    """Return one child placement of a node by attr name."""
    for child in node.children():
        if child.attr.full_name == full_name:
            return child
    raise KeyError(f'no placement of {full_name}')
