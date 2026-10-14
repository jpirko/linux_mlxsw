# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The effective form of a CTLV family.

The specifications of one inheritance chain resolved into one schema: a type
for every field from where it is declared, every record composed of its own
fields and what it places and includes, and one tree per op request and reply.
A rule enforced here is not re-enforced downstream.
"""

from __future__ import annotations

import dataclasses
from collections.abc import Iterable
from typing import TypeVar

from . import sizes
from . import spec
from .errors import CtlvError

MAX_FLAG_BITS: int = 64

# These root enums are stored more narrowly than a generic enum range.
ROOT_ENUM_VALUES_MAX: dict[str, int] = {
    'op-type': spec.U8_MAX,
    'op-availability': spec.U8_MAX,
    'op-fd-access': spec.U8_MAX,
    'blob-direction': spec.U8_MAX,
    # BIT(value) is stored in struct ctlv_node_schema.blob.backings.
    'blob-type': 15,
}

# Type kinds, bits 47..46 of a type, as enum ctlv_type_kind has them.
TYPE_KIND_NAMED: int = 0
TYPE_KIND_REQUEST: int = 2
TYPE_KIND_REPLY: int = 3
TYPE_KINDS: dict[str, int] = {
    'named': TYPE_KIND_NAMED, 'field': TYPE_KIND_NAMED,
    'request': TYPE_KIND_REQUEST, 'reply': TYPE_KIND_REPLY,
    'payload': TYPE_KIND_REPLY,
}

# What the 64 bits of a type hold room for.
MAX_RECORD_INDEX: int = (1 << 14) - 1
MAX_UNNAMED_DEPTH: int = 4
MAX_UNNAMED_PEERS: int = 15
MAX_FIELD_NUMBER: int = spec.U16_MAX


def _check_count(where: str, what: str, count: int, maximum: int) -> None:
    if count > maximum:
        raise CtlvError(where, f'{count} {what} exceed the {maximum} maximum')


def record_id(role: str, index: int) -> int:
    """The record of a type: its type kind, then its index."""
    return (TYPE_KINDS[role] << 30) | (index << 16)


def type_of(family_id: int, record: int, number: int) -> int:
    return (family_id << 48) | (record << 16) | number


@dataclasses.dataclass(frozen=True)
class EnumExtension:
    """An enum extension point, whose values one descendant family gives."""

    name: str
    family_id: int
    ext_id: int


@dataclasses.dataclass(frozen=True)
class Definition:
    """An effective enum or flags definition, value_docs in values' order.

    extension is set on an enum extension point, which has no values, and on
    the enum filling it, which belongs to the family that fills it.
    """

    name: str
    kind: str
    family_id: int
    values: tuple[tuple[str, int], ...]
    value_docs: tuple[str | None, ...]
    header: spec.HeaderEnum | None = None
    doc: str | None = None
    extension: EnumExtension | None = None

    def numbers(self) -> tuple[int, ...]:
        return tuple(number for _, number in self.values)

    @property
    def is_extension_point(self) -> bool:
        return (self.extension is not None and
                self.extension.family_id == self.family_id)

    @property
    def is_fill(self) -> bool:
        return (self.extension is not None and
                self.extension.family_id != self.family_id)

    @property
    def max_value(self) -> int:
        """The highest value, 0 for an extension point nothing filled."""
        return max(self.numbers(), default=0)


@dataclasses.dataclass(frozen=True)
class Field:
    """A field, with its complete TLV type.

    record is the name of the record declaring it, which its constant and
    helpers take; shared marks a field of records:, which takes its own.
    full_name says where it is, and holds is the path of the record it holds.
    """

    record: str
    name: str
    family_id: int
    record_id: int
    number: int
    type: int
    decl: spec.FieldDecl
    full_name: str
    shared: bool = False
    holds: str | None = None

    @property
    def c_name(self) -> str:
        return self.name if self.shared else f'{self.record}-{self.name}'


@dataclasses.dataclass(frozen=True)
class Member:
    """A field as one record has it: its own, placed or included."""

    field: Field
    required: bool
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class Record:
    """A named record, a field of records:, an op's request or reply root,
    or an unnamed record inside one of them.

    role says which: named, field, request, reply, payload or unnamed. path
    is unique, name is what constants and helpers take. fields are the ones
    declared here, members the whole record.
    """

    name: str
    family_id: int
    record_id: int
    role: str
    path: str
    fields: tuple[Field, ...]
    members: tuple[Member, ...]
    doc: str | None = None


@dataclasses.dataclass(frozen=True)
class Extension:
    """An extension point, which one descendant family's record fills."""

    name: str
    family_id: int
    ext_id: int
    optional: bool = False


@dataclasses.dataclass(frozen=True)
class Fill:
    """The record one family puts into an extension point it inherited.

    root stands for the extension point, so its children are what splices in.
    """

    name: str
    owner_family_id: int
    family_id: int
    ext_id: int
    root: Node
    record: Record


@dataclasses.dataclass(frozen=True)
class Node:
    """One node of an expanded op tree. The root has no field.

    extensions are the extension points this record holds. A recursive record
    becomes a node naming its record in expands and childless nodes below it
    naming it in reenters, so walking children terminates.
    """

    field: Field | None
    required: bool
    max_occurrences: int
    required_children: tuple[Node, ...]
    optional_children: tuple[Node, ...]
    extensions: tuple[Extension, ...] = ()
    expands: str | None = None
    reenters: str | None = None

    @property
    def is_root(self) -> bool:
        return self.field is None

    @property
    def child_field(self) -> Field:
        """The field of a node below the root, which every one has."""
        assert self.field is not None
        return self.field

    def children(self) -> tuple[Node, ...]:
        return self.required_children + self.optional_children


@dataclasses.dataclass(frozen=True)
class Message:
    """One message root that no op declares, built by the core unasked.

    Only the root family declares one.
    """

    name: str
    family_id: int
    record_id: int
    root: Node
    max_len: int
    limit: str


@dataclasses.dataclass(frozen=True)
class Op:
    """An effective op.

    availability is the permission to enable and disable the op per device,
    not a state. optional means a device has the op only once its family opts
    in. pre_post asks for a pair of this op's own, inside the one the device
    puts around every op. reply is an event's payload.
    """

    name: str
    type: str
    fd_access: str
    availability: str
    optional: bool
    pre_post: bool
    family_id: int
    local_id: int
    op_id: int
    request: Node | None
    reply: Node | None
    sizes: sizes.OpSizes
    doc: str | None = None
    request_record: Record | None = None
    reply_record: Record | None = None


_Owned = TypeVar('_Owned', Definition, Record, Field, Extension,
                 EnumExtension, Op)


@dataclasses.dataclass(frozen=True)
class Family:
    """One effective family: itself plus everything it inherits.

    owners maps a family ID to its name for every family in the chain. fills
    is what a generator emits, chain_fills what a device answers with.
    """

    name: str
    family_id: int
    path: str
    chain: tuple[spec.FamilySpec, ...]
    owners: dict[int, str]
    definitions: dict[str, Definition]
    records: dict[str, Record]
    fields: dict[str, Field]
    extensions: dict[str, Extension]
    ops: tuple[Op, ...]
    fills: tuple[Fill, ...]
    chain_fills: tuple[Fill, ...]
    messages: tuple[Message, ...]
    enum_extensions: dict[str, EnumExtension] = dataclasses.field(
        default_factory=dict)

    @property
    def parent(self) -> str | None:
        return self.chain[-1].inherits

    @property
    def license(self) -> str:
        return self.chain[-1].license

    @property
    def doc(self) -> str | None:
        return self.chain[-1].doc

    def _local(self, items: Iterable[_Owned]) -> tuple[_Owned, ...]:
        return tuple(item for item in items
                     if item.family_id == self.family_id)

    @property
    def local_definitions(self) -> tuple[Definition, ...]:
        return self._local(self.definitions.values())

    @property
    def local_records(self) -> tuple[Record, ...]:
        return self._local(self.records.values())

    @property
    def local_fields(self) -> tuple[Field, ...]:
        return self._local(self.fields.values())

    @property
    def local_extensions(self) -> tuple[Extension, ...]:
        return self._local(self.extensions.values())

    @property
    def local_enum_extensions(self) -> tuple[EnumExtension, ...]:
        return self._local(self.enum_extensions.values())

    @property
    def enum_fills(self) -> tuple[Definition, ...]:
        """The enum extension points this family fills, as it fills them."""
        return tuple(definition for definition in self.local_definitions
                     if definition.is_fill)

    @property
    def local_ops(self) -> tuple[Op, ...]:
        return self._local(self.ops)

    def fill_of(self, extension: Extension) -> Fill | None:
        """What this chain put into one extension point, if anything did."""
        for fill in self.chain_fills:
            if (fill.family_id, fill.ext_id) == (extension.family_id,
                                                 extension.ext_id):
                return fill
        return None


def field_uses(family: Family, family_ids: set[int] | None = None
               ) -> tuple[dict[int, Field], dict[int, Field]]:
    """The fields read in a request and written in a reply or an event.

    Two dicts by type, in the order the fields are met, over the ops and
    fills of the families in family_ids, the whole chain by default.
    """
    ids = set(family.owners) if family_ids is None else set(family_ids)
    inputs: dict[int, Field] = {}
    outputs: dict[int, Field] = {}

    def walk(node: Node, found: dict[int, Field]) -> None:
        for child in node.children():
            found.setdefault(child.child_field.type, child.child_field)
            walk(child, found)
        for extension in node.extensions:
            fill = family.fill_of(extension)
            if fill is not None and fill.owner_family_id in ids:
                walk(fill.root, found)

    for op in family.ops:
        if op.family_id not in ids:
            continue
        if op.request is not None:
            walk(op.request, inputs)
        if op.reply is not None:
            walk(op.reply, outputs)
    for message in family.messages:
        if message.family_id in ids:
            walk(message.root, outputs)
    return inputs, outputs


def own_uses(family: Family) -> tuple[dict[int, Field], dict[int, Field]]:
    """The uses this family adds to its parent's, which it has helpers for.

    A helper is named after the family declaring the field, and emitted by
    the first family of the chain whose ops or fills read or write it.
    """
    inputs, outputs = field_uses(family)
    parent_inputs, parent_outputs = field_uses(
        family, set(family.owners) - {family.family_id})
    return ({key: field for key, field in inputs.items()
             if key not in parent_inputs},
            {key: field for key, field in outputs.items()
             if key not in parent_outputs})


def load(paths: spec.Paths) -> Family:
    """Load the family at paths.spec, resolved against its whole chain."""
    registry = spec.load_registry(paths)
    return _Resolver(paths, registry).resolve()


class _Resolver:
    def __init__(self, paths: spec.Paths, registry: spec.Registry) -> None:
        self.paths = paths
        self.registry = registry
        self.definitions: dict[str, Definition] = {}
        self.records: dict[str, Record] = {}
        # Named records and fields of records:, by name, and where each is.
        self.named: dict[str, Record] = {}
        self.declared: dict[str, tuple[int, str, int, spec.RecordSpec]] = {}
        self.fields: dict[str, Field] = {}
        self.c_names: dict[tuple[int, str], Field] = {}
        self.extensions: dict[str, Extension] = {}
        self.enum_extensions: dict[str, EnumExtension] = {}
        self.fill_records: dict[str, Record] = {}
        self.fillers: dict[str, str] = {}
        # Where each extension point sits: message, context, record level.
        self.sites: dict[str, list[tuple[str, str, int]]] = {}
        self.ops: list[Op] = []
        self.used: set[str] = set()
        self.building: tuple[str, ...] = ()

    def resolve(self) -> Family:
        chain = self._load_chain()
        self._check_names(chain)
        self._check_order(chain)
        target = chain[-1]
        for family in chain:
            family_id = self._family_id(family)
            self._add_definitions(family, family_id, family.enums, 'enum')
            self._add_definitions(family, family_id, family.flags, 'flags')
            self._add_records(family, family_id)
            self._add_ops(family, family_id)
            self._check_used(family, family_id)
        fills = self._fills(target)
        chain_fills = self._fills(None)
        self._check_fills(chain_fills)
        _check_count(f'{target.path}: family {target.name}',
                     'extension fills', len(fills), spec.U32_MAX)
        records = sorted(self.records.values(), key=_record_order)
        resolved = Family(
            name=target.name,
            family_id=self._family_id(target),
            path=target.path,
            chain=chain,
            owners={self._family_id(item): item.name for item in chain},
            definitions=self.definitions,
            records={record.path: record for record in records},
            fields=self.fields,
            extensions=self.extensions,
            ops=tuple(self.ops),
            fills=fills,
            chain_fills=chain_fills,
            messages=self._messages(target),
            enum_extensions=self.enum_extensions,
        )
        _check_composed(resolved)
        _check_docs(target)
        return resolved

    def _messages(self, family: spec.FamilySpec) -> tuple[Message, ...]:
        """The message roots this family declares that no op uses.

        The root family alone has them, and has to have them.
        """
        messages = []
        for name in sizes.CORE_MESSAGES:
            record = self.named.get(name)
            if record is None or record.role != 'named':
                if family.name != spec.CORE_FAMILY_NAME:
                    continue
                raise CtlvError(f'{family.path}: family {family.name}',
                                f'the root family has to declare the "{name}" '
                                'record, which the core builds a message of')
            if record.family_id != self._family_id(family):
                continue
            where = self.declared[name][1]
            root = self._root(record.members, where, ())
            if _extensions_anywhere(root):
                raise CtlvError(where, 'a message the core builds cannot '
                                       'hold an extension point')
            self._check_subtree(where, root, 'message')
            _check_tree_size(where, root)
            messages.append(Message(
                name=name,
                family_id=record.family_id,
                record_id=record.record_id,
                root=root,
                max_len=sizes.message_max_len(where, name, root),
                limit=sizes.CORE_MESSAGES[name][0],
            ))
        return tuple(messages)

    def _error_info_max_len(self) -> int:
        """The longest error information the core builds, 0 without one."""
        record = self.named.get(sizes.ERROR_INFO)
        if record is None:
            return 0
        where = self.declared[record.name][1]
        root = self._root(record.members, where, ())
        self._check_subtree(where, root, 'message')
        return sizes.root_max_len(root)

    def _fills(self, family: spec.FamilySpec | None) -> tuple[Fill, ...]:
        """The extension points one family fills, with what it puts in each.

        Passing None asks for the fills of the whole chain.
        """
        fills = []
        for name, filler in self.fillers.items():
            if family is not None and filler != family.name:
                continue
            extension = self.extensions[name]
            record = self.fill_records[name]
            where = self.declared[name][1]
            root = self._root(record.members, where, ())
            _check_tree_size(where, root)
            if _reenters_anywhere(root):
                raise CtlvError(where, 'a fill cannot re-enter a record')
            owner_family_id = self.registry.family_id(filler)
            assert owner_family_id is not None
            fills.append(Fill(
                name=name,
                owner_family_id=owner_family_id,
                family_id=extension.family_id,
                ext_id=extension.ext_id,
                root=root,
                record=record,
            ))
        return tuple(fills)

    # Loading and identity

    def _load_chain(self) -> tuple[spec.FamilySpec, ...]:
        family = spec.load(self.paths.spec, self.paths.schema)
        if self.paths.spec.stem != family.name:
            raise CtlvError(f'{family.path}: family {family.name}',
                            'a specification must be named after its family, '
                            f'so this file must be {family.name}.yaml')
        chain = [family]
        seen = [family.name]
        self._check_root(family)
        while chain[0].inherits:
            parent_name = chain[0].inherits
            child = chain[0]
            if parent_name in seen:
                # seen runs from the selected family to its ancestors.
                raise CtlvError(f'{child.path}: family {child.name}',
                                'inheritance cycle: ' +
                                ' -> '.join(seen + [parent_name]))
            parent_path = self.paths.sibling_spec(parent_name)
            parent = spec.load(parent_path, self.paths.schema)
            if parent.name != parent_name:
                raise CtlvError(f'{parent_path}: family {parent.name}',
                                f'expected family {parent_name}')
            self._check_root(parent)
            seen.append(parent_name)
            chain.insert(0, parent)
        return tuple(chain)

    def _check_root(self, family: spec.FamilySpec) -> None:
        """Require every family but the root one to name what it inherits."""
        if family.name == spec.CORE_FAMILY_NAME:
            if family.inherits:
                raise CtlvError(f'{family.path}: family {family.name}',
                                'the root family heads every chain, so it '
                                'inherits nothing')
        elif not family.inherits:
            raise CtlvError(f'{family.path}: family {family.name}',
                            'must name the family it inherits, which is the '
                            f'root {spec.CORE_FAMILY_NAME} for a family '
                            'directly below it')

    def _check_names(self, chain: tuple[spec.FamilySpec, ...]) -> None:
        """Require every child family name to carry its whole ancestry."""
        for parent, child in zip(chain, chain[1:]):
            suffix = child.name.removeprefix(f'{parent.name}-')
            if not suffix or suffix == child.name:
                raise CtlvError(f'{child.path}: family {child.name}',
                                f'a family that inherits {parent.name} must '
                                f'be named {parent.name}-<suffix>')

    def _check_order(self, chain: tuple[spec.FamilySpec, ...]) -> None:
        """Require a parent to be registered before what inherits it.

        Ascending positions let the core walk a chain once and index it.
        """
        for parent, child in zip(chain, chain[1:]):
            if self._family_id(parent) >= self._family_id(child):
                raise CtlvError(f'{child.path}: family {child.name}',
                                f'is registered before {parent.name}, the '
                                'family it inherits')

    def _family_id(self, family: spec.FamilySpec) -> int:
        family_id = self.registry.family_id(family.name)
        if family_id is None:
            raise CtlvError(f'{family.path}: family {family.name}',
                            f'not registered in {self.registry.path}')
        return family_id

    # Definitions

    def _add_definitions(self, family: spec.FamilySpec, family_id: int,
                         definitions: tuple[spec.ValueList, ...],
                         kind: str) -> None:
        for definition in definitions:
            where = f'{family.path}: {kind} {definition.name}'
            existing = self.definitions.get(definition.name)
            if (existing is not None and existing.is_extension_point and
                    kind == 'enum'):
                self._fill_enum(definition, family_id, existing, where)
                continue
            if existing is not None:
                owner = self._owner_name(existing.family_id)
                if existing.is_fill:
                    raise CtlvError(where, 'enum extension point '
                                           f'{definition.name} was already '
                                           f'filled by family {owner}')
                raise CtlvError(where, f'{definition.name} is already '
                                       f'declared by {owner}')
            if definition.extension:
                self._enum_extension(definition, family_id, where)
                continue
            if kind == 'flags' and len(definition.values) > MAX_FLAG_BITS:
                raise CtlvError(where, f'more than {MAX_FLAG_BITS} flag bits')
            if kind == 'enum':
                _check_count(where, 'enum values', len(definition.values),
                             spec.U32_MAX)
                if family_id == spec.CORE_FAMILY_ID:
                    maximum = ROOT_ENUM_VALUES_MAX.get(definition.name)
                    if maximum is not None:
                        _check_count(where, 'enum values',
                                     len(definition.values), maximum)
            # Enum values from one, zero being reserved, unless a header
            # numbers them; flag bits from zero.
            first = 1 if kind == 'enum' else 0
            values = tuple((value.name, first + index
                            if value.number is None else value.number)
                           for index, value in enumerate(definition.values))
            self.definitions[definition.name] = Definition(
                definition.name, kind, family_id, values,
                tuple(value.doc for value in definition.values),
                definition.header, definition.doc)

    def _enum_extension(self, definition: spec.ValueList, family_id: int,
                        where: str) -> None:
        if family_id == spec.CORE_FAMILY_ID:
            raise CtlvError(where, 'the root family has no extension points')
        ext_id = 1 + sum(point.family_id == family_id
                         for point in self.enum_extensions.values())
        _check_count(where, 'enum extension points', ext_id, spec.U32_MAX)
        extension = EnumExtension(definition.name, family_id, ext_id)
        self.enum_extensions[definition.name] = extension
        self.definitions[definition.name] = Definition(
            definition.name, 'enum', family_id, (), (), None, definition.doc,
            extension)

    def _fill_enum(self, definition: spec.ValueList, family_id: int,
                   point: Definition, where: str) -> None:
        """Give an inherited enum extension point its values."""
        if definition.extension:
            owner = self._owner_name(point.family_id)
            raise CtlvError(where, f'enum extension point {definition.name} '
                                   f'is declared by {owner}; a further one '
                                   'needs a new name')
        if definition.header:
            raise CtlvError(where, 'a fill numbers its own values, which a '
                                   'header cannot')
        _check_count(where, 'enum values', len(definition.values),
                     spec.U32_MAX)
        self.definitions[definition.name] = Definition(
            definition.name, 'enum', family_id,
            tuple((value.name, 1 + index)
                  for index, value in enumerate(definition.values)),
            tuple(value.doc for value in definition.values), None,
            definition.doc, point.extension)

    def _definition(self, decl: spec.FieldDecl) -> spec.FieldDecl:
        """Resolve a field naming an enum or flags into which one it is, and
        a count naming an enum into one occurrence per value."""
        if decl.count_enum is not None:
            decl = dataclasses.replace(decl,
                                       max_occurrences=self._count(decl))
        if decl.kind != 'definition':
            return decl
        assert decl.ref is not None
        definition = self.definitions.get(decl.ref)
        if definition is None:
            raise CtlvError(decl.where, f'"{decl.ref}" is not a schema kind, '
                                        'an enum or flags')
        return dataclasses.replace(decl, kind=definition.kind)

    def _count(self, decl: spec.FieldDecl) -> int:
        name = decl.count_enum
        assert name is not None
        definition = self.definitions.get(name)
        if definition is None or definition.kind != 'enum':
            raise CtlvError(decl.where, f'count {name} is not an enum')
        if definition.is_extension_point:
            raise CtlvError(decl.where, f'count {name} is an enum extension '
                                        'point, whose values a descendant '
                                        'family gives')
        return len(definition.values)

    # Records

    def _owner_name(self, family_id: int) -> str:
        return self.registry.entries[family_id].name

    def _add_records(self, family: spec.FamilySpec, family_id: int) -> None:
        where = f'{family.path}: family {family.name}'
        _check_count(where, 'named records', len(family.records),
                     MAX_RECORD_INDEX)
        own = []
        for index, record in enumerate(family.records, start=1):
            self._check_new_record(record)
            self.declared[record.name] = (family_id, record.where, index,
                                          record)
            own.append(record.name)
        self.used = set()
        for name in own:
            self._named(name, None)
        for name in own:
            if name in self.extensions and name not in self.fillers:
                self._fill(family, name)

    def _check_new_record(self, record: spec.RecordSpec) -> None:
        name = record.name
        extension = self.extensions.get(name)
        if extension is not None:
            if name in self.fillers:
                raise CtlvError(record.where, 'extension point was already '
                                              'filled by family '
                                              f'{self.fillers[name]}')
            if record.body is None:
                raise CtlvError(record.where, 'a fill is a record, not a '
                                              'field of records:')
            return
        if name in self.declared:
            owner = self._owner_name(self.declared[name][0])
            raise CtlvError(record.where, f'already declared by {owner}')

    def _fill(self, family: spec.FamilySpec, name: str) -> None:
        self.fillers[name] = family.name
        self.fill_records[name] = self.named[name]

    def _check_used(self, family: spec.FamilySpec, family_id: int) -> None:
        """A family that inherits declares a record to fill or to use."""
        if family_id == spec.CORE_FAMILY_ID:
            return
        for record in family.records:
            if record.name in self.used or record.name in self.fill_records:
                continue
            raise CtlvError(record.where,
                            f'no inherited extension point is called '
                            f'{record.name}, and nothing in {family.name} '
                            'uses it')

    def _named(self, name: str, where: str | None) -> Record:
        """Return the named record or field of records: called name."""
        if name in self.named:
            return self.named[name]
        if name not in self.declared:
            if name in self.extensions:
                raise CtlvError(where, f'{name} is an extension point, which '
                                       'extension names')
            raise CtlvError(where, f'no record is called {name}')
        if name in self.building:
            raise CtlvError(where, 'record cycle: ' +
                                   ' -> '.join(self.building + (name,)))
        family_id, declared_at, index, record_spec = self.declared[name]
        self.building += (name,)
        try:
            if record_spec.body is not None:
                record = self._record(record_spec.body, name, family_id,
                                      record_id('named', index), 'named',
                                      name, name, 0, declared_at,
                                      record_spec.doc)
            else:
                record = self._field_record(record_spec, family_id, index)
        finally:
            self.building = self.building[:-1]
        self.named[name] = record
        return record

    def _field_record(self, record_spec: spec.RecordSpec, family_id: int,
                      index: int) -> Record:
        """A field of records:: a record of one field, under its name."""
        name = record_spec.name
        identifier = record_id('field', index)
        assert record_spec.field is not None
        decl = self._definition(record_spec.field)
        holds = None
        if decl.kind == 'record' and decl.ref is not None:
            holds = self._held(decl.ref, decl.where)
        field = self._new_field(Field(
            record=name, name=name, family_id=family_id,
            record_id=identifier, number=1,
            type=type_of(family_id, identifier, 1), decl=decl,
            full_name=name, shared=True, holds=holds))
        record = Record(name=name, family_id=family_id,
                        record_id=identifier, role='field', path=name,
                        fields=(field,), members=(Member(field, True),),
                        doc=record_spec.doc)
        self.records[name] = record
        return record

    def _held(self, name: str, where: str) -> str:
        """Return the path of the named record a field holds."""
        self.used.add(name)
        if name in self.building:
            # A record holding itself, which the tree re-enters.
            is_field = self.declared[name][3].body is None
        else:
            is_field = self._named(name, where).role == 'field'
        if is_field:
            raise CtlvError(where, f'record {name} holds a named record, and '
                                   f'{name} is a field of records:, which '
                                   'bare record places')
        return name

    def _record(self, body: spec.Body, name: str, family_id: int,
                identifier: int, role: str, path: str, owner: str, depth: int,
                where: str, doc: str | None = None) -> Record:
        """Number the fields of one record and compose its members."""
        own, members, unnamed, number = [], [], 0, 0
        for item in body.items:
            if isinstance(item, spec.Include):
                members += self._include(item)
                continue
            if isinstance(item, spec.Place):
                target = self._named(item.name, item.where)
                self.used.add(item.name)
                if target.role != 'field':
                    raise CtlvError(item.where, f'bare record places a field '
                                                f'of records:, and '
                                                f'{item.name} is a record, '
                                                f'which record {item.name} '
                                                'holds or $include splices')
                members.append(Member(target.fields[0], not item.optional,
                                      item.doc))
                continue
            number += 1
            _check_count(item.where, 'fields', number, MAX_FIELD_NUMBER)
            decl = self._definition(item)
            if role in ('request', 'reply', 'payload'):
                full_name = f'{owner} {decl.name}'
            else:
                full_name = f'{path}.{decl.name}'
            holds = None
            if decl.kind == 'record' and decl.body is not None:
                unnamed += 1
                if unnamed > MAX_UNNAMED_PEERS:
                    raise CtlvError(item.where, f'more than '
                                                f'{MAX_UNNAMED_PEERS} '
                                                'unnamed records side by '
                                                f'side in {path}; name one '
                                                'in records:')
                if depth >= MAX_UNNAMED_DEPTH:
                    raise CtlvError(item.where, f'unnamed records go more '
                                                f'than {MAX_UNNAMED_DEPTH} '
                                                f'deep below {owner}; name '
                                                'one in records:')
                holds = self._record(
                    decl.body, f'{owner}-{decl.name}', family_id,
                    identifier | (unnamed << (12 - 4 * depth)), 'unnamed',
                    full_name, owner, depth + 1, item.where).path
            elif decl.kind == 'record' and decl.ref is not None:
                holds = self._held(decl.ref, decl.where)
            elif decl.extension is not None:
                self._extension(decl.extension, family_id, decl.where)
            field = self._new_field(Field(
                record=name, name=decl.name, family_id=family_id,
                record_id=identifier, number=number,
                type=type_of(family_id, identifier, number), decl=decl,
                full_name=full_name, holds=holds))
            own.append(field)
            members.append(Member(field, not decl.optional, decl.doc))
        self._check_members(where, path, members)
        record = Record(name=name, family_id=family_id, record_id=identifier,
                        role=role, path=path, fields=tuple(own),
                        members=tuple(members), doc=doc)
        self.records[path] = record
        return record

    def _include(self, item: spec.Include) -> list[Member]:
        target = self._named(item.name, item.where)
        self.used.add(item.name)
        if target.role == 'field':
            raise CtlvError(item.where, f'$include splices a named record, '
                                        f'and {item.name} is a field of '
                                        'records:, which bare record places')
        return [dataclasses.replace(member,
                                    required=member.required and
                                    not item.optional)
                for member in target.members]

    def _check_members(self, where: str, path: str,
                       members: list[Member]) -> None:
        names: dict[str, str] = {}
        types: set[int] = set()
        for member in members:
            field = member.field
            if field.type in types:
                raise CtlvError(where, f'{field.full_name} is in {path} '
                                       'twice')
            if field.name in names:
                raise CtlvError(where, f'two fields of {path} are called '
                                       f'{field.name}: {names[field.name]} '
                                       f'and {field.full_name}')
            names[field.name] = field.full_name
            types.add(field.type)

    def _new_field(self, field: Field) -> Field:
        clash = self.c_names.get((field.family_id, field.c_name))
        if clash is not None:
            hint = ''
            kinds = {clash.record_id >> 30, field.record_id >> 30}
            if (clash.record_id >> 16 & MAX_RECORD_INDEX ==
                    field.record_id >> 16 & MAX_RECORD_INDEX and
                    kinds == {TYPE_KIND_REQUEST, TYPE_KIND_REPLY}):
                hint = ('; a field an op has in both its request and its '
                        'reply is a field of records:')
            raise CtlvError(field.decl.where, f'{clash.full_name} and '
                                              f'{field.full_name} would both '
                                              f'be named {field.c_name}'
                                              f'{hint}')
        self.c_names[(field.family_id, field.c_name)] = field
        self.fields[field.full_name] = field
        return field

    def _extension(self, name: str, family_id: int, where: str) -> Extension:
        extension = self.extensions.get(name)
        if extension is not None:
            if extension.family_id != family_id:
                owner = self._owner_name(extension.family_id)
                raise CtlvError(where, f'extension point {name} is '
                                       f'declared by {owner}; a further one '
                                       'needs a new name')
            return extension
        if name in self.declared:
            raise CtlvError(where, f'{name} is a record, which record '
                                   f'{name} holds')
        if family_id == spec.CORE_FAMILY_ID:
            raise CtlvError(where, 'the root family has no extension points')
        ext_id = 1 + sum(point.family_id == family_id
                         for point in self.extensions.values())
        _check_count(where, 'extension points', ext_id, spec.U32_MAX)
        extension = Extension(name, family_id, ext_id)
        self.extensions[name] = extension
        return extension

    # Ops

    def _add_ops(self, family: spec.FamilySpec, family_id: int) -> None:
        _check_count(f'{family.path}: family {family.name}', 'ops',
                     len(family.ops), MAX_RECORD_INDEX)
        error_info_max_len = self._error_info_max_len()
        for local_id, op in enumerate(family.ops, start=1):
            where = f'{family.path}: op {op.name}'
            if any(existing.name == op.name for existing in self.ops):
                raise CtlvError(where, f'op "{op.name}" is already declared')
            request_record = reply_record = None
            request = reply = None
            if op.request is not None:
                request_record = self._message(op, op.request, family_id,
                                               local_id, 'request')
                request = self._tree(request_record, where + ' request')
            if op.reply is not None:
                which = 'payload' if op.type == 'event' else 'reply'
                reply_record = self._message(op, op.reply, family_id,
                                             local_id, which)
                reply = self._tree(reply_record, f'{where} {which}')
            self._check_context(where, op, request, reply)
            self.ops.append(Op(
                name=op.name,
                type=op.type,
                fd_access=op.fd_access,
                availability=op.availability,
                optional=op.optional,
                pre_post=op.pre_post,
                family_id=family_id,
                local_id=local_id,
                op_id=(family_id << 48) | local_id,
                request=request,
                reply=reply,
                sizes=sizes.compute(where, op, request, reply,
                                    error_info_max_len),
                doc=op.doc,
                request_record=request_record,
                reply_record=reply_record,
            ))

    def _message(self, op: spec.OpSpec, message: spec.MessageSpec,
                 family_id: int, local_id: int, which: str) -> Record:
        if message.ref is not None:
            return self.named[self._held(message.ref, message.where)]
        return self._record(message.body, op.name, family_id,
                            record_id(which, local_id), which,
                            f'{op.name} {which}', op.name, 0, message.where,
                            message.doc)

    def _tree(self, record: Record, where: str) -> Node:
        root = self._root(record.members, where, ())
        _check_tree_size(where, root)
        return root

    def _root(self, members: tuple[Member, ...], where: str,
              opened: tuple[str, ...]) -> Node:
        children = self._children(members, where, opened, 0)
        return Node(None, True, 1,
                    tuple(node for node in children if node.required),
                    tuple(node for node in children if not node.required))

    def _children(self, members: tuple[Member, ...], where: str,
                  opened: tuple[str, ...], depth: int) -> list[Node]:
        return [self._node(member.field, member.required, where, opened,
                           depth)
                for member in members]

    def _node(self, field: Field, required: bool, where: str,
              opened: tuple[str, ...], depth: int) -> Node:
        decl = field.decl
        if decl.kind != 'record':
            return Node(field, required, decl.max_occurrences, (), ())
        if decl.extension is not None:
            return Node(field, required, decl.max_occurrences, (), (),
                        extensions=(self.extensions[decl.extension],))
        assert field.holds is not None
        record = self.records[field.holds]
        named = record.role == 'named'
        if named and record.name in opened:
            # Open above, so one level further down rather than a new one.
            return Node(field, required, decl.max_occurrences, (), (),
                        reenters=record.name)
        if depth + 1 > sizes.MAX_DECLARED_RECORD_DEPTH:
            raise CtlvError(where, f'{field.full_name} goes deeper than the '
                                   f'{sizes.MAX_DECLARED_RECORD_DEPTH} level '
                                   'limit')
        children = self._children(record.members, where,
                                  opened + ((record.name,) if named else ()),
                                  depth + 1)
        return Node(field, required, decl.max_occurrences,
                    tuple(child for child in children if child.required),
                    tuple(child for child in children if not child.required),
                    expands=record.name if named else None)

    def _check_context(self, where: str, op: spec.OpSpec, request: Node | None,
                       reply: Node | None) -> None:
        if request is not None:
            self._check_subtree(where, request, 'request')
        if reply is None:
            return
        if op.type == 'query':
            self._check_subtree(where, reply, 'query reply')
            self._check_reentry(where, reply)
        elif op.type == 'event':
            self._check_subtree(where, reply, 'event')
        else:
            self._check_subtree(where, reply, 'action reply')

    def _check_reentry(self, where: str, root: Node) -> None:
        """A repeating level cannot also be widened by a descendant."""
        if not _reenters_anywhere(root) or not _extensions_anywhere(root):
            return
        raise CtlvError(f'{where} query reply',
                        're-enters a record and holds an extension point, '
                        'and a descendant cannot widen a level that repeats')

    def _check_fills(self, fills: tuple[Fill, ...]) -> None:
        """Hold each fill to every place its extension point sits.

        A fill nested in another is met after it, so its places are known.
        """
        for fill in fills:
            where = self.declared[fill.name][1]
            for site, context, level in self.sites.get(fill.name, []):
                self._check_subtree(f'{where} in {site}', fill.root, context,
                                    level)

    def _check_subtree(self, where: str, node: Node, context: str,
                       level: int = 0) -> None:
        """Hold a tree to its context and note where its extensions sit.

        level is how many records deep node is, the root being 0.
        """
        if node.field is not None:
            name = node.field.full_name
            if (node.max_occurrences == spec.UNLIMITED and
                    context != 'query reply'):
                raise CtlvError(f'{where} {context}',
                                f'{name} has no count limit, which only a '
                                'query reply may contain')
            if node.reenters is not None and context != 'query reply':
                raise CtlvError(f'{where} {context}',
                                f'{name} re-enters {node.reenters}, so it '
                                'has no maximum length, which only a query '
                                'reply may contain')
            if node.field.decl.kind == 'blob' and context != 'request':
                raise CtlvError(f'{where} {context}',
                                f'{name} is a blob, which only a request '
                                'may contain')
            if (node.field.decl.kind == 'record' and
                    level > sizes.MAX_DECLARED_RECORD_DEPTH):
                raise CtlvError(f'{where} {context}',
                                f'{name} goes deeper than the '
                                f'{sizes.MAX_DECLARED_RECORD_DEPTH} level '
                                'limit')
        for extension in node.extensions:
            self.sites.setdefault(extension.name, []).append(
                (where, context, level))
        for child in node.children():
            self._check_subtree(where, child, context, level + 1)


def _record_order(record: Record) -> tuple[int, bool, int, int, int]:
    """Named records first, each before the records it holds, then ops."""
    kind = record.record_id >> 30
    index = record.record_id >> 16 & MAX_RECORD_INDEX
    return (record.family_id, kind != TYPE_KIND_NAMED, index, kind,
            record.record_id & 0xffff)


def _reenters_anywhere(node: Node) -> bool:
    return any(child.reenters is not None or _reenters_anywhere(child)
               for child in node.children())


def _extensions_anywhere(node: Node) -> bool:
    return bool(node.extensions) or any(_extensions_anywhere(child)
                                        for child in node.children())


def composed_members(family: Family, node: Node) -> list[tuple[Node, bool]]:
    """The children of node with the fills of its extension points."""
    members = [(child, False) for child in node.required_children]
    members += [(child, True) for child in node.optional_children]
    for extension in node.extensions:
        fill = family.fill_of(extension)
        if fill is not None:
            members += composed_members(family, fill.root)
    return members


def _check_composed(family: Family) -> None:
    def walk_composed(where: str, node: Node) -> int:
        members = composed_members(family, node)
        required = sum(not optional for _child, optional in members)
        optional = len(members) - required

        _check_count(where, 'composed required fields', required,
                     spec.U32_MAX)
        _check_count(where, 'composed optional fields', optional,
                     spec.U32_MAX)
        count = 1
        for child, _optional in members:
            count += walk_composed(where, child)
        return count

    for op in family.ops:
        total = 0
        for which, root in (('request', op.request), ('reply', op.reply)):
            if root is None:
                continue
            where = f'{family.path}: op {op.name} {which}'
            count = walk_composed(where, root)
            _check_count(where, 'composed schema nodes', count,
                         spec.U32_MAX)
            total += count
        _check_count(f'{family.path}: op {op.name}',
                     'composed request and reply nodes', total,
                     spec.U32_MAX)


def _check_docs(family: spec.FamilySpec) -> None:
    """Require the comment that documents what a family declares.

    The values of a set a header numbers are the header's to describe.
    """
    _check_doc(f'{family.path}: family {family.name}', family.doc)
    for values in family.enums + family.flags:
        _check_doc(values.where, values.doc)
        if values.header:
            continue
        for value in values.values:
            if not value.doc:
                raise CtlvError(value.where, 'needs a comment of its own '
                                             'right above it, so the values '
                                             'go one to a line')
    for record in family.records:
        _check_doc(record.where, record.doc)
        if record.body is not None:
            _check_body_docs(record.body)
    for op in family.ops:
        _check_doc(op.where, op.doc)
        for message in (op.request, op.reply):
            if message is not None:
                _check_body_docs(message.body)


def _check_body_docs(body: spec.Body) -> None:
    for item in body.items:
        if isinstance(item, spec.FieldDecl):
            _check_doc(item.where, item.doc)
            if item.body is not None:
                _check_body_docs(item.body)


def _check_doc(where: str, doc: str | None) -> None:
    if not doc:
        raise CtlvError(where, 'needs a comment right above it, which is its '
                               'documentation')


def _node_count(node: Node) -> int:
    return 1 + sum(_node_count(child) for child in node.children())


def _extension_count(node: Node) -> int:
    return len(node.extensions) + sum(_extension_count(child)
                                      for child in node.children())


def _check_tree_size(where: str, root: Node) -> None:
    _check_count(where, 'schema nodes', _node_count(root), spec.U32_MAX)
    _check_count(where, 'schema extension points', _extension_count(root),
                 spec.U32_MAX)
