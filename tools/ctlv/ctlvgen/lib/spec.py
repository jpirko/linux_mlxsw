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
from collections.abc import Callable
from typing import Any

import jsonschema

from . import cenum
from . import yamlload
from .errors import CtlvError

# A generated file carries the license of its specification's first line.
LICENSE: re.Pattern[str] = re.compile(
    r'#\s*SPDX-License-Identifier:\s*(\S.*?)\s*$')

# The registry is indexed by family ID, so the root family is its first entry.
CORE_FAMILY_NAME: str = 'ctlv'
CORE_FAMILY_ID: int = 0

# Where a $header is, under the kernel tree: an enum is taken from uapi only.
UAPI_DIR: str = 'include/uapi'

MAX_FAMILY_NAME_LEN: int = 63
U8_MAX: int = (1 << 8) - 1
U16_MAX: int = (1 << 16) - 1
U32_MAX: int = (1 << 32) - 1
MAX_FAMILY_ID: int = U16_MAX

# Zero means unlimited, as introspection reports it.
UNLIMITED: int = 0

U64_MAX: int = (1 << 64) - 1
S64_MIN: int = -(1 << 63)
S64_MAX: int = (1 << 63) - 1
S8_MIN: int = -(1 << 7)
S8_MAX: int = (1 << 7) - 1

# The limits a spec, a snapshot and the docs write by name.
NAMED_NUMBERS: dict[str, int] = {}
for _bits in (8, 16, 32, 64):
    NAMED_NUMBERS[f'u{_bits}-max'] = (1 << _bits) - 1
    NAMED_NUMBERS[f's{_bits}-min'] = -(1 << (_bits - 1))
    NAMED_NUMBERS[f's{_bits}-max'] = (1 << (_bits - 1)) - 1
SIZE_SUFFIXES: tuple[tuple[str, int], ...] = (
    ('G', 1 << 30), ('M', 1 << 20), ('K', 1 << 10))

INTEGER_KINDS: tuple[str, ...] = ('uint', 'sint')
SCALAR_KINDS: tuple[str, ...] = ('uint', 'sint', 'bool', 'string', 'binary',
                                 'enum', 'flags')
SCHEMA_KINDS: tuple[str, ...] = ('uint', 'sint', 'bool', 'string', 'binary',
                                 'blob')
UNITS: tuple[str, ...] = ('seconds', 'hertz', 'bytes', 'celsius')
BACKINGS: tuple[str, ...] = ('va',)
DIRECTIONS: dict[str, str] = {'in': 'input', 'out': 'output', 'inout': 'inout'}
DEFAULT_BACKINGS: tuple[str, ...] = ('va',)

FD_ACCESSES: tuple[str, ...] = ('read', 'read-write')
DEFAULT_FD_ACCESS: str = 'read-write'

# Every op has one of these, and only an action or a query may be dynamic.
AVAILABILITIES: tuple[str, ...] = ('fixed', 'dynamic')
DEFAULT_AVAILABILITY: str = 'fixed'

_NAME: str = r'[a-z0-9]+(?:-[a-z0-9]+)*'
_FIELD_KEY: re.Pattern[str] = re.compile(
    rf'(?P<name>{_NAME})(?:\[(?P<count>[0-9]*|{_NAME})\])?(?P<optional>\?)?')
_KIND: re.Pattern[str] = re.compile(
    rf'(?P<kind>{_NAME})(?:\((?P<args>[^()]*)\))?')
_REFERENCE: re.Pattern[str] = re.compile(
    rf'(?P<what>record|extension)(?: +(?P<name>{_NAME}))?')
_INCLUDE: re.Pattern[str] = re.compile(rf'(?P<name>{_NAME})(?P<optional>\?)?')
_NUMBER: re.Pattern[str] = re.compile(r'(?P<value>-?[0-9]+)(?P<suffix>[KMG]?)')
_SCALE: re.Pattern[str] = re.compile(r'scale +(?P<value>-?[0-9]+)')
_ALIGN: re.Pattern[str] = re.compile(r'align +(?P<value>\S+)')


def number_text(value: int) -> str:
    """Return an integer as a spec writes it, a limit by its name."""
    for name, number in NAMED_NUMBERS.items():
        if number == value:
            return name
    return str(value)


def size_text(value: int) -> str:
    """Return a byte count as a spec writes it."""
    for suffix, scale in SIZE_SUFFIXES:
        if value >= scale and value % scale == 0:
            return f'{value // scale}{suffix}'
    if value >= 1 << 10:
        return number_text(value)
    return str(value)


@dataclasses.dataclass(frozen=True)
class Value:
    """One value of an enum or a flags definition, numbered if a header's."""

    name: str
    where: str
    doc: str | None = None
    number: int | None = None


@dataclasses.dataclass(frozen=True)
class HeaderEnum:
    """Enum tag of include/uapi/<path>, its constants before end if given."""

    path: str
    tag: str
    end: str | None = None


@dataclasses.dataclass(frozen=True)
class ValueList:
    """An enum or a flags definition, or an enum extension point."""

    name: str
    where: str
    values: tuple[Value, ...]
    header: HeaderEnum | None = None
    doc: str | None = None
    extension: bool = False


@dataclasses.dataclass(frozen=True)
class BlobDecl:
    """The blob properties of one blob field declaration."""

    direction: str
    backings: tuple[str, ...]
    max_length: int
    alignment: int


@dataclasses.dataclass(frozen=True)
class FieldDecl:
    """One field declaration.

    kind is the schema kind, enum or flags, or record for a record, which holds
    the fields written in place in body, the named record ref, or the
    extension point extension. A field that does not apply stays None.
    count_enum is the enum a count names, whose values the chain counts.
    """

    name: str
    kind: str
    where: str
    optional: bool = False
    max_occurrences: int = 1
    count_enum: str | None = None
    ref: str | None = None
    body: Body | None = None
    extension: str | None = None
    blob: BlobDecl | None = None
    min: int | None = None
    max: int | None = None
    min_length: int | None = None
    max_length: int | None = None
    unit: str | None = None
    scale: int | None = None
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class Place:
    """A field of records: placed by bare record, under its own name."""

    name: str
    optional: bool
    where: str
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class Include:
    """A named record whose fields are spliced in with their IDs."""

    name: str
    optional: bool
    where: str


@dataclasses.dataclass(frozen=True)
class Body:
    """A record as written: fields, placed records and includes, in order."""

    items: tuple[FieldDecl | Place | Include, ...] = ()


@dataclasses.dataclass(frozen=True)
class RecordSpec:
    """One entry of records:, a named record or a field of records:."""

    name: str
    where: str
    body: Body | None = None
    field: FieldDecl | None = None
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class MessageSpec:
    """A request or a reply: a record in place, or a named one."""

    where: str
    body: Body = Body()
    ref: str | None = None
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class OpSpec:
    """One op."""

    name: str
    type: str
    where: str
    request: MessageSpec | None
    reply: MessageSpec | None = None
    fd_access: str = DEFAULT_FD_ACCESS
    availability: str = DEFAULT_AVAILABILITY
    optional: bool = False
    pre_post: bool = False
    suggested_reply_length: int | None = None
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class FamilySpec:
    """One specification file, syntax-checked and normalized."""

    path: str
    name: str
    license: str
    inherits: str | None
    enums: tuple[ValueList, ...]
    flags: tuple[ValueList, ...]
    records: tuple[RecordSpec, ...]
    ops: tuple[OpSpec, ...]
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class RegistryEntry:
    name: str
    deprecated: bool


@dataclasses.dataclass(frozen=True)
class Registry:
    """The append-only family registry, indexed by family ID."""

    path: str
    entries: tuple[RegistryEntry, ...]

    @property
    def license(self) -> str:
        return _license(self.path)

    def family_id(self, name: str) -> int | None:
        """Return the registered family ID of name, or None if unregistered."""
        for family_id, entry in enumerate(self.entries):
            if entry.name == name:
                return family_id
        return None


class Paths:
    """Where the specs, the registry and the meta-schemas are."""

    def __init__(self, spec: pathlib.Path | str,
                 schema: pathlib.Path | str | None = None,
                 registry: pathlib.Path | str | None = None,
                 abi: pathlib.Path | str | None = None) -> None:
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
        self.registry_abi = self.abi_dir / 'families.registry.yaml'
        self.generated = self.doc_dir / 'generated-files.yaml'
        self.generated_schema = self.doc_dir / 'generated-files-schema.yaml'
        self.kernel_dir = self.schema.parent.parent.parent

    def sibling_spec(self, name: str) -> pathlib.Path:
        """Return the path of another family's specification.

        The one root family lives where the meta-schema does, so a corpus of
        fixtures inherits it rather than a copy.
        """
        if name == CORE_FAMILY_NAME:
            return self.schema.parent / 'specs' / f'{name}.yaml'
        return self.spec_dir / f'{name}.yaml'

    def abi(self, name: str) -> pathlib.Path:
        """Return the path of one family's checked-in ABI snapshot."""
        return self.abi_dir / f'{name}.yaml'


def load_registry(paths: Paths, path: pathlib.Path | str | None = None,
                  text: str | None = None) -> Registry:
    """Load and check the family registry, or the one in text."""
    path = pathlib.Path(path or paths.registry)
    data = yamlload.load(path, text)
    _validate_syntax(path, data, paths.registry_schema)

    entries = []
    seen = set()
    for item in data['families']:
        if isinstance(item, dict):
            entry = RegistryEntry(item['name'], item['deprecated'])
        else:
            entry = RegistryEntry(item, False)
        _family_name(f'{path}: family {entry.name}', entry.name)
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


def load(path: pathlib.Path | str, schema: pathlib.Path) -> FamilySpec:
    """Load, syntax-check and normalize one family specification."""
    path = pathlib.Path(path)
    data, lines = yamlload.load_with_lines(path)
    _validate_syntax(path, data, schema)

    reader = _Reader(path, lines,
                     pathlib.Path(schema).parent.parent.parent / UAPI_DIR)
    name = _family_name(f'{path}: family {data["family"]}', data['family'])
    inherits = data.get('inherits')
    if inherits is not None:
        inherits = _family_name(f'{path}: family {name} parent', inherits)
    return FamilySpec(
        path=str(path),
        name=name,
        license=_license(path),
        inherits=inherits,
        enums=reader.value_lists(data.get('enums', {}), 'enum'),
        flags=reader.value_lists(data.get('flags', {}), 'flags'),
        records=reader.records(data.get('records', {})),
        ops=reader.ops(data.get('ops', {})),
        doc=reader.doc(data, 'family'),
    )


def _license(path: pathlib.Path | str) -> str:
    """Return the SPDX identifier the specification's first line carries."""
    with open(path, 'r', encoding='utf-8') as stream:
        found = LICENSE.match(stream.readline())
    if found is None:
        raise CtlvError(str(path),
                        'the first line must be an SPDX-License-Identifier '
                        'comment, which every generated file inherits')
    return found.group(1)


def _validate_syntax(path: pathlib.Path, data: dict[str, Any],
                     schema_path: pathlib.Path) -> None:
    schema = yamlload.load(schema_path)
    validator = jsonschema.Draft7Validator(schema)
    error = jsonschema.exceptions.best_match(validator.iter_errors(data))
    if error is None:
        return
    where = f'{path}: {error.json_path}' if error.absolute_path else str(path)
    raise CtlvError(where, error.message)


def _family_name(where: str, name: str) -> str:
    if len(name) > MAX_FAMILY_NAME_LEN:
        raise CtlvError(where, f'name is {len(name)} characters, more than '
                               f'the {MAX_FAMILY_NAME_LEN} maximum')
    return name


def _arguments(text: str) -> list[str]:
    return [part.strip() for part in text.split(',')]


class _Reader:
    """Parses the parts of one specification, knowing its lines for docs."""

    def __init__(self, path: pathlib.Path, lines: list[str],
                 uapi: pathlib.Path | None = None) -> None:
        self.path = path
        self.lines = lines
        self.uapi = uapi

    def doc(self, mapping: dict[str, Any], key: str) -> str | None:
        """Return the comment lines right above key, without their #."""
        return self._comment(getattr(mapping, 'lines', {}).get(key))

    def _comment(self, line: int | None) -> str | None:
        if line is None:
            return None
        texts = []
        for index in range(line - 1, -1, -1):
            text = self.lines[index].strip()
            if not text.startswith('#') or LICENSE.match(text):
                break
            texts.append(text[1:].strip())
        return '\n'.join(reversed(texts)).strip() or None

    def where(self, mapping: dict[str, Any], key: str, what: str) -> str:
        return self._at(getattr(mapping, 'lines', {}).get(key), what)

    def _at(self, line: int | None, what: str) -> str:
        if line is None:
            return f'{self.path}: {what}'
        return f'{self.path}:{line + 1}: {what}'

    def _values(self, raw: list[str], after: int | None,
                what: str) -> tuple[Value, ...]:
        """Each value of a set, with the comment right above its line.

        after is the line of the key the values follow. A value on that line,
        or on another value's, has no comment of its own.
        """
        lines = getattr(raw, 'lines', [None] * len(raw))
        values = []
        for name, line in zip(raw, lines):
            doc = None if line == after else self._comment(line)
            values.append(Value(name, self._at(line, f'{what} value {name}'),
                                doc))
            after = line
        return tuple(values)

    def value_lists(self, raw_lists: dict[str, Any],
                    what: str) -> tuple[ValueList, ...]:
        lists = []
        for name, raw in raw_lists.items():
            where = self.where(raw_lists, name, f'{what} {name}')
            if name in SCHEMA_KINDS or name in ('record', 'extension'):
                raise CtlvError(where, f'"{name}" is a keyword of the spec '
                                       f'language, so no {what} is called '
                                       'that')
            if raw == 'extension':
                lists.append(ValueList(name, where, (),
                                       doc=self.doc(raw_lists, name),
                                       extension=True))
                continue
            if isinstance(raw, dict):
                lists.append(self._header_enum(name, where, raw,
                                               self.doc(raw_lists, name)))
                continue
            after = getattr(raw_lists, 'lines', {}).get(name)
            seen = set()
            for value in raw:
                if value in seen:
                    raise CtlvError(where, f'duplicate value "{value}"')
                seen.add(value)
            lists.append(ValueList(name, where,
                                   self._values(raw, after, f'{what} {name}'),
                                   doc=self.doc(raw_lists, name)))
        return tuple(lists)

    def _header_enum(self, name: str, where: str, raw: dict[str, str],
                     doc: str | None) -> ValueList:
        header = HeaderEnum(raw['$header'], raw['$enum'], raw.get('$end'))
        if self.uapi is None:
            raise CtlvError(where, 'no kernel tree to read headers from')
        constants = cenum.read(where, self.uapi, header.path, header.tag,
                               header.end)
        values = tuple(Value(constant, f'{where} value {constant}',
                             number=number)
                       for constant, number in constants)
        return ValueList(name, where, values, header, doc)

    def records(self, raw_records: dict[str, Any]) -> tuple[RecordSpec, ...]:
        records = []
        for key, raw in raw_records.items():
            found = _FIELD_KEY.fullmatch(key)
            assert found is not None
            name = found['name']
            where = self.where(raw_records, key, f'record {name}')
            doc = self.doc(raw_records, key)
            if found['optional']:
                raise CtlvError(where, 'a definition cannot be optional; '
                                       'whatever places it says so')
            if isinstance(raw, dict):
                if found['count'] is not None:
                    raise CtlvError(where, 'a count belongs to a field, and '
                                           'this is a record')
                records.append(RecordSpec(name, body=self.body(raw, where),
                                          doc=doc, where=where))
                continue
            decl = self.field(key, raw, where, doc)
            if isinstance(decl, Place):
                raise CtlvError(where, 'bare record places a field of '
                                       'records:, and this is where one is '
                                       'declared')
            if decl.kind == 'record' and decl.ref is None:
                raise CtlvError(where, 'a field of records: is shared by '
                                       'name, and an extension point is '
                                       'not shared')
            records.append(RecordSpec(name, field=decl, doc=doc,
                                      where=where))
        return tuple(records)

    def body(self, raw: dict[str, Any], where: str) -> Body:
        items: list[FieldDecl | Place | Include] = []
        names = set()
        for key, value in raw.items():
            if key == '$include':
                for text in [value] if isinstance(value, str) else value:
                    found = _INCLUDE.fullmatch(text)
                    assert found is not None
                    items.append(Include(found['name'],
                                         bool(found['optional']),
                                         self.where(raw, key,
                                                    f'$include {text}')))
                continue
            item = self.field(key, value, where, self.doc(raw, key), raw)
            if item.name in names:
                raise CtlvError(item.where, f'two fields are called '
                                            f'{item.name}')
            names.add(item.name)
            items.append(item)
        return Body(tuple(items))

    def field(self, key: str, raw: str | dict[str, Any], owner: str,
              doc: str | None,
              mapping: dict[str, Any] | None = None) -> FieldDecl | Place:
        """Return the FieldDecl or the Place one key and its value say."""
        found = _FIELD_KEY.fullmatch(key)
        assert found is not None
        name = found['name']
        optional = bool(found['optional'])
        count = found['count']
        where = owner
        if mapping is not None:
            where = self.where(mapping, key, f'field {name}')
        occurrences, count_enum = self._occurrences(where, count)
        common: dict[str, Any] = {'name': name, 'optional': optional,
                                  'max_occurrences': occurrences,
                                  'count_enum': count_enum, 'doc': doc,
                                  'where': where}
        if isinstance(raw, dict):
            return FieldDecl(kind='record', body=self.body(raw, where),
                             **common)
        text = raw.strip()
        reference = _REFERENCE.fullmatch(text)
        if reference is not None:
            return self._reference(where, reference, count, common)
        found = _KIND.fullmatch(text)
        if found is None:
            raise CtlvError(where, f'"{text}" is not a schema kind')
        kind, args = found['kind'], found['args']
        if kind in INTEGER_KINDS:
            return FieldDecl(kind=kind, **self._integer(where, kind, args),
                             **common)
        if kind == 'bool':
            if args is not None:
                raise CtlvError(where, 'a bool takes no bounds')
            return FieldDecl(kind=kind, **common)
        if kind in ('string', 'binary'):
            return FieldDecl(kind=kind, **self._lengths(where, kind, args),
                             **common)
        if kind == 'blob':
            return FieldDecl(kind=kind, blob=self._blob(where, args),
                             **common)
        if args is not None:
            raise CtlvError(where, f'{kind} names an enum or flags, which '
                                   'take no bounds')
        # Which of the two it names, the chain says.
        return FieldDecl(kind='definition', ref=kind, **common)

    def _reference(self, where: str, reference: re.Match[str],
                   count: str | None,
                   common: dict[str, Any]) -> FieldDecl | Place:
        target = reference['name']
        if reference['what'] == 'extension':
            if target is None:
                raise CtlvError(where, 'extension needs the name of its '
                                       'extension point')
            return FieldDecl(kind='record', extension=target, **common)
        if target is not None:
            return FieldDecl(kind='record', ref=target, **common)
        if count is not None:
            raise CtlvError(where, 'a field of records: takes its count '
                                   'from its declaration, not where it is '
                                   'placed')
        return Place(common['name'], common['optional'], where,
                     common['doc'])

    def _occurrences(self, where: str,
                     count: str | None) -> tuple[int, str | None]:
        """A count, or the enum it names, which semantics.py counts."""
        if count is None:
            return 1, None
        if count == '':
            return UNLIMITED, None
        if not count.isdigit():
            return 1, count
        value = int(count)
        if value < 1 or value > U32_MAX:
            raise CtlvError(where, f'count {value} is outside 1 to u32-max')
        return value, None

    def _integer_value(self, where: str, text: str, kind: str,
                       unit: str | None) -> int:
        if text in NAMED_NUMBERS:
            return NAMED_NUMBERS[text]
        found = _NUMBER.fullmatch(text)
        if found is None:
            raise CtlvError(where, f'"{text}" is not a number')
        if found['suffix'] and unit != 'bytes':
            raise CtlvError(where, f'"{text}": K, M and G are for byte '
                                   f'counts; give the {kind} the bytes unit')
        factor = dict(SIZE_SUFFIXES).get(found['suffix'], 1)
        return int(found['value']) * factor

    def _size(self, where: str, text: str, what: str,
              limit: int = U32_MAX) -> int:
        if text in NAMED_NUMBERS:
            value = NAMED_NUMBERS[text]
        else:
            found = _NUMBER.fullmatch(text)
            if found is None:
                raise CtlvError(where, f'{what} "{text}" is not a number')
            value = int(found['value'])
            for suffix, scale in SIZE_SUFFIXES:
                if found['suffix'] == suffix:
                    value *= scale
        if value < 0 or value > limit:
            raise CtlvError(where, f'{what} {text} is outside 0 to '
                                   f'{number_text(limit)}')
        return value

    def _range(self, where: str, text: str,
               parse: Callable[[str], int]) -> tuple[int | None, int | None]:
        low, _, high = text.partition('..')
        return (parse(low.strip()) if low.strip() else None,
                parse(high.strip()) if high.strip() else None)

    def _integer(self, where: str, kind: str,
                 args: str | None) -> dict[str, Any]:
        low, high = (0, U64_MAX) if kind == 'uint' else (S64_MIN, S64_MAX)
        minimum, maximum, unit, scale = low, high, None, None
        bounds = None
        for arg in _arguments(args) if args is not None else ():
            scaled = _SCALE.fullmatch(arg)
            if '..' in arg and bounds is None:
                bounds = arg
            elif arg in UNITS and unit is None:
                unit = arg
            elif scaled is not None and scale is None:
                scale = int(scaled['value'])
            else:
                raise CtlvError(where, f'"{arg}" is not a range, a unit or '
                                       'a scale, or is given twice')
        if bounds is not None:
            first, last = self._range(
                where, bounds,
                lambda text: self._integer_value(where, text, kind, unit))
            minimum = low if first is None else first
            maximum = high if last is None else last
        if minimum < low or minimum > high:
            raise CtlvError(where, f'minimum {minimum} is outside the '
                                   f'{kind} range')
        if maximum < low or maximum > high:
            raise CtlvError(where, f'maximum {maximum} is outside the '
                                   f'{kind} range')
        if minimum > maximum:
            raise CtlvError(where, f'minimum {minimum} is greater than '
                                   f'maximum {maximum}')
        if scale is not None and unit is None:
            raise CtlvError(where, 'scale requires a unit')
        if unit is not None and scale is None:
            scale = 0
        if scale is not None and (scale < S8_MIN or scale > S8_MAX):
            raise CtlvError(where, f'scale {scale} is outside the s8 range')
        return {'min': minimum, 'max': maximum, 'unit': unit, 'scale': scale}

    def _lengths(self, where: str, kind: str,
                 args: str | None) -> dict[str, Any]:
        if args is None or '..' not in args and not args.strip():
            raise CtlvError(where, f'a {kind} needs a maximum length, '
                                   f'as {kind}(..N)')

        def size(text: str) -> int:
            return self._size(where, text, 'length')

        if '..' in args:
            minimum, maximum = self._range(where, args, size)
        else:
            minimum = maximum = size(args.strip())
        if maximum is None:
            raise CtlvError(where, f'a {kind} needs a maximum length, '
                                   f'as {kind}(..N)')
        minimum = minimum or 0
        if minimum > maximum:
            raise CtlvError(where, f'minimum length {minimum} is greater '
                                   f'than maximum length {maximum}')
        return {'min_length': minimum, 'max_length': maximum}

    def _blob(self, where: str, args: str) -> BlobDecl:
        parts = _arguments(args) if args is not None else ['']
        direction = DIRECTIONS.get(parts[0])
        if direction is None:
            raise CtlvError(where, 'a blob starts with its direction, in, '
                                   'out or inout')
        maximum, alignment, backings = None, None, None
        for arg in parts[1:]:
            aligned = _ALIGN.fullmatch(arg)
            if arg.startswith('..') and maximum is None:
                maximum = self._size(where, arg[2:].strip(), 'blob length',
                                     U64_MAX)
            elif aligned is not None and alignment is None:
                alignment = self._size(where, aligned['value'],
                                       'blob alignment')
            elif backings is None and all(word in BACKINGS
                                          for word in arg.split()):
                backings = tuple(arg.split())
                if len(set(backings)) != len(backings):
                    raise CtlvError(where, f'"{arg}" names a backing twice')
            else:
                raise CtlvError(where, f'"{arg}" is not a maximum length, an '
                                       'alignment or a list of backings, or '
                                       'is given twice')
        maximum = U32_MAX if maximum is None else maximum
        alignment = 1 if alignment is None else alignment
        if maximum < 1:
            raise CtlvError(where, 'a blob holds at least one byte')
        if alignment < 1 or alignment & (alignment - 1):
            raise CtlvError(where, f'blob alignment {alignment} is not a '
                                   'power of two')
        if maximum % alignment:
            raise CtlvError(where, f'blob length {size_text(maximum)} is not '
                                   'a multiple of alignment '
                                   f'{size_text(alignment)}')
        return BlobDecl(direction=direction,
                        backings=backings or DEFAULT_BACKINGS,
                        max_length=maximum, alignment=alignment)

    def message(self, raw_op: dict[str, Any], key: str,
                where: str) -> MessageSpec:
        raw = raw_op.get(key)
        doc = self.doc(raw_op, key)
        if raw is None or raw == {}:
            return MessageSpec(where=where, doc=doc)
        if isinstance(raw, str):
            found = _REFERENCE.fullmatch(raw)
            assert found is not None
            return MessageSpec(ref=found['name'], where=where, doc=doc)
        return MessageSpec(body=self.body(raw, where), where=where, doc=doc)

    def ops(self, raw_ops: dict[str, Any]) -> tuple[OpSpec, ...]:
        ops = []
        for name, raw in raw_ops.items():
            where = self.where(raw_ops, name, f'op {name}')
            op_type = raw['type']
            suggested = raw.get('suggested-reply-length')
            if suggested is not None:
                suggested = self._size(where, str(suggested),
                                       'suggested-reply-length')
            self._check_op_keys(where, raw, op_type)
            request = self.message(raw, 'request', f'{where} request')
            reply = None
            if 'reply' in raw:
                reply = self.message(raw, 'reply', f'{where} reply')
            ops.append(OpSpec(
                name=name,
                type=op_type,
                where=where,
                request=request,
                reply=reply,
                fd_access=raw.get('fd-access', DEFAULT_FD_ACCESS),
                availability=raw.get('availability', DEFAULT_AVAILABILITY),
                optional=raw.get('optional', False),
                pre_post=raw.get('pre-post', False),
                suggested_reply_length=suggested,
                doc=self.doc(raw_ops, name),
            ))
        return tuple(ops)

    def _check_op_keys(self, where: str, raw: dict[str, Any],
                       op_type: str) -> None:
        if op_type == 'query' and 'reply' not in raw:
            raise CtlvError(where, 'a query op must declare a reply')
        if 'suggested-reply-length' in raw and op_type != 'query':
            raise CtlvError(where, 'suggested-reply-length is only valid on '
                                   'a query op')
