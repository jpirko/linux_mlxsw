# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the ABI snapshots and the compatibility checker.

Each case takes the current schema and a snapshot differing from it in exactly
one way, as the corresponding mistake would look after an edit.
"""

from __future__ import annotations

import copy
import pathlib
import tempfile
import unittest
from collections.abc import Callable
from typing import Any

from tests import support

from ctlvgen.lib import compat
from ctlvgen.lib import semantics
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


class TestCheckedInSnapshots(unittest.TestCase):
    """A snapshot exists for a registered family and only for one."""

    def test_every_family_matches_its_snapshot(self) -> None:
        for name in support.FAMILIES:
            with self.subTest(family=name):
                family = support.load(name)
                compat.check(family, support.ABI_DIR / f'{name}.yaml')

    def test_a_rendered_snapshot_reloads_unchanged(self) -> None:
        for name in support.FAMILIES:
            with self.subTest(family=name):
                family = support.load(name)
                self.assertEqual(
                    compat.render(family),
                    (support.ABI_DIR / f'{name}.yaml').read_text(
                        encoding='utf-8'))

    def test_a_snapshot_contains_only_its_own_abi(self) -> None:
        parent = compat.build(support.load_family('ctlv-fixture'))
        child = compat.build(support.load_family('ctlv-fixture-child'))

        self.assertIn('records-dump', parent['ops'])
        self.assertNotIn('records-dump', child['ops'])
        self.assertNotIn('extension-point', parent['fills'])
        self.assertIn('extension-point', child['fills'])
        # An enum extension point has no values; the fill has them.
        self.assertEqual({'variant': 1}, parent['enum-extensions'])
        self.assertNotIn('variant', parent['definitions'])
        self.assertEqual({}, child['enum-extensions'])
        self.assertEqual({'plain': 1, 'fancy': 2},
                         child['definitions']['variant']['values'])

    def test_no_snapshot_has_lost_its_specification(self) -> None:
        """A family cannot be dropped or renamed out of the tree.

        No comparison against a snapshot catches that, so the two directories
        are compared as well.
        """
        self.assertEqual(
            sorted(path.stem for path in support.SPEC_DIR.glob('*.yaml')),
            sorted(path.stem for path in support.ABI_DIR.glob('*.yaml')
                   if path.name != 'families.registry.yaml'))

    def test_the_registry_matches_its_baseline(self) -> None:
        paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')
        registry = spec.load_registry(paths)

        self.assertEqual(
            compat.registry_render(registry),
            paths.registry_abi.read_text(encoding='utf-8'))

    def test_a_fixture_family_has_no_snapshot(self) -> None:
        self.assertFalse((support.TESTS_DIR / 'abi').exists())


class TestRoundTrip(unittest.TestCase):
    """A rendered snapshot has to survive the strict loader and the checker.

    The fixtures included, whose snapshots go to a temporary file.
    """

    def test_a_render_reloads_and_still_matches_its_family(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                family = support.load_any(name)
                with tempfile.TemporaryDirectory() as directory:
                    path = pathlib.Path(directory) / f'{name}.yaml'
                    path.write_text(compat.render(family), encoding='utf-8')
                    compat.check(family, path)

    def test_every_line_fits_80_columns(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                text = compat.render(support.load_any(name))
                self.assertLessEqual(max(map(len, text.splitlines())), 80)

    def test_an_enum_count_and_a_byte_count_keep_their_spelling(self) -> None:
        family = fixture_with('size: uint(bytes)', 'size: uint(..4K, bytes)')
        text = compat.render(family)
        self.assertIn('max-occurrences: mode', text)
        self.assertIn('max: 4K, unit: bytes', text)


class TestRegistryCompatibility(unittest.TestCase):
    @staticmethod
    def registry(*entries: str | tuple[str, bool]) -> spec.Registry:
        normalized = []
        for entry in entries:
            if isinstance(entry, tuple):
                normalized.append(spec.RegistryEntry(entry[0], entry[1]))
            else:
                normalized.append(spec.RegistryEntry(entry, False))
        return spec.Registry('test', tuple(normalized))

    def problems(self, old: spec.Registry, current: spec.Registry) -> str:
        return '\n'.join(compat.registry_problems(old, current))

    def test_removal_is_rejected(self) -> None:
        old = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True))
        current = self.registry('ctlv', 'ctlv-a')

        self.assertIn('family ID 2 (ctlv-b) was removed',
                      self.problems(old, current))

    def test_reorder_rename_and_reuse_are_rejected(self) -> None:
        old = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True))

        reordered = self.registry('ctlv', 'ctlv-b', ('ctlv-a', True))
        self.assertIn('family ID 1 was ctlv-a, now ctlv-b',
                      self.problems(old, reordered))
        renamed = self.registry('ctlv', 'ctlv-c', ('ctlv-b', True))
        self.assertIn('family ID 1 was ctlv-a, now ctlv-c',
                      self.problems(old, renamed))
        reused = self.registry('ctlv', 'ctlv-a', 'ctlv-c')
        self.assertIn('family ID 2 was ctlv-b, now ctlv-c',
                      self.problems(old, reused))
        undeprecated = self.registry('ctlv', 'ctlv-a', 'ctlv-b')
        self.assertIn('was reused after deprecation',
                      self.problems(old, undeprecated))

    def test_deprecation_and_append_are_compatible_updates(self) -> None:
        old = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True))
        deprecated = self.registry('ctlv', ('ctlv-a', True),
                                   ('ctlv-b', True))
        appended = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True),
                                 'ctlv-c')

        self.assertEqual('', self.problems(old, deprecated))
        self.assertEqual('', self.problems(old, appended))
        self.assertEqual(3, appended.family_id('ctlv-c'))


def problems_against(family: semantics.Family,
                     mutate: Callable[[dict[str, Any]], None]) -> str | None:
    """Check family against its own snapshot, changed by mutate.

    Return what the checker reports, or None for a compatible change.
    """
    was = copy.deepcopy(compat.build(family))
    mutate(was)
    with tempfile.TemporaryDirectory() as directory:
        path = pathlib.Path(directory) / f'{family.name}.yaml'
        path.write_text(compat.render_data(was), encoding='utf-8')
        try:
            compat.check(family, path)
        except CtlvError as error:
            return str(error)
    return None


def fixture_with(old: str, new: str) -> semantics.Family:
    """The fixture family, with old in its specification replaced by new."""
    text = (support.FIXTURE_SPEC_DIR / 'ctlv-fixture.yaml').read_text(
        encoding='utf-8')
    assert old in text
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        (root / 'specs').mkdir()
        (root / 'families.yaml').write_text(
            support.FIXTURE_REGISTRY.read_text(encoding='utf-8'),
            encoding='utf-8')
        path = root / 'specs' / 'ctlv-fixture.yaml'
        path.write_text(text.replace(old, new), encoding='utf-8')
        return support.load_fixture(path)


class TestIncompatibleChanges(unittest.TestCase):
    """The snapshot is what the ABI was; the family is what it is now."""

    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')

    def check_against(self, mutate: Callable[[dict[str, Any]], None]) -> str:
        problems = problems_against(self.family, mutate)
        assert problems is not None, 'the change was accepted'
        return problems

    def test_a_reordered_registry_moves_a_family_id(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['family-id'] = 2

        self.assertIn('family-id was 2, now 1', self.check_against(mutate))

    def test_a_renamed_family(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['family'] = 'ctlv-fixture-old'

        self.assertIn('family was renamed from ctlv-fixture-old to '
                      'ctlv-fixture', self.check_against(mutate))

    def test_a_removed_field(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['serial'] = {
                'number': 10, 'kind': 'uint', 'required': False,
                'max-occurrences': 1}

        self.assertIn('field scalar.serial was removed',
                      self.check_against(mutate))

    def test_a_removed_record(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['sensor'] = {'record-id': 0x00050000,
                                        'fields': {}, 'members': {}}

        self.assertIn('record sensor was removed', self.check_against(mutate))

    def test_a_renumbered_field(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['freq']['number'] = 7

        self.assertIn('field scalar.freq: number was 7, now 2',
                      self.check_against(mutate))

    def test_a_renumbered_record(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar-pair']['record-id'] = 0x00050000

        self.assertIn('record scalar-pair: ID was 0x00050000, now '
                      '0x00040000', self.check_against(mutate))

    def test_a_moved_unnamed_record(self) -> None:
        """Its path is part of every ID below it."""
        def mutate(was: dict[str, Any]) -> None:
            was['records']['records-dump level1']['record-id'] = 0xc0042000

        self.assertIn('record records-dump level1: ID was 0xc0042000, now '
                      '0xc0041000', self.check_against(mutate))

    def test_a_changed_type(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['u']['fields']['u']['kind'] = 'sint'

        self.assertIn('field u.u: kind was sint, now uint',
                      self.check_against(mutate))

    def test_a_changed_unit_or_scale(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['freq']['scale'] = -6

        self.assertIn('field scalar.freq: scale was -6, now 3',
                      self.check_against(mutate))

    def test_a_changed_kind_is_reported_as_one(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['mode'] = {
                'number': 4, 'kind': 'uint', 'required': True, 'min': 0,
                'max': 20, 'max-occurrences': 1,
            }

        problems = self.check_against(mutate)
        self.assertIn('field scalar.mode: kind was uint, now enum', problems)
        self.assertNotIn('narrowed', problems)

    def test_a_narrowed_maximum(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['text']['max-length'] = 255

        self.assertIn('field scalar.text: max-length was narrowed from 255 '
                      'to 15', self.check_against(mutate))

    def test_a_raised_minimum(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['u']['fields']['u']['min'] = 0

        self.assertIn('field u.u: min was narrowed from 0 to 1',
                      self.check_against(mutate))

    def test_a_narrowed_repetition(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            request = was['records']['counts-get request']
            request['fields']['few']['max-occurrences'] = 4
            reply = was['records']['records-dump reply']
            reply['fields']['level1']['max-occurrences'] = spec.UNLIMITED

        problems = self.check_against(mutate)
        self.assertIn('field counts-get request.few: max-occurrences was '
                      'narrowed from 4 to 3', problems)
        self.assertIn('field records-dump reply.level1: max-occurrences was '
                      'narrowed from unlimited to 2', problems)

    def test_a_count_that_now_names_an_enum(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            request = was['records']['counts-get request']
            request['fields']['per-mode']['max-occurrences'] = 3

        self.assertIn('field counts-get request.per-mode: max-occurrences was '
                      '3, now mode', self.check_against(mutate))

    def test_a_removed_blob_backing(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            request = was['records']['blobs-submit request']
            request['fields']['output-fd']['blob']['backings'].append('va')

        self.assertIn('field blobs-submit request.output-fd: blob backing va '
                      'was removed', self.check_against(mutate))

    def test_a_renumbered_enum_value(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['definitions']['mode']['values']['enabled'] = 3

        self.assertIn('enum mode: value enabled was 3, now 2',
                      self.check_against(mutate))

    def test_a_removed_enum_value(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['definitions']['mode']['values']['draining'] = 4

        self.assertIn('enum mode: value draining was removed',
                      self.check_against(mutate))

    def test_a_header_end_moved_earlier(self) -> None:
        family = fixture_with('$end: HASH_ALGO__LAST',
                              '$end: HASH_ALGO_SHA256')

        def mutate(was: dict[str, Any]) -> None:
            header = was['definitions']['hash-algo']['header']
            header['$end'] = 'HASH_ALGO__LAST'

        problems = problems_against(family, mutate)
        assert problems is not None, 'the change was accepted'
        self.assertIn('enum hash-algo: $end was HASH_ALGO__LAST, and may '
                      'only move later', problems)

    def test_a_header_end_added(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['definitions']['hash-algo']['header']['$end']

        self.assertIn('enum hash-algo: $end HASH_ALGO__LAST leaves values out',
                      self.check_against(mutate))

    def test_another_header_enum(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            header = was['definitions']['hash-algo']['header']
            header['$enum'] = 'hash_algo_old'

        self.assertIn('enum hash-algo was enum hash_algo_old of '
                      '<linux/hash_info.h>, now enum hash_algo of '
                      '<linux/hash_info.h>', self.check_against(mutate))

    def test_values_now_from_a_header(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['definitions']['hash-algo']['header']

        self.assertIn('enum hash-algo now takes its values from '
                      '<linux/hash_info.h>', self.check_against(mutate))

    def test_a_removed_op(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['thing-stopped'] = {
                'op-id': 9, 'type': 'action', 'availability': 'fixed',
                'optional': False, 'pre-post': False,
                'request': 'thing-stopped request', 'reply': None,
                'fd-access': 'read-write'}

        self.assertIn('op thing-stopped was removed',
                      self.check_against(mutate))

    def test_a_renumbered_op(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['records-dump']['op-id'] = 6

        self.assertIn('op records-dump: op-id was 6, now 4',
                      self.check_against(mutate))

    def test_a_changed_op_type(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['records-dump']['type'] = 'action'

        self.assertIn('op records-dump: type was action, now query',
                      self.check_against(mutate))

    def test_a_withdrawn_availability(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['noop']['availability'] = 'dynamic'

        self.assertIn('op noop: availability was dynamic, now fixed',
                      self.check_against(mutate))

    def test_a_changed_fd_access(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['noop']['fd-access'] = 'read-write'
            was['ops']['scalars-set']['fd-access'] = 'read'

        problems = self.check_against(mutate)
        self.assertIn('op noop: fd-access was read-write, now read', problems)
        self.assertIn('op scalars-set: fd-access was read, now read-write',
                      problems)

    def test_a_granted_availability(self) -> None:
        """A caller built against a fixed op never expects -EOPNOTSUPP."""
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['counts-get']['availability'] = 'fixed'

        self.assertIn('op counts-get: availability was fixed, now dynamic',
                      self.check_against(mutate))

    def test_a_withdrawn_hook_pair(self) -> None:
        """Taking a pair away leaves a family that no longer compiles."""
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['noop']['pre-post'] = True

        self.assertIn('op noop: pre-post was True, now False',
                      self.check_against(mutate))

    def test_a_granted_hook_pair(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['counts-get']['pre-post'] = False

        self.assertIn('op counts-get: pre-post was False, now True',
                      self.check_against(mutate))

    def test_a_reparented_family(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['parent'] = 'ctlv-fixture-old'

        self.assertIn('the inherited family was ctlv-fixture-old, now ctlv',
                      self.check_against(mutate))

    def test_a_changed_extension_point(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            reply = was['records']['records-dump reply']
            reply['fields']['extension']['extension'] = 'old-extension'
            was['extensions']['old-extension'] = 2

        problems = self.check_against(mutate)
        self.assertIn('field records-dump reply.extension: extension was '
                      'old-extension, now extension-point', problems)
        self.assertIn('extension point old-extension was removed', problems)

    def test_a_renumbered_extension_point(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['extensions']['extension-point'] = 2

        self.assertIn('extension point extension-point: ID was 2, now 1',
                      self.check_against(mutate))

    def test_a_changed_enum_extension_point(self) -> None:
        """Its ID is apart from the record extension points'."""
        def mutate(was: dict[str, Any]) -> None:
            was['enum-extensions']['variant'] = 2
            was['enum-extensions']['old-variant'] = 1

        problems = self.check_against(mutate)
        self.assertIn('enum extension point variant: ID was 2, now 1',
                      problems)
        self.assertIn('enum extension point old-variant was removed',
                      problems)

    def test_an_optional_field_that_became_required(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['delta']['required'] = False
            was['records']['scalar']['members']['record s'] = False

        problems = self.check_against(mutate)
        self.assertIn('field scalar.delta became required', problems)
        self.assertIn('record scalar: record s became required', problems)

    def test_a_reply_field_that_became_optional(self) -> None:
        """A program may count on what a reply always carried."""
        def mutate(was: dict[str, Any]) -> None:
            reply = was['records']['records-dump reply']
            reply['fields']['extension']['required'] = True

        self.assertIn('field records-dump reply.extension became optional',
                      self.check_against(mutate))

    def test_a_new_request_field_must_be_optional(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['records']['scalar']['fields']['bounded']
            del was['records']['optional-check request']['members'][
                'record u']

        problems = self.check_against(mutate)
        self.assertIn('record scalar: new field bounded must be optional',
                      problems)
        self.assertIn('record optional-check request: new record u must be '
                      'optional', problems)

    def test_a_new_reply_field_must_be_optional(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['records']['records-dump reply']['fields']['level1']

        self.assertIn('record records-dump reply: new field level1 must be '
                      'optional', self.check_against(mutate))

    def test_a_removed_placed_record(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['noop request']['members']['record u'] = False
            level1 = was['records']['records-dump level1']
            level1['members']['record s'] = False

        problems = self.check_against(mutate)
        self.assertIn('record noop request: record u was removed', problems)
        self.assertIn('record records-dump level1: record s was removed',
                      problems)

    def test_a_removed_reply_schema(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['noop']['reply'] = 'noop reply'

        self.assertIn('op noop: the reply schema was removed',
                      self.check_against(mutate))

    def test_an_ack_action_cannot_gain_a_reply(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['scalars-set']['reply'] = None

        self.assertIn('op scalars-set: an ACK-only action gained a reply '
                      'schema', self.check_against(mutate))

    def test_a_request_moved_to_another_record(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['ops']['scalars-set']['request'] = 'scalars-set request'

        self.assertIn('op scalars-set: the request was record scalars-set '
                      'request, now scalar', self.check_against(mutate))

    def test_every_problem_is_reported_at_once(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['family-id'] = 9
            was['ops']['records-dump']['op-id'] = 9

        problems = self.check_against(mutate)
        self.assertIn('family-id was 9, now 1', problems)
        self.assertIn('op records-dump: op-id was 9, now 4', problems)


class TestFillCompatibility(unittest.TestCase):
    """A fill is a record of the child, checked as one."""

    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture-child')

    def check_against(self, mutate: Callable[[dict[str, Any]], None]) -> str:
        problems = problems_against(self.family, mutate)
        assert problems is not None, 'the change was accepted'
        return problems

    def test_a_removed_fill_member(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['extension-point']['members']['record u'] = False

        self.assertIn('record extension-point: record u was removed',
                      self.check_against(mutate))

    def test_a_fill_member_that_became_required(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            fields = was['records']['extension-point']['fields']
            fields['tag']['required'] = False

        self.assertIn('field extension-point.tag became required',
                      self.check_against(mutate))

    def test_a_fill_member_that_became_optional(self) -> None:
        """The fill is part of the replies its extension point is in."""
        def mutate(was: dict[str, Any]) -> None:
            members = was['records']['extension-point']['members']
            members['record serial'] = True

        self.assertIn('record extension-point: record serial became optional',
                      self.check_against(mutate))

    def test_an_extension_filled_by_another_record(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['fills']['extension-point'] = 'old-fill'
            was['fills']['other-extension'] = 'other-fill'

        problems = self.check_against(mutate)
        self.assertIn('extension fill extension-point was record old-fill, '
                      'now extension-point', problems)
        self.assertIn('extension fill other-extension was removed', problems)

    def test_an_appended_optional_fill_member(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['records']['extension-point']['members']['record serial']

        self.assertIsNone(problems_against(self.family, mutate))

    def test_an_enum_fill_is_append_only(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            values = was['definitions']['variant']['values']
            values['plain'] = 2
            values['old'] = 3

        problems = self.check_against(mutate)
        self.assertIn('enum variant: value plain was 2, now 1', problems)
        self.assertIn('enum variant: value old was removed', problems)

    def test_an_appended_enum_fill_value(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['definitions']['variant']['values']['fancy']

        self.assertIsNone(problems_against(self.family, mutate))


class TestCompatibleChanges(unittest.TestCase):
    """Growth that userspace built against the snapshot still tolerates."""

    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')

    def check_against(self, mutate: Callable[[dict[str, Any]], None]) -> None:
        self.assertIsNone(problems_against(self.family, mutate))

    def test_an_appended_optional_request_field(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['records']['counts-get request']['fields']['few']

        self.check_against(mutate)

    def test_a_relaxed_requiredness(self) -> None:
        """Of a request, which a program built against it still fills."""
        def mutate(was: dict[str, Any]) -> None:
            request = was['records']['counts-get request']
            request['fields']['few']['required'] = True

        self.check_against(mutate)

    def test_a_widened_bound(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            was['records']['scalar']['fields']['text']['max-length'] = 8
            was['records']['u']['fields']['u']['min'] = 2

        self.check_against(mutate)

    def test_a_header_end_moved_later(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            header = was['definitions']['hash-algo']['header']
            header['$end'] = 'HASH_ALGO_SHA3_512'

        self.check_against(mutate)

    def test_a_widened_repetition(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            request = was['records']['counts-get request']
            request['fields']['few']['max-occurrences'] = 2
            reply = was['records']['counts-get reply']
            reply['fields']['many']['max-occurrences'] = 1

        self.check_against(mutate)

    def test_an_appended_record_and_op(self) -> None:
        def mutate(was: dict[str, Any]) -> None:
            del was['records']['pinged payload']
            del was['ops']['pinged']

        self.check_against(mutate)


if __name__ == '__main__':
    unittest.main()
