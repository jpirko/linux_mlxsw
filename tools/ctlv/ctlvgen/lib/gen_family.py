# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The descriptor of one family: who answers each of its ops.

One struct ctlv_family and one struct ctlv_op per action or query
this family declares, plus the helpers a device uses to decide them. A family
answers what it declared: an inherited op is answered through the descriptor
of the family that declared it, by that family implementation.
"""

from __future__ import annotations

from collections.abc import Iterable

from . import cgen
from . import semantics
from . import spec

# The instance, the request/reply, and a context for core helpers. A device
# embeds the instance, so a callback reaches its state with container_of().
CALLBACK_PARAMS: tuple[str, ...] = (
    'struct ctlv *ctlv', 'struct ctlv_op_ctx *ctx',
    'const struct ctlv_cursor *request', 'struct ctlv_cursor *reply')

# What a hook takes, and what each half returns: a pre may refuse the op.
HOOK_PARAMS: tuple[str, ...] = ('struct ctlv *ctlv',
                                'struct ctlv_op_ctx *ctx')
HOOK_RETURNS: tuple[tuple[str, str], ...] = (('pre', 'int'), ('post', 'void'))

# The two halves of one extension point, entered from the message that has it.
EXT_HOOKS: tuple[tuple[str, str, tuple[str, ...]], ...] = (
    ('fill', 'int', ('struct ctlv *ctlv', 'struct ctlv_op_ctx *ctx',
                     'const struct ctlv_cursor *request',
                     'struct ctlv_cursor *record')),
    ('read', 'void', ('struct ctlv *ctlv', 'struct ctlv_op_ctx *ctx',
                      'const struct ctlv_cursor *record')),
)

# The state helpers of a dynamically available op.
STATES: tuple[str, ...] = ('enable', 'disable')

INPUT_MACROS: dict[str, str] = {
    'uint': 'U64',
    'sint': 'S64',
    'bool': 'BOOL',
    'string': 'STRING',
    'binary': 'BINARY',
    'enum': 'U64',
    'flags': 'U64',
    'record': 'RECORD',
    'blob': 'BLOB',
}

PUT_MACROS: dict[str, str] = {
    'uint': 'U64',
    'sint': 'S64',
    'bool': 'BOOL',
    'string': 'STRING',
    'binary': 'BINARY',
    'enum': 'U64',
    'flags': 'U64',
}


def bindings(family: semantics.Family) -> tuple[semantics.Op, ...]:
    """The ops family binds, in the order it binds them.

    Its own actions and queries, in declaration order. An op it inherited is
    answered by the family that declared it, and an event by nobody.
    """
    return tuple(op for op in family.local_ops if op.type != 'event')


def _implementer(family: semantics.Family) -> str:
    """Who writes the callbacks of a family."""
    if family.name == spec.CORE_FAMILY_NAME:
        return 'the CTLV core'
    return 'this family'


def _macro(writer: cgen.Writer, name: str, args: tuple[str, ...]) -> None:
    writer.parenthesized(f'{name}(', args, ')')


def emit_header(writer: cgen.Writer, names: cgen.Names,
                family: semantics.Family) -> None:
    """Append the family API to a generated kernel header."""
    inputs, outputs = semantics.own_uses(family)
    bound = bindings(family)
    if bound:
        writer.comment(f'Implemented by {_implementer(family)}, one per '
                       'action and query it declares.')
        for op in bound:
            writer.signature('int', names.callback(op), CALLBACK_PARAMS)
        writer.line()
    _hooks(writer, names, family)
    _ext_hooks(writer, names, family)
    writer.declaration('extern const struct ctlv_family',
                       f'{names.descriptor()};')
    writer.line()
    writer.line('#ifndef _CTLV_GENERATED_METADATA_ONLY')
    _helpers(writer, names, family)
    _readers(writer, names, inputs.values())
    _writers(writer, names, outputs.values())
    _creators(writer, names, family)
    writer.line('#endif /* _CTLV_GENERATED_METADATA_ONLY */')
    writer.line()


def _hooks(writer: cgen.Writer, names: cgen.Names,
           family: semantics.Family) -> None:
    """The pair around one op, for each op that asked for one."""
    marked = [op for op in bindings(family) if op.pre_post]
    if not marked:
        return
    writer.comment('Per-op hooks inside the device pair.')
    for first, op in enumerate(marked):
        if first:
            writer.line()
        for which, returns in HOOK_RETURNS:
            writer.signature(returns, names.op_hook(op, which), HOOK_PARAMS)
    writer.line()


def _ext_hooks(writer: cgen.Writer, names: cgen.Names,
               family: semantics.Family) -> None:
    """The fill and read callbacks of each extension point the family fills."""
    for fill in family.fills:
        owner = family.owners[fill.family_id]
        writer.comment(f'{fill.name} of {owner}: fill and read callbacks.')
        for which, returns, params in EXT_HOOKS:
            writer.signature(returns, names.ext_callback(fill, which),
                             params)
        writer.line()


def _helpers(writer: cgen.Writer, names: cgen.Names,
             family: semantics.Family) -> None:
    """What one device may decide about each op this family declares."""
    ops = [op for op in family.local_ops if op.type != 'event' and
           (op.optional or op.availability == 'dynamic')]
    if not ops:
        return
    writer.line()
    writer.comment('Optional publication and dynamic availability helpers.')
    for op in ops:
        if op.optional:
            _macro(writer, 'CTLV_OP_PROVIDE',
                   (names.op_ident(op), names.op_id(op)))
        if op.availability == 'dynamic':
            for state in STATES:
                _macro(writer, f'CTLV_OP_{state.upper()}',
                       (names.op_ident(op), names.op_id(op)))


def _readers(writer: cgen.Writer, names: cgen.Names,
             fields: Iterable[semantics.Field]) -> None:
    """What reads one field of a validated request, wherever it is placed.

    A reader finds its field by type among the members of the record it is
    handed.
    """
    fields = tuple(fields)
    if not fields:
        return
    writer.line()
    writer.comment('Reading a validated message. A blob has a resolver '
                   'instead of a getter.')
    for field in fields:
        repeated = field.decl.max_occurrences != 1
        name = names.field_ident(field)
        writer.doc(field.decl.doc)
        _macro(writer, 'CTLV_IN_REF',
               (name, names.field(field), cgen.KINDS[field.decl.kind]))
        if repeated:
            _iterator(writer, names, field)
        else:
            _macro(writer, 'CTLV_IN_HAS', (name,))
        if field.decl.kind == 'blob':
            macro = 'CTLV_IN_' + ('EACH_' if repeated else '') + 'BLOB'
            _macro(writer, macro, (name,))
        elif cgen.getter_value(field):
            macro = 'CTLV_IN_' + ('EACH_' if repeated else '') + \
                    INPUT_MACROS[field.decl.kind]
            _macro(writer, macro, (name,))


def _writers(writer: cgen.Writer, names: cgen.Names,
             fields: Iterable[semantics.Field]) -> None:
    """What a callback appends to the reply it was handed.

    One helper per field, returning whether its TLV made it into the message.
    """
    fields = tuple(field for field in fields
                   if cgen.output_writers(names, field))
    if not fields:
        return
    writer.line()
    writer.comment('Writing a reply. An append is atomic; a record ends with '
                   'ctlv_record_end().')
    for field in fields:
        if field.decl.kind == 'record':
            macro = 'CTLV_RECORD_START'
        else:
            macro = f'CTLV_PUT_{PUT_MACROS[field.decl.kind]}'
        writer.doc(field.decl.doc)
        _macro(writer, macro,
               (names.field_ident(field), names.field(field),
                cgen.KINDS[field.decl.kind]))


def _creators(writer: cgen.Writer, names: cgen.Names,
              family: semantics.Family) -> None:
    """What begins each event this family declares.

    A creator hands back a context and the cursor of its root.
    """
    events = [op for op in family.local_ops if op.type == 'event']
    if not events:
        return
    writer.line()
    writer.comment('Building one event, passed to ctlv_event_emit() or '
                   'ctlv_event_abort() once.')
    for op in events:
        _macro(writer, 'CTLV_EVENT_CREATE',
               (names.op_ident(op), names.op_id(op)))


def _iterator(writer: cgen.Writer, names: cgen.Names,
              field: semantics.Field) -> None:
    """Every occurrence of a repeated field, in message order."""
    writer.define_call(
        f'{names.field_helper(field, "for_each")}(_cursor, _record)',
        'CTLV_IN_FOR_EACH', names.field_ident(field), '_cursor', '_record')


def emit_source(writer: cgen.Writer, names: cgen.Names,
                family: semantics.Family) -> None:
    """Append the family bindings to a generated kernel source."""
    _bindings(writer, names, family)
    _ext_ops(writer, names, family)
    _descriptor(writer, names, family)


def _bindings(writer: cgen.Writer, names: cgen.Names,
              family: semantics.Family) -> None:
    bound = bindings(family)
    if not bound:
        return
    writer.comment('Local action and query bindings in declaration order.')
    writer.declaration('static const struct ctlv_op',
                       f'{names.ops()}[] = {{')
    for op in bound:
        suffix = '_PRE_POST' if op.pre_post else ''
        writer.line(f'\tCTLV_OP_{op.type.upper()}{suffix}('
                    f'{names.op_ident(op)}),')
    writer.line('};')
    writer.line()


def _ext_ops(writer: cgen.Writer, names: cgen.Names,
             family: semantics.Family) -> None:
    """One entry per extension point this family fills, in the same order."""
    if not family.fills:
        return
    writer.comment('Extension-point callbacks.')
    writer.declaration('static const struct ctlv_ext_ops',
                       f'{names.ext_ops()}[] = {{')
    for fill in family.fills:
        args = (cgen.family_const(family.owners[fill.family_id]),
                names.extension(family.extensions[fill.name]),
                names.ext_ident(fill))
        writer.parenthesized('\tCTLV_EXT_OPS(', args, '),')
    writer.line('};')
    writer.line()


def _descriptor(writer: cgen.Writer, names: cgen.Names,
                family: semantics.Family) -> None:
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('Root-family descriptor used by the CTLV core.')
    else:
        writer.comment('Family descriptor used by ctlv_init().')
    writer.declaration('const struct ctlv_family',
                       f'{names.descriptor()} = {{')
    writer.line(f'\t.schema = &{names.schema()},')
    parent = _parent_descriptor(family)
    if parent:
        writer.line(f'\t.parent = &{parent},')
    if bindings(family):
        writer.line(f'\t.ops = {names.ops()},')
        writer.line(f'\t.ops_count = ARRAY_SIZE({names.ops()}),')
    if family.fills:
        ext_ops = names.ext_ops()
        writer.line(f'\t.ext_ops = {ext_ops},')
        writer.line(f'\t.ext_ops_count = ARRAY_SIZE({ext_ops}),')
    writer.line('};')
    # The one symbol an inheriting family's module needs of this one.
    writer.line(f'EXPORT_SYMBOL_NS_GPL({names.descriptor()}, "CTLV");')


def _parent_descriptor(family: semantics.Family) -> str | None:
    """The direct parent descriptor, none only for the root family."""
    if family.parent is None:
        return None
    return cgen.Names(family.chain[-2]).descriptor()
