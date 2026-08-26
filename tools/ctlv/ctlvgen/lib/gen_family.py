# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The descriptor of one family: who answers the wire.

One struct ctlv_family and one struct ctlv_op per action or query
this family declares, plus the helpers a device uses to decide them. A family
answers what it declared: an inherited op is answered through the descriptor
of the family that declared it, by that family implementation.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import spec

# The family's priv, the request/reply, and a context for core helpers.
CALLBACK_PARAMS = ('void *priv', 'struct ctlv_op_ctx *ctx',
                   'const struct ctlv_cursor *request',
                   'struct ctlv_cursor *reply')

# What a hook takes, and what each half returns: a pre may refuse the op.
HOOK_PARAMS = ('void *priv', 'struct ctlv_op_ctx *ctx')
HOOK_RETURNS = (('pre', 'int'), ('post', 'void'))

# How every family prepares, initializes and finalizes its priv.
LIFECYCLE_HOOKS = (
    ('priv_size', 'size_t', ('void',)),
    ('pre_init', 'const void', ('const void *init_info',)),
    ('init', 'int', ('struct ctlv_device *ctlvdev',
                     'const void *init_info', 'void *priv')),
    ('post_init', 'int', ('struct ctlv_device *ctlvdev', 'void *priv')),
    ('pre_fini', 'void', ('struct ctlv_device *ctlvdev', 'void *priv')),
    ('fini', 'void', ('struct ctlv_device *ctlvdev', 'void *priv')),
)
OPTIONAL_LIFECYCLE_HOOKS = ('pre_init', 'post_init', 'pre_fini')

# The reply-side callback for an extension point.
STUB_HOOKS = (
    ('fill', 'int', ('void *priv', 'struct ctlv_op_ctx *ctx',
                     'const struct ctlv_cursor *request',
                     'struct ctlv_cursor *nest')),
)

# The state helpers of a dynamically available op.
STATES = ('enable', 'disable')


@dataclasses.dataclass(frozen=True)
class ReaderScope:
    name: tuple
    node: object
    tree: str
    node_index: int


INPUT_MACROS = {
    'uint': 'U64',
    'sint': 'S64',
    'bool': 'BOOL',
    'string': 'STRING',
    'binary': 'BINARY',
    'enum': 'U64',
    'flags': 'U64',
    'nested': 'NEST',
    'blob': 'BLOB',
}

PUT_MACROS = {
    'uint': 'U64',
    'sint': 'S64',
    'bool': 'BOOL',
    'string': 'STRING',
    'binary': 'BINARY',
    'enum': 'U64',
    'flags': 'U64',
}


def bindings(family):
    """The ops family binds, in the order it binds them.

    Its own actions and queries, in declaration order. An op it inherited is
    answered by the family that declared it, and an event by nobody.
    """
    return tuple(op for op in family.local_ops if op.type != 'event')


def _implementer(family):
    """Who writes the callbacks of a family."""
    if family.name == spec.CORE_FAMILY_NAME:
        return 'the CTLV core'
    return 'this family'


def _macro(writer, name, args):
    writer.parenthesized(f'{name}(', args, ')')


def emit_header(writer, names, family, reader_scopes):
    """Append the family API to a generated kernel header."""
    bound = bindings(family)
    if bound:
        writer.comment(f'Implemented by {_implementer(family)}, one per '
                       'action and query it declares.')
    for op in bound:
        writer.signature('int', names.callback(op), CALLBACK_PARAMS)
    if bound:
        writer.line()
    _hooks(writer, names, family)
    _lifecycle_hooks(writer, names, family)
    _stub_hooks(writer, names, family)
    writer.line()
    writer.declaration('extern const struct ctlv_family',
                       f'{names.descriptor()};')
    writer.line()
    writer.line('#ifndef _CTLV_GENERATED_METADATA_ONLY')
    _helpers(writer, names, family)
    _readers(writer, names, reader_scopes)
    _writers(writer, names, family)
    _creators(writer, names, family)
    writer.line('#endif /* _CTLV_GENERATED_METADATA_ONLY */')
    writer.line()


def _hooks(writer, names, family):
    """The pair around every op, and the pair around one op."""
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('Root-family hooks around every core action or query.')
    else:
        writer.comment('Family-wide hooks around every effective non-core op.')
    for which, returns in HOOK_RETURNS:
        writer.signature(returns, names.family_hook(which), HOOK_PARAMS)

    marked = [op for op in bindings(family) if op.pre_post]
    if not marked:
        return
    writer.line()
    writer.comment('Per-op hooks nested inside the family-wide pair.')
    for first, op in enumerate(marked):
        if first:
            writer.line()
        for which, returns in HOOK_RETURNS:
            writer.signature(returns, names.op_hook(op, which), HOOK_PARAMS)


def _declared_lifecycle_hooks(family):
    return tuple(hook for hook in LIFECYCLE_HOOKS
                 if hook[0] not in OPTIONAL_LIFECYCLE_HOOKS or
                 getattr(family, hook[0]))


def _lifecycle_hooks(writer, names, family):
    """The generated lifecycle of this family's priv."""
    writer.line()
    writer.comment('Core-allocated family priv lifecycle.')
    for which, returns, params in _declared_lifecycle_hooks(family):
        name = names.lifecycle_hook(which)

        writer.signature(returns, f'*{name}' if which == 'pre_init' else name,
                         params)


def _stub_hooks(writer, names, family):
    """What this family puts into each stub it fills."""
    if not family.fills:
        return
    for fill in family.fills:
        owner = family.owners[fill.family_id]
        writer.line()
        writer.comment(f'{fill.name} of {owner}: fill callback.')
        for which, returns, params in STUB_HOOKS:
            writer.signature(returns, names.stub_callback(fill, which),
                             params)


def _helpers(writer, names, family):
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


def _readers(writer, names, scopes):
    """Placement-specific indexed access to callback requests."""
    if not any(scope.node.children() for scope in scopes):
        return
    writer.line()
    writer.comment('Reading a validated message. A blob has a resolver '
                   'instead of a getter.')
    for scope in scopes:
        for slot, child in enumerate(scope.node.children()):
            attr = child.attr
            repeated = attr.decl.max_occurrences != 1
            name = _input_name(names, scope, attr)
            _macro(writer, 'CTLV_IN_REF',
                   _input_args(names, scope, slot, attr))
            if repeated:
                _iterator(writer, names, scope, attr)
            else:
                _macro(writer, 'CTLV_IN_HAS', (name,))
            if attr.decl.kind == 'blob':
                macro = 'CTLV_IN_' + ('EACH_' if repeated else '') + 'BLOB'
                _macro(writer, macro, (name,))
            elif cgen.attr_getter_value(attr):
                macro = 'CTLV_IN_' + ('EACH_' if repeated else '') + \
                        INPUT_MACROS[attr.decl.kind]
                _macro(writer, macro, (name,))


def _writers(writer, names, family):
    """What a callback appends to the reply it was handed.

    One helper per attr, returning whether the attr made it into the message.
    """
    attrs = [attr for group in family.local_groups for attr in group.attrs
             if cgen.attr_writers(names, attr)]
    if not attrs:
        return
    writer.line()
    writer.comment('Writing a reply. An append is atomic; a nest ends with '
                   'ctlv_nest_end().')
    for attr in attrs:
        if attr.decl.kind == 'nested':
            macro = 'CTLV_NEST_START'
        else:
            macro = f'CTLV_PUT_{PUT_MACROS[attr.decl.kind]}'
        what = 'start' if attr.decl.kind == 'nested' else 'put'
        name = names.attr_helper(attr, what).removesuffix(f'_{what}')
        _macro(writer, macro,
               (name,
                names.attr(attr), cgen.KINDS[attr.decl.kind]))


def _creators(writer, names, family):
    """What begins one event of each kind this family declares.

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


def _input_name(names, scope, attr):
    return names.input_ref(scope, attr).removesuffix('_ref')


def _input_args(names, scope, slot, attr):
    return (_input_name(names, scope, attr), scope.tree, names.attr(attr),
            str(scope.node_index), str(slot), cgen.KINDS[attr.decl.kind])


def _iterator(writer, names, scope, attr):
    """Every occurrence of a repeated attr, in wire order."""
    writer.define_call(
        f'{names.input_iterator(scope, attr)}(_attr, _nest)',
        'CTLV_IN_FOR_EACH', names.input_name(scope, attr), '_attr', '_nest')


def emit_source(writer, names, family):
    """Append the family bindings to a generated kernel source."""
    _bindings(writer, names, family)
    _stub_ops(writer, names, family)
    _descriptor(writer, names, family)


def _bindings(writer, names, family):
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


def _stub_ops(writer, names, family):
    """One entry per extension point this family fills, in the same order."""
    if not family.fills:
        return
    writer.comment('Extension-point callbacks.')
    writer.declaration('static const struct ctlv_stub_ops',
                       f'{names.stub_ops()}[] = {{')
    for fill in family.fills:
        attr_set = family.sets[fill.name]
        args = (cgen.family_const(family.owners[fill.family_id]),
                names.attr_set(attr_set), names.stub_ident(fill))
        writer.parenthesized('\tCTLV_STUB_OPS(', args, '),')
    writer.line('};')
    writer.line()


def _descriptor(writer, names, family):
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('Root-family descriptor used by the CTLV core.')
    else:
        writer.comment('Family descriptor used by ctlv_create().')
    writer.declaration('const struct ctlv_family',
                       f'{names.descriptor()} = {{')
    writer.line(f'\t.schema = &{names.schema()},')
    parent = _parent_descriptor(family)
    if parent:
        writer.line(f'\t.parent = &{parent},')
    if bindings(family):
        writer.line(f'\t.ops = {names.ops()},')
        writer.line(f'\t.ops_count = ARRAY_SIZE({names.ops()}),')
    for which, _ in HOOK_RETURNS:
        writer.line(f'\t.{which} = {names.family_hook(which)},')
    for which, _, _ in _declared_lifecycle_hooks(family):
        writer.line(f'\t.{which} = {names.lifecycle_hook(which)},')
    if family.fills:
        stub_ops = names.stub_ops()
        writer.line(f'\t.stub_ops = {stub_ops},')
        writer.line(f'\t.stub_ops_count = ARRAY_SIZE({stub_ops}),')
    writer.line('};')
    # The one symbol an inheriting family's module needs of this one.
    writer.line(f'EXPORT_SYMBOL_NS_GPL({names.descriptor()}, "CTLV");')


def _parent_descriptor(family):
    """The direct parent descriptor, none only for the root family."""
    if family.parent is None:
        return None
    return cgen.Names(family.chain[-2]).descriptor()
