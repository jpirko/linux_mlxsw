# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The kernel metadata of one family.

Each family describes only what it declares, with one pointer to the family it
inherits. Each op request and reply becomes one flat, breadth-first array of
struct ctlv_schema_node the core walks by index. IDs are emitted as numbers,
since the path of the header naming them is not part of the specification.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import sizes
from . import spec


def root_macro(*names):
    """Return a constant of the root family's generated UAPI header."""
    return cgen.macro(spec.CORE_FAMILY_NAME, *names)


# An op's type and availability reach userspace verbatim, named by the root.
OP_TYPES = {kind: root_macro('op-type', kind)
            for kind in ('action', 'query', 'event')}

AVAILABILITIES = {availability: root_macro('op-availability', availability)
                  for availability in spec.AVAILABILITIES}

SIZE_FIELDS = ('request_max_len', 'action_reply_max_len',
               'query_reply_min_len', 'query_reply_suggested_len',
               'event_record_max_len')

# Lengths that have to fit the inline message limit. An event record is checked
# against the event queue instead.
INLINE_FIELDS = ('request_max_len', 'action_reply_max_len',
                 'query_reply_min_len', 'query_reply_suggested_len')

# A bound written as its symbolic extreme stays readable.
U64_MAX = (1 << 64) - 1
S64_MIN = -(1 << 63)
S64_MAX = (1 << 63) - 1
EXTREMES = {
    ('uint', U64_MAX): 'U64_MAX',
    ('sint', S64_MIN): 'S64_MIN',
    ('sint', S64_MAX): 'S64_MAX',
}


@dataclasses.dataclass(frozen=True)
class FlatNode:
    """One node of a flattened tree, with its children as an index range."""

    node: object
    first_child: int
    n_children: int
    n_required_children: int


def flatten(root):
    """Return the nodes of one tree breadth first, the root first.

    A re-entering node points back at nodes already laid out, which is what
    keeps the array finite.
    """
    flat = []
    queue = [(root, {})]
    next_index = 1
    while queue:
        node, levels = queue.pop(0)
        if node.reenters is not None:
            first_child, n_children, n_required = levels[node.reenters]
            flat.append(FlatNode(node=node, first_child=first_child,
                                 n_children=n_children,
                                 n_required_children=n_required))
            continue
        children = node.children()
        level = (next_index if children else 0, len(children),
                 len(node.required_children))
        flat.append(FlatNode(node=node, first_child=level[0],
                             n_children=level[1],
                             n_required_children=level[2]))
        next_index += len(children)
        if node.expands:
            levels = levels | {node.expands: level}
        queue += [(child, levels) for child in children]
    return tuple(flat)


def stub_refs(root):
    """Return the stubs of one tree, each with the node index it joins."""
    refs = []
    for index, flat in enumerate(flatten(root)):
        for stub in flat.node.stubs:
            refs.append((index, stub))
    return tuple(refs)


def render_header(family):
    """Return the kernel header of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    guard = names.kernel_guard()
    local = family.local_ops

    writer = cgen.preamble(family, 'kernel', 'header')
    writer.comment(f'Schema metadata of the {family.name} CTLV family.')
    writer.line()
    writer.open_guard(guard)
    writer.includes(*_header_includes(family))

    writer.comment('Ops this family declares, in declaration order.')
    writer.define(names.n_ops(), str(len(local)))
    writer.line()
    if local:
        writer.declaration('extern const struct ctlv_op_schema',
                           f'{names.op_schemas()}[{names.n_ops()}];')
        writer.line()
    writer.declaration('extern const struct ctlv_family_schema',
                       f'{names.schema()};')
    writer.line()
    _message_decls(writer, names, family)
    writer.close_guard(guard)
    return writer.text()


def _message_decls(writer, names, family):
    """The messages the core builds itself, and the room each one needs."""
    for message in family.messages:
        writer.comment(f'The {message.name} message, built by the core.')
        writer.define(names.message_max_len(message), str(message.max_len))
        writer.line()
        writer.declaration('extern const struct ctlv_schema_tree',
                           f'{names.message_schema(message)};')
        writer.line()


def _header_includes(family):
    """<linux/ctlv.h>, and the parent's header when there is a parent."""
    if family.parent is None:
        return ('<linux/ctlv.h>',)
    parent = cgen.Names(family.chain[-2]).kernel_header()
    if parent.startswith('<'):
        return tuple(sorted(('<linux/ctlv.h>', parent)))
    return ('<linux/ctlv.h>', '', parent)


def render_source(family):
    """Return the kernel metadata source of family as text."""
    names = cgen.Names(family)
    cgen.symbols(family)
    trees = _trees(family)

    writer = cgen.preamble(family, 'kernel', 'source')
    writer.includes(*_includes(names, family, trees))
    _assumptions(writer, family)
    for op, which, root in trees:
        _nodes(writer, family, names.nodes(op, which), f'{op.name} {which}',
               root)
        _stubs(writer, names, op, which, root)
    _fills(writer, family, names)
    _messages(writer, family, names)
    _table(writer, names, family)
    _descriptor(writer, names, family)
    _uevent(writer, family)
    _limits(writer, family, trees)
    return writer.text()


def _trees(family):
    """Every tree of every local op, in the order they are emitted."""
    trees = []
    for op in family.local_ops:
        if op.request is not None:
            trees.append((op, 'request', op.request))
        if op.reply is not None:
            trees.append((op, 'reply', op.reply))
    return trees


def _roots(family, trees):
    """Every emitted tree root: an op's, a fill's and a core message's."""
    return [root for _, _, root in trees] + \
           [fill.root for fill in family.fills] + \
           [message.root for message in family.messages]


def _kinds(roots):
    return {flat.node.attr.decl.kind
            for root in roots for flat in flatten(root)
            if not flat.node.is_root}


def _includes(names, family, trees):
    headers = ['<linux/array_size.h>', '<linux/build_bug.h>',
               '<linux/ctlv.h>', '<linux/export.h>']
    # The root family's enums name op types, availabilities and blob backings.
    uapi = [cgen.uapi_header(spec.CORE_FAMILY_NAME, in_kernel=True)]
    kinds = _kinds(_roots(family, trees))
    if 'blob' in kinds:
        # BIT(), for the mask of permitted blob backings.
        headers.append('<linux/bits.h>')
    if kinds & {'uint', 'sint'}:
        # The symbolic extremes of an unbounded integer bound.
        headers.append('<linux/limits.h>')
    groups = [sorted(headers), uapi, [names.kernel_header()]]
    lines = []
    for group in groups:
        if not group:
            continue
        if lines:
            lines.append('')
        lines += group
    return lines


def _assumptions(writer, family):
    writer.comment('Every length below was computed from these.')
    writer.line('static_assert(sizeof(struct ctlv_attr) == '
                f'{sizes.ATTR_HDR_LEN});')
    writer.line(f'static_assert(CTLV_ALIGNTO == {sizes.ALIGNTO});')
    if _has_event(family):
        writer.line('static_assert(sizeof(struct ctlv_event_hdr) == '
                    f'{sizes.EVENT_HDR_LEN});')
    writer.line()


def _has_event(family):
    return any(op.type == 'event' for op in family.local_ops)


def _nodes(writer, family, symbol, what, root):
    writer.comment(what)
    writer.declaration('static const struct ctlv_schema_node',
                       f'{symbol}[] = {{')
    for flat in flatten(root):
        _node(writer, family, flat)
    writer.line('};')
    writer.line()


def _node(writer, family, flat):
    node = flat.node
    if node.is_root:
        writer.line('\t/* root */')
    elif node.reenters is not None:
        writer.line(f'\t/* {node.attr.full_name}, again from '
                    f'{node.reenters} */')
    else:
        writer.line(f'\t/* {node.attr.full_name} */')
    writer.line('\t{')
    if node.is_root:
        writer.line('\t\t.attr_id = CTLV_ATTR_ID_ROOT,')
        writer.line('\t\t.kind = CTLV_SCHEMA_NEST,')
    else:
        writer.line(f'\t\t.attr_id = {node.attr.attr_id:#018x}ULL,')
        writer.line(f'\t\t.kind = {cgen.KINDS[node.attr.decl.kind]},')
    if node.max_occurrences:
        writer.line(f'\t\t.max_occurrences = {node.max_occurrences},')
    else:
        writer.line('\t\t.max_occurrences = 0, /* unlimited */')
    if node.reenters is not None:
        writer.line('\t\t.reenters = true,')
    if flat.n_children:
        writer.line(f'\t\t.first_child = {flat.first_child},')
        writer.line(f'\t\t.n_children = {flat.n_children},')
        writer.line('\t\t.n_required_children = '
                    f'{flat.n_required_children},')
    if not node.is_root:
        _constraint(writer, family, node.attr.decl)
    writer.line('\t},')


def _stubs(writer, names, op, which, root):
    """The extension points of one tree, by the node they widen."""
    refs = stub_refs(root)
    if not refs:
        return
    writer.comment(f'{op.name} {which} extension points')
    writer.declaration('static const struct ctlv_stub_ref',
                       f'{names.stubs(op, which)}[] = {{')
    for index, stub in refs:
        writer.line(f'\t/* {stub.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.family_id = {stub.family_id},')
        writer.line(f'\t\t.set_id = {stub.set_id},')
        writer.line(f'\t\t.node = {index},')
        if stub.optional:
            writer.line('\t\t.optional = true,')
        writer.line('\t},')
    writer.line('};')
    writer.line()


def _fills(writer, family, names):
    """The members this family puts into the stubs it inherited."""
    if not family.fills:
        return
    for fill in family.fills:
        owner = family.owners[fill.family_id]
        _nodes(writer, family, names.fill_nodes(fill),
               f'{fill.name} of {owner}, filled here', fill.root)
    writer.comment('The extension points this family fills.')
    writer.declaration('static const struct ctlv_stub_fill',
                       f'{names.fills()}[] = {{')
    for fill in family.fills:
        writer.line(f'\t/* {fill.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.family_id = {fill.family_id},')
        writer.line(f'\t\t.set_id = {fill.set_id},')
        writer.line('\t\t.members = '
                    f'CTLV_SCHEMA_TREE({names.fill_nodes(fill)}),')
        writer.line('\t},')
    writer.line('};')
    writer.line()


def _messages(writer, family, names):
    """The messages the core builds where no op declared one."""
    for message in family.messages:
        _nodes(writer, family, names.message_nodes(message),
               f'{message.name}, which the core builds', message.root)
        writer.declaration('const struct ctlv_schema_tree',
                           f'{names.message_schema(message)} =')
        writer.line(f'\tCTLV_SCHEMA_TREE({names.message_nodes(message)});')
        writer.line(f'EXPORT_SYMBOL_NS_GPL({names.message_schema(message)}, '
                    '"CTLV");')
        writer.line()
        writer.assertion(names.message_max_len(message), '<=', message.limit)
        writer.line()


def _constraint(writer, family, decl):
    """The union member of one attr, and its unit when it declares one."""
    if decl.kind in ('uint', 'sint'):
        low = _bound(decl.kind, decl.min)
        high = _bound(decl.kind, decl.max)
        writer.line(f'\t\t.range = {{ .min = {low}, .max = {high} }},')
        if decl.unit is not None:
            writer.line(f'\t\t.unit = CTLV_UNIT_{cgen.macro(decl.unit)},')
            writer.line(f'\t\t.scale = {decl.scale},')
    elif decl.kind == 'enum':
        highest = max(family.definitions[decl.ref].numbers())
        writer.line(f'\t\t.range = {{ .min = 1, .max = {highest} }},')
    elif decl.kind == 'flags':
        mask = 0
        for bit in family.definitions[decl.ref].numbers():
            mask |= 1 << bit
        writer.line(f'\t\t.flags_mask = {mask:#x}ULL,')
    elif decl.kind in ('string', 'binary'):
        writer.line(f'\t\t.length = {{ .min_len = {decl.min_length}, '
                    f'.max_len = {decl.max_length} }},')
    elif decl.kind == 'blob':
        _blob(writer, decl.blob)


def _bound(kind, value):
    return EXTREMES.get((kind, value), str(value))


def _blob(writer, blob):
    writer.line('\t\t.blob = {')
    writer.line(f'\t\t\t.max_len = {blob.max_length},')
    writer.line(f'\t\t\t.alignment = {blob.alignment},')
    _backings(writer, blob.backings)
    direction = root_macro('blob-direction', blob.direction)
    writer.line(f'\t\t\t.direction = {direction},')
    writer.line('\t\t},')


def _backings(writer, backings):
    terms = [f'BIT({root_macro("blob-type", backing)})'
             for backing in backings]
    if len(terms) == 1:
        writer.line(f'\t\t\t.backings = {terms[0]},')
        return
    writer.line(f'\t\t\t.backings = {terms[0]} |')
    for term in terms[1:-1]:
        writer.line(f'\t\t\t\t    {term} |')
    writer.line(f'\t\t\t\t    {terms[-1]},')


def _table(writer, names, family):
    local = family.local_ops
    if not local:
        return
    writer.comment('The ops this family declares, in declaration order.')
    writer.declaration('const struct ctlv_op_schema',
                       f'{names.op_schemas()}[{names.n_ops()}] = {{')
    for op in local:
        writer.line(f'\t/* {op.name} */')
        writer.line('\t{')
        writer.line(f'\t\t.op_id = {op.op_id:#018x}ULL,')
        writer.line(f'\t\t.type = {OP_TYPES[op.type]},')
        writer.line(f'\t\t.availability = {AVAILABILITIES[op.availability]},')
        # Registration holds a descriptor's hooks against this.
        if op.pre_post:
            writer.line('\t\t.pre_post = true,')
        for which, root in (('request', op.request), ('reply', op.reply)):
            if root is not None:
                _tree_field(writer, names, op, which, root)
        _sizes(writer, op.sizes)
        writer.line('\t},')
    writer.line('};')
    writer.line()
    writer.assertion(f'ARRAY_SIZE({names.op_schemas()})', '==', names.n_ops())
    writer.line()


def _tree_field(writer, names, op, which, root):
    """One request or reply tree, with its extension points when it has any."""
    symbol = names.nodes(op, which)
    if not stub_refs(root):
        value = f'CTLV_SCHEMA_TREE({symbol}),'
        line = f'\t\t.{which} = {value}'
        if cgen.width(line) <= cgen.LINE_LIMIT:
            writer.line(line)
        else:
            writer.line(f'\t\t.{which} =')
            writer.line(f'\t\t\t{value}')
        return
    writer.line(f'\t\t.{which} =')
    writer.line(f'\t\t\tCTLV_SCHEMA_TREE_STUBS({symbol},')
    writer.line(f'\t\t\t\t\t       {names.stubs(op, which)}),')


def _sizes(writer, op_sizes):
    present = [(field, getattr(op_sizes, field)) for field in SIZE_FIELDS
               if getattr(op_sizes, field)]
    if not present:
        return
    writer.line('\t\t.sizes = {')
    for field, value in present:
        writer.line(f'\t\t\t.{field} = {value},')
    writer.line('\t\t},')


def _descriptor(writer, names, family):
    """This family's node of the schema graph."""
    writer.comment('The family ID is its position in the central registry.')
    writer.declaration('const struct ctlv_family_schema',
                       f'{names.schema()} = {{')
    writer.line(f'\t.name = "{family.name}",')
    writer.line(f'\t.family_id = {family.family_id},')
    parent = _parent_schema(family)
    if parent:
        writer.line(f'\t.parent = &{parent},')
    if family.local_ops:
        writer.line(f'\t.ops = {names.op_schemas()},')
        writer.line(f'\t.n_ops = {names.n_ops()},')
    if family.fills:
        writer.line(f'\t.fills = {names.fills()},')
        writer.line(f'\t.n_fills = ARRAY_SIZE({names.fills()}),')
    writer.line('};')
    # A chain may cross a module boundary, and only metadata crosses it.
    writer.line(f'EXPORT_SYMBOL_NS_GPL({names.schema()}, "CTLV");')
    writer.line()


def _parent_schema(family):
    """The schema this family inherits, none for the root of every chain."""
    if family.parent is None:
        return None
    return cgen.Names(family.chain[-2]).schema()


def _uevent(writer, family):
    """What this chain's names cost a device's uevent environment."""
    if family.parent is None:
        # No device answers the root, so no uevent announces it here.
        return
    names = [item.name for item in family.chain]
    variables = [f'CTLV_FAMILY={names[-1]}']
    variables += [f'CTLV_COMPATIBLE_{index}={name}'
                  for index, name in enumerate(names)]
    variables.append(f'CTLV_COMPATIBLE_N={len(names)}')
    # "KEY=value" plus the NUL, as add_uevent_var() charges it.
    writer.comment('What this family and the chain below it cost a uevent.')
    writer.line(f'static_assert({sum(len(item) + 1 for item in variables)} <= '
                'CTLV_UEVENT_MAX_LEN);')
    writer.line(f'static_assert({len(variables)} <= CTLV_UEVENT_MAX_VARS);')
    writer.line()


def _limits(writer, family, trees):
    local = family.local_ops
    if not local:
        return
    writer.comment('The generated numbers against the limits they were '
                   'computed from.')
    inline = max(getattr(op.sizes, field)
                 for op in local for field in INLINE_FIELDS)
    writer.line(f'static_assert({inline} <= CTLV_MAX_INLINE_MESSAGE_LEN);')
    if _has_event(family):
        record = max(op.sizes.event_record_max_len for op in local)
        writer.line(f'static_assert({record} <= CTLV_MAX_EVENT_QUEUE_LEN);')
    depth = max(_nest_depth(root) for root in _roots(family, trees))
    if depth:
        writer.line(f'static_assert({depth} <= '
                    'CTLV_MAX_DECLARED_NEST_DEPTH);')


def _nest_depth(node):
    """Deepest nest below node, which is itself at depth zero."""
    depths = [1 + _nest_depth(child) for child in node.children()
              if child.attr.decl.kind == 'nested']
    return max(depths, default=0)
