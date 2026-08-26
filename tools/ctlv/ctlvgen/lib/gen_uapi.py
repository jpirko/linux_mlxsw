# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The UAPI header of one family.

Names for numbers: the type of every field and the ID of every op, complete,
enum values, flag bits and units. The message format is the transport
header's. An inherited declaration is not repeated, already having a name in
the header of the family that owns it.
"""

from __future__ import annotations

import textwrap
from collections.abc import Iterable

from . import cgen
from . import semantics
from . import spec


def render(family: semantics.Family) -> str:
    """Return the UAPI header of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    guard = names.uapi_guard()

    writer = cgen.preamble(family, 'uapi', 'header')
    _file_comment(writer, family)
    writer.line()
    writer.open_guard(guard)
    writer.includes(*_includes(family))

    _definitions(writer, family, names)
    _records(writer, family, names)
    _ops(writer, family, names)

    writer.close_guard(guard)
    return writer.text()


def render_families(registry: spec.Registry) -> str:
    """Return the family-number header of one registry."""
    guard = '_UAPI_CTLV_CTLV_FAMILIES_GEN_H'
    values = [(cgen.family_const(entry.name), family_id, None)
              for family_id, entry in enumerate(registry.entries)]
    writer = cgen.preamble(registry, 'uapi', 'header', registry.path)

    writer.comment('CTLV family numbers.')
    writer.line()
    writer.open_guard(guard)
    _numbered_enum(writer, '_ctlv_family', values)
    writer.line()
    writer.close_guard(guard)
    return writer.text()


def _includes(family: semantics.Family) -> list[str]:
    headers = ['<ctlv/ctlv.h>', cgen.families_uapi_header()]
    if family.parent:
        # Inherited constants live in the parent's header, so one include is
        # enough for the whole effective schema.
        headers.append(cgen.uapi_header(family.parent))
    own = family.local_definitions
    if any(definition.kind == 'flags' for definition in own):
        # _BITULL(), which is the form of BIT_ULL() that userspace has.
        headers.append('<linux/const.h>')
    headers += [f'<{definition.header.path}>' for definition in own
                if definition.header]
    return sorted(set(headers))


def _file_comment(writer: cgen.Writer, family: semantics.Family) -> None:
    paragraphs = [
        f'Constants of the {family.name} CTLV family. Types and op IDs are '
        'complete; the message format carrying them is in <ctlv/ctlv.h>.',
    ]
    if family.parent:
        paragraphs.append(
            f'The inherited {family.parent} header is included here, so one '
            'include covers the whole effective schema.')
    lines: list[str] = []
    for paragraph in paragraphs:
        if lines:
            lines.append('')
        lines += textwrap.wrap(paragraph, cgen.COMMENT_WIDTH,
                               break_on_hyphens=False)
    writer.comment(*lines)


def _definitions(writer: cgen.Writer, family: semantics.Family,
                 names: cgen.Names) -> None:
    for definition in family.local_definitions:
        if definition.header or definition.is_extension_point:
            continue
        if definition.kind == 'enum':
            _enum(writer, names, definition)
        else:
            _flags(writer, names, definition)
        writer.line()


def _enum(writer: cgen.Writer, names: cgen.Names,
          definition: semantics.Definition) -> None:
    if definition.extension is not None:
        owner = names.effective.owners[definition.extension.family_id]
        writer.comment(f'enum {definition.name}, an extension point of '
                       f'{owner}')
    else:
        writer.comment(f'enum {definition.name}')
    values = [(names.value(definition, value_name), number, doc)
              for (value_name, number), doc in zip(definition.values,
                                                   definition.value_docs)]
    _numbered_enum(writer, names.definition(definition), values)


def _flags(writer: cgen.Writer, names: cgen.Names,
           definition: semantics.Definition) -> None:
    writer.comment(f'flags {definition.name}')
    bits = [(names.value_bit(definition, value_name), bit, None)
            for value_name, bit in definition.values]
    _numbered_enum(writer, names.bit_enum(definition), bits)
    writer.line()
    values = []
    for (value_name, _bit), doc in zip(definition.values,
                                       definition.value_docs):
        value = names.value(definition, value_name)
        writer.doc(doc)
        writer.define(value,
                      f'_BITULL({names.value_bit(definition, value_name)})')
        values.append(value)
    _flags_mask(writer, names.value_mask(definition), values)


def _records(writer: cgen.Writer, family: semantics.Family,
             names: cgen.Names) -> None:
    """One #define per field, under the record that holds it."""
    for record in family.local_records:
        if not record.fields:
            continue
        writer.comment(record.path)
        for field in record.fields:
            writer.doc(field.decl.doc)
            writer.define_block(
                names.field(field),
                f'\tCTLV_TYPE({names.family_const()}, '
                f'{record.record_id:#010x}, {field.number})')
            if field.decl.unit is not None:
                writer.define(names.field_unit(field),
                              f'CTLV_UNIT_{cgen.macro(field.decl.unit)}')
                writer.define(names.field_scale(field), str(field.decl.scale))
        writer.line()


def _numbered_enum(writer: cgen.Writer, name: str,
                   values: Iterable[tuple[str, int, str | None]]) -> None:
    # Every member says its value, so that none moves with its neighbours.
    writer.line(f'enum {name} {{')
    for value, number, doc in values:
        writer.doc(doc, '\t')
        writer.line(f'\t{value} = {number},')
    writer.line('};')


def _flags_mask(writer: cgen.Writer, name: str, values: list[str]) -> None:
    if len(values) == 1:
        writer.define(name, values[0])
        return
    lines = [f'\t({values[0]} |']
    lines += [f'\t {value} |' for value in values[1:-1]]
    lines.append(f'\t {values[-1]})')
    writer.define_block(name, *lines)


def _ops(writer: cgen.Writer, family: semantics.Family,
         names: cgen.Names) -> None:
    own = family.local_ops
    if not own:
        return
    writer.comment('ops')
    for op in own:
        writer.define_call(names.op_id(op), 'CTLV_OP', names.family_const(),
                           str(op.local_id))
    writer.line()
