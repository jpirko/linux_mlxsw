# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""ABI snapshots and the compatibility checker.

A change is compatible only if the new schema still contains everything the
checked-in snapshot describes, with the same IDs, no narrowed bound and no
placement that became required. A new placement has to be optional. Updating a
snapshot is deliberate: it is how the change gets reviewed.
"""

from __future__ import annotations

import yaml

from . import spec
from . import yamlload
from .errors import CtlvError

HEADER = '''\
# SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
%YAML 1.2
---
# Checked-in ABI snapshot of the {name} family, written by
# "make -C tools/ctlv abi-update".
#
# The compatibility checker compares the specification against this file, so
# updating it is how a UAPI change gets reviewed. An update that removes or
# renumbers anything, narrows a bound, or makes a placement required is an ABI
# break, whatever this file says afterwards.
'''


class _Dumper(yaml.SafeDumper):
    """Dumper that never emits anchors, which the strict loader rejects."""

    def ignore_aliases(self, data):
        return True

    def increase_indent(self, flow=False, indentless=False):
        # Indent a sequence under its key, not flush left as pyyaml does.
        return super().increase_indent(flow, False)


def build(family):
    """Return the snapshot of one effective family as plain data."""
    return {
        'family': family.name,
        'family-id': family.family_id,
        'parent': family.parent,
        'definitions': _definitions(family),
        'groups': _groups(family),
        'sets': _sets(family),
        'ops': _ops(family),
    }


def render(family):
    """Return the snapshot of one effective family as YAML text."""
    body = yaml.dump(build(family), Dumper=_Dumper, sort_keys=False,
                     default_flow_style=False, width=76)
    return HEADER.format(name=family.name) + body


def check(family, path):
    """Compare a family against its snapshot, or raise with every problem."""
    snapshot = yamlload.load(path)
    problems = _compare(snapshot, build(family))
    if problems:
        raise CtlvError(f'{family.name} against {path}',
                        'the schema is not compatible with its snapshot\n' +
                        '\n'.join(f'  {problem}' for problem in problems))


def _definitions(family):
    definitions = {}
    for definition in family.definitions.values():
        definitions[definition.name] = {
            'kind': definition.kind,
            'family-id': definition.family_id,
            'values': dict(definition.values),
        }
    return definitions


def _groups(family):
    groups = {}
    for group in family.groups.values():
        groups[group.name] = {
            'family-id': group.family_id,
            'group-id': group.group_id,
            'attrs': {attr.name: _attr(attr) for attr in group.attrs},
        }
    return groups


def _attr(attr):
    decl = attr.decl
    entry = {'local-id': attr.local_id, 'kind': decl.kind}
    if decl.ref is not None:
        entry['definition'] = decl.ref
    for name, value in (('min', decl.min), ('max', decl.max),
                        ('min-length', decl.min_length),
                        ('max-length', decl.max_length),
                        ('unit', decl.unit), ('scale', decl.scale)):
        if value is not None:
            entry[name] = value
    if decl.blob is not None:
        entry['blob'] = {
            'direction': decl.blob.direction,
            'backings': list(decl.blob.backings),
            'max-length': decl.blob.max_length,
            'alignment': decl.blob.alignment,
        }
    entry['max-occurrences'] = decl.max_occurrences
    return entry


def _sets(family):
    return {attr_set.name: {'family-id': attr_set.family_id,
                            'set-id': attr_set.set_id}
            for attr_set in family.sets.values()}


def _ops(family):
    ops = {}
    for op in family.ops:
        ops[op.name] = {
            'family-id': op.family_id,
            'op-id': op.local_id,
            'type': op.type,
            'availability': op.availability,
            'pre-post': op.pre_post,
            'request': _tree(op.request),
            'reply': _tree(op.reply),
        }
    return ops


def _tree(root):
    if root is None:
        return None
    return [_node(child) for child in root.children()]


def _node(node):
    entry = {'attr': node.attr.full_name,
             'required': node.required,
             'max-occurrences': node.max_occurrences}
    if node.reenters is not None:
        # The members are recorded above, so record where they come from.
        entry['reenters'] = node.reenters
        return entry
    children = [_node(child) for child in node.children()]
    if children:
        entry['children'] = children
    return entry


def _compare(old, new):
    problems = []
    if old['family'] != new['family']:
        problems.append(f'family was renamed from {old["family"]} to '
                        f'{new["family"]}')
    if old['family-id'] != new['family-id']:
        problems.append(f'family-id was {old["family-id"]}, now '
                        f'{new["family-id"]}')
    # A name carries its ancestry, so reparenting renames every symbol.
    if old['parent'] != new['parent']:
        problems.append(f'the inherited family was {old["parent"]}, now '
                        f'{new["parent"]}')
    _compare_definitions(problems, old['definitions'], new['definitions'])
    _compare_groups(problems, old['groups'], new['groups'])
    _compare_sets(problems, old['sets'], new['sets'])
    _compare_ops(problems, old['ops'], new['ops'])
    return problems


def _compare_definitions(problems, old, new):
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'{was["kind"]} {name} was removed')
            continue
        where = f'{was["kind"]} {name}'
        if now['kind'] != was['kind']:
            problems.append(f'{where} is now a {now["kind"]} definition')
        if now['family-id'] != was['family-id']:
            problems.append(f'{where}: family-id was {was["family-id"]}, now '
                            f'{now["family-id"]}')
        for value, number in was['values'].items():
            if value not in now['values']:
                problems.append(f'{where}: value {value} was removed')
            elif now['values'][value] != number:
                problems.append(f'{where}: value {value} was {number}, now '
                                f'{now["values"][value]}')


def _compare_groups(problems, old, new):
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'attr group {name} was removed')
            continue
        where = f'attr group {name}'
        for key in ('group-id', 'family-id'):
            if was[key] != now[key]:
                problems.append(f'{where}: {key} was {was[key]}, now '
                                f'{now[key]}')
        for attr, attr_was in was['attrs'].items():
            attr_now = now['attrs'].get(attr)
            if attr_now is None:
                problems.append(f'attr {name}.{attr} was removed')
                continue
            _compare_attr(problems, f'attr {name}.{attr}', attr_was, attr_now)


def _compare_attr(problems, where, was, now):
    for key in ('local-id', 'kind', 'definition', 'unit', 'scale'):
        if was.get(key) != now.get(key):
            problems.append(f'{where}: {key} was {was.get(key)}, now '
                            f'{now.get(key)}')
    for key in ('min', 'min-length'):
        if key in was and now.get(key, 0) > was[key]:
            problems.append(f'{where}: {key} was narrowed from {was[key]} to '
                            f'{now[key]}')
    for key in ('max', 'max-length'):
        if key in was and now.get(key, 0) < was[key]:
            problems.append(f'{where}: {key} was narrowed from {was[key]} to '
                            f'{now[key]}')
    _compare_occurrences(problems, where, was, now)
    if 'blob' in was:
        _compare_blob(problems, where, was['blob'], now.get('blob', {}))


def _compare_occurrences(problems, where, was, now):
    old_max = was['max-occurrences']
    new_max = now['max-occurrences']
    if old_max == new_max:
        return
    if old_max == spec.UNLIMITED or new_max < old_max:
        problems.append(f'{where}: max-occurrences was narrowed from '
                        f'{_occurrences(old_max)} to {_occurrences(new_max)}')


def _compare_blob(problems, where, was, now):
    for key in ('direction', 'alignment'):
        if was.get(key) != now.get(key):
            problems.append(f'{where}: blob {key} was {was.get(key)}, now '
                            f'{now.get(key)}')
    for backing in was.get('backings', []):
        if backing not in now.get('backings', []):
            problems.append(f'{where}: blob backing {backing} was removed')
    if now.get('max-length', 0) < was.get('max-length', 0):
        problems.append(f'{where}: blob max-length was narrowed from '
                        f'{was["max-length"]} to {now["max-length"]}')


def _compare_sets(problems, old, new):
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'attr set {name} was removed')
            continue
        for key in ('family-id', 'set-id'):
            if was[key] != now[key]:
                problems.append(f'attr set {name}: {key} was {was[key]}, now '
                                f'{now[key]}')


def _compare_ops(problems, old, new):
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'op {name} was removed')
            continue
        where = f'op {name}'
        # Whether an op may be disabled is what userspace compiles against;
        # whether it is disabled right now is per device.
        for key in ('family-id', 'op-id', 'type', 'availability', 'pre-post'):
            if was[key] != now[key]:
                problems.append(f'{where}: {key} was {was[key]}, now '
                                f'{now[key]}')
        for what in ('request', 'reply'):
            if was[what] is not None and now[what] is None:
                problems.append(f'{where}: the {what} schema was removed')
            elif was[what] is not None:
                _compare_nodes(problems, f'{where} {what}', was[what],
                               now[what])


def _compare_nodes(problems, where, old, new):
    by_name = {node['attr']: node for node in new}
    for was in old:
        now = by_name.pop(was['attr'], None)
        if now is None:
            problems.append(f'{where}: {was["attr"]} was removed')
            continue
        if now['required'] and not was['required']:
            problems.append(f'{where}: {was["attr"]} became required')
        if was.get('reenters') != now.get('reenters'):
            problems.append(f'{where}: {was["attr"]} re-entered '
                            f'{was.get("reenters")}, now '
                            f'{now.get("reenters")}')
        _compare_occurrences(problems, f'{where} {was["attr"]}', was, now)
        _compare_nodes(problems, f'{where} {was["attr"]}',
                       was.get('children', []), now.get('children', []))
    for node in by_name.values():
        if node['required']:
            problems.append(f'{where}: new {node["attr"]} must be optional')


def _occurrences(max_occurrences):
    if max_occurrences == spec.UNLIMITED:
        return 'unlimited'
    return str(max_occurrences)
