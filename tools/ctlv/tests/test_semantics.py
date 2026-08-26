# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the effective schema: generated IDs, inheritance and records."""

from __future__ import annotations

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

LICENSE: str = '# SPDX-License-Identifier: GPL-2.0\n'


def deep_spec(depth: int) -> str:
    """A specification whose request is the given number of records deep."""
    lines = [LICENSE.rstrip('\n'), 'family: ctlv-deep',
             f'inherits: {spec.CORE_FAMILY_NAME}', '', 'records:']
    for level in range(1, depth + 1):
        lines.append(f'  s{level}:')
        if level < depth:
            lines.append(f'    l{level + 1}: record s{level + 1}')
        else:
            lines.append('    leaf: uint')
    lines += ['', 'ops:', '  get:', '    type: action', '    request:',
              '      l1: record s1']
    return '\n'.join(lines) + '\n'


def deep_fill_specs(level: int, depth: int) -> dict[str, str]:
    """An extension point at a record level, filled depth records deeper."""
    parent = deep_spec(level - 1).replace('    leaf: uint',
                                          '    ext?: extension deep-extension')
    lines = [LICENSE.rstrip('\n'), 'family: ctlv-deep-child',
             'inherits: ctlv-deep', '', 'records:', '  deep-extension:']
    for below in range(1, depth + 1):
        lines += [f'    f{below}: record t{below}', f'  t{below}:']
    lines.append('    leaf: uint')
    return {'ctlv-deep': parent, 'ctlv-deep-child': '\n'.join(lines) + '\n'}


def documented(text: str) -> str:
    """A specification with a comment above every line after its license.

    Validation wants every declaration documented, which no test here is about.
    """
    lines = text.splitlines(keepends=True)
    out = lines[:1]
    for line in lines[1:]:
        if line.strip():
            indent = line[:len(line) - len(line.lstrip())]
            out.append(f'{indent}# Declared for a test.\n')
        out.append(line)
    return ''.join(out)


def load_temporary(specs: dict[str, str] | str, name: str) -> semantics.Family:
    """Load one family from specifications written to a temporary tree.

    Either one specification as text or several, keyed by family name. The
    registry lists the root first, as every registry must, then each
    specification in the order it was given. All but the root, which is
    documented already, get a comment above every line.
    """
    if isinstance(specs, str):
        specs = {name: specs}
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root / 'specs').mkdir()
        entries = ['ctlv']
        for family, text in specs.items():
            if family != 'ctlv':
                text = documented(text)
                entries.append(family)
            (root / 'specs' / f'{family}.yaml').write_text(text,
                                                           encoding='utf-8')
        (root / 'families.yaml').write_text(
            'families:\n' + ''.join(f'  - {item}\n' for item in entries),
            encoding='utf-8')
        return semantics.load(spec.Paths(root / 'specs' / f'{name}.yaml',
                                         schema=support.SCHEMA))


def names(nodes: tuple[semantics.Node, ...]) -> tuple[str, ...]:
    return tuple(node.child_field.full_name for node in nodes)


class TestFamilyIdentity(unittest.TestCase):
    def test_registry_order_is_the_family_id(self) -> None:
        self.assertEqual(1, support.load_family('ctlv-fixture').family_id)
        self.assertEqual(2, support.load_family('ctlv-fixture-child')
                         .family_id)

    def test_the_core_family_owns_id_zero(self) -> None:
        self.assertEqual(0, support.load('ctlv').family_id)

    def test_chain(self) -> None:
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
    """Declaration order makes the IDs, so it is spelled out here."""

    family: semantics.Family
    child: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def test_record_ids_follow_declaration_order(self) -> None:
        """Type kind, then the index, then the path of unnamed records."""
        expected = {
            'scalar': 0x00010000,
            'u': 0x00020000,
            's': 0x00030000,
            'scalar-pair': 0x00040000,
            'noop request': 0x80010000,
            'scalars-set reply': 0xc0020000,
            'counts-get request': 0x80030000,
            'counts-get reply': 0xc0030000,
            'records-dump request': 0x80040000,
            'records-dump reply': 0xc0040000,
            'records-dump level1': 0xc0041000,
            'records-dump level1.level2': 0xc0041100,
            'blobs-submit request': 0x80050000,
            'optional-check request': 0x80060000,
            'thing-changed payload': 0xc0070000,
            'pinged payload': 0xc0080000,
        }
        self.assertEqual(expected,
                         {record.path: record.record_id
                          for record in self.family.local_records})

    def test_field_types_follow_declaration_order(self) -> None:
        expected = {
            'u': 0x0001000200000001,
            's': 0x0001000300000001,
            'scalar.delta': 0x0001000100000001,
            'scalar.freq': 0x0001000100000002,
            'scalar.size': 0x0001000100000003,
            'scalar.mode': 0x0001000100000004,
            'scalar.quirks': 0x0001000100000005,
            'scalar.flag': 0x0001000100000006,
            'scalar.text': 0x0001000100000007,
            'scalar.fixed': 0x0001000100000008,
            'scalar.bounded': 0x0001000100000009,
            'counts-get few': 0x0001800300000001,
            'counts-get mode': 0x0001800300000002,
            'counts-get request': 0x0001800300000003,
            'counts-get algo': 0x0001800300000004,
            'counts-get per-mode': 0x0001800300000005,
            'counts-get algos': 0x0001800300000006,
            'counts-get many': 0x0001c00300000001,
            'records-dump level1': 0x0001c00400000001,
            'records-dump extension': 0x0001c00400000002,
            'records-dump level1.level2': 0x0001c00410000001,
            'records-dump level1.level2.level3': 0x0001c00411000001,
            'blobs-submit input-va': 0x0001800500000001,
            'blobs-submit output-fd': 0x0001800500000002,
            'blobs-submit inout-any': 0x0001800500000003,
            'blobs-submit pieces': 0x0001800500000004,
            'optional-check mode': 0x0001800600000001,
            'optional-check variant': 0x0001800600000002,
        }
        self.assertEqual(expected,
                         {field.full_name: field.type
                          for field in self.family.local_fields})

    def test_a_placed_field_keeps_the_type_of_its_declaration(self) -> None:
        scalars = support.op(self.family, 'scalars-set')
        level1 = support.child_of(
            support.reply(self.family, 'records-dump'),
            'records-dump level1')
        optional = support.request(self.family, 'optional-check')
        for node in (scalars.request, scalars.reply, level1, optional):
            assert node is not None
            self.assertEqual(self.family.fields['u'].type,
                             support.child_of(node, 'u').child_field.type)

    def test_op_ids_follow_declaration_order_across_op_types(self) -> None:
        expected = {
            'noop': 0x0001000000000001,
            'scalars-set': 0x0001000000000002,
            'counts-get': 0x0001000000000003,
            'records-dump': 0x0001000000000004,
            'blobs-submit': 0x0001000000000005,
            'optional-check': 0x0001000000000006,
            'thing-changed': 0x0001000000000007,
            'pinged': 0x0001000000000008,
        }
        self.assertEqual(expected,
                         {op.name: op.op_id for op in self.family.local_ops})

    def test_enum_values_start_at_one_because_zero_is_reserved(self) -> None:
        mode = self.family.definitions['mode']
        self.assertEqual((('disabled', 1), ('enabled', 2), ('auto', 3)),
                         mode.values)

    def test_a_header_enum_is_numbered_as_c_numbers_it(self) -> None:
        algo = self.family.definitions['hash-algo']
        assert algo.header is not None
        self.assertEqual('linux/hash_info.h', algo.header.path)
        self.assertEqual((('HASH_ALGO_MD4', 0), ('HASH_ALGO_MD5', 1),
                          ('HASH_ALGO_SHA1', 2)), algo.values[:3])

    def test_flag_bits_start_at_bit_zero(self) -> None:
        quirks = self.family.definitions['quirks']
        self.assertEqual((('slow', 0), ('noisy', 1), ('flaky', 2)),
                         quirks.values)

    def test_child_definitions_use_the_child_family_id(self) -> None:
        self.assertEqual(0x0002000200000001,
                         self.child.fields['extension-point.tag'].type)
        self.assertEqual(0x0002000000000001,
                         support.op(self.child, 'child-info-get').op_id)

    def test_inherited_definitions_keep_their_owner_ids(self) -> None:
        for name, field in self.family.fields.items():
            self.assertEqual(field.type, self.child.fields[name].type)
        for op in self.family.ops:
            self.assertEqual(op.op_id,
                             support.op(self.child, op.name).op_id)


class TestExtensions(unittest.TestCase):
    def extension_of(self, family: semantics.Family) -> semantics.Node:
        return support.child_of(support.reply(family, 'records-dump'),
                                'records-dump extension')

    def test_an_unfilled_extension_is_an_empty_record(self) -> None:
        family = support.load_family('ctlv-fixture')
        extension = self.extension_of(family)
        self.assertEqual((family.extensions['extension-point'],),
                         extension.extensions)
        self.assertEqual((), extension.children())
        self.assertIsNone(family.fill_of(family.extensions['extension-point']))
        self.assertEqual([], semantics.composed_members(family, extension))

    def test_a_filled_extension_reaches_every_inherited_use(self) -> None:
        child = support.load_family('ctlv-fixture-child')
        members = semantics.composed_members(child, self.extension_of(child))
        self.assertEqual([('extension-point.tag', False), ('serial', True)],
                         [(node.child_field.full_name, optional)
                          for node, optional in members])

    def test_a_fill_is_a_record_of_the_family_that_fills(self) -> None:
        child = support.load_family('ctlv-fixture-child')
        [fill] = child.fills
        # The extension point keeps the IDs of the family declaring it.
        self.assertEqual((1, 1, 2), (fill.family_id, fill.ext_id,
                                     fill.owner_family_id))
        # The fill is the child's second record, after serial.
        self.assertEqual(0x00020000, fill.record.record_id)
        self.assertEqual(0x00010000, child.records['serial'].record_id)

    def test_two_fills_may_place_one_field(self) -> None:
        """Each extension point is a record of its own."""
        parent = LICENSE + '''\
family: ctlv-slot
inherits: ctlv

ops:
  set:
    type: action
    request:
      first?: extension first-extension
      second?: extension second-extension
'''
        child = LICENSE + '''\
family: ctlv-slot-child
inherits: ctlv-slot

records:
  a: uint

  first-extension:
    a: record

  second-extension:
    a: record
'''
        family = load_temporary({'ctlv-slot': parent,
                                 'ctlv-slot-child': child},
                                'ctlv-slot-child')
        request = support.request(family, 'set')
        for name in ('set first', 'set second'):
            with self.subTest(extension=name):
                members = semantics.composed_members(
                    family, support.child_of(request, name))
                self.assertEqual([family.fields['a']],
                                 [node.field for node, _ in members])

    def test_a_child_name_may_not_spell_an_ancestor_helper(self) -> None:
        """A helper has no separator after the family, unlike a constant."""
        parent = LICENSE + '''\
family: ctlv-foo
inherits: ctlv

ops:
  bar:
    type: action
    request:
      x: uint
'''
        child = LICENSE + '''\
family: ctlv-foo-bar
inherits: ctlv-foo

records:
  x: uint

ops:
  child:
    type: action
    request:
      x: record
'''
        family = load_temporary({'ctlv-foo': parent,
                                 'ctlv-foo-bar': child},
                                'ctlv-foo-bar')
        constants = cgen.Names(family)
        self.assertEqual('CTLV_FOO_T_BAR_X',
                         constants.field(family.fields['bar x']))
        self.assertEqual('CTLV_FOO_BAR_T_X',
                         constants.field(family.fields['x']))
        with self.assertRaises(CtlvError) as caught:
            cgen.symbols(family)
        self.assertIn('generated name ctlv_foo_bar_x_ref is already used by '
                      'field bar x', str(caught.exception))


class TestEnumExtensions(unittest.TestCase):
    def test_an_enum_extension_point_has_no_values(self) -> None:
        family = support.load_family('ctlv-fixture')
        variant = family.definitions['variant']
        self.assertTrue(variant.is_extension_point)
        self.assertEqual((), variant.values)
        self.assertEqual(0, variant.max_value)
        # Numbered apart from the record extension point, also number 1.
        self.assertEqual(semantics.EnumExtension('variant', 1, 1),
                         variant.extension)
        self.assertEqual(1, family.extensions['extension-point'].ext_id)
        self.assertEqual((), family.enum_fills)

    def test_a_fill_gives_values_of_the_family_that_fills(self) -> None:
        child = support.load_family('ctlv-fixture-child')
        variant = child.definitions['variant']
        self.assertTrue(variant.is_fill)
        self.assertEqual(2, variant.family_id)
        self.assertEqual((('plain', 1), ('fancy', 2)), variant.values)
        self.assertEqual(semantics.EnumExtension('variant', 1, 1),
                         variant.extension)
        self.assertEqual((variant,), child.enum_fills)
        # An inherited field takes it by name, as it took the point.
        self.assertEqual('variant',
                         child.fields['optional-check variant'].decl.ref)

    def test_a_grandchild_may_fill_what_its_parent_left(self) -> None:
        parent = LICENSE + '''\
family: ctlv-kind
inherits: ctlv

enums:
  kind: extension

ops:
  set:
    type: action
    request:
      kind?: kind
'''
        child = LICENSE + '''\
family: ctlv-kind-child
inherits: ctlv-kind

ops:
  child-noop:
    type: action
'''
        grandchild = LICENSE + '''\
family: ctlv-kind-child-leaf
inherits: ctlv-kind-child

enums:
  kind:
    - only
'''
        specs = {'ctlv-kind': parent, 'ctlv-kind-child': child,
                 'ctlv-kind-child-leaf': grandchild}
        middle = load_temporary(specs, 'ctlv-kind-child')
        self.assertTrue(middle.definitions['kind'].is_extension_point)
        leaf = load_temporary(specs, 'ctlv-kind-child-leaf')
        kind = leaf.definitions['kind']
        self.assertEqual((3, (('only', 1),)), (kind.family_id, kind.values))
        self.assertEqual(semantics.EnumExtension('kind', 1, 1),
                         kind.extension)


class TestComposition(unittest.TestCase):
    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')

    def request_of(self, name: str) -> semantics.Node:
        return support.request(self.family, name)

    def test_a_named_record_keeps_its_members_in_order(self) -> None:
        request = self.request_of('scalars-set')
        self.assertEqual(
            ('u', 's', 'scalar.delta', 'scalar.freq', 'scalar.size',
             'scalar.mode', 'scalar.quirks', 'scalar.flag', 'scalar.text',
             'scalar.fixed', 'scalar.bounded'),
            names(request.children()))
        self.assertTrue(all(node.required for node in request.children()))

    def test_a_question_mark_makes_a_field_optional(self) -> None:
        request = self.request_of('optional-check')
        self.assertEqual(('u', 'optional-check mode',
                          'optional-check variant'),
                         names(request.children()))
        self.assertTrue(support.child_of(request, 'u').required)
        self.assertFalse(support.child_of(request,
                                          'optional-check mode').required)

    def test_a_named_record_keeps_its_own_classification(self) -> None:
        child = support.load_family('ctlv-fixture-child')
        reply = support.reply(child, 'child-info-get')
        self.assertEqual(('extension-point.tag',),
                         names(reply.required_children))
        self.assertEqual(('serial',), names(reply.optional_children))

    def test_an_optional_extension_keeps_its_members_required(self) -> None:
        child = support.load_family('ctlv-fixture-child')
        extension = support.child_of(support.reply(child, 'records-dump'),
                                     'records-dump extension')
        self.assertFalse(extension.required)
        members = dict((node.child_field.full_name, optional)
                       for node, optional
                       in semantics.composed_members(child, extension))
        self.assertFalse(members['extension-point.tag'])

    def test_an_optional_include_makes_every_member_optional(self) -> None:
        text = LICENSE + '''\
family: ctlv-include
inherits: ctlv

records:
  pair:
    a: uint
    b?: uint

ops:
  set:
    type: action
    request:
      $include: pair
  get:
    type: action
    reply:
      $include: pair?
'''
        family = load_temporary(text, 'ctlv-include')
        request = support.request(family, 'set')
        reply = support.reply(family, 'get')
        self.assertEqual((('pair.a',), ('pair.b',)),
                         (names(request.required_children),
                          names(request.optional_children)))
        self.assertEqual(((), ('pair.a', 'pair.b')),
                         (names(reply.required_children),
                          names(reply.optional_children)))
        self.assertEqual(names(request.children()), names(reply.children()))

    def test_max_occurrences_comes_from_the_declaration(self) -> None:
        request = self.request_of('counts-get')
        self.assertEqual(3, support.child_of(request, 'counts-get few')
                         .max_occurrences)
        reply = support.reply(self.family, 'counts-get')
        self.assertEqual(spec.UNLIMITED,
                         support.child_of(reply, 'counts-get many')
                         .max_occurrences)

    def test_a_count_naming_an_enum_is_one_per_value(self) -> None:
        request = self.request_of('counts-get')
        self.assertEqual(3, support.child_of(request, 'counts-get per-mode')
                         .max_occurrences)
        algos = support.child_of(request, 'counts-get algos')
        self.assertEqual(len(self.family.definitions['hash-algo'].values),
                         algos.max_occurrences)
        self.assertEqual('hash-algo', algos.child_field.decl.count_enum)

    def test_a_field_of_records_may_take_an_enum_or_flags(self) -> None:
        """Only a record type names a record the field holds."""
        text = LICENSE + '''\
family: ctlv-shade
inherits: ctlv

enums:
  color:
    - red

flags:
  caps:
    - wide

records:
  shade: color
  mask: caps

ops:
  set:
    type: action
    request:
      shade?: record
      mask?: record
'''
        family = load_temporary(text, 'ctlv-shade')
        self.assertEqual([('shade', 'enum'), ('mask', 'flags')],
                         [(node.child_field.full_name,
                           node.child_field.decl.kind)
                          for node in support.request(family,
                                                      'set').children()])


SHARED_BLOB: str = LICENSE + '''\
family: ctlv-shared
inherits: ctlv

records:
  thing-values:
    id: uint

  thing-item:
    $include: thing-values
    data?: blob(in, ..4K, va)

ops:
  set:
    type: action
    request:
      item: record thing-item
  get:
    type: action
    reply:
      item:
        $include: thing-values
'''


class TestReplyBlob(unittest.TestCase):
    """A reply never holds a blob, so it composes a record without one."""

    def test_a_reply_shares_the_fields_without_the_blob(self) -> None:
        family = load_temporary(SHARED_BLOB, 'ctlv-shared')
        request = support.child_of(support.request(family, 'set'),
                                   'set item')
        reply = support.child_of(support.reply(family, 'get'),
                                 'get item')
        self.assertEqual(('thing-values.id', 'thing-item.data'),
                         names(request.children()))
        self.assertEqual(('thing-values.id',), names(reply.children()))
        self.assertEqual(request.children()[0].child_field.type,
                         reply.children()[0].child_field.type)

    def test_a_reply_refuses_a_blob_even_an_optional_one(self) -> None:
        text = SHARED_BLOB.replace('      item:\n        $include: '
                                   'thing-values\n',
                                   '      item: record thing-item\n')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(text, 'ctlv-shared')
        self.assertIn('thing-item.data is a blob, which only a request may '
                      'contain', str(caught.exception))


SLOTS: str = LICENSE + '''\
family: ctlv-slot
inherits: ctlv

ops:
  set:
    type: action
    request:
      in?: extension in-extension
    reply:
      out?: extension out-extension
  list:
    type: query
    reply:
      listed?: extension listed-extension
'''


class TestFillContext(unittest.TestCase):
    """A fill is held to the message its extension point is in."""

    def load_fill(self, record: str, member: str) -> semantics.Family:
        child = LICENSE + f'''\
family: ctlv-slot-child
inherits: ctlv-slot

records:
  {record}:
    {member}
'''
        return load_temporary({'ctlv-slot': SLOTS,
                               'ctlv-slot-child': child}, 'ctlv-slot-child')

    def test_only_a_query_reply_fill_may_be_unlimited(self) -> None:
        self.load_fill('listed-extension', 'items[]: uint')
        with self.assertRaises(CtlvError) as caught:
            self.load_fill('out-extension', 'items[]: uint')
        self.assertIn('op set action reply: out-extension.items has no count '
                      'limit', str(caught.exception))

    def test_only_a_request_fill_may_hold_a_blob(self) -> None:
        self.load_fill('in-extension', 'data?: blob(in, ..4K, va)')
        with self.assertRaises(CtlvError) as caught:
            self.load_fill('out-extension', 'data?: blob(in, ..4K, va)')
        self.assertIn('out-extension.data is a blob, which only a request '
                      'may contain', str(caught.exception))


class TestCoreMessages(unittest.TestCase):
    """A message the core builds is held to the rules of an action reply."""

    def root_with_error_info(self, line: str) -> str:
        """The root specification with one member added to error-info."""
        text = (support.SPEC_DIR / 'ctlv.yaml').read_text(encoding='utf-8')
        head = '  error-info:\n'
        self.assertIn(head, text)
        at = text.index(head) + len(head)
        member = f'    # Declared for a test.\n    {line}\n'
        return text[:at] + member + text[at:]

    def test_an_unlimited_member_is_refused_before_sizing(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            load_temporary(self.root_with_error_info('notes[]?: uint'),
                           spec.CORE_FAMILY_NAME)
        self.assertIn('error-info.notes has no count limit',
                      str(caught.exception))

    def test_a_blob_is_refused(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            load_temporary(
                self.root_with_error_info('data?: blob(in, ..4K, va)'),
                spec.CORE_FAMILY_NAME)
        self.assertIn('error-info.data is a blob', str(caught.exception))


class TestFieldUses(unittest.TestCase):
    """Which family's header carries a field's kernel helpers."""

    def test_a_core_message_is_written_by_the_root_family(self) -> None:
        root = support.load('ctlv')
        written = set()

        def walk(node: semantics.Node) -> None:
            for child in node.children():
                written.add(child.child_field.type)
                walk(child)

        for message in root.messages:
            walk(message.root)
        self.assertTrue(written)
        self.assertLessEqual(written, set(semantics.own_uses(root)[1]))
        child = support.load('ctlv-attestation')
        self.assertFalse(written & set(semantics.own_uses(child)[1]))

    def test_a_fill_is_used_by_the_family_that_fills(self) -> None:
        parent = support.load_family('ctlv-fixture')
        child = support.load_family('ctlv-fixture-child')
        tag = child.fields['extension-point.tag'].type
        self.assertNotIn(tag, semantics.field_uses(parent)[1])
        self.assertIn(tag, semantics.own_uses(child)[1])


class TestOpAvailability(unittest.TestCase):
    """Whether a family may take an op away from one device."""

    family: semantics.Family
    child: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def root_with(self, op: str, line: str) -> str:
        """The root specification with one line added to one op."""
        text = (support.SPEC_DIR / 'ctlv.yaml').read_text(encoding='utf-8')
        head = f'  {op}:\n    type: '
        self.assertIn(head, text)
        at = text.index('\n', text.index(head) + len(head)) + 1
        return text[:at] + f'    {line}\n' + text[at:]

    def test_an_op_is_fixed_unless_it_says_otherwise(self) -> None:
        self.assertEqual('fixed',
                         support.op(self.family, 'noop').availability)
        self.assertEqual('fixed',
                         support.op(self.family, 'pinged').availability)

    def test_fd_access_is_inherited(self) -> None:
        self.assertEqual('read', support.op(self.family, 'noop').fd_access)
        self.assertEqual('read-write',
                         support.op(self.family, 'scalars-set').fd_access)
        for op in self.family.ops:
            self.assertEqual(op.fd_access,
                             support.op(self.child, op.name).fd_access)

    def test_both_executable_op_types_may_be_dynamic(self) -> None:
        self.assertEqual('dynamic',
                         support.op(self.family, 'scalars-set').availability)
        self.assertEqual('dynamic',
                         support.op(self.family, 'counts-get').availability)

    def test_an_inherited_op_keeps_the_availability_it_was_given(self) -> None:
        for op in self.family.ops:
            self.assertEqual(op.availability,
                             support.op(self.child, op.name).availability)
        self.assertEqual('dynamic',
                         support.op(self.child, 'child-dump')
                         .availability)

    def test_an_inherited_op_keeps_the_hook_pair_it_asked_for(self) -> None:
        """A child answers an inherited op, so it implements its hooks too."""
        self.assertTrue(support.op(self.family, 'counts-get').pre_post)
        for op in self.family.ops:
            self.assertEqual(op.pre_post,
                             support.op(self.child, op.name).pre_post)

    def test_the_root_family_declares_every_op_fixed(self) -> None:
        """Its ops are answered for every device, so none is ever absent."""
        for op in support.load(spec.CORE_FAMILY_NAME).ops:
            with self.subTest(op=op.name):
                self.assertEqual('fixed', op.availability)

    def test_the_root_family_is_read_by_the_availability_rules_of_any(
            self) -> None:
        """Its ops are fixed by its own specification, not by a rule."""
        family = load_temporary(
            self.root_with('op-get', 'availability: dynamic'),
            spec.CORE_FAMILY_NAME)
        self.assertEqual('dynamic',
                         support.op(family, 'op-get').availability)
        with self.assertRaises(CtlvError) as caught:
            load_temporary(self.root_with('op-changed',
                                          'availability: dynamic'),
                           spec.CORE_FAMILY_NAME)
        self.assertIn('availability is only valid on an action or a query op',
                      str(caught.exception))

    def test_the_root_family_may_declare_an_op_optional(self) -> None:
        """And an event still cannot be one, having nothing to opt in to."""
        family = load_temporary(self.root_with('op-get', 'optional: true'),
                                spec.CORE_FAMILY_NAME)
        self.assertTrue(support.op(family, 'op-get').optional)
        with self.assertRaises(CtlvError) as caught:
            load_temporary(self.root_with('op-changed', 'optional: true'),
                           spec.CORE_FAMILY_NAME)
        self.assertIn('optional is only valid on an action or a query op',
                      str(caught.exception))

    def test_an_event_cannot_declare_fd_access(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            load_temporary(self.root_with('op-changed', 'fd-access: read'),
                           spec.CORE_FAMILY_NAME)
        self.assertIn('fd-access is only valid on an action or a query op',
                      str(caught.exception))


class TestInheritedNames(unittest.TestCase):
    """A child name has to carry the name of every family above it."""

    def load_chain(self, *names: str) -> semantics.Family:
        specs = {}
        for index, name in enumerate(names):
            parent = names[index - 1] if index else spec.CORE_FAMILY_NAME
            specs[name] = LICENSE + f'family: {name}\ninherits: {parent}\n'
        specs[names[0]] += ('\nops:\n  get:\n    type: action\n'
                            '    request:\n      thing: uint\n')
        return load_temporary(specs, names[-1])

    def test_every_ancestor_prefix_is_carried_through_the_chain(self) -> None:
        family = self.load_chain('ctlv-mid', 'ctlv-mid-child')
        self.assertEqual(('ctlv', 'ctlv-mid', 'ctlv-mid-child'),
                         tuple(item.name for item in family.chain))
        inherited = support.op(family, 'get')
        self.assertEqual('ctlv-mid', family.owners[inherited.family_id])

    def test_the_root_is_a_prefix_like_any_other(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('elsewhere')
        self.assertIn('a family that inherits ctlv must be named '
                      'ctlv-<suffix>', str(caught.exception))

    def test_a_dot_is_not_an_inheritance_separator(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('ctlv-root', 'ctlv-root.child')
        self.assertIn('does not match', str(caught.exception))

    def test_the_local_suffix_cannot_be_empty(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load_chain('ctlv-root', 'ctlv-root-')
        self.assertIn('does not match', str(caught.exception))


class TestRecordDepth(unittest.TestCase):
    """One level short of the transport, which op-schema-get spends."""

    def load_deep(self, depth: int) -> semantics.Family:
        return load_temporary(deep_spec(depth), 'ctlv-deep')

    def test_records_up_to_the_depth_limit_are_accepted(self) -> None:
        family = self.load_deep(7)
        node = support.request(family, 'get')
        depth = 0
        while node.children():
            node = node.children()[0]
            depth += 1
        # 7 records plus the leaf inside the innermost one.
        self.assertEqual(8, depth)

    def test_records_past_the_depth_limit_are_rejected(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load_deep(8)
        self.assertIn('goes deeper than the 7 level limit',
                      str(caught.exception))

    def test_an_extension_point_is_a_record_level(self) -> None:
        load_temporary(deep_fill_specs(7, 0), 'ctlv-deep')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(deep_fill_specs(8, 0), 'ctlv-deep')
        self.assertIn('s7.ext goes deeper than the 7 level limit',
                      str(caught.exception))

    def test_a_fill_counts_from_its_extension_point(self) -> None:
        """The kernel walks a fill at the level of the point it fills."""
        load_temporary(deep_fill_specs(6, 1), 'ctlv-deep-child')
        with self.assertRaises(CtlvError) as caught:
            load_temporary(deep_fill_specs(6, 2), 'ctlv-deep-child')
        self.assertIn('t1.f2 goes deeper than the 7 level limit',
                      str(caught.exception))

    def test_the_deepest_message_can_still_be_described(self) -> None:
        """A description of it is one record deeper and has to fit as well."""
        self.assertEqual(sizes.MAX_RECORD_DEPTH,
                         sizes.MAX_DECLARED_RECORD_DEPTH + 1)


class TestRecursion(unittest.TestCase):
    """A node describing a node is where a level re-enters itself."""

    reply: semantics.Node

    @classmethod
    def setUpClass(cls) -> None:
        cls.reply = support.reply(support.load('ctlv'), 'op-schema-get')

    def node_level(self) -> semantics.Node:
        return support.child_of(self.reply, 'op-schema-get request')

    def test_the_level_that_expands_the_record_names_it(self) -> None:
        self.assertEqual('schema-node', self.node_level().expands)

    def test_a_re_entry_has_no_children_of_its_own(self) -> None:
        for name in ('schema-node.fields', 'schema-node.optional-fields'):
            with self.subTest(field=name):
                node = support.child_of(self.node_level(), name)
                self.assertEqual('schema-node', node.reenters)
                self.assertEqual((), node.children())

    def test_what_a_re_entry_returns_to_holds_the_re_entry(self) -> None:
        """Which is what lets one node describe every level below it."""
        level = self.node_level()
        member = support.child_of(level, 'schema-node.fields')
        self.assertEqual(level.expands, member.reenters)

    def test_both_trees_of_the_op_are_described_the_same_way(self) -> None:
        for name in ('op-schema-get request', 'op-schema-get reply'):
            with self.subTest(tree=name):
                node = support.child_of(self.reply, name)
                self.assertEqual('schema-node', node.expands)
                self.assertEqual(self.node_level().children(),
                                 node.children(),
                                 'the same members, node for node')


class TestDump(unittest.TestCase):
    def test_repeated_renders_are_identical(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                first = dump.render(support.load_any(name))
                second = dump.render(support.load_any(name))
                self.assertEqual(first, second)

    def test_every_op_appears(self) -> None:
        family = support.load_family('ctlv-fixture-child')
        rendered = dump.render(family)
        for op in family.ops:
            self.assertIn(f'{op.name} = 0x{op.op_id:016x}', rendered)


if __name__ == '__main__':
    unittest.main()
