# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""C output scaffolding shared by the CTLV generators.

The preamble ctlv-regen.sh recovers a file's specification and mode from, which
is why the specification stays directly above the marker.
"""

from __future__ import annotations

import os

from . import semantics
from . import spec
from .errors import CtlvError

REGEN = 'tools/ctlv/ctlv-regen.sh'

LINE_LIMIT = 100
HARD_LINE_LIMIT = 100
TAB_WIDTH = 8

# C spelling of each primary form. A test checks these against
# enum ctlv_schema_kind.
KINDS = {
    'uint': 'CTLV_SCHEMA_UINT',
    'sint': 'CTLV_SCHEMA_SINT',
    'bool': 'CTLV_SCHEMA_BOOL',
    'string': 'CTLV_SCHEMA_STRING',
    'binary': 'CTLV_SCHEMA_BINARY',
    'enum': 'CTLV_SCHEMA_ENUM',
    'flags': 'CTLV_SCHEMA_FLAGS',
    'nested': 'CTLV_SCHEMA_NEST',
    'blob': 'CTLV_SCHEMA_BLOB',
}

LIFECYCLE_API_SYMBOLS = (
    'ctlv_create',
    'ctlv_destroy',
    'ctlv_family_priv_size_fn',
    'ctlv_family_pre_init_fn',
    'ctlv_family_init_fn',
    'ctlv_family_post_init_fn',
    'ctlv_family_pre_fini_fn',
    'ctlv_family_fini_fn',
    'ctlv_priv_parent',
    'ctlv_priv_child',
)


def width(line):
    """Return the display width of one line, a tab being eight columns."""
    return len(line.expandtabs(TAB_WIDTH))


def ident(*names):
    """Return the lower case C spelling of one or more schema names."""
    return '_'.join(name.replace('-', '_') for name in names)


def macro(*names):
    """Return the upper case C spelling of one or more schema names."""
    return ident(*names).upper()


def enum_tag(*names):
    return f'_{ident(*names)}'


def family_suffix(family_name):
    suffix = family_name
    if family_name.startswith('ctlv-'):
        suffix = family_name.removeprefix('ctlv-')
    return macro(suffix)


def family_const(family_name):
    if family_name == 'ctlv':
        return 'CTLV_FAMILY_ROOT'
    return f'CTLV_FAMILY_{family_suffix(family_name)}'


def uapi_header(family_name, in_kernel=False):
    """Return the include of the generated UAPI header of family_name."""
    prefix = 'uapi/' if in_kernel else ''
    return f'<{prefix}linux/{ident(family_name)}_gen.h>'


def families_uapi_header(in_kernel=False):
    prefix = 'uapi/' if in_kernel else ''
    return f'<{prefix}linux/ctlv_families_gen.h>'


def kernel_header(family_name):
    """Return the include of the generated kernel header of family_name.

    The root family's interface is installed beside <linux/ctlv.h>; any other
    family's is private to the family implementation compiling it.
    """
    name = f'{ident(family_name)}_gen.h'
    if family_name == spec.CORE_FAMILY_NAME:
        return f'<linux/{name}>'
    return f'"{name}"'


def spec_path(path):
    """Return path relative to the kernel tree that holds it.

    ctlv-regen.sh resolves it from the tree root, so it must not depend on how
    the generator was invoked. Outside a kernel tree it is recorded as given.
    """
    full = os.path.abspath(path)
    relative = ''
    while True:
        relative = os.path.join(os.path.basename(full), relative)
        parent = os.path.dirname(full)
        if os.path.exists(os.path.join(parent, 'MAINTAINERS')):
            return relative[:-1]
        if parent == full:
            return str(path)
        full = parent


def wrapped(opening, items, closing, trailer='', adaptive=False):
    """Return the lines of one parenthesized list, wrapped at the limit.

    trailer is what a continued line ends with beyond the item: a backslash
    inside a macro definition, nothing elsewhere.
    """
    parts = [f'{item}, ' for item in items[:-1]] + [items[-1] + closing]
    column = width(opening)
    part_width = max(width(part.rstrip() + trailer) for part in parts)
    indent_column = column
    if adaptive:
        indent_column = min(column, max(0, LINE_LIMIT - part_width))
    indent = '\t' * (indent_column // TAB_WIDTH) + \
        ' ' * (indent_column % TAB_WIDTH)
    lines = []
    line = opening
    for part in parts:
        if width(line + part.rstrip() + trailer) > LINE_LIMIT:
            if line != opening:
                lines.append(line.rstrip())
            elif adaptive and opening.rstrip():
                lines.append(opening.rstrip())
            else:
                line += part
                continue
            line = indent
        line += part
    lines.append(line.rstrip())
    return lines


class Writer:
    """The lines of one generated file."""

    def __init__(self):
        self._lines = []

    def line(self, text=''):
        self._lines.append(text)

    def comment(self, *lines):
        """One comment block, one input line per output line."""
        if len(lines) == 1:
            self.line(f'/* {lines[0]} */')
            return
        self.line('/*')
        for text in lines:
            self.line(f' * {text}'.rstrip())
        self.line(' */')

    def define_block(self, name, *lines):
        """One #define whose value takes more than the one line."""
        header = f'#define {name} \\'
        if width(header) > LINE_LIMIT and '(' in name:
            macro, params = name.removesuffix(')').split('(', 1)
            for text in wrapped(f'#define {macro}(',
                                tuple(params.split(', ')), ')', ' \\',
                                adaptive=True):
                self.line(f'{text} \\')
        else:
            self.line(header)
        for text in lines[:-1]:
            self.line(f'{text} \\')
        self.line(lines[-1])

    def define(self, name, value):
        """One #define, continued when it would run past the line limit."""
        text = f'#define {name} {value}'
        if width(text) <= LINE_LIMIT:
            self.line(text)
            return
        self.define_block(name, f'\t{value}')

    def define_call(self, name, function, *args):
        """One #define whose value is a call, wrapped where it has to be."""
        call = f'{function}({", ".join(args)})'
        if width(f'#define {name} {call}') <= LINE_LIMIT:
            self.line(f'#define {name} {call}')
            return
        if width(f'\t{call}') <= LINE_LIMIT:
            self.define_block(name, f'\t{call}')
            return
        self.define_block(
            name, *wrapped(f'\t{function}(', args, ')', ' \\',
                           adaptive=True))

    def declaration(self, prefix, declarator):
        """One declaration, continued when it would run past the limit."""
        text = f'{prefix} {declarator}'
        if width(text) <= LINE_LIMIT:
            self.line(text)
            return
        indent = prefix[:len(prefix) - len(prefix.lstrip('\t'))]
        self.line(prefix)
        continuation = f'{indent}\t'
        if width(f'{continuation}{declarator}') > LINE_LIMIT:
            continuation = '\t'
        self.line(f'{continuation}{declarator}')

    def parenthesized(self, opening, items, closing):
        """One parenthesized list, wrapped at the line limit."""
        for text in wrapped(opening, items, closing, adaptive=True):
            self.line(text)

    def assertion(self, left, operator, right):
        """One static_assert, broken after its operator when it must be."""
        opening = 'static_assert('
        text = f'{opening}{left} {operator} {right});'
        if width(text) <= LINE_LIMIT:
            self.line(text)
            return
        column = width(opening)
        indent = '\t' * (column // TAB_WIDTH) + ' ' * (column % TAB_WIDTH)
        self.line(f'{opening}{left} {operator}')
        self.line(f'{indent}{right});')

    def signature(self, prefix, name, params, closing=');'):
        """One function signature, wrapped at the line limit."""
        lines = wrapped(f'{prefix} {name}(', params, closing)
        one_line = f'{prefix} {name}({", ".join(params)}{closing}'
        if lines[0].endswith('(') and width(one_line) <= HARD_LINE_LIMIT:
            self.line(one_line)
            return
        if max(width(line) for line in lines) <= LINE_LIMIT:
            for text in lines:
                self.line(text)
            return
        if name.startswith('*'):
            self.line(f'{prefix} *')
            name = name.removeprefix('*')
        else:
            self.line(prefix)
        declarator = f'{name}({", ".join(params)}{closing}'
        if width(declarator) <= HARD_LINE_LIMIT:
            self.line(declarator)
            return
        opening = f'{name}('
        lines = wrapped(opening, params, closing)
        if max(width(line) for line in lines) > LINE_LIMIT:
            self.line(opening)
            self.parenthesized('\t', params, closing)
        else:
            for text in lines:
                self.line(text)

    def open_guard(self, guard):
        self.line(f'#ifndef {guard}')
        self.line(f'#define {guard}')
        self.line()

    def close_guard(self, guard):
        self.line(f'#endif /* {guard} */')

    def includes(self, *headers):
        """One include block, an empty entry separating two groups."""
        for header in headers:
            self.line(f'#include {header}' if header else '')
        self.line()

    def text(self):
        while self._lines and not self._lines[-1]:
            self._lines.pop()
        return '\n'.join(self._lines) + '\n'


def preamble(origin, mode, kind, source=None):
    """Return a Writer holding the preamble of one generated file."""
    writer = Writer()
    if kind == 'source':
        writer.line(f'// SPDX-License-Identifier: {origin.license}')
    else:
        writer.line(f'/* SPDX-License-Identifier: {origin.license} */')
    writer.line(f'/* Do not edit, regenerate with {REGEN}, from: */')
    writer.line(f'/*\t{spec_path(source or origin.path)} */')
    writer.line(f'/* CTLV-GEN {mode} {kind} */')
    writer.line()
    return writer


class Names:
    """The C names of one effective family's declarations.

    A declaration is named after the family that owns it, so an inherited attr
    keeps the name its own family's header gives it.
    """

    def __init__(self, family):
        self.family = family
        self.prefix = macro(family.name)
        self.ident = ident(family.name)

    def owner_prefix(self, family_id):
        return macro(self.family.owners[family_id])

    def owner_ident(self, family_id):
        return ident(self.family.owners[family_id])

    # Names that a family's UAPI header defines.

    def family_const(self):
        return family_const(self.family.name)

    def group(self, group):
        return f'{self.owner_prefix(group.family_id)}_GROUP_' \
               f'{macro(group.name)}'

    def group_enum(self, group):
        return enum_tag(self.family.owners[group.family_id], 'group')

    def group_attr_enum(self, group):
        return enum_tag(self.family.owners[group.family_id],
                        'attr', group.name)

    def group_metadata_enum(self, group):
        return enum_tag(self.family.owners[group.family_id],
                        'attr', group.name, 'metadata')

    def group_attr(self, group):
        return f'_{self.owner_prefix(group.family_id)}_ATTR_' \
               f'{macro(group.name)}'

    def family_attr(self, group):
        return f'_{self.owner_prefix(group.family_id)}_ATTR'

    def local_attr_prefix(self, group):
        return f'_{self.owner_prefix(group.family_id)}_ATTR_' \
               f'{macro(group.name)}_'

    def local_attr(self, attr):
        group = self.family.groups[attr.group]
        return f'{self.local_attr_prefix(group)}{macro(attr.name)}'

    def attr(self, attr):
        return f'{self.owner_prefix(attr.family_id)}_ATTR_' \
               f'{macro(attr.group, attr.name)}'

    def attr_unit(self, attr):
        return f'{self.attr(attr)}_UNIT'

    def attr_scale(self, attr):
        return f'{self.attr(attr)}_SCALE'

    def attr_set(self, attr_set):
        return f'{self.owner_prefix(attr_set.family_id)}_SET_' \
               f'{macro(attr_set.name)}'

    def set_enum(self, attr_set):
        return enum_tag(self.family.owners[attr_set.family_id], 'set')

    def op_prefix(self, op):
        return f'_{self.owner_prefix(op.family_id)}_OP_'

    def op(self, op):
        return f'{self.op_prefix(op)}{macro(op.name)}'

    def operation(self, op):
        return f'_{self.owner_prefix(op.family_id)}_OP'

    def op_enum(self, op):
        return enum_tag(self.family.owners[op.family_id], 'op')

    def op_id(self, op):
        return f'{self.owner_prefix(op.family_id)}_OP_{macro(op.name)}'

    def definition(self, definition):
        """The C type name of an enum definition."""
        return enum_tag(self.family.owners[definition.family_id],
                        definition.name)

    def value(self, definition, value_name):
        """One enum value or flag bit."""
        return f'{self.owner_prefix(definition.family_id)}_' \
               f'{macro(definition.name, value_name)}'

    def value_bit(self, definition, value_name):
        return f'{self.value(definition, value_name)}_BIT'

    def bit_enum(self, definition):
        return enum_tag(self.family.owners[definition.family_id],
                        definition.name, 'bit')

    def value_mask(self, definition):
        """The mask of every bit a flags definition declares."""
        return f'{self.owner_prefix(definition.family_id)}_' \
               f'{macro(definition.name)}_MASK'

    # Names that a family's kernel metadata defines: its own ops and the
    # stubs it fills.

    def op_schemas(self):
        return f'{self.ident}_op_schemas'

    def schema(self):
        return f'{self.ident}_schema'

    def stub_fill_schemas(self):
        return f'{self.ident}_stub_fill_schemas'

    def node_schemas(self, op, which):
        # File scope, so no family prefix: an op name is unique within its
        # effective family.
        return f'{ident(op.name)}_{which}_node_schemas'

    def stub_schemas(self, op, which):
        return f'{ident(op.name)}_{which}_stub_schemas'

    def fill_node_schemas(self, fill):
        return f'{ident(fill.name)}_fill_node_schemas'

    def message_node_schemas(self, message):
        return f'{ident(message.name)}_node_schemas'

    def message_schema(self, message):
        """The tree the core builds one of its own messages against."""
        return f'{self.ident}_{ident(message.name)}_schema'

    def message_max_len(self, message):
        return f'{self.prefix}_{macro(message.name)}_MAX_LEN'

    # Names that a family's device descriptor defines. Named after the family
    # that declared what they answer: an inherited op is answered by that
    # family implementation, on whatever device it arrived at.

    def descriptor(self):
        return f'{self.ident}_family'

    def ops(self):
        return f'{self.ident}_ops'

    def stub_ops(self):
        return f'{self.ident}_stub_ops'

    def stub_ident(self, fill):
        return f'{self.ident}_{ident(fill.name)}'

    def stub_callback(self, fill, which):
        """One half of what this family does with an extension point."""
        return f'{self.stub_ident(fill)}_{which}'

    def lifecycle_hook(self, which):
        """One operation in the lifecycle of this family's priv."""
        return f'{self.ident}_{which}'

    def op_ident(self, op):
        return f'{self.owner_ident(op.family_id)}_{ident(op.name)}'

    def callback(self, op):
        return f'{self.op_ident(op)}_{op.type}'

    def op_helper(self, op, what):
        """What one device decides about one op."""
        return f'{self.op_ident(op)}_{what}'

    def attr_helper(self, attr, what):
        """One helper for one attr: reading it, or appending it.

        Named after the family that declares the attr, like the constant that
        names it on the wire.
        """
        return f'{self.owner_ident(attr.family_id)}_' \
               f'{ident(attr.group, attr.name)}_{what}'

    def attr_iterator(self, attr):
        """What walks every occurrence of one repeated attr."""
        return f'{self.owner_ident(attr.family_id)}_{ident(attr.group)}_' \
               f'for_each_{ident(attr.name)}'

    def input_name(self, scope, attr):
        """The stem of one request-placement-specific reader."""
        owner_parts = self.family.name.split('-')
        group = ()
        if attr.group not in (*scope.name, *owner_parts):
            group = (attr.group,)
        parts = (*scope.name, *group, attr.name)
        parts = tuple(part for index, part in enumerate(parts)
                      if not index or part != parts[index - 1])
        return f'{self.ident}_{ident(*parts)}'

    def input_helper(self, scope, attr, what):
        return f'{self.input_name(scope, attr)}_{what}'

    def input_iterator(self, scope, attr):
        return self.input_helper(scope, attr, 'for_each')

    def input_ref(self, scope, attr):
        return self.input_helper(scope, attr, 'ref')

    def family_hook(self, which):
        """One half of the pair that wraps every op of this family."""
        return f'{self.ident}_op_{which}'

    def op_hook(self, op, which):
        """One half of the pair that wraps this op alone."""
        return f'{self.op_ident(op)}_{which}'

    # Where a generated file lives: the installed constants, the schema
    # metadata a family compiles, and the descriptor it registers.

    def uapi_guard(self):
        return f'_UAPI_LINUX_{self.prefix}_GEN_H'

    def uapi_header(self, in_kernel=False):
        return uapi_header(self.family.name, in_kernel)

    def kernel_guard(self):
        return f'_LINUX_{self.prefix}_GEN_H'

    def kernel_header(self):
        return kernel_header(self.family.name)

    def user_guard(self):
        return f'_CTLV_{self.prefix}_USER_GEN_H'

    def user_header(self):
        return f'"{self.ident}_user_gen.h"'

    # Names that a family's userspace binding defines, after the family that
    # declared the op, as a per-op helper is in the kernel.

    def user_op(self, op):
        return f'{self.owner_ident(op.family_id)}_{ident(op.name)}'

    def user_op_helper(self, op, what):
        return f'{self.user_op(op)}_{what}'

    def user_op_len(self, op, what):
        """One length of one op, as its specification computed it."""
        return f'{self.op_id(op)}_{macro(what)}'

    def user_node_schema(self, op, which):
        # File scope, so no family prefix, as on the kernel side.
        return f'{ident(op.name)}_{which}_node_schema'

    def user_spec(self):
        """What this family's specification says, to compare a device to."""
        return f'{self.ident}_spec'

    def user_op_specs(self):
        return f'{self.ident}_op_specs'


def attr_getter_value(attr):
    """Whether an attr has a getter beside the reader that locates it."""
    return attr.decl.kind != 'blob' and \
        not (attr.decl.kind == 'nested' and attr.decl.max_occurrences != 1)


def input_readers(names, scope, attr):
    readers = [names.input_ref(scope, attr)]
    readers.append(names.input_iterator(scope, attr)
                   if attr.decl.max_occurrences != 1
                   else names.input_helper(scope, attr, 'present'))
    if attr.decl.kind == 'blob':
        readers.append(names.input_helper(scope, attr, 'resolve'))
    elif attr_getter_value(attr):
        readers.append(names.input_helper(scope, attr, 'get'))
    return tuple(readers)


def attr_writers(names, attr):
    """The names of what appends one attr, as the family API declares them.

    A blob has none: a request carries one and a reply never does.
    """
    if attr.decl.kind == 'blob':
        return ()
    what = 'start' if attr.decl.kind == 'nested' else 'put'
    return (names.attr_helper(attr, what),)


# What a blob attr may be backed by, and the libctlv call describing one. A
# putter exists per backing the placement permits, so what a caller may
# describe is decided by the specification.
BLOB_PUTTERS = {
    'va': ('va', ('const void *address', '__u64 length')),
    'memfd': ('memfd', ('int fd', '__u64 offset', '__u64 length')),
}

# What an event gets in the place of a wrapper.
EVENT_HELPERS = ('is', 'subscribe', 'unsubscribe')

# The lengths a user-mode header states for one op, by sizes.OpSizes member.
USER_LENGTHS = {
    'request_max_len': 'request-max-len',
    'action_reply_max_len': 'action-reply-max-len',
    'query_reply_min_len': 'query-reply-min-len',
    'query_reply_suggested_len': 'query-reply-suggested-len',
    'event_record_max_len': 'event-record-max-len',
}


def user_attr_readers(names, attr):
    """The names of what reads one attr in the userspace binding."""
    if attr.decl.kind == 'blob':
        return ()
    readers = [names.attr_iterator(attr) if attr.decl.max_occurrences != 1
               else names.attr_helper(attr, 'present')]
    if attr_getter_value(attr):
        readers.append(names.attr_helper(attr, 'get'))
    return tuple(readers)


def user_attr_writers(names, attr):
    """The names of what appends one attr in the userspace binding.

    A blob has one putter per backing the placement allows, where the kernel
    side has none.
    """
    if attr.decl.kind == 'blob':
        return tuple(names.attr_helper(attr, f'put_{BLOB_PUTTERS[backing][0]}')
                     for backing in attr.decl.blob.backings)
    what = 'start' if attr.decl.kind == 'nested' else 'put'
    return (names.attr_helper(attr, what),)


def user_symbols(family):
    """Return every C name of family's binding, mapped to its origin.

    The binding covers this family's own attrs and ops, reaching the rest
    through the parent's header.
    """
    names = Names(family)
    table = {}

    def add(name, origin):
        previous = table.get(name)
        if previous is not None:
            raise CtlvError(f'{family.path}: {origin}',
                            f'generated name {name} is already used by '
                            f'{previous}')
        table[name] = origin

    for group in family.local_groups:
        for attr in group.attrs:
            what = f'attr {attr.full_name}'
            for helper in user_attr_readers(names, attr) + \
                    user_attr_writers(names, attr):
                add(helper, what)
    for op in family.local_ops:
        what = f'op {op.name}'
        # An event has those helpers and no wrapper of its own name.
        if op.type == 'event':
            for helper in EVENT_HELPERS:
                add(names.user_op_helper(op, helper), what)
        else:
            add(names.user_op(op), what)
            if op.request is not None:
                add(names.user_op_helper(op, 'request_new'), what)
        for field, what_len in USER_LENGTHS.items():
            if getattr(op.sizes, field):
                add(names.user_op_len(op, what_len), what)
    return table


def _stubs_anywhere(node):
    return bool(node.stubs) or any(_stubs_anywhere(child)
                                   for child in node.children())


def symbols(family):
    """Return every generated C name of family, mapped to its declaration.

    Dashes become underscores, so two YAML names can spell one C name. That
    has to be an error: one of the two constants would silently not exist.
    """
    names = Names(family)
    table = {}

    def add(name, origin):
        previous = table.get(name)
        if previous is not None:
            raise CtlvError(f'{family.path}: {origin}',
                            f'generated name {name} is already used by '
                            f'{previous}')
        table[name] = origin

    for name in LIFECYCLE_API_SYMBOLS:
        add(name, 'the CTLV lifecycle API')
    add(names.family_const(), f'family {family.name}')
    # A child header may include every ancestor's family API, and built-in
    # families also share one linker namespace with the root.
    for family_id, owner in family.owners.items():
        owner_ident = ident(owner)
        if any(op.family_id == family_id for op in family.ops):
            add(f'{owner_ident}_op_schemas', f'family {owner}')
        add(f'{owner_ident}_schema', f'family {owner}')
        add(f'{owner_ident}_family', f'family {owner}')
        for which in ('pre', 'post'):
            add(f'{owner_ident}_op_{which}', f'family {owner}')
        for which in ('priv_size', 'pre_init', 'init', 'post_init',
                      'pre_fini', 'fini'):
            add(f'{owner_ident}_device_{which}', f'family {owner}')
    for fill in family.fills:
        for which in ('fill', 'read'):
            add(names.stub_callback(fill, which), f'attr set {fill.name}')
    for definition in family.definitions.values():
        what = f'{definition.kind} {definition.name}'
        if definition.kind == 'enum':
            add(f'enum {names.definition(definition)}', what)
        else:
            add(f'enum {names.bit_enum(definition)}', what)
            add(names.value_mask(definition), what)
        for value_name, _ in definition.values:
            if definition.kind == 'flags':
                add(names.value_bit(definition, value_name), what)
            add(names.value(definition, value_name), what)
    group_enums = set()
    family_attrs = set()
    for group in family.groups.values():
        group_enum = f'enum {names.group_enum(group)}'
        if group_enum not in group_enums:
            add(group_enum, f'family {family.owners[group.family_id]} groups')
            group_enums.add(group_enum)
        family_attr = names.family_attr(group)
        if family_attr not in family_attrs:
            add(family_attr, f'family {family.owners[group.family_id]} attrs')
            family_attrs.add(family_attr)
        add(names.group(group), f'attr group {group.name}')
        add(f'enum {names.group_attr_enum(group)}',
            f'attr group {group.name}')
        if any(attr.decl.unit is not None for attr in group.attrs):
            add(f'enum {names.group_metadata_enum(group)}',
                f'attr group {group.name} metadata')
        add(names.group_attr(group), f'attr group {group.name}')
        for attr in group.attrs:
            what = f'attr {attr.full_name}'
            add(names.local_attr(attr), what)
            add(names.attr(attr), what)
            if attr.decl.unit is not None:
                add(names.attr_unit(attr), what)
                add(names.attr_scale(attr), what)
            for helper in attr_writers(names, attr):
                add(helper, what)
    for ancestor in family.chain[:-1]:
        ancestor_names = Names(ancestor)
        owner_family_id = next(
            family_id for family_id, owner in family.owners.items()
            if owner == ancestor.name)

        for scope in semantics.input_scopes(family, owner_family_id):
            for child in scope.node.children():
                what = f'attr {child.attr.full_name} in ancestor ' \
                       f'{ancestor.name}'
                for helper in input_readers(ancestor_names, scope,
                                            child.attr):
                    add(helper, what)
    for scope in semantics.input_scopes(family):
        for child in scope.node.children():
            what = f'attr {child.attr.full_name} in {".".join(scope.name)}'
            for helper in input_readers(names, scope, child.attr):
                add(helper, what)
    set_enums = set()
    for attr_set in family.sets.values():
        set_enum = f'enum {names.set_enum(attr_set)}'
        if set_enum not in set_enums:
            add(set_enum, f'family {family.owners[attr_set.family_id]} sets')
            set_enums.add(set_enum)
        add(names.attr_set(attr_set), f'attr set {attr_set.name}')
    operation_enums = set()
    for op in family.ops:
        what = f'op {op.name}'
        operation_enum = f'enum {names.op_enum(op)}'
        if operation_enum not in operation_enums:
            add(operation_enum,
                f'family {family.owners[op.family_id]} operations')
            add(names.operation(op),
                f'family {family.owners[op.family_id]} operations')
            operation_enums.add(operation_enum)
        add(names.op(op), what)
        add(names.op_id(op), what)
        if op.type == 'event':
            add(names.op_helper(op, 'create'), what)
            continue
        add(names.callback(op), what)
        if op.optional:
            add(names.op_helper(op, 'provide'), what)
        if op.availability == 'dynamic':
            add(names.op_helper(op, 'enable'), what)
            add(names.op_helper(op, 'disable'), what)
        if op.pre_post:
            add(names.op_hook(op, 'pre'), what)
            add(names.op_hook(op, 'post'), what)
    for message in family.messages:
        what = f'attr set {message.name}'
        add(names.message_node_schemas(message), what)
        add(names.message_schema(message), what)
        add(names.message_max_len(message), what)
    if family.fills:
        add(names.stub_fill_schemas(), f'family {family.name}')
        add(names.stub_ops(), f'family {family.name}')
    if any(op.type != 'event' for op in family.local_ops):
        add(names.ops(), f'family {family.name}')
    for op in family.local_ops:
        what = f'op {op.name}'
        for which in ('request', 'reply'):
            root = getattr(op, which)
            if root is None:
                continue
            add(names.node_schemas(op, which), what)
            if _stubs_anywhere(root):
                add(names.stub_schemas(op, which), what)
    for fill in family.fills:
        add(names.fill_node_schemas(fill), f'attr set {fill.name}')
    return table
