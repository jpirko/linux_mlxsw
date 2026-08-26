# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The UAPI header of one family.

Names for numbers: complete attr and op IDs, group and attr-set IDs, enum
values, flag bits and units. The wire format is the transport
header's. An inherited declaration is not repeated, already having a name in
the header of the family that owns it.
"""

from __future__ import annotations

import textwrap

from . import cgen

# " * " plus the text has to stay inside the line limit.
COMMENT_WIDTH = cgen.LINE_LIMIT - 3


def render(family):
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
    _groups(writer, family, names)
    _sets(writer, family, names)
    _ops(writer, family, names)

    writer.close_guard(guard)
    return writer.text()


def render_families(registry):
    """Return the family-number header of one registry."""
    guard = '_UAPI_LINUX_CTLV_FAMILIES_GEN_H'
    values = [(cgen.family_const(entry.name), family_id)
              for family_id, entry in enumerate(registry.entries)]
    writer = cgen.preamble(registry, 'uapi', 'header', registry.path)

    writer.comment('CTLV family numbers.')
    writer.line()
    writer.open_guard(guard)
    _sequential_enum(writer, '_ctlv_family', values)
    writer.line()
    writer.close_guard(guard)
    return writer.text()


def _includes(family):
    headers = ['<linux/ctlv.h>', cgen.families_uapi_header()]
    if family.parent:
        # Inherited constants live in the parent's header, so one include is
        # enough for the whole effective schema.
        headers.append(cgen.uapi_header(family.parent))
    if any(definition.kind == 'flags'
           for definition in _own(family, family.definitions.values())):
        # _BITULL(), which is the form of BIT_ULL() that userspace has.
        headers.append('<linux/const.h>')
    return sorted(headers)


def _file_comment(writer, family):
    paragraphs = [
        f'Constants of the {family.name} CTLV family. Attr and op IDs are '
        'complete; the wire format carrying them is in <linux/ctlv.h>.',
    ]
    if family.parent:
        paragraphs.append(
            f'The inherited {family.parent} header is included here, so one '
            'include covers the whole effective schema.')
    lines = []
    for paragraph in paragraphs:
        if lines:
            lines.append('')
        lines += textwrap.wrap(paragraph, COMMENT_WIDTH)
    writer.comment(*lines)


def _own(family, declarations):
    """Only what this family declares itself, in declaration order."""
    return [item for item in declarations
            if item.family_id == family.family_id]


def _definitions(writer, family, names):
    for definition in _own(family, family.definitions.values()):
        if definition.kind == 'enum':
            _enum(writer, names, definition)
        else:
            _flags(writer, names, definition)
        writer.line()


def _enum(writer, names, definition):
    writer.comment(f'enum {definition.name}')
    values = [(names.value(definition, value_name), number)
              for value_name, number in definition.values]
    _sequential_enum(writer, names.definition(definition), values)


def _flags(writer, names, definition):
    writer.comment(f'flags {definition.name}')
    bits = [(names.value_bit(definition, value_name), bit)
            for value_name, bit in definition.values]
    _sequential_enum(writer, names.bit_enum(definition), bits)
    writer.line()
    values = []
    for value_name, _bit in definition.values:
        value = names.value(definition, value_name)
        writer.define(value,
                      f'_BITULL({names.value_bit(definition, value_name)})')
        values.append(value)
    _flags_mask(writer, names.value_mask(definition), values)


def _groups(writer, family, names):
    groups = _own(family, family.groups.values())
    if not groups:
        return
    writer.comment('attr groups')
    _sequential_enum(writer, names.group_enum(groups[0]),
                     [(names.group(group), group.group_id)
                      for group in groups])
    writer.line()
    writer.define_call(
        f'{names.family_attr(groups[0])}(_group, _attr)', 'CTLV_ATTR',
        names.family_const(), '(_group)', '(_attr)')
    writer.line()
    for group in groups:
        if not group.attrs:
            continue
        writer.comment(f'attrs of group {group.name}')
        writer.define_call(f'{names.group_attr(group)}(_attr)',
                           names.family_attr(group),
                           names.group(group),
                           f'{names.local_attr_prefix(group)}##_attr')
        writer.line()
        _sequential_enum(writer, names.group_attr_enum(group),
                         [(names.local_attr(attr), attr.local_id)
                          for attr in group.attrs])
        writer.line()
        for attr in group.attrs:
            _attr(writer, names, group, attr)
        _attr_metadata(writer, names, group)
        writer.line()


def _attr(writer, names, group, attr):
    writer.define_call(names.attr(attr), names.group_attr(group),
                       cgen.macro(attr.name))


def _attr_metadata(writer, names, group):
    attrs = [attr for attr in group.attrs if attr.decl.unit is not None]
    if not attrs:
        return
    writer.line()
    writer.comment(f'metadata of group {group.name}')
    writer.line(f'enum {names.group_metadata_enum(group)} {{')
    for attr in attrs:
        writer.line(f'\t{names.attr_unit(attr)} = '
                    f'CTLV_UNIT_{cgen.macro(attr.decl.unit)},')
        writer.line(f'\t{names.attr_scale(attr)} = {attr.decl.scale},')
    writer.line('};')


def _sequential_enum(writer, name, values):
    first = values[0][1]

    assert all(number == first + index
               for index, (_value, number) in enumerate(values))
    writer.line(f'enum {name} {{')
    for index, (value, number) in enumerate(values):
        assignment = f' = {number}' if not index and number else ''
        writer.line(f'\t{value}{assignment},')
    writer.line('};')


def _flags_mask(writer, name, values):
    if len(values) == 1:
        writer.define(name, values[0])
        return
    lines = [f'\t({values[0]} |']
    lines += [f'\t {value} |' for value in values[1:-1]]
    lines.append(f'\t {values[-1]})')
    writer.define_block(name, *lines)


def _sets(writer, family, names):
    own = _own(family, family.sets.values())
    if not own:
        return
    writer.comment('attr sets')
    _sequential_enum(writer, names.set_enum(own[0]),
                     [(names.attr_set(attr_set), attr_set.set_id)
                      for attr_set in own])
    writer.line()


def _ops(writer, family, names):
    own = _own(family, family.ops)
    if not own:
        return
    writer.comment('ops')
    _sequential_enum(writer, names.op_enum(own[0]),
                     [(names.op(op), op.local_id)
                      for op in own])
    writer.line()
    writer.define_call(f'{names.operation(own[0])}(_op)',
                       'CTLV_OP', names.family_const(),
                       f'{names.op_prefix(own[0])}##_op')
    for op in own:
        writer.define_call(names.op_id(op), names.operation(op),
                           cgen.macro(op.name))
    writer.line()
