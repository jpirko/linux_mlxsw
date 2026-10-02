# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for loading and normalizing one specification.

What a declaration leaves out is filled in once, here, so that no later stage
has to know the defaults.
"""

from __future__ import annotations

import pathlib
import tempfile
import unittest
from unittest import mock
from typing import Any

from tests import support

from ctlvgen.lib import gen_kernel
from ctlvgen.lib import semantics
from ctlvgen.lib import sizes
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


def declarations(family: spec.FamilySpec) -> dict[str, spec.FieldDecl]:
    """Every field declaration of a specification by name, the first kept."""
    found: dict[str, spec.FieldDecl] = {}

    def walk(body: spec.Body) -> None:
        for item in body.items:
            if isinstance(item, spec.FieldDecl):
                found.setdefault(item.name, item)
                if item.body is not None:
                    walk(item.body)

    for record in family.records:
        if record.field is not None:
            found.setdefault(record.name, record.field)
        else:
            assert record.body is not None
            walk(record.body)
    for op in family.ops:
        for message in (op.request, op.reply):
            if message is not None:
                walk(message.body)
    return found


class TestNormalization(unittest.TestCase):
    family: spec.FamilySpec
    fields: dict[str, spec.FieldDecl]
    ops: dict[str, spec.OpSpec]

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = spec.load(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                               support.SCHEMA)
        cls.fields = declarations(cls.family)
        cls.ops = {op.name: op for op in cls.family.ops}

    def test_declared_integer_bounds_are_kept(self) -> None:
        field = self.fields['u']
        self.assertEqual(('uint', 1, 4096),
                         (field.kind, field.min, field.max))
        field = self.fields['s']
        self.assertEqual(('sint', -100, 100),
                         (field.kind, field.min, field.max))

    def test_an_unsigned_field_defaults_to_the_whole_range(self) -> None:
        field = self.fields['size']
        self.assertEqual((0, spec.U64_MAX), (field.min, field.max))

    def test_a_signed_field_defaults_to_the_whole_range(self) -> None:
        field = self.fields['delta']
        self.assertEqual((spec.S64_MIN, spec.S64_MAX), (field.min, field.max))

    def test_unit_implies_a_scale_of_zero(self) -> None:
        self.assertEqual(('bytes', 0),
                         (self.fields['size'].unit, self.fields['size'].scale))
        self.assertEqual(('hertz', 3),
                         (self.fields['freq'].unit, self.fields['freq'].scale))

    def test_a_field_without_a_unit_has_no_scale(self) -> None:
        self.assertIsNone(self.fields['u'].unit)
        self.assertIsNone(self.fields['u'].scale)

    def test_a_bool_has_nothing_to_normalize(self) -> None:
        field = self.fields['flag']
        self.assertEqual('bool', field.kind)
        for value in (field.min, field.max, field.min_length,
                      field.max_length, field.unit, field.scale):
            self.assertIsNone(value)

    def test_a_declared_string_minimum_is_kept(self) -> None:
        field = self.fields['text']
        self.assertEqual((1, 15), (field.min_length, field.max_length))

    def test_a_string_minimum_defaults_to_zero(self) -> None:
        family = spec.load(
            support.FIXTURE_SPEC_DIR / 'ctlv-fixture-child.yaml',
            support.SCHEMA)
        tag = declarations(family)['tag']
        self.assertEqual((0, 31), (tag.min_length, tag.max_length))

    def test_a_fixed_binary_length_becomes_equal_bounds(self) -> None:
        field = self.fields['fixed']
        self.assertEqual((8, 8), (field.min_length, field.max_length))

    def test_a_bounded_binary_keeps_both_bounds(self) -> None:
        field = self.fields['bounded']
        self.assertEqual((4, 64), (field.min_length, field.max_length))

    def test_an_empty_count_becomes_zero(self) -> None:
        self.assertEqual(spec.UNLIMITED, self.fields['many'].max_occurrences)
        self.assertEqual(0, spec.UNLIMITED)

    def test_a_count_and_a_question_mark_come_from_the_key(self) -> None:
        field = self.fields['few']
        self.assertEqual((3, True), (field.max_occurrences, field.optional))

    def test_a_count_naming_an_enum_is_resolved_later(self) -> None:
        self.assertEqual('mode', self.fields['per-mode'].count_enum)
        self.assertIsNone(self.fields['few'].count_enum)

    def test_a_field_occurs_once_and_is_required_by_default(self) -> None:
        field = self.fields['delta']
        self.assertEqual((1, False), (field.max_occurrences, field.optional))

    def test_an_enum_or_flags_is_resolved_later(self) -> None:
        field = self.fields['quirks']
        self.assertEqual(('definition', 'quirks'), (field.kind, field.ref))

    def test_a_header_enum_keeps_its_header(self) -> None:
        enums = {enum.name: enum for enum in self.family.enums}
        self.assertEqual(spec.HeaderEnum('linux/hash_info.h', 'hash_algo',
                                         'HASH_ALGO__LAST'),
                         enums['hash-algo'].header)
        self.assertEqual((('HASH_ALGO_MD4', 0), ('HASH_ALGO_MD5', 1)),
                         tuple((value.name, value.number)
                               for value in enums['hash-algo'].values[:2]))
        self.assertIsNone(enums['mode'].header)

    def test_a_value_keeps_the_comment_above_it(self) -> None:
        enums = {enum.name: enum for enum in self.family.enums}
        flags = {flag.name: flag for flag in self.family.flags}
        self.assertEqual(('The first value.', 'The second value.',
                          'The third value.'),
                         tuple(value.doc for value in enums['mode'].values))
        self.assertEqual('Bit 0.', flags['quirks'].values[0].doc)

    def blob(self, name: str) -> spec.BlobDecl:
        blob = self.fields[name].blob
        assert blob is not None
        return blob

    def test_a_suffix_scales_a_byte_count(self) -> None:
        self.assertEqual(1 << 20, self.blob('output-fd').max_length)
        self.assertEqual(4096, self.ops['records-dump'].suggested_reply_length)

    def test_blob_alignment_defaults_to_one_byte(self) -> None:
        self.assertEqual(1, self.blob('input-va').alignment)
        self.assertEqual(4096, self.blob('output-fd').alignment)

    def test_blob_backings_are_kept_in_order(self) -> None:
        self.assertEqual(('va',), self.blob('input-va').backings)
        self.assertEqual(('memfd', 'dma-buf'),
                         self.blob('output-fd').backings)

    def test_an_omitted_request_is_an_empty_record(self) -> None:
        noop = self.ops['noop']
        assert noop.request is not None
        self.assertEqual((spec.Body(), None),
                         (noop.request.body, noop.request.ref))
        self.assertIsNone(noop.reply)

    def test_an_event_has_no_request(self) -> None:
        pinged = self.ops['pinged']
        self.assertIsNone(pinged.request)
        assert pinged.reply is not None
        self.assertEqual(spec.Body(), pinged.reply.body)

    def test_an_op_is_fixed_unless_it_declares_otherwise(self) -> None:
        self.assertEqual(spec.DEFAULT_AVAILABILITY,
                         self.ops['noop'].availability)
        self.assertEqual('fixed', spec.DEFAULT_AVAILABILITY)
        self.assertEqual('dynamic', self.ops['scalars-set'].availability)

    def test_an_op_requires_read_write_unless_it_declares_read(self) -> None:
        self.assertEqual('read', self.ops['noop'].fd_access)
        self.assertEqual(spec.DEFAULT_FD_ACCESS,
                         self.ops['scalars-set'].fd_access)
        self.assertEqual('read-write', spec.DEFAULT_FD_ACCESS)

    def test_an_op_is_mandatory_unless_it_declares_otherwise(self) -> None:
        self.assertFalse(self.ops['noop'].optional)
        self.assertFalse(self.ops['pinged'].optional)
        self.assertTrue(self.ops['optional-check'].optional)

    def test_an_op_has_no_pair_of_its_own_unless_it_asks(self) -> None:
        self.assertFalse(self.ops['noop'].pre_post)
        self.assertFalse(self.ops['pinged'].pre_post)
        self.assertTrue(self.ops['counts-get'].pre_post)

    def test_a_message_naming_a_record_keeps_the_name(self) -> None:
        request = self.ops['scalars-set'].request
        assert request is not None
        self.assertEqual(('scalar', spec.Body()), (request.ref, request.body))

    def test_placed_records_and_includes_stay_as_written(self) -> None:
        reply = self.ops['scalars-set'].reply
        assert reply is not None
        [place] = reply.body.items
        self.assertIsInstance(place, spec.Place)
        self.assertEqual(('u', False), (place.name, place.optional))
        reply = self.ops['thing-changed'].reply
        assert reply is not None
        [include] = reply.body.items
        self.assertIsInstance(include, spec.Include)
        self.assertEqual(('scalar-pair', False),
                         (include.name, include.optional))


class TestValueDocs(unittest.TestCase):
    """Only a value on a line of its own has a comment of its own."""

    def values(self, text: str) -> list[tuple[str, str | None]]:
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'ctlv-values.yaml'
            path.write_text('# SPDX-License-Identifier: GPL-2.0\n'
                            'family: ctlv-values\ninherits: ctlv\n\n'
                            'enums:\n' + text, encoding='utf-8')
            [enum] = spec.load(path, support.SCHEMA).enums
        return [(value.name, value.doc) for value in enum.values]

    def test_a_value_takes_the_comment_right_above_it(self) -> None:
        self.assertEqual([('a', 'The first.'), ('b', None)],
                         self.values('  # The set.\n  mode:\n'
                                     '    # The first.\n    - a\n    - b\n'))

    def test_values_on_the_line_of_their_set_take_none(self) -> None:
        self.assertEqual([('a', None), ('b', None)],
                         self.values('  # The set.\n  mode: [a, b]\n'))

    def test_values_a_header_defines_take_none(self) -> None:
        self.assertEqual([('HASH_ALGO_MD4', None)],
                         self.values('  hash-algo:\n'
                                     '    $header: linux/hash_info.h\n'
                                     '    # Not a value\'s.\n'
                                     '    $enum: hash_algo\n'
                                     '    $end: HASH_ALGO_MD5\n'))


class TestHeaderEnums(unittest.TestCase):
    """An enum a uapi header defines, as a specification names it."""

    def load(self, text: str) -> spec.ValueList:
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'ctlv-header.yaml'
            path.write_text('# SPDX-License-Identifier: GPL-2.0\n'
                            'family: ctlv-header\ninherits: ctlv\n\n'
                            'enums:\n  mode:\n' + text, encoding='utf-8')
            [enum] = spec.load(path, support.SCHEMA).enums
        return enum

    def test_constants_keep_their_names_and_numbers(self) -> None:
        mode = self.load('    $header: linux/dpll.h\n'
                         '    $enum: dpll_mode\n'
                         '    $end: __DPLL_MODE_MAX\n')
        self.assertEqual(spec.HeaderEnum('linux/dpll.h', 'dpll_mode',
                                         '__DPLL_MODE_MAX'), mode.header)
        self.assertEqual([('DPLL_MODE_MANUAL', 1), ('DPLL_MODE_AUTOMATIC', 2)],
                         [(value.name, value.number) for value in mode.values])

    def test_without_an_end_every_constant_is_a_value(self) -> None:
        mode = self.load('    $header: linux/dpll.h\n'
                         '    $enum: dpll_mode\n')
        self.assertEqual(['DPLL_MODE_MANUAL', 'DPLL_MODE_AUTOMATIC',
                          '__DPLL_MODE_MAX', 'DPLL_MODE_MAX'],
                         [value.name for value in mode.values])

    def test_a_header_enum_lists_no_values(self) -> None:
        with self.assertRaisesRegex(CtlvError, r'\$values'):
            self.load('    $header: linux/hash_info.h\n'
                      '    $enum: hash_algo\n'
                      '    $values: [md5]\n')

    def test_only_a_uapi_header_is_read(self) -> None:
        with self.assertRaisesRegex(CtlvError,
                                    'crypto/hash_info.h> is not a uapi'):
            self.load('    $header: crypto/hash_info.h\n'
                      '    $enum: hash_algo\n')


class TestRepresentableDomains(unittest.TestCase):
    def field(self, text: str, key: str = 'value') -> spec.FieldDecl:
        reader = spec._Reader(pathlib.Path('test.yaml'), [])
        decl = reader.field(key, text, 'test field', None)
        assert isinstance(decl, spec.FieldDecl)
        return decl

    def blob(self, text: str) -> spec.BlobDecl:
        blob = self.field(text).blob
        assert blob is not None
        return blob

    def syntax(self, name: str = 'x', **data: Any) -> None:
        spec._validate_syntax(pathlib.Path('test.yaml'),
                              {'family': name, **data}, support.SCHEMA)

    def test_names_fit_the_discovery_field(self) -> None:
        self.assertEqual(
            'x' * spec.MAX_FAMILY_NAME_LEN,
            spec._family_name('test', 'x' * spec.MAX_FAMILY_NAME_LEN))
        with self.assertRaises(CtlvError):
            spec._family_name(
                'test', 'x' * (spec.MAX_FAMILY_NAME_LEN + 1))
        self.syntax(name='x' * spec.MAX_FAMILY_NAME_LEN)
        with self.assertRaises(CtlvError):
            self.syntax(name='x' * (spec.MAX_FAMILY_NAME_LEN + 1))

        # Only a family name travels in discovery.
        long_name = 'x' * (spec.MAX_FAMILY_NAME_LEN + 1)
        self.syntax(records={long_name: {long_name: 'bool'}},
                    ops={long_name: {'type': 'action',
                                     'request': f'record {long_name}'}})

    def test_scale_fits_s8(self) -> None:
        for value in (spec.S8_MIN, spec.S8_MAX):
            with self.subTest(value=value):
                self.assertEqual(
                    value, self.field(f'uint(bytes, scale {value})').scale)
        for value in (spec.S8_MIN - 1, spec.S8_MAX + 1):
            with self.subTest(value=value):
                with self.assertRaisesRegex(CtlvError, 'outside the s8'):
                    self.field(f'uint(bytes, scale {value})')

    def test_integer_bounds_fit_their_payload(self) -> None:
        large = spec.S64_MAX + 1
        field = self.field(f'uint({large}..{large})')

        self.assertEqual(large, field.min)
        self.assertEqual(f'{large}ULL', gen_kernel._bound('uint', large))
        self.assertEqual(spec.U64_MAX, self.field('uint(..u64-max)').max)
        field = self.field('sint(s64-min..s64-max)')
        self.assertEqual((spec.S64_MIN, spec.S64_MAX), (field.min, field.max))

        for text in (f'uint(..{spec.U64_MAX + 1})',
                     f'sint({spec.S64_MIN - 1}..)'):
            with self.subTest(field=text):
                with self.assertRaisesRegex(CtlvError, 'outside the'):
                    self.field(text)

    def test_only_a_byte_count_takes_a_size_suffix(self) -> None:
        field = self.field('uint(1..4K, bytes)')
        self.assertEqual((1, 4096), (field.min, field.max))
        with self.assertRaisesRegex(CtlvError, 'give the uint the bytes'):
            self.field('uint(1..4K)')

    def test_occurrences_and_lengths_fit_u32(self) -> None:
        self.assertEqual(
            spec.U32_MAX,
            self.field('bool', key=f'value[{spec.U32_MAX}]').max_occurrences)
        self.assertEqual(spec.U32_MAX,
                         self.field(f'binary({spec.U32_MAX})').max_length)
        self.assertEqual(spec.U32_MAX,
                         self.field('string(..u32-max)').max_length)
        for key, text in ((f'value[{spec.U32_MAX + 1}]', 'bool'),
                          ('value[0]', 'bool'),
                          ('value', f'binary({spec.U32_MAX + 1})'),
                          ('value', f'string(..{spec.U32_MAX + 1})'),
                          ('value', 'string(..8G)')):
            with self.subTest(key=key, field=text):
                with self.assertRaisesRegex(CtlvError, 'u32-max'):
                    self.field(text, key=key)

    def test_suggested_reply_fits_the_transport_field(self) -> None:
        maximum = sizes.MAX_INLINE_MESSAGE_LEN
        op = {'type': 'query', 'reply': {'value': 'bool'}}

        for written in (maximum, '128K'):
            with self.subTest(written=written):
                self.syntax(ops={'get': {**op,
                                         'suggested-reply-length': written}})
        reader = spec._Reader(pathlib.Path('test.yaml'), [])
        parsed = reader.ops({'get': {**op, 'suggested-reply-length': '128K'}})
        self.assertEqual(maximum, parsed[0].suggested_reply_length)
        self.assertEqual(maximum, sizes._suggested_len('test', maximum, 40))
        with self.assertRaisesRegex(CtlvError, 'inline message limit'):
            sizes._suggested_len('test', maximum + 1, 40)

    def test_blob_bounds_fit_their_metadata(self) -> None:
        blob = self.blob('blob(in, ..u64-max, va)')
        self.assertEqual(spec.U64_MAX, blob.max_length)
        self.assertEqual('U64_MAX', gen_kernel._blob_args(blob)[0])
        self.assertEqual(spec.U32_MAX,
                         self.blob('blob(out)').max_length)

        aligned = self.blob(f'blob(in, ..{1 << 63}, align 2G, va)')
        self.assertEqual((1 << 63, 1 << 31),
                         (aligned.max_length, aligned.alignment))
        self.assertEqual([f'{1 << 63}ULL', 'SZ_2G'],
                         gen_kernel._blob_args(aligned)[:2])

        for text in (f'blob(in, ..{spec.U64_MAX + 1}, va)',
                     f'blob(in, ..{1 << 32}, align 4G, va)'):
            with self.subTest(field=text):
                with self.assertRaisesRegex(CtlvError, 'outside 0 to'):
                    self.field(text)

    def test_narrow_root_enums_fit_their_fields(self) -> None:
        family = spec.FamilySpec(
            path='test', name='ctlv', license='GPL-2.0', inherits=None,
            enums=(), flags=(), records=(), ops=())

        for name, maximum in (('blob-type', 15),
                              ('op-type', spec.U8_MAX)):
            with self.subTest(enum=name):
                resolver = object.__new__(semantics._Resolver)
                resolver.definitions = {}
                values = tuple(spec.Value(f'value-{i}', 'test')
                               for i in range(maximum))
                resolver._add_definitions(
                    family, spec.CORE_FAMILY_ID,
                    (spec.ValueList(name, 'test', values),), 'enum')
                resolver = object.__new__(semantics._Resolver)
                resolver.definitions = {}
                extra = spec.Value('one-too-many', 'test')
                with self.assertRaisesRegex(CtlvError, 'enum values exceed'):
                    resolver._add_definitions(
                        family, spec.CORE_FAMILY_ID,
                        (spec.ValueList(name, 'test', values + (extra,)),),
                        'enum')

    def test_flags_fit_their_u64_mask(self) -> None:
        values = [f'bit-{i}' for i in range(semantics.MAX_FLAG_BITS)]

        self.syntax(flags={'bits': values})
        with self.assertRaises(CtlvError):
            self.syntax(flags={'bits': values + ['one-too-many']})

    def test_generated_counts_fit_u32(self) -> None:
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
    def load(self, text: str) -> spec.Registry:
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'families.yaml'
            path.write_text(text, encoding='utf-8')
            paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml',
                               schema=support.SCHEMA, registry=path)
            return spec.load_registry(paths)

    def registered(self) -> spec.Registry:
        return spec.load_registry(
            spec.Paths(support.SPEC_DIR / 'ctlv.yaml'))

    def test_the_registry_holds_the_implemented_families(self) -> None:
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

    def test_the_fixture_registry_numbers_the_fixture_families(self) -> None:
        registry = spec.load_registry(
            spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                       schema=support.SCHEMA))
        self.assertEqual(1, registry.family_id('ctlv-fixture'))
        self.assertEqual(2, registry.family_id('ctlv-fixture-child'))
        self.assertIsNone(registry.family_id('nothing'))

    def test_the_core_family_heads_the_registry(self) -> None:
        registry = self.registered()
        self.assertEqual(spec.CORE_FAMILY_ID,
                         registry.family_id(spec.CORE_FAMILY_NAME))

    def test_a_registry_not_headed_by_the_core_family_is_rejected(
            self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n  - one\n  - ctlv\n')
        self.assertIn('must be the first entry', str(caught.exception))

    def test_an_empty_registry_is_rejected(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load('families: []\n')
        self.assertIn('should be non-empty', str(caught.exception))

    def test_deprecating_the_core_family_is_rejected(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n'
                      '  - name: ctlv\n'
                      '    deprecated: true\n')
        self.assertIn('cannot be deprecated', str(caught.exception))

    def test_a_duplicate_family_is_rejected(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load('families:\n  - ctlv\n  - one\n  - one\n')
        self.assertIn('duplicate family', str(caught.exception))

    def test_a_registry_name_fits_discovery(self) -> None:
        name = 'x' * spec.MAX_FAMILY_NAME_LEN

        self.assertEqual(1, self.load(f'families: [ctlv, {name}]\n')
                         .family_id(name))
        with self.assertRaises(CtlvError):
            self.load(f'families: [ctlv, {name}x]\n')

    def test_a_deprecated_entry_keeps_its_id_reserved(self) -> None:
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

    registry: spec.Registry
    stems: list[str]
    fixture_stems: list[str]

    @classmethod
    def setUpClass(cls) -> None:
        cls.registry = spec.load_registry(
            spec.Paths(support.SPEC_DIR / 'ctlv.yaml'))
        cls.stems = sorted(path.stem
                           for path in support.SPEC_DIR.glob('*.yaml'))
        cls.fixture_stems = sorted(
            path.stem for path in support.FIXTURE_SPEC_DIR.glob('*.yaml'))

    def test_every_registered_family_has_a_specification(self) -> None:
        for entry in self.registry.entries:
            with self.subTest(family=entry.name):
                self.assertIn(entry.name, self.stems)

    def test_every_specification_is_registered(self) -> None:
        for stem in self.stems:
            with self.subTest(family=stem):
                self.assertIsNotNone(self.registry.family_id(stem))

    def test_no_fixture_family_is_registered(self) -> None:
        for stem in self.fixture_stems:
            with self.subTest(family=stem):
                self.assertIsNone(self.registry.family_id(stem))

    def test_the_tested_families_are_all_of_them(self) -> None:
        self.assertEqual(self.stems, sorted(support.FAMILIES))
        self.assertEqual(self.fixture_stems,
                         sorted(support.FIXTURE_FAMILIES))


class TestPaths(unittest.TestCase):
    def test_everything_is_derived_from_the_specification_path(self) -> None:
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

    def test_a_fixture_uses_the_registry_beside_it(self) -> None:
        """This is what lets a fixture live outside Documentation at all."""
        paths = spec.Paths(support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml',
                           schema=support.SCHEMA)
        self.assertEqual(support.FIXTURE_REGISTRY, paths.registry)
        self.assertEqual(support.KERNEL_DIR, paths.kernel_dir)
        self.assertEqual(support.FIXTURE_SPEC_DIR / 'ctlv-fixture-child.yaml',
                         paths.sibling_spec('ctlv-fixture-child'))

    def test_a_custom_abi_directory_includes_the_registry_baseline(
            self) -> None:
        paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml',
                           abi='/tmp/ctlv-abi')

        self.assertEqual(pathlib.Path('/tmp/ctlv-abi/ctlv.yaml'),
                         paths.abi('ctlv'))
        self.assertEqual(
            pathlib.Path('/tmp/ctlv-abi/families.registry.yaml'),
            paths.registry_abi)


if __name__ == '__main__':
    unittest.main()
