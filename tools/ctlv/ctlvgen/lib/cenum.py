# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""The constants of an enum in a uapi header.

Only what a uapi enum needs is understood: integer literals, the enum's
earlier constants, parentheses and integer operators. Anything else, a macro
included, is refused rather than guessed at.
"""

from __future__ import annotations

import pathlib
import re
from typing import NoReturn

from .errors import CtlvError

_COMMENT = re.compile(r'/\*.*?\*/|//[^\n]*', re.S)
_TOKEN = re.compile(r'\s*(?:(?P<number>0[xX][0-9a-fA-F]+|[0-9]+)[uUlL]*|'
                    r'(?P<name>[A-Za-z_]\w*)|(?P<op><<|>>|[-+*/%&|^~()]))')
_IDENT = re.compile(r'[A-Za-z_]\w*')

# The binary operators by C precedence, loosest first.
_BINARY: tuple[tuple[str, ...], ...] = (
    ('|',), ('^',), ('&',), ('<<', '>>'), ('+', '-'), ('*', '/', '%'))


def read(where: str, include: pathlib.Path, header: str, tag: str,
         end: str | None) -> tuple[tuple[str, int], ...]:
    """Return enum tag of include/header, as constants before end."""
    try:
        text = (include / header).read_text(encoding='utf-8')
    except OSError:
        raise CtlvError(where, f'<{header}> is not a uapi header') from None
    return parse(where, text, f'<{header}>', tag, end)


def parse(where: str, text: str, header: str, tag: str,
          end: str | None) -> tuple[tuple[str, int], ...]:
    """Return enum tag of header text, as constants before end."""
    found = re.search(rf'\benum\s+{re.escape(tag)}\s*{{([^}}]*)}}',
                      _COMMENT.sub(' ', text))
    if found is None:
        raise CtlvError(where, f'{header} has no enum {tag}')
    constants: list[tuple[str, int]] = []
    numbers: dict[str, int] = {}
    number = -1
    for item in _items(found.group(1)):
        name, equals, value = (part.strip() for part in item.partition('='))
        if not _IDENT.fullmatch(name):
            raise CtlvError(where, f'enum {tag} has {item!r}, which is not a '
                                   'constant')
        if equals:
            number = _Expression(where, tag, name, value, numbers).value()
        else:
            number += 1
        if number < 0:
            raise CtlvError(where, f'{name} of enum {tag} is {number}, a '
                                   'negative value')
        constants.append((name, number))
        numbers[name] = number
    if end is None:
        return tuple(constants)
    if end not in numbers:
        raise CtlvError(where, f'enum {tag} has no {end}')
    kept = tuple(constants[:[name for name, _ in constants].index(end)])
    if not kept:
        raise CtlvError(where, f'enum {tag} has no constant before {end}')
    for name, number in kept:
        if number >= numbers[end]:
            raise CtlvError(where, f'{name} of enum {tag} is {number}, not '
                                   f'below {end}')
    return kept


def _items(body: str) -> list[str]:
    """The enumerators of an enum body, split at the commas outside ()."""
    items: list[str] = []
    depth = start = 0
    for index, char in enumerate(body):
        depth += {'(': 1, ')': -1}.get(char, 0)
        if char == ',' and depth == 0:
            items.append(body[start:index])
            start = index + 1
    items.append(body[start:])
    return [item.strip() for item in items if item.strip()]


class _Expression:
    """One constant's initializer, evaluated as C evaluates it."""

    def __init__(self, where: str, tag: str, name: str, text: str,
                 numbers: dict[str, int]) -> None:
        self.what = f'{name} of enum {tag}'
        self.where = where
        self.tag = tag
        self.numbers = numbers
        self.tokens: list[tuple[str, str]] = []
        position = 0
        text = text.rstrip()
        while position < len(text):
            found = _TOKEN.match(text, position)
            if found is None or found.lastgroup is None:
                self._fail(f'cannot read {text!r}')
            self.tokens.append((found.lastgroup,
                                found.group(found.lastgroup)))
            position = found.end()
        self.position = 0

    def _fail(self, problem: str) -> NoReturn:
        raise CtlvError(self.where, f'{self.what}: {problem}')

    def value(self) -> int:
        result = self._binary(0)
        if self.position != len(self.tokens):
            self._fail(f'cannot read {self.tokens[self.position][1]!r}')
        return result

    def _peek(self) -> str | None:
        if self.position == len(self.tokens):
            return None
        return self.tokens[self.position][1]

    def _next(self) -> tuple[str, str]:
        if self.position == len(self.tokens):
            self._fail('the value ends too soon')
        self.position += 1
        return self.tokens[self.position - 1]

    def _binary(self, level: int) -> int:
        if level == len(_BINARY):
            return self._unary()
        left = self._binary(level + 1)
        while self._peek() in _BINARY[level]:
            operator = self._next()[1]
            left = self._apply(operator, left, self._binary(level + 1))
        return left

    def _unary(self) -> int:
        kind, token = self._next()
        if token in ('-', '+', '~'):
            operand = self._unary()
            return {'-': -operand, '+': operand, '~': ~operand}[token]
        if token == '(':
            inner = self._binary(0)
            if self._next()[1] != ')':
                self._fail('a ( is not closed')
            return inner
        if kind == 'number':
            if len(token) > 1 and token[0] == '0' and token[1] not in 'xX':
                return int(token, 8)
            return int(token, 0)
        if kind == 'name' and token in self.numbers:
            return self.numbers[token]
        if kind == 'name':
            self._fail(f'{token} is not an earlier constant of enum '
                       f'{self.tag}')
        self._fail(f'cannot read {token!r}')

    def _apply(self, operator: str, left: int, right: int) -> int:
        if operator in ('/', '%'):
            if right == 0:
                self._fail('divides by zero')
            quotient = abs(left) // abs(right)
            if (left < 0) != (right < 0):
                quotient = -quotient
            return quotient if operator == '/' else left - right * quotient
        if operator in ('<<', '>>') and right < 0:
            self._fail('shifts by a negative count')
        return {
            '|': lambda: left | right,
            '^': lambda: left ^ right,
            '&': lambda: left & right,
            '<<': lambda: left << right,
            '>>': lambda: left >> right,
            '+': lambda: left + right,
            '-': lambda: left - right,
            '*': lambda: left * right,
        }[operator]()
