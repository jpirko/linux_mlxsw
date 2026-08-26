# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Every file in bad-specs/ must be rejected with the diagnostic it names.

A case carries an "# expect:" line whose text must appear in the diagnostic; a
file that a case needs but that must load carries "# fixture:" instead. A case
is rejected the way --validate rejects it, C names built and all.
"""

import re
import unittest

from tests import support

from ctlvgen.lib.errors import CtlvError

EXPECT = re.compile(r'^#\s*expect:\s*(.+)$', re.M)
FIXTURE = re.compile(r'^#\s*fixture:', re.M)


class TestBadSpecs(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.paths = sorted(support.BAD_SPEC_DIR.glob('*.yaml'))

    def read(self, path):
        return path.read_text(encoding='utf-8', errors='replace')

    def test_the_corpus_is_not_empty(self):
        self.assertTrue(self.paths)

    def test_every_file_declares_what_it_is(self):
        for path in self.paths:
            with self.subTest(case=path.name):
                text = self.read(path)
                self.assertTrue(EXPECT.search(text) or FIXTURE.search(text),
                                'neither an expect nor a fixture line')

    def test_every_case_is_rejected_with_its_diagnostic(self):
        for path in self.paths:
            expected = EXPECT.search(self.read(path))
            if expected is None:
                continue
            with self.subTest(case=path.name):
                with self.assertRaises(CtlvError) as caught:
                    support.validate_fixture(path)
                self.assertIn(expected.group(1).strip(),
                              str(caught.exception))

    def test_every_fixture_loads(self):
        for path in self.paths:
            if FIXTURE.search(self.read(path)) is None:
                continue
            with self.subTest(fixture=path.name):
                support.load_fixture(path)


if __name__ == '__main__':
    unittest.main()
