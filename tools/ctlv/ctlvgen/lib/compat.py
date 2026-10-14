# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""ABI snapshots and the compatibility checker.

A change is compatible only if the new schema still contains everything the
checked-in snapshot describes, with the same IDs, no narrowed bound, no
member that became required and no member of a reply that became optional.
A new member has to be optional. Regenerating
rewrites a snapshot, refusing an incompatible one unless the break is meant,
so the snapshot's diff is how the change gets reviewed.

The snapshot is keyed by record path. A record's ID comes from where it is in
the spec and a field's type from its record and number, so the checker only
compares them; it assigns nothing.
"""

from __future__ import annotations

import pathlib
import re
from typing import Any

import yaml

from . import cgen
from . import semantics
from . import spec
from . import yamlload
from .errors import CtlvError

HEADER: str = '''\
# SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
%YAML 1.2
---
# Checked-in ABI snapshot of the {name} family, from
# {spec}, written by
# "make -C tools/ctlv regen".
#
# Updating the file is how a UAPI change gets reviewed; an update that
# removes or renumbers anything, narrows a bound, or makes a member required
# is an ABI break, whatever the file says afterwards.
'''

REGISTRY_HEADER: str = '''\
# SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
%YAML 1.2
---
# Checked-in ABI baseline of the ordered CTLV family registry.
# Written by "make -C tools/ctlv regen".
'''

SNAPSHOT_INDENT: int = 2

_SIZE: re.Pattern[str] = re.compile(r'(?P<value>[0-9]+)(?P<suffix>[KMG])')

# The bounds that are byte counts, which a snapshot writes as a spec does.
_LENGTHS: tuple[str, ...] = ('min-length', 'max-length', 'length', 'alignment')


class _Dumper(yaml.SafeDumper):
    """Dumper that never emits anchors, which the strict loader rejects."""

    def ignore_aliases(self, data: object) -> bool:
        return True

    def increase_indent(self, flow: bool = False,
                        indentless: bool = False) -> None:
        # Indent a sequence under its key, not flush left as pyyaml does.
        return super().increase_indent(flow, False)


class _FlowMap(dict[str, Any]):
    pass


class _FlowSeq(list[str]):
    pass


class _Hex(int):
    pass


_Dumper.add_representer(
    _FlowMap,
    lambda dumper, value: dumper.represent_mapping(
        'tag:yaml.org,2002:map', value, flow_style=True))
_Dumper.add_representer(
    _FlowSeq,
    lambda dumper, value: dumper.represent_sequence(
        'tag:yaml.org,2002:seq', value, flow_style=True))
_Dumper.add_representer(
    _Hex,
    lambda dumper, value: dumper.represent_scalar(
        'tag:yaml.org,2002:int', f'{value:#010x}'))


def build(family: semantics.Family) -> dict[str, Any]:
    """Return one family's ABI in the comparison form."""
    return {
        'family': family.name,
        'family-id': family.family_id,
        'parent': family.parent,
        'spec': cgen.spec_path(family.path),
        'definitions': _definitions(family.local_definitions),
        'records': _records(family),
        'extensions': {extension.name: extension.ext_id
                       for extension in family.local_extensions},
        'enum-extensions': {extension.name: extension.ext_id
                            for extension in family.local_enum_extensions},
        'fills': {fill.name: fill.record.path for fill in family.fills},
        'ops': _ops(family.local_ops),
    }


def render(family: semantics.Family) -> str:
    """Return one family's compact snapshot as YAML text."""
    return render_data(build(family))


def render_data(data: dict[str, Any]) -> str:
    """Render comparison-form data as a compact snapshot."""
    body = yaml.dump(_compact(data), Dumper=_Dumper, sort_keys=False,
                     default_flow_style=False, width=1000000,
                     indent=SNAPSHOT_INDENT)
    body = '\n'.join(line for source in body.splitlines()
                     for line in _wrap_flow(source)) + '\n'
    return HEADER.format(name=data['family'], spec=data['spec']) + body


def _wrap_flow(line: str, width: int = 80) -> tuple[str, ...]:
    """Wrap one flow collection with continuations aligned after its opener.

    Where an item does not fit there, the collection starts a line of its own
    below its key.
    """
    if len(line) <= width:
        return (line,)
    starts = [index for index in (line.find('{'), line.find('['))
              if index >= 0]
    if not starts:
        return (line,)
    start = min(starts)
    end, items = _flow_items(line, start)
    if end is None:
        return (line,)

    wrapped = _wrap_items(line[:start + 1], line[end:], start + 1, items,
                          width)
    key = line[:start].rstrip()
    if max(map(len, wrapped)) <= width or not key.endswith(':'):
        return wrapped
    indent = len(line) - len(line.lstrip()) + SNAPSHOT_INDENT
    return (key,) + _wrap_items(' ' * indent + line[start], line[end:],
                                indent + 1, items, width)


def _wrap_items(prefix: str, suffix: str, indent_width: int,
                items: tuple[str, ...], width: int) -> tuple[str, ...]:
    indent = ' ' * indent_width
    wrapped = []
    current = prefix
    for index, item in enumerate(items):
        last = index + 1 == len(items)
        token = item + ('' if last else ',')
        separator = '' if current.endswith(('{', '[')) else ' '
        closing = len(suffix) if last else 0
        if len(current) + len(separator) + len(token) + closing > width:
            wrapped.append(current)
            current = indent + token
        else:
            current += separator + token
    current += suffix
    wrapped.append(current)
    return tuple(wrapped)


def _flow_items(line: str,
                start: int) -> tuple[int | None, tuple[str, ...]]:
    pairs = {'{': '}', '[': ']'}
    stack = [line[start]]
    quote = None
    escape = False
    item_start = start + 1
    items = []
    for index in range(start + 1, len(line)):
        char = line[index]
        if quote is not None:
            if escape:
                escape = False
            elif char == '\\':
                escape = True
            elif char == quote:
                quote = None
            continue
        if char in ('"', "'"):
            quote = char
        elif char in pairs:
            stack.append(char)
        elif char in pairs.values():
            if pairs[stack[-1]] != char:
                return None, ()
            stack.pop()
            if not stack:
                items.append(line[item_start:index].strip())
                return index, tuple(item for item in items if item)
        elif char == ',' and len(stack) == 1:
            items.append(line[item_start:index].strip())
            item_start = index + 1
    return None, ()


def breaks(family: semantics.Family, path: pathlib.Path | str,
           text: str | None = None) -> list[str]:
    """Every way family breaks the snapshot in path, or the one in text."""
    return _compare(_expand(yamlload.load(path, text)), build(family),
                    _reply_records(family))


def check(family: semantics.Family, path: pathlib.Path) -> None:
    """Compare a family against its snapshot, or raise with every problem."""
    problems = breaks(family, path)
    if problems:
        raise CtlvError(f'{family.name} against {path}',
                        'the schema is not compatible with its snapshot\n' +
                        '\n'.join(f'  {problem}' for problem in problems))


# The comparison form, from a resolved family.

def _definitions(
        items: tuple[semantics.Definition, ...]) -> dict[str, dict[str, Any]]:
    return {definition.name: _definition(definition)
            for definition in items if not definition.is_extension_point}


def _definition(definition: semantics.Definition) -> dict[str, Any]:
    entry: dict[str, Any] = {'kind': definition.kind,
                             'values': dict(definition.values)}
    if definition.header:
        entry['header'] = _header_keys(definition.header)
    return entry


def _header_keys(header: spec.HeaderEnum) -> dict[str, str]:
    keys = {'$header': header.path, '$enum': header.tag}
    if header.end:
        keys['$end'] = header.end
    return keys


def _records(family: semantics.Family) -> dict[str, dict[str, Any]]:
    records = {}
    for record in family.local_records:
        own = {field.type for field in record.fields}
        records[record.path] = {
            'record-id': record.record_id,
            'fields': {member.field.name: _field(member.field,
                                                 member.required)
                       for member in record.members
                       if member.field.type in own},
            'members': _foreign(family, record, own),
        }
    return records


def _foreign(family: semantics.Family, record: semantics.Record,
             own: set[int]) -> dict[str, bool]:
    """What a record has that it does not declare: placed records and includes.

    An include is one member, optional when it made a field optional that the
    record it splices requires.
    """
    members = {}
    for member in record.members:
        field = member.field
        if field.type in own:
            continue
        if field.shared:
            members[f'record {field.name}'] = member.required
            continue
        key = f'$include {field.record}'
        source = family.records[field.record]
        declared = any(item.required for item in source.members
                       if item.field.type == field.type)
        members.setdefault(key, True)
        if declared and not member.required:
            members[key] = False
    return members


def _reply_records(family: semantics.Family) -> set[str]:
    """The paths of the records a reply of the chain carries, fills too."""
    owners = {field.type: record.path for record in family.records.values()
              for field in record.fields}
    found: set[str] = set()

    def walk(node: semantics.Node) -> None:
        for extension in node.extensions:
            fill = family.fill_of(extension)
            if fill is not None:
                found.add(fill.record.path)
        for child, _optional in semantics.composed_members(family, node):
            found.add(owners[child.child_field.type])
            if child.child_field.holds is not None:
                found.add(child.child_field.holds)
            walk(child)

    for op in family.ops:
        if op.reply is not None:
            assert op.reply_record is not None
            found.add(op.reply_record.path)
            walk(op.reply)
    return found


def _field(field: semantics.Field, required: bool) -> dict[str, Any]:
    decl = field.decl
    entry = {'number': field.number, 'kind': decl.kind,
             'required': required,
             'max-occurrences': decl.count_enum or decl.max_occurrences}
    if decl.kind in ('enum', 'flags'):
        entry['definition'] = decl.ref
    if decl.kind == 'record':
        if decl.extension is not None:
            entry['extension'] = decl.extension
        else:
            entry['record'] = field.holds
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
    return entry


def _ops(items: tuple[semantics.Op, ...]) -> dict[str, dict[str, Any]]:
    ops = {}
    for op in items:
        entry = {
            'op-id': op.local_id,
            'type': op.type,
            'availability': op.availability,
            'optional': op.optional,
            'pre-post': op.pre_post,
            'request': op.request_record.path if op.request_record else None,
            'reply': op.reply_record.path if op.reply_record else None,
        }
        if op.type != 'event':
            entry['fd-access'] = op.fd_access
        ops[op.name] = entry
    return ops


# The compact form a snapshot is written in.

def _compact(data: dict[str, Any]) -> dict[str, Any]:
    family = _FlowMap({
        'name': data['family'],
        'id': data['family-id'],
    })
    if data['parent'] is not None:
        family['inherits'] = data['parent']

    compact: dict[str, Any] = {'family': family}
    for kind, section in (('enum', 'enums'), ('flags', 'flags')):
        definitions = {
            name: _FlowMap(entry.get('header', entry['values']))
            for name, entry in data['definitions'].items()
            if entry['kind'] == kind
        }
        if definitions:
            compact[section] = definitions
    if data['records']:
        compact['records'] = {path: _compact_record(record)
                              for path, record in data['records'].items()}
    if data['extensions']:
        compact['extensions'] = _FlowMap(data['extensions'])
    if data['enum-extensions']:
        compact['enum-extensions'] = _FlowMap(data['enum-extensions'])
    if data['fills']:
        compact['fills'] = _FlowMap(data['fills'])
    if data['ops']:
        compact['ops'] = {name: _compact_op(entry)
                          for name, entry in data['ops'].items()}
    return compact


def _compact_record(record: dict[str, Any]) -> dict[str, Any]:
    compact: dict[str, Any] = {'id': _Hex(record['record-id'])}
    if record['fields']:
        compact['fields'] = {name: _compact_field(field)
                             for name, field in record['fields'].items()}
    if record['members']:
        compact['members'] = _FlowSeq(
            key + ('' if required else '?')
            for key, required in record['members'].items())
    return compact


def _compact_field(field: dict[str, Any]) -> _FlowMap:
    kind = field['kind']
    compact = _FlowMap({'number': field['number']})
    if kind in ('enum', 'flags'):
        compact[kind] = field['definition']
    elif kind == 'blob':
        blob = field['blob']
        compact['blob'] = blob['direction']
        compact['backings'] = _FlowSeq(blob['backings'])
        compact['max-length'] = _size(blob['max-length'])
        if blob['alignment'] != 1:
            compact['alignment'] = _size(blob['alignment'])
    elif kind == 'record':
        if 'extension' in field:
            compact['extension'] = field['extension']
        else:
            compact['record'] = field['record']
    else:
        compact['type'] = kind

    if kind in ('uint', 'sint'):
        low, high = _integer_extremes(kind)
        bound = _size if field.get('unit') == 'bytes' else _integer
        if field.get('min', low) != low:
            compact['min'] = bound(field['min'])
        if field.get('max', high) != high:
            compact['max'] = bound(field['max'])
        if 'unit' in field:
            compact['unit'] = field['unit']
            if field['scale']:
                compact['scale'] = field['scale']
    elif kind in ('string', 'binary'):
        minimum = field.get('min-length', 0)
        maximum = field['max-length']
        if kind == 'binary' and minimum == maximum:
            compact['length'] = _size(maximum)
        else:
            if minimum:
                compact['min-length'] = _size(minimum)
            compact['max-length'] = _size(maximum)

    maximum = field['max-occurrences']
    if maximum != 1:
        compact['max-occurrences'] = (
            'unlimited' if maximum == spec.UNLIMITED else maximum)
    if not field['required']:
        compact['optional'] = True
    return compact


def _integer(value: int) -> int | str:
    """An integer bound as a spec writes it, a plain one as a number."""
    text = spec.number_text(value)
    return value if text == str(value) else text


def _size(value: int) -> int | str:
    """A byte count as a spec writes it, a plain one as a number."""
    text = spec.size_text(value)
    return value if text == str(value) else text


def _integer_extremes(kind: str) -> tuple[int, int]:
    return (0, spec.U64_MAX) if kind == 'uint' else (spec.S64_MIN,
                                                     spec.S64_MAX)


def _compact_op(op: dict[str, Any]) -> _FlowMap:
    abi = _FlowMap({
        'id': op['op-id'],
        'type': op['type'],
    })
    if (op['type'] != 'event' and
            op.get('fd-access') != spec.DEFAULT_FD_ACCESS):
        abi['fd-access'] = op.get('fd-access')
    availability = op.get('availability', spec.DEFAULT_AVAILABILITY)
    if availability != spec.DEFAULT_AVAILABILITY:
        abi['availability'] = availability
    if op.get('optional', False):
        abi['optional'] = True
    if op.get('pre-post', False):
        abi['pre-post'] = True
    if op.get('request') is not None:
        abi['request'] = op['request']
    if op.get('reply') is not None:
        abi['payload' if op['type'] == 'event' else 'reply'] = op['reply']
    return abi


def _expand(compact: dict[str, Any]) -> dict[str, Any]:
    family = compact['family']
    definitions = {}
    for kind, section in (('enum', 'enums'), ('flags', 'flags')):
        for name, values in compact.get(section, {}).items():
            definitions[name] = {
                'kind': kind,
                'values': {} if '$header' in values else values,
            }
            if '$header' in values:
                definitions[name]['header'] = dict(values)
    return {
        'family': family['name'],
        'family-id': family['id'],
        'parent': family.get('inherits'),
        'definitions': definitions,
        'records': {path: _expand_record(path, record)
                    for path, record in compact.get('records', {}).items()},
        'extensions': dict(compact.get('extensions', {})),
        'enum-extensions': dict(compact.get('enum-extensions', {})),
        'fills': dict(compact.get('fills', {})),
        'ops': _expand_ops(compact.get('ops', {})),
    }


def _expand_record(path: str, compact: dict[str, Any]) -> dict[str, Any]:
    members = {}
    for item in compact.get('members', []):
        members[item.removesuffix('?')] = not item.endswith('?')
    return {
        'record-id': compact['id'],
        'fields': {name: _expand_field(f'{path}.{name}', field)
                   for name, field in compact.get('fields', {}).items()},
        'members': members,
    }


def _expand_field(where: str, compact: dict[str, Any]) -> dict[str, Any]:
    primary = [name for name in ('type', 'enum', 'flags', 'blob', 'record',
                                 'extension')
               if name in compact]
    if len(primary) != 1:
        raise CtlvError(f'ABI snapshot field {where}',
                        'expected one of type, enum, flags, blob, record or '
                        'extension')
    kind = primary[0]
    if kind == 'type':
        kind = compact['type']
    elif kind in ('record', 'extension'):
        kind = 'record'
    field = {
        'number': compact['number'],
        'kind': kind,
        'required': not compact.get('optional', False),
        'max-occurrences': compact.get('max-occurrences', 1),
    }
    if field['max-occurrences'] == 'unlimited':
        field['max-occurrences'] = spec.UNLIMITED
    if kind in ('enum', 'flags'):
        field['definition'] = compact[kind]
    elif kind == 'record':
        for key in ('record', 'extension'):
            if key in compact:
                field[key] = compact[key]
    elif kind in ('uint', 'sint'):
        low, high = _integer_extremes(kind)
        field['min'] = _number(where, compact.get('min', low))
        field['max'] = _number(where, compact.get('max', high))
        if 'unit' in compact:
            field['unit'] = compact['unit']
            field['scale'] = compact.get('scale', 0)
    elif kind in ('string', 'binary'):
        if 'length' in compact:
            field['min-length'] = _number(where, compact['length'])
            field['max-length'] = field['min-length']
        else:
            field['min-length'] = _number(where,
                                          compact.get('min-length', 0))
            field['max-length'] = _number(where, compact['max-length'])
    elif kind == 'blob':
        field['blob'] = {
            'direction': compact['blob'],
            'backings': list(compact['backings']),
            'max-length': _number(where, compact['max-length']),
            'alignment': _number(where, compact.get('alignment', 1)),
        }
    return field


def _number(where: str, value: int | str) -> int:
    """A number as a snapshot writes it: plain, a limit's name or a size."""
    if isinstance(value, int):
        return value
    if value in spec.NAMED_NUMBERS:
        return spec.NAMED_NUMBERS[value]
    found = _SIZE.fullmatch(str(value))
    if found is None:
        raise CtlvError(f'ABI snapshot field {where}',
                        f'"{value}" is not a number')
    scale = dict(spec.SIZE_SUFFIXES)[found['suffix']]
    return int(found['value']) * scale


def _expand_ops(ops: dict[str, Any]) -> dict[str, dict[str, Any]]:
    expanded = {}
    for name, abi in ops.items():
        op_type = abi['type']
        entry = {
            'op-id': abi['id'],
            'type': op_type,
            'availability': abi.get(
                'availability', spec.DEFAULT_AVAILABILITY),
            'optional': abi.get('optional', False),
            'pre-post': abi.get('pre-post', False),
            'request': abi.get('request'),
            'reply': abi.get('payload' if op_type == 'event' else 'reply'),
        }
        if op_type != 'event':
            entry['fd-access'] = abi.get(
                'fd-access', spec.DEFAULT_FD_ACCESS)
        expanded[name] = entry
    return expanded


def registry_build(
        registry: spec.Registry) -> dict[str, list[str | dict[str, Any]]]:
    """Return the registry in the same normalized form as its source."""
    families: list[str | dict[str, Any]] = []
    for entry in registry.entries:
        if entry.deprecated:
            families.append({'name': entry.name, 'deprecated': True})
        else:
            families.append(entry.name)
    return {'families': families}


def registry_render(registry: spec.Registry) -> str:
    body = yaml.dump(registry_build(registry), Dumper=_Dumper,
                     sort_keys=False, default_flow_style=False, width=76)
    return REGISTRY_HEADER + body


def registry_problems(old: spec.Registry,
                      current: spec.Registry) -> list[str]:
    problems = []
    for family_id, was in enumerate(old.entries):
        if family_id >= len(current.entries):
            problems.append(f'family ID {family_id} ({was.name}) was removed')
            continue
        now = current.entries[family_id]
        if now.name != was.name:
            problems.append(f'family ID {family_id} was {was.name}, now '
                            f'{now.name}')
        elif was.deprecated and not now.deprecated:
            problems.append(f'family ID {family_id} ({was.name}) was reused '
                            'after deprecation')
    return problems


# The comparison.

def _compare(old: dict[str, Any], new: dict[str, Any],
             replies: set[str]) -> list[str]:
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
    _compare_records(problems, old['records'], new['records'], replies)
    _compare_extensions(problems, 'extension point', old['extensions'],
                        new['extensions'])
    _compare_extensions(problems, 'enum extension point',
                        old['enum-extensions'], new['enum-extensions'])
    _compare_fills(problems, old['fills'], new['fills'])
    _compare_ops(problems, old['ops'], new['ops'])
    return problems


def _compare_definitions(problems: list[str], old: dict[str, dict[str, Any]],
                         new: dict[str, dict[str, Any]]) -> None:
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'{was["kind"]} {name} was removed')
            continue
        where = f'{was["kind"]} {name}'
        if now['kind'] != was['kind']:
            problems.append(f'{where} is now a {now["kind"]} definition')
        if 'header' in was or 'header' in now:
            _compare_header(problems, where, was, now)
            continue
        for value, number in was['values'].items():
            if value not in now['values']:
                problems.append(f'{where}: value {value} was removed')
            elif now['values'][value] != number:
                problems.append(f'{where}: value {value} was {number}, now '
                                f'{now["values"][value]}')


def _compare_header(problems: list[str], where: str, was: dict[str, Any],
                    now: dict[str, Any]) -> None:
    """A header numbers its enum, so only its $end may change, to later."""
    old, new = was.get('header'), now.get('header')
    if old is None or new is None:
        source = new or old
        assert source is not None
        problems.append(f'{where} {"now" if old is None else "no longer"} '
                        f'takes its values from <{source["$header"]}>')
        return
    if (old['$header'], old['$enum']) != (new['$header'], new['$enum']):
        problems.append(f'{where} was enum {old["$enum"]} of '
                        f'<{old["$header"]}>, now enum {new["$enum"]} of '
                        f'<{new["$header"]}>')
        return
    end = old.get('$end')
    if end == new.get('$end'):
        return
    if end is None:
        problems.append(f'{where}: $end {new["$end"]} leaves values out')
    elif end not in now['values']:
        problems.append(f'{where}: $end was {end}, and may only move later')


def _compare_records(problems: list[str], old: dict[str, dict[str, Any]],
                     new: dict[str, dict[str, Any]],
                     replies: set[str]) -> None:
    for path, was in old.items():
        now = new.get(path)
        where = f'record {path}'
        if now is None:
            problems.append(f'{where} was removed')
            continue
        if was['record-id'] != now['record-id']:
            problems.append(f'{where}: ID was {was["record-id"]:#010x}, now '
                            f'{now["record-id"]:#010x}')
        reply = path in replies
        for name, field_was in was['fields'].items():
            field_now = now['fields'].get(name)
            if field_now is None:
                problems.append(f'field {path}.{name} was removed')
                continue
            _compare_field(problems, f'field {path}.{name}', field_was,
                           field_now, reply)
        for name, field_now in now['fields'].items():
            if name not in was['fields'] and field_now['required']:
                problems.append(f'{where}: new field {name} must be '
                                'optional')
        _compare_members(problems, where, was['members'], now['members'],
                         reply)


def _compare_field(problems: list[str], where: str, was: dict[str, Any],
                   now: dict[str, Any], reply: bool) -> None:
    for key in ('number', 'kind', 'definition', 'record', 'extension',
                'unit', 'scale'):
        if was.get(key) != now.get(key):
            problems.append(f'{where}: {key} was {was.get(key)}, now '
                            f'{now.get(key)}')
    if now['required'] and not was['required']:
        problems.append(f'{where} became required')
    if reply and was['required'] and not now['required']:
        problems.append(f'{where} became optional')
    # The bounds of one schema kind say nothing about another's.
    if was['kind'] != now['kind']:
        return
    for key in ('min', 'min-length'):
        if key in was and now.get(key, 0) > was[key]:
            problems.append(f'{where}: {key} was narrowed from '
                            f'{_text(key, was[key])} to '
                            f'{_text(key, now[key])}')
    for key in ('max', 'max-length'):
        if key in was and now.get(key, 0) < was[key]:
            problems.append(f'{where}: {key} was narrowed from '
                            f'{_text(key, was[key])} to '
                            f'{_text(key, now[key])}')
    _compare_occurrences(problems, where, was, now)
    if 'blob' in was:
        _compare_blob(problems, where, was['blob'], now.get('blob', {}))


def _text(key: str, value: int) -> str:
    return spec.size_text(value) if key in _LENGTHS else \
        spec.number_text(value)


def _compare_occurrences(problems: list[str], where: str, was: dict[str, Any],
                         now: dict[str, Any]) -> None:
    old_max = was['max-occurrences']
    new_max = now['max-occurrences']
    if old_max == new_max or new_max == spec.UNLIMITED:
        return
    # One enum's count only grows; another's, or a number, is not comparable.
    if isinstance(old_max, str) or isinstance(new_max, str):
        problems.append(f'{where}: max-occurrences was '
                        f'{_occurrences(old_max)}, now '
                        f'{_occurrences(new_max)}')
    elif old_max == spec.UNLIMITED or new_max < old_max:
        problems.append(f'{where}: max-occurrences was narrowed from '
                        f'{_occurrences(old_max)} to {_occurrences(new_max)}')


def _compare_blob(problems: list[str], where: str, was: dict[str, Any],
                  now: dict[str, Any]) -> None:
    for key in ('direction', 'alignment'):
        if was.get(key) != now.get(key):
            problems.append(f'{where}: blob {key} was {was.get(key)}, now '
                            f'{now.get(key)}')
    for backing in was.get('backings', []):
        if backing not in now.get('backings', []):
            problems.append(f'{where}: blob backing {backing} was removed')
    if now.get('max-length', 0) < was.get('max-length', 0):
        problems.append(f'{where}: blob max-length was narrowed from '
                        f'{spec.size_text(was["max-length"])} to '
                        f'{spec.size_text(now["max-length"])}')


def _compare_members(problems: list[str], where: str, old: dict[str, bool],
                     new: dict[str, bool], reply: bool) -> None:
    for key, required in old.items():
        if key not in new:
            problems.append(f'{where}: {key} was removed')
        elif new[key] and not required:
            problems.append(f'{where}: {key} became required')
        elif reply and required and not new[key]:
            problems.append(f'{where}: {key} became optional')
    for key, required in new.items():
        if key not in old and required:
            problems.append(f'{where}: new {key} must be optional')


def _compare_extensions(problems: list[str], what: str, old: dict[str, int],
                        new: dict[str, int]) -> None:
    for name, ext_id in old.items():
        if name not in new:
            problems.append(f'{what} {name} was removed')
        elif new[name] != ext_id:
            problems.append(f'{what} {name}: ID was {ext_id}, now '
                            f'{new[name]}')


def _compare_fills(problems: list[str], old: dict[str, str],
                   new: dict[str, str]) -> None:
    for name, path in old.items():
        if name not in new:
            problems.append(f'extension fill {name} was removed')
        elif new[name] != path:
            problems.append(f'extension fill {name} was record {path}, now '
                            f'{new[name]}')


def _compare_ops(problems: list[str], old: dict[str, dict[str, Any]],
                 new: dict[str, dict[str, Any]]) -> None:
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'op {name} was removed')
            continue
        where = f'op {name}'
        # Whether an op may be disabled or withheld is what userspace compiles
        # against; whether it is either right now is per device.
        for key in ('op-id', 'type', 'fd-access',
                    'availability', 'optional', 'pre-post'):
            if was.get(key) != now.get(key):
                problems.append(f'{where}: {key} was {was.get(key)}, now '
                                f'{now.get(key)}')
        for what in ('request', 'reply'):
            if (what == 'reply' and was['type'] == 'action' and
                    was[what] is None and now[what] is not None):
                problems.append(f'{where}: an ACK-only action gained a '
                                'reply schema')
            elif was[what] is not None and now[what] is None:
                problems.append(f'{where}: the {what} schema was removed')
            elif was[what] is not None and was[what] != now[what]:
                problems.append(f'{where}: the {what} was record '
                                f'{was[what]}, now {now[what]}')


def _occurrences(max_occurrences: int | str) -> str:
    if max_occurrences == spec.UNLIMITED:
        return 'unlimited'
    return str(max_occurrences)
