# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""C output scaffolding shared by the CTLV generators.

The preamble ctlv-regen.sh recovers a file's specification and mode from, which
is why the specification stays directly above the marker.
"""

from __future__ import annotations

import os
import pathlib
import textwrap
from collections.abc import Callable

from . import semantics
from . import spec
from .errors import CtlvError

REGEN: str = 'tools/ctlv/ctlv-regen.sh'

LINE_LIMIT: int = 100
HARD_LINE_LIMIT: int = 100
TAB_WIDTH: int = 8

# " * " plus the text has to stay inside the line limit.
COMMENT_WIDTH: int = LINE_LIMIT - 3

# C spelling of each schema kind. A test checks these against
# enum ctlv_schema_kind.
KINDS: dict[str, str] = {
    'uint': 'CTLV_SCHEMA_UINT',
    'sint': 'CTLV_SCHEMA_SINT',
    'bool': 'CTLV_SCHEMA_BOOL',
    'string': 'CTLV_SCHEMA_STRING',
    'binary': 'CTLV_SCHEMA_BINARY',
    'enum': 'CTLV_SCHEMA_ENUM',
    'flags': 'CTLV_SCHEMA_FLAGS',
    'record': 'CTLV_SCHEMA_RECORD',
    'blob': 'CTLV_SCHEMA_BLOB',
}

LIFECYCLE_API_SYMBOLS: tuple[str, ...] = (
    'ctlv_init',
    'ctlv_publish',
    'ctlv_unpublish',
    'ctlv_fini',
    'ctlv_uevent',
    'ctlv_family_of',
    'ctlv_op_provide',
    'ctlv_op_state_set',
    'ctlv_device_class_register',
    'ctlv_device_class_unregister',
    'ctlv_device_init',
    'ctlv_device_fini',
    'ctlv_device_add',
    'ctlv_device_del',
)


def width(line: str) -> int:
    """Return the display width of one line, a tab being eight columns."""
    return len(line.expandtabs(TAB_WIDTH))


def ident(*names: str) -> str:
    """Return the lower case C spelling of one or more schema names."""
    return '_'.join(name.replace('-', '_') for name in names)


def macro(*names: str) -> str:
    """Return the upper case C spelling of one or more schema names."""
    return ident(*names).upper()


def enum_tag(*names: str) -> str:
    return f'_{ident(*names)}'


def header_bounds(definition: semantics.Definition) -> tuple[str, str]:
    """A header enum's lowest constant and $end - 1, or its highest.

    A number in a gap between constants is in range, for the family to refuse.
    """
    assert definition.header is not None
    low = min(definition.values, key=lambda value: value[1])[0]
    if definition.header.end:
        return low, f'{definition.header.end} - 1'
    return low, max(definition.values, key=lambda value: value[1])[0]


def family_suffix(family_name: str) -> str:
    suffix = family_name
    if family_name.startswith('ctlv-'):
        suffix = family_name.removeprefix('ctlv-')
    return macro(suffix)


def family_const(family_name: str) -> str:
    if family_name == 'ctlv':
        return 'CTLV_FAMILY_ROOT'
    return f'CTLV_FAMILY_{family_suffix(family_name)}'


def uapi_header(family_name: str, in_kernel: bool = False) -> str:
    """Return the include of the generated UAPI header of family_name."""
    prefix = 'uapi/' if in_kernel else ''
    return f'<{prefix}ctlv/{ident(family_name)}_gen.h>'


def families_uapi_header(in_kernel: bool = False) -> str:
    prefix = 'uapi/' if in_kernel else ''
    return f'<{prefix}ctlv/ctlv_families_gen.h>'


def kernel_header(family_name: str) -> str:
    """Return the include of the generated kernel header of family_name.

    The root family's interface is installed beside <linux/ctlv.h>; any other
    family's is private to the family implementation compiling it.
    """
    name = f'{ident(family_name)}_gen.h'
    if family_name == spec.CORE_FAMILY_NAME:
        return f'<linux/{name}>'
    return f'"{name}"'


def spec_path(path: pathlib.Path | str) -> str:
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


def wrapped(opening: str, items: tuple[str, ...], closing: str,
            trailer: str = '', adaptive: bool = False) -> list[str]:
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

    def __init__(self) -> None:
        self._lines: list[str] = []

    def line(self, text: str = '') -> None:
        self._lines.append(text)

    def comment(self, *lines: str) -> None:
        """One comment block, one input line per output line."""
        if len(lines) == 1:
            self.line(f'/* {lines[0]} */')
            return
        self.line('/*')
        for text in lines:
            self.line(f' * {text}'.rstrip())
        self.line(' */')

    def doc(self, doc: str | None, indent: str = '') -> None:
        """One declaration's doc, as one comment."""
        if not doc:
            return
        text = ' '.join(doc.split())
        if width(f'{indent}/* {text} */') <= LINE_LIMIT:
            self.line(f'{indent}/* {text} */')
            return
        self.line(f'{indent}/*')
        for line in textwrap.wrap(text, COMMENT_WIDTH - width(indent),
                                  break_on_hyphens=False):
            self.line(f'{indent} * {line}')
        self.line(f'{indent} */')

    def define_block(self, name: str, *lines: str) -> None:
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

    def define(self, name: str, value: str) -> None:
        """One #define, continued when it would run past the line limit."""
        text = f'#define {name} {value}'
        if width(text) <= LINE_LIMIT:
            self.line(text)
            return
        self.define_block(name, f'\t{value}')

    def define_call(self, name: str, function: str, *args: str) -> None:
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

    def declaration(self, prefix: str, declarator: str) -> None:
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

    def parenthesized(self, opening: str, items: tuple[str, ...],
                      closing: str) -> None:
        """One parenthesized list, wrapped at the line limit."""
        for text in wrapped(opening, items, closing, adaptive=True):
            self.line(text)

    def assertion(self, left: str, operator: str, right: int | str) -> None:
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

    def signature(self, prefix: str, name: str, params: tuple[str, ...],
                  closing: str = ');') -> None:
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

    def open_guard(self, guard: str) -> None:
        self.line(f'#ifndef {guard}')
        self.line(f'#define {guard}')
        self.line()

    def close_guard(self, guard: str) -> None:
        self.line(f'#endif /* {guard} */')

    def includes(self, *headers: str) -> None:
        """One include block, an empty entry separating two groups."""
        for header in headers:
            self.line(f'#include {header}' if header else '')
        self.line()

    def text(self) -> str:
        while self._lines and not self._lines[-1]:
            self._lines.pop()
        return '\n'.join(self._lines) + '\n'


def preamble(origin: semantics.Family | spec.Registry, mode: str, file: str,
             source: str | None = None) -> Writer:
    """Return a Writer holding the preamble of one generated file."""
    writer = Writer()
    if file == 'source':
        writer.line(f'// SPDX-License-Identifier: {origin.license}')
    else:
        writer.line(f'/* SPDX-License-Identifier: {origin.license} */')
    writer.line('/*')
    writer.line(f' * Generated from {spec_path(source or origin.path)} by')
    writer.line(f' * {REGEN}. Do not edit.')
    writer.line(' */')
    writer.line(f'/* CTLV-GEN {mode} {file} */')
    writer.line()
    return writer


class Names:
    """The C names of one effective family's declarations.

    A declaration is named after the family that owns it, so an inherited
    field keeps the name its own family's header gives it.
    """

    def __init__(self, family: semantics.Family | spec.FamilySpec) -> None:
        self.family = family
        self.prefix = macro(family.name)
        self.ident = ident(family.name)

    @property
    def effective(self) -> semantics.Family:
        """The resolved family, which every name but its files' needs."""
        assert isinstance(self.family, semantics.Family)
        return self.family

    def owner_prefix(self, family_id: int) -> str:
        return macro(self.effective.owners[family_id])

    def owner_ident(self, family_id: int) -> str:
        return ident(self.effective.owners[family_id])

    # Names that a family's UAPI header defines.

    def family_const(self) -> str:
        return family_const(self.family.name)

    def field(self, field: semantics.Field) -> str:
        """The type of one field, after the record holding it."""
        return f'{self.owner_prefix(field.family_id)}_T_{macro(field.c_name)}'

    def field_unit(self, field: semantics.Field) -> str:
        return f'{self.field(field)}_UNIT'

    def field_scale(self, field: semantics.Field) -> str:
        return f'{self.field(field)}_SCALE'

    def op_id(self, op: semantics.Op) -> str:
        return f'{self.owner_prefix(op.family_id)}_OP_{macro(op.name)}'

    def definition(self, definition: semantics.Definition) -> str:
        """The C type name of an enum definition."""
        return enum_tag(self.effective.owners[definition.family_id],
                        definition.name)

    def value(self, definition: semantics.Definition, value_name: str) -> str:
        """One enum value or flag bit."""
        if definition.header:
            return value_name
        return f'{self.owner_prefix(definition.family_id)}_' \
               f'{macro(definition.name, value_name)}'

    def value_bit(self, definition: semantics.Definition,
                  value_name: str) -> str:
        return f'{self.value(definition, value_name)}_BIT'

    def bit_enum(self, definition: semantics.Definition) -> str:
        return enum_tag(self.effective.owners[definition.family_id],
                        definition.name, 'bit')

    def value_mask(self, definition: semantics.Definition) -> str:
        """The mask of every bit a flags definition declares."""
        return f'{self.owner_prefix(definition.family_id)}_' \
               f'{macro(definition.name)}_MASK'

    # Names that a family's kernel metadata defines: its own ops, its
    # extension points and the ones it fills.

    def extension(self, extension: semantics.Extension) -> str:
        """The ID one extension point has among its family's."""
        return f'{self.owner_prefix(extension.family_id)}_EXT_' \
               f'{macro(extension.name)}'

    def enum_extension(self, extension: semantics.EnumExtension) -> str:
        """The ID one enum extension point has among its family's."""
        return f'{self.owner_prefix(extension.family_id)}_ENUM_EXT_' \
               f'{macro(extension.name)}'

    def enum_fill_schemas(self) -> str:
        return f'{self.ident}_enum_fill_schemas'

    def op_schemas(self) -> str:
        return f'{self.ident}_op_schemas'

    def schema(self) -> str:
        return f'{self.ident}_schema'

    def ext_fill_schemas(self) -> str:
        return f'{self.ident}_ext_fill_schemas'

    def node_schemas(self, op: semantics.Op, which: str) -> str:
        # File scope, so no family prefix: an op name is unique within its
        # effective family.
        return f'{ident(op.name)}_{which}_node_schemas'

    def ext_schemas(self, op: semantics.Op, which: str) -> str:
        return f'{ident(op.name)}_{which}_ext_schemas'

    def fill_node_schemas(self, fill: semantics.Fill) -> str:
        return f'{ident(fill.name)}_fill_node_schemas'

    def message_node_schemas(self, message: semantics.Message) -> str:
        return f'{ident(message.name)}_node_schemas'

    def message_schema(self, message: semantics.Message) -> str:
        """The tree the core builds one of its own messages against."""
        return f'{self.ident}_{ident(message.name)}_schema'

    def message_max_len(self, message: semantics.Message) -> str:
        return f'{self.prefix}_{macro(message.name)}_MAX_LEN'

    # Names that a family's device descriptor defines. Named after the family
    # that declared what they answer: an inherited op is answered by that
    # family implementation, on whatever device it arrived at.

    def descriptor(self) -> str:
        return f'{self.ident}_family'

    def ops(self) -> str:
        return f'{self.ident}_ops'

    def ext_ops(self) -> str:
        return f'{self.ident}_ext_ops'

    def ext_ident(self, fill: semantics.Fill) -> str:
        return f'{self.ident}_{ident(fill.name)}'

    def ext_callback(self, fill: semantics.Fill, which: str) -> str:
        """One half of what this family does with an extension point."""
        return f'{self.ext_ident(fill)}_{which}'

    def op_ident(self, op: semantics.Op) -> str:
        return f'{self.owner_ident(op.family_id)}_{ident(op.name)}'

    def callback(self, op: semantics.Op) -> str:
        return f'{self.op_ident(op)}_{op.type}'

    def op_helper(self, op: semantics.Op, what: str) -> str:
        """What one device decides about one op."""
        return f'{self.op_ident(op)}_{what}'

    def field_ident(self, field: semantics.Field) -> str:
        """The stem of every helper of one field, after its constant."""
        return f'{self.owner_ident(field.family_id)}_{ident(field.c_name)}'

    def field_helper(self, field: semantics.Field, what: str) -> str:
        """One helper of one field: reading it, or appending it."""
        return f'{self.field_ident(field)}_{what}'

    def op_hook(self, op: semantics.Op, which: str) -> str:
        """One half of the pair that wraps this op alone."""
        return f'{self.op_ident(op)}_{which}'

    # Where a generated file lives: the installed constants, the schema
    # metadata a family compiles, and the descriptor it registers.

    def uapi_guard(self) -> str:
        return f'_UAPI_CTLV_{self.prefix}_GEN_H'

    def uapi_header(self, in_kernel: bool = False) -> str:
        return uapi_header(self.family.name, in_kernel)

    def kernel_guard(self) -> str:
        return f'_LINUX_{self.prefix}_GEN_H'

    def kernel_header(self) -> str:
        return kernel_header(self.family.name)

    def user_guard(self) -> str:
        return f'_CTLV_{self.prefix}_USER_GEN_H'

    def user_header(self) -> str:
        return f'"{self.ident}_user_gen.h"'

    # Names that a family's userspace binding defines, after the family that
    # declared the op, as a per-op helper is in the kernel.

    def user_op(self, op: semantics.Op) -> str:
        return f'{self.owner_ident(op.family_id)}_{ident(op.name)}'

    def user_op_helper(self, op: semantics.Op, what: str) -> str:
        return f'{self.user_op(op)}_{what}'

    def user_op_len(self, op: semantics.Op, what: str) -> str:
        """One length of one op, as its specification computed it."""
        return f'{self.op_id(op)}_{macro(what)}'

    def user_node_schema(self, op: semantics.Op, which: str) -> str:
        # File scope, so no family prefix, as on the kernel side.
        return f'{ident(op.name)}_{which}_node_schema'

    def user_spec(self) -> str:
        """What this family's specification says, to compare a device to."""
        return f'{self.ident}_spec'

    def user_op_specs(self) -> str:
        return f'{self.ident}_op_specs'


def getter_value(field: semantics.Field) -> bool:
    """Whether a field has a getter beside the reader that locates it."""
    return field.decl.kind != 'blob' and \
        not (field.decl.kind == 'record' and field.decl.max_occurrences != 1)


def input_readers(names: Names, field: semantics.Field) -> tuple[str, ...]:
    """The names of what reads one field of a request."""
    readers = [names.field_helper(field, 'ref')]
    readers.append(names.field_helper(field, 'for_each')
                   if field.decl.max_occurrences != 1
                   else names.field_helper(field, 'present'))
    if field.decl.kind == 'blob':
        readers.append(names.field_helper(field, 'resolve'))
    elif getter_value(field):
        readers.append(names.field_helper(field, 'get'))
    return tuple(readers)


def output_writers(names: Names, field: semantics.Field) -> tuple[str, ...]:
    """The names of what appends one field to a reply or an event.

    A blob has none: a request carries one and a reply never does.
    """
    if field.decl.kind == 'blob':
        return ()
    what = 'start' if field.decl.kind == 'record' else 'put'
    return (names.field_helper(field, what),)


# What a blob field may be backed by, and the libctlv call describing one. A
# putter exists per backing the field permits, so what a caller may
# describe is decided by the specification.
BLOB_PUTTERS: dict[str, tuple[str, tuple[str, ...]]] = {
    'va': ('va', ('const void *address', '__u64 length')),
    'memfd': ('memfd', ('int fd', '__u64 offset', '__u64 length')),
    'dma-buf': ('dma_buf', ('int fd', '__u64 offset', '__u64 length')),
}

# What an event gets in the place of a wrapper.
EVENT_HELPERS: tuple[str, ...] = ('is', 'subscribe', 'unsubscribe')

# The lengths a user-mode header states for one op, by sizes.OpSizes member.
# What a device answers with, its descendants' fills included, op-get says.
USER_LENGTHS: dict[str, str] = {
    'request_max_len': 'request-max-len',
}


def user_readers(names: Names, field: semantics.Field) -> tuple[str, ...]:
    """The names of what reads one field in the userspace binding."""
    if field.decl.kind == 'blob':
        return ()
    readers = [names.field_helper(field, 'for_each')
               if field.decl.max_occurrences != 1
               else names.field_helper(field, 'present')]
    if getter_value(field):
        readers.append(names.field_helper(field, 'get'))
    return tuple(readers)


def user_writers(names: Names, field: semantics.Field) -> tuple[str, ...]:
    """The names of what appends one field in the userspace binding.

    A blob has one putter per backing the field allows, where the kernel
    side has none.
    """
    if field.decl.kind == 'blob':
        assert field.decl.blob is not None
        return tuple(names.field_helper(field,
                                        f'put_{BLOB_PUTTERS[backing][0]}')
                     for backing in field.decl.blob.backings)
    what = 'start' if field.decl.kind == 'record' else 'put'
    return (names.field_helper(field, what),)


class _Table:
    """Generated C names, each mapped to what it was generated for."""

    def __init__(self, family: semantics.Family) -> None:
        self.family = family
        self.names: dict[str, str] = {}

    def add(self, name: str, origin: str) -> None:
        previous = self.names.get(name)
        if previous is not None:
            raise CtlvError(f'{self.family.path}: {origin}',
                            f'generated name {name} is already used by '
                            f'{previous}')
        self.names[name] = origin


def user_symbols(family: semantics.Family) -> dict[str, str]:
    """Return every C name of family's binding, mapped to its origin.

    The binding covers this family's own fields and ops, reaching the rest
    through the parent's header.
    """
    names = Names(family)
    table = _Table(family)

    for field in family.local_fields:
        what = f'field {field.full_name}'
        for helper in user_readers(names, field) + \
                user_writers(names, field):
            table.add(helper, what)
    for op in family.local_ops:
        what = f'op {op.name}'
        # An event has those helpers and no wrapper of its own name.
        if op.type == 'event':
            for helper in EVENT_HELPERS:
                table.add(names.user_op_helper(op, helper), what)
        else:
            table.add(names.user_op(op), what)
            if op.request is not None:
                table.add(names.user_op_helper(op, 'request_new'), what)
        for member, what_len in USER_LENGTHS.items():
            if getattr(op.sizes, member):
                table.add(names.user_op_len(op, what_len), what)
    return table.names


def _extensions_anywhere(node: semantics.Node) -> bool:
    return bool(node.extensions) or any(_extensions_anywhere(child)
                                        for child in node.children())


def symbols(family: semantics.Family) -> dict[str, str]:
    """Return every generated C name of family, mapped to its declaration.

    Dashes become underscores, so two YAML names can spell one C name. That
    has to be an error: one of the two constants would silently not exist.
    """
    names = Names(family)
    table = _Table(family)
    add = table.add

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
    for fill in family.fills:
        for which in ('fill', 'read'):
            add(names.ext_callback(fill, which), f'record {fill.name}')
    for definition in family.definitions.values():
        if definition.header or definition.is_extension_point:
            continue
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
    for field in family.fields.values():
        what = f'field {field.full_name}'
        add(names.field(field), what)
        if field.decl.unit is not None:
            add(names.field_unit(field), what)
            add(names.field_scale(field), what)
    for extension in family.extensions.values():
        add(names.extension(extension), f'extension {extension.name}')
    for enum_extension in family.enum_extensions.values():
        add(names.enum_extension(enum_extension),
            f'enum {enum_extension.name}')
    _helper_symbols(family, add)
    for op in family.ops:
        what = f'op {op.name}'
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
        what = f'record {message.name}'
        add(names.message_node_schemas(message), what)
        add(names.message_schema(message), what)
        add(names.message_max_len(message), what)
    if family.fills:
        add(names.ext_fill_schemas(), f'family {family.name}')
        add(names.ext_ops(), f'family {family.name}')
    if family.enum_fills:
        add(names.enum_fill_schemas(), f'family {family.name}')
    if any(op.type != 'event' for op in family.local_ops):
        add(names.ops(), f'family {family.name}')
    for op in family.local_ops:
        what = f'op {op.name}'
        for which in ('request', 'reply'):
            root = getattr(op, which)
            if root is None:
                continue
            add(names.node_schemas(op, which), what)
            if _extensions_anywhere(root):
                add(names.ext_schemas(op, which), what)
    for fill in family.fills:
        add(names.fill_node_schemas(fill), f'record {fill.name}')
    return table.names


def _helper_symbols(family: semantics.Family,
                    add: Callable[[str, str], None]) -> None:
    """The readers and writers of the whole chain, each where it is emitted.

    Every family of the chain emits the helpers its own uses add, so the
    chain's helpers are those of each prefix of it.
    """
    names = Names(family)
    inputs, outputs = semantics.field_uses(family)
    for field in inputs.values():
        for helper in input_readers(names, field):
            add(helper, f'field {field.full_name}')
    for field in outputs.values():
        for helper in output_writers(names, field):
            add(helper, f'field {field.full_name}')
