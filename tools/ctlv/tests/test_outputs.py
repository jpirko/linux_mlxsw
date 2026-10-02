# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Whole-tree and generated-output completeness checks."""

import pathlib
import shutil
import tempfile
import types
import unittest

from tests import support

from ctlvgen.lib import outputs
from ctlvgen.lib import spec
from ctlvgen.lib import tree
from ctlvgen.lib.errors import CtlvError


class TestTreeCoverage(unittest.TestCase):
    def test_specs_and_snapshots_come_from_the_registry(self):
        registry = spec.Registry('test', (
            spec.RegistryEntry('ctlv', False),
            spec.RegistryEntry('ctlv-a', False),
        ))
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            spec_dir = root / 'specs'
            abi_dir = root / 'abi'
            spec_dir.mkdir()
            abi_dir.mkdir()
            for name in ('ctlv', 'ctlv-a'):
                (spec_dir / f'{name}.yaml').touch()
                (abi_dir / f'{name}.yaml').touch()
            paths = types.SimpleNamespace(
                spec_dir=spec_dir, abi_dir=abi_dir,
                registry_abi=abi_dir / 'families.registry.yaml')

            self.assertEqual([], tree.coverage_problems(paths, registry))
            (spec_dir / 'ctlv-extra.yaml').touch()
            (abi_dir / 'ctlv-extra.yaml').touch()
            self.assertEqual(
                ['unexpected specification ctlv-extra.yaml',
                 'unexpected ABI snapshot ctlv-extra.yaml'],
                tree.coverage_problems(paths, registry))
            (spec_dir / 'ctlv-extra.yaml').unlink()
            (abi_dir / 'ctlv-extra.yaml').unlink()
            (spec_dir / 'ctlv-a.yaml').unlink()
            (abi_dir / 'ctlv-a.yaml').unlink()
            self.assertEqual(
                ['missing specification ctlv-a.yaml',
                 'missing ABI snapshot ctlv-a.yaml'],
                tree.coverage_problems(paths, registry))

            (spec_dir / 'ctlv-a.yaml').touch()
            (abi_dir / 'ctlv.yaml').unlink()
            self.assertEqual(
                ['missing ABI snapshot ctlv.yaml'],
                tree.coverage_problems(
                    paths, registry,
                    allowed_missing_snapshots={'ctlv-a'}))


class TestGeneratedOutputs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')

    def test_manifest_is_complete_and_unique(self):
        plan = outputs.load(self.paths)

        self.assertEqual(29, len(plan))
        self.assertEqual(29, len({item.path for item in plan}))
        self.assertEqual(29, len({(item.family, item.flavour)
                                 for item in plan}))
        registry_outputs = [item for item in plan if item.family is None]
        self.assertEqual(
            [('Documentation/ctlv/families.yaml',
              'include/uapi/linux/ctlv_families_gen.h'),
             ('tools/ctlv/tests/families.yaml',
              'drivers/ctlv/tests/linux/ctlv_families_gen.h')],
            [(str(item.source), str(item.path))
             for item in registry_outputs])

    def test_manifest_paths_cannot_name_repository_metadata(self):
        with self.assertRaisesRegex(CtlvError,
                                    'not a normalized relative path'):
            outputs._relative('test', '.git/generated.c')

    def test_one_missing_output_is_detected_and_recreated(self):
        victim = pathlib.Path('drivers/ctlv/tests/'
                              'ctlv_fixture_gen.h')
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            outputs.reconcile(self.paths, root)
            (root / victim).unlink()

            with self.assertRaisesRegex(CtlvError,
                                        f'missing {victim}') as caught:
                outputs.reconcile(self.paths, root, check=True)
            self.assertFalse((root / victim).exists())
            self.assertIn('generated outputs', str(caught.exception))

            outputs.reconcile(self.paths, root)
            self.assertTrue((root / victim).exists())
            outputs.reconcile(self.paths, root, check=True)

    def test_an_unplanned_marked_output_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            outputs.reconcile(self.paths, root)
            source = root / 'include/uapi/linux/ctlv_gen.h'
            unexpected = root / 'unexpected.c'
            shutil.copyfile(source, unexpected)

            with self.assertRaisesRegex(CtlvError,
                                        'unexpected files'):
                outputs.reconcile(self.paths, root, check=True)

    def test_a_stale_output_is_updated_only_with_its_marker(self):
        victim = pathlib.Path('include/uapi/linux/ctlv_families_gen.h')
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            outputs.reconcile(self.paths, root)
            path = root / victim
            generated = path.read_text(encoding='utf-8')
            path.write_text(generated + '\n', encoding='utf-8')

            with self.assertRaisesRegex(CtlvError, f'stale {victim}'):
                outputs.reconcile(self.paths, root, check=True)
            outputs.reconcile(self.paths, root)
            self.assertEqual(generated, path.read_text(encoding='utf-8'))

            path.write_text('ordinary source\n', encoding='utf-8')
            with self.assertRaisesRegex(CtlvError,
                                        'refusing to overwrite files'):
                outputs.reconcile(self.paths, root)
            self.assertEqual('ordinary source\n',
                             path.read_text(encoding='utf-8'))


if __name__ == '__main__':
    unittest.main()
