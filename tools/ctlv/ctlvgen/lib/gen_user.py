# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The userspace binding of one family.

The header is names, and a child's includes its parent's. The source is the
specification as data: one struct ctlv_schema_node tree per op message, in the
shape op-schema-get reports, so a device can be compared with it.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import sizes
from . import spec
from .errors import CtlvError

# What every wrapper starts and ends with. A caller wanting only the errno
# passes NULL for the error.
DEV_PARAM = 'struct ctlv_dev *dev'
ERROR_PARAM = 'struct ctlv_error *error'

# What a request is built in, and what an answer arrives in.
REQUEST_PARAM = 'struct ctlv_msg *request'
REPLY_PARAM = 'struct ctlv_reply **reply'

# The nest a reader reads from, and the one occurrence it found there.
NEST_PARAM = 'const struct ctlv_attr *nest'
FOUND = 'attr'

# What a getter of each primary form hands back, and the libctlv reader that
# answers it. An enum and a flags payload are read as the integer they are.
READERS = {
    'uint': (('__u64 *value',), 'ctlv_get_u64', ()),
    'sint': (('__s64 *value',), 'ctlv_get_s64', ()),
    'bool': (('bool *value',), 'ctlv_get_bool', ()),
    'string': (('const char **value',), 'ctlv_get_str', ()),
    'binary': (('const void **value', '__u32 *len'), 'ctlv_get_bin',
               ('value', 'len')),
    'enum': (('__u64 *value', 'bool *known'), 'ctlv_get_enum',
             ('value', 'known')),
    'flags': (('__u64 *value', '__u64 *unknown'), 'ctlv_get_flags',
              ('value', 'unknown')),
}

# What a putter of each primary form takes, and the libctlv append it wraps.
WRITERS = {
    'uint': (('__u64 value',), 'ctlv_put_u64'),
    'sint': (('__s64 value',), 'ctlv_put_s64'),
    'bool': (('bool value',), 'ctlv_put_bool'),
    'string': (('const char *value',), 'ctlv_put_str'),
    'binary': (('const void *value', '__u32 len'), 'ctlv_put_bin'),
    'enum': (('__u64 value',), 'ctlv_put_u64'),
    'flags': (('__u64 value',), 'ctlv_put_u64'),
}

# Which schema member each length of an op fills, and its name in the header.
LENGTHS = tuple(cgen.USER_LENGTHS.items())

# Bits of struct ctlv_schema_node's present mask, by the member they cover.
PRESENT = {
    'min': 'CTLV_SCHEMA_HAS_MIN',
    'max': 'CTLV_SCHEMA_HAS_MAX',
    'min_length': 'CTLV_SCHEMA_HAS_MIN_LENGTH',
    'max_length': 'CTLV_SCHEMA_HAS_MAX_LENGTH',
    'flags_mask': 'CTLV_SCHEMA_HAS_FLAGS_MASK',
    'unit': 'CTLV_SCHEMA_HAS_UNIT',
    'scale': 'CTLV_SCHEMA_HAS_SCALE',
    'blob': 'CTLV_SCHEMA_HAS_BLOB',
    'reenters': 'CTLV_SCHEMA_HAS_REENTERS',
}

# What a blob placement contributes, in the order the core reports it, and the
# root family's enum naming the backings it allows.
BLOB_MEMBERS = ('blob_direction', 'blob_backings', 'blob_max_length',
                'blob_alignment')
BLOB_TYPES = 'blob-type'

# The extremes of an unbounded integer bound, stored as the unsigned pattern of
# the value. The most negative signed one has no literal.
EXTREMES = {
    ('uint', spec.U64_MAX): 'UINT64_MAX',
    ('sint', spec.S64_MIN): '(__u64)INT64_MIN',
    ('sint', spec.S64_MAX): 'INT64_MAX',
}

OP_TYPES = {kind: cgen.macro(spec.CORE_FAMILY_NAME, 'op-type', kind)
            for kind in ('action', 'query', 'event')}

AVAILABILITIES = {
    availability: cgen.macro(spec.CORE_FAMILY_NAME, 'op-availability',
                             availability)
    for availability in spec.AVAILABILITIES
}


def _reject_root(family):
    """The root family's userspace half is libctlv, written by hand."""
    if family.name == spec.CORE_FAMILY_NAME:
        raise CtlvError(f'{family.path}: family {family.name}',
                        'the root family has no generated userspace binding, '
                        'because libctlv implements its ops by hand as the '
                        'bootstrap every caller starts from')


def render_header(family):
    """Return the userspace binding header of family as text."""
    _reject_root(family)
    names = cgen.Names(family)
    cgen.symbols(family)
    cgen.user_symbols(family)
    guard = names.user_guard()

    writer = cgen.preamble(family, 'user', 'header')
    writer.comment(f'Userspace binding of the {family.name} CTLV family.')
    writer.line()
    writer.open_guard(guard)
    writer.includes(*_includes(family, names))

    writer.comment('What this family\'s specification says, for holding a '
                   'device against it.')
    writer.declaration('extern const struct ctlv_family_spec',
                       f'{names.user_spec()};')
    _ops(writer, names, family)
    _writers(writer, names, family)
    _readers(writer, names, family)
    _events(writer, names, family)
    writer.line()
    writer.close_guard(guard)
    return writer.text()


def _includes(family, names):
    """libctlv, this family's constants, and the parent's binding."""
    headers = ['<ctlv.h>', '', names.uapi_header()]
    if family.parent != spec.CORE_FAMILY_NAME:
        headers += ['', cgen.Names(family.chain[-2]).user_header()]
    return tuple(headers)


def _ops(writer, names, family):
    """One call per action and query this family declares.

    A wrapper needing a reply buffer asks the device for the length.
    """
    ops = [op for op in family.local_ops if op.type != 'event']
    if not ops:
        return
    writer.line()
    writer.comment('Executing an op. A query is reassembled into the one '
                   'reply it fills.')
    for op in ops:
        writer.line()
        _lengths(writer, names, op)
        if op.request is not None:
            _request_new(writer, names, op)
            writer.line()
        _wrapper(writer, names, op)


def _lengths(writer, names, op):
    """The lengths of one op's messages, as this specification computed."""
    for field, what in LENGTHS:
        value = getattr(op.sizes, field)
        if value:
            writer.define(names.user_op_len(op, what), str(value))
    writer.line()


def _request_new(writer, names, op):
    """A request buffer this op's longest request fits in.

    Only the size to start at: past it the message grows to what the transport
    allows, a descendant's device taking more.
    """
    writer.signature('static inline int',
                     names.user_op_helper(op, 'request_new'),
                     ('struct ctlv_msg **request',), ')')
    writer.line('{')
    writer.declaration('\tconst __u32 len =',
                       f'{names.user_op_len(op, "request-max-len")};')
    writer.line()
    writer.line('\treturn ctlv_msg_new(len, request);')
    writer.line('}')


def _wrapper(writer, names, op):
    """The one call that executes one op.

    An action with no reply is told there is no buffer.
    """
    params = [DEV_PARAM]
    if op.request is not None:
        params.append(REQUEST_PARAM)
    if op.reply is not None:
        params.append(REPLY_PARAM)
    params.append(ERROR_PARAM)
    writer.signature('static inline int', names.user_op(op), tuple(params),
                     ')')
    writer.line('{')
    request = 'request' if op.request is not None else 'NULL'
    if op.type == 'query':
        arguments = ('dev', names.op(op), request, 'reply', 'error')
        call = 'ctlv_query'
    elif op.reply is not None:
        arguments = ('dev', names.op(op), request, 'reply', 'error')
        call = 'ctlv_action'
    else:
        arguments = ('dev', names.op(op), request, '0', 'NULL', 'error')
        call = 'ctlv_action_sized'
    writer.parenthesized(f'\treturn {call}(', arguments, ');')
    writer.line('}')


def _writers(writer, names, family):
    """What an application puts into a request, one putter per attr."""
    attrs = [attr for group in family.local_groups for attr in group.attrs]
    if not attrs:
        return
    writer.line()
    writer.comment('Building a request. ctlv_msg_err() reports the first '
                   'failure.')
    for attr in attrs:
        writer.line()
        if attr.decl.kind == 'blob':
            _blob_putters(writer, names, attr)
        elif attr.decl.kind == 'nested':
            _nest_opener(writer, names, attr)
        else:
            _putter(writer, names, attr)


def _nest_opener(writer, names, attr):
    writer.signature('static inline int', names.attr_helper(attr, 'start'),
                     ('struct ctlv_msg *msg',), ')')
    writer.line('{')
    writer.parenthesized('\treturn ctlv_nest_start(',
                         ('msg', names.attr(attr)), ');')
    writer.line('}')


def _putter(writer, names, attr):
    params, put = WRITERS[attr.decl.kind]
    values = tuple(param.split()[-1].lstrip('*') for param in params)
    writer.signature('static inline int', names.attr_helper(attr, 'put'),
                     ('struct ctlv_msg *msg', *params), ')')
    writer.line('{')
    writer.parenthesized(f'\treturn {put}(',
                         ('msg', names.attr(attr), *values), ');')
    writer.line('}')


def _blob_putters(writer, names, attr):
    """One descriptor writer per backing this placement allows."""
    for first, backing in enumerate(attr.decl.blob.backings):
        suffix, params = cgen.BLOB_PUTTERS[backing]
        if first:
            writer.line()
        values = tuple(param.split()[-1].lstrip('*') for param in params)
        writer.signature('static inline int',
                         names.attr_helper(attr, f'put_{suffix}'),
                         ('struct ctlv_msg *msg', *params), ')')
        writer.line('{')
        writer.parenthesized(f'\treturn ctlv_put_blob_{suffix}(',
                             ('msg', names.attr(attr), *values), ');')
        writer.line('}')


def _readers(writer, names, family):
    """What an application reads out of a reply or an event record.

    A getter answers whether it read anything: a device wrote it, not the
    validator.
    """
    attrs = [attr for group in family.local_groups for attr in group.attrs
             if cgen.user_attr_readers(names, attr)]
    if not attrs:
        return
    writer.line()
    writer.comment('Reading a reply or a record. A getter answers whether it '
                   'could read.')
    for attr in attrs:
        writer.line()
        if attr.decl.max_occurrences != 1:
            _iterator(writer, names, attr)
        else:
            _present(writer, names, attr)
        if cgen.attr_getter_value(attr):
            writer.line()
            _getter(writer, names, attr)


def _present(writer, names, attr):
    writer.signature('static inline bool', names.attr_helper(attr, 'present'),
                     (NEST_PARAM,), ')')
    writer.line('{')
    writer.parenthesized('\treturn ctlv_attr_find(',
                         ('nest', names.attr(attr)), ');')
    writer.line('}')


def _iterator(writer, names, attr):
    """Every occurrence of a repeated attr, in the device's own order."""
    writer.define_call(f'{names.attr_iterator(attr)}(_attr, _nest)',
                       'ctlv_for_each_id', '_attr', '_nest',
                       names.attr(attr))


def _getter(writer, names, attr):
    """What one attr holds, or that it did not hold it.

    A single attr is looked up in the nest it is given, a repeated one is read
    where its iterator left the cursor.
    """
    getter = names.attr_helper(attr, 'get')
    single = attr.decl.max_occurrences == 1
    found = NEST_PARAM if single else f'const struct ctlv_attr *{FOUND}'

    if attr.decl.kind == 'nested':
        # A nest holds no value of its own: what a caller wants is the nest.
        writer.signature('static inline const struct ctlv_attr',
                         f'*{getter}', (NEST_PARAM,), ')')
        writer.line('{')
        writer.parenthesized('\treturn ctlv_attr_find(',
                             ('nest', names.attr(attr)), ');')
        writer.line('}')
        return

    params, reader, extra = READERS[attr.decl.kind]
    writer.signature('static inline bool', getter, (found, *params), ')')
    writer.line('{')
    if single:
        writer.line(f'\tconst struct ctlv_attr *{FOUND};')
        writer.line()
        writer.parenthesized(f'\t{FOUND} = ctlv_attr_find(',
                             ('nest', names.attr(attr)), ');')
    arguments = (FOUND,) + _reader_arguments(names, attr, extra)
    writer.parenthesized(f'\treturn {FOUND} && !{reader}(', arguments, ');')
    writer.line('}')


def _reader_arguments(names, attr, extra):
    """What the libctlv reader takes beyond the attr it reads.

    An enum and a flags payload are read against what this caller knows, so
    anything outside is reported as a newer device's.
    """
    definition = names.family.definitions.get(attr.decl.ref)
    if attr.decl.kind == 'flags':
        return (names.value_mask(definition),) + extra
    if attr.decl.kind == 'enum':
        return (str(max(definition.numbers())),) + extra
    return extra or ('value',)


def _events(writer, names, family):
    """What arrives unasked once it has been asked for."""
    events = [op for op in family.local_ops if op.type == 'event']
    if not events:
        return
    writer.line()
    writer.comment('Watching for events. A subscription is per open file '
                   'description.')
    for op in events:
        writer.line()
        _lengths(writer, names, op)
        writer.signature('static inline bool', names.user_op_helper(op, 'is'),
                         ('const struct ctlv_event *event',), ')')
        writer.line('{')
        writer.line(f'\treturn ctlv_event_op_id(event) == {names.op(op)};')
        writer.line('}')
        for what in ('subscribe', 'unsubscribe'):
            writer.line()
            writer.signature('static inline int',
                             names.user_op_helper(op, what),
                             (DEV_PARAM, ERROR_PARAM), ')')
            writer.line('{')
            writer.line(f'\tconst __u64 op_id = {names.op(op)};')
            writer.line()
            writer.line(f'\treturn ctlv_{what}(dev, &op_id, 1, error);')
            writer.line('}')


class Placement:
    """One node of a composed tree, with the members that arrive at it.

    A composed tree is what a device answers, extension points filled in.
    """

    def __init__(self, node, reenters=0):
        self.node = node
        self.reenters = reenters
        self.attrs = []
        self.optional_attrs = []
        # The arrays holding this node's members, once they have a name.
        self.attrs_symbol = None
        self.optional_symbol = None

    @property
    def members(self):
        return self.attrs + self.optional_attrs


def compose(family, root):
    """Return the composed tree of one op message."""
    return _place(family, root, ())


def _place(family, node, stack):
    """One node and everything below it, fills spliced in."""
    if node.reenters is not None:
        return Placement(node, reenters=_levels(node, stack))
    place = Placement(node)
    _splice(family, node, False, stack + (node,), place)
    return place


def _levels(node, stack):
    """How far up the node whose members are this one's as well.

    One means the node this one is a member of, as the core reports it.
    """
    for levels, above in enumerate(reversed(stack), start=1):
        if above.expands == node.reenters:
            return levels
    # The resolver rejects a re-entry of a set that nothing expanded above.
    raise AssertionError(f'{node.attr.full_name} re-enters {node.reenters} '
                         'below nothing that expands it')


def _splice(family, node, force_optional, stack, place):
    """The members of one level: what it declares, then what filled it."""
    for child in node.required_children:
        target = place.optional_attrs if force_optional else place.attrs
        target.append(_place(family, child, stack))
    for child in node.optional_children:
        place.optional_attrs.append(_place(family, child, stack))
    for stub in node.stubs:
        fill = _fill(family, stub)
        if fill is None:
            # Nothing in this chain filled it, so it contributes no member.
            continue
        _splice(family, fill.root, force_optional or stub.optional, stack,
                place)


def _fill(family, stub):
    """What this chain put into one extension point, if anything did."""
    for fill in family.chain_fills:
        if (fill.family_id, fill.set_id) == (stub.family_id, stub.set_id):
            return fill
    return None


def _sizing(place):
    """One composed tree as an IR node, for the length arithmetic."""
    if place.reenters:
        return place.node
    return dataclasses.replace(
        place.node,
        required_children=tuple(_sizing(member) for member in place.attrs),
        optional_children=tuple(_sizing(member)
                                for member in place.optional_attrs),
        stubs=())


def render_source(family):
    """Return the userspace binding source of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    if family.name != spec.CORE_FAMILY_NAME:
        cgen.user_symbols(family)

    ops = _spec_ops(family)
    writer = cgen.preamble(family, 'user', 'source')
    writer.includes(*_source_includes(family, names, ops))
    writer.comment('What each op\'s messages may hold, composed as the core '
                   'composes them.')
    writer.line()
    for entry in ops:
        for which, place in (('request', entry.request),
                             ('reply', entry.reply)):
            if place is not None:
                _tree(writer, names, entry.op, which, place)
    _table(writer, names, ops)
    _spec(writer, names, family, ops)
    return writer.text()


def _source_includes(family, names, ops):
    """libctlv, and this family's own names where it has any."""
    headers = ['<ctlv.h>']
    if _has_integer(ops):
        headers = ['<stdint.h>', '', '<ctlv.h>']
    if family.name != spec.CORE_FAMILY_NAME:
        headers += ['', names.user_header()]
    return tuple(headers)


def _has_integer(ops):
    """Whether any bound of any tree of these ops is an integer's."""
    def integers(place):
        node = place.node
        if not node.is_root and node.attr.decl.kind in ('uint', 'sint'):
            return True
        return any(integers(member) for member in place.members)

    return any(integers(place) for entry in ops
               for place in (entry.request, entry.reply) if place)


class Entry:
    """One op of a specification table: the op and its composed messages."""

    def __init__(self, family, op):
        self.op = op
        self.request = compose(family, op.request) if op.request else None
        self.reply = compose(family, op.reply) if op.reply else None
        where = f'{family.path}: op {op.name}'
        self.sizes = sizes.composed(where, op,
                                    _sizing(self.request) if self.request
                                    else None,
                                    _sizing(self.reply) if self.reply
                                    else None)


def _spec_ops(family):
    """Every op a device of this family answers, as this leaf composes it.

    An inherited op is described again, since a fill anywhere in the chain
    widens its message. The root's ops are libctlv's own table.
    """
    if family.name == spec.CORE_FAMILY_NAME:
        _reject_root_stub(family)
        ops = family.local_ops
    else:
        ops = [op for op in family.ops
               if family.owners[op.family_id] != spec.CORE_FAMILY_NAME]
    return tuple(Entry(family, op) for op in ops)


def _reject_root_stub(family):
    """One table serves every chain, so the root may not be widened."""
    for attr_set in family.sets.values():
        if attr_set.stub:
            raise CtlvError(f'{family.path}: attr set {attr_set.name}',
                            'the root family cannot declare an extension '
                            'point, because its ops are described once for '
                            'every chain')


def _tree(writer, names, op, which, place):
    """One message: the arrays of its levels, deepest first, then its root."""
    symbol = names.user_nodes(op, which)
    _arrays(writer, names, symbol, place, set())
    writer.comment(f'{op.name} {which}')
    writer.declaration('static const struct ctlv_schema_node',
                       f'{symbol} = {{')
    _members(writer, names, place, 1)
    writer.line('};')
    writer.line()


def _arrays(writer, names, path, place, taken):
    """The member arrays below place, each emitted before whoever holds it.

    An array is named after the path down to the placement it carries.
    """
    for member in place.members:
        if member.members:
            _arrays(writer, names, _path(path, member, taken), member, taken)
    for what, members in (('attrs', place.attrs),
                          ('optional_attrs', place.optional_attrs)):
        if not members:
            continue
        symbol = f'{path}_{what}'
        if what == 'attrs':
            place.attrs_symbol = symbol
        else:
            place.optional_symbol = symbol
        where = 'root' if place.node.is_root else place.node.attr.full_name
        writer.comment(f'{where}, {what.replace("_", " ")}')
        writer.declaration('static const struct ctlv_schema_node',
                           f'{symbol}[] = {{')
        for member in members:
            _node(writer, names, member)
        writer.line('};')
        writer.line()


def _path(path, place, taken):
    """The name of one placement below path, which no other placement has.

    Two placements of one attr in one nest are two nodes, so repeats are
    counted.
    """
    candidate = f'{path}_{cgen.ident(place.node.attr.name)}'
    symbol = candidate
    repeat = 1
    while symbol in taken:
        repeat += 1
        symbol = f'{candidate}_{repeat}'
    taken.add(symbol)
    return symbol


def _node(writer, names, place):
    """One member, as an element of the array that holds it."""
    node = place.node
    if place.reenters:
        writer.line(f'\t/* {node.attr.full_name}, again from '
                    f'{node.reenters} */')
    else:
        writer.line(f'\t/* {node.attr.full_name} */')
    writer.line('\t{')
    _members(writer, names, place, 2)
    writer.line('\t},')


def _members(writer, names, place, depth):
    """What one node says about itself, in the order the core reports it."""
    node = place.node
    tab = '\t' * depth
    if node.is_root:
        writer.line(f'{tab}.attr_id = CTLV_ATTR_ID_ROOT,')
        writer.line(f'{tab}.kind = CTLV_SCHEMA_NEST,')
    else:
        writer.line(f'{tab}.attr_id = {node.attr.attr_id:#018x}ULL,')
        writer.line(f'{tab}.kind = {cgen.KINDS[node.attr.decl.kind]},')
    if node.max_occurrences:
        writer.line(f'{tab}.max_occurrences = {node.max_occurrences},')
    else:
        writer.line(f'{tab}.max_occurrences = 0, /* unlimited */')
    _bounds(writer, names, place, tab)
    if place.attrs_symbol:
        writer.declaration(f'{tab}.attrs =', f'{place.attrs_symbol},')
        writer.line(f'{tab}.n_attrs = {len(place.attrs)},')
    if place.optional_symbol:
        writer.declaration(f'{tab}.optional_attrs =',
                           f'{place.optional_symbol},')
        writer.line(f'{tab}.n_optional_attrs = '
                    f'{len(place.optional_attrs)},')


def _bounds(writer, names, place, tab):
    """The bounds of the one form this payload takes, with the present mask."""
    fields = _fields(names, place)
    if not fields:
        return
    bits = [PRESENT[name] for name, _ in fields]
    _or_lines(writer, f'{tab}.present = ', bits)
    for name, value in fields:
        if name == 'blob':
            for member, part in zip(BLOB_MEMBERS, value):
                writer.line(f'{tab}.{member} = {part},')
        else:
            writer.line(f'{tab}.{name} = {value},')


def _or_lines(writer, opening, terms):
    """One or-ed list of bits, broken after an operator where it must be."""
    column = cgen.width(opening)
    indent = '\t' * (column // cgen.TAB_WIDTH) + ' ' * (column %
                                                        cgen.TAB_WIDTH)
    line = opening
    for first, term in enumerate(terms):
        part = term if not first else f' | {term}'
        if first and cgen.width(f'{line}{part} |') > cgen.LINE_LIMIT:
            writer.line(f'{line} |')
            line = indent + term
        else:
            line += part
    writer.line(f'{line},')


def _fields(names, place):
    """Every bounded member of one node, with the value the core reports."""
    node = place.node
    if node.is_root:
        return ()
    decl = node.attr.decl
    fields = []
    if decl.kind in ('uint', 'sint'):
        fields += [('min', _bound(decl.kind, decl.min)),
                   ('max', _bound(decl.kind, decl.max))]
    elif decl.kind == 'enum':
        # Values are numbered from one, so the highest of them is the bound.
        fields += [('min', '1'),
                   ('max', str(max(_numbers(names, decl.ref))))]
    elif decl.kind == 'flags':
        mask = 0
        for bit in _numbers(names, decl.ref):
            mask |= 1 << bit
        fields.append(('flags_mask', _u64(mask)))
    elif decl.kind in ('string', 'binary'):
        fields += [('min_length', _u64(decl.min_length)),
                   ('max_length', _u64(decl.max_length))]
    elif decl.kind == 'blob':
        fields.append(('blob', _blob(names, decl.blob)))
    if decl.unit is not None:
        fields += [('unit', f'CTLV_UNIT_{cgen.macro(decl.unit)}'),
                   ('scale', str(decl.scale))]
    if place.reenters:
        fields.append(('reenters', str(place.reenters)))
    return tuple(fields)


def _numbers(names, ref):
    """The numbers of the definition one attr refers to."""
    return names.family.definitions[ref].numbers()


def _blob(names, blob):
    """What a blob placement allows, in the order the core reports it."""
    direction = cgen.macro(spec.CORE_FAMILY_NAME, 'blob-direction',
                           blob.direction)
    types = dict(names.family.definitions[BLOB_TYPES].values)
    mask = 0
    for backing in blob.backings:
        mask |= 1 << types[backing]
    return (direction, _u64(mask), _u64(blob.max_length),
            _u64(blob.alignment))


def _bound(kind, value):
    extreme = EXTREMES.get((kind, value))
    if extreme:
        return extreme
    if kind == 'sint':
        return _s64(value)
    return _u64(value)


def _u64(value):
    """One unsigned value, hexadecimal once decimal stops being readable."""
    if value < 1 << 16:
        return str(value)
    return f'{value:#x}ULL'


def _s64(value):
    """One signed bound, as the unsigned payload the wire carries."""
    if value >= 0:
        return _u64(value)
    return f'(__u64)(__s64){value}'


def _table(writer, names, ops):
    """Every op of one table, with what its messages may hold."""
    if not ops:
        return
    writer.comment('Every op a device of this family answers, in chain '
                   'declaration order.')
    writer.declaration('static const struct ctlv_op_spec',
                       f'{names.user_op_specs()}[] = {{')
    for entry in ops:
        op = entry.op
        writer.line(f'\t/* {op.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.op_id = {op.op_id:#018x}ULL,')
        writer.line(f'\t\t.type = {OP_TYPES[op.type]},')
        if op.type != 'event':
            writer.line('\t\t.availability = '
                        f'{AVAILABILITIES[op.availability]},')
        for field, _ in LENGTHS:
            value = getattr(entry.sizes, field)
            if value:
                writer.line(f'\t\t.{field} = {value},')
        for which in ('request', 'reply'):
            if getattr(entry, which) is not None:
                writer.line(f'\t\t.{which} = '
                            f'&{names.user_nodes(op, which)},')
        writer.line('\t},')
    writer.line('};')
    writer.line()


def _spec(writer, names, family, ops):
    """This family's node of the specification graph."""
    writer.comment('What a device of this family is, for a caller holding one '
                   'against it.')
    writer.declaration('const struct ctlv_family_spec',
                       f'{names.user_spec()} = {{')
    writer.line(f'\t.name = "{family.name}",')
    writer.line(f'\t.family_id = {family.family_id},')
    if family.name != spec.CORE_FAMILY_NAME:
        parent = cgen.Names(family.chain[-2]).user_spec()
        writer.line(f'\t.parent = &{parent},')
    if ops:
        writer.line(f'\t.op = {names.user_op_specs()},')
        writer.line(f'\t.n_op = {len(ops)},')
    writer.line('};')
