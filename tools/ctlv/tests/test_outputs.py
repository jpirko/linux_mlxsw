# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Whole-tree and generated-output completeness checks."""

from __future__ import annotations

import pathlib
import shutil
import subprocess
import tempfile
import unittest

from tests import support

from ctlvgen.lib import compat
from ctlvgen.lib import outputs
from ctlvgen.lib import spec
from ctlvgen.lib import tree
from ctlvgen.lib.errors import CtlvError


class TestTreeCoverage(unittest.TestCase):
    def test_specs_and_snapshots_come_from_the_registry(self) -> None:
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
            paths = spec.Paths(spec_dir / 'ctlv.yaml', abi=abi_dir)

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
    paths: spec.Paths

    @classmethod
    def setUpClass(cls) -> None:
        cls.paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')

    def test_manifest_is_complete_and_unique(self) -> None:
        plan = outputs.load(self.paths)

        self.assertEqual(37, len(plan))
        self.assertEqual(37, len({item.path for item in plan}))
        self.assertEqual(37, len({(item.source, item.flavour)
                                 for item in plan}))
        registry_outputs = [item for item in plan if item.family is None]
        self.assertEqual(
            [('Documentation/ctlv/families.yaml',
              'include/uapi/ctlv/ctlv_families_gen.h'),
             ('tools/ctlv/tests/families.yaml',
              'lib/ctlv/tests/ctlv/ctlv_families_gen.h')],
            [(str(item.source), str(item.path))
             for item in registry_outputs])

    def test_manifest_paths_cannot_name_repository_metadata(self) -> None:
        with self.assertRaisesRegex(CtlvError,
                                    'not a normalized relative path'):
            outputs._relative('test', '.git/generated.c')

    def test_one_missing_output_is_detected_and_recreated(self) -> None:
        victim = pathlib.Path('lib/ctlv/tests/'
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

    def test_an_unplanned_marked_output_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            outputs.reconcile(self.paths, root)
            source = root / 'include/uapi/ctlv/ctlv_gen.h'
            unexpected = root / 'unexpected.c'
            shutil.copyfile(source, unexpected)

            with self.assertRaisesRegex(CtlvError,
                                        'unexpected files'):
                outputs.reconcile(self.paths, root, check=True)

    def test_a_stale_output_is_updated_only_with_its_marker(self) -> None:
        victim = pathlib.Path('include/uapi/ctlv/ctlv_families_gen.h')
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


def git(root: pathlib.Path, *args: str) -> None:
    subprocess.run(['git', '-c', 'user.name=ctlv',
                    '-c', 'user.email=ctlv@localhost',
                    '-c', 'commit.gpgsign=false', *args],
                   cwd=root, capture_output=True, check=True)


class TestRegeneration(unittest.TestCase):
    """The ABI snapshots are regenerated with the rest, keeping the ABI."""

    paths: spec.Paths
    victim: pathlib.PurePosixPath = pathlib.PurePosixPath(
        'Documentation/ctlv/abi/ctlv-test.yaml')
    problem: str = 'ctlv-test: op noop: fd-access was read-write, now read'

    @classmethod
    def setUpClass(cls) -> None:
        cls.paths = spec.Paths(support.SPEC_DIR / 'ctlv.yaml')

    @staticmethod
    def broken() -> str:
        """ctlv-test's snapshot as if noop were read-write before."""
        was = compat.build(support.load('ctlv-test'))
        was['ops']['noop']['fd-access'] = 'read-write'
        return compat.render_data(was)

    def test_a_missing_snapshot_is_detected_and_recreated(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            tree.update(self.paths, root)
            self.assertEqual(
                (support.KERNEL_DIR / self.victim).read_text(encoding='utf-8'),
                (root / self.victim).read_text(encoding='utf-8'))
            tree.check(self.paths, root)
            (root / self.victim).unlink()

            with self.assertRaisesRegex(CtlvError, f'missing {self.victim}'):
                tree.check(self.paths, root)
            self.assertIn(f'missing {self.victim}',
                          tree.update(self.paths, root))
            tree.check(self.paths, root)

    def test_a_break_is_written_only_when_meant(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            tree.update(self.paths, root)
            path = root / self.victim
            path.write_text(self.broken(), encoding='utf-8')

            with self.assertRaisesRegex(CtlvError, self.problem):
                tree.check(self.paths, root)
            with self.assertRaisesRegex(CtlvError, 'ABI_BREAK=1'):
                tree.update(self.paths, root)
            self.assertEqual(self.broken(), path.read_text(encoding='utf-8'))

            self.assertEqual(
                (f'ABI break, {self.problem}', f'stale {self.victim}'),
                tree.update(self.paths, root, abi_break=True))
            tree.check(self.paths, root)

    @unittest.skipUnless(shutil.which('git'), 'needs git')
    def test_what_is_kept_is_what_was_committed(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            tree.update(self.paths, root)
            git(root, 'init', '-q')
            git(root, 'add', '-A')
            git(root, 'commit', '-q', '-m', 'regenerated')
            path = root / self.victim

            path.write_text(self.broken(), encoding='utf-8')
            self.assertEqual((f'stale {self.victim}',),
                             tree.update(self.paths, root))

            path.write_text(self.broken(), encoding='utf-8')
            git(root, 'commit', '-q', '-a', '-m', 'broken')
            with self.assertRaisesRegex(CtlvError, self.problem):
                tree.update(self.paths, root)


if __name__ == '__main__':
    unittest.main()
