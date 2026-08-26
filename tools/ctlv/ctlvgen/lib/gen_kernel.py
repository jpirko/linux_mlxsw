# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The kernel metadata of one family.

Each family describes only what it declares. Each op request and reply becomes
one flat, breadth-first array of struct ctlv_node_schema the core walks by
index.
"""

from __future__ import annotations

import dataclasses

from . import cgen
from . import gen_family
from . import semantics
from . import spec


def root_macro(*names):
    """Return a constant of the root family's generated UAPI header."""
    return cgen.macro(spec.CORE_FAMILY_NAME, *names)


FD_ACCESSES = {access: root_macro('op-fd-access', access)
               for access in spec.FD_ACCESSES}

AVAILABILITIES = {availability: root_macro('op-availability', availability)
                  for availability in spec.AVAILABILITIES}

# Lengths that have to fit the inline message limit. An event record is checked
# against the event queue instead.
INLINE_FIELDS = ('request_max_len', 'action_reply_max_len',
                 'query_reply_min_len', 'query_reply_suggested_len')

# A bound written as its symbolic extreme stays readable.
U64_MAX = (1 << 64) - 1
S64_MIN = -(1 << 63)
S64_MAX = (1 << 63) - 1
EXTREMES = {
    ('uint', spec.U32_MAX): 'U32_MAX',
    ('uint', U64_MAX): 'U64_MAX',
    ('sint', S64_MIN): 'S64_MIN',
    ('sint', S64_MAX): 'S64_MAX',
}


@dataclasses.dataclass(frozen=True)
class FlatNode:
    """One node of a flattened tree, with its children as an index range."""

    node: object
    first_child: int
    children_count: int
    required_children_count: int


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
            first_child, children_count, required_count = levels[node.reenters]
            flat.append(FlatNode(node=node, first_child=first_child,
                                 children_count=children_count,
                                 required_children_count=required_count))
            continue
        children = node.children()
        level = (next_index if children else 0, len(children),
                 len(node.required_children))
        flat.append(FlatNode(node=node, first_child=level[0],
                             children_count=level[1],
                             required_children_count=level[2]))
        next_index += len(children)
        if node.expands:
            levels = levels | {node.expands: level}
        queue += [(child, levels) for child in children]
    return tuple(flat)


def stub_schemas(root):
    """Return each stub schema with the node index it joins."""
    schemas = []
    for index, flat in enumerate(flatten(root)):
        for stub in flat.node.stubs:
            schemas.append((index, stub))
    return tuple(schemas)


def render_header(family, *, family_api):
    """Return one family's generated kernel header."""
    names = cgen.Names(family)
    cgen.symbols(family)
    guard = names.kernel_guard()
    local = family.local_ops

    writer = cgen.preamble(family, 'kernel', 'header')
    what = 'Kernel metadata and family API' if family_api else \
           'Schema metadata'
    writer.comment(f'{what} of the {family.name} CTLV family.')
    writer.line()
    writer.open_guard(guard)
    writer.includes(*_header_includes(family, names, family_api))

    if local:
        writer.comment('Ops this family declares, in declaration order.')
        writer.declaration('extern const struct ctlv_op_schema',
                           f'{names.op_schemas()}[];')
        writer.line()
    writer.declaration('extern const struct ctlv_family_schema',
                       f'{names.schema()};')
    writer.line()
    _message_decls(writer, names, family)
    if family_api:
        gen_family.emit_header(writer, names, family,
                               _reader_scopes(family, names))
    writer.close_guard(guard)
    return writer.text()


def _reader_scopes(family, names):
    scopes = []
    indexes = {}
    for scope in semantics.input_scopes(family):
        if scope.kind == 'op':
            root = family.local_ops[scope.index].request
            tree = f'&{names.op_schemas()}[{scope.index}].request'
        else:
            root = family.fills[scope.index].root
            tree = f'&{names.schema()}.stub_fill_schemas[' \
                   f'{scope.index}].members'
        key = (scope.kind, scope.index)
        if key not in indexes:
            indexes[key] = {id(flat.node): index
                            for index, flat in enumerate(flatten(root))}
        scopes.append(gen_family.ReaderScope(
            name=scope.name, node=scope.node, tree=tree,
            node_index=indexes[key][id(scope.node)]))
    return tuple(scopes)


def _message_decls(writer, names, family):
    """The messages the core builds itself, and the room each one needs."""
    for message in family.messages:
        writer.comment(f'The {message.name} message, built by the core.')
        writer.define(names.message_max_len(message), str(message.max_len))
        writer.line()
        writer.declaration('extern const struct ctlv_schema_tree',
                           f'{names.message_schema(message)};')
        writer.line()


def _header_includes(family, names, family_api):
    """The generated types, constants and direct parent API."""
    headers = ['<linux/ctlv_family.h>']
    if not family_api:
        return tuple(headers)
    headers += ['<linux/err.h>', '<linux/errno.h>', '',
                names.uapi_header(in_kernel=True)]
    if family.parent not in (None, spec.CORE_FAMILY_NAME):
        headers += ['', cgen.Names(family.chain[-2]).kernel_header()]
    return tuple(headers)


def render_source(family, *, family_api):
    """Return one family's generated kernel source."""
    names = cgen.Names(family)
    cgen.symbols(family)
    trees = _trees(family)

    writer = cgen.preamble(family, 'kernel', 'source')
    if family_api:
        writer.line('#define _CTLV_GENERATED_METADATA_ONLY')
    writer.includes(*_includes(names, family, trees, family_api))
    for op, which, root in trees:
        _node_schemas(writer, family, names,
                      names.node_schemas(op, which), root)
        _stub_schemas(writer, family, names, op, which, root)
    _stub_fill_schemas(writer, family, names)
    _messages(writer, family, names)
    _table(writer, names, family)
    _descriptor(writer, names, family)
    _uevent(writer, family)
    _limits(writer, family, trees)
    if family_api:
        if family.local_ops:
            writer.line()
        gen_family.emit_source(writer, names, family)
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


def _includes(names, family, trees, family_api):
    headers = ['<linux/array_size.h>', '<linux/build_bug.h>',
               '<linux/ctlv_family.h>']
    if family_api:
        headers.append('<linux/export.h>')
    # The root family's enums name op types, availabilities and blob backings.
    uapi = [cgen.uapi_header(spec.CORE_FAMILY_NAME, in_kernel=True)]
    if family.name != spec.CORE_FAMILY_NAME:
        uapi.append(names.uapi_header(in_kernel=family_api))
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


def _has_event(family):
    return any(op.type == 'event' for op in family.local_ops)


def _node_schemas(writer, family, names, symbol, root):
    writer.declaration('static const struct ctlv_node_schema',
                       f'{symbol}[] = {{')
    for flat in flatten(root):
        _node(writer, family, names, flat)
    writer.line('};')
    writer.line()


def _node(writer, family, names, flat):
    macro, args = _node_initializer(family, names, flat)
    if macro == 'CTLV_NODE_SCHEMA_BLOB':
        _blob_node(writer, macro, args)
    else:
        writer.parenthesized(f'\t{macro}(', args, '),')


def _blob_node(writer, macro, args):
    opening = f'\t{macro}('

    for line in cgen.wrapped(opening, args[:4], ','):
        writer.line(line)
    column = cgen.width(opening)
    indent = '\t' * (column // cgen.TAB_WIDTH) + \
        ' ' * (column % cgen.TAB_WIDTH)
    backings = args[4].split(' | ')
    for backing in backings[:-1]:
        writer.line(f'{indent}{backing} |')
    writer.line(f'{indent}{backings[-1]},')
    writer.line(f'{indent}{args[5]}),')


def _node_initializer(family, names, flat):
    node = flat.node

    if node.is_root:
        return 'CTLV_NODE_SCHEMA_ROOT', (
            str(flat.first_child),
            str(flat.children_count),
            str(flat.required_children_count),
        )

    decl = node.attr.decl
    kind = cgen.KINDS[decl.kind].removeprefix('CTLV_SCHEMA_')
    macro = f'CTLV_NODE_SCHEMA_{kind}'
    occurrences = str(node.max_occurrences) if node.max_occurrences else \
        'CTLV_NODE_SCHEMA_UNLIMITED'
    args = [names.attr(node.attr), occurrences]
    if decl.kind == 'nested':
        if node.reenters is not None:
            macro += '_REENTERS'
        args += [str(flat.first_child), str(flat.children_count),
                 str(flat.required_children_count)]
    else:
        if decl.kind in ('uint', 'sint') and decl.unit is not None:
            macro += '_UNIT'
        args += _constraint_args(family, names, decl)
    return macro, tuple(args)


def _stub_schemas(writer, family, names, op, which, root):
    """The extension points of one tree, by the node they widen."""
    schemas = stub_schemas(root)
    if not schemas:
        return
    writer.comment('Extension points selected by this tree.')
    writer.declaration('static const struct ctlv_stub_schema',
                       f'{names.stub_schemas(op, which)}[] = {{')
    for index, stub in schemas:
        macro = 'CTLV_STUB_SCHEMA_OPTIONAL' if stub.optional else \
            'CTLV_STUB_SCHEMA'
        attr_set = family.sets[stub.name]
        args = (cgen.family_const(family.owners[stub.family_id]),
                names.attr_set(attr_set), str(index))
        writer.parenthesized(f'\t{macro}(', args, '),')
    writer.line('};')
    writer.line()


def _stub_fill_schemas(writer, family, names):
    """The members this family puts into the stubs it inherited."""
    if not family.fills:
        return
    for fill in family.fills:
        _node_schemas(writer, family, names,
                      names.fill_node_schemas(fill), fill.root)
    writer.comment('The extension points this family fills.')
    writer.declaration('static const struct ctlv_stub_fill_schema',
                       f'{names.stub_fill_schemas()}[] = {{')
    for fill in family.fills:
        attr_set = family.sets[fill.name]
        args = (cgen.family_const(family.owners[fill.family_id]),
                names.attr_set(attr_set), cgen.ident(fill.name))
        writer.parenthesized('\tCTLV_STUB_FILL_SCHEMA(', args, '),')
    writer.line('};')
    writer.line()


def _messages(writer, family, names):
    """The messages the core builds where no op declared one."""
    for message in family.messages:
        _node_schemas(writer, family, names,
                      names.message_node_schemas(message), message.root)
        writer.declaration('const struct ctlv_schema_tree',
                           f'{names.message_schema(message)} = {{')
        node_schemas = names.message_node_schemas(message)
        writer.line(f'\t.node_schemas = {node_schemas},')
        writer.line('\t.node_schemas_count =')
        writer.line(f'\t\tARRAY_SIZE({node_schemas}),')
        writer.line('};')
        writer.line()
        writer.assertion(names.message_max_len(message), '<=', message.limit)
        writer.line()


def _constraint_args(family, names, decl):
    """The kind-specific arguments of one node-schema macro."""
    if decl.kind in ('uint', 'sint'):
        low = _bound(decl.kind, decl.min)
        high = _bound(decl.kind, decl.max)
        args = [low, high]
        if decl.unit is not None:
            args += [f'CTLV_UNIT_{cgen.macro(decl.unit)}', str(decl.scale)]
        return args
    if decl.kind == 'enum':
        highest = max(family.definitions[decl.ref].numbers())
        return [str(highest)]
    if decl.kind == 'flags':
        return [names.value_mask(family.definitions[decl.ref])]
    if decl.kind in ('string', 'binary'):
        return [str(decl.min_length), str(decl.max_length)]
    if decl.kind == 'blob':
        return _blob_args(decl.blob)
    return []


def _bound(kind, value):
    extreme = EXTREMES.get((kind, value))

    if extreme:
        return extreme
    if kind == 'uint' and value > spec.S64_MAX:
        return f'{value}ULL'
    return str(value)


def _blob_args(blob):
    direction = root_macro('blob-direction', blob.direction)
    return [_bound('uint', blob.max_length), str(blob.alignment),
            _backings(blob.backings), direction]


def _backings(backings):
    terms = [f'BIT({root_macro("blob-type", backing)})'
             for backing in backings]
    return ' | '.join(terms)


def _table(writer, names, family):
    local = family.local_ops
    if not local:
        return
    writer.comment('The ops this family declares, in declaration order.')
    writer.declaration('const struct ctlv_op_schema',
                       f'{names.op_schemas()}[] = {{')
    for op in local:
        macro, args = _op_schema_initializer(names, op)
        writer.parenthesized(f'\t{macro}(', args, '),')
    writer.line('};')
    writer.line()


def _op_schema_initializer(names, op):
    sizes = op.sizes
    if op.type == 'action':
        macro = 'CTLV_OP_SCHEMA_ACTION'
        args = [names.op_id(op), FD_ACCESSES[op.fd_access],
                AVAILABILITIES[op.availability],
                str(sizes.request_max_len),
                str(sizes.action_reply_max_len),
                _op_tree(op, 'request'), _op_tree(op, 'reply')]
    elif op.type == 'query':
        macro = 'CTLV_OP_SCHEMA_QUERY'
        args = [names.op_id(op), FD_ACCESSES[op.fd_access],
                AVAILABILITIES[op.availability],
                str(sizes.request_max_len),
                str(sizes.query_reply_min_len),
                str(sizes.query_reply_suggested_len),
                _op_tree(op, 'request'), _op_tree(op, 'reply')]
    else:
        macro = 'CTLV_OP_SCHEMA_EVENT'
        args = [names.op_id(op), str(sizes.event_record_max_len),
                _op_tree(op, 'reply')]
    if op.optional:
        args.append('.optional = true')
    if op.pre_post:
        args.append('.pre_post = true')
    return macro, tuple(args)


def _op_tree(op, which):
    root = getattr(op, which)
    if root is None:
        return 'CTLV_SCHEMA_TREE_NONE()'
    macro = 'CTLV_SCHEMA_TREE_STUBS' if stub_schemas(root) else \
        'CTLV_SCHEMA_TREE'
    return f'{macro}({cgen.ident(op.name, which)})'


def _descriptor(writer, names, family):
    """This family's node of the schema graph."""
    writer.comment('The family ID is its position in the central registry.')
    writer.declaration('const struct ctlv_family_schema',
                       f'{names.schema()} = {{')
    writer.line(f'\t.name = "{family.name}",')
    writer.line(f'\t.family_id = {family.family_id},')
    if family.local_ops:
        writer.line(f'\t.ops = {names.op_schemas()},')
        writer.line(f'\t.ops_count = ARRAY_SIZE({names.op_schemas()}),')
    if family.fills:
        stub_fill_schemas = names.stub_fill_schemas()
        writer.line(f'\t.stub_fill_schemas = {stub_fill_schemas},')
        writer.line(f'\t.stub_fill_schemas_count = '
                    f'ARRAY_SIZE({stub_fill_schemas}),')
    writer.line('};')
    writer.line()


def _uevent(writer, family):
    """What this chain's names cost a device's uevent environment."""
    names = [item.name for item in family.chain]
    variables = [f'CTLV_FAMILY={names[-1]}']
    variables += [f'CTLV_COMPATIBLE_{index}={name}'
                  for index, name in enumerate(names)]
    variables.append(f'CTLV_COMPATIBLE_N={len(names)}')
    # "KEY=value" plus the NUL, as add_uevent_var() charges it.
    writer.comment('Maximum uevent cost for this family chain.')
    writer.line(f'static_assert({sum(len(item) + 1 for item in variables)} <= '
                'CTLV_UEVENT_MAX_LEN);')
    writer.line(f'static_assert({len(variables)} <= CTLV_UEVENT_MAX_VARS);')
    writer.line()


def _limits(writer, family, trees):
    local = family.local_ops
    if not local:
        return
    writer.comment('Guards for generated maxima.')
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
