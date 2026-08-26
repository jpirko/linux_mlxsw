# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the emitted C of every specification in the tree.

The text itself: reproducible, inside the kernel's line limit, naming only the
family's own declarations, with the preamble ctlv-regen.sh needs. What the
numbers mean is KUnit's, and where they come from is test_sizes.py's.
"""

import os
import re
import subprocess
import sys
import tempfile
import unittest

from tests import support

from ctlvgen.lib import cgen
from ctlvgen.lib import gen_family
from ctlvgen.lib import gen_kernel
from ctlvgen.lib import gen_uapi
from ctlvgen.lib import gen_user
from ctlvgen.lib import spec


def kernel_header(family):
    return gen_kernel.render_header(
        family, family_api=family.name in support.FAMILIES)


def kernel_source(family):
    return gen_kernel.render_source(
        family, family_api=family.name in support.FAMILIES)


def schema_header(family):
    return gen_kernel.render_header(family, family_api=False)


def schema_source(family):
    return gen_kernel.render_source(family, family_api=False)


def family_header(family):
    return gen_kernel.render_header(family, family_api=True)


def family_source(family):
    return gen_kernel.render_source(family, family_api=True)


# What each mode and kind emits, and where ctlv-regen.sh writes it.
FLAVOURS = (
    ('uapi', 'header', gen_uapi.render),
    ('kernel', 'header', kernel_header),
    ('kernel', 'source', kernel_source),
    ('user', 'header', gen_user.render_header),
    ('user', 'source', gen_user.render_source),
)

# The marker ctlv-regen.sh recovers the parameters from, and the line above it.
MARKER = re.compile(r'^/\* CTLV-GEN (uapi|kernel|user) '
                    r'(header|source) \*/$', re.M)
SPEC_LINE = re.compile(r'^/\*\t(\S+\.yaml) \*/$', re.M)

GUARD_OPEN = re.compile(r'^#ifndef (\S+)\n#define \1$', re.M)


def flavours(name):
    """What one family emits: for the root, all but a userspace header."""
    return tuple(flavour for flavour in FLAVOURS
                 if name != spec.CORE_FAMILY_NAME or
                 flavour[:2] != ('user', 'header'))


def flat(text):
    """One line, so a test says what it wants and not how it was broken."""
    return re.sub(r'\s+', ' ', text).replace('* ', '*').replace('( ', '(')


def emitted():
    """Every family in the tree, in every flavour, as text."""
    for name in support.ALL_FAMILIES:
        family = support.load_any(name)
        for mode, kind, render in flavours(name):
            yield name, mode, kind, render(family)


class TestText(unittest.TestCase):
    """Rules that hold for every generated file."""

    @classmethod
    def setUpClass(cls):
        cls.files = tuple(emitted())

    def test_the_tree_has_something_to_emit(self):
        self.assertEqual(sum(len(flavours(name))
                             for name in support.ALL_FAMILIES),
                         len(self.files))

    def test_a_file_ends_with_exactly_one_newline(self):
        for name, mode, kind, text in self.files:
            with self.subTest(family=name, mode=mode, kind=kind):
                self.assertTrue(text.endswith('\n'))
                self.assertFalse(text.endswith('\n\n'))

    def test_no_line_is_longer_than_the_kernel_allows(self):
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

    def test_no_line_has_trailing_whitespace(self):
        for name, mode, kind, text in self.files:
            for number, line in enumerate(text.splitlines(), 1):
                with self.subTest(family=name, mode=mode, kind=kind,
                                  line=number):
                    self.assertEqual(line.rstrip(), line)

    def test_the_license_is_the_specifications_own(self):
        for name in support.ALL_FAMILIES:
            family = support.load_any(name)
            for mode, kind, render in flavours(name):
                with self.subTest(family=name, mode=mode, kind=kind):
                    first = render(family).splitlines()[0]
                    comment = '//' if kind == 'source' else '/*'
                    self.assertTrue(first.startswith(comment), first)
                    self.assertIn(f'SPDX-License-Identifier: {family.license}',
                                  first)

    def test_the_preamble_says_what_regen_needs(self):
        for name, mode, kind, text in self.files:
            with self.subTest(family=name, mode=mode, kind=kind):
                marker = MARKER.search(text)
                self.assertIsNotNone(marker)
                self.assertEqual((mode, kind), marker.groups())
                spec_line = SPEC_LINE.search(text)
                self.assertIsNotNone(spec_line)
                path = support.KERNEL_DIR / spec_line.group(1)
                self.assertTrue(path.exists(), spec_line.group(1))

    def test_a_header_guards_itself(self):
        for name, mode, kind, text in self.files:
            if kind != 'header':
                continue
            with self.subTest(family=name, mode=mode):
                guard = GUARD_OPEN.search(text)
                self.assertIsNotNone(guard)
                self.assertIn(f'#endif /* {guard.group(1)} */', text)
                self.assertEqual(1, text.count(f'#define {guard.group(1)}'))


class TestUapiHeader(unittest.TestCase):
    def setUp(self):
        self.family = support.load_family('ctlv-fixture-child')
        self.text = gen_uapi.render(self.family)
        self.names = cgen.Names(self.family)

    def test_every_own_declaration_is_defined_once(self):
        for attr in self.family.attrs.values():
            if attr.family_id != self.family.family_id:
                continue
            with self.subTest(attr=attr.full_name):
                self.assertEqual(1, self.text.count(
                    f'#define {self.names.attr(attr)} '))

    def test_an_inherited_declaration_is_not_redefined(self):
        parent = support.load_family('ctlv-fixture')
        for attr in parent.attrs.values():
            with self.subTest(attr=attr.full_name):
                self.assertNotIn(f'#define {self.names.attr(attr)} ',
                                 self.text)

    def test_an_id_is_composed_of_the_parts_it_names(self):
        attr = self.family.attrs['child.tag']
        group = self.family.groups[attr.group]
        text = flat(self.text.replace('\\\n', ''))

        self.assertIn(
            flat(f'#define {self.names.group_attr(group)}(_attr) '
                 f'{self.names.family_attr(group)}('
                 f'{self.names.group(group)}, '
                 f'{self.names.local_attr_prefix(group)}##_attr)'),
            text)
        self.assertIn(
            flat(f'#define {self.names.family_attr(group)}(_group, _attr) '
                 f'CTLV_ATTR({self.names.family_const()}, '
                 f'(_group), (_attr))'),
            text)
        self.assertIn(
            flat(f'#define {self.names.attr(attr)} '
                 f'{self.names.group_attr(group)}('
                 f'{cgen.macro(attr.name)})'),
            text)
        self.assertLess(
            self.text.index(f'#define {self.names.group_attr(group)}'),
            self.text.index(f'enum {self.names.group_attr_enum(group)}'))

    def test_group_and_attr_numbers_are_sequential_enums(self):
        groups = list(self.family.local_groups)
        group = groups[0]

        self.assertIn(f'enum {self.names.group_enum(group)} {{', self.text)
        self.assertIn(f'\t{self.names.group(group)} = 1,', self.text)
        self.assertIn(f'enum {self.names.group_attr_enum(group)} {{',
                      self.text)
        for index, attr in enumerate(group.attrs):
            assignment = ' = 1' if not index else ''
            self.assertIn(f'\t{self.names.local_attr(attr)}{assignment},',
                          self.text)

    def test_family_set_and_operation_numbers_are_enums(self):
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        paths = spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                           schema=support.SCHEMA)
        registry = spec.load_registry(paths)
        families = gen_uapi.render_families(registry)
        flat_text = flat(text.replace('\\\n', ''))

        self.assertNotIn('enum _ctlv_family {', text)
        self.assertIn('enum _ctlv_family {', families)
        self.assertIn('\tCTLV_FAMILY_ROOT,', families)
        self.assertIn(f'\t{names.family_const()},', families)
        for line in families.splitlines():
            self.assertLessEqual(cgen.width(line), cgen.LINE_LIMIT, line)
        sets = list(family.local_sets)
        self.assertIn(f'enum {names.set_enum(sets[0])} {{', text)
        for index, attr_set in enumerate(sets):
            assignment = f' = {attr_set.set_id}' if not index else ''
            self.assertIn(f'\t{names.attr_set(attr_set)}{assignment},',
                          text)
        ops = list(family.local_ops)
        self.assertIn(f'enum {names.op_enum(ops[0])} {{',
                      text)
        for index, op in enumerate(ops):
            assignment = f' = {op.local_id}' if not index else ''
            self.assertIn(f'\t{names.op(op)}{assignment},',
                          text)
            self.assertIn(
                flat(f'#define {names.op_id(op)} '
                     f'{names.operation(op)}({cgen.macro(op.name)})'),
                flat_text)
        self.assertIn(
            flat(f'#define {names.operation(ops[0])}(_op) '
                 f'CTLV_OP({names.family_const()}, '
                 f'{names.op_prefix(ops[0])}##_op)'),
            flat_text)

    def test_only_the_first_value_of_an_enum_is_numbered(self):
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)

        for definition in family.local_definitions:
            if definition.kind != 'enum':
                continue
            for index, (value_name, number) in enumerate(definition.values):
                assignment = f' = {number}' if not index else ''
                self.assertIn(
                    f'\t{names.value(definition, value_name)}{assignment},',
                    text)

    def test_a_flags_mask_names_its_bits(self):
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
            for index, (value_name, bit) in enumerate(definition.values):
                assignment = f' = {bit}' if not index and bit else ''
                bit_name = names.value_bit(definition, value_name)
                self.assertIn(
                    f'\t{bit_name}{assignment},', text)
                value = names.value(definition, value_name)
                self.assertIn(value, mask)
                self.assertIn(
                    flat(f'#define {value} _BITULL({bit_name})'),
                    flat_text)
            self.assertNotRegex(mask, r'0x[0-9a-f]+ULL')

    def test_attr_unit_metadata_is_an_enum(self):
        family = support.load_family('ctlv-fixture')
        names = cgen.Names(family)
        text = gen_uapi.render(family)
        group = family.groups['scalar']
        attrs = [attr for attr in group.attrs if attr.decl.unit is not None]

        self.assertIn(f'enum {names.group_metadata_enum(group)} {{', text)
        for attr in attrs:
            self.assertIn(f'\t{names.attr_unit(attr)} = CTLV_UNIT_',
                          text)
            self.assertIn(f'\t{names.attr_scale(attr)} = '
                          f'{attr.decl.scale},', text)


class TestKernelSource(unittest.TestCase):
    def setUp(self):
        self.family = support.load_family('ctlv-fixture')
        self.text = schema_source(self.family)
        self.names = cgen.Names(self.family)

    def trees(self):
        for op in self.family.local_ops:
            for which in ('request', 'reply'):
                root = getattr(op, which)
                if root is not None:
                    yield op, which, root

    def test_every_tree_becomes_one_node_schema_array(self):
        for op, which, root in self.trees():
            symbol = self.names.node_schemas(op, which)
            with self.subTest(op=op.name, which=which):
                self.assertIn(f'static const struct ctlv_node_schema '
                              f'{symbol}[] = {{', self.text)
                stub_schemas = gen_kernel.stub_schemas(root)
                tree_macro = 'CTLV_SCHEMA_TREE_STUBS' if stub_schemas else \
                    'CTLV_SCHEMA_TREE'
                self.assertIn(
                    f'{tree_macro}({cgen.ident(op.name, which)})',
                    self.text)
                if not stub_schemas:
                    continue
                symbol = self.names.stub_schemas(op, which)
                self.assertIn(f'static const struct ctlv_stub_schema '
                              f'{symbol}[] = {{', self.text)

    def test_a_node_schema_array_has_one_entry_per_node(self):
        for op, which, root in self.trees():
            symbol = self.names.node_schemas(op, which)
            body = self.text.split(f'{symbol}[] = {{\n')[1].split('};')[0]
            initializers = re.findall(
                r'^\tCTLV_NODE_SCHEMA_[A-Z_]+\(', body, re.M)
            with self.subTest(op=op.name, which=which):
                self.assertEqual(len(gen_kernel.flatten(root)),
                                 len(initializers))

    def test_a_child_index_follows_its_parent(self):
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

    def test_the_descriptor_counts_the_unsized_table(self):
        symbol = self.names.op_schemas()

        self.assertIn(f'const struct ctlv_op_schema {symbol}[] = {{',
                      self.text)
        self.assertIn(f'\t.ops_count = ARRAY_SIZE({symbol}),', self.text)
        self.assertNotIn(f'static_assert(ARRAY_SIZE({symbol})', self.text)

    def test_every_op_states_its_availability(self):
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

    def test_every_executable_op_states_its_fd_access(self):
        executable = [op for op in self.family.local_ops
                      if op.type != 'event']
        for op in executable:
            with self.subTest(op=op.name):
                access = op.fd_access.upper().replace('-', '_')
                self.assertIn(f'CTLV_OP_FD_ACCESS_{access}', self.text)
        self.assertEqual(len(executable),
                         self.text.count('CTLV_OP_FD_ACCESS_'))

    def test_only_an_op_that_asked_for_a_hook_pair_is_marked(self):
        """The one field here describing an implementation, not a message."""
        marked = [op for op in self.family.local_ops if op.pre_post]
        self.assertEqual(['counts-get'], [op.name for op in marked])
        self.assertEqual(len(marked),
                         self.text.count('.pre_post = true'))
        self.assertNotIn('.pre_post = false', self.text)

    def test_a_device_bit_is_not_numbered_here(self):
        """Which bit an op owns depends on the family, not the declaration."""
        self.assertNotIn('state_index', self.text)
        self.assertNotIn('state_index', schema_header(self.family))

    def test_the_schema_has_no_ancestry_or_export(self):
        self.assertIn(f'const struct ctlv_family_schema '
                      f'{self.names.schema()} = {{', self.text)
        self.assertNotIn('\t.parent =', self.text)
        self.assertNotIn(
            f'EXPORT_SYMBOL_NS_GPL({self.names.schema()}, "CTLV");',
            self.text)

    def test_every_schema_header_stands_alone(self):
        for name in support.FIXTURE_FAMILIES:
            header = schema_header(support.load_family(name))

            self.assertEqual(1, header.count('#include'))
            self.assertIn('#include <linux/ctlv_family.h>', header)

    def test_the_wire_layout_guards_are_not_repeated(self):
        self.assertNotIn('static_assert(sizeof(struct ctlv_attr)', self.text)
        self.assertNotIn('static_assert(CTLV_ALIGNTO', self.text)
        self.assertNotIn('static_assert(sizeof(struct ctlv_event_hdr)',
                         self.text)

    def test_the_limits_are_asserted(self):
        self.assertIn('<= CTLV_MAX_INLINE_MESSAGE_LEN);', self.text)
        self.assertIn('<= CTLV_MAX_EVENT_QUEUE_LEN);', self.text)
        self.assertIn('<= CTLV_MAX_DECLARED_NEST_DEPTH);', self.text)

    def test_what_the_chain_costs_a_uevent_is_asserted(self):
        """The names a device announces, charged as an environment charges."""
        variables = ['CTLV_FAMILY=ctlv-fixture', 'CTLV_COMPATIBLE_0=ctlv',
                     'CTLV_COMPATIBLE_1=ctlv-fixture',
                     'CTLV_COMPATIBLE_N=2']
        total = sum(len(item) + 1 for item in variables)
        self.assertIn(f'static_assert({total} <= CTLV_UEVENT_MAX_LEN);',
                      self.text)
        self.assertIn(f'static_assert({len(variables)} '
                      '<= CTLV_UEVENT_MAX_VARS);', self.text)

    def test_the_root_family_fits_its_one_level_chain(self):
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

    @classmethod
    def setUpClass(cls):
        cls.family = support.load(spec.CORE_FAMILY_NAME)
        cls.flat = gen_kernel.flatten(
            support.op(cls.family, 'op-schema-get').reply)

    def test_a_re_entry_points_at_the_level_it_is_part_of(self):
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

    def test_the_tree_holds_one_level_per_message_and_no_more(self):
        """Which is what keeps a schema of any depth a finite tree."""
        attrs = [flat.node.attr.full_name for flat in self.flat
                 if not flat.node.is_root]
        self.assertEqual(2, attrs.count('schema.attrs'))
        self.assertEqual(2, attrs.count('schema.optional-attrs'))
        self.assertEqual(37, len(self.flat))

    def test_the_generated_node_marks_reentry(self):
        text = schema_source(self.family)
        self.assertEqual(4, text.count('CTLV_NODE_SCHEMA_NEST_REENTERS('))


class TestCoreMessages(unittest.TestCase):
    """The messages the core builds where no op declared one.

    Error information is the whole of it, so it belongs to the root family.
    """

    def setUp(self):
        self.family = support.load(spec.CORE_FAMILY_NAME)
        self.names = cgen.Names(self.family)
        self.source = schema_source(self.family)
        self.header = schema_header(self.family)
        self.message = self.family.messages[0]

    def test_the_root_family_declares_the_error_information(self):
        self.assertEqual(['error-info'],
                         [message.name for message in self.family.messages])

    def test_the_message_becomes_a_tree_of_its_own(self):
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

    def test_the_length_is_stated_and_held_to_the_transports_limit(self):
        length = self.names.message_max_len(self.message)
        self.assertIn(f'#define {length} {self.message.max_len}', self.header)
        self.assertIn(f'static_assert({length} <= CTLV_MAX_ERROR_LEN);',
                      self.source)

    def test_the_core_is_the_only_builder(self):
        """A chain has one error message, the one its root declared."""
        for name in ('ctlv-fixture', 'ctlv-fixture-child'):
            family = support.load_family(name)
            with self.subTest(family=name):
                self.assertEqual((), family.messages)
                self.assertNotIn('error_info',
                                 schema_source(family))


class TestFamilyDescriptor(unittest.TestCase):
    """The hooks a family has to implement, and where they may appear.

    The inheriting fixture is the subject of the family-wide pair, which every
    family implements. The pair of one op is the subject of the whole chain:
    the marked op is the parent's, so only the parent implements its pair.
    """

    def setUp(self):
        self.family = support.load_family('ctlv-fixture-child')
        self.header = family_header(self.family)
        self.source = family_source(self.family)
        self.names = cgen.Names(self.family)

    def test_only_callbacks_carry_the_op_type_suffix(self):
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

    def test_lifecycle_names_need_no_device_infix(self):
        self.assertEqual('ctlv_fixture_child_init',
                         self.names.lifecycle_hook('init'))

    def test_the_family_pair_is_generated_whatever_the_ops_asked_for(self):
        self.assertIn('Family-wide hooks around every effective non-core op.',
                      self.header)
        for which, returns in (('pre', 'int'), ('post', 'void')):
            symbol = self.names.family_hook(which)
            with self.subTest(which=which):
                self.assertIn(f'{returns} {symbol}(void *priv, '
                              'struct ctlv_op_ctx *ctx);',
                              flat(self.header))
                self.assertIn(f'\t.{which} = {symbol},', self.source)

    def test_only_an_op_that_asked_for_a_pair_gets_one(self):
        """And only in the family that declared that op."""
        self.assertEqual(['counts-get'],
                         [op.name for op in self.family.ops if op.pre_post])
        parent = family_header(support.load_family('ctlv-fixture'))
        self.assertIn('Per-op hooks nested inside the family-wide pair.',
                      parent)
        for name in support.FIXTURE_FAMILIES:
            family = support.load_family(name)
            header = family_header(family)
            source = family_source(family)
            names = cgen.Names(family)
            for op in family.ops:
                for which in ('pre', 'post'):
                    symbol = names.op_hook(op, which)
                    with self.subTest(family=name, op=op.name, which=which):
                        declaration = (f'{symbol}(void *priv, '
                                       'struct ctlv_op_ctx *ctx);')
                        binding = (
                            f'\tCTLV_OP_{op.type.upper()}_PRE_POST('
                            f'{names.op_ident(op)}),')
                        if op.pre_post and op in family.local_ops:
                            self.assertIn(declaration, flat(header))
                            self.assertIn(binding, source)
                        else:
                            # The declaration rather than the name: a getter
                            # of the op's own attr holds _pre inside _present.
                            self.assertNotIn(declaration, flat(header))
                            self.assertNotIn(binding, source)

    def test_a_hook_reaches_no_constant_a_caller_compiles_against(self):
        """A pair is who implements an op, not what it looks like."""
        wire = gen_uapi.render(self.family)
        metadata = schema_header(self.family)
        metadata += schema_source(self.family)
        symbols = [self.names.family_hook(which) for which in ('pre', 'post')]
        symbols += [self.names.op_hook(op, which) for op in self.family.ops
                    for which in ('pre', 'post')]
        for symbol in symbols:
            with self.subTest(symbol=symbol):
                self.assertNotIn(symbol, wire + metadata)
        self.assertNotIn('pre_post', wire)
        self.assertNotIn('PRE_POST', wire)


class TestRootDescriptor(unittest.TestCase):
    """The root descriptor, emitted like every other family's."""

    def setUp(self):
        self.family = support.load(spec.CORE_FAMILY_NAME)
        self.header = family_header(self.family)
        self.source = family_source(self.family)
        self.names = cgen.Names(self.family)

    def test_the_root_binds_its_own_action_and_query_ops(self):
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

    def test_the_root_implements_the_pair_every_family_implements(self):
        self.assertIn('Root-family hooks around every core action or query.',
                      self.header)
        for which, returns in (('pre', 'int'), ('post', 'void')):
            symbol = self.names.family_hook(which)
            with self.subTest(which=which):
                self.assertIn(f'{returns} {symbol}(void *priv, '
                              'struct ctlv_op_ctx *ctx);',
                              flat(self.header))
                self.assertIn(f'\t.{which} = {symbol},', self.source)

    def test_the_root_descriptor_is_exported_for_direct_children(self):
        self.assertIn(
            f'EXPORT_SYMBOL_NS_GPL({self.names.descriptor()}, "CTLV");',
            self.source)

        direct = support.load_family('ctlv-fixture')
        source = family_source(direct)
        self.assertIn('\t.parent = &ctlv_family,', source)

    def test_the_tables_of_a_chain_partition_its_ops(self):
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

    def setUp(self):
        self.family = support.load_any('ctlv-fixture')
        self.header = family_header(self.family)
        self.names = cgen.Names(self.family)

    def test_an_event_gets_a_creator_naming_its_op(self):
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

    def test_an_action_or_query_has_no_creator(self):
        for op in self.family.local_ops:
            if op.type == 'event':
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(self.names.op_helper(op, 'create'),
                                 self.header)

    def test_an_inherited_event_keeps_its_own_familys_creator(self):
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

    def setUp(self):
        self.family = support.load_family('ctlv-fixture-child')
        self.parent = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.parent_names = cgen.Names(self.parent)
        self.uapi = gen_uapi.render(self.family)
        self.header = schema_header(self.family)
        self.source = schema_source(self.family)
        self.family_header = family_header(self.family)
        self.family_source = family_source(self.family)

    def test_only_the_childs_own_ops_are_described(self):
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

    def test_an_inherited_length_is_not_repeated(self):
        """A length belongs to the family that described the op."""
        for op in self.family.ops:
            if op in self.family.local_ops:
                continue
            with self.subTest(op=op.name):
                self.assertNotIn(self.names.op_id(op), self.source)

    def test_the_schema_carries_no_parent_link(self):
        self.assertNotIn('\t.parent =', self.source)
        self.assertNotIn(self.parent_names.schema(), self.source)
        self.assertNotIn(self.parent_names.kernel_header(), self.header)

    def test_the_whole_chain_is_what_a_device_announces(self):
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

    def test_a_filled_extension_point_is_the_childs_own_metadata(self):
        fill = self.family.fills[0]
        self.assertEqual(1, len(self.family.fills))
        stub_fill_schemas = self.names.stub_fill_schemas()
        owner = cgen.family_const(self.family.owners[fill.family_id])
        attr_set = self.names.attr_set(self.family.sets[fill.name])
        initializer = (
            f'CTLV_STUB_FILL_SCHEMA({owner}, {attr_set}, '
            f'{cgen.ident(fill.name)})'
        )
        self.assertIn(f'static const struct ctlv_stub_fill_schema '
                      f'{stub_fill_schemas}[] = {{', self.source)
        self.assertIn(initializer, flat(self.source))
        self.assertIn(f'.stub_fill_schemas = {stub_fill_schemas},',
                      flat(self.source))
        self.assertIn(f'.stub_fill_schemas_count = '
                      f'ARRAY_SIZE({stub_fill_schemas}),',
                      flat(self.source))
        # The nest a fill joins keeps the ID the declaring family gave it.
        self.assertNotIn('nest_extension', self.source)

    def test_a_parent_header_is_included_rather_than_repeated(self):
        self.assertIn(f'#include {cgen.uapi_header(self.parent.name)}',
                      self.uapi)
        self.assertIn(f'#include {self.parent_names.kernel_header()}',
                      self.family_header)

    def test_a_child_binds_its_own_ops_and_inherits_the_rest(self):
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

    def test_what_a_child_does_with_the_extension_point_it_fills(self):
        """One pair per fill, in the order the metadata has them."""
        fill = self.family.fills[0]
        self.assertEqual(1, len(self.family.fills))
        for which, returns in (('fill', 'int'), ('read', 'void')):
            symbol = self.names.stub_callback(fill, which)
            with self.subTest(which=which):
                self.assertIn(f'{returns} {symbol}(', self.family_header)
                self.assertIn(f'{symbol}(void *priv, ',
                              flat(self.family_header))
        stub_ops = self.names.stub_ops()
        owner = cgen.family_const(self.family.owners[fill.family_id])
        attr_set = self.names.attr_set(self.family.sets[fill.name])
        initializer = (
            f'CTLV_STUB_OPS({owner}, {attr_set}, '
            f'{self.names.stub_ident(fill)})'
        )
        self.assertIn(f'static const struct ctlv_stub_ops '
                      f'{stub_ops}[] = {{', self.family_source)
        self.assertIn(initializer, flat(self.family_source))
        self.assertIn(f'.stub_ops = {stub_ops},',
                      flat(self.family_source))
        self.assertIn(f'.stub_ops_count = ARRAY_SIZE({stub_ops}),',
                      flat(self.family_source))

    def test_every_family_gets_its_declared_priv_lifecycle(self):
        for name in support.ALL_FAMILIES:
            family = support.load_any(name)
            header = family_header(family)
            source = family_source(family)
            names = cgen.Names(family)
            lifecycle = (
                ('priv_size', 'size_t'),
                ('pre_init', 'const void'),
                ('init', 'int'),
                ('post_init', 'int'),
                ('pre_fini', 'void'),
                ('fini', 'void'),
            )

            for which, returns in lifecycle:
                symbol = names.lifecycle_hook(which)
                optional = which in ('pre_init', 'post_init', 'pre_fini')
                expected = not optional or getattr(family, which)
                with self.subTest(family=name, which=which):
                    declaration = ('const void *' if which == 'pre_init'
                                   else f'{returns} ')
                    if not expected:
                        self.assertNotIn(f'{declaration}{symbol}(', header)
                        self.assertNotIn(f'\t.{which} = {symbol},', source)
                    elif which == 'pre_init':
                        self.assertIn(
                            f'{symbol}(const void *init_info)', flat(header))
                    if expected:
                        self.assertIn(f'{declaration}{symbol}(', header)
                        self.assertIn(f'\t.{which} = {symbol},', source)

    def test_a_helper_is_generated_by_the_family_that_declared_the_op(self):
        """What a device may decide about one op, for this family's own ops.

        Only an optional action or query may be opted in to, only a dynamic
        one may be enabled and disabled, and an event is neither.
        """
        for op in self.family.ops:
            expected = ()
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

    The fixture declares one attr of every primary form, at every cardinality.
    """

    def setUp(self):
        self.family = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.header = family_header(self.family)
        self.templates = (support.KERNEL_DIR / 'include' / 'linux' /
                          'ctlv_family.h').read_text()
        self.scopes = gen_kernel._reader_scopes(self.family, self.names)
        self.attrs = {attr.full_name: attr
                      for group in self.family.local_groups
                      for attr in group.attrs}

    def scope(self, *name):
        return next(scope for scope in self.scopes if scope.name == name)

    def placement(self, scope, full_name):
        for slot, child in enumerate(scope.node.children()):
            if child.attr.full_name == full_name:
                return child.attr, slot
        raise KeyError((scope.name, full_name))

    def reference(self, scope, full_name):
        attr, _slot = self.placement(scope, full_name)
        return self.names.input_ref(scope, attr)

    def base(self, scope, full_name):
        return self.reference(scope, full_name).removesuffix('_ref')

    def test_a_form_is_read_by_the_reader_of_that_form(self):
        """One C type per primary form, and the kind it is held to."""
        scope = self.scope('scalars-set')
        forms = {
            'scalar.u': 'U64',
            'scalar.s': 'S64',
            'scalar.mode': 'U64',
            'scalar.quirks': 'U64',
            'scalar.flag': 'BOOL',
            'scalar.text': 'STRING',
            'scalar.fixed': 'BINARY',
        }
        for full_name, form in forms.items():
            attr, slot = self.placement(scope, full_name)
            name = self.base(scope, full_name)
            with self.subTest(attr=full_name):
                self.assertIn(
                    flat(f'CTLV_IN_REF({name}, {scope.tree}, '
                         f'{self.names.attr(attr)}, {scope.node_index}, '
                         f'{slot}, {cgen.KINDS[attr.decl.kind]})'),
                    flat(self.header))
                self.assertIn(
                    f'CTLV_IN_HAS({name})', self.header)
                self.assertIn(
                    f'CTLV_IN_{form}({name})', self.header)

    def test_presence_is_what_an_optional_attr_is_asked_about(self):
        scope = self.scope('collapse-check')
        name = self.base(scope, 'scalar.u')

        self.assertIn(f'CTLV_IN_HAS({name})', self.header)
        self.assertIn('return ctlv_attr_ok(&attr);', self.templates)

    def test_a_repeated_attr_is_read_through_its_iterator_alone(self):
        """No presence helper: an absent one is an iteration of nothing.

        The getter reads the occurrence the iterator left the cursor on, so it
        takes that cursor rather than the nest.
        """
        scope = self.scope('counts-get')
        attr, _slot = self.placement(scope, 'count.few')
        iterator = self.names.input_iterator(scope, attr)
        name = self.names.input_name(scope, attr)
        self.assertIn(
            flat(f'#define {iterator}(_attr, _nest) \\\n'
                 f'CTLV_IN_FOR_EACH({name}, _attr, _nest)'),
            flat(self.header))
        self.assertIn('#define CTLV_IN_FOR_EACH', self.templates)
        self.assertIn('for (struct ctlv_cursor *__ctlv_attr =',
                      self.templates)
        self.assertIn('ctlv_input_attr(_attr, _nest, _name##_ref());',
                      self.templates)
        self.assertIn('ctlv_attr_ok(__ctlv_attr);', self.templates)
        self.assertIn('ctlv_attr_next(__ctlv_attr))', self.templates)
        name = self.base(scope, 'count.few')
        self.assertIn(f'CTLV_IN_EACH_U64({name})',
                      self.header)
        self.assertIn('return ctlv_attr_u64(attr);', self.templates)

    def test_a_repeated_binary_failure_clears_its_length(self):
        macro = '#define CTLV_IN_EACH_BINARY'

        self.assertIn(macro, self.templates)
        self.assertIn('*len = 0;', self.templates)
        self.assertIn('return NULL;', self.templates)

    def test_a_nest_is_stood_on_rather_than_valued(self):
        """What a nest holds is read by reading its members."""
        scope = self.scope('counts-get')
        attr, _slot = self.placement(scope, 'nest.request')
        self.assertIn(self.names.input_iterator(scope, attr), self.header)
        readers = cgen.input_readers(self.names, scope, attr)
        self.assertNotIn(self.names.input_helper(scope, attr, 'get'), readers)

    def test_a_blob_is_resolved_rather_than_valued(self):
        """A resolver hands back the memory the descriptor names.

        It needs the request being executed for that, so it is the one reader
        that takes the context.
        """
        scope = self.scope('blobs-submit')
        for full_name in ('region.input-va', 'region.output-fd'):
            name = self.base(scope, full_name)
            with self.subTest(attr=full_name):
                self.assertIn(f'CTLV_IN_HAS({name})',
                              self.header)
                self.assertIn(f'CTLV_IN_BLOB({name})',
                              self.header)
        self.assertIn('return ctlv_blob_resolve(ctx, &attr);',
                      self.templates)

    def test_a_repeated_blob_is_resolved_where_its_iterator_stopped(self):
        """The cursor of an occurrence is the descriptor to resolve."""
        scope = self.scope('blobs-submit')
        attr, _slot = self.placement(scope, 'region.pieces')
        self.assertIn(self.names.input_iterator(scope, attr), self.header)
        name = self.base(scope, 'region.pieces')
        self.assertIn(
            f'CTLV_IN_EACH_BLOB({name})',
            self.header)
        self.assertIn('return ctlv_blob_resolve(ctx, attr);',
                      self.templates)

    def test_every_reader_is_declared_where_its_name_was_taken(self):
        """The names the symbol table reserves are the names emitted here."""
        child = support.load_family('ctlv-fixture-child')
        names = cgen.Names(child)
        header = family_header(child)
        header_flat = flat(header)
        scopes = gen_kernel._reader_scopes(child, names)

        for scope in scopes:
            for member in scope.node.children():
                attr = member.attr
                name = names.input_ref(scope, attr).removesuffix('_ref')
                repeated = attr.decl.max_occurrences != 1
                with self.subTest(scope=scope.name,
                                  attr=attr.full_name):
                    self.assertIn(flat(f'CTLV_IN_REF({name},'),
                                  header_flat)
                    if repeated:
                        self.assertIn(names.input_iterator(scope, attr),
                                      header)
                    else:
                        self.assertIn(
                            flat(f'CTLV_IN_HAS({name})'),
                            header_flat)
                    if attr.decl.kind == 'blob':
                        self.assertIn(
                            flat(f'CTLV_IN_'
                                 f'{"EACH_" if repeated else ""}'
                                 f'BLOB({name})'),
                            header_flat)
                    elif cgen.attr_getter_value(attr):
                        form = gen_family.INPUT_MACROS[attr.decl.kind]
                        self.assertIn(
                            flat(f'CTLV_IN_'
                                 f'{"EACH_" if repeated else ""}'
                                 f'{form}({name})'),
                            header_flat)


class TestWriters(unittest.TestCase):
    """What a callback writes a reply with, on the same fixture family."""

    def setUp(self):
        self.family = support.load_family('ctlv-fixture')
        self.names = cgen.Names(self.family)
        self.header = family_header(self.family)
        self.attrs = {attr.full_name: attr
                      for group in self.family.local_groups
                      for attr in group.attrs}

    def writer(self, full_name, what='put'):
        return self.names.attr_helper(self.attrs[full_name], what)

    def base(self, full_name, what='put'):
        return self.writer(full_name, what).removesuffix(f'_{what}')

    def test_a_form_is_written_by_the_serializer_of_that_form(self):
        """The mirror of the getters: one C type per primary form."""
        forms = {
            'scalar.u': ('U64', 'CTLV_SCHEMA_UINT'),
            'scalar.s': ('S64', 'CTLV_SCHEMA_SINT'),
            'scalar.mode': ('U64', 'CTLV_SCHEMA_ENUM'),
            'scalar.quirks': ('U64', 'CTLV_SCHEMA_FLAGS'),
            'scalar.flag': ('BOOL', 'CTLV_SCHEMA_BOOL'),
            'scalar.text': ('STRING', 'CTLV_SCHEMA_STRING'),
            'scalar.fixed': ('BINARY', 'CTLV_SCHEMA_BINARY'),
        }
        for full_name, (form, kind) in forms.items():
            attr = self.attrs[full_name]
            with self.subTest(attr=full_name):
                self.assertIn(
                    flat(f'CTLV_PUT_{form}({self.base(full_name)}, '
                         f'{self.names.attr(attr)}, {kind})'),
                    flat(self.header))

    def test_a_nest_is_opened_onto_a_cursor_of_the_callers_own(self):
        attr = self.attrs['nest.extension']
        self.assertIn(
            flat(f'CTLV_NEST_START('
                 f'{self.base("nest.extension", "start")}, '
                 f'{self.names.attr(attr)}, CTLV_SCHEMA_NEST)'),
            flat(self.header))
        self.assertNotIn(self.writer('nest.extension'), self.header)

    def test_a_repeated_attr_is_appended_one_occurrence_at_a_time(self):
        """No iterator on this side: another occurrence is another call."""
        for full_name in ('count.few', 'count.many'):
            attr = self.attrs[full_name]
            with self.subTest(attr=full_name):
                self.assertIn(
                    flat(f'CTLV_PUT_U64({self.base(full_name)}, '
                         f'{self.names.attr(attr)}, CTLV_SCHEMA_UINT)'),
                    flat(self.header))

    def test_a_blob_is_not_written_at_all(self):
        """A blob describes a caller's memory, so a reply never holds one."""
        for full_name in ('region.input-va', 'region.output-fd'):
            attr = self.attrs[full_name]
            with self.subTest(attr=full_name):
                self.assertEqual((), cgen.attr_writers(self.names, attr))

    def test_every_serializer_is_declared_where_its_name_was_taken(self):
        child = support.load_family('ctlv-fixture-child')
        names = cgen.Names(child)
        header = family_header(child)
        for group in child.groups.values():
            for attr in group.attrs:
                local = attr in [own for own_group in child.local_groups
                                 for own in own_group.attrs]
                for writer in cgen.attr_writers(names, attr):
                    with self.subTest(attr=attr.full_name, writer=writer):
                        what = 'start' if attr.decl.kind == 'nested' else 'put'
                        base = writer.removesuffix(f'_{what}')
                        if attr.decl.kind == 'nested':
                            macro = 'CTLV_NEST_START'
                        else:
                            macro = ('CTLV_PUT_' +
                                     gen_family.PUT_MACROS[attr.decl.kind])
                        self.assertEqual(
                            local, flat(f'{macro}({base},') in flat(header))


class TestUserBinding(unittest.TestCase):
    """What an application gets, on the fixture family and its child.

    The header is names, and the source is the specification as data.
    """

    def setUp(self):
        self.family = support.load_family('ctlv-fixture')
        self.child = support.load_family('ctlv-fixture-child')
        self.names = cgen.Names(self.family)
        self.child_names = cgen.Names(self.child)
        self.header = gen_user.render_header(self.family)
        self.source = gen_user.render_source(self.family)
        self.child_header = gen_user.render_header(self.child)
        self.child_source = gen_user.render_source(self.child)

    def test_an_action_and_a_query_are_one_call_each(self):
        """And the buffer a reply needs comes from the device.

        An inheriting family's device answers with more than this
        specification computed, so only an op that answers nothing may state a
        length here.
        """
        for op in self.family.local_ops:
            if op.type == 'event':
                continue
            with self.subTest(op=op.name):
                if op.type == 'query':
                    call = 'ctlv_query'
                elif op.reply is not None:
                    call = 'ctlv_action'
                else:
                    call = 'ctlv_action_sized'
                self.assertIn(flat(f'static inline int '
                                   f'{self.names.user_op(op)}('
                                   'struct ctlv_dev *dev'), flat(self.header))
                self.assertIn(f'return {call}(dev, {self.names.op_id(op)},',
                              flat(self.header))

    def test_an_event_is_subscribed_to_rather_than_executed(self):
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

    def test_a_request_starts_at_this_family_and_is_not_capped_there(self):
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

    def test_a_length_this_family_computed_is_published(self):
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

    def test_every_helper_is_defined_where_its_name_was_taken(self):
        """The names the symbol table reserves are the names emitted here."""
        local = [attr for group in self.child.local_groups
                 for attr in group.attrs]
        for group in self.child.groups.values():
            for attr in group.attrs:
                helpers = cgen.user_attr_readers(self.child_names, attr) + \
                    cgen.user_attr_writers(self.child_names, attr)
                for helper in helpers:
                    with self.subTest(attr=attr.full_name, helper=helper):
                        self.assertEqual(attr in local,
                                         helper in self.child_header)

    def test_a_child_reaches_its_parents_binding_through_one_include(self):
        self.assertIn(f'#include {self.names.user_header()}',
                      self.child_header)
        self.assertIn(f'\t.parent = &{self.names.user_spec()},',
                      self.child_source)

    def test_the_root_family_has_no_binding_to_include(self):
        """Its ops are libctlv's own, so there is nothing above the fixture."""
        self.assertIn('#include <ctlv.h>', self.header)
        self.assertIn(f'#include {self.names.uapi_header()}', self.header)
        self.assertEqual(2, self.header.count('#include'),
                         'libctlv and this family\'s constants, no parent')
        with self.assertRaises(cgen.CtlvError):
            gen_user.render_header(support.load(spec.CORE_FAMILY_NAME))

    def test_the_table_describes_every_op_a_device_answers(self):
        """An inherited op included, because a fill of this family widens it.

        The root's ops are the exception: libctlv carries their one table.
        """
        described = [op.name for op in self.child.ops
                     if self.child.owners[op.family_id] !=
                     spec.CORE_FAMILY_NAME]
        self.assertIn('child-dump', described)
        self.assertIn('nests-dump', described)
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

    def test_the_table_carries_fd_access_for_executable_ops(self):
        for op in self.family.local_ops:
            access = op.fd_access.upper().replace('-', '_')
            field = f'\t\t.fd_access = CTLV_OP_FD_ACCESS_{access},'
            with self.subTest(op=op.name):
                if op.type == 'event':
                    continue
                self.assertIn(field, self.source)

    def test_the_table_marks_only_optional_ops(self):
        for entry in gen_user._spec_ops(self.family):
            op = entry.op
            body = self.source.split(
                f'\t\t.op_id = {self.names.op_id(op)},\n', 1)[1]
            body = body.split('\t},', 1)[0]
            with self.subTest(op=op.name):
                self.assertEqual(op.optional,
                                 '\t\t.optional = true,' in body)

    def test_a_filled_extension_point_widens_an_inherited_message(self):
        """Which is why a family describes an op its ancestor declared."""
        extension = self.family.attrs['nest.extension']
        empty = flat(f'.attr_id = {self.names.attr(extension)}, '
                     '.kind = CTLV_SCHEMA_NEST, '
                     '.max_occurrences = 1, },')
        self.assertIn(empty, flat(self.source))
        self.assertNotIn(empty, flat(self.child_source))
        for attr in ('child.tag', 'child.serial'):
            with self.subTest(attr=attr):
                found = self.child.attrs[attr]
                self.assertIn(
                    flat(f'.attr_id = {self.child_names.attr(found)},'),
                    flat(self.child_source))
                self.assertNotIn(self.child_names.attr(found), self.source)

    def test_a_re_entering_node_says_how_far_up_its_members_are(self):
        """The root family's table is the one that has any."""
        root = gen_user.render_source(support.load(spec.CORE_FAMILY_NAME))
        self.assertIn('\t\t.present = CTLV_NODE_SCHEMA_HAS_REENTERS,',
                      root)
        self.assertIn('\t\t.reenters = 1,', root)
        self.assertNotIn('.reenters', self.source)


class TestSpecPath(unittest.TestCase):
    def test_a_path_is_recorded_relative_to_the_kernel_tree(self):
        self.assertEqual('Documentation/ctlv/specs/ctlv.yaml',
                         cgen.spec_path(support.SPEC_DIR / 'ctlv.yaml'))
        self.assertEqual('tools/ctlv/tests/specs/ctlv-fixture.yaml',
                         cgen.spec_path(support.FIXTURE_SPEC_DIR /
                                        'ctlv-fixture.yaml'))


class TestOutputFile(unittest.TestCase):
    """The file handling that ctlv-regen.sh depends on."""

    CLI = support.TOOL_DIR / 'ctlvgen' / 'cli.py'

    def run_cli(self, *args, hash_seed=None):
        env = dict(os.environ)
        if hash_seed is not None:
            env['PYTHONHASHSEED'] = hash_seed
        result = subprocess.run([sys.executable, str(self.CLI), *args],
                                capture_output=True, text=True, check=False,
                                env=env)
        self.assertEqual('', result.stderr)
        self.assertEqual(0, result.returncode)
        return result.stdout

    def spec_args(self):
        return ('--spec', str(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml'),
                '--schema', str(support.SCHEMA))

    def test_generating_does_not_depend_on_the_hash_seed(self):
        for mode, kind, _ in FLAVOURS:
            with self.subTest(mode=mode, kind=kind):
                args = (*self.spec_args(), '--mode', mode, f'--{kind}')
                if mode == 'kernel':
                    args += ('--schema-only',)
                self.assertEqual(self.run_cli(*args, hash_seed='0'),
                                 self.run_cli(*args, hash_seed='12345'))

    def test_kernel_output_combines_only_a_registered_family(self):
        registered = self.run_cli(
            '--spec', str(support.SPEC_DIR / 'ctlv-test.yaml'),
            '--mode', 'kernel', '--header')
        fixture = self.run_cli(
            *self.spec_args(), '--mode', 'kernel', '--header',
            '--schema-only')

        self.assertIn('ctlv_test_schema', registered)
        self.assertIn('ctlv_test_init', registered)
        self.assertIn('ctlv_fixture_schema', fixture)
        self.assertNotIn('ctlv_fixture_init', fixture)

    def test_an_unchanged_output_is_left_alone(self):
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
