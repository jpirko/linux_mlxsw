# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The UAPI header of one family.

Names for numbers: complete attr and op IDs, group and attr-set IDs, enum
values, flag bits and units. The wire format is the hand-written transport
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

    writer.define(names.family_id(), str(family.family_id))
    writer.line()

    _definitions(writer, family, names)
    _groups(writer, family, names)
    _sets(writer, family, names)
    _ops(writer, family, names)

    writer.close_guard(guard)
    return writer.text()


def _includes(family):
    headers = ['<linux/ctlv.h>']
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
    writer.line(f'enum {names.definition(definition)} {{')
    for value_name, number in definition.values:
        writer.line(f'\t{names.value(definition, value_name)} = {number},')
    writer.line('};')


def _flags(writer, names, definition):
    writer.comment(f'flags {definition.name}')
    mask = 0
    for value_name, bit in definition.values:
        writer.define(names.value(definition, value_name), f'_BITULL({bit})')
        mask |= 1 << bit
    writer.define(names.value_mask(definition), f'{mask:#x}ULL')


def _groups(writer, family, names):
    for group in _own(family, family.groups.values()):
        writer.comment(f'attr group {group.name}')
        writer.define(names.group(group), str(group.group_id))
        writer.line()
        for attr in group.attrs:
            _attr(writer, names, group, attr)
        writer.line()


def _attr(writer, names, group, attr):
    writer.define_call(names.attr(attr), 'CTLV_ATTR_ID', names.family_id(),
                       names.group(group), str(attr.local_id))
    if attr.decl.unit is None:
        return
    writer.define(names.attr_unit(attr),
                  f'CTLV_UNIT_{cgen.macro(attr.decl.unit)}')
    writer.define(names.attr_scale(attr), str(attr.decl.scale))


def _sets(writer, family, names):
    own = _own(family, family.sets.values())
    if not own:
        return
    writer.comment('attr sets')
    for attr_set in own:
        writer.define(names.attr_set(attr_set), str(attr_set.set_id))
    writer.line()


def _ops(writer, family, names):
    own = _own(family, family.ops)
    if not own:
        return
    writer.comment('ops')
    for op in own:
        writer.define_call(names.op(op), 'CTLV_OP_ID', names.family_id(),
                           str(op.local_id))
    writer.line()
