# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Deterministic text rendering of an effective family.

A debugging window and a golden-test surface, not an interchange format: every
generated ID, effective bound and computed length, in declaration order.
"""

from __future__ import annotations

from . import semantics
from . import sizes
from . import spec

INDENT: str = '  '


def render(family: semantics.Family) -> str:
    """Return the dump of one effective family as a string."""
    lines = []
    lines.append(f'family: {family.name}')
    lines.append(f'family-id: {family.family_id}')
    lines.append('chain: ' + ' -> '.join(item.name for item in family.chain))

    _definitions(lines, family)
    _records(lines, family)
    _extensions(lines, family)
    _ops(lines, family)
    return '\n'.join(lines) + '\n'


def _definitions(lines: list[str], family: semantics.Family) -> None:
    if not family.definitions:
        return
    lines.append('')
    lines.append('definitions:')
    for definition in family.definitions.values():
        extension = definition.extension
        what = ''
        if extension is not None:
            what = f', extension {extension.ext_id} of family ' \
                   f'{extension.family_id}'
        lines.append(f'{INDENT}{definition.kind} {definition.name} '
                     f'(family {definition.family_id}{what})')
        for name, number in definition.values:
            if definition.kind == 'enum':
                lines.append(f'{INDENT * 2}{name} = {number}')
            else:
                lines.append(f'{INDENT * 2}{name} = bit {number}')


def _records(lines: list[str], family: semantics.Family) -> None:
    if not family.records:
        return
    lines.append('')
    lines.append('records:')
    for record in family.records.values():
        lines.append(f'{INDENT}{record.path} = {record.record_id:#010x} '
                     f'{record.role} (family {record.family_id})')
        for member in record.members:
            field = member.field
            requirement = '' if member.required else '?'
            lines.append(f'{INDENT * 2}{field.name}{requirement} = '
                         f'{_id(field.type)} {_field_detail(field)}')


def _field_detail(field: semantics.Field) -> str:
    decl = field.decl
    detail = [decl.kind]
    if decl.kind in ('uint', 'sint'):
        detail.append(f'[{decl.min}, {decl.max}]')
        if decl.unit is not None:
            detail.append(f'unit {decl.unit} scale {decl.scale}')
    elif decl.kind in ('string', 'binary'):
        detail.append(f'length [{decl.min_length}, {decl.max_length}]')
    elif decl.kind in ('enum', 'flags'):
        assert decl.ref is not None
        detail.append(decl.ref)
    elif decl.kind == 'blob':
        assert decl.blob is not None
        detail.append(decl.blob.direction)
        detail.append('backings ' + ','.join(decl.blob.backings))
        detail.append(f'max-length {decl.blob.max_length}')
        detail.append(f'alignment {decl.blob.alignment}')
    elif decl.extension is not None:
        detail.append(f'extension {decl.extension}')
    else:
        detail.append(f'record {field.holds}')
    detail.append(_occurrences(decl.max_occurrences))
    return ' '.join(detail)


def _extensions(lines: list[str], family: semantics.Family) -> None:
    if not family.extensions:
        return
    lines.append('')
    lines.append('extensions:')
    for extension in family.extensions.values():
        fill = family.fill_of(extension)
        filled = f', filled by family {fill.owner_family_id}' if fill else ''
        lines.append(f'{INDENT}{extension.name} = {extension.ext_id} '
                     f'(family {extension.family_id}{filled})')


def _ops(lines: list[str], family: semantics.Family) -> None:
    if not family.ops:
        return
    lines.append('')
    lines.append('ops:')
    for op in family.ops:
        lines.append(f'{INDENT}{op.name} = {_id(op.op_id)} {op.type}')
        lines.append(f'{INDENT * 2}fd-access: {op.fd_access}')
        if op.request is not None:
            _tree(lines, 'request', op.request)
        if op.reply is None:
            lines.append(f'{INDENT * 2}reply: none')
        else:
            _tree(lines, 'reply', op.reply)
        _sizes(lines, op.sizes)


def _tree(lines: list[str], what: str, root: semantics.Node) -> None:
    lines.append(f'{INDENT * 2}{what}:')
    if not root.children():
        lines.append(f'{INDENT * 3}(empty)')
        return
    _children(lines, root, 3)


def _children(lines: list[str], node: semantics.Node, depth: int) -> None:
    for child in node.children():
        requirement = 'required' if child.required else 'optional'
        line = (f'{INDENT * depth}{child.child_field.full_name} '
                f'{_occurrences(child.max_occurrences)} {requirement}')
        if child.reenters is not None:
            line += f' -> {child.reenters}'
        lines.append(line)
        _children(lines, child, depth + 1)


def _sizes(lines: list[str], op_sizes: sizes.OpSizes) -> None:
    lines.append(f'{INDENT * 2}sizes:')
    reported = (
        ('request-max-len', op_sizes.request_max_len),
        ('action-reply-max-len', op_sizes.action_reply_max_len),
        ('query-reply-min-len', op_sizes.query_reply_min_len),
        ('query-reply-suggested-len', op_sizes.query_reply_suggested_len),
        ('error-info-max-len', op_sizes.error_info_max_len),
    )
    for name, value in reported:
        if value:
            lines.append(f'{INDENT * 3}{name}: {value}')


def _occurrences(max_occurrences: int) -> str:
    if max_occurrences == spec.UNLIMITED:
        return 'x*'
    return f'x{max_occurrences}'


def _id(value: int) -> str:
    return f'0x{value:016x}'
