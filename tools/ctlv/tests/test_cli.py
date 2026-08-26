# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the command line contract of the CTLV schema tool.

The tool is run the way the build runs it, in its own process, which is also
how a dump's determinism is checked: two hash seeds, the same bytes.
"""

import json
import os
import subprocess
import sys
import unittest

from tests import support

CLI = support.TOOL_DIR / 'ctlvgen' / 'cli.py'

# argparse exit status for a usage error.
USAGE_ERROR = 2


def run_cli(*args, hash_seed=None):
    env = dict(os.environ)
    if hash_seed is not None:
        env['PYTHONHASHSEED'] = hash_seed
    return subprocess.run([sys.executable, str(CLI), *args],
                          capture_output=True, text=True, check=False,
                          env=env)


class TestUsage(unittest.TestCase):
    def test_help(self):
        result = run_cli('--help')
        self.assertEqual(0, result.returncode)
        self.assertIn('--spec', result.stdout)

    def test_spec_is_required(self):
        result = run_cli('--validate')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_action_is_required(self):
        result = run_cli('--spec', 'nonexistent.yaml')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_actions_are_exclusive(self):
        result = run_cli('--spec', 'nonexistent.yaml', '--validate', '--dump')
        self.assertEqual(USAGE_ERROR, result.returncode)

    def test_kernel_output_options_are_scoped(self):
        result = run_cli('--spec', 'nonexistent.yaml', '--mode', 'uapi',
                         '--header', '--schema-only')
        self.assertEqual(USAGE_ERROR, result.returncode)
        result = run_cli('--spec', 'nonexistent.yaml', '--mode', 'kernel',
                         '--rst')
        self.assertEqual(USAGE_ERROR, result.returncode)


class TestCheckedInSpecs(unittest.TestCase):
    """Every specification in the tree, registered or fixture.

    A fixture needs the meta-schema pointed at, as the tool's Makefile does it.
    """

    def args_of(self, name):
        if name in support.FAMILIES:
            return ('--spec', str(support.SPEC_DIR / f'{name}.yaml'))
        return ('--spec', str(support.FIXTURE_SPEC_DIR / f'{name}.yaml'),
                '--schema', str(support.SCHEMA))

    def test_every_spec_validates(self):
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                result = run_cli(*self.args_of(name), '--validate')
                self.assertEqual('', result.stderr)
                self.assertEqual(0, result.returncode)

    def test_every_registered_spec_matches_its_abi_snapshot(self):
        for name in support.FAMILIES:
            with self.subTest(family=name):
                result = run_cli(*self.args_of(name), '--abi-check')
                self.assertEqual('', result.stderr)
                self.assertEqual(0, result.returncode)

    def test_a_dump_does_not_depend_on_the_hash_seed(self):
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                first = run_cli(*self.args_of(name), '--dump', hash_seed='0')
                second = run_cli(*self.args_of(name), '--dump',
                                 hash_seed='12345')
                self.assertEqual(0, first.returncode)
                self.assertEqual(first.stdout, second.stdout)
                self.assertTrue(first.stdout)

    def test_runtime_manifest_names_the_effective_family(self):
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
        self.assertIn('child.extra',
                      [item['name'] for item in manifest['attrs']])
        definitions = {item['name']: item
                       for item in manifest['definitions']}
        quirks = {item['name']: item['value']
                  for item in definitions['echo-quirk']['values']}
        self.assertEqual({'slow': 1, 'noisy': 2}, quirks)

    def test_runtime_manifest_does_not_depend_on_the_hash_seed(self):
        for name in support.ALL_FAMILIES:
            with self.subTest(family=name):
                args = (*self.args_of(name), '--runtime-json')
                first = run_cli(*args, hash_seed='0')
                second = run_cli(*args, hash_seed='12345')
                self.assertEqual(0, first.returncode)
                self.assertEqual(first.stdout, second.stdout)


class TestDiagnostics(unittest.TestCase):
    def test_a_rejected_spec_fails_with_one_message(self):
        case = support.BAD_SPEC_DIR / 'ctlv-bad-unknown-attr.yaml'
        result = run_cli('--spec', str(case), '--schema',
                         str(support.SCHEMA), '--validate')
        self.assertEqual(1, result.returncode)
        self.assertEqual('', result.stdout)
        self.assertEqual(1, len(result.stderr.splitlines()))
        self.assertIn('unknown attr "thing.nope"', result.stderr)

    def test_a_missing_spec_is_reported_without_a_traceback(self):
        result = run_cli('--spec', 'nonexistent.yaml', '--validate')
        self.assertEqual(1, result.returncode)
        self.assertIn('cannot read', result.stderr)
        self.assertNotIn('Traceback', result.stderr)


if __name__ == '__main__':
    unittest.main()
