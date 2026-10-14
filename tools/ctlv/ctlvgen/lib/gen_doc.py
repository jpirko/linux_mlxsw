# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The reference documentation of one family.

Each op's request and reply as the tree the core admits, with every field's
number, schema kind, bounds and doc, and what each descendant puts into an
extension point shown where it lands. An inherited declaration is documented
by the family that owns it.
"""

from __future__ import annotations

import textwrap
from typing import Union

from . import cgen
from . import semantics
from . import spec

# Documentation/doc-guide/sphinx.rst asks for lines that fit a terminal, which
# is why a generated tree is an indented literal block.
LINE_LIMIT: int = 79

# A literal block's indent, the columns of one tree row inside it, and how far
# in from its row a doc goes.
BLOCK_INDENT: int = 4
NAME_WIDTH: int = 30
NUMBER_WIDTH: int = 4
DOC_INDENT: int = 4

TYPES: dict[str, str] = {
    'action': 'action',
    'query': 'query',
    'event': 'event',
}

# A blob's direction as a spec writes it.
DIRECTIONS: dict[str, str] = {value: key
                              for key, value in spec.DIRECTIONS.items()}


def render(family: semantics.Family,
           descendants: tuple[semantics.Family, ...] = ()) -> str:
    """Return the reference page of family as reStructuredText.

    descendants are the families inheriting this one, whose fills appear in
    its trees.
    """
    writer = _Writer()
    _preamble(writer, family)
    _title(writer, family)
    _intro(writer, family)
    _definitions(writer, family)
    _records(writer, family, descendants)
    _extensions(writer, family, descendants)
    _fills(writer, family, descendants)
    _ops(writer, family, descendants)
    _messages(writer, family, descendants)
    return writer.text()


def signatures(family: semantics.Family, width: int = 100) -> str:
    """Return every op this family declares as one call per line."""
    lines = []
    for op in family.local_ops:
        lines += _signature(family, op, width)
    return '\n'.join(lines) + '\n' if lines else ''


class _Writer:
    """The lines of one generated page."""

    def __init__(self) -> None:
        self._lines: list[str] = []

    def line(self, text: str = '') -> None:
        self._lines.append(text.rstrip())

    def lines(self, *texts: str) -> None:
        for text in texts:
            self.line(text)

    def paragraph(self, text: str) -> None:
        """One paragraph, wrapped, followed by a blank line.

        A hyphenated family name is never broken, so a grep for one finds it.
        """
        for line in _wrap(text, LINE_LIMIT):
            self.line(line)
        self.line()

    def title(self, text: str) -> None:
        """The one title of the page, overlined and underlined."""
        self.line('=' * len(text))
        self.line(text)
        self.line('=' * len(text))
        self.line()

    def heading(self, text: str, char: str) -> None:
        """One section heading, underlined the width of its title."""
        self.line(text)
        self.line(char * len(text))
        self.line()

    def block(self, *texts: str) -> None:
        """One literal block, indented, followed by a blank line."""
        self.line('::')
        self.line()
        for text in texts:
            self.line(f'{" " * BLOCK_INDENT}{text}' if text else '')
        self.line()

    def field(self, name: str, value: int | str) -> None:
        self.line(f':{name}: {value}')

    def text(self) -> str:
        while self._lines and not self._lines[-1]:
            self._lines.pop()
        return '\n'.join(self._lines) + '\n'


def _wrap(text: str, width: int) -> list[str]:
    return textwrap.wrap(' '.join(text.split()), width,
                         break_on_hyphens=False, break_long_words=False)


def _preamble(writer: _Writer, family: semantics.Family) -> None:
    """The comment ctlv-regen.sh recovers the generation parameters from."""
    writer.lines(
        f'.. SPDX-License-Identifier: {family.license}',
        f'.. Generated from {cgen.spec_path(family.path)} by',
        f'   {cgen.REGEN}. Do not edit.',
        '.. CTLV-GEN doc rst',
    )
    writer.line()


def _title(writer: _Writer, family: semantics.Family) -> None:
    writer.title(family.name)


def _intro(writer: _Writer, family: semantics.Family) -> None:
    writer.field('Family ID', family.family_id)
    writer.field('Chain', ' -> '.join(item.name for item in family.chain))
    writer.field('Specification', cgen.spec_path(family.path))
    writer.line()
    if family.doc:
        writer.paragraph(family.doc)
    if family.parent:
        writer.paragraph(
            f'This family inherits ``{family.parent}`` and its page describes '
            f'what ``{family.name}`` declares itself, together with the '
            'extension points it fills. An inherited op or field keeps the '
            'name and the description its own family gives it, so a program '
            'speaking to a device of this family uses both pages.')
    else:
        writer.paragraph(
            'This is the family every other family inherits: its ops are the '
            'ones the core answers itself, on every device, whatever '
            'family registered it.')
    writer.paragraph(
        'A tree lists one field per line: its name as the specification '
        'writes it, its number and its schema kind. A field\'s type is the '
        'family, the record above it and that number. A field of records: '
        'is a record of its own, which its line names.')


def _definitions(writer: _Writer, family: semantics.Family) -> None:
    own = family.local_definitions
    if not own:
        return
    writer.heading('Enumerations and flags', '=')
    for definition in own:
        writer.heading(definition.name, '-')
        if definition.doc:
            writer.paragraph(definition.doc)
        if definition.is_extension_point:
            writer.paragraph(
                'An enum extension point: the family of a device gives the '
                'values, by declaring an enum of this name. On a device '
                'whose family gives none, no value is valid.')
            continue
        if definition.extension is not None:
            owner = family.owners[definition.extension.family_id]
            writer.paragraph(f'The values this family gives the enum '
                             f'extension point of {owner}.')
        if definition.kind == 'enum':
            writer.paragraph(
                'A payload restricted to these values. An unknown value is '
                'refused on a request and tolerated on a reply, so a caller '
                'built against an older specification keeps working.')
            header = definition.header
            if header:
                before = f' before ``{header.end}``' if header.end else ''
                writer.paragraph(
                    f'These are the constants of ``enum {header.tag}`` in '
                    f'``<{header.path}>``{before}, under its names and '
                    'numbers. A number between two of them passes CTLV, for '
                    'the family to refuse.')
            rows = _value_rows(definition, '')
        else:
            writer.paragraph(
                'A payload restricted to these bits. An unknown bit is '
                'refused on a request and tolerated on a reply.')
            rows = _value_rows(definition, 'bit ')
        writer.block(*rows)


def _value_rows(definition: semantics.Definition, prefix: str) -> list[str]:
    """One line per value, its doc below."""
    rows = []
    width = LINE_LIMIT - BLOCK_INDENT - DOC_INDENT
    for (name, number), doc in zip(definition.values, definition.value_docs):
        rows.append(f'{prefix}{number} = {name}')
        if doc:
            rows += [f'{" " * DOC_INDENT}{line}' for line in _wrap(doc, width)]
    return rows


def _records(writer: _Writer, family: semantics.Family,
             descendants: tuple[semantics.Family, ...]) -> None:
    """The named records and the fields of records: this family declares."""
    own = [record for record in family.local_records
           if record.role in ('named', 'field')]
    if not own:
        return
    writer.heading('Records', '=')
    writer.paragraph(
        'What an op places by name. A named record is placed with record or '
        'spliced with $include; a field of records: is placed with a bare '
        'record and has one type wherever it is.')
    for record in own:
        writer.heading(record.name, '-')
        if record.doc:
            writer.paragraph(record.doc)
        if record.role == 'field':
            field = record.fields[0]
            writer.block(f'record {record.record_id:#010x}, field '
                         f'{field.number}: {_kind_text(family, field)}')
            continue
        rows = [f'record {record.record_id:#010x}', '']
        rows += _record_rows(family, descendants, record, 0, (record.name,))
        writer.block(*rows)


def _record_rows(family: semantics.Family,
                 descendants: tuple[semantics.Family, ...],
                 record: semantics.Record, depth: int,
                 opened: tuple[str, ...] = ()) -> list[str]:
    """One line per member of a record, in spec order, records expanded.

    opened are the named records above, which a record of one re-enters.
    """
    rows = []
    included = None
    for member in record.members:
        field = member.field
        origin = None
        if field.record_id != record.record_id and not field.shared:
            origin = field.record
        if origin is not None and origin != included:
            source = family.records[origin]
            rows.append(f'{"  " * depth}$include {origin}, record '
                        f'{source.record_id:#010x}')
        included = origin
        inner, reenters = None, False
        if field.decl.kind == 'record' and field.decl.extension is None:
            assert field.holds is not None
            inner = family.records[field.holds]
            reenters = inner.role == 'named' and inner.name in opened
        name = field.name + _suffix(member.required, field.decl)
        rows += _row(family, name, field, depth, _doc(family, member),
                     reenters)
        if inner is not None and not reenters:
            below = opened + ((inner.name,) if inner.role == 'named'
                              else ())
            rows += _record_rows(family, descendants, inner, depth + 1,
                                 below)
        if field.decl.extension is not None:
            rows += _fill_rows(family, descendants,
                               family.extensions[field.decl.extension],
                               depth + 1)
    return rows


def _doc(family: semantics.Family, member: semantics.Member) -> str | None:
    """A member's doc: where it is placed, else where it is declared."""
    if member.doc:
        return member.doc
    if member.field.decl.doc:
        return member.field.decl.doc
    if member.field.shared:
        return family.records[member.field.record].doc
    return None


def _extensions(writer: _Writer, family: semantics.Family,
                descendants: tuple[semantics.Family, ...]) -> None:
    own = family.local_extensions
    if not own:
        return
    writer.heading('Extension points', '=')
    writer.paragraph(
        'A place a descendant adds members to, by declaring a record of the '
        'same name. It is empty on this family\'s own devices; a descendant '
        'that fills it lengthens every message that reaches it.')
    rows = []
    for extension in own:
        fillers = []
        for child in descendants:
            fill = child.fill_of(extension)
            if fill is not None:
                fillers.append(child.owners[fill.owner_family_id])
        filled = ', filled by ' + ', '.join(fillers) if fillers else ''
        rows.append(f'{extension.name}, ID {extension.ext_id}{filled}')
    writer.block(*rows)


def _fills(writer: _Writer, family: semantics.Family,
           descendants: tuple[semantics.Family, ...]) -> None:
    if not family.fills:
        return
    writer.heading('Extension points this family fills', '=')
    writer.paragraph(
        'Each of these completes an extension point an ancestor declared. '
        'What it adds appears in every message of every op that reaches that '
        'point, so an inherited op admits a longer message on a device of '
        'this family than on a device of the family that declared it.')
    for fill in family.fills:
        owner = family.owners.get(fill.family_id, str(fill.family_id))
        writer.heading(f'{fill.name} of {owner}', '-')
        if fill.record.doc:
            writer.paragraph(fill.record.doc)
        rows = [f'record {fill.record.record_id:#010x}', '']
        rows += _record_rows(family, descendants, fill.record, 0)
        writer.block(*rows)


def _ops(writer: _Writer, family: semantics.Family,
         descendants: tuple[semantics.Family, ...]) -> None:
    own = family.local_ops
    if not own:
        return
    writer.heading('Ops', '=')
    writer.paragraph(
        'One section per op this family declares. An action is executed and '
        'acknowledged, a query is executed and answered in as many fragments '
        'as its answer needs, and an event is produced by the device and read '
        'from a subscribed file description. In short:')
    lines = []
    for op in own:
        lines += _signature(family, op, LINE_LIMIT - BLOCK_INDENT)
    writer.block(*lines)
    for op in own:
        _op(writer, family, descendants, op)


def _op(writer: _Writer, family: semantics.Family,
        descendants: tuple[semantics.Family, ...], op: semantics.Op) -> None:
    writer.heading(op.name, '-')
    writer.field('Op ID', f'``{_id(op.op_id)}``, op {op.local_id}')
    writer.field('Type', TYPES[op.type])
    if op.type != 'event':
        writer.field('FD access', _fd_access(op.fd_access))
        writer.field('Availability', _availability(op.availability))
    writer.line()
    if op.doc:
        writer.paragraph(op.doc)
    if op.type == 'event':
        writer.paragraph(
            'An event of this op reaches a file description that subscribed '
            'to it, and is dropped for one that did not.')
        _message(writer, family, descendants, 'Payload', op.reply_record)
    else:
        _message(writer, family, descendants, 'Request', op.request_record)
        _message(writer, family, descendants, 'Reply', op.reply_record)
    _sizes(writer, op)


def _availability(availability: str) -> str:
    if availability == 'dynamic':
        return 'dynamic, so a device may disable and enable it at any time'
    return 'fixed, so a device that publishes it always answers it'


def _fd_access(fd_access: str) -> str:
    if fd_access == 'read':
        return 'read, so an observation or full-control fd may execute it'
    return 'read-write, so only a full-control fd may execute it'


def _message(writer: _Writer, family: semantics.Family,
             descendants: tuple[semantics.Family, ...], what: str,
             record: semantics.Record | None) -> None:
    if record is None:
        writer.paragraph(f'{what}: none.')
        return
    if record.role == 'named':
        writer.paragraph(f'{what}: record {record.name}, '
                         f'{record.record_id:#010x}, as Records has it.')
        return
    if record.doc:
        writer.paragraph(record.doc)
    rows = _record_rows(family, descendants, record, 0)
    if not rows:
        writer.paragraph(f'{what}: an empty root, record '
                         f'{record.record_id:#010x}.')
        return
    writer.paragraph(f'{what}, record {record.record_id:#010x}:')
    writer.block(*rows)


def _fill_rows(family: semantics.Family,
               descendants: tuple[semantics.Family, ...],
               extension: semantics.Extension, depth: int) -> list[str]:
    """What each family below puts into one extension point."""
    rows = []
    for child in (family, *descendants):
        fill = child.fill_of(extension)
        if fill is None or (child is not family and
                            fill.owner_family_id != child.family_id):
            continue
        owner = child.owners[fill.owner_family_id]
        rows.append(f'{"  " * depth}{owner}: record '
                    f'{fill.record.record_id:#010x} of family '
                    f'{fill.owner_family_id}')
        rows += _record_rows(child, descendants, fill.record, depth + 1)
    return rows


def _row(family: semantics.Family, name: str, field: semantics.Field,
         depth: int, doc: str | None, reenters: bool = False) -> list[str]:
    """One field: name, number and schema kind, its doc below."""
    indent = '  ' * depth
    label = f'{indent}{name}'
    number = str(field.number)
    what = _kind_text(family, field, reenters)
    if field.shared:
        what += f', record {field.record_id:#010x}'
    column = NAME_WIDTH + NUMBER_WIDTH
    parts = _wrap(what, LINE_LIMIT - BLOCK_INDENT - column) or ['']
    if len(label) >= NAME_WIDTH:
        rows = [label, f'{" " * NAME_WIDTH}{number:<{NUMBER_WIDTH}}{parts[0]}']
    else:
        rows = [f'{label:<{NAME_WIDTH}}{number:<{NUMBER_WIDTH}}{parts[0]}']
    rows += [f'{" " * column}{part}' for part in parts[1:]]
    if doc:
        doc_indent = indent + ' ' * DOC_INDENT
        width = LINE_LIMIT - BLOCK_INDENT - len(doc_indent)
        rows += [f'{doc_indent}{line}' for line in _wrap(doc, width)]
    return rows


def _suffix(required: bool, decl: spec.FieldDecl) -> str:
    """The [N] and ? a spec writes after a field name."""
    suffix = ''
    if decl.count_enum is not None:
        suffix = f'[{decl.count_enum}]'
    elif decl.max_occurrences == spec.UNLIMITED:
        suffix = '[]'
    elif decl.max_occurrences != 1:
        suffix = f'[{decl.max_occurrences}]'
    return suffix + ('' if required else '?')


def _kind_text(family: semantics.Family, field: semantics.Field,
               reenters: bool = False) -> str:
    """A field's schema kind and bounds, as a spec reads."""
    decl = field.decl
    if decl.kind in ('uint', 'sint'):
        return decl.kind + _bounds(_integer_bounds(decl), _unit(decl))
    if decl.kind == 'bool':
        return 'bool'
    if decl.kind in ('string', 'binary'):
        return decl.kind + _bounds(_length_bounds(decl), ())
    if decl.kind == 'enum':
        assert decl.ref is not None
        return decl.ref
    if decl.kind == 'flags':
        assert decl.ref is not None
        names = ', '.join(name for name, _ in
                          family.definitions[decl.ref].values)
        return f'{decl.ref}: {names}'
    if decl.kind == 'blob':
        assert decl.blob is not None
        return 'blob' + _bounds((), _blob_parts(decl.blob))
    if decl.extension is not None:
        return f'extension {decl.extension}'
    assert field.holds is not None
    record = family.records[field.holds]
    what = f'record {record.record_id:#010x}'
    if record.role == 'named':
        what = f'record {record.name} {record.record_id:#010x}'
    if reenters:
        what += ', which this re-enters'
    return what


def _bounds(range_parts: tuple[str, ...], other_parts: tuple[str, ...]) -> str:
    parts = list(range_parts) + list(other_parts)
    return f' {", ".join(parts)}' if parts else ''


def _integer_bounds(decl: spec.FieldDecl) -> tuple[str, ...]:
    low = None if decl.min == _default_min(decl.kind) else decl.min
    high = None if decl.max == _default_max(decl.kind) else decl.max
    if low is None and high is None:
        return ()
    text = spec.size_text if decl.unit == 'bytes' else spec.number_text
    low_text = text(low) if low is not None else ''
    high_text = text(high) if high is not None else ''
    return (f'{low_text}..{high_text}',)


def _default_min(kind: str) -> int:
    return 0 if kind == 'uint' else spec.S64_MIN


def _default_max(kind: str) -> int:
    return spec.U64_MAX if kind == 'uint' else spec.S64_MAX


def _unit(decl: spec.FieldDecl) -> tuple[str, ...]:
    if decl.unit is None:
        return ()
    parts = [decl.unit]
    if decl.scale:
        parts.append(f'scale {decl.scale}')
    return tuple(parts)


def _length_bounds(decl: spec.FieldDecl) -> tuple[str]:
    assert decl.min_length is not None and decl.max_length is not None
    if decl.min_length == decl.max_length:
        return (spec.size_text(decl.max_length),)
    low = spec.size_text(decl.min_length) if decl.min_length else ''
    return (f'{low}..{spec.size_text(decl.max_length)}',)


def _blob_parts(blob: spec.BlobDecl) -> tuple[str, ...]:
    parts = [DIRECTIONS[blob.direction]]
    if blob.max_length != spec.U32_MAX:
        parts.append(f'..{spec.size_text(blob.max_length)}')
    if blob.alignment != 1:
        parts.append(f'align {spec.size_text(blob.alignment)}')
    parts.append(' '.join(blob.backings))
    return tuple(parts)


def _sizes(writer: _Writer, op: semantics.Op) -> None:
    """What a caller has to bring and what it gets back, in bytes."""
    reported = (
        ('a schema-known request of at most {} bytes',
         op.sizes.request_max_len),
        ('a reply of at most {} bytes', op.sizes.action_reply_max_len),
        ('a reply buffer of at least {} bytes',
         op.sizes.query_reply_min_len),
        ('{} bytes of reply buffer suggested',
         op.sizes.query_reply_suggested_len),
        ('an event of at most {} bytes', op.sizes.event_max_len),
    )
    rows = [label.format(value) for label, value in reported if value]
    if not rows:
        return
    writer.paragraph('Lengths on a device of this family:')
    writer.block(*rows)


def _messages(writer: _Writer, family: semantics.Family,
              descendants: tuple[semantics.Family, ...]) -> None:
    if not family.messages:
        return
    writer.heading('Messages no op declares', '=')
    writer.paragraph(
        'The core writes these itself, so they belong to no op: a '
        'caller reads one out of the buffer it offered for a reply.')
    for message in family.messages:
        writer.heading(message.name, '-')
        record = family.records[message.name]
        writer.block(f'record {message.record_id:#010x}', '',
                     *_record_rows(family, descendants, record, 0,
                                   (record.name,)))
        writer.paragraph(f'At most {message.max_len} bytes, bounded by '
                         f'{message.limit}.')


def _signature(family: semantics.Family, op: semantics.Op,
               width: int) -> list[str]:
    """One op as a call: what it takes, and what it answers."""
    if op.type == 'event':
        assert op.reply_record is not None
        return _layout((f'{op.name} event ', _shape(family, op.reply_record)),
                       width)
    request = _members(family, op.request_record) if op.request_record \
        else []
    call = (f'{op.name}(', request, ')')
    if op.reply_record is None:
        return _layout((call,), width)
    return _layout((call, ' -> ', _shape(family, op.reply_record)), width)


_Piece = Union[str, tuple[str, list['_Piece'], str]]


def _shape(family: semantics.Family, record: semantics.Record) -> _Piece:
    """A message root: a named record by name, else its members."""
    if record.role == 'named':
        return record.name
    members = _members(family, record)
    return ('{ ', members, ' }') if members else '{}'


def _layout(pieces: tuple[_Piece, ...], width: int) -> list[str]:
    """One signature, wrapped inside its braces where it is too long.

    A piece is text or a group, an opener, its members and a closer. A member
    that does not fit goes on a line of its own, below the first one; a reply
    that does not fit goes on an indented line.
    """
    lines = ['']
    for piece in pieces:
        if piece == ' -> ':
            lines[-1] += piece
            continue
        if lines[-1].endswith(' -> ') and \
                len(lines[-1]) + len(_flat(piece)) > width:
            lines[-1] = lines[-1].rstrip()
            lines.append('    ')
        _place(piece, lines, width)
    return lines


def _place(item: _Piece, lines: list[str], width: int) -> None:
    """Append item to lines, wrapping inside it where it does not fit."""
    flat = _flat(item)
    if isinstance(item, str) or len(lines[-1]) + len(flat) <= width:
        lines[-1] += flat
        return
    opener, members, closer = item
    lines[-1] += opener
    inner = len(lines[-1])
    for index, member in enumerate(members):
        tail = closer if index == len(members) - 1 else ','
        if index:
            if len(lines[-1]) + 1 + len(_flat(member)) + len(tail) <= width:
                lines[-1] += ' '
            else:
                lines.append(' ' * inner)
        _place(member, lines, width - len(tail))
        lines[-1] += tail


def _flat(item: _Piece) -> str:
    if isinstance(item, str):
        return item
    opener, members, closer = item
    return opener + ', '.join(_flat(member) for member in members) + closer


def _members(family: semantics.Family,
             record: semantics.Record) -> list[_Piece]:
    """Each member of one record, a record written out in place."""
    parts: list[_Piece] = []
    included = None
    for member in record.members:
        field = member.field
        origin = None
        if field.record_id != record.record_id and not field.shared:
            origin = field.record
        if origin is not None:
            if origin != included:
                parts.append(origin)
            included = origin
            continue
        included = None
        name = field.name + _suffix(member.required, field.decl)
        parts.append(_member(family, field, name))
    return parts


def _member(family: semantics.Family, field: semantics.Field,
            name: str) -> _Piece:
    decl = field.decl
    if decl.kind == 'blob':
        assert decl.blob is not None
        return f'{name}: {DIRECTIONS[decl.blob.direction]} blob'
    if decl.kind != 'record':
        return name
    if decl.extension is not None:
        return f'{name}: {decl.extension}'
    assert field.holds is not None
    record = family.records[field.holds]
    if record.role == 'named':
        return f'{name}: {record.name}'
    return (f'{name}: {{ ', _members(family, record), ' }')


def _id(value: int) -> str:
    return f'0x{value:016x}'
