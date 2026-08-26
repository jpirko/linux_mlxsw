# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Strict YAML subset loader.

Anchors, aliases, merge keys, explicit tags, duplicate mapping keys and
multiple documents per file are all rejected: they let two readers of a file
that defines an ABI disagree about what it says.
"""

from __future__ import annotations

import pathlib
from collections.abc import Hashable, Iterator
from typing import Any, NoReturn

import yaml

from .errors import CtlvError


class Mapping(dict[Any, Any]):
    """A mapping that knows the line each of its keys is on, from zero."""

    def __init__(self) -> None:
        super().__init__()
        self.lines: dict[Any, int] = {}


class Sequence(list[Any]):
    """A list that knows the line each of its items starts on, from zero."""

    def __init__(self) -> None:
        super().__init__()
        self.lines: list[int] = []


def _mark(path: pathlib.Path | str, mark: yaml.error.Mark | None) -> str:
    if mark is None:
        return str(path)
    return f'{path}:{mark.line + 1}'


class _StrictLoader(yaml.SafeLoader):
    """SafeLoader without the features that make a document ambiguous."""

    def __init__(self, stream: str, path: pathlib.Path | str) -> None:
        super().__init__(stream)
        self.ctlv_path = path

    def _reject(self, mark: yaml.error.Mark, what: str) -> NoReturn:
        raise CtlvError(_mark(self.ctlv_path, mark), what)

    def compose_node(self, parent: yaml.nodes.Node | None,
                     index: int) -> yaml.nodes.Node | None:
        event = self.peek_event()
        mark = event.start_mark
        if isinstance(event, yaml.events.AliasEvent):
            self._reject(mark, 'YAML aliases are not allowed')
        if getattr(event, 'anchor', None) is not None:
            self._reject(mark, 'YAML anchors are not allowed')
        if _has_explicit_tag(event):
            self._reject(mark,
                         f'explicit YAML tag "{event.tag}" is not allowed')
        return super().compose_node(parent, index)

    def construct_mapping(self, node: yaml.nodes.MappingNode,
                          deep: bool = False) -> dict[Hashable, Any]:
        seen = set()
        for key_node, _ in node.value:
            if key_node.tag == 'tag:yaml.org,2002:merge':
                self._reject(key_node.start_mark,
                             'YAML merge keys are not allowed')
            key = self.construct_object(key_node, deep=deep)
            if key in seen:
                self._reject(key_node.start_mark, f'duplicate key "{key}"')
            seen.add(key)
        return super().construct_mapping(node, deep=deep)


def _construct_map(loader: _StrictLoader,
                   node: yaml.nodes.MappingNode) -> Iterator[Mapping]:
    data = Mapping()
    yield data
    data.update(loader.construct_mapping(node))
    for key_node, _value in node.value:
        key = loader.construct_object(key_node)
        data.lines[key] = key_node.start_mark.line


def _construct_seq(loader: _StrictLoader,
                   node: yaml.nodes.SequenceNode) -> Iterator[Sequence]:
    data = Sequence()
    yield data
    data.extend(loader.construct_sequence(node))
    data.lines = [item.start_mark.line for item in node.value]


_StrictLoader.add_constructor('tag:yaml.org,2002:map', _construct_map)
_StrictLoader.add_constructor('tag:yaml.org,2002:seq', _construct_seq)


def _has_explicit_tag(event: yaml.events.Event) -> bool:
    if getattr(event, 'tag', None) is None:
        return False
    implicit = getattr(event, 'implicit', False)
    if isinstance(implicit, tuple):
        return not (implicit[0] or implicit[1])
    return not implicit


def load(path: pathlib.Path | str, text: str | None = None) -> Mapping:
    """Return the single YAML document in path, or in text read from it."""
    return load_with_lines(path, text)[0]


def load_with_lines(path: pathlib.Path | str,
                    text: str | None = None) -> tuple[Mapping, list[str]]:
    """Return the document in path, or in text, and the lines of its text."""
    try:
        if text is None:
            with open(path, 'r', encoding='utf-8') as stream:
                text = stream.read()
        loader = _StrictLoader(text, path)
        try:
            data = loader.get_single_data()
        finally:
            loader.dispose()
    except OSError as exc:
        raise CtlvError(str(path), f'cannot read: {exc.strerror}') from exc
    except yaml.YAMLError as exc:
        mark = getattr(exc, 'problem_mark', None)
        described = [getattr(exc, 'context', None),
                     getattr(exc, 'problem', None)]
        problem = ', '.join(part for part in described if part) or str(exc)
        raise CtlvError(_mark(path, mark), problem) from exc

    if not isinstance(data, Mapping):
        raise CtlvError(str(path), 'expected a YAML mapping at the top level')
    return data, text.splitlines()
