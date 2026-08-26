# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The device descriptor of one family: who answers the wire.

One struct ctlv_family, one struct ctlv_op binding per action and query this
family declares, and the helpers a device decides one of them with. A family
answers what it declared: an inherited op is answered through the descriptor of
the family that declared it, by the driver that wrote it.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import spec

# The family's priv, the request/reply, and a context for core helpers.
CALLBACK_PARAMS = ('void *priv', 'struct ctlv_op_ctx *ctx',
                   'const struct ctlv_cursor *request',
                   'struct ctlv_cursor *reply')

CALLBACK_FIELDS = {
    'action': 'action',
    'query': 'query',
}

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

# The two halves of one extension point, entered from the message that has it.
STUB_HOOKS = (
    ('fill', 'int', ('void *priv', 'struct ctlv_op_ctx *ctx',
                     'const struct ctlv_cursor *request',
                     'struct ctlv_cursor *nest')),
    ('read', 'void', ('void *priv', 'struct ctlv_op_ctx *ctx',
                      'const struct ctlv_cursor *nest')),
)

# The state each half of the pair puts a dynamically available op into.
STATES = (('enable', 'false'), ('disable', 'true'))

# The one decision a device makes about an optional op and cannot take back.
PROVIDE = 'provide'

# And what every event has instead: where one begins.
CREATE = 'create'

# What a blob attr has in the place of a getter.
RESOLVE = 'resolve'


@dataclasses.dataclass(frozen=True)
class ReaderScope:
    name: tuple
    node: object
    tree: str
    node_index: int


# What a getter of each primary form returns, and the core reader answering it.
# An enum and a flags attr are read as the unsigned integer they are.
READERS = {
    'uint': ('u64', 'ctlv_attr_u64'),
    'sint': ('s64', 'ctlv_attr_s64'),
    'bool': ('bool', 'ctlv_attr_bool'),
    'string': ('const char *', 'ctlv_attr_string'),
    'binary': ('const void *', 'ctlv_attr_binary'),
    'enum': ('u64', 'ctlv_attr_u64'),
    'flags': ('u64', 'ctlv_attr_u64'),
}

# What a serializer of each primary form takes, and the core append it wraps.
WRITERS = {
    'uint': (('u64 value',), 'ctlv_put_u64'),
    'sint': (('s64 value',), 'ctlv_put_s64'),
    'bool': (('bool value',), 'ctlv_put_bool'),
    'string': (('const char *value',), 'ctlv_put_string'),
    'binary': (('const void *value', 'u32 len'), 'ctlv_put_binary'),
    'enum': (('u64 value',), 'ctlv_put_u64'),
    'flags': (('u64 value',), 'ctlv_put_u64'),
}

# The nest a getter reads from, and the attr it found there.
NEST_PARAM = 'const struct ctlv_cursor *nest'
FOUND = 'attr'

# The nest a serializer appends to, which it changes and so is not const.
OUT_PARAM = 'struct ctlv_cursor *nest'


def bindings(family):
    """The ops family binds, in the order it binds them.

    Its own actions and queries, in declaration order. An op it inherited is
    answered by the family that declared it, and an event by nobody.
    """
    return tuple(op for op in family.local_ops if op.type != 'event')


def _implementer(family):
    """Who writes the callbacks of family: its driver, or the framework."""
    if family.name == spec.CORE_FAMILY_NAME:
        return 'the framework'
    return 'this family\'s driver'


def emit_header(writer, names, family, reader_scopes):
    """Append the family driver API to a generated kernel header."""
    bound = bindings(family)
    if bound:
        writer.comment(f'Implemented by {_implementer(family)}, one per '
                       'action and query it declares.')
    for op in bound:
        writer.signature('int', names.callback(op), CALLBACK_PARAMS)
    _hooks(writer, names, family)
    _lifecycle_hooks(writer, names, family)
    _stub_hooks(writer, names, family)
    writer.line()
    writer.declaration('extern const struct ctlv_family',
                       f'{names.descriptor()};')
    _helpers(writer, names, family)
    _readers(writer, names, reader_scopes)
    _writers(writer, names, family)
    _creators(writer, names, family)
    writer.line()


def _hooks(writer, names, family):
    """The pair around every op, and the pair around one op."""
    writer.line()
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('Called around every core action or query,',
                       'on every device.')
    else:
        writer.comment('Called around every effective non-core action or '
                       'query,',
                       'on a device created with this family.')
    for which, returns in HOOK_RETURNS:
        writer.signature(returns, names.family_hook(which), HOOK_PARAMS)

    marked = [op for op in bindings(family) if op.pre_post]
    if not marked:
        return
    writer.line()
    writer.comment('One pair per local op that asked for one, called inside',
                   'the active family-wide pair.')
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
    writer.comment('The lifecycle of this family\'s core-allocated priv.')
    for which, returns, params in _declared_lifecycle_hooks(family):
        name = names.lifecycle_hook(which)

        writer.signature(returns, f'*{name}' if which == 'pre_init' else name,
                         params)


def _stub_hooks(writer, names, family):
    """What this family puts into and takes out of each stub it fills."""
    if not family.fills:
        return
    for fill in family.fills:
        owner = family.owners[fill.family_id]
        writer.line()
        writer.comment(f'{fill.name} of {owner}: what this family puts into '
                       'it, and reads.')
        for which, returns, params in STUB_HOOKS:
            writer.signature(returns, names.stub_callback(fill, which),
                             params)


def _helpers(writer, names, family):
    """What one device may decide about each op this family declares."""
    helpers = []
    for op in family.local_ops:
        if op.type == 'event':
            continue
        if op.optional:
            helpers.append((op, PROVIDE, 'ctlv_device_op_provide', ()))
        if op.availability == 'dynamic':
            helpers += [(op, state, 'ctlv_device_op_state_set', (disabled,))
                        for state, disabled in STATES]
    if not helpers:
        return
    writer.line()
    writer.comment('What family initialization may decide about one op before '
                   'publication.')
    for first, (op, what, setter, args) in enumerate(helpers):
        if first:
            writer.line()
        writer.signature('static inline void', names.op_helper(op, what),
                         ('struct ctlv_device *ctlvdev',), ')')
        writer.line('{')
        writer.parenthesized(f'\t{setter}(',
                             ('ctlvdev', f'{op.op_id:#018x}ULL', *args), ');')
        writer.line('}')


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
            writer.line()
            _reference(writer, names, scope, slot, attr)
            writer.line()
            if attr.decl.max_occurrences != 1:
                _iterator(writer, names, scope, slot, attr)
            else:
                _present(writer, names, scope, slot, attr)
            if attr.decl.kind == 'blob':
                writer.line()
                _resolver(writer, names, scope, slot, attr)
            elif cgen.attr_getter_value(attr):
                writer.line()
                _getter(writer, names, scope, slot, attr)


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
        writer.line()
        if attr.decl.kind == 'nested':
            _nest_opener(writer, names, attr)
        else:
            _putter(writer, names, attr)


def _creators(writer, names, family):
    """What begins one event of each kind this family declares.

    A creator hands back a context and the cursor of its root.
    """
    events = [op for op in family.local_ops if op.type == 'event']
    if not events:
        return
    writer.line()
    writer.comment('Building one event, passed to ctlv_op_emit() or '
                   'ctlv_op_abort() once.')
    for op in events:
        writer.line()
        writer.signature('static inline struct ctlv_op_ctx',
                         f'*{names.op_helper(op, CREATE)}',
                         ('struct ctlv_device *ctlvdev',
                          'struct ctlv_cursor **reply', 'gfp_t gfp'), ')')
        writer.line('{')
        writer.parenthesized('\treturn ctlv_event_create(',
                             ('ctlvdev', f'{op.op_id:#018x}ULL', 'reply',
                              'gfp'), ');')
        writer.line('}')


def _nest_opener(writer, names, attr):
    """Open one nested attr onto a cursor the caller placed on its stack."""
    writer.signature('static inline bool', names.attr_helper(attr, 'start'),
                     (OUT_PARAM, 'struct ctlv_cursor *nested'), ')')
    writer.line('{')
    writer.parenthesized('\treturn ctlv_nest_start(',
                         ('nest', 'nested', names.attr(attr),
                          cgen.KINDS[attr.decl.kind]), ');')
    writer.line('}')


def _putter(writer, names, attr):
    """Append one occurrence of one attr, with what it is to hold."""
    params, put = WRITERS[attr.decl.kind]
    values = tuple(param.split()[-1].lstrip('*') for param in params)
    writer.signature('static inline bool', names.attr_helper(attr, 'put'),
                     (OUT_PARAM, *params), ')')
    writer.line('{')
    writer.parenthesized(f'\treturn {put}(',
                         ('nest', names.attr(attr),
                          cgen.KINDS[attr.decl.kind], *values), ');')
    writer.line('}')


def _reference(writer, names, scope, slot, attr):
    value = ('\t((struct ctlv_input_ref){',
             *cgen.wrapped(
                 '\t\t',
                 (scope.tree, names.attr(attr), str(scope.node_index),
                  str(slot), cgen.KINDS[attr.decl.kind]), ' })', ' \\'))
    writer.define_block(f'{names.input_ref(scope, attr)}()', *value)


def _find(writer, cursor, nest, names, scope, attr):
    writer.parenthesized('\tctlv_input_attr(',
                         (cursor, nest,
                          f'{names.input_ref(scope, attr)}()'), ');')


def _check(writer, cursor, names, scope, attr, failure=()):
    check = cgen.wrapped('\tif (!ctlv_input_cursor(',
                         (cursor, f'{names.input_ref(scope, attr)}()'),
                         ')) {' if failure else '))')
    for line in check:
        writer.line(line)
    for line in failure:
        writer.line(f'\t\t{line}')
    if failure:
        writer.line('\t}')


def _present(writer, names, scope, slot, attr):
    """Whether the one occurrence an attr may have is there."""
    writer.signature('static inline bool',
                     names.input_helper(scope, attr, 'present'),
                     (NEST_PARAM,), ')')
    writer.line('{')
    writer.line(f'\tstruct ctlv_cursor {FOUND};')
    writer.line()
    _find(writer, f'&{FOUND}', 'nest', names, scope, attr)
    writer.line(f'\treturn ctlv_attr_ok(&{FOUND});')
    writer.line('}')


def _iterator(writer, names, scope, slot, attr):
    """Every occurrence of a repeated attr, in the order the wire has them."""
    head = ('\tfor (struct ctlv_cursor *__ctlv_attr =',
            *cgen.wrapped(
                '\t\tctlv_input_attr(',
                ('_attr', '_nest', f'{names.input_ref(scope, attr)}()'),
                ');', ' \\'))
    writer.define_block(f'{names.input_iterator(scope, attr)}(_attr, _nest)',
                        *head,
                        '\t     ctlv_attr_ok(__ctlv_attr);',
                        '\t     ctlv_attr_next(__ctlv_attr))')


def _getter(writer, names, scope, slot, attr):
    """What one attr holds.

    A single attr is looked up in the nest it is given, a repeated one is read
    where its iterator left the cursor.
    """
    getter = names.input_helper(scope, attr, 'get')
    if attr.decl.kind == 'nested':
        # A nest holds no value of its own: reading one means standing on it.
        writer.signature('static inline void', getter,
                         ('struct ctlv_cursor *nested', NEST_PARAM), ')')
        writer.line('{')
        _find(writer, 'nested', 'nest', names, scope, attr)
        writer.line('}')
        return

    single = attr.decl.max_occurrences == 1
    returns, reader = READERS[attr.decl.kind]
    params = (NEST_PARAM if single else f'const struct ctlv_cursor *{FOUND}',)
    if attr.decl.kind == 'binary':
        params += ('u32 *len',)
    prefix, declarator = f'static inline {returns}', getter
    if returns.endswith('*'):
        prefix, declarator = prefix.removesuffix('*').rstrip(), f'*{getter}'
    writer.signature(prefix, declarator, params, ')')
    writer.line('{')
    if single:
        writer.line(f'\tstruct ctlv_cursor {FOUND};')
        writer.line()
        _find(writer, f'&{FOUND}', 'nest', names, scope, attr)
    else:
        if attr.decl.kind == 'binary':
            _check(writer, FOUND, names, scope, attr,
                   ('*len = 0;', 'return NULL;'))
        else:
            _check(writer, FOUND, names, scope, attr)
        if returns.endswith('*') and attr.decl.kind != 'binary':
            writer.line('\t\treturn NULL;')
        elif returns == 'bool':
            writer.line('\t\treturn false;')
        else:
            writer.line('\t\treturn 0;')
    arguments = [f'&{FOUND}' if single else FOUND]
    if attr.decl.kind == 'binary':
        arguments.append('len')
    writer.parenthesized(f'\treturn {reader}(', arguments, ');')
    writer.line('}')


def _resolver(writer, names, scope, slot, attr):
    """The memory one blob attr describes, acquired for one op.

    The policy arrives with the attr, so the only argument is the descriptor.
    """
    single = attr.decl.max_occurrences == 1
    params = ('struct ctlv_op_ctx *ctx',
              NEST_PARAM if single else f'const struct ctlv_cursor *{FOUND}')
    writer.signature('static inline struct ctlv_blob',
                     f'*{names.input_helper(scope, attr, RESOLVE)}',
                     params, ')')
    writer.line('{')
    if single:
        writer.line(f'\tstruct ctlv_cursor {FOUND};')
        writer.line()
        _find(writer, f'&{FOUND}', 'nest', names, scope, attr)
    else:
        _check(writer, FOUND, names, scope, attr)
        writer.line('\t\treturn ERR_PTR(-EINVAL);')
    writer.parenthesized('\treturn ctlv_blob_resolve(',
                         ('ctx', f'{"&" if single else ""}{FOUND}'), ');')
    writer.line('}')


def emit_source(writer, names, family):
    """Append the family bindings to a generated kernel source."""
    _bindings(writer, names, family)
    _stub_ops(writer, names, family)
    _descriptor(writer, names, family)


def _bindings(writer, names, family):
    bound = bindings(family)
    writer.comment('One entry per action and query this family declares, in '
                   'declaration order.')
    writer.define(names.n_bindings(), str(len(bound)))
    writer.line()
    if not bound:
        return
    writer.declaration('static const struct ctlv_op',
                       f'{names.ops()}[{names.n_bindings()}] = {{')
    for op in bound:
        writer.line(f'\t/* {op.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.{CALLBACK_FIELDS[op.type]} = '
                    f'{names.callback(op)},')
        if op.pre_post:
            for which, _ in HOOK_RETURNS:
                writer.line(f'\t\t.{which} = {names.op_hook(op, which)},')
        writer.line('\t},')
    writer.line('};')
    writer.line()
    writer.assertion(f'ARRAY_SIZE({names.ops()})', '==', names.n_bindings())
    writer.line()


def _stub_ops(writer, names, family):
    """One entry per extension point this family fills, in the same order."""
    if not family.fills:
        return
    writer.comment('What this family does with each extension point it '
                   'fills.')
    writer.declaration('static const struct ctlv_stub_ops',
                       f'{names.stub_ops()}[] = {{')
    for fill in family.fills:
        writer.line(f'\t/* {fill.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.family_id = {fill.family_id},')
        writer.line(f'\t\t.set_id = {fill.set_id},')
        for which, _, _ in STUB_HOOKS:
            writer.line(f'\t\t.{which} = '
                        f'{names.stub_callback(fill, which)},')
        writer.line('\t},')
    writer.line('};')
    writer.line()


def _descriptor(writer, names, family):
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('What the framework answers the root family\'s ops '
                       'through.')
    else:
        writer.comment('What a driver creates devices with.')
    writer.declaration('const struct ctlv_family',
                       f'{names.descriptor()} = {{')
    writer.line(f'\t.schema = &{names.schema()},')
    parent = _parent_descriptor(family)
    if parent:
        writer.line(f'\t.parent = &{parent},')
    if bindings(family):
        writer.line(f'\t.ops = {names.ops()},')
    writer.line(f'\t.n_ops = {names.n_bindings()},')
    for which, _ in HOOK_RETURNS:
        writer.line(f'\t.{which} = {names.family_hook(which)},')
    for which, _, _ in _declared_lifecycle_hooks(family):
        writer.line(f'\t.{which} = {names.lifecycle_hook(which)},')
    if family.fills:
        writer.line(f'\t.stubs = {names.stub_ops()},')
        writer.line(f'\t.n_stubs = ARRAY_SIZE({names.stub_ops()}),')
    writer.line('};')
    # The one symbol an inheriting family's module needs of this one.
    writer.line(f'EXPORT_SYMBOL_NS_GPL({names.descriptor()}, "CTLV");')


def _parent_descriptor(family):
    """The direct parent descriptor, none only for the root family."""
    if family.parent is None:
        return None
    return cgen.Names(family.chain[-2]).descriptor()
