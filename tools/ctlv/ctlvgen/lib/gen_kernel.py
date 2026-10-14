# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The kernel metadata of one family.

Each family describes only what it declares. Each op request and reply becomes
one flat, breadth-first array of struct ctlv_node_schema the core walks by
index.
"""

from __future__ import annotations

import dataclasses
import re

from . import cgen
from . import gen_family
from . import semantics
from . import spec


def root_macro(*names: str) -> str:
    """Return a constant of the root family's generated UAPI header."""
    return cgen.macro(spec.CORE_FAMILY_NAME, *names)


FD_ACCESSES: dict[str, str] = {access: root_macro('op-fd-access', access)
                               for access in spec.FD_ACCESSES}

AVAILABILITIES: dict[str, str] = {
    availability: root_macro('op-availability', availability)
    for availability in spec.AVAILABILITIES
}

# Lengths that have to fit the inline message limit. An event is checked
# against the event queue instead.
INLINE_FIELDS: tuple[str, ...] = (
    'request_max_len', 'action_reply_max_len', 'query_reply_min_len',
    'query_reply_suggested_len')

# The limits a spec writes by name, as <linux/limits.h> spells them.
LIMITS: dict[int, str] = {value: cgen.macro(name)
                          for name, value in spec.NAMED_NUMBERS.items()}

_USES_LIMITS: re.Pattern[str] = re.compile(
    r'\b[SU](?:8|16|32|64)_(?:MIN|MAX)\b')
_USES_SIZES: re.Pattern[str] = re.compile(r'\bSZ_[0-9]+[KMG]\b')


@dataclasses.dataclass(frozen=True)
class FlatNode:
    """One node of a flattened tree, with its children as an index range."""

    node: semantics.Node
    first_child: int
    children_count: int
    required_children_count: int


def flatten(root: semantics.Node) -> tuple[FlatNode, ...]:
    """Return the nodes of one tree breadth first, the root first.

    A re-entering node points back at nodes already laid out, which is what
    keeps the array finite.
    """
    flat = []
    levels: dict[str, tuple[int, int, int]] = {}
    queue = [(root, levels)]
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


def ext_schemas(
        root: semantics.Node) -> tuple[tuple[int, semantics.Extension], ...]:
    """Return each extension point schema with the node index it joins."""
    schemas = []
    for index, flat in enumerate(flatten(root)):
        for extension in flat.node.extensions:
            schemas.append((index, extension))
    return tuple(schemas)


def render_header(family: semantics.Family, *, family_api: bool) -> str:
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

    _extensions(writer, names, family)
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
        gen_family.emit_header(writer, names, family)
    writer.close_guard(guard)
    return writer.text()


def _extensions(writer: cgen.Writer, names: cgen.Names,
                family: semantics.Family) -> None:
    """The ID of each extension point this family declares."""
    own = family.local_extensions
    if own:
        writer.comment('Extension points, as a descendant\'s fill names '
                       'one.')
        for extension in own:
            writer.define(names.extension(extension), str(extension.ext_id))
        writer.line()
    enums = family.local_enum_extensions
    if enums:
        writer.comment('Enum extension points, numbered apart from the '
                       'others.')
        for enum_extension in enums:
            writer.define(names.enum_extension(enum_extension),
                          str(enum_extension.ext_id))
        writer.line()


def _message_decls(writer: cgen.Writer, names: cgen.Names,
                   family: semantics.Family) -> None:
    """The messages the core builds itself, and the room each one needs."""
    for message in family.messages:
        writer.comment(f'The {message.name} message, built by the core.')
        writer.define(names.message_max_len(message), str(message.max_len))
        writer.line()
        writer.declaration('extern const struct ctlv_schema_tree',
                           f'{names.message_schema(message)};')
        writer.line()


def _header_includes(family: semantics.Family, names: cgen.Names,
                     family_api: bool) -> tuple[str, ...]:
    """The generated types, constants and direct parent API."""
    headers = ['<linux/ctlv_family.h>']
    parent = []
    if family.parent not in (None, spec.CORE_FAMILY_NAME):
        # Its extension IDs, which this family's fills name.
        parent = ['', cgen.Names(family.chain[-2]).kernel_header()]
    if not family_api:
        return tuple(headers + parent)
    headers += ['<linux/err.h>', '<linux/errno.h>', '',
                names.uapi_header(in_kernel=True)]
    return tuple(headers + parent)


def render_source(family: semantics.Family, *, family_api: bool) -> str:
    """Return one family's generated kernel source."""
    names = cgen.Names(family)
    cgen.symbols(family)
    trees = _trees(family)

    body = cgen.Writer()
    for op, which, root in trees:
        _node_schemas(body, family, names, names.node_schemas(op, which),
                      root)
        _ext_schemas(body, names, op, which, root)
    _ext_fill_schemas(body, family, names)
    _enum_fill_schemas(body, family, names)
    _messages(body, family, names)
    _table(body, names, family)
    _descriptor(body, names, family)
    _uevent(body, family)
    _limits(body, family, trees)
    if family_api:
        if family.local_ops:
            body.line()
        gen_family.emit_source(body, names, family)
    text = body.text()

    writer = cgen.preamble(family, 'kernel', 'source')
    if family_api:
        writer.line('#define _CTLV_GENERATED_METADATA_ONLY')
    writer.includes(*_includes(names, family, family_api, text))
    return f'{writer.text()}\n{text}'


_Tree = tuple[semantics.Op, str, semantics.Node]


def _trees(family: semantics.Family) -> list[_Tree]:
    """Every tree of every local op, in the order they are emitted."""
    trees = []
    for op in family.local_ops:
        if op.request is not None:
            trees.append((op, 'request', op.request))
        if op.reply is not None:
            trees.append((op, 'reply', op.reply))
    return trees


def _roots(family: semantics.Family,
           trees: list[_Tree]) -> list[semantics.Node]:
    """Every emitted tree root: an op's, a fill's and a core message's."""
    return [root for _, _, root in trees] + \
           [fill.root for fill in family.fills] + \
           [message.root for message in family.messages]


def _includes(names: cgen.Names, family: semantics.Family, family_api: bool,
              body: str) -> list[str]:
    headers = ['<linux/array_size.h>', '<linux/build_bug.h>',
               '<linux/ctlv_family.h>']
    if family_api:
        headers.append('<linux/export.h>')
    # The root family's enums name op types, availabilities and blob backings.
    uapi = [cgen.uapi_header(spec.CORE_FAMILY_NAME, in_kernel=True)]
    if family.name != spec.CORE_FAMILY_NAME:
        uapi.append(names.uapi_header(in_kernel=family_api))
    if 'BIT(' in body:
        # The mask of permitted blob backings.
        headers.append('<linux/bits.h>')
    if _USES_LIMITS.search(body):
        headers.append('<linux/limits.h>')
    if _USES_SIZES.search(body):
        headers.append('<linux/sizes.h>')
    groups = [sorted(headers), uapi, [names.kernel_header()]]
    lines: list[str] = []
    for group in groups:
        if not group:
            continue
        if lines:
            lines.append('')
        lines += group
    return lines


def _has_event(family: semantics.Family) -> bool:
    return any(op.type == 'event' for op in family.local_ops)


def _node_schemas(writer: cgen.Writer, family: semantics.Family,
                  names: cgen.Names, symbol: str,
                  root: semantics.Node) -> None:
    writer.declaration('static const struct ctlv_node_schema',
                       f'{symbol}[] = {{')
    for flat in flatten(root):
        _node(writer, family, names, flat)
    writer.line('};')
    writer.line()


def _node(writer: cgen.Writer, family: semantics.Family, names: cgen.Names,
          flat: FlatNode) -> None:
    macro, args = _node_initializer(family, names, flat)
    if macro == 'CTLV_NODE_SCHEMA_BLOB':
        _blob_node(writer, macro, args)
    else:
        writer.parenthesized(f'\t{macro}(', args, '),')


def _blob_node(writer: cgen.Writer, macro: str, args: tuple[str, ...]) -> None:
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


def _node_initializer(family: semantics.Family, names: cgen.Names,
                      flat: FlatNode) -> tuple[str, tuple[str, ...]]:
    node = flat.node

    if node.is_root:
        return 'CTLV_NODE_SCHEMA_ROOT', (
            str(flat.first_child),
            str(flat.children_count),
            str(flat.required_children_count),
        )

    decl = node.child_field.decl
    kind = cgen.KINDS[decl.kind].removeprefix('CTLV_SCHEMA_')
    macro = f'CTLV_NODE_SCHEMA_{kind}'
    occurrences = str(node.max_occurrences) if node.max_occurrences else \
        'CTLV_NODE_SCHEMA_UNLIMITED'
    args = [names.field(node.child_field), occurrences]
    if decl.kind == 'record':
        if node.reenters is not None:
            macro += '_REENTERS'
        args += [str(flat.first_child), str(flat.children_count),
                 str(flat.required_children_count)]
    else:
        if decl.kind in ('uint', 'sint') and decl.unit is not None:
            macro += '_UNIT'
        if decl.kind == 'enum':
            assert decl.ref is not None
            definition = family.definitions[decl.ref]
            if definition.header:
                macro += '_RANGE'
            elif definition.extension is not None:
                macro += '_EXT'
        args += _constraint_args(family, names, decl)
    return macro, tuple(args)


def _ext_schemas(writer: cgen.Writer, names: cgen.Names, op: semantics.Op,
                 which: str, root: semantics.Node) -> None:
    """The extension points of one tree, by the node they widen."""
    schemas = ext_schemas(root)
    if not schemas:
        return
    writer.comment('Extension points selected by this tree.')
    writer.declaration('static const struct ctlv_ext_schema',
                       f'{names.ext_schemas(op, which)}[] = {{')
    for index, extension in schemas:
        macro = 'CTLV_EXT_SCHEMA_OPTIONAL' if extension.optional else \
            'CTLV_EXT_SCHEMA'
        args = (cgen.family_const(names.effective.owners[extension.family_id]),
                names.extension(extension), str(index))
        writer.parenthesized(f'\t{macro}(', args, '),')
    writer.line('};')
    writer.line()


def _ext_fill_schemas(writer: cgen.Writer, family: semantics.Family,
                      names: cgen.Names) -> None:
    """The members this family puts into the extension points it inherited."""
    if not family.fills:
        return
    for fill in family.fills:
        _node_schemas(writer, family, names,
                      names.fill_node_schemas(fill), fill.root)
    writer.comment('The extension points this family fills.')
    writer.declaration('static const struct ctlv_ext_fill_schema',
                       f'{names.ext_fill_schemas()}[] = {{')
    for fill in family.fills:
        extension = family.extensions[fill.name]
        args = (cgen.family_const(family.owners[fill.family_id]),
                names.extension(extension), cgen.ident(fill.name))
        writer.parenthesized('\tCTLV_EXT_FILL_SCHEMA(', args, '),')
    writer.line('};')
    writer.line()


def _enum_fill_schemas(writer: cgen.Writer, family: semantics.Family,
                       names: cgen.Names) -> None:
    """How many values this family gives each enum extension point."""
    fills = family.enum_fills
    if not fills:
        return
    writer.comment('The enum extension points this family fills.')
    writer.declaration('static const struct ctlv_enum_fill_schema',
                       f'{names.enum_fill_schemas()}[] = {{')
    for fill in fills:
        assert fill.extension is not None
        args = (cgen.family_const(family.owners[fill.extension.family_id]),
                names.enum_extension(fill.extension), str(fill.max_value))
        writer.parenthesized('\tCTLV_ENUM_FILL_SCHEMA(', args, '),')
    writer.line('};')
    writer.line()


def _messages(writer: cgen.Writer, family: semantics.Family,
              names: cgen.Names) -> None:
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


def _constraint_args(family: semantics.Family, names: cgen.Names,
                     decl: spec.FieldDecl) -> list[str]:
    """The schema kind's arguments of one node-schema macro."""
    if decl.kind in ('uint', 'sint'):
        assert decl.min is not None and decl.max is not None
        args = [_bound(decl.kind, decl.min), _bound(decl.kind, decl.max)]
        if decl.unit is not None:
            args += [f'CTLV_UNIT_{cgen.macro(decl.unit)}', str(decl.scale)]
        return args
    if decl.kind == 'enum':
        assert decl.ref is not None
        definition = family.definitions[decl.ref]
        if definition.header:
            return list(cgen.header_bounds(definition))
        if definition.extension is not None:
            extension = definition.extension
            owner = names.effective.owners[extension.family_id]
            return [cgen.family_const(owner),
                    names.enum_extension(extension)]
        return [str(max(definition.numbers()))]
    if decl.kind == 'flags':
        assert decl.ref is not None
        return [names.value_mask(family.definitions[decl.ref])]
    if decl.kind in ('string', 'binary'):
        assert decl.min_length is not None and decl.max_length is not None
        return [_length(decl.min_length), _length(decl.max_length)]
    if decl.kind == 'blob':
        assert decl.blob is not None
        return _blob_args(decl.blob)
    return []


def _bound(kind: str, value: int) -> str:
    """One integer bound, a limit by the name the spec gives it."""
    if value in LIMITS:
        return LIMITS[value]
    if kind == 'uint' and value > spec.S64_MAX:
        return f'{value}ULL'
    return str(value)


def _length(value: int) -> str:
    """One byte count, as <linux/sizes.h> spells a power of two."""
    # Its largest G name is SZ_512G.
    if 1 << 10 <= value <= 1 << 39 and not value & (value - 1):
        return f'SZ_{spec.size_text(value)}'
    if value in (spec.U32_MAX, spec.U64_MAX):
        return LIMITS[value]
    if value > spec.S64_MAX:
        return f'{value}ULL'
    return str(value)


def _blob_args(blob: spec.BlobDecl) -> list[str]:
    direction = root_macro('blob-direction', blob.direction)
    return [_length(blob.max_length), _length(blob.alignment),
            _backings(blob.backings), direction]


def _backings(backings: tuple[str, ...]) -> str:
    terms = [f'BIT({root_macro("blob-type", backing)})'
             for backing in backings]
    return ' | '.join(terms)


def _table(writer: cgen.Writer, names: cgen.Names,
           family: semantics.Family) -> None:
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


def _op_schema_initializer(names: cgen.Names,
                           op: semantics.Op) -> tuple[str, tuple[str, ...]]:
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
        args = [names.op_id(op), str(sizes.event_max_len),
                _op_tree(op, 'reply')]
    if op.optional:
        args.append('.optional = true')
    if op.pre_post:
        args.append('.pre_post = true')
    return macro, tuple(args)


def _op_tree(op: semantics.Op, which: str) -> str:
    root = getattr(op, which)
    if root is None:
        return 'CTLV_SCHEMA_TREE_NONE()'
    macro = 'CTLV_SCHEMA_TREE_EXTS' if ext_schemas(root) else \
        'CTLV_SCHEMA_TREE'
    return f'{macro}({cgen.ident(op.name, which)})'


def _descriptor(writer: cgen.Writer, names: cgen.Names,
                family: semantics.Family) -> None:
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
        ext_fill_schemas = names.ext_fill_schemas()
        writer.line(f'\t.ext_fill_schemas = {ext_fill_schemas},')
        writer.line(f'\t.ext_fill_schemas_count = '
                    f'ARRAY_SIZE({ext_fill_schemas}),')
    if family.enum_fills:
        enum_fill_schemas = names.enum_fill_schemas()
        writer.line(f'\t.enum_fill_schemas = {enum_fill_schemas},')
        writer.line(f'\t.enum_fill_schemas_count = '
                    f'ARRAY_SIZE({enum_fill_schemas}),')
    writer.line('};')
    writer.line()


def _uevent(writer: cgen.Writer, family: semantics.Family) -> None:
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


def _limits(writer: cgen.Writer, family: semantics.Family,
            trees: list[tuple[semantics.Op, str, semantics.Node]]) -> None:
    local = family.local_ops
    if not local:
        return
    writer.comment('Guards for generated maxima.')
    inline = max(getattr(op.sizes, field)
                 for op in local for field in INLINE_FIELDS)
    writer.line(f'static_assert({inline} <= CTLV_MAX_INLINE_MESSAGE_LEN);')
    if _has_event(family):
        longest = max(op.sizes.event_max_len for op in local)
        writer.line(f'static_assert({longest} <= CTLV_MAX_EVENT_QUEUE_LEN);')
    depth = max(_record_depth(root) for root in _roots(family, trees))
    if depth:
        writer.line(f'static_assert({depth} <= '
                    'CTLV_MAX_DECLARED_RECORD_DEPTH);')


def _record_depth(node: semantics.Node) -> int:
    """Deepest record below node, which is itself at depth zero."""
    depths = [1 + _record_depth(child) for child in node.children()
              if child.child_field.decl.kind == 'record']
    return max(depths, default=0)
