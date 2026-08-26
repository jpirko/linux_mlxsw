# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Strict YAML subset loader.

Anchors, aliases, merge keys, explicit tags, duplicate mapping keys and
multiple documents per file are all rejected: they let two readers of a file
that defines an ABI disagree about what it says.
"""

import yaml

from .errors import CtlvError


def _mark(path, mark):
    if mark is None:
        return str(path)
    return f'{path}:{mark.line + 1}'


class _StrictLoader(yaml.SafeLoader):
    """SafeLoader without the features that make a document ambiguous."""

    def __init__(self, stream, path):
        super().__init__(stream)
        self.ctlv_path = path

    def _reject(self, mark, what):
        raise CtlvError(_mark(self.ctlv_path, mark), what)

    def compose_node(self, parent, index):
        event = self.peek_event()
        if isinstance(event, yaml.events.AliasEvent):
            self._reject(event.start_mark, 'YAML aliases are not allowed')
        if getattr(event, 'anchor', None) is not None:
            self._reject(event.start_mark, 'YAML anchors are not allowed')
        if _has_explicit_tag(event):
            self._reject(event.start_mark,
                         f'explicit YAML tag "{event.tag}" is not allowed')
        return super().compose_node(parent, index)

    def construct_mapping(self, node, deep=False):
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


def _has_explicit_tag(event):
    if event.tag is None:
        return False
    implicit = getattr(event, 'implicit', False)
    if isinstance(implicit, tuple):
        return not (implicit[0] or implicit[1])
    return not implicit


def load(path):
    """Return the single YAML document in path as plain Python objects."""
    try:
        with open(path, 'r', encoding='utf-8') as stream:
            loader = _StrictLoader(stream, path)
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

    if not isinstance(data, dict):
        raise CtlvError(str(path), 'expected a YAML mapping at the top level')
    return data
