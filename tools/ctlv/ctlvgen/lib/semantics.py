# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The effective form of a CTLV family.

The specifications of one inheritance chain resolved into one schema: numeric
IDs from declaration positions, selectors expanded into placements, and one
tree per op request and reply. A rule enforced here is not re-enforced
downstream.
"""

from __future__ import annotations

import dataclasses

from . import sizes
from . import spec
from .errors import CtlvError

MAX_FLAG_BITS = 64


@dataclasses.dataclass(frozen=True)
class Definition:
    """An effective enum or flags definition."""

    name: str
    kind: str
    family_id: int
    values: tuple

    def numbers(self):
        return tuple(number for _, number in self.values)


@dataclasses.dataclass(frozen=True)
class Attr:
    """An effective attr, with its complete wire ID."""

    group: str
    name: str
    family_id: int
    group_id: int
    local_id: int
    attr_id: int
    decl: spec.AttrDecl

    @property
    def full_name(self):
        return f'{self.group}.{self.name}'


@dataclasses.dataclass(frozen=True)
class Group:
    name: str
    family_id: int
    group_id: int
    attrs: tuple


@dataclasses.dataclass(frozen=True)
class AttrSet:
    """An effective attr set. A stub that nobody filled stays a stub."""

    name: str
    family_id: int
    set_id: int
    stub: bool
    attrs: tuple
    optional_attrs: tuple


@dataclasses.dataclass(frozen=True)
class Stub:
    """One place where a descendant's fill of a stub splices in.

    The declaring family's metadata carries this in place of those members.
    """

    name: str
    family_id: int
    set_id: int
    optional: bool


@dataclasses.dataclass(frozen=True)
class Fill:
    """The members one family puts into a stub it inherited.

    root stands for the stub itself, so its children are what splices in.
    """

    name: str
    family_id: int
    set_id: int
    root: Node


@dataclasses.dataclass(frozen=True)
class Node:
    """One node of an expanded op tree. The root has no attr.

    stubs are the extension points selected here and still unfilled. A
    recursive nest becomes a node naming the set in expands and childless
    nodes below it naming it in reenters, so walking children terminates.
    """

    attr: Attr
    required: bool
    max_occurrences: int
    required_children: tuple
    optional_children: tuple
    stubs: tuple = ()
    expands: str = None
    reenters: str = None

    @property
    def is_root(self):
        return self.attr is None

    def children(self):
        return self.required_children + self.optional_children


@dataclasses.dataclass(frozen=True)
class Message:
    """One message root that no op declares, built by the core unasked.

    Only the root family declares one.
    """

    name: str
    family_id: int
    set_id: int
    root: Node
    max_len: int
    limit: str


@dataclasses.dataclass(frozen=True)
class Op:
    """An effective op.

    availability is the permission to enable and disable the op per device,
    not a state. pre_post asks for a pair of this op's own, on top of the
    family-wide one.
    """

    name: str
    type: str
    availability: str
    pre_post: bool
    family_id: int
    local_id: int
    op_id: int
    request: Node
    reply: Node
    sizes: sizes.OpSizes


@dataclasses.dataclass(frozen=True)
class Family:
    """One effective family: itself plus everything it inherits.

    owners maps a family ID to its name for every family in the chain. fills
    is what a generator emits, chain_fills what a device answers with.
    """

    name: str
    family_id: int
    path: str
    chain: tuple
    owners: dict
    definitions: dict
    groups: dict
    attrs: dict
    sets: dict
    ops: tuple
    fills: tuple
    chain_fills: tuple
    messages: tuple

    @property
    def parent(self):
        return self.chain[-1].inherits

    @property
    def license(self):
        return self.chain[-1].license

    def _local(self, items):
        return tuple(item for item in items
                     if item.family_id == self.family_id)

    @property
    def local_definitions(self):
        return self._local(self.definitions.values())

    @property
    def local_groups(self):
        return self._local(self.groups.values())

    @property
    def local_sets(self):
        """Sets this family declares, a stub it inherited excluded."""
        return self._local(self.sets.values())

    @property
    def local_ops(self):
        return self._local(self.ops)


def load(paths):
    """Load the family at paths.spec, resolved against its whole chain."""
    registry = spec.load_registry(paths)
    return _Resolver(paths, registry).resolve()


class _Resolver:
    def __init__(self, paths, registry):
        self.paths = paths
        self.registry = registry
        self.definitions = {}
        self.groups = {}
        self.attrs = {}
        self.sets = {}
        self.ops = []
        # Which family filled each stub, to name it if a second one tries.
        self.stub_fillers = {}
        # Where each set was declared, for diagnostics.
        self.set_paths = {}

    def resolve(self):
        chain = self._load_chain()
        self._check_names(chain)
        self._check_order(chain)
        leaf = chain[-1]
        for family in chain:
            self._add_family(family)
        self._check_sets()
        for family in chain:
            self._add_ops(family)
        return Family(
            name=leaf.name,
            family_id=self._family_id(leaf),
            path=leaf.path,
            chain=chain,
            owners={self._family_id(item): item.name for item in chain},
            definitions=self.definitions,
            groups=self.groups,
            attrs=self.attrs,
            sets=self.sets,
            ops=tuple(self.ops),
            fills=self._fills(leaf),
            chain_fills=self._fills(None),
            messages=self._messages(leaf),
        )

    def _messages(self, leaf):
        """The message roots this family declares that no op selects.

        The root family alone has them, and has to have them.
        """
        messages = []
        for name in sizes.CORE_MESSAGES:
            attr_set = self.sets.get(name)
            if attr_set is None:
                if leaf.name != spec.CORE_FAMILY_NAME:
                    continue
                raise CtlvError(f'{leaf.path}: family {leaf.name}',
                                f'the root family has to declare the "{name}" '
                                'attr set, which the core builds a message of')
            if attr_set.family_id != self._family_id(leaf):
                continue
            where = f'{self.set_paths[name]}: attr set {name}'
            nodes, stubs = self._expand_placements(
                spec.Placements(attr_set.attrs, attr_set.optional_attrs),
                where, (), 0, (name,))
            if stubs:
                raise CtlvError(where, 'a message the core builds cannot '
                                       'select an extension point')
            root = _root(nodes)
            messages.append(Message(
                name=name,
                family_id=attr_set.family_id,
                set_id=attr_set.set_id,
                root=root,
                max_len=sizes.message_max_len(where, name, root),
                limit=sizes.CORE_MESSAGES[name][0],
            ))
        return tuple(messages)

    def _fills(self, leaf):
        """The stubs leaf fills, with the members it puts in each.

        A leaf of None asks for the fills of the whole chain.
        """
        fills = []
        for name, filler in self.stub_fillers.items():
            if leaf is not None and filler != leaf.name:
                continue
            attr_set = self.sets[name]
            nodes, stubs = self._expand_placements(
                spec.Placements(attr_set.attrs, attr_set.optional_attrs),
                f'{self.set_paths[name]}: attr set {name}', (), 0, (name,))
            fills.append(Fill(
                name=name,
                family_id=attr_set.family_id,
                set_id=attr_set.set_id,
                root=_root(nodes, stubs),
            ))
        return tuple(fills)

    # Loading and identity

    def _load_chain(self):
        leaf = spec.load(self.paths.spec, self.paths.schema)
        if self.paths.spec.stem != leaf.name:
            raise CtlvError(f'{leaf.path}: family {leaf.name}',
                            'a specification must be named after its family, '
                            f'so this file must be {leaf.name}.yaml')
        chain = [leaf]
        seen = [leaf.name]
        self._check_root(leaf)
        while chain[0].inherits:
            parent_name = chain[0].inherits
            child = chain[0]
            if parent_name in seen:
                # seen runs leaf to ancestor, so this reads as X, Y, X.
                raise CtlvError(f'{child.path}: family {child.name}',
                                'inheritance cycle: ' +
                                ' -> '.join(seen + [parent_name]))
            parent_path = self.paths.sibling_spec(parent_name)
            parent = spec.load(parent_path, self.paths.schema)
            if parent.name != parent_name:
                raise CtlvError(f'{parent_path}: family {parent.name}',
                                f'expected family {parent_name}')
            self._check_root(parent)
            seen.append(parent_name)
            chain.insert(0, parent)
        return tuple(chain)

    def _check_root(self, family):
        """Require every family but the root one to name what it inherits."""
        if family.name == spec.CORE_FAMILY_NAME:
            if family.inherits:
                raise CtlvError(f'{family.path}: family {family.name}',
                                'the root family heads every chain, so it '
                                'inherits nothing')
        elif not family.inherits:
            raise CtlvError(f'{family.path}: family {family.name}',
                            'must name the family it inherits, which is the '
                            f'root {spec.CORE_FAMILY_NAME} for a family '
                            'directly below it')

    def _check_names(self, chain):
        """Require every child family name to carry its whole ancestry.

        A dash is the only separator; a dot selects group.attr.
        """
        for parent, child in zip(chain, chain[1:]):
            suffix = child.name.removeprefix(f'{parent.name}-')
            if not suffix or suffix == child.name:
                raise CtlvError(f'{child.path}: family {child.name}',
                                f'a family that inherits {parent.name} must '
                                f'be named {parent.name}-<suffix>')

    def _check_order(self, chain):
        """Require a parent to be registered before what inherits it.

        Ascending positions let the core walk a chain once and index it.
        """
        for parent, child in zip(chain, chain[1:]):
            if self._family_id(parent) >= self._family_id(child):
                raise CtlvError(f'{child.path}: family {child.name}',
                                f'is registered before {parent.name}, the '
                                'family it inherits')

    def _family_id(self, family):
        family_id = self.registry.family_id(family.name)
        if family_id is None:
            raise CtlvError(f'{family.path}: family {family.name}',
                            f'not registered in {self.registry.path}')
        return family_id

    # Declarations

    def _add_family(self, family):
        family_id = self._family_id(family)
        self._add_definitions(family, family_id, family.enums, 'enum')
        self._add_definitions(family, family_id, family.flags, 'flags')
        self._add_groups(family, family_id)
        self._add_sets(family, family_id)

    def _add_definitions(self, family, family_id, definitions, kind):
        for definition in definitions:
            where = f'{family.path}: {kind} {definition.name}'
            self._reject_redeclaration(where, self.definitions,
                                       definition.name, 'enum or flags')
            if kind == 'flags' and len(definition.values) > MAX_FLAG_BITS:
                raise CtlvError(where, f'more than {MAX_FLAG_BITS} flag bits')
            # Enum values from one, zero being reserved; flag bits from zero.
            first = 1 if kind == 'enum' else 0
            values = tuple((name, first + index)
                           for index, name in enumerate(definition.values))
            self.definitions[definition.name] = Definition(
                definition.name, kind, family_id, values)

    def _add_groups(self, family, family_id):
        for group_id, group in enumerate(family.groups, start=1):
            where = f'{family.path}: attr group {group.name}'
            self._reject_redeclaration(where, self.groups, group.name,
                                       'attr group')
            self._reject_redeclaration(where, self.sets, group.name,
                                       'attr set')
            attrs = []
            names = set()
            for local_id, decl in enumerate(group.attrs, start=1):
                attr_where = f'{family.path}: attr {group.name}.{decl.name}'
                if decl.name in names:
                    raise CtlvError(attr_where, 'duplicate attr')
                names.add(decl.name)
                self._check_reference(attr_where, decl)
                attrs.append(Attr(
                    group=group.name,
                    name=decl.name,
                    family_id=family_id,
                    group_id=group_id,
                    local_id=local_id,
                    attr_id=(family_id << 48) | (group_id << 32) | local_id,
                    decl=decl,
                ))
            effective = Group(group.name, family_id, group_id, tuple(attrs))
            self.groups[group.name] = effective
            for attr in attrs:
                self.attrs[attr.full_name] = attr

    def _check_reference(self, where, decl):
        if decl.kind not in ('enum', 'flags'):
            return
        definition = self.definitions.get(decl.ref)
        if definition is None:
            raise CtlvError(where, f'unknown {decl.kind} "{decl.ref}"')
        if definition.kind != decl.kind:
            raise CtlvError(where, f'"{decl.ref}" is a {definition.kind} '
                                   f'definition, not {decl.kind}')

    def _add_sets(self, family, family_id):
        set_id = 0
        for attr_set in family.sets:
            where = f'{family.path}: attr set {attr_set.name}'
            existing = self.sets.get(attr_set.name)
            if existing is not None:
                self._fill_stub(where, family, existing, attr_set)
                continue
            self._reject_redeclaration(where, self.groups, attr_set.name,
                                       'attr group')
            set_id += 1
            self.set_paths[attr_set.name] = family.path
            self.sets[attr_set.name] = AttrSet(
                name=attr_set.name,
                family_id=family_id,
                set_id=set_id,
                stub=attr_set.stub,
                attrs=attr_set.attrs,
                optional_attrs=attr_set.optional_attrs,
            )

    def _fill_stub(self, where, family, existing, filler):
        """Replace an inherited empty stub with a descendant's members."""
        filled_by = self.stub_fillers.get(existing.name)
        if filled_by is not None:
            raise CtlvError(where, f'stub was already filled by family '
                                   f'{filled_by}')
        if not existing.stub:
            raise CtlvError(where, 'attr set is already declared by an '
                                   'inherited family')
        if filler.stub:
            raise CtlvError(where, 'a stub cannot be redeclared; a further '
                                   'extension point needs a new name')
        self.stub_fillers[existing.name] = family.name
        self.set_paths[existing.name] = family.path
        # A fill completes an inherited set, so the IDs are the declarer's.
        self.sets[existing.name] = dataclasses.replace(
            existing, stub=False, attrs=filler.attrs,
            optional_attrs=filler.optional_attrs)

    def _check_sets(self):
        """Expand every set, so one that no op selects is still checked."""
        for name, attr_set in self.sets.items():
            where = f'{self.set_paths[name]}: attr set {name}'
            # Seeding the stack makes a cycle read from the set expanded.
            _nodes, _stubs = self._expand_placements(
                spec.Placements(attr_set.attrs, attr_set.optional_attrs),
                where, (), 0, set_stack=(attr_set.name,),
                opened=(attr_set.name,))

    def _reject_redeclaration(self, where, namespace, name, what):
        if name in namespace:
            raise CtlvError(where, f'{what} "{name}" is already declared')

    # Ops

    def _add_ops(self, family):
        family_id = self._family_id(family)
        for local_id, op in enumerate(family.ops, start=1):
            where = f'{family.path}: op {op.name}'
            if any(existing.name == op.name for existing in self.ops):
                raise CtlvError(where, f'op "{op.name}" is already declared')
            request = None
            if op.request is not None:
                request = self._tree(op.request, where + ' request')
            reply = None
            if op.reply is not None:
                reply = self._tree(op.reply, where + ' reply')
            self._check_context(where, op, request, reply)
            self.ops.append(Op(
                name=op.name,
                type=op.type,
                availability=op.availability,
                pre_post=op.pre_post,
                family_id=family_id,
                local_id=local_id,
                op_id=(family_id << 48) | local_id,
                request=request,
                reply=reply,
                sizes=sizes.compute(where, op, request, reply),
            ))

    def _tree(self, placements, where):
        children, stubs = self._expand_placements(placements, where, (), 0)
        return _root(children, stubs)

    def _expand_placements(self, placements, where, stack, depth,
                           set_stack=(), opened=()):
        """Return the child nodes of one level and the stubs selected there."""
        merged = {}
        expanded, stubs = self._expand(placements.attrs, False, where,
                                       set_stack)
        optional, more = self._expand(placements.optional_attrs, True, where,
                                      set_stack)
        expanded += optional
        stubs += more
        for attr, required in expanded:
            entry = merged.get(attr.attr_id)
            if entry is None:
                merged[attr.attr_id] = [attr, required]
            elif required:
                # A required selection at this level beats an optional one.
                entry[1] = True
        nodes = tuple(self._node(attr, required, where, stack, depth, opened)
                      for attr, required in merged.values())
        return nodes, _merge_stubs(stubs)

    def _expand(self, selectors, force_optional, where, stack):
        expanded = []
        stubs = ()
        for selector in selectors:
            attrs, found = self._expand_one(selector, force_optional, where,
                                            stack)
            expanded += attrs
            stubs += found
        return expanded, stubs

    def _expand_one(self, selector, force_optional, where, stack):
        if '.' in selector:
            attr = self.attrs.get(selector)
            if attr is None:
                raise CtlvError(where, f'unknown attr "{selector}"')
            return [(attr, not force_optional)], ()
        group = self.groups.get(selector)
        if group is not None:
            return [(attr, not force_optional) for attr in group.attrs], ()
        attr_set = self.sets.get(selector)
        if attr_set is None:
            raise CtlvError(where, f'unknown selector "{selector}"')
        if attr_set.stub:
            # Nothing to expand until a descendant fills it.
            return [], (Stub(attr_set.name, attr_set.family_id,
                             attr_set.set_id, force_optional),)
        if selector in stack:
            raise CtlvError(where, 'attr set cycle: ' +
                                   ' -> '.join(stack + (selector,)))
        stack += (selector,)
        # Under an optional selection every member becomes optional.
        expanded, stubs = self._expand(attr_set.attrs, force_optional, where,
                                       stack)
        optional, more = self._expand(attr_set.optional_attrs, True, where,
                                      stack)
        return expanded + optional, stubs + more

    def _node(self, attr, required, where, stack, depth, opened=()):
        decl = attr.decl
        if decl.kind != 'nested':
            return Node(attr, required, decl.max_occurrences, (), ())
        holds = self._nested_set(where, attr)
        if decl.recursive and holds in opened:
            # Open above, so one level further down rather than a new one.
            return Node(attr, required, decl.max_occurrences, (), (),
                        reenters=holds)
        if attr.attr_id in stack:
            names = [self._attr_by_id(i).full_name for i in stack]
            raise CtlvError(where, 'nesting cycle: ' +
                                   ' -> '.join(names + [attr.full_name]))
        if depth + 1 > sizes.MAX_DECLARED_NEST_DEPTH:
            raise CtlvError(where, f'{attr.full_name} nests deeper than the '
                                   f'{sizes.MAX_DECLARED_NEST_DEPTH} level '
                                   'limit')
        children, stubs = self._expand_placements(
            spec.Placements(decl.attrs, decl.optional_attrs), where,
            stack + (attr.attr_id,), depth + 1,
            opened=opened + ((holds,) if holds else ()))
        node = Node(attr, required, decl.max_occurrences,
                    tuple(child for child in children if child.required),
                    tuple(child for child in children if not child.required),
                    stubs, expands=holds)
        if decl.recursive and not _reenters(node, holds):
            raise CtlvError(where, f'{attr.full_name} is recursive, but attr '
                                   f'set "{holds}" never selects it again')
        return node

    def _nested_set(self, where, attr):
        """The one attr set a nest holds, when that is all it holds.

        Only such a nest can be re-entered, a re-entry meaning the members
        open above.
        """
        decl = attr.decl
        if not decl.recursive:
            if decl.optional_attrs or len(decl.attrs) != 1:
                return None
            return decl.attrs[0] if decl.attrs[0] in self.sets else None
        target = decl.attrs[0]
        if target in self.groups:
            raise CtlvError(where, f'{attr.full_name} is recursive, so it '
                                   f'nests an attr set, and "{target}" is an '
                                   'attr group')
        if target not in self.sets:
            raise CtlvError(where, f'unknown selector "{target}"')
        return target

    def _attr_by_id(self, attr_id):
        for attr in self.attrs.values():
            if attr.attr_id == attr_id:
                return attr
        raise AssertionError(f'no attr with ID {attr_id:#x}')

    def _check_context(self, where, op, request, reply):
        if request is not None:
            self._check_subtree(where, request, 'request')
        if reply is None:
            return
        if op.type == 'query':
            self._check_subtree(where, reply, 'query reply')
            self._check_reentry(where, reply)
        elif op.type == 'event':
            self._check_subtree(where, reply, 'event')
        else:
            self._check_subtree(where, reply, 'action reply')

    def _check_reentry(self, where, root):
        """A repeating level cannot also be widened by a descendant."""
        if not _reenters_anywhere(root) or not _stubs_anywhere(root):
            return
        raise CtlvError(f'{where} query reply',
                        're-enters a level and selects an extension point, '
                        'and a descendant cannot widen a level that repeats')

    def _check_subtree(self, where, node, context):
        if node.attr is not None:
            name = node.attr.full_name
            if (node.max_occurrences == spec.UNLIMITED and
                    context != 'query reply'):
                raise CtlvError(f'{where} {context}',
                                f'{name} has unlimited occurrences, which '
                                'only a query reply may contain')
            if node.reenters is not None and context != 'query reply':
                raise CtlvError(f'{where} {context}',
                                f'{name} re-enters "{node.reenters}", so it '
                                'has no maximum length, which only a query '
                                'reply may contain')
            if node.attr.decl.kind == 'blob' and context != 'request':
                raise CtlvError(f'{where} {context}',
                                f'{name} is a blob attr, which only a '
                                'request may contain')
        for child in node.children():
            self._check_subtree(where, child, context)


def _reenters(node, name):
    """Whether anything below node re-enters name."""
    return any(child.reenters == name or _reenters(child, name)
               for child in node.children())


def _reenters_anywhere(node):
    return any(child.reenters is not None or _reenters_anywhere(child)
               for child in node.children())


def _stubs_anywhere(node):
    return bool(node.stubs) or any(_stubs_anywhere(child)
                                   for child in node.children())


def _root(children, stubs=()):
    return Node(None, True, 1,
                tuple(node for node in children if node.required),
                tuple(node for node in children if not node.required),
                stubs)


def _merge_stubs(stubs):
    """One reference per stub selected at a level, required winning."""
    merged = {}
    for stub in stubs:
        previous = merged.get(stub.name)
        if previous is None or not stub.optional:
            merged[stub.name] = stub
    return tuple(merged.values())
