# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The reference documentation of one family.

Complete IDs, the payload each attr carries, the bounds it is checked against
and the tree the framework admits, from the same specification the headers and
the kernel metadata come from. An inherited declaration is documented by the
family that owns it.
"""

from __future__ import annotations

import textwrap

from . import cgen
from . import spec

# Documentation/doc-guide/sphinx.rst asks for lines that fit a terminal, which
# is why a generated table is an indented literal block.
LINE_LIMIT = 79

# What each primary form is called in prose, spelled as the specification does.
KINDS = {
    'uint': 'unsigned integer',
    'sint': 'signed integer',
    'bool': 'boolean',
    'string': 'string',
    'binary': 'binary',
    'enum': 'enumeration',
    'flags': 'flags',
    'nested': 'nest',
    'blob': 'blob',
}

TYPES = {
    'action': 'action',
    'query': 'query',
    'event': 'event',
}


def render(family):
    """Return the reference page of family as reStructuredText."""
    writer = _Writer()
    _preamble(writer, family)
    _title(writer, family)
    _intro(writer, family)
    _definitions(writer, family)
    _groups(writer, family)
    _sets(writer, family)
    _fills(writer, family)
    _ops(writer, family)
    _messages(writer, family)
    return writer.text()


class _Writer:
    """The lines of one generated page."""

    def __init__(self):
        self._lines = []

    def line(self, text=''):
        self._lines.append(text.rstrip())

    def lines(self, *texts):
        for text in texts:
            self.line(text)

    def paragraph(self, text):
        """One paragraph, wrapped, followed by a blank line.

        A hyphenated family name is never broken, so a grep for one finds it.
        """
        for line in textwrap.wrap(text, LINE_LIMIT, break_on_hyphens=False,
                                  break_long_words=False):
            self.line(line)
        self.line()

    def title(self, text):
        """The one title of the page, overlined and underlined."""
        self.line('=' * len(text))
        self.line(text)
        self.line('=' * len(text))
        self.line()

    def heading(self, text, char):
        """One section heading, underlined the width of its title."""
        self.line(text)
        self.line(char * len(text))
        self.line()

    def block(self, *texts):
        """One literal block, indented, followed by a blank line."""
        self.line('::')
        self.line()
        for text in texts:
            self.line(f'    {text}' if text else '')
        self.line()

    def field(self, name, value):
        self.line(f':{name}: {value}')

    def text(self):
        while self._lines and not self._lines[-1]:
            self._lines.pop()
        return '\n'.join(self._lines) + '\n'


def _preamble(writer, family):
    """The comment ctlv-regen.sh recovers the generation parameters from."""
    writer.lines(
        f'.. SPDX-License-Identifier: {family.license}',
        f'.. Do not edit, regenerate with {cgen.REGEN}, from:',
        f'..\t{cgen.spec_path(family.path)}',
        '.. CTLV-GEN doc rst',
    )
    writer.line()


def _title(writer, family):
    writer.title(family.name)


def _intro(writer, family):
    writer.field('Family ID', family.family_id)
    writer.field('Chain', ' -> '.join(item.name for item in family.chain))
    writer.field('Specification', cgen.spec_path(family.path))
    writer.line()
    if family.parent:
        writer.paragraph(
            f'This family inherits ``{family.parent}`` and its page describes '
            f'what ``{family.name}`` declares itself, together with the '
            'extension points it fills. An inherited op or attr keeps the '
            'name and the description its own family gives it, so a program '
            'speaking to a device of this family uses both pages.')
    else:
        writer.paragraph(
            'This is the family every other family inherits: its ops are the '
            'ones the framework answers itself, on every device, whatever '
            'family registered it.')


def _definitions(writer, family):
    own = family.local_definitions
    if not own:
        return
    writer.heading('Enumerations and flags', '=')
    for definition in own:
        writer.heading(definition.name, '-')
        if definition.kind == 'enum':
            writer.paragraph(
                'A payload restricted to these values. An unknown value is '
                'refused on a request and tolerated on a reply, so a caller '
                'built against an older specification keeps working.')
            rows = [f'{number} = {name}' for name, number in
                    definition.values]
        else:
            writer.paragraph(
                'A payload restricted to these bits. An unknown bit is '
                'refused on a request and tolerated on a reply.')
            rows = [f'bit {number} = {name}' for name, number in
                    definition.values]
        writer.block(*rows)


def _groups(writer, family):
    own = family.local_groups
    if not own:
        return
    writer.heading('Attrs', '=')
    writer.paragraph(
        'One complete attr ID per line, with the payload it carries and what '
        'the framework checks it against before a callback sees it. The '
        'bounds are ABI: widening one is a new attr rather than a new bound.')
    for group in own:
        writer.heading(f'group {group.name}', '-')
        rows = []
        for attr in group.attrs:
            rows.append(f'{attr.full_name}')
            rows.append(f'    id      {_id(attr.attr_id)}')
            for label, value in _attr_detail(attr.decl):
                rows.append(f'    {label:<8}{value}')
        writer.block(*rows)


def _attr_detail(decl):
    """The documented properties of one attr, label and value each."""
    detail = [('payload', KINDS[decl.kind])]
    if decl.kind in ('uint', 'sint'):
        detail.append(('range', f'{decl.min} to {decl.max}'))
        if decl.unit is not None:
            detail.append(('unit', f'{decl.unit}, scale {decl.scale}'))
    elif decl.kind in ('string', 'binary'):
        detail.append(('length', f'{decl.min_length} to {decl.max_length}'))
    elif decl.kind in ('enum', 'flags'):
        detail.append(('values', decl.ref))
    elif decl.kind == 'blob':
        detail.append(('direction', decl.blob.direction))
        detail.append(('backings', ', '.join(decl.blob.backings)))
        detail.append(('length', f'up to {decl.blob.max_length}'))
        detail.append(('align', str(decl.blob.alignment)))
    elif decl.kind == 'nested':
        detail.append(('members', ', '.join(decl.attrs)))
        if decl.optional_attrs:
            detail.append(('optional', ', '.join(decl.optional_attrs)))
    if decl.max_occurrences != 1:
        detail.append(('repeats', _occurrences(decl.max_occurrences)))
    return detail


def _sets(writer, family):
    own = family.local_sets
    stubs = [item for item in own if item.stub]
    sets = [item for item in own if not item.stub]
    if not own:
        return
    writer.heading('Attr sets', '=')
    writer.paragraph(
        'A set is a named group of members that an op selects for a message '
        'or a nest selects for a level, so one shape can be used in more '
        'than one place.')
    for attr_set in sets:
        writer.heading(attr_set.name, '-')
        writer.block(*_set_rows(attr_set))
    if not stubs:
        return
    writer.heading('Extension points', '-')
    writer.paragraph(
        'A stub is a place a descendant may add members to. It is empty here '
        'and stays empty on this family\'s own devices; a child that fills it '
        'lengthens every message that reaches it, which is why the lengths '
        'below belong to this family rather than to the op.')
    for attr_set in stubs:
        writer.block(*_set_rows(attr_set))


def _set_rows(attr_set):
    rows = []
    if attr_set.attrs:
        rows += _listing('required', attr_set.attrs)
    if attr_set.optional_attrs:
        rows += _listing('optional', attr_set.optional_attrs)
    if not rows:
        rows.append('(no members)')
    return rows


def _listing(label, names):
    """One label and its names, wrapped so a long set stays readable."""
    # The four spaces a literal block is indented by, plus the label.
    width = LINE_LIMIT - 4 - len(label) - 1
    wrapped = textwrap.wrap(', '.join(names), width, break_on_hyphens=False,
                            break_long_words=False)
    return [f'{label} {wrapped[0]}'] + [f'{" " * len(label)} {line}'
                                        for line in wrapped[1:]]


def _fills(writer, family):
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
        writer.block(*_tree_rows(fill.root, 0))


def _ops(writer, family):
    own = family.local_ops
    if not own:
        return
    writer.heading('Ops', '=')
    writer.paragraph(
        'One section per op this family declares. An action is executed and '
        'acknowledged, a query is executed and answered in as many fragments '
        'as its answer needs, and an event is produced by the device and read '
        'from a subscribed file description.')
    for op in own:
        _op(writer, family, op)


def _op(writer, family, op):
    writer.heading(op.name, '-')
    writer.field('Op ID', f'``{_id(op.op_id)}``')
    writer.field('Type', TYPES[op.type])
    if op.type != 'event':
        writer.field('Availability', _availability(op.availability))
    writer.line()
    if op.type == 'event':
        writer.paragraph(
            'A record of this op reaches a file description that subscribed '
            'to it, and is dropped for one that did not.')
    _message(writer, 'Request', op.request)
    _message(writer, 'Reply', op.reply)
    _sizes(writer, op)


def _availability(availability):
    if availability == 'dynamic':
        return 'dynamic, so a device may disable and enable it at any time'
    return 'fixed, so a device that publishes it always answers it'


def _message(writer, what, root):
    if root is None:
        writer.paragraph(f'{what}: none.')
        return
    rows = _tree_rows(root, 0)
    if not rows:
        writer.paragraph(f'{what}: an empty root.')
        return
    writer.paragraph(f'{what}:')
    writer.block(*rows)


def _tree_rows(node, depth):
    """One line per member of a tree, indented by its level."""
    rows = []
    for child in node.children():
        indent = '  ' * depth
        detail = [_occurrences(child.max_occurrences),
                  'required' if child.required else 'optional']
        if child.reenters is not None:
            detail.append(f're-enters {child.reenters}')
        rows.append(f'{indent}{child.attr.full_name} '
                    f'({", ".join(detail)})')
        rows += _tree_rows(child, depth + 1)
    for stub in node.stubs:
        indent = '  ' * depth
        requirement = 'optional' if stub.optional else 'required'
        rows.append(f'{indent}<{stub.name}> (extension point, {requirement})')
    return rows


def _sizes(writer, op):
    """What a caller has to bring and what it gets back, in bytes."""
    reported = (
        ('a request of at most {} bytes', op.sizes.request_max_len),
        ('a reply of at most {} bytes', op.sizes.action_reply_max_len),
        ('a reply buffer of at least {} bytes',
         op.sizes.query_reply_min_len),
        ('{} bytes of reply buffer suggested',
         op.sizes.query_reply_suggested_len),
        ('a record of at most {} bytes', op.sizes.event_record_max_len),
    )
    rows = [label.format(value) for label, value in reported if value]
    if not rows:
        return
    writer.paragraph('Lengths on a device of this family:')
    writer.block(*rows)


def _messages(writer, family):
    if not family.messages:
        return
    writer.heading('Messages no op declares', '=')
    writer.paragraph(
        'The framework writes these itself, so they belong to no op: a '
        'caller reads one out of the buffer it offered for a reply.')
    for message in family.messages:
        writer.heading(message.name, '-')
        writer.block(*_tree_rows(message.root, 0))
        writer.paragraph(f'At most {message.max_len} bytes, bounded by '
                         f'{message.limit}.')


def _occurrences(max_occurrences):
    if max_occurrences == spec.UNLIMITED:
        return 'any number'
    if max_occurrences == 1:
        return 'once'
    return f'up to {max_occurrences} times'


def _id(value):
    return f'0x{value:016x}'
