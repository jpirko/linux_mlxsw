# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the emitted C and docs of every specification in the tree.

The text itself: reproducible, inside the kernel's line limit, naming only the
family's own declarations, with the preamble ctlv-regen.sh needs. What the
numbers mean is KUnit's, and where they come from is test_sizes.py's.
"""

from __future__ import annotations

import dataclasses
import os
import pathlib
import re
import subprocess
import sys
import tempfile
import unittest
from collections.abc import Callable, Iterator

from tests import support

from ctlvgen.lib import cgen
from ctlvgen.lib import gen_doc
from ctlvgen.lib import gen_family
from ctlvgen.lib import gen_kernel
from ctlvgen.lib import gen_uapi
from ctlvgen.lib import gen_user
from ctlvgen.lib import semantics
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


def kernel_header(family: semantics.Family) -> str:
    return gen_kernel.render_header(
        family, family_api=family.name in support.FAMILIES)


def kernel_source(family: semantics.Family) -> str:
    return gen_kernel.render_source(
        family, family_api=family.name in support.FAMILIES)


def schema_header(family: semantics.Family) -> str:
    return gen_kernel.render_header(family, family_api=False)


def schema_source(family: semantics.Family) -> str:
    return gen_kernel.render_source(family, family_api=False)


def family_header(family: semantics.Family) -> str:
    return gen_kernel.render_header(family, family_api=True)


def family_source(family: semantics.Family) -> str:
    return gen_kernel.render_source(family, family_api=True)


# What each mode and kind emits, and where ctlv-regen.sh writes it.
FLAVOURS: tuple[Flavour, ...] = (
    ('uapi', 'header', gen_uapi.render),
    ('kernel', 'header', kernel_header),
    ('kernel', 'source', kernel_source),
    ('user', 'header', gen_user.render_header),
    ('user', 'source', gen_user.render_source),
)

# The marker ctlv-regen.sh recovers the parameters from, and the comment
# above it naming the specification.
MARKER: re.Pattern[str] = re.compile(r'^/\* CTLV-GEN (uapi|kernel|user) '
                                     r'(header|source) \*/$', re.M)
SPEC_LINE: re.Pattern[str] = re.compile(
    r'^ \* Generated from (\S+\.yaml) by$', re.M)

GUARD_OPEN: re.Pattern[str] = re.compile(r'^#ifndef (\S+)\n#define \1$', re.M)


Flavour = tuple[str, str, Callable[[semantics.Family], str]]


def flavours(name: str) -> tuple[Flavour, ...]:
    """What one family emits: for the root, all but a userspace header."""
    return tuple(flavour for flavour in FLAVOURS
                 if name != spec.CORE_FAMILY_NAME or
                 flavour[:2] != ('user', 'header'))


def flat(text: str) -> str:
    """One line, so a test says what it wants and not how it was broken."""
    return re.sub(r'\s+', ' ', text).replace('* ', '*').replace('( ', '(')


def emitted() -> Iterator[tuple[str, ...]]:
    """Every family in the tree, in every flavour, as text."""
    for name in support.ALL_FAMILIES:
        family = support.load_any(name)
        for mode, kind, render in flavours(name):
            yield name, mode, kind, render(family)


class TestText(unittest.TestCase):
    """Rules that hold for every generated file."""

    files: tuple[tuple[str, ...], ...]

    @classmethod
    def setUpClass(cls) -> None:
        cls.files = tuple(emitted())

    def test_the_tree_has_something_to_emit(self) -> None:
        self.assertEqual(sum(len(flavours(name))
                             for name in support.ALL_FAMILIES),
                         len(self.files))

    def test_a_file_ends_with_exactly_one_newline(self) -> None:
        for name, mode, kind, text in self.files:
            with self.subTest(family=name, mode=mode, kind=kind):
                self.assertTrue(text.endswith('\n'))
                self.assertFalse(text.endswith('\n\n'))

    def test_no_line_is_longer_than_the_kernel_allows(self) -> None:
        for name, mode, kind, text in self.files:
            for number, line in enumerate(text.splitlines(), 1):
                # An SPDX comment is one token, and a dual-licensed UAPI
                # header's is 82 columns wide.
                if 'SPDX-License-Identifier' in line:
                    continue
                with self.subTest(family=name, mode=mode, kind=kind,
                                  line=number):
                    self.assertLessEqual(cgen.width(line),
                                         cgen.HARD_LINE_LIMIT,
                                         line)

    def test_no_line_has_trailing_whitespace(self) -> None:
        for name, mode, kind, text in self.files:
            for number, line in enumerate(text.splitlines(), 1):
                with self.subTest(family=name, mode=mode, kind=kind,
                                  line=number):
                    self.assertEqual(line.rstrip(), line)

    def test_the_license_is_the_specifications_own(self) -> None:
        for name in support.ALL_FAMILIES:
            family = support.load_any(name)
            for mode, kind, render in flavours(name):
                with self.subTest(family=name, mode=mode, kind=kind):
                    first = render(family).splitlines()[0]
                    comment = '//' if kind == 'source' else '/*'
                    self.assertTrue(first.startswith(comment), first)
                    self.assertIn(f'SPDX-License-Identifier: {family.license}',
                                  first)

    def test_the_preamble_says_what_regen_needs(self) -> None:
        for name, mode, kind, text in self.files:
            with self.subTest(family=name, mode=mode, kind=kind):
                marker = MARKER.search(text)
                assert marker is not None
                self.assertEqual((mode, kind), marker.groups())
                spec_line = SPEC_LINE.search(text)
                assert spec_line is not None
                path = support.KERNEL_DIR / spec_line.group(1)
                self.assertTrue(path.exists(), spec_line.group(1))

    def test_a_header_guards_itself(self) -> None:
        for name, mode, kind, text in self.files:
            if kind != 'header':
                continue
            with self.subTest(family=name, mode=mode):
                guard = GUARD_OPEN.search(text)
                assert guard is not None
                self.assertIn(f'#endif /* {guard.group(1)} */', text)
                self.assertEqual(1, text.count(f'#define {guard.group(1)}'))


class TestUapiHeader(unittest.TestCase):
    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture-child')
        self.text = gen_uapi.render(self.family)
        self.names = cgen.Names(self.family)

    def test_every_own_field_is_defined_once(self) -> None:
        for field in self.family.local_fields:
            with self.subTest(field=field.full_name):
                self.assertEqual(1, self.text.count(
                    f'#define {self.names.field(field)} '))

    def test_an_inherited_field_is_not_redefined(self) -> None:
        parent = support.load_family('ctlv-fixture')
        for field in parent.fields.values():
            with self.subTest(field=field.full_name):
                self.assertNotIn(f'#define {self.names.field(field)} ',
                                 self.text)

    def test_a_type_is_composed_of_family_record_and_number(self) -> None:
        """The record ID is spelled out, so the header is the ABI to read."""
        text = flat(self.text.replace('\\\n', ''))
        for field in self.family.local_fields:
            with self.subTest(field=field.full_name):
                self.assertIn(
                    flat(f'#define {self.names.field(field)} '
                         f'CTLV_TYPE({self.names.family_const()}, '
                         f'{field.record_id:#010x}, {field.number})'),
                    text)

    def test_only_a_definition_is_an_enum(self) -> None:
        """A type and an op are each one define, never an enum member."""
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        tags = set()
        for definition in family.local_definitions:
            if definition.header or definition.is_extension_point:
                continue
            tags.add(names.definition(definition)
                     if definition.kind == 'enum'
                     else names.bit_enum(definition))
        self.assertEqual(tags, set(re.findall(r'^enum (\S+) \{$',
                                              gen_uapi.render(family),
                                              re.M)))

    def test_family_numbers_are_an_enum_and_op_ids_are_defines(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        paths = spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                           schema=support.SCHEMA)
        registry = spec.load_registry(paths)
        families = gen_uapi.render_families(registry)

        number = [entry.name for entry in registry.entries].index(family.name)
        self.assertNotIn('enum _ctlv_family {', text)
        self.assertIn('enum _ctlv_family {', families)
        self.assertIn('\tCTLV_FAMILY_ROOT = 0,', families)
        self.assertIn(f'\t{names.family_const()} = {number},', families)
        for line in families.splitlines():
            self.assertLessEqual(cgen.width(line), cgen.LINE_LIMIT, line)
        for op in family.local_ops:
            with self.subTest(op=op.name):
                self.assertIn(f'#define {names.op_id(op)} '
                              f'CTLV_OP({names.family_const()}, '
                              f'{op.local_id})\n', text)

    def test_every_value_of_an_enum_is_numbered(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)

        for definition in family.local_definitions:
            if definition.kind != 'enum' or definition.header:
                continue
            for value_name, number in definition.values:
                self.assertIn(
                    f'\t{names.value(definition, value_name)} = {number},',
                    text)
        for body in re.findall(r'^enum \S+ \{\n(.*?)^\};', text, re.M | re.S):
            for line in body.splitlines():
                if not line.lstrip().startswith(('/*', '*')):
                    self.assertRegex(line, r'^\t\w+ = \d+,$')

    def test_every_value_has_its_comment_above_it(self) -> None:
        """A flag's goes above its mask, which is what a caller names."""
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        mode = family.definitions['mode']
        quirks = family.definitions['quirks']

        self.assertIn('\t/* The first value. */\n'
                      f'\t{names.value(mode, "disabled")} = 1,\n', text)
        self.assertIn(f'/* Bit 0. */\n#define {names.value(quirks, "slow")}',
                      text)
        self.assertIn(f'\t{names.value_bit(quirks, "slow")} = 0,\n', text)
        self.assertNotIn('\t/* Bit 0. */\n', text)

    def test_an_enum_fill_is_an_enum_of_the_family_that_fills(self) -> None:
        """The point has no values, so the declaring family has no enum."""
        variant = self.family.definitions['variant']
        self.assertIn('/* enum variant, an extension point of ctlv-fixture '
                      '*/\n'
                      f'enum {self.names.definition(variant)} {{\n',
                      self.text)
        self.assertIn(f'\t{self.names.value(variant, "fancy")} = 2,\n',
                      self.text)
        self.assertEqual('CTLV_FIXTURE_CHILD_VARIANT_FANCY',
                         self.names.value(variant, 'fancy'))
        parent = gen_uapi.render(support.load_family('ctlv-fixture'))
        self.assertNotIn('enum _ctlv_fixture_variant', parent)

    def test_a_header_enum_is_included_rather_than_defined(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        algo = family.definitions['hash-algo']

        self.assertIn('#include <linux/hash_info.h>\n', text)
        for value_name, _number in algo.values:
            self.assertNotIn(names.value(algo, value_name), text)

    def test_a_flags_mask_names_its_bits(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        flat_text = flat(text.replace('\\\n', ''))

        for definition in family.local_definitions:
            if definition.kind != 'flags':
                continue
            self.assertIn(f'enum {names.bit_enum(definition)} {{', text)
            mask = text.split(f'#define {names.value_mask(definition)}',
                              1)[1].split('\n\n', 1)[0]
            for value_name, bit in definition.values:
                bit_name = names.value_bit(definition, value_name)
                self.assertIn(f'\t{bit_name} = {bit},', text)
                value = names.value(definition, value_name)
                self.assertIn(value, mask)
                self.assertIn(
                    flat(f'#define {value} _BITULL({bit_name})'),
                    flat_text)
            self.assertNotRegex(mask, r'0x[0-9a-f]+ULL')

    def test_unit_metadata_is_defined_beside_the_type(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        fields = [field for field in family.local_fields
                  if field.decl.unit is not None]

        self.assertEqual(['scalar.freq', 'scalar.size'],
                         [field.full_name for field in fields])
        for field in fields:
            with self.subTest(field=field.full_name):
                assert field.decl.unit is not None
                self.assertIn(f'#define {names.field_unit(field)} '
                              f'CTLV_UNIT_{cgen.macro(field.decl.unit)}\n',
                              text)
                self.assertIn(f'#define {names.field_scale(field)} '
                              f'{field.decl.scale}\n', text)


class TestReferencePage(unittest.TestCase):
    def test_every_value_has_its_comment_below_it(self) -> None:
        text = gen_doc.render(support.load_family('ctlv-fixture'))

        self.assertIn('    1 = disabled\n        The first value.\n', text)
        self.assertIn('    bit 0 = slow\n        Bit 0.\n', text)
        self.assertIn('    0 = HASH_ALGO_MD4\n    1 = HASH_ALGO_MD5\n', text)

    def test_counts_and_byte_counts_read_as_a_spec_writes_them(self) -> None:
        family = support.load_family('ctlv-fixture')
        self.assertIn('per-mode[mode]?', gen_doc.render(family))
        self.assertIn('algos[hash-algo]?', gen_doc.signatures(family))
        reader = spec._Reader(pathlib.Path('test.yaml'), [])
        decl = reader.field('value', 'uint(1..4K, bytes)', 'test', None)
        assert isinstance(decl, spec.FieldDecl)
        self.assertEqual(('1..4K',), gen_doc._integer_bounds(decl))


class TestKernelSource(unittest.TestCase):
    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture')
        self.text = schema_source(self.family)
        self.names = cgen.Names(self.family)

    def trees(self) -> Iterator[tuple[semantics.Op, str, semantics.Node]]:
        for op in self.family.local_ops:
            for which in ('request', 'reply'):
                root = getattr(op, which)
                if root is not None:
                    yield op, which, root

    def test_every_tree_becomes_one_node_schema_array(self) -> None:
        for op, which, root in self.trees():
            symbol = self.names.node_schemas(op, which)
            with self.subTest(op=op.name, which=which):
                self.assertIn(f'static const struct ctlv_node_schema '
                              f'{symbol}[] = {{', self.text)
                ext_schemas = gen_kernel.ext_schemas(root)
                tree_macro = 'CTLV_SCHEMA_TREE_EXTS' if ext_schemas else \
                    'CTLV_SCHEMA_TREE'
                self.assertIn(
                    f'{tree_macro}({cgen.ident(op.name, which)})',
                    self.text)
                if not ext_schemas:
                    continue
                symbol = self.names.ext_schemas(op, which)
                self.assertIn(f'static const struct ctlv_ext_schema '
                              f'{symbol}[] = {{', self.text)

    def test_a_node_schema_array_has_one_entry_per_node(self) -> None:
        for op, which, root in self.trees():
            symbol = self.names.node_schemas(op, which)
            body = self.text.split(f'{symbol}[] = {{\n')[1].split('};')[0]
            initializers = re.findall(
                r'^\tCTLV_NODE_SCHEMA_[A-Z_]+\(', body, re.M)
            with self.subTest(op=op.name, which=which):
                self.assertEqual(len(gen_kernel.flatten(root)),
                                 len(initializers))

    def test_a_child_index_follows_its_parent(self) -> None:
        """Breadth first is what makes a child range contiguous."""
        for op, which, root in self.trees():
            flattened = gen_kernel.flatten(root)
            for index, flat in enumerate(flattened):
                with self.subTest(op=op.name, which=which, node=index):
                    if not flat.children_count:
                        self.assertEqual(0, flat.first_child)
                        continue
                    self.assertGreater(flat.first_child, index)
                    self.assertLessEqual(flat.required_children_count,
                                         flat.children_count)

    def test_the_descriptor_counts_the_unsized_table(self) -> None:
        symbol = self.names.op_schemas()

        self.assertIn(f'const struct ctlv_op_schema {symbol}[] = {{',
                      self.text)
        self.assertIn(f'\t.ops_count = ARRAY_SIZE({symbol}),', self.text)
        self.assertNotIn(f'static_assert(ARRAY_SIZE({symbol})', self.text)

    def test_every_op_states_its_availability(self) -> None:
        """Zero is neither mode, so the field is never left out."""
        for op in self.family.local_ops:
            with self.subTest(op=op.name):
                if op.type == 'event':
                    self.assertIn(
                        f'CTLV_OP_SCHEMA_EVENT({self.names.op_id(op)},',
                        self.text)
                    continue
                self.assertIn(
                    f'CTLV_OP_AVAILABILITY_{op.availability.upper()}',
                    self.text)
        executable = [op for op in self.family.local_ops
                      if op.type != 'event']
        self.assertEqual(len(executable),
                         self.text.count('CTLV_OP_AVAILABILITY_'))

    def test_every_executable_op_states_its_fd_access(self) -> None:
        executable = [op for op in self.family.local_ops
                      if op.type != 'event']
        for op in executable:
            with self.subTest(op=op.name):
                access = op.fd_access.upper().replace('-', '_')
                self.assertIn(f'CTLV_OP_FD_ACCESS_{access}', self.text)
        self.assertEqual(len(executable),
                         self.text.count('CTLV_OP_FD_ACCESS_'))

    def test_only_an_op_that_asked_for_a_hook_pair_is_marked(self) -> None:
        """The one field here describing an implementation, not a message."""
        marked = [op for op in self.family.local_ops if op.pre_post]
        self.assertEqual(['counts-get'], [op.name for op in marked])
        self.assertEqual(len(marked),
                         self.text.count('.pre_post = true'))
        self.assertNotIn('.pre_post = false', self.text)

    def test_a_device_bit_is_not_numbered_here(self) -> None:
        """Which bit an op owns depends on the family, not the declaration."""
        self.assertNotIn('state_index', self.text)
        self.assertNotIn('state_index', schema_header(self.family))

    def test_the_schema_has_no_ancestry_or_export(self) -> None:
        self.assertIn(f'const struct ctlv_family_schema '
                      f'{self.names.schema()} = {{', self.text)
        self.assertNotIn('\t.parent =', self.text)
        self.assertNotIn(
            f'EXPORT_SYMBOL_NS_GPL({self.names.schema()}, "CTLV");',
            self.text)

    def test_a_schema_header_includes_only_the_extension_ids_it_names(
            self) -> None:
        """Those are its parent's, unless the parent is the root."""
        parent = support.load_family('ctlv-fixture')
        for name in support.FIXTURE_FAMILIES:
            family = support.load_family(name)
            expected = ['<linux/ctlv_family.h>']
            if family.parent == parent.name:
                expected.append(cgen.Names(parent).kernel_header())
            with self.subTest(family=name):
                self.assertEqual(expected, re.findall(
                    r'^#include (\S+)$', schema_header(family), re.M))

    def test_the_transport_layout_guards_are_not_repeated(self) -> None:
        self.assertNotIn('static_assert(sizeof(struct ctlv_hdr)', self.text)
        self.assertNotIn('static_assert(CTLV_ALIGNTO', self.text)
        self.assertNotIn('static_assert(sizeof(struct ctlv_event_hdr)',
                         self.text)

    def test_a_header_enum_is_bounded_and_held_by_the_headers_names(
            self) -> None:
        algo = self.family.definitions['hash-algo']
        field = self.family.fields['counts-get algo']

        self.assertEqual('HASH_ALGO_MD4',
                         self.names.value(algo, 'HASH_ALGO_MD4'))
        self.assertIn(
            flat(f'CTLV_NODE_SCHEMA_ENUM_RANGE({self.names.field(field)}, '
                 '1, HASH_ALGO_MD4, HASH_ALGO__LAST - 1)'),
            flat(self.text))
        self.assertNotIn('static_assert(HASH_ALGO', self.text)

    def test_a_header_enum_without_an_end_ends_at_its_highest(self) -> None:
        algo = self.family.definitions['hash-algo']
        assert algo.header is not None
        endless = dataclasses.replace(
            algo, header=dataclasses.replace(algo.header, end=None),
            values=(('HASH_ALGO_SHA1', 2), ('HASH_ALGO_MD5', 1)))
        self.assertEqual(('HASH_ALGO_MD5', 'HASH_ALGO_SHA1'),
                         cgen.header_bounds(endless))

    def test_an_enum_extension_point_is_named_in_place_of_a_range(
            self) -> None:
        """The device's family chain gives the maximum at run time."""
        field = self.family.fields['optional-check variant']
        extension = self.family.enum_extensions['variant']
        constant = self.names.enum_extension(extension)

        self.assertEqual('CTLV_FIXTURE_ENUM_EXT_VARIANT', constant)
        self.assertIn(
            flat(f'CTLV_NODE_SCHEMA_ENUM_EXT({self.names.field(field)}, 1, '
                 f'{self.names.family_const()}, {constant}),'),
            flat(self.text))
        self.assertIn(f'#define {constant} 1\n', schema_header(self.family))
        self.assertNotIn('enum_fill_schemas', self.text)

    def test_the_limits_are_asserted(self) -> None:
        self.assertIn('<= CTLV_MAX_INLINE_MESSAGE_LEN);', self.text)
        self.assertIn('<= CTLV_MAX_EVENT_QUEUE_LEN);', self.text)
        self.assertIn('<= CTLV_MAX_DECLARED_RECORD_DEPTH);', self.text)

    def test_what_the_chain_costs_a_uevent_is_asserted(self) -> None:
        """The names a device announces, charged as an environment charges."""
        variables = ['CTLV_FAMILY=ctlv-fixture', 'CTLV_COMPATIBLE_0=ctlv',
                     'CTLV_COMPATIBLE_1=ctlv-fixture',
                     'CTLV_COMPATIBLE_N=2']
        total = sum(len(item) + 1 for item in variables)
        self.assertIn(f'static_assert({total} <= CTLV_UEVENT_MAX_LEN);',
                      self.text)
        self.assertIn(f'static_assert({len(variables)} '
                      '<= CTLV_UEVENT_MAX_VARS);', self.text)

    def test_the_root_family_fits_its_one_level_chain(self) -> None:
        root = schema_source(support.load(spec.CORE_FAMILY_NAME))
        variables = ['CTLV_FAMILY=ctlv', 'CTLV_COMPATIBLE_0=ctlv',
                     'CTLV_COMPATIBLE_N=1']
        total = sum(len(item) + 1 for item in variables)

        self.assertIn(f'static_assert({total} <= CTLV_UEVENT_MAX_LEN);',
                      root)
        self.assertIn(f'static_assert({len(variables)} '
                      '<= CTLV_UEVENT_MAX_VARS);', root)


class TestReEntry(unittest.TestCase):
    """A re-entered level is pointed back at rather than emitted again."""

    family: semantics.Family
    flat: tuple[gen_kernel.FlatNode, ...]

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load(spec.CORE_FAMILY_NAME)
        cls.flat = gen_kernel.flatten(
            support.reply(cls.family, 'op-schema-get'))

    def test_a_re_entry_points_at_the_level_it_is_part_of(self) -> None:
        found = 0
        for index, flat in enumerate(self.flat):
            if flat.node.reenters is None:
                continue
            found += 1
            with self.subTest(node=index):
                self.assertLess(flat.first_child, index)
                self.assertGreaterEqual(
                    flat.first_child + flat.children_count, index,
                    'the level a node re-enters holds that node')
        self.assertEqual(4, found, 'both members of both trees')

    def test_the_tree_holds_one_level_per_message_and_no_more(self) -> None:
        """Which is what keeps a schema of any depth a finite tree."""
        fields = [flat.node.child_field.full_name for flat in self.flat
                  if not flat.node.is_root]
        self.assertEqual(2, fields.count('schema-node.fields'))
        self.assertEqual(2, fields.count('schema-node.optional-fields'))
        self.assertEqual(37, len(self.flat))

    def test_the_generated_node_marks_reentry(self) -> None:
        text = schema_source(self.family)
        self.assertEqual(4, text.count('CTLV_NODE_SCHEMA_RECORD_REENTERS('))


class TestCoreMessages(unittest.TestCase):
    """The messages the core builds where no op declared one.

    Error information is the whole of it, so it belongs to the root family.
    """

    def setUp(self) -> None:
        self.family = support.load(spec.CORE_FAMILY_NAME)
        self.names = cgen.Names(self.family)
        self.source = schema_source(self.family)
        self.header = schema_header(self.family)
        self.message = self.family.messages[0]

    def test_the_root_family_declares_the_error_information(self) -> None:
        self.assertEqual(['error-info'],
                         [message.name for message in self.family.messages])

    def test_the_message_becomes_a_tree_of_its_own(self) -> None:
        node_schemas = self.names.message_node_schemas(self.message)
        schema = self.names.message_schema(self.message)
        self.assertIn(
            f'static const struct ctlv_node_schema {node_schemas}[] = {{',
            self.source)
        self.assertIn(f'const struct ctlv_schema_tree {schema} = {{\n'
                      f'\t.node_schemas = {node_schemas},\n'
                      '\t.node_schemas_count =\n'
                      f'\t\tARRAY_SIZE({node_schemas}),\n'
                      '};',
                      self.source)
        self.assertIn(f'extern const struct ctlv_schema_tree {schema};',
                      self.header)
        self.assertNotIn(f'EXPORT_SYMBOL_NS_GPL({schema},', self.source)

    def test_the_length_is_stated_and_held_to_the_transports_limit(
            self) -> None:
        length = self.names.message_max_len(self.message)
        self.assertIn(f'#define {length} {self.message.max_len}', self.header)
        self.assertIn(f'static_assert({length} <= CTLV_MAX_ERROR_LEN);',
                      self.source)

    def test_the_core_is_the_only_builder(self) -> None:
        """A chain has one error message, the one its root declared."""
        for name in ('ctlv-fixture', 'ctlv-fixture-child'):
            family = support.load_family(name)
            with self.subTest(family=name):
                self.assertEqual((), family.messages)
                self.assertNotIn('error_info',
                                 schema_source(family))


class TestFamilyDescriptor(unittest.TestCase):
    """The hooks a family has to implement, and where they may appear.

    The pair around every op is the device's, so no family has one. The pair
    of one op is the subject of the whole chain: the marked op is the
    parent's, so only the parent implements its pair.
    """

    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture-child')
        self.header = family_header(self.family)
        self.source = family_source(self.family)
        self.names = cgen.Names(self.family)

    def test_only_callbacks_carry_the_op_type_suffix(self) -> None:
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)

        for op in gen_family.bindings(family):
            with self.subTest(op=op.name):
                self.assertTrue(names.callback(op).endswith(
                    f'_{op.type}'))
                self.assertFalse(names.op_helper(op, 'enable').endswith(
                    f'_{op.type}_enable'))
                self.assertFalse(names.op_hook(op, 'pre').endswith(
                    f'_{op.type}_pre'))

    def test_the_device_and_not_the_family_wraps_every_op(self) -> None:
        for name in support.ALL_FAMILIES:
            source = family_source(support.load_any(name))
            for which in ('pre', 'post'):
                with self.subTest(family=name, which=which):
                    self.assertNotIn(f'\t.{which} = ', source)

    def test_only_an_op_that_asked_for_a_pair_gets_one(self) -> None:
        """And only in the family that declared that op."""
        self.assertEqual(['counts-get'],
                         [op.name for op in self.family.ops if op.pre_post])
        parent = family_header(support.load_family('ctlv-fixture'))
        self.assertIn('Per-op hooks inside the device pair.', parent)
        for name in support.FIXTURE_FAMILIES:
            family = support.load_family(name)
            header = family_header(family)
            source = family_source(family)
            names = cgen.Names(family)
            for op in family.ops:
                for which in ('pre', 'post'):
                    symbol = names.op_hook(op, which)
                    with self.subTest(family=name, op=op.name, which=which):
                        declaration = (f'{symbol}(struct ctlv *ctlv, '
                                       'struct ctlv_op_ctx *ctx);')
                        binding = (
                            f'\tCTLV_OP_{op.type.upper()}_PRE_POST('
                            f'{names.op_ident(op)}),')
                        if op.pre_post and op in family.local_ops:
                            self.assertIn(declaration, flat(header))
                            self.assertIn(binding, source)
                        else:
                            # The declaration rather than the name: a getter
                            # of the op's own field holds _pre inside _present.
                            self.assertNotIn(declaration, flat(header))
                            self.assertNotIn(binding, source)

    def test_a_hook_reaches_no_constant_a_caller_compiles_against(
            self) -> None:
        """A pair is who implements an op, not what it looks like."""
        uapi = gen_uapi.render(self.family)
        metadata = schema_header(self.family)
        metadata += schema_source(self.family)
        symbols = [self.names.op_hook(op, which) for op in self.family.ops
                   for which in ('pre', 'post')]
        for symbol in symbols:
            with self.subTest(symbol=symbol):
                self.assertNotIn(symbol, uapi + metadata)
        self.assertNotIn('pre_post', uapi)
        self.assertNotIn('PRE_POST', uapi)


class TestRootDescriptor(unittest.TestCase):
    """The root descriptor, emitted like every other family's."""

    def setUp(self) -> None:
        self.family = support.load(spec.CORE_FAMILY_NAME)
        self.header = family_header(self.family)
        self.source = family_source(self.family)
        self.names = cgen.Names(self.family)

    def test_the_root_binds_its_own_action_and_query_ops(self) -> None:
        bound = [op.name for op in gen_family.bindings(self.family)]
        self.assertEqual([op.name for op in self.family.ops
                          if op.type != 'event'], bound)
        self.assertIn(f'static const struct ctlv_op '
                      f'{self.names.ops()}[] = {{', self.source)
        self.assertIn(f'\t.ops_count = ARRAY_SIZE({self.names.ops()}),',
                      self.source)
        for op in self.family.ops:
            symbol = self.names.callback(op)
            with self.subTest(op=op.name):
                if op.type == 'event':
                    self.assertNotIn(symbol, self.source)
                else:
                    suffix = '_PRE_POST' if op.pre_post else ''
                    binding = (
                        f'\tCTLV_OP_{op.type.upper()}{suffix}('
                        f'{self.names.op_ident(op)}),')
                    self.assertIn(binding, self.source)

    def test_the_root_descriptor_is_exported_for_direct_children(self) -> None:
        self.assertIn(
            f'EXPORT_SYMBOL_NS_GPL({self.names.descriptor()}, "CTLV");',
            self.source)

        direct = support.load_family('ctlv-fixture')
        source = family_source(direct)
        self.assertIn('\t.parent = &ctlv_family,', source)

    def test_the_tables_of_a_chain_partition_its_ops(self) -> None:
        """One table per family, each holding what that family declared."""
        for name in support.FIXTURE_FAMILIES:
            family = support.load_any(name)
            bound = []
            for level in family.chain:
                owner = support.load_any(level.name)
                bound += [op.name for op in gen_family.bindings(owner)]
            with self.subTest(family=name):
                # Root ops belong to the CTLV core, not this family.
                for op in self.family.local_ops:
                    self.assertNotIn(op.name,
                                     [item.name for item in
                                      gen_family.bindings(family)])
                self.assertEqual(sorted(bound),
                                 sorted(op.name for op in family.ops
                                        if op.type != 'event'))


class TestEventCreators(unittest.TestCase):
    """Where one event of each declared kind begins.

    The fixture declares two events and its child inherits both.
    """

    def setUp(self) -> None:
        self.family = support.load_any('ctlv-fixture')
        self.header = family_header(self.family)
        self.names = cgen.Names(self.family)

    def test_an_event_gets_a_creator_naming_its_op(self) -> None:
        events = [op for op in self.family.local_ops if op.type == 'event']
        self.assertEqual(['thing-changed', 'pinged'],
                         [op.name for op in events])
        for op in events:
            with self.subTest(op=op.name):
                self.assertIn(
                    flat(f'CTLV_EVENT_CREATE('
                         f'{self.names.op_ident(op)}, '
                         f'{self.names.op_id(op)})'),
                    flat(self.header))

    def test_an_action_or_query_has_no_creator(self) -> None:
        for op in self.family.local_ops:
            if op.type == 'event':
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(self.names.op_helper(op, 'create'),
                                 self.header)

    def test_an_inherited_event_keeps_its_own_familys_creator(self) -> None:
        """One event is created by the family that described it."""
        child = support.load_any('ctlv-fixture-child')
        header = family_header(child)
        names = cgen.Names(child)
        for op in child.ops:
            if op.type != 'event':
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(f'*{names.op_helper(op, "create")}(', header)
                self.assertIn(f'#include {self.names.kernel_header()}', header)


class TestInheritedEmission(unittest.TestCase):
    """What a child emits: its own declarations and one pointer upwards."""

    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture-child')
        self.parent = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.parent_names = cgen.Names(self.parent)
        self.uapi = gen_uapi.render(self.family)
        self.header = schema_header(self.family)
        self.source = schema_source(self.family)
        self.family_header = family_header(self.family)
        self.family_source = family_source(self.family)

    def test_only_the_childs_own_ops_are_described(self) -> None:
        local = [op.name for op in self.family.local_ops]
        self.assertEqual(['child-info-get', 'child-dump'], local)
        self.assertIn(f'extern const struct ctlv_op_schema '
                      f'{self.names.op_schemas()}[];', self.header)
        self.assertNotIn('_N_OPS', self.header)
        for op in self.family.ops:
            with self.subTest(op=op.name):
                for which in ('request', 'reply'):
                    if getattr(op, which) is None:
                        continue
                    symbol = self.names.node_schemas(op, which)
                    if op in self.family.local_ops:
                        self.assertIn(f'{symbol}[] = {{', self.source)
                    else:
                        self.assertNotIn(symbol, self.source)

    def test_an_inherited_length_is_not_repeated(self) -> None:
        """A length belongs to the family that described the op."""
        for op in self.family.ops:
            if op in self.family.local_ops:
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(self.names.op_id(op), self.source)

    def test_the_schema_carries_no_parent_link(self) -> None:
        self.assertNotIn('\t.parent =', self.source)
        self.assertNotIn(self.parent_names.schema(), self.source)

    def test_the_whole_chain_is_what_a_device_announces(self) -> None:
        """Every family of it, so a rule may match an ancestor."""
        variables = ['CTLV_FAMILY=ctlv-fixture-child',
                     'CTLV_COMPATIBLE_0=ctlv',
                     'CTLV_COMPATIBLE_1=ctlv-fixture',
                     'CTLV_COMPATIBLE_2=ctlv-fixture-child',
                     'CTLV_COMPATIBLE_N=3']
        total = sum(len(item) + 1 for item in variables)
        self.assertIn(f'static_assert({total} <= CTLV_UEVENT_MAX_LEN);',
                      self.source)
        self.assertIn(f'static_assert({len(variables)} '
                      '<= CTLV_UEVENT_MAX_VARS);', self.source)

    def test_a_filled_extension_point_is_the_childs_own_metadata(self) -> None:
        fill = self.family.fills[0]
        self.assertEqual(1, len(self.family.fills))
        ext_fill_schemas = self.names.ext_fill_schemas()
        owner = cgen.family_const(self.family.owners[fill.family_id])
        extension = self.names.extension(
            self.family.extensions[fill.name])
        initializer = (
            f'CTLV_EXT_FILL_SCHEMA({owner}, {extension}, '
            f'{cgen.ident(fill.name)})'
        )
        self.assertIn(f'static const struct ctlv_ext_fill_schema '
                      f'{ext_fill_schemas}[] = {{', self.source)
        self.assertIn(initializer, flat(self.source))
        self.assertIn(f'.ext_fill_schemas = {ext_fill_schemas},',
                      flat(self.source))
        self.assertIn(f'.ext_fill_schemas_count = '
                      f'ARRAY_SIZE({ext_fill_schemas}),',
                      flat(self.source))
        # The record a fill joins keeps the ID the declaring family gave it.
        self.assertNotIn('record_extension', self.source)

    def test_an_enum_fill_gives_its_highest_value(self) -> None:
        [variant] = self.family.enum_fills
        assert variant.extension is not None
        table = self.names.enum_fill_schemas()
        initializer = (
            f'CTLV_ENUM_FILL_SCHEMA({self.parent_names.family_const()}, '
            f'{self.names.enum_extension(variant.extension)}, 2),'
        )
        self.assertIn(f'static const struct ctlv_enum_fill_schema '
                      f'{table}[] = {{', self.source)
        self.assertIn(initializer, flat(self.source))
        self.assertIn(f'.enum_fill_schemas = {table},', flat(self.source))
        self.assertIn(f'.enum_fill_schemas_count = ARRAY_SIZE({table}),',
                      flat(self.source))
        # The constant is the declaring family's, which the child includes.
        self.assertNotIn('#define CTLV_FIXTURE_ENUM_EXT_VARIANT',
                         self.header)

    def test_a_parent_header_is_included_rather_than_repeated(self) -> None:
        self.assertIn(f'#include {cgen.uapi_header(self.parent.name)}',
                      self.uapi)
        self.assertIn(f'#include {self.parent_names.kernel_header()}',
                      self.family_header)

    def test_a_child_binds_its_own_ops_and_inherits_the_rest(self) -> None:
        """Its table is its own declarations, in declaration order.

        What it inherited is answered through the descriptor of the family that
        declared it, which is the one symbol of that family this one names.
        """
        answered = gen_family.bindings(self.family)
        self.assertEqual([op.name for op in self.family.local_ops
                          if op.type != 'event'],
                         [op.name for op in answered])
        for op in self.family.ops:
            if op in self.family.local_ops:
                continue
            self.assertNotIn(self.names.callback(op), self.family_source)
        body = self.family_source.split(
            f'{self.names.ops()}[] = {{\n')[1]
        body = body.split('};')[0]
        offsets = [body.index(self.names.op_ident(op)) for op in answered]
        self.assertEqual(sorted(offsets), offsets)
        self.assertNotIn('_N_BINDINGS', self.family_source)
        self.assertIn(f'\t.ops_count = ARRAY_SIZE({self.names.ops()}),',
                      self.family_source)
        self.assertIn(f'\t.parent = &{self.parent_names.descriptor()},',
                      self.family_source)

    def test_what_a_child_does_with_the_extension_point_it_fills(self) -> None:
        """One pair per fill, in the order the metadata has them."""
        fill = self.family.fills[0]
        self.assertEqual(1, len(self.family.fills))
        for which, returns in (('fill', 'int'), ('read', 'void')):
            symbol = self.names.ext_callback(fill, which)
            with self.subTest(which=which):
                self.assertIn(f'{returns} {symbol}(', self.family_header)
                self.assertIn(f'{symbol}(struct ctlv *ctlv, ',
                              flat(self.family_header))
        ext_ops = self.names.ext_ops()
        owner = cgen.family_const(self.family.owners[fill.family_id])
        extension = self.names.extension(
            self.family.extensions[fill.name])
        initializer = (
            f'CTLV_EXT_OPS({owner}, {extension}, '
            f'{self.names.ext_ident(fill)})'
        )
        self.assertIn(f'static const struct ctlv_ext_ops '
                      f'{ext_ops}[] = {{', self.family_source)
        self.assertIn(initializer, flat(self.family_source))
        self.assertIn(f'.ext_ops = {ext_ops},',
                      flat(self.family_source))
        self.assertIn(f'.ext_ops_count = ARRAY_SIZE({ext_ops}),',
                      flat(self.family_source))

    def test_a_helper_is_generated_by_the_family_that_declared_the_op(
            self) -> None:
        """What a device may decide about one op, for this family's own ops.

        Only an optional action or query may be opted in to, only a dynamic
        one may be enabled and disabled, and an event is neither.
        """
        for op in self.family.ops:
            expected: tuple[str, ...] = ()
            if op.type != 'event':
                if op.optional:
                    expected += ('provide',)
                if op.availability == 'dynamic':
                    expected += ('enable', 'disable')
            for what in ('provide', 'enable', 'disable'):
                declaration = f'CTLV_OP_{what.upper()}(' \
                              f'{self.names.op_ident(op)},'
                with self.subTest(op=op.name, helper=what):
                    if what in expected and op in self.family.local_ops:
                        self.assertIn(declaration, self.family_header)
                    else:
                        self.assertNotIn(declaration, self.family_header)


class TestReaders(unittest.TestCase):
    """What a callback reads a message with.

    The fixture's requests hold one field of every schema kind, at every
    cardinality.
    """

    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.header = family_header(self.family)
        self.templates = (support.KERNEL_DIR / 'include' / 'linux' /
                          'ctlv_family.h').read_text()

    def field(self, full_name: str) -> semantics.Field:
        return self.family.fields[full_name]

    def base(self, full_name: str) -> str:
        return self.names.field_ident(self.field(full_name))

    def test_a_kind_is_read_by_the_reader_of_that_kind(self) -> None:
        """One C type per schema kind, and the kind it is held to."""
        forms = {
            'u': 'U64',
            's': 'S64',
            'scalar.mode': 'U64',
            'scalar.quirks': 'U64',
            'scalar.flag': 'BOOL',
            'scalar.text': 'STRING',
            'scalar.fixed': 'BINARY',
        }
        for full_name, form in forms.items():
            field = self.field(full_name)
            name = self.base(full_name)
            with self.subTest(field=full_name):
                self.assertIn(
                    flat(f'CTLV_IN_REF({name}, {self.names.field(field)}, '
                         f'{cgen.KINDS[field.decl.kind]})'),
                    flat(self.header))
                self.assertIn(f'CTLV_IN_HAS({name})', self.header)
                self.assertIn(f'CTLV_IN_{form}({name})', self.header)

    def test_a_field_has_its_doc_above_its_readers(self) -> None:
        doc = self.field('counts-get few').decl.doc
        self.assertIn(flat(f'/* {doc} */\n'
                           f'CTLV_IN_REF({self.base("counts-get few")},'),
                      flat(self.header))

    def test_a_field_has_one_reader_wherever_it_is_placed(self) -> None:
        """u is in three requests; a reader finds it by type in any record."""
        self.assertEqual(1, self.header.count(
            f'CTLV_IN_REF({self.base("u")},'))
        self.assertIn('ctlv_input_field(&cursor, record, _name##_ref());',
                      self.templates)

    def test_only_a_field_some_request_holds_is_read(self) -> None:
        for full_name in ('counts-get many', 'records-dump level1'):
            with self.subTest(field=full_name):
                self.assertNotIn(f'CTLV_IN_REF({self.base(full_name)},',
                                 self.header)

    def test_presence_is_what_an_optional_field_is_asked_about(self) -> None:
        name = self.base('optional-check mode')

        self.assertIn(f'CTLV_IN_HAS({name})', self.header)
        self.assertIn('return ctlv_cursor_ok(&cursor);', self.templates)

    def test_a_repeated_field_is_read_through_its_iterator_alone(self) -> None:
        """No presence helper: an absent one is an iteration of nothing.

        The getter reads the occurrence the iterator left the cursor on, so it
        takes that cursor rather than the record.
        """
        field = self.field('counts-get few')
        name = self.base('counts-get few')
        self.assertIn(
            flat(f'#define {self.names.field_helper(field, "for_each")}'
                 '(_cursor, _record) \\\n'
                 f'CTLV_IN_FOR_EACH({name}, _cursor, _record)'),
            flat(self.header))
        self.assertNotIn(f'CTLV_IN_HAS({name})', self.header)
        self.assertIn('#define CTLV_IN_FOR_EACH', self.templates)
        self.assertIn('for (struct ctlv_cursor *__ctlv_cursor =',
                      self.templates)
        self.assertIn('ctlv_input_field(_cursor, _record, _name##_ref());',
                      self.templates)
        self.assertIn('ctlv_cursor_ok(__ctlv_cursor);', self.templates)
        self.assertIn('ctlv_cursor_next(__ctlv_cursor))', self.templates)
        self.assertIn(f'CTLV_IN_EACH_U64({name})', self.header)
        self.assertIn('return ctlv_cursor_u64(cursor);', self.templates)

    def test_a_repeated_binary_failure_clears_its_length(self) -> None:
        macro = '#define CTLV_IN_EACH_BINARY'

        self.assertIn(macro, self.templates)
        self.assertIn('*len = 0;', self.templates)
        self.assertIn('return NULL;', self.templates)

    def test_a_record_is_stood_on_rather_than_valued(self) -> None:
        """What a record holds is read by reading its members."""
        field = self.field('counts-get request')
        self.assertIn(self.names.field_helper(field, 'for_each'), self.header)
        readers = cgen.input_readers(self.names, field)
        self.assertNotIn(self.names.field_helper(field, 'get'), readers)

    def test_a_blob_is_resolved_rather_than_valued(self) -> None:
        """A resolver hands back the memory the descriptor names.

        It needs the request being executed for that, so it is the one reader
        that takes the context.
        """
        for full_name in ('blobs-submit input-va', 'blobs-submit output-fd'):
            name = self.base(full_name)
            with self.subTest(field=full_name):
                self.assertIn(f'CTLV_IN_HAS({name})', self.header)
                self.assertIn(f'CTLV_IN_BLOB({name})', self.header)
        self.assertIn('return ctlv_blob_resolve(ctx, &cursor);',
                      self.templates)

    def test_a_repeated_blob_is_resolved_where_its_iterator_stopped(
            self) -> None:
        """The cursor of an occurrence is the descriptor to resolve."""
        field = self.field('blobs-submit pieces')
        self.assertIn(self.names.field_helper(field, 'for_each'), self.header)
        self.assertIn(f'CTLV_IN_EACH_BLOB({self.base("blobs-submit pieces")})',
                      self.header)
        self.assertIn('return ctlv_blob_resolve(ctx, cursor);',
                      self.templates)

    def test_every_reader_is_declared_where_its_name_was_taken(self) -> None:
        """The names the symbol table reserves are the names emitted here."""
        for name in support.FIXTURE_FAMILIES:
            family = support.load_family(name)
            names = cgen.Names(family)
            header = flat(family_header(family))
            symbols = cgen.symbols(family)
            for field in semantics.own_uses(family)[0].values():
                base = names.field_ident(field)
                each = 'EACH_' if field.decl.max_occurrences != 1 else ''
                with self.subTest(family=name, field=field.full_name):
                    for reader in cgen.input_readers(names, field):
                        self.assertIn(reader, symbols)
                    self.assertIn(f'CTLV_IN_REF({base},', header)
                    if each:
                        self.assertIn(names.field_helper(field, 'for_each'),
                                      header)
                    else:
                        self.assertIn(f'CTLV_IN_HAS({base})', header)
                    if field.decl.kind == 'blob':
                        self.assertIn(f'CTLV_IN_{each}BLOB({base})', header)
                    elif cgen.getter_value(field):
                        form = gen_family.INPUT_MACROS[field.decl.kind]
                        self.assertIn(f'CTLV_IN_{each}{form}({base})',
                                      header)


class TestWriters(unittest.TestCase):
    """What a callback writes a reply with, on the same fixture family."""

    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.header = family_header(self.family)

    def writer(self, full_name: str, what: str = 'put') -> str:
        return self.names.field_helper(self.family.fields[full_name], what)

    def base(self, full_name: str, what: str = 'put') -> str:
        return self.writer(full_name, what).removesuffix(f'_{what}')

    def test_a_kind_is_written_by_the_serializer_of_that_kind(self) -> None:
        """The mirror of the getters: one C type per schema kind.

        On ctlv-test, whose replies hold every kind the fixture's requests do.
        """
        family = support.load('ctlv-test')
        names = cgen.Names(family)
        header = flat(family_header(family))
        written: dict[str, semantics.Field] = {}
        for field in semantics.own_uses(family)[1].values():
            written.setdefault(field.decl.kind, field)
        self.assertLessEqual(set(gen_family.PUT_MACROS), set(written))
        for kind, form in gen_family.PUT_MACROS.items():
            field = written[kind]
            with self.subTest(kind=kind, field=field.full_name):
                self.assertIn(
                    flat(f'CTLV_PUT_{form}({names.field_ident(field)}, '
                         f'{names.field(field)}, {cgen.KINDS[kind]})'),
                    header)

    def test_a_field_has_its_doc_above_its_writer(self) -> None:
        doc = self.family.fields['counts-get many'].decl.doc
        self.assertIn(flat(f'/* {doc} */\n'
                           f'CTLV_PUT_U64({self.base("counts-get many")},'),
                      flat(self.header))

    def test_a_record_is_opened_onto_a_cursor_of_the_callers_own(self) -> None:
        field = self.family.fields['records-dump extension']
        self.assertIn(
            flat(f'CTLV_RECORD_START('
                 f'{self.base("records-dump extension", "start")}, '
                 f'{self.names.field(field)}, CTLV_SCHEMA_RECORD)'),
            flat(self.header))
        self.assertNotIn(self.writer('records-dump extension'), self.header)

    def test_a_repeated_field_is_appended_one_occurrence_at_a_time(
            self) -> None:
        """No iterator on this side: another occurrence is another call."""
        field = self.family.fields['counts-get many']
        self.assertIn(
            flat(f'CTLV_PUT_U64({self.base("counts-get many")}, '
                 f'{self.names.field(field)}, CTLV_SCHEMA_UINT)'),
            flat(self.header))
        self.assertNotIn(self.names.field_helper(field, 'for_each'),
                         self.header)

    def test_only_a_field_some_reply_holds_is_written(self) -> None:
        for full_name in ('scalar.text', 'counts-get few'):
            with self.subTest(field=full_name):
                self.assertNotIn(self.writer(full_name), self.header)

    def test_a_blob_is_not_written_at_all(self) -> None:
        """A blob describes a caller's memory, so a reply never holds one."""
        for full_name in ('blobs-submit input-va', 'blobs-submit output-fd'):
            field = self.family.fields[full_name]
            with self.subTest(field=full_name):
                self.assertEqual((), cgen.output_writers(self.names, field))

    def test_every_serializer_is_declared_where_its_name_was_taken(
            self) -> None:
        """And only by the first family of the chain writing its field."""
        for name in support.FIXTURE_FAMILIES:
            family = support.load_family(name)
            names = cgen.Names(family)
            header = flat(family_header(family))
            symbols = cgen.symbols(family)
            outputs = semantics.own_uses(family)[1]
            for field in semantics.field_uses(family)[1].values():
                for writer in cgen.output_writers(names, field):
                    with self.subTest(family=name, field=field.full_name):
                        self.assertIn(writer, symbols)
                        if field.decl.kind == 'record':
                            macro = 'CTLV_RECORD_START'
                            base = writer.removesuffix('_start')
                        else:
                            macro = ('CTLV_PUT_' +
                                     gen_family.PUT_MACROS[field.decl.kind])
                            base = writer.removesuffix('_put')
                        self.assertEqual(field.type in outputs,
                                         f'{macro}({base},' in header)


class TestUserBinding(unittest.TestCase):
    """What an application gets, on the fixture family and its child.

    The header is names, and the source is the specification as data.
    """

    def setUp(self) -> None:
        self.family = support.load_family('ctlv-fixture')
        self.child = support.load_family('ctlv-fixture-child')
        self.names = cgen.Names(self.family)
        self.child_names = cgen.Names(self.child)
        self.header = gen_user.render_header(self.family)
        self.source = gen_user.render_source(self.family)
        self.child_header = gen_user.render_header(self.child)
        self.child_source = gen_user.render_source(self.child)

    def test_an_action_and_a_query_are_one_call_each(self) -> None:
        """And the buffer a reply needs comes from the device.

        An inheriting family's device answers with more than this
        specification computed, and even an op that answers nothing needs room
        for the error information the device may send instead.
        """
        for op in self.family.local_ops:
            if op.type == 'event':
                continue
            with self.subTest(op=op.name):
                call = 'ctlv_query' if op.type == 'query' else 'ctlv_action'
                self.assertIn(flat(f'static inline int '
                                   f'{self.names.user_op(op)}('
                                   'struct ctlv_dev *dev'), flat(self.header))
                self.assertIn(f'return {call}(dev, {self.names.op_id(op)},',
                              flat(self.header))

    def test_a_field_has_its_doc_above_its_helpers(self) -> None:
        """Once above its putters, and once above its readers."""
        many = self.family.fields['counts-get many']
        doc = f'/* {many.decl.doc} */\n'
        self.assertIn(flat(f'{doc}static inline int '
                           f'{self.names.field_helper(many, "put")}('),
                      flat(self.header))
        self.assertIn(flat(f'{doc}#define '
                           f'{self.names.field_helper(many, "for_each")}('),
                      flat(self.header))
        self.assertEqual(2, self.header.count(doc))

    def test_a_header_enum_is_bounded_by_its_constants(self) -> None:
        field = self.family.fields['counts-get algo']
        node = self.source.split(f'.type = {self.names.field(field)},',
                                 1)[1].split('}', 1)[0]

        self.assertIn('.min = HASH_ALGO_MD4,', node)
        self.assertIn('.max = HASH_ALGO__LAST - 1,', node)
        self.assertEqual(('HASH_ALGO_MD4', 'HASH_ALGO__LAST - 1', 'value',
                          'known'),
                         gen_user._reader_arguments(self.names, field,
                                                    ('value', 'known')))

    def test_an_extension_enum_is_read_as_a_number(self) -> None:
        """What it may hold is the device family's, so nothing is known.

        Each binding records the range its own family's device introspects.
        """
        field = self.family.fields['optional-check variant']
        getter = self.names.field_helper(field, 'get')
        self.assertIn(flat(f'static inline bool {getter}(const struct '
                           'ctlv_hdr *record, __u64 *value)'),
                      flat(self.header))
        self.assertEqual(('value',),
                         gen_user._reader_arguments(self.names, field, ()))
        for source, maximum in ((self.source, 0), (self.child_source, 2)):
            with self.subTest(maximum=maximum):
                node = source.split(f'.type = {self.names.field(field)},',
                                    1)[1].split('}', 1)[0]
                self.assertIn('.kind = CTLV_SCHEMA_ENUM,', node)
                self.assertIn('.min = 1,', node)
                self.assertIn(f'.max = {maximum},', node)

    def test_an_event_is_subscribed_to_rather_than_executed(self) -> None:
        events = [op for op in self.family.local_ops if op.type == 'event']
        self.assertEqual(['thing-changed', 'pinged'],
                         [op.name for op in events])
        for op in events:
            with self.subTest(op=op.name):
                for what in ('is', 'subscribe', 'unsubscribe'):
                    self.assertIn(
                        f'{self.names.user_op_helper(op, what)}(',
                        self.header)
                # Nothing to execute, so no wrapper and no request builder.
                self.assertNotIn(f'{self.names.user_op(op)}(struct ctlv_dev',
                                 flat(self.header))
                self.assertNotIn(
                    self.names.user_op_helper(op, 'request_new'), self.header)

    def test_a_request_starts_at_this_family_and_is_not_capped_there(
            self) -> None:
        """A descendant's device takes a longer request than this one does."""
        for op in self.family.local_ops:
            if op.type == 'event' or op.request is None:
                continue
            with self.subTest(op=op.name):
                helper = self.names.user_op_helper(op, 'request_new')
                length = self.names.user_op_len(op, 'request-max-len')
                self.assertIn(f'{helper}(struct ctlv_msg **request)',
                              flat(self.header))
                self.assertIn(f'const __u32 len = {length};',
                              flat(self.header))
        self.assertIn('return ctlv_msg_new(len, request);', self.header)
        self.assertNotIn('ctlv_msg_new_max(', self.header)

    def test_a_length_this_family_computed_is_published(self) -> None:
        """As numbers a caller may hold a device to, and no more than that."""
        for op in self.family.local_ops:
            for field, what in gen_user.LENGTHS:
                value = getattr(op.sizes, field)
                with self.subTest(op=op.name, length=what):
                    define = f'#define {self.names.user_op_len(op, what)} '
                    if value:
                        self.assertIn(f'{define}{value}\n', self.header)
                    else:
                        self.assertNotIn(define, self.header)

    def test_what_a_device_answers_with_is_left_to_the_device(self) -> None:
        """A descendant's fills lengthen it, so only op-get can say."""
        for op in self.family.local_ops:
            for what in ('action-reply-max-len', 'query-reply-min-len',
                         'query-reply-suggested-len', 'event-max-len'):
                with self.subTest(op=op.name, length=what):
                    self.assertNotIn(self.names.user_op_len(op, what),
                                     self.header)

    def test_one_blank_line_however_little_an_op_declares(self) -> None:
        """As checkpatch wants it, for an op without a request or reply too."""
        for text in (self.header, self.source, self.child_header,
                     self.child_source):
            self.assertNotIn('\n\n\n', text)

    def test_every_helper_is_defined_where_its_name_was_taken(self) -> None:
        """The names the symbol table reserves are the names emitted here."""
        symbols = cgen.user_symbols(self.child)
        local = self.child.local_fields
        for field in self.child.fields.values():
            helpers = cgen.user_readers(self.child_names, field) + \
                cgen.user_writers(self.child_names, field)
            for helper in helpers:
                with self.subTest(field=field.full_name, helper=helper):
                    self.assertEqual(field in local, helper in symbols)
                    self.assertEqual(field in local,
                                     helper in self.child_header)

    def test_a_child_reaches_its_parents_binding_through_one_include(
            self) -> None:
        self.assertIn(f'#include {self.names.user_header()}',
                      self.child_header)
        self.assertIn(f'\t.parent = &{self.names.user_spec()},',
                      self.child_source)

    def test_the_root_family_has_no_binding_to_include(self) -> None:
        """Its ops are libctlv's own, so there is nothing above the fixture."""
        self.assertIn('#include <ctlv.h>', self.header)
        self.assertIn(f'#include {self.names.uapi_header()}', self.header)
        self.assertEqual(2, self.header.count('#include'),
                         'libctlv and this family\'s constants, no parent')
        with self.assertRaises(CtlvError):
            gen_user.render_header(support.load(spec.CORE_FAMILY_NAME))

    def test_the_table_describes_every_op_a_device_answers(self) -> None:
        """An inherited op included, because a fill of this family widens it.

        The root's ops are the exception: libctlv carries their one table.
        """
        described = [op.name for op in self.child.ops
                     if self.child.owners[op.family_id] !=
                     spec.CORE_FAMILY_NAME]
        self.assertIn('child-dump', described)
        self.assertIn('records-dump', described)
        offsets = [
            self.child_source.index(
                f'.op_id = {self.child_names.op_id(op)},')
            for op in self.child.ops
            if self.child.owners[op.family_id] != spec.CORE_FAMILY_NAME
        ]
        self.assertEqual(sorted(offsets), offsets)
        self.assertIn(f'\t.ops_count = {len(described)},',
                      self.child_source)
        for op in self.child.ops:
            if self.child.owners[op.family_id] != spec.CORE_FAMILY_NAME:
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(f'.op_id = {self.child_names.op_id(op)},',
                                 self.child_source)

    def test_the_table_carries_fd_access_for_executable_ops(self) -> None:
        for op in self.family.local_ops:
            access = op.fd_access.upper().replace('-', '_')
            field = f'\t\t.fd_access = CTLV_OP_FD_ACCESS_{access},'
            with self.subTest(op=op.name):
                if op.type == 'event':
                    continue
                self.assertIn(field, self.source)

    def test_the_table_marks_only_optional_ops(self) -> None:
        for entry in gen_user._spec_ops(self.family):
            op = entry.op
            body = self.source.split(
                f'\t\t.op_id = {self.names.op_id(op)},\n', 1)[1]
            body = body.split('\t},', 1)[0]
            with self.subTest(op=op.name):
                self.assertEqual(op.optional,
                                 '\t\t.optional = true,' in body)

    def test_a_filled_extension_point_widens_an_inherited_message(
            self) -> None:
        """Which is why a family describes an op its ancestor declared."""
        extension = self.family.fields['records-dump extension']
        empty = flat(f'.type = {self.names.field(extension)}, '
                     '.kind = CTLV_SCHEMA_RECORD, '
                     '.max_occurrences = 1, },')
        self.assertIn(empty, flat(self.source))
        self.assertNotIn(empty, flat(self.child_source))
        for full_name in ('extension-point.tag', 'serial'):
            with self.subTest(field=full_name):
                found = self.child.fields[full_name]
                self.assertIn(
                    flat(f'.type = {self.child_names.field(found)},'),
                    flat(self.child_source))
                self.assertNotIn(self.child_names.field(found), self.source)

    def test_a_re_entering_node_says_how_far_up_its_members_are(self) -> None:
        """The root family's table is the one that has any."""
        root = gen_user.render_source(support.load(spec.CORE_FAMILY_NAME))
        self.assertIn('\t\t.present = CTLV_NODE_SCHEMA_HAS_REENTERS,',
                      root)
        self.assertIn('\t\t.reenters = 1,', root)
        self.assertNotIn('.reenters', self.source)


class TestSpecPath(unittest.TestCase):
    def test_a_path_is_recorded_relative_to_the_kernel_tree(self) -> None:
        self.assertEqual('Documentation/ctlv/specs/ctlv.yaml',
                         cgen.spec_path(support.SPEC_DIR / 'ctlv.yaml'))
        self.assertEqual('tools/ctlv/tests/specs/ctlv-fixture.yaml',
                         cgen.spec_path(support.FIXTURE_SPEC_DIR /
                                        'ctlv-fixture.yaml'))


class TestOutputFile(unittest.TestCase):
    """The file handling that ctlv-regen.sh depends on."""

    CLI = support.TOOL_DIR / 'ctlvgen' / 'cli.py'

    def run_cli(self, *args: str, hash_seed: str | None = None) -> str:
        env = dict(os.environ)
        if hash_seed is not None:
            env['PYTHONHASHSEED'] = hash_seed
        result = subprocess.run([sys.executable, str(self.CLI), *args],
                                capture_output=True, text=True, check=False,
                                env=env)
        self.assertEqual('', result.stderr)
        self.assertEqual(0, result.returncode)
        return result.stdout

    def spec_args(self) -> tuple[str, ...]:
        return ('--spec', str(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml'),
                '--schema', str(support.SCHEMA))

    def test_generating_does_not_depend_on_the_hash_seed(self) -> None:
        for mode, kind, _ in FLAVOURS:
            with self.subTest(mode=mode, kind=kind):
                args = (*self.spec_args(), '--mode', mode, f'--{kind}')
                if mode == 'kernel':
                    args += ('--schema-only',)
                self.assertEqual(self.run_cli(*args, hash_seed='0'),
                                 self.run_cli(*args, hash_seed='12345'))

    def test_kernel_output_combines_only_a_registered_family(self) -> None:
        registered = self.run_cli(
            '--spec', str(support.SPEC_DIR / 'ctlv-test.yaml'),
            '--mode', 'kernel', '--header')
        fixture = self.run_cli(
            *self.spec_args(), '--mode', 'kernel', '--header',
            '--schema-only')

        self.assertIn('ctlv_test_schema', registered)
        self.assertIn('extern const struct ctlv_family ctlv_test_family;',
                      registered)
        self.assertIn('ctlv_fixture_schema', fixture)
        self.assertNotIn('extern const struct ctlv_family '
                         'ctlv_fixture_family;', fixture)

    def test_an_unchanged_output_is_left_alone(self) -> None:
        args = (*self.spec_args(), '--mode', 'kernel', '--source',
                '--schema-only')
        with tempfile.TemporaryDirectory() as tmp:
            path = os.path.join(tmp, 'out.c')
            self.run_cli(*args, '-o', path)
            stamped = os.stat(path).st_mtime_ns
            os.utime(path, ns=(stamped, stamped - 10**9))
            before = os.stat(path).st_mtime_ns
            self.run_cli(*args, '--cmp-out', '-o', path)
            self.assertEqual(before, os.stat(path).st_mtime_ns)
            with open(path, 'w', encoding='utf-8') as changed:
                changed.write('stale\n')
            self.run_cli(*args, '--cmp-out', '-o', path)
            with open(path, encoding='utf-8') as rewritten:
                self.assertIn('CTLV-GEN', rewritten.read())


if __name__ == '__main__':
    unittest.main()
