# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the effective schema: generated IDs, inheritance and selectors."""

import pathlib
import tempfile
import unittest

from tests import support

from ctlvgen.lib import cgen
from ctlvgen.lib import dump
from ctlvgen.lib import semantics
from ctlvgen.lib import sizes
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


def deep_spec(nests):
    """A specification whose request nests the given number of levels."""
    lines = ['# SPDX-License-Identifier: GPL-2.0', 'family:',
             '  name: ctlv-deep', f'  inherits: {spec.CORE_FAMILY_NAME}',
             'attr-groups:', '  - name: n', '    attrs:']
    for level in range(1, nests + 1):
        lines += [f'      - name: l{level}', f'        attrs: s{level}']
    lines += ['      - name: leaf', '        type: uint', 'attr-sets:']
    for level in range(1, nests + 1):
        member = f'n.l{level + 1}' if level < nests else 'n.leaf'
        lines += [f'  - name: s{level}', f'    attrs: {member}']
    lines += ['ops:', '  - name: get', '    type: action', '    request:',
              '      attrs: n.l1']
    return '\n'.join(lines) + '\n'


def load_temporary(specs, name):
    """Load one family from specifications written to a temporary tree.

    Either one specification as text or several, keyed by family name. The
    registry lists the root first, as every registry must, then each
    specification in the order it was given.
    """
    if isinstance(specs, str):
        specs = {name: specs}
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root / 'specs').mkdir()
        entries = ['ctlv']
        for family, text in specs.items():
            (root / 'specs' / f'{family}.yaml').write_text(text,
                                                           encoding='utf-8')
            if family != 'ctlv':
                entries.append(family)
        (root / 'families.yaml').write_text(
            'families:\n' + ''.join(f'  - {item}\n' for item in entries),
            encoding='utf-8')
        return semantics.load(spec.Paths(root / 'specs' / f'{name}.yaml',
                                         schema=support.SCHEMA))


class TestFamilyIdentity(unittest.TestCase):
    def test_registry_order_is_the_family_id(self):
        self.assertEqual(1, support.load_family('ctlv-fixture').family_id)
        self.assertEqual(2, support.load_family('ctlv-fixture-child')
                         .family_id)

    def test_the_core_family_owns_id_zero(self):
        self.assertEqual(0, support.load('ctlv').family_id)

    def test_chain(self):
        """Every chain is rooted at the core's own family."""
        child = support.load_family('ctlv-fixture-child')
        self.assertEqual(('ctlv', 'ctlv-fixture', 'ctlv-fixture-child'),
                         tuple(item.name for item in child.chain))
        self.assertEqual(('ctlv', 'ctlv-fixture'),
                         tuple(item.name for item
                               in support.load_family('ctlv-fixture').chain))
        self.assertEqual(('ctlv',),
                         tuple(item.name for item
                               in support.load('ctlv').chain))


class TestGeneratedIds(unittest.TestCase):
    """Declaration order is the wire identity, so it is spelled out here."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def test_group_and_attr_ids_follow_declaration_order(self):
        expected = {
            'scalar.u': 0x0001000100000001,
            'scalar.s': 0x0001000100000002,
            'scalar.delta': 0x0001000100000003,
            'scalar.freq': 0x0001000100000004,
            'scalar.size': 0x0001000100000005,
            'scalar.mode': 0x0001000100000006,
            'scalar.quirks': 0x0001000100000007,
            'scalar.flag': 0x0001000100000008,
            'scalar.text': 0x0001000100000009,
            'scalar.fixed': 0x000100010000000a,
            'scalar.bounded': 0x000100010000000b,
            'count.few': 0x0001000200000001,
            'count.many': 0x0001000200000002,
            'nest.level1': 0x0001000300000001,
            'nest.level2': 0x0001000300000002,
            'nest.level3': 0x0001000300000003,
            'nest.extension': 0x0001000300000004,
            'nest.request': 0x0001000300000005,
            'region.input-va': 0x0001000400000001,
            'region.output-fd': 0x0001000400000002,
            'region.inout-any': 0x0001000400000003,
            'region.pieces': 0x0001000400000004,
        }
        got = {attr.full_name: attr.attr_id
               for group in self.family.local_groups for attr in group.attrs}
        self.assertEqual(expected, got)

    def test_op_ids_follow_declaration_order_across_op_types(self):
        expected = {
            'noop': 0x0001000000000001,
            'scalars-set': 0x0001000000000002,
            'counts-get': 0x0001000000000003,
            'nests-dump': 0x0001000000000004,
            'blobs-submit': 0x0001000000000005,
            'collapse-check': 0x0001000000000006,
            'thing-changed': 0x0001000000000007,
            'pinged': 0x0001000000000008,
        }
        self.assertEqual(expected,
                         {op.name: op.op_id for op in self.family.local_ops})

    def test_attr_set_ids_follow_declaration_order(self):
        expected = {
            'extension-point': 1,
            'scalar-key': 2,
            'scalar-pair': 3,
            'level2-members': 4,
            'level1-members': 5,
            'every-scalar': 6,
            'collapse': 7,
        }
        self.assertEqual(expected,
                         {attr_set.name: attr_set.set_id
                          for attr_set in self.family.local_sets})

    def test_enum_values_start_at_one_because_zero_is_reserved(self):
        mode = self.family.definitions['mode']
        self.assertEqual((('disabled', 1), ('enabled', 2), ('auto', 3)),
                         mode.values)

    def test_flag_bits_start_at_bit_zero(self):
        quirks = self.family.definitions['quirks']
        self.assertEqual((('slow', 0), ('noisy', 1), ('flaky', 2)),
                         quirks.values)

    def test_child_definitions_use_the_child_family_id(self):
        self.assertEqual(0x0002000100000001,
                         self.child.attrs['child.tag'].attr_id)
        self.assertEqual(0x0002000000000001,
                         support.op(self.child, 'child-info-get').op_id)

    def test_inherited_definitions_keep_their_owner_ids(self):
        for name, attr in self.family.attrs.items():
            self.assertEqual(attr.attr_id, self.child.attrs[name].attr_id)
        for op in self.family.ops:
            self.assertEqual(op.op_id,
                             support.op(self.child, op.name).op_id)


class TestStubs(unittest.TestCase):
    def test_an_unfilled_stub_is_an_empty_nest(self):
        family = support.load_family('ctlv-fixture')
        self.assertTrue(family.sets['extension-point'].stub)
        extension = support.placement(
            support.op(family, 'nests-dump').reply, 'nest.extension')
        self.assertEqual((), extension.children())

    def test_a_filled_stub_reaches_every_inherited_use(self):
        child = support.load_family('ctlv-fixture-child')
        extension = support.placement(
            support.op(child, 'nests-dump').reply, 'nest.extension')
        self.assertEqual(('child.tag', 'child.serial'),
                         tuple(node.attr.full_name
                               for node in extension.children()))

    def test_a_stub_fill_does_not_consume_a_child_set_id(self):
        child = support.load_family('ctlv-fixture-child')
        filled = child.sets['extension-point']
        self.assertFalse(filled.stub)
        # The fill keeps the ids of the family that declared the stub.
        self.assertEqual((1, 1), (filled.family_id, filled.set_id))
        # child-key is declared after the fill and is still its first own set.
        own = child.sets['child-key']
        self.assertEqual((2, 1), (own.family_id, own.set_id))

    def test_a_fill_cannot_duplicate_an_effective_member(self):
        parent = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-slot
  inherits: ctlv
attr-sets:
  - name: first-extension
    stub: true
  - name: second-extension
    stub: true
ops:
  - name: set
    type: action
    request:
      attrs:
        - first-extension
        - second-extension
'''
        child = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-slot-child
  inherits: ctlv-slot
attr-groups:
  - name: g
    attrs:
      - name: a
        type: uint
attr-sets:
  - name: first-extension
    attrs: g.a
  - name: second-extension
    attrs: g.a
'''
        with self.assertRaises(CtlvError) as caught:
            load_temporary({'ctlv-slot': parent,
                            'ctlv-slot-child': child},
                           'ctlv-slot-child')
        self.assertIn('attr g.a is selected more than once',
                      str(caught.exception))

    def test_a_stub_fill_cannot_introduce_recursion(self):
        parent = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-recursive-fill
  inherits: ctlv
attr-sets:
  - name: extension
    stub: true
ops:
  - name: get
    type: query
    reply:
      attrs: extension
'''
        child = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-recursive-fill-child
  inherits: ctlv-recursive-fill
attr-groups:
  - name: node
    attrs:
      - name: leaf
        type: uint
      - name: self
        attrs: recursive-node
        recursive: true
        max-occurrences: unlimited
attr-sets:
  - name: recursive-node
    attrs: node.leaf
    optional-attrs: node.self
  - name: extension
    attrs: node.self
'''
        with self.assertRaises(CtlvError) as caught:
            load_temporary({
                'ctlv-recursive-fill': parent,
                'ctlv-recursive-fill-child': child,
            }, 'ctlv-recursive-fill-child')
        self.assertIn('stub fill cannot contain a recursive attr',
                      str(caught.exception))

    def test_a_child_name_does_not_redefine_an_ancestor_helper(self):
        parent = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-foo
  inherits: ctlv
attr-groups:
  - name: bar
    attrs:
      - name: x
        type: uint
ops:
  - name: bar
    type: action
    request:
      attrs: bar.x
'''
        child = '''\
# SPDX-License-Identifier: GPL-2.0
family:
  name: ctlv-foo-bar
  inherits: ctlv-foo
attr-groups:
  - name: x
    attrs:
      - name: x
        type: uint
ops:
  - name: child
    type: action
    request:
      attrs: x.x
'''
        family = load_temporary({'ctlv-foo': parent,
                                 'ctlv-foo-bar': child},
                                'ctlv-foo-bar')
        names = cgen.Names(family)
        symbols = cgen.symbols(family)
        parent_helper = names.group_attr(family.groups['bar'])
        child_helper = names.family_attr(family.groups['x'])

        self.assertEqual('_CTLV_FOO_ATTR_BAR', parent_helper)
        self.assertEqual('_CTLV_FOO_BAR_ATTR', child_helper)
        self.assertIn(parent_helper, symbols)
        self.assertIn(child_helper, symbols)


class TestSelectorExpansion(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')

    def request_of(self, name):
        return support.op(self.family, name).request

    def test_a_group_selector_takes_every_attr_in_order(self):
        request = self.request_of('scalars-set')
        self.assertEqual(
            ('scalar.u', 'scalar.s', 'scalar.delta', 'scalar.freq',
             'scalar.size', 'scalar.mode', 'scalar.quirks', 'scalar.flag',
             'scalar.text', 'scalar.fixed', 'scalar.bounded'),
            tuple(node.attr.full_name for node in request.children()))
        self.assertTrue(all(node.required for node in request.children()))

    def test_required_beats_optional(self):
        request = self.request_of('collapse-check')
        self.assertEqual(('scalar.u', 'scalar.mode'),
                         tuple(node.attr.full_name
                               for node in request.children()))
        self.assertTrue(support.placement(request, 'scalar.u').required)
        self.assertFalse(support.placement(request, 'scalar.mode').required)

    def test_a_required_set_keeps_its_internal_classification(self):
        child = support.load_family('ctlv-fixture-child')
        reply = support.op(child, 'child-info-get').reply
        self.assertEqual(('child.tag',),
                         tuple(node.attr.full_name
                               for node in reply.required_children))
        self.assertEqual(('child.serial',),
                         tuple(node.attr.full_name
                               for node in reply.optional_children))

    def test_an_optional_set_makes_every_member_optional(self):
        child = support.load_family('ctlv-fixture-child')
        extension = support.placement(
            support.op(child, 'nests-dump').reply, 'nest.extension')
        self.assertFalse(extension.required)
        # Only the selection became optional, not a member of the set.
        self.assertTrue(support.placement(extension, 'child.tag').required)

    def test_max_occurrences_comes_from_the_declaration(self):
        request = self.request_of('counts-get')
        self.assertEqual(3, support.placement(request, 'count.few')
                         .max_occurrences)
        reply = support.op(self.family, 'counts-get').reply
        self.assertEqual(spec.UNLIMITED,
                         support.placement(reply, 'count.many')
                         .max_occurrences)


class TestOpAvailability(unittest.TestCase):
    """Whether a family may take an op away from one device."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def test_an_op_is_fixed_unless_it_says_otherwise(self):
        self.assertEqual('fixed',
                         support.op(self.family, 'noop').availability)
        self.assertEqual('fixed',
                         support.op(self.family, 'pinged').availability)

    def test_fd_access_is_inherited(self):
        self.assertEqual('read', support.op(self.family, 'noop').fd_access)
        self.assertEqual('read-write',
                         support.op(self.family, 'scalars-set').fd_access)
        for op in self.family.ops:
            self.assertEqual(op.fd_access,
                             support.op(self.child, op.name).fd_access)

    def test_both_executable_op_types_may_be_dynamic(self):
        self.assertEqual('dynamic',
                         support.op(self.family, 'scalars-set').availability)
        self.assertEqual('dynamic',
                         support.op(self.family, 'counts-get').availability)

    def test_an_inherited_op_keeps_the_availability_it_was_given(self):
        for op in self.family.ops:
            self.assertEqual(op.availability,
                             support.op(self.child, op.name).availability)
        self.assertEqual('dynamic',
                         support.op(self.child, 'child-dump')
                         .availability)

    def test_an_inherited_op_keeps_the_hook_pair_it_asked_for(self):
        """A child answers an inherited op, so it implements its hooks too."""
        self.assertTrue(support.op(self.family, 'counts-get').pre_post)
        for op in self.family.ops:
            self.assertEqual(op.pre_post,
                             support.op(self.child, op.name).pre_post)

    def test_the_root_family_declares_every_op_fixed(self):
        """Its ops are answered for every device, so none is ever absent."""
        for op in support.load(spec.CORE_FAMILY_NAME).ops:
            with self.subTest(op=op.name):
                self.assertEqual('fixed', op.availability)

    def test_the_root_family_is_read_by_the_availability_rules_of_any(self):
        """Its ops are fixed by its own specification, not by a rule."""
        text = (support.SPEC_DIR / 'ctlv.yaml').read_text(encoding='utf-8')
        dynamic = text.replace('  - name: ops-dump\n    type: query\n',
                               '  - name: ops-dump\n    type: query\n'
                               '    availability: dynamic\n')
        family = load_temporary(dynamic, spec.CORE_FAMILY_NAME)
        self.assertEqual('dynamic',
                         support.op(family, 'ops-dump').availability)
        event = text.replace('  - name: op-changed\n    type: event\n',
                             '  - name: op-changed\n    type: event\n'
                             '    availability: dynamic\n')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(event, spec.CORE_FAMILY_NAME)
        self.assertIn('availability is only valid on an action or a query op',
                      str(caught.exception))

    def test_the_root_family_may_declare_an_op_optional(self):
        """And an event still cannot be one, having nothing to opt in to."""
        text = (support.SPEC_DIR / 'ctlv.yaml').read_text(encoding='utf-8')
        optional = text.replace('  - name: ops-dump\n    type: query\n',
                                '  - name: ops-dump\n    type: query\n'
                                '    optional: true\n')
        family = load_temporary(optional, spec.CORE_FAMILY_NAME)
        self.assertTrue(support.op(family, 'ops-dump').optional)
        event = text.replace('  - name: op-changed\n    type: event\n',
                             '  - name: op-changed\n    type: event\n'
                             '    optional: true\n')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(event, spec.CORE_FAMILY_NAME)
        self.assertIn('optional is only valid on an action or a query op',
                      str(caught.exception))

    def test_an_event_cannot_declare_fd_access(self):
        text = (support.SPEC_DIR / 'ctlv.yaml').read_text(encoding='utf-8')
        event = text.replace('  - name: op-changed\n    type: event\n',
                             '  - name: op-changed\n    type: event\n'
                             '    fd-access: read\n')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(event, spec.CORE_FAMILY_NAME)
        self.assertIn('fd-access is only valid on an action or a query op',
                      str(caught.exception))


class TestInheritedNames(unittest.TestCase):
    """A child name has to carry the name of every family above it."""

    def load_chain(self, *names):
        specs = {}
        for index, name in enumerate(names):
            parent = names[index - 1] if index else spec.CORE_FAMILY_NAME
            specs[name] = ('# SPDX-License-Identifier: GPL-2.0\n'
                           f'family:\n  name: {name}\n'
                           f'  inherits: {parent}\n')
        specs[names[0]] += ('attr-groups:\n  - name: g\n    attrs:\n'
                            '      - name: thing\n        type: uint\n'
                            'ops:\n  - name: get\n    type: action\n'
                            '    request:\n      attrs: g.thing\n')
        return load_temporary(specs, names[-1])

    def test_every_ancestor_prefix_is_carried_through_the_chain(self):
        family = self.load_chain('ctlv-mid', 'ctlv-mid-child')
        self.assertEqual(('ctlv', 'ctlv-mid', 'ctlv-mid-child'),
                         tuple(item.name for item in family.chain))
        inherited = support.op(family, 'get')
        self.assertEqual('ctlv-mid', family.owners[inherited.family_id])

    def test_the_root_is_a_prefix_like_any_other(self):
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('elsewhere')
        self.assertIn('a family that inherits ctlv must be named '
                      'ctlv-<suffix>', str(caught.exception))

    def test_a_dot_is_not_an_inheritance_separator(self):
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('ctlv-root', 'ctlv-root.child')
        self.assertIn('does not match', str(caught.exception))

    def test_the_local_suffix_cannot_be_empty(self):
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('ctlv-root', 'ctlv-root-')
        self.assertIn('does not match', str(caught.exception))


class TestNestDepth(unittest.TestCase):
    """One level short of the transport, which op-schema-get spends."""

    def load_deep(self, nests):
        return load_temporary(deep_spec(nests), 'ctlv-deep')

    def test_nesting_up_to_the_limit_is_accepted(self):
        family = self.load_deep(31)
        node = support.op(family, 'get').request
        depth = 0
        while node.children():
            node = node.children()[0]
            depth += 1
        # 31 nests plus the leaf inside the innermost one.
        self.assertEqual(32, depth)

    def test_nesting_past_the_limit_is_rejected(self):
        with self.assertRaises(CtlvError) as caught:
            self.load_deep(32)
        self.assertIn('nests deeper than the 31 level limit',
                      str(caught.exception))

    def test_the_deepest_message_can_still_be_described(self):
        """A description of it is one nest deeper and has to fit as well."""
        self.assertEqual(sizes.MAX_NEST_DEPTH,
                         sizes.MAX_DECLARED_NEST_DEPTH + 1)


class TestRecursion(unittest.TestCase):
    """A node describing a node is where a level re-enters itself."""

    @classmethod
    def setUpClass(cls):
        cls.reply = support.op(support.load('ctlv'), 'op-schema-get').reply

    def node_level(self):
        return support.placement(self.reply, 'schema.request')

    def test_the_level_that_expands_the_set_names_it(self):
        self.assertEqual('schema-node', self.node_level().expands)

    def test_a_re_entry_has_no_children_of_its_own(self):
        for name in ('schema.attrs', 'schema.optional-attrs'):
            with self.subTest(attr=name):
                node = support.placement(self.node_level(), name)
                self.assertEqual('schema-node', node.reenters)
                self.assertEqual((), node.children())

    def test_what_a_re_entry_returns_to_holds_the_re_entry(self):
        """Which is what lets one node describe every level below it."""
        level = self.node_level()
        member = support.placement(level, 'schema.attrs')
        self.assertEqual(level.expands, member.reenters)

    def test_both_trees_of_the_op_are_described_the_same_way(self):
        for name in ('schema.request', 'schema.reply'):
            with self.subTest(tree=name):
                node = support.placement(self.reply, name)
                self.assertEqual('schema-node', node.expands)
                self.assertEqual(self.node_level().children(),
                                 node.children(),
                                 'the same members, node for node')


class TestDump(unittest.TestCase):
    def test_repeated_renders_are_identical(self):
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                first = dump.render(support.load_any(name))
                second = dump.render(support.load_any(name))
                self.assertEqual(first, second)

    def test_every_op_appears(self):
        family = support.load_family('ctlv-fixture-child')
        rendered = dump.render(family)
        for op in family.ops:
            self.assertIn(f'{op.name} = 0x{op.op_id:016x}', rendered)


if __name__ == '__main__':
    unittest.main()
