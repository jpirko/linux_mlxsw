# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The as-written form of a CTLV family specification.

The strict YAML subset, the meta-schema, and every rule that one declaration
decides on its own. Comparing declarations and walking a chain are
semantics.py's. Omitted values are normalized here, once.
"""

from __future__ import annotations

import dataclasses
import pathlib
import re

import jsonschema

from . import yamlload
from .errors import CtlvError

# A generated file carries the license of its specification's first line.
LICENSE = re.compile(r'#\s*SPDX-License-Identifier:\s*(\S.*?)\s*$')

# The registry is indexed by family ID, so the root family is its first entry.
CORE_FAMILY_NAME = 'ctlv'
CORE_FAMILY_ID = 0

MAX_FAMILY_ID = 65535

# Zero means unlimited, as the introspection wire form has it.
UNLIMITED = 0

U64_MAX = (1 << 64) - 1
S64_MIN = -(1 << 63)
S64_MAX = (1 << 63) - 1

PRIMARY_KEYS = ('type', 'enum', 'flags', 'attrs', 'blob')

INTEGER_KINDS = ('uint', 'sint')
SCALAR_KINDS = ('uint', 'sint', 'bool', 'string', 'binary', 'enum', 'flags')

# Every op has one of these, and only an action or a query may be dynamic.
AVAILABILITIES = ('fixed', 'dynamic')
DEFAULT_AVAILABILITY = 'fixed'

# Keys each primary form may add, beyond its own, name and max-occurrences.
_ACCESSORY_KEYS = {
    'uint': {'min', 'max', 'unit', 'scale'},
    'sint': {'min', 'max', 'unit', 'scale'},
    'bool': set(),
    'string': {'min-length', 'max-length'},
    'binary': {'length', 'min-length', 'max-length'},
    'enum': set(),
    'flags': set(),
    'nested': {'optional-attrs', 'recursive'},
    'blob': set(),
}
_COMMON_KEYS = {'name', 'max-occurrences'}


@dataclasses.dataclass(frozen=True)
class ValueList:
    """An enum or a flags definition."""

    name: str
    values: tuple


@dataclasses.dataclass(frozen=True)
class BlobDecl:
    """The blob properties of one blob attr declaration."""

    direction: str
    backings: tuple
    max_length: int
    alignment: int


@dataclasses.dataclass(frozen=True)
class AttrDecl:
    """One attr declaration inside an attr group.

    kind is the resolved primary form, and a field that does not apply stays
    None. recursive marks a nest whose attr set may select this attr again.
    """

    name: str
    kind: str
    ref: str = None
    attrs: tuple = ()
    optional_attrs: tuple = ()
    recursive: bool = False
    blob: BlobDecl = None
    min: int = None
    max: int = None
    min_length: int = None
    max_length: int = None
    unit: str = None
    scale: int = None
    max_occurrences: int = 1


@dataclasses.dataclass(frozen=True)
class AttrGroupSpec:
    name: str
    attrs: tuple


@dataclasses.dataclass(frozen=True)
class AttrSetSpec:
    name: str
    stub: bool
    attrs: tuple
    optional_attrs: tuple


@dataclasses.dataclass(frozen=True)
class Placements:
    """The required and optional selectors declared at one level."""

    attrs: tuple = ()
    optional_attrs: tuple = ()


@dataclasses.dataclass(frozen=True)
class OpSpec:
    name: str
    type: str
    request: Placements
    reply: Placements = None
    availability: str = DEFAULT_AVAILABILITY
    pre_post: bool = False
    suggested_reply_length: int = None


@dataclasses.dataclass(frozen=True)
class FamilySpec:
    """One specification file, syntax-checked and normalized."""

    path: str
    name: str
    license: str
    inherits: str
    enums: tuple
    flags: tuple
    groups: tuple
    sets: tuple
    ops: tuple


@dataclasses.dataclass(frozen=True)
class RegistryEntry:
    name: str
    deprecated: bool


@dataclasses.dataclass(frozen=True)
class Registry:
    """The append-only family registry, indexed by family ID."""

    path: str
    entries: tuple

    def family_id(self, name):
        """Return the registered family ID of name, or None if unregistered."""
        for family_id, entry in enumerate(self.entries):
            if entry.name == name:
                return family_id
        return None


class Paths:
    """Where the specs, the registry and the meta-schemas are."""

    def __init__(self, spec, schema=None, registry=None, abi=None):
        self.spec = pathlib.Path(spec)
        self.spec_dir = self.spec.parent
        self.doc_dir = self.spec_dir.parent
        self.schema = pathlib.Path(schema or self.doc_dir / 'ctlv-schema.yaml')
        self.registry = pathlib.Path(registry or
                                     self.doc_dir / 'families.yaml')
        # The two meta-schemas ship together, so overriding one locates both.
        self.registry_schema = (self.schema.parent /
                                'ctlv-registry-schema.yaml')
        self.abi_dir = pathlib.Path(abi or self.doc_dir / 'abi')

    def sibling_spec(self, name):
        """Return the path of another family's specification.

        The one root family lives where the meta-schema does, so a corpus of
        fixtures inherits it rather than a copy.
        """
        if name == CORE_FAMILY_NAME:
            return self.schema.parent / 'specs' / f'{name}.yaml'
        return self.spec_dir / f'{name}.yaml'

    def abi(self, name):
        """Return the path of one family's checked-in ABI snapshot."""
        return self.abi_dir / f'{name}.yaml'


def load_registry(paths):
    """Load and check the family registry."""
    path = paths.registry
    data = yamlload.load(path)
    _validate_syntax(path, data, paths.registry_schema)

    entries = []
    seen = set()
    for item in data['families']:
        if isinstance(item, dict):
            entry = RegistryEntry(item['name'], item['deprecated'])
        else:
            entry = RegistryEntry(item, False)
        if entry.name in seen:
            raise CtlvError(f'{path}: {entry.name}', 'duplicate family')
        seen.add(entry.name)
        entries.append(entry)

    core = entries[CORE_FAMILY_ID]
    if core.name != CORE_FAMILY_NAME:
        raise CtlvError(f'{path}: {core.name}',
                        f'family ID {CORE_FAMILY_ID} belongs to '
                        f'{CORE_FAMILY_NAME}, so it must be the first entry')
    if core.deprecated:
        raise CtlvError(f'{path}: {core.name}',
                        'the core family cannot be deprecated')
    if len(entries) - 1 > MAX_FAMILY_ID:
        raise CtlvError(str(path),
                        f'{len(entries)} entries need a family ID past the '
                        f'{MAX_FAMILY_ID} maximum')
    return Registry(str(path), tuple(entries))


def load(path, schema=None):
    """Load, syntax-check and normalize one family specification."""
    path = pathlib.Path(path)
    data = yamlload.load(path)
    _validate_syntax(path, data, schema)

    family = data['family']
    return FamilySpec(
        path=str(path),
        name=family['name'],
        license=_license(path),
        inherits=family.get('inherits'),
        enums=_value_lists(path, data.get('enums', []), 'enum'),
        flags=_value_lists(path, data.get('flags', []), 'flags'),
        groups=_groups(path, data.get('attr-groups', [])),
        sets=_sets(path, data.get('attr-sets', [])),
        ops=_ops(path, data.get('ops', [])),
    )


def _license(path):
    """Return the SPDX identifier the specification's first line carries."""
    with open(path, 'r', encoding='utf-8') as stream:
        found = LICENSE.match(stream.readline())
    if found is None:
        raise CtlvError(str(path),
                        'the first line must be an SPDX-License-Identifier '
                        'comment, which every generated file inherits')
    return found.group(1)


def _validate_syntax(path, data, schema_path):
    schema = yamlload.load(schema_path)
    validator = jsonschema.Draft7Validator(schema)
    error = jsonschema.exceptions.best_match(validator.iter_errors(data))
    if error is None:
        return
    where = f'{path}: {error.json_path}' if error.absolute_path else str(path)
    raise CtlvError(where, error.message)


def _value_lists(path, raw_lists, what):
    lists = []
    for raw in raw_lists:
        where = f'{path}: {what} {raw["name"]}'
        seen = set()
        for value in raw['values']:
            if value in seen:
                raise CtlvError(where, f'duplicate value "{value}"')
            seen.add(value)
        lists.append(ValueList(raw['name'], tuple(raw['values'])))
    return tuple(lists)


def _groups(path, raw_groups):
    groups = []
    for raw in raw_groups:
        attrs = tuple(_attr(f'{path}: attr {raw["name"]}.{a["name"]}', a)
                      for a in raw['attrs'])
        groups.append(AttrGroupSpec(raw['name'], attrs))
    return tuple(groups)


def _attr(where, raw):
    primary = _primary_key(where, raw)
    kind = _kind(primary, raw)
    _reject_unusable_keys(where, raw, primary, kind)

    attr = {
        'name': raw['name'],
        'kind': kind,
        'max_occurrences': _max_occurrences(raw),
    }
    if kind in INTEGER_KINDS:
        attr.update(_integer_bounds(where, raw, kind))
    elif kind == 'string':
        attr.update(_string_lengths(where, raw))
    elif kind == 'binary':
        attr.update(_binary_lengths(where, raw))
    elif kind in ('enum', 'flags'):
        attr['ref'] = raw[kind]
    elif kind == 'nested':
        attr['attrs'] = _selectors(raw['attrs'])
        attr['optional_attrs'] = _selectors(raw.get('optional-attrs'))
        attr['recursive'] = raw.get('recursive', False)
        if attr['recursive']:
            _check_recursive(where, attr)
    elif kind == 'blob':
        attr['blob'] = _blob(where, raw['blob'])
    # A bool has nothing to normalize.
    return AttrDecl(**attr)


def _check_recursive(where, attr):
    """A recursive nest is one named set, which is what it re-enters."""
    if attr['optional_attrs']:
        raise CtlvError(where, 'a recursive attr cannot declare '
                               'optional-attrs; the attr set it names says '
                               'which of its own members are optional')
    if len(attr['attrs']) != 1:
        raise CtlvError(where, 'a recursive attr nests exactly one attr set')
    if '.' in attr['attrs'][0]:
        raise CtlvError(where, f'"{attr["attrs"][0]}" is one attr, and a '
                               'recursive attr nests an attr set')


def _primary_key(where, raw):
    present = [key for key in PRIMARY_KEYS if key in raw]
    if not present:
        raise CtlvError(where, 'no primary declaration; expected one of '
                               'type, enum, flags, attrs or blob')
    if len(present) > 1:
        raise CtlvError(where, 'conflicting primary declarations: ' +
                               ', '.join(present))
    return present[0]


def _kind(primary, raw):
    if primary == 'type':
        return raw['type']
    if primary == 'attrs':
        return 'nested'
    return primary


def _reject_unusable_keys(where, raw, primary, kind):
    allowed = _COMMON_KEYS | {primary} | _ACCESSORY_KEYS[kind]
    unusable = sorted(set(raw) - allowed)
    if unusable:
        raise CtlvError(where, f'a {kind} attr cannot declare ' +
                               ', '.join(unusable))


def _max_occurrences(raw):
    value = raw.get('max-occurrences', 1)
    return UNLIMITED if value == 'unlimited' else value


def _integer_bounds(where, raw, kind):
    low, high = (0, U64_MAX) if kind == 'uint' else (S64_MIN, S64_MAX)
    minimum = raw.get('min', low)
    maximum = raw.get('max', high)
    if minimum < low or minimum > high:
        raise CtlvError(where, f'min {minimum} is outside the {kind} range')
    if maximum < low or maximum > high:
        raise CtlvError(where, f'max {maximum} is outside the {kind} range')
    if minimum > maximum:
        raise CtlvError(where, f'min {minimum} is greater than max {maximum}')
    if 'scale' in raw and 'unit' not in raw:
        raise CtlvError(where, 'scale requires unit')
    return {
        'min': minimum,
        'max': maximum,
        'unit': raw.get('unit'),
        'scale': raw.get('scale', 0) if 'unit' in raw else None,
    }


def _string_lengths(where, raw):
    if 'max-length' not in raw:
        raise CtlvError(where, 'a string attr requires max-length')
    minimum = raw.get('min-length', 0)
    maximum = raw['max-length']
    if minimum > maximum:
        raise CtlvError(where, f'min-length {minimum} is greater than '
                               f'max-length {maximum}')
    return {'min_length': minimum, 'max_length': maximum}


def _binary_lengths(where, raw):
    if 'length' in raw:
        if 'min-length' in raw or 'max-length' in raw:
            raise CtlvError(where, 'length cannot be combined with '
                                   'min-length or max-length')
        return {'min_length': raw['length'], 'max_length': raw['length']}
    if 'max-length' not in raw:
        raise CtlvError(where, 'a binary attr requires length or max-length')
    minimum = raw.get('min-length', 0)
    maximum = raw['max-length']
    if minimum > maximum:
        raise CtlvError(where, f'min-length {minimum} is greater than '
                               f'max-length {maximum}')
    return {'min_length': minimum, 'max_length': maximum}


def _blob(where, raw):
    alignment = raw.get('alignment', 1)
    if alignment & (alignment - 1):
        raise CtlvError(where, f'blob alignment {alignment} is not a power '
                               'of two')
    if raw['max-length'] % alignment:
        raise CtlvError(where, f'blob max-length {raw["max-length"]} is not '
                               f'a multiple of alignment {alignment}')
    return BlobDecl(direction=raw['direction'],
                    backings=tuple(raw['allowed-backings']),
                    max_length=raw['max-length'],
                    alignment=alignment)


def _sets(path, raw_sets):
    sets = []
    for raw in raw_sets:
        where = f'{path}: attr set {raw["name"]}'
        stub = raw.get('stub', False)
        attrs = _selectors(raw.get('attrs'))
        optional = _selectors(raw.get('optional-attrs'))
        if stub and (attrs or optional):
            raise CtlvError(where, 'a stub attr set cannot declare members')
        if not stub and not attrs and not optional:
            raise CtlvError(where, 'attr set has no members; declare '
                                   'stub: true for an extension point')
        sets.append(AttrSetSpec(raw['name'], stub, attrs, optional))
    return tuple(sets)


def _ops(path, raw_ops):
    ops = []
    for raw in raw_ops:
        where = f'{path}: op {raw["name"]}'
        op_type = raw['type']
        if op_type == 'event' and 'request' in raw:
            raise CtlvError(where, 'an event op cannot declare a request')
        if op_type == 'query' and 'reply' not in raw:
            raise CtlvError(where, 'a query op must declare a reply')
        if 'suggested-reply-length' in raw and op_type != 'query':
            raise CtlvError(where, 'suggested-reply-length is only valid on '
                                   'a query op')
        # An event is not executed, so there is nothing to enable or disable.
        if 'availability' in raw and op_type == 'event':
            raise CtlvError(where, 'availability is only valid on an action '
                                   'or a query op')
        # A hook pair wraps the execution of an op, and an event has none.
        if 'pre-post' in raw and op_type == 'event':
            raise CtlvError(where, 'pre-post is only valid on an action or a '
                                   'query op')
        # An omitted action or query request means a root with no children.
        request = None
        if op_type != 'event':
            request = _placements(raw.get('request'))
        # Omitted: an action reply is ACK-only, an event reply payload-free.
        reply = None
        if 'reply' in raw:
            reply = _placements(raw['reply'])
        elif op_type == 'event':
            reply = Placements()
        ops.append(OpSpec(
            name=raw['name'],
            type=op_type,
            request=request,
            reply=reply,
            availability=raw.get('availability', DEFAULT_AVAILABILITY),
            pre_post=raw.get('pre-post', False),
            suggested_reply_length=raw.get('suggested-reply-length'),
        ))
    return tuple(ops)


def _placements(raw):
    if raw is None:
        return Placements()
    return Placements(_selectors(raw.get('attrs')),
                      _selectors(raw.get('optional-attrs')))


def _selectors(raw):
    if raw is None:
        return ()
    if isinstance(raw, str):
        return (raw,)
    return tuple(raw)
