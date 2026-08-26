# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The userspace binding of one family.

The header is names, and a child's includes its parent's. The source is the
specification as data: one struct ctlv_node_schema tree per op message, in the
shape op-schema-get reports, so a device can be compared with it.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import semantics
from . import sizes
from . import spec
from .errors import CtlvError

# What every wrapper starts and ends with. A caller wanting only the errno
# passes NULL for the error.
DEV_PARAM: str = 'struct ctlv_dev *dev'
ERROR_PARAM: str = 'struct ctlv_error *error'

# What a request is built in, and what an answer arrives in.
REQUEST_PARAM: str = 'struct ctlv_msg *request'
REPLY_PARAM: str = 'struct ctlv_reply **reply'

# The record a reader reads from, and the one TLV it found there.
RECORD_PARAM: str = 'const struct ctlv_hdr *record'
FOUND: str = 'hdr'

# What a getter of each schema kind hands back, and the libctlv reader that
# answers it. An enum and a flags payload are read as the integer they are.
READERS: dict[str, tuple[tuple[str, ...], str, tuple[str, ...]]] = {
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

# What a putter of each schema kind takes, and the libctlv append it wraps.
WRITERS: dict[str, tuple[tuple[str, ...], str]] = {
    'uint': (('__u64 value',), 'ctlv_put_u64'),
    'sint': (('__s64 value',), 'ctlv_put_s64'),
    'bool': (('bool value',), 'ctlv_put_bool'),
    'string': (('const char *value',), 'ctlv_put_str'),
    'binary': (('const void *value', '__u32 len'), 'ctlv_put_bin'),
    'enum': (('__u64 value',), 'ctlv_put_u64'),
    'flags': (('__u64 value',), 'ctlv_put_u64'),
}

# Which schema member each length of an op fills, and its name in the header.
LENGTHS: tuple[tuple[str, str], ...] = tuple(cgen.USER_LENGTHS.items())

# The lengths an op table entry holds, by sizes.OpSizes member.
TABLE_LENGTHS: tuple[str, ...] = tuple(
    field.name for field in dataclasses.fields(sizes.OpSizes))

# Bits of struct ctlv_node_schema's present mask, by the member they cover.
PRESENT: dict[str, str] = {
    'min': 'CTLV_NODE_SCHEMA_HAS_MIN',
    'max': 'CTLV_NODE_SCHEMA_HAS_MAX',
    'min_length': 'CTLV_NODE_SCHEMA_HAS_MIN_LENGTH',
    'max_length': 'CTLV_NODE_SCHEMA_HAS_MAX_LENGTH',
    'flags_mask': 'CTLV_NODE_SCHEMA_HAS_FLAGS_MASK',
    'unit': 'CTLV_NODE_SCHEMA_HAS_UNIT',
    'scale': 'CTLV_NODE_SCHEMA_HAS_SCALE',
    'blob': 'CTLV_NODE_SCHEMA_HAS_BLOB',
    'reenters': 'CTLV_NODE_SCHEMA_HAS_REENTERS',
}

# What a blob node contributes, in the order the core reports it, and the
# root family's enum naming the backings it allows.
BLOB_MEMBERS: tuple[str, ...] = ('blob_direction', 'blob_backings',
                                 'blob_max_length', 'blob_alignment')
BLOB_TYPES: str = 'blob-type'

# The extremes of an unbounded integer bound, stored as the unsigned pattern of
# the value. The most negative signed one has no literal.
EXTREMES: dict[tuple[str, int], str] = {
    ('uint', spec.U64_MAX): 'UINT64_MAX',
    ('sint', spec.S64_MIN): '(__u64)INT64_MIN',
    ('sint', spec.S64_MAX): 'INT64_MAX',
}

OP_TYPES: dict[str, str] = {
    op_type: cgen.macro(spec.CORE_FAMILY_NAME, 'op-type', op_type)
    for op_type in ('action', 'query')
}

FD_ACCESSES: dict[str, str] = {
    access: cgen.macro(spec.CORE_FAMILY_NAME, 'op-fd-access', access)
    for access in spec.FD_ACCESSES
}

AVAILABILITIES: dict[str, str] = {
    availability: cgen.macro(spec.CORE_FAMILY_NAME, 'op-availability',
                             availability)
    for availability in spec.AVAILABILITIES
}


def _reject_root(family: semantics.Family) -> None:
    """The root family's userspace API is provided by libctlv."""
    if family.name == spec.CORE_FAMILY_NAME:
        raise CtlvError(f'{family.path}: family {family.name}',
                        'the root family has no generated userspace binding, '
                        'because libctlv implements its ops directly as the '
                        'bootstrap every caller starts from')


def render_header(family: semantics.Family) -> str:
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
    writer.line()
    writer.close_guard(guard)
    return writer.text()


def _includes(family: semantics.Family, names: cgen.Names) -> tuple[str, ...]:
    """libctlv, this family's constants, and the parent's binding."""
    headers = ['<ctlv.h>', '', names.uapi_header()]
    if family.parent != spec.CORE_FAMILY_NAME:
        headers += ['', cgen.Names(family.chain[-2]).user_header()]
    return tuple(headers)


def _ops(writer: cgen.Writer, names: cgen.Names,
         family: semantics.Family) -> None:
    """One call per action and query this family declares.

    A wrapper needing a reply buffer asks the device for the length.
    """
    ops = family.local_ops
    if not ops:
        return
    writer.line()
    writer.comment('Executing an op. A query is reassembled into the one '
                   'reply it fills.')
    for op in ops:
        writer.line()
        if op.request is not None:
            _lengths(writer, names, op)
            _request_new(writer, names, op)
            writer.line()
        _wrapper(writer, names, op)


def _lengths(writer: cgen.Writer, names: cgen.Names, op: semantics.Op) -> None:
    """The longest request of one op, as this specification computed."""
    for field, what in LENGTHS:
        value = getattr(op.sizes, field)
        if value:
            writer.define(names.user_op_len(op, what), str(value))
    writer.line()


def _request_new(writer: cgen.Writer, names: cgen.Names,
                 op: semantics.Op) -> None:
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


def _wrapper(writer: cgen.Writer, names: cgen.Names, op: semantics.Op) -> None:
    """The one call that executes one op."""
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
    reply = 'reply' if op.reply is not None else 'NULL'
    call = 'ctlv_query' if op.type == 'query' else 'ctlv_action'
    arguments = ('dev', names.op_id(op), request, reply, 'error')
    writer.parenthesized(f'\treturn {call}(', arguments, ');')
    writer.line('}')


def _writers(writer: cgen.Writer, names: cgen.Names,
             family: semantics.Family) -> None:
    """What an application puts into a request, one putter per field.

    Every field has one, so a test can put a field where it does not belong.
    """
    fields = family.local_fields
    if not fields:
        return
    writer.line()
    writer.comment('Building a request. ctlv_msg_err() reports the first '
                   'failure.')
    for field in fields:
        writer.line()
        writer.doc(field.decl.doc)
        if field.decl.kind == 'blob':
            _blob_putters(writer, names, field)
        elif field.decl.kind == 'record':
            _record_opener(writer, names, field)
        else:
            _putter(writer, names, field)


def _record_opener(writer: cgen.Writer, names: cgen.Names,
                   field: semantics.Field) -> None:
    writer.signature('static inline int', names.field_helper(field, 'start'),
                     ('struct ctlv_msg *msg',), ')')
    writer.line('{')
    writer.parenthesized('\treturn ctlv_record_start(',
                         ('msg', names.field(field)), ');')
    writer.line('}')


def _putter(writer: cgen.Writer, names: cgen.Names,
            field: semantics.Field) -> None:
    params, put = WRITERS[field.decl.kind]
    values = tuple(param.split()[-1].lstrip('*') for param in params)
    writer.signature('static inline int', names.field_helper(field, 'put'),
                     ('struct ctlv_msg *msg', *params), ')')
    writer.line('{')
    writer.parenthesized(f'\treturn {put}(',
                         ('msg', names.field(field), *values), ');')
    writer.line('}')


def _blob_putters(writer: cgen.Writer, names: cgen.Names,
                  field: semantics.Field) -> None:
    """One descriptor writer per backing this field allows."""
    assert field.decl.blob is not None
    for first, backing in enumerate(field.decl.blob.backings):
        suffix, params = cgen.BLOB_PUTTERS[backing]
        if first:
            writer.line()
        values = tuple(param.split()[-1].lstrip('*') for param in params)
        writer.signature('static inline int',
                         names.field_helper(field, f'put_{suffix}'),
                         ('struct ctlv_msg *msg', *params), ')')
        writer.line('{')
        writer.parenthesized(f'\treturn ctlv_put_blob_{suffix}(',
                             ('msg', names.field(field), *values), ');')
        writer.line('}')


def _readers(writer: cgen.Writer, names: cgen.Names,
             family: semantics.Family) -> None:
    """What an application reads out of a reply.

    A getter answers whether it read anything: a device wrote it, not the
    validator.
    """
    fields = [field for field in family.local_fields
              if cgen.user_readers(names, field)]
    if not fields:
        return
    writer.line()
    writer.comment('Reading a reply. A getter answers whether it could '
                   'read.')
    for field in fields:
        writer.line()
        writer.doc(field.decl.doc)
        if field.decl.max_occurrences != 1:
            _iterator(writer, names, field)
        else:
            _present(writer, names, field)
        if cgen.getter_value(field):
            writer.line()
            _getter(writer, names, field)


def _present(writer: cgen.Writer, names: cgen.Names,
             field: semantics.Field) -> None:
    writer.signature('static inline bool',
                     names.field_helper(field, 'present'), (RECORD_PARAM,),
                     ')')
    writer.line('{')
    writer.parenthesized('\treturn ctlv_hdr_find(',
                         ('record', names.field(field)), ');')
    writer.line('}')


def _iterator(writer: cgen.Writer, names: cgen.Names,
              field: semantics.Field) -> None:
    """Every occurrence of a repeated field, in the device's own order."""
    writer.define_call(
        f'{names.field_helper(field, "for_each")}(_hdr, _record)',
        'ctlv_for_each_type', '_hdr', '_record', names.field(field))


def _getter(writer: cgen.Writer, names: cgen.Names,
            field: semantics.Field) -> None:
    """What one field holds, or that it did not hold it.

    A single field is looked up in the record it is given, a repeated one is
    read where its iterator left the cursor.
    """
    getter = names.field_helper(field, 'get')
    single = field.decl.max_occurrences == 1
    found = RECORD_PARAM if single else f'const struct ctlv_hdr *{FOUND}'

    if field.decl.kind == 'record':
        # A record holds no value of its own: what a caller wants is itself.
        writer.signature('static inline const struct ctlv_hdr',
                         f'*{getter}', (RECORD_PARAM,), ')')
        writer.line('{')
        writer.parenthesized('\treturn ctlv_hdr_find(',
                             ('record', names.field(field)), ');')
        writer.line('}')
        return

    kind = field.decl.kind
    if _extension_enum(names, field):
        # What is known depends on the device's family, so it is a number.
        kind = 'uint'
    params, reader, extra = READERS[kind]
    writer.signature('static inline bool', getter, (found, *params), ')')
    writer.line('{')
    if single:
        writer.line(f'\tconst struct ctlv_hdr *{FOUND};')
        writer.line()
        writer.parenthesized(f'\t{FOUND} = ctlv_hdr_find(',
                             ('record', names.field(field)), ');')
    arguments = (FOUND,) + _reader_arguments(names, field, extra)
    writer.parenthesized(f'\treturn {FOUND} && !{reader}(', arguments, ');')
    writer.line('}')


def _reader_arguments(names: cgen.Names, field: semantics.Field,
                      extra: tuple[str, ...]) -> tuple[str, ...]:
    """What the libctlv reader takes beyond the TLV it reads.

    An enum and a flags payload are read against what this caller knows, so
    anything outside is reported as a newer device's.
    """
    if (field.decl.kind not in ('enum', 'flags') or
            _extension_enum(names, field)):
        return extra or ('value',)
    assert field.decl.ref is not None
    definition = names.effective.definitions[field.decl.ref]
    if field.decl.kind == 'flags':
        return (names.value_mask(definition),) + extra
    if definition.header:
        return cgen.header_bounds(definition) + extra
    numbers = definition.numbers()
    return (str(min(numbers)), str(max(numbers))) + extra


def _extension_enum(names: cgen.Names, field: semantics.Field) -> bool:
    if field.decl.kind != 'enum':
        return False
    assert field.decl.ref is not None
    return names.effective.definitions[field.decl.ref].extension is not None


class ComposedNode:
    """One node of a composed tree, with the members that arrive at it.

    A composed tree is what a device answers, extension points filled in.
    """

    def __init__(self, node: semantics.Node, reenters: int = 0) -> None:
        self.node = node
        self.reenters = reenters
        self.fields: list[ComposedNode] = []
        self.optional_fields: list[ComposedNode] = []
        # The arrays holding this node's members, once they have a name.
        self.fields_symbol: str | None = None
        self.optional_symbol: str | None = None

    @property
    def members(self) -> list[ComposedNode]:
        return self.fields + self.optional_fields


def compose(family: semantics.Family, root: semantics.Node) -> ComposedNode:
    """Return the composed tree of one op message."""
    return _place(family, root, ())


def _place(family: semantics.Family, node: semantics.Node,
           stack: tuple[semantics.Node, ...]) -> ComposedNode:
    """One node and everything below it, fills spliced in."""
    if node.reenters is not None:
        return ComposedNode(node, reenters=_levels(node, stack))
    place = ComposedNode(node)
    _splice(family, node, False, stack + (node,), place)
    return place


def _levels(node: semantics.Node, stack: tuple[semantics.Node, ...]) -> int:
    """How far up the node whose members are this one's as well.

    One means the node this one is a member of, as the core reports it.
    """
    for levels, above in enumerate(reversed(stack), start=1):
        if above.expands == node.reenters:
            return levels
    # The resolver rejects a re-entry of a record nothing expanded above.
    raise AssertionError(f'{node.child_field.full_name} re-enters '
                         f'{node.reenters} below nothing that expands it')


def _splice(family: semantics.Family, node: semantics.Node,
            force_optional: bool, stack: tuple[semantics.Node, ...],
            place: ComposedNode) -> None:
    """The members of one level: what it declares, then what filled it."""
    for child in node.required_children:
        target = place.optional_fields if force_optional else place.fields
        target.append(_place(family, child, stack))
    for child in node.optional_children:
        place.optional_fields.append(_place(family, child, stack))
    for extension in node.extensions:
        fill = family.fill_of(extension)
        if fill is None:
            # Nothing in this chain filled it, so it contributes no member.
            continue
        _splice(family, fill.root, force_optional or extension.optional, stack,
                place)


def _sizing(place: ComposedNode) -> semantics.Node:
    """One composed tree as an IR node, for the length arithmetic."""
    if place.reenters:
        return place.node
    return dataclasses.replace(
        place.node,
        required_children=tuple(_sizing(member) for member in place.fields),
        optional_children=tuple(_sizing(member)
                                for member in place.optional_fields),
        extensions=())


def render_source(family: semantics.Family) -> str:
    """Return the userspace binding source of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    if family.name != spec.CORE_FAMILY_NAME:
        cgen.user_symbols(family)

    ops = _spec_ops(family)
    writer = cgen.preamble(family, 'user', 'source')
    writer.includes(*_source_includes(family, names, ops))
    writer.comment('Effective request and reply schemas.')
    writer.line()
    for entry in ops:
        for which, place in (('request', entry.request),
                             ('reply', entry.reply)):
            if place is not None:
                _tree(writer, names, entry.op, which, place)
    _table(writer, names, ops)
    _spec(writer, names, family, ops)
    return writer.text()


def _source_includes(family: semantics.Family, names: cgen.Names,
                     ops: tuple[Entry, ...]) -> tuple[str, ...]:
    """libctlv, and this family's own names where it has any."""
    headers = ['<ctlv.h>']
    if _has_integer(ops):
        headers = ['<stdint.h>', '', '<ctlv.h>']
    if family.name != spec.CORE_FAMILY_NAME:
        headers += ['', names.user_header()]
    return tuple(headers)


def _has_integer(ops: tuple[Entry, ...]) -> bool:
    """Whether any bound of any tree of these ops is an integer's."""
    def integers(place: ComposedNode) -> bool:
        node = place.node
        if not node.is_root and node.child_field.decl.kind in ('uint', 'sint'):
            return True
        return any(integers(member) for member in place.members)

    return any(integers(place) for entry in ops
               for place in (entry.request, entry.reply) if place)


class Entry:
    """One op of a specification table: the op and its composed messages."""

    def __init__(self, family: semantics.Family, op: semantics.Op) -> None:
        self.op = op
        self.request = compose(family, op.request) if op.request else None
        self.reply = compose(family, op.reply) if op.reply else None
        where = f'{family.path}: op {op.name}'
        self.sizes = sizes.composed(where, op,
                                    _sizing(self.request) if self.request
                                    else None,
                                    _sizing(self.reply) if self.reply
                                    else None)


def _spec_ops(family: semantics.Family) -> tuple[Entry, ...]:
    """Every op a device of this family answers, as the family composes it.

    An inherited op is described again, since a fill anywhere in the chain
    widens its message. The root's ops are libctlv's own table.
    """
    if family.name == spec.CORE_FAMILY_NAME:
        _reject_root_extension(family)
        ops = family.local_ops
    else:
        ops = tuple(op for op in family.ops
                    if family.owners[op.family_id] != spec.CORE_FAMILY_NAME)
    return tuple(Entry(family, op) for op in ops)


def _reject_root_extension(family: semantics.Family) -> None:
    """One table serves every chain, so the root may not be widened."""
    for extension in family.local_extensions:
        raise CtlvError(f'{family.path}: extension {extension.name}',
                        'the root family cannot declare an extension '
                        'point, because its ops are described once for '
                        'every chain')


def _tree(writer: cgen.Writer, names: cgen.Names, op: semantics.Op, which: str,
          place: ComposedNode) -> None:
    """One message: the arrays of its levels, deepest first, then its root."""
    symbol = names.user_node_schema(op, which)
    _arrays(writer, names, cgen.ident(op.name, which), place, set())
    writer.declaration('static const struct ctlv_node_schema',
                       f'{symbol} = {{')
    _members(writer, names, place, 1)
    writer.line('};')
    writer.line()


def _arrays(writer: cgen.Writer, names: cgen.Names, path: str,
            place: ComposedNode, taken: set[str]) -> None:
    """The member arrays below place, each emitted before whoever holds it.

    An array is named after the path down to the node it carries.
    """
    for member in place.members:
        if member.members:
            _arrays(writer, names, _path(path, member, taken), member, taken)
    for what, members in (('fields', place.fields),
                          ('optional_fields', place.optional_fields)):
        if not members:
            continue
        symbol = f'{path}_{what.removesuffix("s")}_node_schemas'
        if what == 'fields':
            place.fields_symbol = symbol
        else:
            place.optional_symbol = symbol
        where = 'root' if place.node.is_root else \
            place.node.child_field.full_name
        writer.comment(f'{where}, {what.replace("_", " ")}')
        writer.declaration('static const struct ctlv_node_schema',
                           f'{symbol}[] = {{')
        for member in members:
            _node(writer, names, member)
        writer.line('};')
        writer.line()


def _path(path: str, place: ComposedNode, taken: set[str]) -> str:
    """The name of one node below path, which no other node has.

    One field twice in one record is two nodes, so repeats are counted.
    """
    candidate = f'{path}_{cgen.ident(place.node.child_field.name)}'
    symbol = candidate
    repeat = 1
    while symbol in taken:
        repeat += 1
        symbol = f'{candidate}_{repeat}'
    taken.add(symbol)
    return symbol


def _node(writer: cgen.Writer, names: cgen.Names, place: ComposedNode) -> None:
    """One member, as an element of the array that holds it."""
    writer.line('\t{')
    _members(writer, names, place, 2)
    writer.line('\t},')


def _members(writer: cgen.Writer, names: cgen.Names, place: ComposedNode,
             depth: int) -> None:
    """What one node says about itself, in the order the core reports it."""
    node = place.node
    tab = '\t' * depth
    if node.is_root:
        writer.line(f'{tab}.type = CTLV_TYPE_ROOT,')
        writer.line(f'{tab}.kind = CTLV_SCHEMA_RECORD,')
    else:
        writer.declaration(f'{tab}.type =',
                           f'{names.field(node.child_field)},')
        writer.line(f'{tab}.kind = {cgen.KINDS[node.child_field.decl.kind]},')
    if node.max_occurrences:
        writer.line(f'{tab}.max_occurrences = {node.max_occurrences},')
    else:
        writer.line(f'{tab}.max_occurrences = 0, /* unlimited */')
    _bounds(writer, names, place, tab)
    if place.fields_symbol:
        writer.declaration(f'{tab}.fields =', f'{place.fields_symbol},')
        writer.line(f'{tab}.fields_count = {len(place.fields)},')
    if place.optional_symbol:
        writer.declaration(f'{tab}.optional_fields =',
                           f'{place.optional_symbol},')
        writer.line(f'{tab}.optional_fields_count = '
                    f'{len(place.optional_fields)},')


def _bounds(writer: cgen.Writer, names: cgen.Names, place: ComposedNode,
            tab: str) -> None:
    """The bounds of the schema kind this node has, with the present mask."""
    bounds = _bounded(names, place)
    if not bounds:
        return
    bits = [PRESENT[name] for name, _ in bounds]
    _or_lines(writer, f'{tab}.present = ', bits)
    for name, value in bounds:
        if name == 'blob':
            for member, part in zip(BLOB_MEMBERS, value):
                writer.line(f'{tab}.{member} = {part},')
        else:
            writer.line(f'{tab}.{name} = {value},')


def _or_lines(writer: cgen.Writer, opening: str, terms: list[str]) -> None:
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


def _bounded(
        names: cgen.Names,
        place: ComposedNode) -> tuple[tuple[str, str | tuple[str, ...]], ...]:
    """Every bounded member of one node, with the value the core reports."""
    node = place.node
    if node.is_root:
        return ()
    decl = node.child_field.decl
    bounds: list[tuple[str, str | tuple[str, ...]]] = []
    if decl.kind in ('uint', 'sint'):
        assert decl.min is not None and decl.max is not None
        bounds += [('min', _bound(decl.kind, decl.min)),
                   ('max', _bound(decl.kind, decl.max))]
    elif decl.kind == 'enum':
        assert decl.ref is not None
        definition = names.effective.definitions[decl.ref]
        if definition.extension is not None:
            # As the device's family fills it, so nothing when none did.
            bounds += [('min', '1'), ('max', str(definition.max_value))]
        elif definition.header:
            low, high = cgen.header_bounds(definition)
            bounds += [('min', low), ('max', high)]
        else:
            numbers = definition.numbers()
            bounds += [('min', str(min(numbers))),
                       ('max', str(max(numbers)))]
    elif decl.kind == 'flags':
        assert decl.ref is not None
        mask = 0
        for bit in _numbers(names, decl.ref):
            mask |= 1 << bit
        bounds.append(('flags_mask', _u64(mask)))
    elif decl.kind in ('string', 'binary'):
        assert decl.min_length is not None and decl.max_length is not None
        bounds += [('min_length', _u64(decl.min_length)),
                   ('max_length', _u64(decl.max_length))]
    elif decl.kind == 'blob':
        assert decl.blob is not None
        bounds.append(('blob', _blob(names, decl.blob)))
    if decl.unit is not None:
        bounds += [('unit', f'CTLV_UNIT_{cgen.macro(decl.unit)}'),
                   ('scale', str(decl.scale))]
    if place.reenters:
        bounds.append(('reenters', str(place.reenters)))
    return tuple(bounds)


def _numbers(names: cgen.Names, ref: str) -> tuple[int, ...]:
    """The numbers of the definition one field refers to."""
    return names.effective.definitions[ref].numbers()


def _blob(names: cgen.Names, blob: spec.BlobDecl) -> tuple[str, ...]:
    """What a blob node allows, in the order the core reports it."""
    direction = cgen.macro(spec.CORE_FAMILY_NAME, 'blob-direction',
                           blob.direction)
    types = dict(names.effective.definitions[BLOB_TYPES].values)
    mask = 0
    for backing in blob.backings:
        mask |= 1 << types[backing]
    return (direction, _u64(mask), _u64(blob.max_length),
            _u64(blob.alignment))


def _bound(kind: str, value: int) -> str:
    extreme = EXTREMES.get((kind, value))
    if extreme:
        return extreme
    if kind == 'sint':
        return _s64(value)
    return _u64(value)


def _u64(value: int) -> str:
    """One unsigned value, hexadecimal once decimal stops being readable."""
    if value < 1 << 16:
        return str(value)
    return f'{value:#x}ULL'


def _s64(value: int) -> str:
    """One signed bound, as the unsigned payload a message carries."""
    if value >= 0:
        return _u64(value)
    return f'(__u64)(__s64){value}'


def _table(writer: cgen.Writer, names: cgen.Names,
           ops: tuple[Entry, ...]) -> None:
    """Every op of one table, with what its messages may hold."""
    if not ops:
        return
    writer.comment('Effective ops in declaration order.')
    writer.declaration('static const struct ctlv_op_spec',
                       f'{names.user_op_specs()}[] = {{')
    for entry in ops:
        op = entry.op
        writer.line('\t{')
        writer.declaration('\t\t.op_id =', f'{names.op_id(op)},')
        writer.line(f'\t\t.type = {OP_TYPES[op.type]},')
        writer.line(f'\t\t.fd_access = {FD_ACCESSES[op.fd_access]},')
        writer.line('\t\t.availability = '
                    f'{AVAILABILITIES[op.availability]},')
        if op.optional:
            writer.line('\t\t.optional = true,')
        for field in TABLE_LENGTHS:
            value = getattr(entry.sizes, field)
            if value:
                writer.line(f'\t\t.{field} = {value},')
        for which in ('request', 'reply'):
            if getattr(entry, which) is not None:
                writer.line(f'\t\t.{which} = '
                            f'&{names.user_node_schema(op, which)},')
        writer.line('\t},')
    writer.line('};')
    writer.line()


def _spec(writer: cgen.Writer, names: cgen.Names, family: semantics.Family,
          ops: tuple[Entry, ...]) -> None:
    """This family's node of the specification graph."""
    writer.comment('Family specification descriptor.')
    writer.declaration('const struct ctlv_family_spec',
                       f'{names.user_spec()} = {{')
    writer.line(f'\t.name = "{family.name}",')
    writer.line(f'\t.family_id = {family.family_id},')
    if family.name != spec.CORE_FAMILY_NAME:
        parent = cgen.Names(family.chain[-2]).user_spec()
        writer.line(f'\t.parent = &{parent},')
    if ops:
        writer.line(f'\t.ops = {names.user_op_specs()},')
        writer.line(f'\t.ops_count = {len(ops)},')
    writer.line('};')
