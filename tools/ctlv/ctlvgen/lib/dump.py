# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Deterministic text rendering of an effective family.

A debugging window and a golden-test surface, not an interchange format: every
generated ID, effective bound and computed length, in declaration order.
"""

from __future__ import annotations

from . import spec

INDENT = '  '


def render(family):
    """Return the dump of one effective family as a string."""
    lines = []
    lines.append(f'family: {family.name}')
    lines.append(f'family-id: {family.family_id}')
    lines.append('chain: ' + ' -> '.join(item.name for item in family.chain))

    _definitions(lines, family)
    _groups(lines, family)
    _sets(lines, family)
    _ops(lines, family)
    return '\n'.join(lines) + '\n'


def _definitions(lines, family):
    if not family.definitions:
        return
    lines.append('')
    lines.append('definitions:')
    for definition in family.definitions.values():
        lines.append(f'{INDENT}{definition.kind} {definition.name} '
                     f'(family {definition.family_id})')
        for name, number in definition.values:
            if definition.kind == 'enum':
                lines.append(f'{INDENT * 2}{name} = {number}')
            else:
                lines.append(f'{INDENT * 2}{name} = bit {number}')


def _groups(lines, family):
    if not family.groups:
        return
    lines.append('')
    lines.append('attr-groups:')
    for group in family.groups.values():
        lines.append(f'{INDENT}{group.name} (family {group.family_id}, '
                     f'group {group.group_id})')
        for attr in group.attrs:
            lines.append(f'{INDENT * 2}{attr.full_name} = '
                         f'{_id(attr.attr_id)} {_attr_detail(attr.decl)}')


def _attr_detail(decl):
    detail = [decl.kind]
    if decl.kind in ('uint', 'sint'):
        detail.append(f'[{decl.min}, {decl.max}]')
        if decl.unit is not None:
            detail.append(f'unit {decl.unit} scale {decl.scale}')
    elif decl.kind in ('string', 'binary'):
        detail.append(f'length [{decl.min_length}, {decl.max_length}]')
    elif decl.kind in ('enum', 'flags'):
        detail.append(decl.ref)
    elif decl.kind == 'blob':
        detail.append(decl.blob.direction)
        detail.append('backings ' + ','.join(decl.blob.backings))
        detail.append(f'max-length {decl.blob.max_length}')
        detail.append(f'alignment {decl.blob.alignment}')
    else:
        detail.append('attrs ' + ','.join(decl.attrs))
        if decl.optional_attrs:
            detail.append('optional-attrs ' + ','.join(decl.optional_attrs))
    detail.append(_occurrences(decl.max_occurrences))
    return ' '.join(detail)


def _sets(lines, family):
    if not family.sets:
        return
    lines.append('')
    lines.append('attr-sets:')
    for attr_set in family.sets.values():
        suffix = ' stub' if attr_set.stub else ''
        lines.append(f'{INDENT}{attr_set.name} (family '
                     f'{attr_set.family_id}, set {attr_set.set_id})'
                     f'{suffix}')
        if attr_set.attrs:
            lines.append(f'{INDENT * 2}attrs: ' + ', '.join(attr_set.attrs))
        if attr_set.optional_attrs:
            lines.append(f'{INDENT * 2}optional-attrs: ' +
                         ', '.join(attr_set.optional_attrs))


def _ops(lines, family):
    if not family.ops:
        return
    lines.append('')
    lines.append('ops:')
    for op in family.ops:
        lines.append(f'{INDENT}{op.name} = {_id(op.op_id)} {op.type}')
        if op.request is not None:
            _tree(lines, 'request', op.request)
        if op.reply is None:
            lines.append(f'{INDENT * 2}reply: none')
        else:
            _tree(lines, 'reply', op.reply)
        _sizes(lines, op.sizes)


def _tree(lines, what, root):
    lines.append(f'{INDENT * 2}{what}:')
    if not root.children():
        lines.append(f'{INDENT * 3}(empty)')
        return
    _children(lines, root, 3)


def _children(lines, node, depth):
    for child in node.children():
        requirement = 'required' if child.required else 'optional'
        line = (f'{INDENT * depth}{child.attr.full_name} '
                f'{_occurrences(child.max_occurrences)} {requirement}')
        if child.reenters is not None:
            line += f' -> {child.reenters}'
        lines.append(line)
        _children(lines, child, depth + 1)


def _sizes(lines, op_sizes):
    lines.append(f'{INDENT * 2}sizes:')
    reported = (
        ('request-max-len', op_sizes.request_max_len),
        ('action-reply-max-len', op_sizes.action_reply_max_len),
        ('query-reply-min-len', op_sizes.query_reply_min_len),
        ('query-reply-suggested-len', op_sizes.query_reply_suggested_len),
        ('event-record-max-len', op_sizes.event_record_max_len),
    )
    for name, value in reported:
        if value:
            lines.append(f'{INDENT * 3}{name}: {value}')


def _occurrences(max_occurrences):
    if max_occurrences == spec.UNLIMITED:
        return 'x*'
    return f'x{max_occurrences}'


def _id(value):
    return f'0x{value:016x}'
