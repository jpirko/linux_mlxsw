# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the command line contract of the CTLV schema tool.

The tool is run the way the build runs it, in its own process, which is also
how a dump's determinism is checked: two hash seeds, the same bytes.
"""

from __future__ import annotations

import json
import os
import pathlib
import subprocess
import sys
import unittest

from tests import support

CLI: pathlib.Path = support.TOOL_DIR / 'ctlvgen' / 'cli.py'

# argparse exit status for a usage error.
USAGE_ERROR: int = 2


def run_cli(*args: str,
            hash_seed: str | None = None) -> subprocess.CompletedProcess[str]:
    env = dict(os.environ)
    if hash_seed is not None:
        env['PYTHONHASHSEED'] = hash_seed
    return subprocess.run([sys.executable, str(CLI), *args],
                          capture_output=True, text=True, check=False,
                          env=env)


class TestUsage(unittest.TestCase):
    def test_help(self) -> None:
        result = run_cli('--help')
        self.assertEqual(0, result.returncode)
        self.assertIn('--spec', result.stdout)

    def test_spec_is_required(self) -> None:
        result = run_cli('--validate')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_action_is_required(self) -> None:
        result = run_cli('--spec', 'nonexistent.yaml')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_actions_are_exclusive(self) -> None:
        result = run_cli('--spec', 'nonexistent.yaml', '--validate', '--dump')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_kernel_output_options_are_scoped(self) -> None:
        result = run_cli('--spec', 'nonexistent.yaml', '--mode', 'uapi',
                         '--header', '--schema-only')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_a_mode_generates_only_its_kinds(self) -> None:
        for mode, kind in (('uapi', '--source'), ('uapi', '--rst'),
                           ('kernel', '--rst'), ('user', '--rst'),
                           ('doc', '--header'), ('doc', '--source')):
            with self.subTest(mode=mode, kind=kind):
                result = run_cli('--spec', 'nonexistent.yaml', '--mode',
                                 mode, kind)
                self.assertEqual(USAGE_ERROR, result.returncode)
                self.assertIn(f'--mode {mode} cannot generate {kind}',
                              result.stderr)


class TestCheckedInSpecs(unittest.TestCase):
    """Every specification in the tree, registered or fixture.

    A fixture needs the meta-schema pointed at, as the tool's Makefile does it.
    """

    def args_of(self, name: str) -> tuple[str, ...]:
        if name in support.FAMILIES:
            return ('--spec', str(support.SPEC_DIR / f'{name}.yaml'))
        return ('--spec', str(support.FIXTURE_SPEC_DIR / f'{name}.yaml'),
                '--schema', str(support.SCHEMA))

    def test_every_spec_validates(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                result = run_cli(*self.args_of(name), '--validate')
                self.assertEqual('', result.stderr)
                self.assertEqual(0, result.returncode)

    def test_a_dump_does_not_depend_on_the_hash_seed(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                first = run_cli(*self.args_of(name), '--dump', hash_seed='0')
                second = run_cli(*self.args_of(name), '--dump',
                                 hash_seed='12345')
                self.assertEqual(0, first.returncode)
                self.assertEqual(first.stdout, second.stdout)
                self.assertTrue(first.stdout)

    def test_runtime_manifest_names_the_effective_family(self) -> None:
        result = run_cli(*self.args_of('ctlv-test-child'),
                         '--runtime-json')
        self.assertEqual('', result.stderr)
        self.assertEqual(0, result.returncode)

        manifest = json.loads(result.stdout)
        self.assertEqual(1, manifest['version'])
        self.assertEqual('ctlv-test-child', manifest['family'])
        self.assertEqual(['ctlv', 'ctlv-test', 'ctlv-test-child'],
                         [item['name'] for item in manifest['chain']])
        self.assertIn('probe-get',
                      [item['name'] for item in manifest['ops']])
        self.assertIn('extension-point.extra',
                      [item['name'] for item in manifest['fields']])
        definitions = {item['name']: item
                       for item in manifest['definitions']}
        quirks = {item['name']: item['value']
                  for item in definitions['echo-quirk']['values']}
        self.assertEqual({'slow': 1, 'noisy': 2}, quirks)

    def test_runtime_manifest_does_not_depend_on_the_hash_seed(self) -> None:
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                args = (*self.args_of(name), '--runtime-json')
                first = run_cli(*args, hash_seed='0')
                second = run_cli(*args, hash_seed='12345')
                self.assertEqual(0, first.returncode)
                self.assertEqual(first.stdout, second.stdout)


class TestDiagnostics(unittest.TestCase):
    def test_a_rejected_spec_fails_with_one_message(self) -> None:
        case = support.BAD_SPEC_DIR / 'ctlv-bad-unknown-record.yaml'
        result = run_cli('--spec', str(case), '--schema',
                         str(support.SCHEMA), '--validate')
        self.assertEqual(1, result.returncode)
        self.assertEqual('', result.stdout)
        self.assertEqual(1, len(result.stderr.splitlines()))
        self.assertIn('no record is called nope', result.stderr)

    def test_a_missing_spec_is_reported_without_a_traceback(self) -> None:
        result = run_cli('--spec', 'nonexistent.yaml', '--validate')
        self.assertEqual(1, result.returncode)
        self.assertIn('cannot read', result.stderr)
        self.assertNotIn('Traceback', result.stderr)


if __name__ == '__main__':
    unittest.main()
