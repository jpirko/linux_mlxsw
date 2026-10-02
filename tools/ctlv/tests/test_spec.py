# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for loading and normalizing one specification.

What a declaration leaves out is filled in once, here, so that no later stage
has to know the defaults.
"""

import pathlib
import tempfile
import unittest
from unittest import mock

from tests import support

from ctlvgen.lib import gen_kernel
from ctlvgen.lib import semantics
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


class TestNormalization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.family = spec.load(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                               support.SCHEMA)
        cls.attrs = {attr.name: attr
                     for group in cls.family.groups
                     for attr in group.attrs}

    def test_declared_integer_bounds_are_kept(self):
        attr = self.attrs['u']
        self.assertEqual(('uint', 1, 4096), (attr.kind, attr.min, attr.max))
        attr = self.attrs['s']
        self.assertEqual(('sint', -100, 100), (attr.kind, attr.min, attr.max))

    def test_an_unsigned_attr_defaults_to_the_whole_range(self):
        attr = self.attrs['size']
        self.assertEqual((0, spec.U64_MAX), (attr.min, attr.max))

    def test_a_signed_attr_defaults_to_the_whole_range(self):
        attr = self.attrs['delta']
        self.assertEqual((spec.S64_MIN, spec.S64_MAX), (attr.min, attr.max))

    def test_unit_implies_a_scale_of_zero(self):
        self.assertEqual(('bytes', 0),
                         (self.attrs['size'].unit, self.attrs['size'].scale))
        self.assertEqual(('hertz', 3),
                         (self.attrs['freq'].unit, self.attrs['freq'].scale))

    def test_an_attr_without_a_unit_has_no_scale(self):
        self.assertIsNone(self.attrs['u'].unit)
        self.assertIsNone(self.attrs['u'].scale)

    def test_a_bool_has_nothing_to_normalize(self):
        attr = self.attrs['flag']
        self.assertEqual('bool', attr.kind)
        for value in (attr.min, attr.max, attr.min_length, attr.max_length,
                      attr.unit, attr.scale):
            self.assertIsNone(value)

    def test_a_declared_string_minimum_is_kept(self):
        attr = self.attrs['text']
        self.assertEqual((1, 15), (attr.min_length, attr.max_length))

    def test_a_string_minimum_defaults_to_zero(self):
        family = spec.load(
            support.FIXTURE_SPEC_DIR / 'ctlv-fixture-child.yaml',
            support.SCHEMA)
        tag = [attr for group in family.groups for attr in group.attrs
               if attr.name == 'tag'][0]
        self.assertEqual((0, 31), (tag.min_length, tag.max_length))

    def test_a_fixed_binary_length_becomes_equal_bounds(self):
        attr = self.attrs['fixed']
        self.assertEqual((8, 8), (attr.min_length, attr.max_length))

    def test_a_bounded_binary_keeps_both_bounds(self):
        attr = self.attrs['bounded']
        self.assertEqual((4, 64), (attr.min_length, attr.max_length))

    def test_unlimited_becomes_zero(self):
        self.assertEqual(spec.UNLIMITED, self.attrs['many'].max_occurrences)
        self.assertEqual(0, spec.UNLIMITED)

    def test_max_occurrences_defaults_to_one(self):
        self.assertEqual(1, self.attrs['u'].max_occurrences)

    def test_blob_alignment_defaults_to_one_byte(self):
        self.assertEqual(1, self.attrs['input-va'].blob.alignment)
        self.assertEqual(4096, self.attrs['output-fd'].blob.alignment)

    def test_an_omitted_request_is_an_empty_root(self):
        noop = [op for op in self.family.ops if op.name == 'noop'][0]
        self.assertEqual(spec.Placements(), noop.request)
        self.assertIsNone(noop.reply)

    def test_an_event_has_no_request(self):
        pinged = [op for op in self.family.ops if op.name == 'pinged'][0]
        self.assertIsNone(pinged.request)
        self.assertEqual(spec.Placements(), pinged.reply)

    def test_an_op_is_fixed_unless_it_declares_otherwise(self):
        by_name = {op.name: op for op in self.family.ops}
        self.assertEqual(spec.DEFAULT_AVAILABILITY,
                         by_name['noop'].availability)
        self.assertEqual('fixed', spec.DEFAULT_AVAILABILITY)
        self.assertEqual('dynamic', by_name['scalars-set'].availability)

    def test_an_op_requires_read_write_unless_it_declares_read(self):
        by_name = {op.name: op for op in self.family.ops}
        self.assertEqual('read', by_name['noop'].fd_access)
        self.assertEqual(spec.DEFAULT_FD_ACCESS,
                         by_name['scalars-set'].fd_access)
        self.assertEqual('read-write', spec.DEFAULT_FD_ACCESS)

    def test_an_op_is_mandatory_unless_it_declares_otherwise(self):
        by_name = {op.name: op for op in self.family.ops}
        self.assertFalse(by_name['noop'].optional)
        self.assertFalse(by_name['pinged'].optional)
        self.assertTrue(by_name['collapse-check'].optional)

    def test_an_op_has_no_pair_of_its_own_unless_it_asks(self):
        by_name = {op.name: op for op in self.family.ops}
        self.assertFalse(by_name['noop'].pre_post)
        self.assertFalse(by_name['pinged'].pre_post)
        self.assertTrue(by_name['counts-get'].pre_post)

    def test_family_init_hooks_are_optional(self):
        self.assertFalse(self.family.pre_init)
        self.assertFalse(self.family.post_init)
        self.assertFalse(self.family.pre_fini)

    def test_a_single_selector_becomes_a_list_of_one(self):
        op = [op for op in self.family.ops if op.name == 'scalars-set'][0]
        self.assertEqual(('every-scalar',), op.request.attrs)
        self.assertEqual((), op.request.optional_attrs)


class TestRepresentableDomains(unittest.TestCase):
    def attr(self, **raw):
        return spec._attr('test attr', {'name': 'value', **raw})

    def syntax(self, attr=None, name='x'):
        data = {'family': {'name': name}}
        if attr is not None:
            data['attr-groups'] = [{'name': 'group', 'attrs': [
                {'name': 'value', **attr},
            ]}]
        spec._validate_syntax(pathlib.Path('test.yaml'), data, support.SCHEMA)

    def syntax_op(self, op):
        data = {'family': {'name': 'x'}, 'ops': [op]}
        spec._validate_syntax(pathlib.Path('test.yaml'), data, support.SCHEMA)

    def test_names_fit_the_discovery_field(self):
        self.assertEqual(
            'x' * spec.MAX_FAMILY_NAME_LEN,
            spec._family_name('test', 'x' * spec.MAX_FAMILY_NAME_LEN))
        with self.assertRaises(CtlvError):
            spec._family_name(
                'test', 'x' * (spec.MAX_FAMILY_NAME_LEN + 1))
        self.syntax(name='x' * spec.MAX_FAMILY_NAME_LEN)
        with self.assertRaises(CtlvError):
            self.syntax(name='x' * (spec.MAX_FAMILY_NAME_LEN + 1))

        long_name = 'x' * (spec.MAX_FAMILY_NAME_LEN + 1)
        data = {
            'family': {'name': 'x'},
            'attr-groups': [{'name': long_name, 'attrs': [
                {'name': long_name, 'type': 'bool'},
            ]}],
            'attr-sets': [{'name': f'{long_name}-set',
                           'attrs': f'{long_name}.{long_name}'}],
        }
        spec._validate_syntax(pathlib.Path('test.yaml'), data, support.SCHEMA)

    def test_scale_fits_s8(self):
        for value in (spec.S8_MIN, spec.S8_MAX):
            with self.subTest(value=value):
                self.assertEqual(
                    value,
                    self.attr(type='uint', unit='bytes', scale=value).scale)
                self.syntax({'type': 'uint', 'unit': 'bytes',
                             'scale': value})
        for value in (spec.S8_MIN - 1, spec.S8_MAX + 1):
            with self.subTest(value=value):
                with self.assertRaises(CtlvError):
                    self.attr(type='uint', unit='bytes', scale=value)
                with self.assertRaises(CtlvError):
                    self.syntax({'type': 'uint', 'unit': 'bytes',
                                 'scale': value})

    def test_integer_bounds_fit_their_payload(self):
        large = spec.S64_MAX + 1
        attr = self.attr(type='uint', min=large, max=large)

        self.assertEqual(large, attr.min)
        self.assertEqual(f'{large}ULL', gen_kernel._bound('uint', large))
        self.syntax({'type': 'uint', 'min': large, 'max': spec.U64_MAX})
        self.syntax({'type': 'sint', 'min': spec.S64_MIN,
                     'max': spec.S64_MAX})

        for raw in (
                {'type': 'uint', 'max': spec.U64_MAX + 1},
                {'type': 'sint', 'min': spec.S64_MIN - 1}):
            with self.subTest(attr=raw):
                with self.assertRaises(CtlvError):
                    self.attr(**raw)
                with self.assertRaises(CtlvError):
                    self.syntax(raw)

    def test_occurrences_and_lengths_fit_u32(self):
        self.assertEqual(
            spec.U32_MAX,
            self.attr(type='bool',
                      **{'max-occurrences': spec.U32_MAX}).max_occurrences)
        self.assertEqual(
            spec.U32_MAX,
            self.attr(type='binary', length=spec.U32_MAX).max_length)
        self.assertEqual(
            spec.U32_MAX,
            self.attr(type='string',
                      **{'max-length': spec.U32_MAX}).max_length)
        self.syntax({'type': 'bool', 'max-occurrences': spec.U32_MAX})
        self.syntax({'type': 'binary', 'length': spec.U32_MAX})
        self.syntax({'type': 'string', 'max-length': spec.U32_MAX})
        with self.assertRaises(CtlvError):
            self.attr(type='bool',
                      **{'max-occurrences': spec.U32_MAX + 1})
        with self.assertRaises(CtlvError):
            self.attr(type='binary', length=spec.U32_MAX + 1)
        with self.assertRaises(CtlvError):
            self.attr(type='string',
                      **{'max-length': spec.U32_MAX + 1})
        with self.assertRaises(CtlvError):
            self.syntax({'type': 'bool',
                         'max-occurrences': spec.U32_MAX + 1})
        with self.assertRaises(CtlvError):
            self.syntax({'type': 'binary', 'length': spec.U32_MAX + 1})
        with self.assertRaises(CtlvError):
            self.syntax({'type': 'string',
                         'max-length': spec.U32_MAX + 1})

    def test_suggested_reply_fits_the_transport_field(self):
        maximum = 1 << 20
        op = {'name': 'get', 'type': 'query', 'reply': {'attrs': 'value'},
              'suggested-reply-length': maximum}

        self.syntax_op(op)
        with self.assertRaises(CtlvError):
            self.syntax_op({**op, 'suggested-reply-length': maximum + 1})

    def test_blob_bounds_fit_their_metadata(self):
        blob = {
            'direction': 'input',
            'allowed-backings': ['va'],
            'max-length': spec.U64_MAX,
        }
        self.assertEqual(spec.U64_MAX, spec._blob('test', blob).max_length)
        self.syntax({'blob': blob})
        args = gen_kernel._blob_args(spec._blob('test', blob))
        self.assertEqual('U64_MAX', args[0])

        aligned = {**blob, 'max-length': 1 << 63,
                   'alignment': 1 << 31}
        self.assertEqual(1 << 31, spec._blob('test', aligned).alignment)
        self.syntax({'blob': aligned})

        for invalid in (
                {**blob, 'max-length': spec.U64_MAX + 1},
                {**blob, 'max-length': 1 << 32, 'alignment': 1 << 32}):
            with self.subTest(blob=invalid):
                with self.assertRaises(CtlvError):
                    spec._blob('test', invalid)
                with self.assertRaises(CtlvError):
                    self.syntax({'blob': invalid})

    def test_narrow_root_enums_fit_their_fields(self):
        family = spec.FamilySpec(
            path='test', name='ctlv', license='GPL-2.0', inherits=None,
            enums=(), flags=(), groups=(), sets=(), ops=())

        for name, maximum in (('blob-type', 15),
                              ('op-type', spec.U8_MAX)):
            with self.subTest(enum=name):
                resolver = object.__new__(semantics._Resolver)
                resolver.definitions = {}
                values = tuple(f'value-{i}' for i in range(maximum))
                resolver._add_definitions(
                    family, spec.CORE_FAMILY_ID,
                    (spec.ValueList(name, values),), 'enum')
                resolver = object.__new__(semantics._Resolver)
                resolver.definitions = {}
                with self.assertRaisesRegex(CtlvError, 'enum values exceed'):
                    resolver._add_definitions(
                        family, spec.CORE_FAMILY_ID,
                        (spec.ValueList(name, values + ('one-too-many',)),),
                        'enum')

    def test_flags_fit_their_u64_mask(self):
        data = {'family': {'name': 'x'}, 'flags': [{
            'name': 'bits',
            'values': [f'bit-{i}' for i in range(semantics.MAX_FLAG_BITS)],
        }]}

        spec._validate_syntax(pathlib.Path('test.yaml'), data, support.SCHEMA)
        data['flags'][0]['values'].append('one-too-many')
        with self.assertRaises(CtlvError):
            spec._validate_syntax(pathlib.Path('test.yaml'), data,
                                  support.SCHEMA)

    def test_generated_counts_fit_u32(self):
        semantics._check_count('test', 'effective ops', spec.U32_MAX,
                               spec.U32_MAX)
        with self.assertRaisesRegex(CtlvError, 'effective ops exceed'):
            semantics._check_count('test', 'effective ops',
                                   spec.U32_MAX + 1, spec.U32_MAX)

        family = support.load_family('ctlv-fixture-child')
        with mock.patch.object(spec, 'U32_MAX', 1):
            with self.assertRaisesRegex(CtlvError, 'composed'):
                semantics._check_composed(family)


class TestRegistry(unittest.TestCase):
    def load(self, text):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'families.yaml'
            path.write_text(text, encoding='utf-8')
            paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml',
                               schema=support.SCHEMA, registry=path)
            return spec.load_registry(paths)

    def registered(self):
        return spec.load_registry(
            spec.Paths(support.SPEC_DIR / 'ctlv.yaml'))

    def test_the_registry_holds_the_implemented_families(self):
        """A name is registered by the change that implements the family."""
        registry = self.registered()
        self.assertEqual(list(support.FAMILIES),
                         [entry.name for entry in registry.entries])
        self.assertEqual(spec.CORE_FAMILY_ID,
                         registry.family_id(spec.CORE_FAMILY_NAME))
        self.assertEqual(1, registry.family_id('ctlv-attestation'))
        self.assertEqual(2, registry.family_id('ctlv-attestation-tdx'))
        self.assertEqual(3, registry.family_id('ctlv-test'))
        self.assertEqual(4, registry.family_id('ctlv-test-child'))
        self.assertEqual(5, registry.family_id('ctlv-attestation-sim'))

    def test_the_fixture_registry_numbers_the_fixture_families(self):
        registry = spec.load_registry(
            spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                       schema=support.SCHEMA))
        self.assertEqual(1, registry.family_id('ctlv-fixture'))
        self.assertEqual(2, registry.family_id('ctlv-fixture-child'))
        self.assertIsNone(registry.family_id('nothing'))

    def test_the_core_family_heads_the_registry(self):
        registry = self.registered()
        self.assertEqual(spec.CORE_FAMILY_ID,
                         registry.family_id(spec.CORE_FAMILY_NAME))

    def test_a_registry_not_headed_by_the_core_family_is_rejected(self):
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n  - one\n  - ctlv\n')
        self.assertIn('must be the first entry', str(caught.exception))

    def test_an_empty_registry_is_rejected(self):
        with self.assertRaises(CtlvError) as caught:
            self.load('families: []\n')
        self.assertIn('should be non-empty', str(caught.exception))

    def test_deprecating_the_core_family_is_rejected(self):
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n'
                      '  - name: ctlv\n'
                      '    deprecated: true\n')
        self.assertIn('cannot be deprecated', str(caught.exception))

    def test_a_duplicate_family_is_rejected(self):
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n  - ctlv\n  - one\n  - one\n')
        self.assertIn('duplicate family', str(caught.exception))

    def test_a_registry_name_fits_discovery(self):
        name = 'x' * spec.MAX_FAMILY_NAME_LEN

        self.assertEqual(1, self.load(f'families: [ctlv, {name}]\n')
                         .family_id(name))
        with self.assertRaises(CtlvError):
            self.load(f'families: [ctlv, {name}x]\n')

    def test_a_deprecated_entry_keeps_its_id_reserved(self):
        registry = self.load('families:\n'
                             '  - ctlv\n'
                             '  - one\n'
                             '  - name: two\n'
                             '    deprecated: true\n'
                             '  - three\n')
        self.assertEqual([False, False, True, False],
                         [entry.deprecated for entry in registry.entries])
        self.assertEqual(3, registry.family_id('three'))


class TestCheckedInTree(unittest.TestCase):
    """The registry, the two spec directories and the test suite agree.

    A specification under Documentation/ctlv/ claims a permanent family ID, so
    it must be registered; one under tests/specs/ must not be.
    """

    @classmethod
    def setUpClass(cls):
        cls.registry = spec.load_registry(
            spec.Paths(support.SPEC_DIR / 'ctlv.yaml'))
        cls.stems = sorted(path.stem
                           for path in support.SPEC_DIR.glob('*.yaml'))
        cls.fixture_stems = sorted(
            path.stem for path in support.FIXTURE_SPEC_DIR.glob('*.yaml'))

    def test_every_registered_family_has_a_specification(self):
        for entry in self.registry.entries:
            with self.subTest(family=entry.name):
                self.assertIn(entry.name, self.stems)

    def test_every_specification_is_registered(self):
        for stem in self.stems:
            with self.subTest(family=stem):
                self.assertIsNotNone(self.registry.family_id(stem))

    def test_no_fixture_family_is_registered(self):
        for stem in self.fixture_stems:
            with self.subTest(family=stem):
                self.assertIsNone(self.registry.family_id(stem))

    def test_the_tested_families_are_all_of_them(self):
        self.assertEqual(self.stems, sorted(support.FAMILIES))
        self.assertEqual(self.fixture_stems,
                         sorted(support.FIXTURE_FAMILIES))


class TestPaths(unittest.TestCase):
    def test_everything_is_derived_from_the_specification_path(self):
        paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')
        self.assertEqual(support.SCHEMA, paths.schema)
        self.assertEqual(support.DOC_DIR / 'families.yaml', paths.registry)
        self.assertEqual(support.DOC_DIR / 'ctlv-registry-schema.yaml',
                         paths.registry_schema)
        self.assertEqual(support.ABI_DIR / 'families.registry.yaml',
                         paths.registry_abi)
        self.assertEqual(support.DOC_DIR / 'generated-files.yaml',
                         paths.generated)
        self.assertEqual(support.KERNEL_DIR, paths.kernel_dir)
        self.assertEqual(support.ABI_DIR / 'ctlv.yaml',
                         paths.abi('ctlv'))

    def test_a_fixture_uses_the_registry_beside_it(self):
        """This is what lets a fixture live outside Documentation at all."""
        paths = spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                           schema=support.SCHEMA)
        self.assertEqual(support.FIXTURE_REGISTRY, paths.registry)
        self.assertEqual(support.KERNEL_DIR, paths.kernel_dir)
        self.assertEqual(support.FIXTURE_SPEC_DIR / 'ctlv-fixture-child.yaml',
                         paths.sibling_spec('ctlv-fixture-child'))

    def test_a_custom_abi_directory_includes_the_registry_baseline(self):
        paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml',
                           abi='/tmp/ctlv-abi')

        self.assertEqual(pathlib.Path('/tmp/ctlv-abi/ctlv.yaml'),
                         paths.abi('ctlv'))
        self.assertEqual(
            pathlib.Path('/tmp/ctlv-abi/families.registry.yaml'),
            paths.registry_abi)


if __name__ == '__main__':
    unittest.main()
