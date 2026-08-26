# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the strict YAML subset.

Each of these features lets two readers of one specification reach different
conclusions about an ABI, so all of them are refused.
"""

from __future__ import annotations

import pathlib
import tempfile
import unittest
from typing import Any

from ctlvgen.lib import yamlload
from ctlvgen.lib.errors import CtlvError


class TestYamlLoad(unittest.TestCase):
    def load(self, text: str) -> dict[str, Any]:
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / 'input.yaml'
            path.write_text(text, encoding='utf-8')
            return yamlload.load(path)

    def assert_rejected(self, text: str, expected: str) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load(text)
        self.assertIn(expected, str(caught.exception))

    def test_plain_mapping(self) -> None:
        self.assertEqual({'family': {'name': 'x'}},
                         self.load('family:\n  name: x\n'))

    def test_directive_and_document_start(self) -> None:
        self.assertEqual({'a': 1}, self.load('%YAML 1.2\n---\na: 1\n'))

    def test_anchor(self) -> None:
        self.assert_rejected('a: &base 1\nb: 2\n',
                             'YAML anchors are not allowed')

    def test_alias(self) -> None:
        # An alias needs an anchor, so the anchor is what is reported first.
        self.assert_rejected('a: &base {x: 1}\nb: *base\n',
                             'YAML anchors are not allowed')

    def test_merge_key(self) -> None:
        self.assert_rejected('a:\n  <<: {x: 1}\n  y: 2\n',
                             'YAML merge keys are not allowed')

    def test_explicit_tag(self) -> None:
        self.assert_rejected('a: !!str 1\n',
                             'explicit YAML tag "tag:yaml.org,2002:str" is '
                             'not allowed')

    def test_duplicate_key(self) -> None:
        self.assert_rejected('a: 1\na: 2\n', 'duplicate key "a"')

    def test_duplicate_key_reports_the_line(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            self.load('a: 1\nb: 2\nb: 3\n')
        self.assertTrue(str(caught.exception).split(': ')[0].endswith(':3'))

    def test_a_sequence_knows_the_line_of_each_item(self) -> None:
        data = self.load('a:\n  - x\n  # A comment.\n  - y\nb: [z, w]\n')
        self.assertEqual((['x', 'y'], [1, 3]), (data['a'], data['a'].lines))
        self.assertEqual([4, 4], data['b'].lines)

    def test_sequence_at_the_top_level(self) -> None:
        self.assert_rejected('- a\n- b\n',
                             'expected a YAML mapping at the top level')

    def test_two_documents(self) -> None:
        self.assert_rejected('a: 1\n---\nb: 2\n',
                             'expected a single document')

    def test_malformed(self) -> None:
        self.assert_rejected('a: [1\n', 'expected')

    def test_missing_file(self) -> None:
        with self.assertRaises(CtlvError) as caught:
            yamlload.load('/nonexistent/ctlv/spec.yaml')
        self.assertIn('cannot read', str(caught.exception))


if __name__ == '__main__':
    unittest.main()
