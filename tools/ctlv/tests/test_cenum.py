# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for reading the constants of an enum in a uapi header."""

from __future__ import annotations

import unittest

from tests import support

from ctlvgen.lib import cenum
from ctlvgen.lib.errors import CtlvError

UAPI = support.KERNEL_DIR / 'include' / 'uapi'


def parse(body: str, end: str | None = None) -> tuple[tuple[str, int], ...]:
    return cenum.parse('test', f'enum e {{{body}}};\n', '<test.h>', 'e', end)


class TestConstants(unittest.TestCase):
    def test_a_constant_counts_on_from_the_one_before(self) -> None:
        self.assertEqual((('A', 0), ('B', 1), ('C', 5), ('D', 6)),
                         parse('A, B, C = 5, D,'))

    def test_numbers_may_start_anywhere_and_leave_gaps(self) -> None:
        self.assertEqual((('A', 3), ('B', 4), ('C', 10)),
                         parse('A = 3, B, C = 10'))

    def test_an_alias_takes_the_number_of_its_constant(self) -> None:
        self.assertEqual((('A', 0), ('B', 1), ('C', 0)),
                         parse('A, B, C = A'))

    def test_an_initializer_is_evaluated_as_c_evaluates_it(self) -> None:
        self.assertEqual(
            (('A', 8), ('B', 11), ('C', 5), ('D', 0), ('E', 255), ('F', 1),
             ('G', 1), ('H', 4), ('I', 10)),
            parse('A = 1 << 3, B = (A | 2) + 1, C = 0x10 / 3, D = 017 % 5,'
                  'E = ~0 & 0xff, F = -7 / 2 + 4, G = -7 % 3 + 2, H = 4U,'
                  'I = 2 + 2 * 4'))

    def test_comments_are_not_constants(self) -> None:
        self.assertEqual((('A', 0), ('B', 1)),
                         parse('/* X, */ A, // Y,\n B'))

    def test_end_leaves_itself_and_every_later_constant_out(self) -> None:
        self.assertEqual((('A', 1), ('B', 2)),
                         parse('A = 1, B, NR, MAX = NR - 1', 'NR'))

    def test_a_uapi_header_is_read_from_the_tree(self) -> None:
        constants = cenum.read('test', UAPI, 'linux/hash_info.h',
                               'hash_algo', 'HASH_ALGO__LAST')
        self.assertEqual(('HASH_ALGO_MD4', 0), constants[0])
        self.assertNotIn('HASH_ALGO__LAST', dict(constants))


class TestRefusals(unittest.TestCase):
    def refused(self, problem: str, body: str, end: str | None = None) -> None:
        with self.assertRaisesRegex(CtlvError, problem):
            parse(body, end)

    def test_a_header_outside_uapi(self) -> None:
        with self.assertRaisesRegex(CtlvError,
                                    'crypto/hash_info.h> is not a uapi'):
            cenum.read('test', UAPI, 'crypto/hash_info.h', 'hash_algo', None)

    def test_a_missing_enum(self) -> None:
        with self.assertRaisesRegex(CtlvError, 'has no enum f'):
            cenum.parse('test', 'enum e { A };\n', '<test.h>', 'f', None)

    def test_a_missing_end(self) -> None:
        self.refused('enum e has no LAST', 'A, B', 'LAST')

    def test_nothing_before_end(self) -> None:
        self.refused('enum e has no constant before A', 'A, B', 'A')

    def test_a_macro(self) -> None:
        self.refused('BIT is not an earlier constant of enum e',
                     'A = BIT(3)')

    def test_a_later_constant(self) -> None:
        self.refused('B is not an earlier constant of enum e', 'A = B, B')

    def test_a_negative_value(self) -> None:
        self.refused('A of enum e is -1, a negative value', 'A = -1')

    def test_a_value_not_below_end(self) -> None:
        self.refused('A of enum e is 5, not below END', 'A = 5, END = 2',
                     'END')

    def test_what_is_not_an_expression(self) -> None:
        for body, problem in (('A = 1 +', 'ends too soon'),
                              ('A = (1', 'ends too soon'),
                              ('A = 1 / 0', 'divides by zero'),
                              ('A = 1 << -1', 'negative count'),
                              ('A = "x"', 'cannot read'),
                              ('A B', 'not a constant')):
            with self.subTest(body=body):
                self.refused(problem, body)


if __name__ == '__main__':
    unittest.main()
