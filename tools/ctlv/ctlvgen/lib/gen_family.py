# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The registration descriptor of one family: who answers the wire.

One struct ctlv_family, one struct ctlv_op binding per effective action and
query in effective order, and the helpers a device decides an op with. A
callback is named after the effective family, which answers inherited ops too.
"""

from __future__ import annotations

from . import cgen
from . import spec

# The request to read, the reply to write, and a context for the helpers.
CALLBACK_PARAMS = ('struct ctlv_op_ctx *ctx',
                   'const struct ctlv_cursor *request',
                   'struct ctlv_cursor *reply')

CALLBACK_FIELDS = {
    'action': 'action',
    'query': 'query',
}

# What a hook takes, and what each half returns: a pre may refuse the op.
HOOK_PARAMS = ('struct ctlv_op_ctx *ctx',)
HOOK_RETURNS = (('pre', 'int'), ('post', 'void'))

# The state each half of the pair puts a dynamically available op into.
STATES = (('enable', 'false'), ('disable', 'true'))

# The one decision a device makes about an op and can never take back.
REMOVE = 'remove'

# And what every event has instead: where one begins.
CREATE = 'create'

# What a blob attr has in the place of a getter.
RESOLVE = 'resolve'

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
    """The effective ops family binds, in the order it binds them.

    Parent to child, declaration order within each family, events left out.
    The framework answers the root's ops, so a driver binds all the others.
    """
    core = spec.CORE_FAMILY_NAME
    is_core = family.name == core
    return tuple(op for op in family.ops if op.type != 'event' and
                 (family.owners[op.family_id] == core) == is_core)


def _implementer(family):
    """Who writes the callbacks of family: its driver, or the framework."""
    if family.name == spec.CORE_FAMILY_NAME:
        return 'the framework'
    return 'this family\'s driver'


def render_header(family):
    """Return the family descriptor header of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    guard = names.family_guard()

    writer = cgen.preamble(family, 'family', 'header')
    writer.comment(f'Registration descriptor of the {family.name} CTLV '
                   'family.')
    writer.line()
    writer.open_guard(guard)
    writer.includes(*_includes(family, names))

    writer.comment(f'Implemented by {_implementer(family)}, one per '
                   'effective action and query.')
    for op in bindings(family):
        writer.signature('int', names.callback(op), CALLBACK_PARAMS)
    _hooks(writer, names, family)
    writer.line()
    writer.declaration('extern const struct ctlv_family',
                       f'{names.descriptor()};')
    _helpers(writer, names, family)
    _readers(writer, names, family)
    _writers(writer, names, family)
    _creators(writer, names, family)
    writer.line()
    writer.close_guard(guard)
    return writer.text()


def _includes(family, names):
    """This family's metadata and constants, and its parent's descriptor."""
    headers = ['<linux/ctlv.h>', '',
               names.uapi_header(in_kernel=True), '',
               names.kernel_header()]
    if family.parent not in (None, spec.CORE_FAMILY_NAME):
        headers.append(cgen.Names(family.chain[-2]).family_header())
    return tuple(headers)


def _hooks(writer, names, family):
    """The pair around every op, and the pair around one op."""
    writer.line()
    writer.comment('Called around every op this family answers, inherited '
                   'ones included.')
    for which, returns in HOOK_RETURNS:
        writer.signature(returns, names.family_hook(which), HOOK_PARAMS)

    marked = [op for op in bindings(family) if op.pre_post]
    if not marked:
        return
    writer.line()
    writer.comment('And one pair per op that asked for one, called inside the '
                   'pair above.')
    for first, op in enumerate(marked):
        if first:
            writer.line()
        for which, returns in HOOK_RETURNS:
            writer.signature(returns, names.op_hook(op, which), HOOK_PARAMS)


def _helpers(writer, names, family):
    """What one device may decide about each op this family declares."""
    helpers = []
    for op in family.local_ops:
        if op.type == 'event':
            continue
        helpers.append((op, REMOVE, 'ctlv_device_op_remove', ()))
        if op.availability == 'dynamic':
            helpers += [(op, state, 'ctlv_device_op_state_set', (disabled,))
                        for state, disabled in STATES]
    if not helpers:
        return
    writer.line()
    writer.comment('What a device may do with one op. Removal must precede '
                   'registration.')
    for first, (op, what, setter, args) in enumerate(helpers):
        if first:
            writer.line()
        writer.signature('static inline void', names.op_helper(op, what),
                         ('struct ctlv_device *ctlvdev',), ')')
        writer.line('{')
        writer.parenthesized(f'\t{setter}(',
                             ('ctlvdev', f'{op.op_id:#018x}ULL', *args), ');')
        writer.line('}')


def _readers(writer, names, family):
    """What a callback reads out of the request it was handed.

    Two helpers per attr: one locating it in its nest, one reading it.
    """
    attrs = [attr for group in family.local_groups for attr in group.attrs]
    if not attrs:
        return
    writer.line()
    writer.comment('Reading a validated message. A blob has a resolver '
                   'instead of a getter.')
    for attr in attrs:
        writer.line()
        if attr.decl.max_occurrences != 1:
            _iterator(writer, names, attr)
        else:
            _present(writer, names, attr)
        if attr.decl.kind == 'blob':
            writer.line()
            _resolver(writer, names, attr)
        elif cgen.attr_getter_value(attr):
            writer.line()
            _getter(writer, names, attr)


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


def _find(writer, cursor, nest, names, attr):
    """The one call that locates an attr, wrapped where it has to be."""
    writer.parenthesized('\tctlv_attr_find(',
                         (cursor, nest, names.attr(attr),
                          cgen.KINDS[attr.decl.kind]), ');')


def _present(writer, names, attr):
    """Whether the one occurrence an attr may have is there."""
    writer.signature('static inline bool', names.attr_helper(attr, 'present'),
                     (NEST_PARAM,), ')')
    writer.line('{')
    writer.line(f'\tstruct ctlv_cursor {FOUND};')
    writer.line()
    _find(writer, f'&{FOUND}', 'nest', names, attr)
    writer.line(f'\treturn ctlv_attr_ok(&{FOUND});')
    writer.line('}')


def _iterator(writer, names, attr):
    """Every occurrence of a repeated attr, in the order the wire has them."""
    head = cgen.wrapped('\tfor (ctlv_attr_find(',
                        ('_attr', '_nest', names.attr(attr),
                         cgen.KINDS[attr.decl.kind]), ');', ' \\')
    writer.define_block(f'{names.attr_iterator(attr)}(_attr, _nest)', *head,
                        '\t     ctlv_attr_ok(_attr); ctlv_attr_next(_attr))')


def _getter(writer, names, attr):
    """What one attr holds.

    A single attr is looked up in the nest it is given, a repeated one is read
    where its iterator left the cursor.
    """
    getter = names.attr_helper(attr, 'get')
    if attr.decl.kind == 'nested':
        # A nest holds no value of its own: reading one means standing on it.
        writer.signature('static inline void', getter,
                         ('struct ctlv_cursor *nested', NEST_PARAM), ')')
        writer.line('{')
        _find(writer, 'nested', 'nest', names, attr)
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
        _find(writer, f'&{FOUND}', 'nest', names, attr)
    arguments = f'&{FOUND}' if single else FOUND
    if attr.decl.kind == 'binary':
        arguments += ', len'
    writer.line(f'\treturn {reader}({arguments});')
    writer.line('}')


def _resolver(writer, names, attr):
    """The memory one blob attr describes, acquired for one op.

    The policy arrives with the attr, so the only argument is the descriptor.
    """
    single = attr.decl.max_occurrences == 1
    params = ('struct ctlv_op_ctx *ctx',
              NEST_PARAM if single else f'const struct ctlv_cursor *{FOUND}')
    writer.signature('static inline struct ctlv_blob',
                     f'*{names.attr_helper(attr, RESOLVE)}', params, ')')
    writer.line('{')
    if single:
        writer.line(f'\tstruct ctlv_cursor {FOUND};')
        writer.line()
        _find(writer, f'&{FOUND}', 'nest', names, attr)
    writer.line(f'\treturn ctlv_blob_resolve(ctx, '
                f'{"&" if single else ""}{FOUND});')
    writer.line('}')


def render_source(family):
    """Return the family descriptor source of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)

    writer = cgen.preamble(family, 'family', 'source')
    writer.includes('<linux/array_size.h>', '<linux/build_bug.h>',
                    '<linux/ctlv.h>', '', names.family_header())
    _bindings(writer, names, family)
    _descriptor(writer, names, family)
    return writer.text()


def _bindings(writer, names, family):
    bound = bindings(family)
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('One entry per action and query of the root family, in '
                       'declaration order.')
    else:
        writer.comment('One entry per effective action and query, inherited '
                       'ops first.')
    writer.define(names.n_bindings(), str(len(bound)))
    writer.line()
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


def _descriptor(writer, names, family):
    if family.name == spec.CORE_FAMILY_NAME:
        writer.comment('What the framework answers the root family\'s ops '
                       'through.')
    else:
        writer.comment('What a driver registers.')
    writer.declaration('const struct ctlv_family',
                       f'{names.descriptor()} = {{')
    writer.line(f'\t.schema = &{names.schema()},')
    writer.line(f'\t.ops = {names.ops()},')
    writer.line(f'\t.n_ops = {names.n_bindings()},')
    for which, _ in HOOK_RETURNS:
        writer.line(f'\t.{which} = {names.family_hook(which)},')
    writer.line('};')
