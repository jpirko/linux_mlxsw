# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the ABI snapshots and the compatibility checker.

Each case takes the current schema and a snapshot differing from it in exactly
one way, as the corresponding mistake would look after an edit.
"""

import copy
import pathlib
import tempfile
import unittest

from tests import support

from ctlvgen.lib import compat
from ctlvgen.lib import spec
from ctlvgen.lib.errors import CtlvError


class TestCheckedInSnapshots(unittest.TestCase):
    """A snapshot exists for a registered family and only for one."""

    def test_every_family_matches_its_snapshot(self):
        for name in support.FAMILIES:
            with self.subTest(family=name):
                family = support.load(name)
                compat.check(family, support.ABI_DIR / f'{name}.yaml')

    def test_a_rendered_snapshot_reloads_unchanged(self):
        for name in support.FAMILIES:
            with self.subTest(family=name):
                family = support.load(name)
                self.assertEqual(
                    compat.render(family),
                    (support.ABI_DIR / f'{name}.yaml').read_text(
                        encoding='utf-8'))

    def test_a_snapshot_contains_only_its_own_abi(self):
        parent = compat.build(support.load_family('ctlv-fixture'))
        child = compat.build(support.load_family('ctlv-fixture-child'))

        self.assertIn('nests-dump', parent['ops'])
        self.assertNotIn('nests-dump', child['ops'])
        self.assertNotIn('extension-point', parent['fills'])
        self.assertIn('extension-point', child['fills'])

    def test_no_snapshot_has_lost_its_specification(self):
        """A family cannot be dropped or renamed out of the tree.

        No comparison against a snapshot catches that, so the two directories
        are compared as well.
        """
        self.assertEqual(
            sorted(path.stem for path in support.SPEC_DIR.glob('*.yaml')),
            sorted(path.stem for path in support.ABI_DIR.glob('*.yaml')
                   if path.name != 'families.registry.yaml'))

    def test_the_registry_matches_its_baseline(self):
        paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')
        registry = spec.load_registry(paths)

        compat.registry_check(paths, registry)
        self.assertEqual(
            compat.registry_render(registry),
            paths.registry_abi.read_text(encoding='utf-8'))

    def test_a_fixture_family_has_no_snapshot(self):
        self.assertFalse((support.TESTS_DIR / 'abi').exists())


class TestRoundTrip(unittest.TestCase):
    """A rendered snapshot has to survive the strict loader and the checker.

    The fixtures included, whose snapshots go to a temporary file.
    """

    def test_a_render_reloads_and_still_matches_its_family(self):
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                family = support.load_any(name)
                with tempfile.TemporaryDirectory() as directory:
                    path = pathlib.Path(directory) / f'{name}.yaml'
                    path.write_text(compat.render(family), encoding='utf-8')
                    compat.check(family, path)


class TestRegistryCompatibility(unittest.TestCase):
    @staticmethod
    def registry(*entries):
        normalized = []
        for entry in entries:
            if isinstance(entry, tuple):
                normalized.append(spec.RegistryEntry(entry[0], entry[1]))
            else:
                normalized.append(spec.RegistryEntry(entry, False))
        return spec.Registry('test', tuple(normalized))

    def problems(self, old, current, exact=False):
        return '\n'.join(compat.registry_problems(old, current, exact))

    def test_removal_is_rejected(self):
        old = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True))
        current = self.registry('ctlv', 'ctlv-a')

        self.assertIn('family ID 2 (ctlv-b) was removed',
                      self.problems(old, current))

    def test_reorder_rename_and_reuse_are_rejected(self):
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

    def test_deprecation_and_append_are_compatible_updates(self):
        old = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True))
        deprecated = self.registry('ctlv', ('ctlv-a', True),
                                   ('ctlv-b', True))
        appended = self.registry('ctlv', 'ctlv-a', ('ctlv-b', True),
                                 'ctlv-c')

        self.assertEqual('', self.problems(old, deprecated))
        self.assertEqual('', self.problems(old, appended))
        self.assertIn('baseline needs updating',
                      self.problems(old, deprecated, exact=True))
        self.assertIn('baseline needs updating',
                      self.problems(old, appended, exact=True))
        self.assertEqual(3, appended.family_id('ctlv-c'))


class TestIncompatibleChanges(unittest.TestCase):
    """The snapshot is what the ABI was; the family is what it is now."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')

    def check_against(self, mutate):
        was = copy.deepcopy(compat.build(self.family))
        mutate(was)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'ctlv-fixture.yaml'
            path.write_text(compat.render_data(was), encoding='utf-8')
            with self.assertRaises(CtlvError) as caught:
                compat.check(self.family, path)
        return str(caught.exception)

    def test_a_reordered_registry_moves_a_family_id(self):
        def mutate(was):
            was['family-id'] = 2

        self.assertIn('family-id was 2, now 1', self.check_against(mutate))

    def test_a_renamed_family(self):
        def mutate(was):
            was['family'] = 'ctlv-fixture-old'

        self.assertIn('family was renamed from ctlv-fixture-old to '
                      'ctlv-fixture', self.check_against(mutate))

    def test_a_removed_attr(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['serial'] = {
                'local-id': 11, 'kind': 'uint', 'max-occurrences': 1}

        self.assertIn('attr scalar.serial was removed',
                      self.check_against(mutate))

    def test_a_removed_group(self):
        def mutate(was):
            was['groups']['sensor'] = {'family-id': 1, 'group-id': 5,
                                       'attrs': {}}

        self.assertIn('attr group sensor was removed',
                      self.check_against(mutate))

    def test_a_renumbered_attr(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['s']['local-id'] = 7

        self.assertIn('attr scalar.s: local-id was 7, now 2',
                      self.check_against(mutate))

    def test_a_renumbered_group(self):
        def mutate(was):
            was['groups']['count']['group-id'] = 5

        self.assertIn('attr group count: group-id was 5, now 2',
                      self.check_against(mutate))

    def test_a_changed_type(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['u']['kind'] = 'sint'

        self.assertIn('attr scalar.u: kind was sint, now uint',
                      self.check_against(mutate))

    def test_a_changed_unit_or_scale(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['freq']['scale'] = -6

        self.assertIn('attr scalar.freq: scale was -6, now 3',
                      self.check_against(mutate))

    def test_a_narrowed_maximum(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['text']['max-length'] = 255

        self.assertIn('attr scalar.text: max-length was narrowed from 255 '
                      'to 15', self.check_against(mutate))

    def test_a_raised_minimum(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['u']['min'] = 0

        self.assertIn('attr scalar.u: min was narrowed from 0 to 1',
                      self.check_against(mutate))

    def test_a_narrowed_repetition(self):
        def mutate(was):
            was['groups']['count']['attrs']['many']['max-occurrences'] = 1

        self.assertIn('attr count.many: max-occurrences was narrowed from '
                      '1 to unlimited', self.check_against(mutate))

    def test_a_removed_blob_backing(self):
        def mutate(was):
            blob = was['groups']['region']['attrs']['output-fd']['blob']
            blob['backings'].append('va')

        self.assertIn('attr region.output-fd: blob backing va was removed',
                      self.check_against(mutate))

    def test_a_renumbered_enum_value(self):
        def mutate(was):
            was['definitions']['mode']['values']['enabled'] = 3

        self.assertIn('enum mode: value enabled was 3, now 2',
                      self.check_against(mutate))

    def test_a_removed_enum_value(self):
        def mutate(was):
            was['definitions']['mode']['values']['draining'] = 4

        self.assertIn('enum mode: value draining was removed',
                      self.check_against(mutate))

    def test_a_renumbered_attr_set(self):
        def mutate(was):
            was['sets']['scalar-key']['set-id'] = 3

        self.assertIn('attr set scalar-key: set-id was 3, now 2',
                      self.check_against(mutate))

    def test_a_removed_op(self):
        def mutate(was):
            was['ops']['thing-stopped'] = {'family-id': 1, 'op-id': 9,
                                           'type': 'action', 'request': [],
                                           'reply': None}

        self.assertIn('op thing-stopped was removed',
                      self.check_against(mutate))

    def test_a_renumbered_op(self):
        def mutate(was):
            was['ops']['nests-dump']['op-id'] = 6

        self.assertIn('op nests-dump: op-id was 6, now 4',
                      self.check_against(mutate))

    def test_a_changed_op_type(self):
        def mutate(was):
            was['ops']['nests-dump']['type'] = 'action'

        self.assertIn('op nests-dump: type was action, now query',
                      self.check_against(mutate))

    def test_a_withdrawn_availability(self):
        def mutate(was):
            was['ops']['noop']['availability'] = 'dynamic'

        self.assertIn('op noop: availability was dynamic, now fixed',
                      self.check_against(mutate))

    def test_a_changed_fd_access(self):
        def mutate(was):
            was['ops']['noop']['fd-access'] = 'read-write'
            was['ops']['scalars-set']['fd-access'] = 'read'

        problems = self.check_against(mutate)
        self.assertIn('op noop: fd-access was read-write, now read', problems)
        self.assertIn('op scalars-set: fd-access was read, now read-write',
                      problems)

    def test_a_granted_availability(self):
        """A caller built against a fixed op never expects -EOPNOTSUPP."""
        def mutate(was):
            was['ops']['counts-get']['availability'] = 'fixed'

        self.assertIn('op counts-get: availability was fixed, now dynamic',
                      self.check_against(mutate))

    def test_a_withdrawn_hook_pair(self):
        """Taking a pair away leaves a family that no longer compiles."""
        def mutate(was):
            was['ops']['noop']['pre-post'] = True

        self.assertIn('op noop: pre-post was True, now False',
                      self.check_against(mutate))

    def test_a_granted_hook_pair(self):
        def mutate(was):
            was['ops']['counts-get']['pre-post'] = False

        self.assertIn('op counts-get: pre-post was False, now True',
                      self.check_against(mutate))

    def test_withdrawn_family_init_hooks(self):
        def mutate(was):
            was['pre-init'] = True
            was['post-init'] = True
            was['pre-fini'] = True

        problems = self.check_against(mutate)
        self.assertIn('family: pre-init was True, now False', problems)
        self.assertIn('family: post-init was True, now False', problems)
        self.assertIn('family: pre-fini was True, now False', problems)

    def test_a_reparented_family(self):
        def mutate(was):
            was['parent'] = 'ctlv-fixture-old'

        self.assertIn('the inherited family was ctlv-fixture-old, now ctlv',
                      self.check_against(mutate))

    def test_a_changed_stub_selection(self):
        def mutate(was):
            extension = was['ops']['nests-dump']['reply'][1]
            extension['children'][0]['stub'] = 'old-extension'

        self.assertIn('stub old-extension was removed',
                      self.check_against(mutate))

    def test_an_optional_request_attr_that_became_required(self):
        def mutate(was):
            was['ops']['scalars-set']['request'][1]['required'] = False

        self.assertIn('op scalars-set request: scalar.s became required',
                      self.check_against(mutate))

    def test_a_new_request_attr_must_be_optional(self):
        def mutate(was):
            del was['ops']['collapse-check']['request'][0]

        self.assertIn('op collapse-check request: new scalar.u must be '
                      'optional', self.check_against(mutate))

    def test_a_new_reply_attr_must_be_optional(self):
        def mutate(was):
            del was['ops']['nests-dump']['reply'][0]

        self.assertIn('op nests-dump reply: new nest.level1 must be optional',
                      self.check_against(mutate))

    def test_a_removed_request_attr(self):
        def mutate(was):
            was['ops']['noop']['request'].append(
                {'attr': 'scalar.mode', 'required': False,
                 'max-occurrences': 1})

        self.assertIn('op noop request: scalar.mode was removed',
                      self.check_against(mutate))

    def test_a_removed_nested_attr(self):
        def mutate(was):
            level1 = was['ops']['nests-dump']['reply'][0]
            level1['children'].append({'attr': 'scalar.text',
                                       'required': False,
                                       'max-occurrences': 1})

        self.assertIn('op nests-dump reply nest.level1: scalar.text was '
                      'removed', self.check_against(mutate))

    def test_a_removed_reply_schema(self):
        def mutate(was):
            was['ops']['noop']['reply'] = [
                {'attr': 'scalar.u', 'required': True,
                 'max-occurrences': 1}]

        self.assertIn('op noop: the reply schema was removed',
                      self.check_against(mutate))

    def test_an_ack_action_cannot_gain_a_reply(self):
        def mutate(was):
            was['ops']['scalars-set']['reply'] = None

        self.assertIn('op scalars-set: an ACK-only action gained a reply '
                      'schema', self.check_against(mutate))

    def test_every_problem_is_reported_at_once(self):
        def mutate(was):
            was['family-id'] = 9
            was['ops']['nests-dump']['op-id'] = 9

        problems = self.check_against(mutate)
        self.assertIn('family-id was 9, now 1', problems)
        self.assertIn('op nests-dump: op-id was 9, now 4', problems)


class TestFillCompatibility(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture-child')

    def check_against(self, mutate):
        was = copy.deepcopy(compat.build(self.family))
        mutate(was)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'ctlv-fixture-child.yaml'
            path.write_text(compat.render_data(was), encoding='utf-8')
            with self.assertRaises(CtlvError) as caught:
                compat.check(self.family, path)
        return str(caught.exception)

    def test_a_removed_fill_member(self):
        def mutate(was):
            was['fills']['extension-point'].append({
                'attr': 'child.dump',
                'required': False,
            })

        self.assertIn('stub fill extension-point: child.dump was removed',
                      self.check_against(mutate))

    def test_a_fill_member_that_became_required(self):
        def mutate(was):
            was['fills']['extension-point'][0]['required'] = False

        self.assertIn('stub fill extension-point: child.tag became required',
                      self.check_against(mutate))


class TestCompatibleChanges(unittest.TestCase):
    """Growth that userspace built against the snapshot still tolerates."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')

    def check_against(self, mutate):
        was = copy.deepcopy(compat.build(self.family))
        mutate(was)
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'ctlv-fixture.yaml'
            path.write_text(compat.render_data(was), encoding='utf-8')
            compat.check(self.family, path)

    def test_an_appended_optional_request_attr(self):
        def mutate(was):
            request = was['ops']['counts-get']['request']
            self.assertEqual('count.few', request[0]['attr'])
            del request[0]

        self.check_against(mutate)

    def test_a_relaxed_requiredness(self):
        def mutate(was):
            was['ops']['nests-dump']['reply'][1]['required'] = True
            was['ops']['counts-get']['request'][0]['required'] = True

        self.check_against(mutate)

    def test_a_widened_bound(self):
        def mutate(was):
            was['groups']['scalar']['attrs']['text']['max-length'] = 8
            was['groups']['scalar']['attrs']['u']['min'] = 2

        self.check_against(mutate)


if __name__ == '__main__':
    unittest.main()
