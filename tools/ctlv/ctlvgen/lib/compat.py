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
# The checker compares this family's declarations and inherited stub fills
# against this file. Updating it is how a UAPI change gets reviewed; an update
# that removes or renumbers anything, narrows a bound, or makes a placement
# required is an ABI break, whatever this file says afterwards.
'''

REGISTRY_HEADER = '''\
# SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
%YAML 1.2
---
# Checked-in ABI baseline of the ordered CTLV family registry.
# Updated deliberately by "make -C tools/ctlv abi-update".
'''


class _Dumper(yaml.SafeDumper):
    """Dumper that never emits anchors, which the strict loader rejects."""

    def ignore_aliases(self, data):
        return True

    def increase_indent(self, flow=False, indentless=False):
        # Indent a sequence under its key, not flush left as pyyaml does.
        return super().increase_indent(flow, False)


class _FlowMap(dict):
    pass


class _FlowSeq(list):
    pass


_Dumper.add_representer(
    _FlowMap,
    lambda dumper, value: dumper.represent_mapping(
        'tag:yaml.org,2002:map', value, flow_style=True))
_Dumper.add_representer(
    _FlowSeq,
    lambda dumper, value: dumper.represent_sequence(
        'tag:yaml.org,2002:seq', value, flow_style=True))


def build(family):
    """Return one family's ABI in the comparison form."""
    return {
        'family': family.name,
        'family-id': family.family_id,
        'parent': family.parent,
        'pre-init': family.pre_init,
        'post-init': family.post_init,
        'pre-fini': family.pre_fini,
        'definitions': _definitions(family.local_definitions),
        'groups': _groups(family.local_groups),
        'sets': _sets(family.local_sets),
        'fills': _fills(family.fills),
        'ops': _ops(family.local_ops),
    }


def render(family):
    """Return one family's compact snapshot as YAML text."""
    return render_data(build(family))


def render_data(data):
    """Render comparison-form data as a compact snapshot."""
    body = yaml.dump(_compact(data), Dumper=_Dumper, sort_keys=False,
                     default_flow_style=False, width=1000000)
    body = '\n'.join(line for source in body.splitlines()
                     for line in _wrap_flow(source)) + '\n'
    return HEADER.format(name=data['family']) + body


def _wrap_flow(line, width=80):
    """Wrap one flow collection with continuations aligned after its opener."""
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

    prefix = line[:start + 1]
    suffix = line[end:]
    indent = ' ' * (start + 1)
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


def _flow_items(line, start):
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


def check(family, path):
    """Compare a family against its snapshot, or raise with every problem."""
    snapshot = _expand(yamlload.load(path))
    problems = _compare(snapshot, build(family))
    if problems:
        raise CtlvError(f'{family.name} against {path}',
                        'the schema is not compatible with its snapshot\n' +
                        '\n'.join(f'  {problem}' for problem in problems))


def _compact(data):
    family = _FlowMap({
        'name': data['family'],
        'id': data['family-id'],
    })
    if data['parent'] is not None:
        family['inherits'] = data['parent']
    for name in ('pre-init', 'post-init', 'pre-fini'):
        if data[name]:
            family[name] = True

    compact = {'family': family}
    for kind, section in (('enum', 'enums'), ('flags', 'flags')):
        definitions = {
            name: _FlowMap(entry['values'])
            for name, entry in data['definitions'].items()
            if entry['kind'] == kind
        }
        if definitions:
            compact[section] = definitions
    if data['groups']:
        compact['attr-groups'] = _compact_groups(data['groups'])
    if data['sets']:
        compact['attr-sets'] = {
            name: _FlowMap({
                'id': entry['set-id'],
                **({'stub': True} if entry.get('stub') else {}),
            })
            for name, entry in data['sets'].items()
        }
    if data['fills']:
        compact['fills'] = {
            name: _compact_tree(nodes)
            for name, nodes in data['fills'].items()
        }
    if data['ops']:
        compact['ops'] = {
            name: _compact_op(entry)
            for name, entry in data['ops'].items()
        }
    return compact


def _compact_groups(groups):
    compact = {}
    for name, group in groups.items():
        compact[name] = {
            'id': group['group-id'],
            'attrs': {
                attr_name: _compact_attr(attr)
                for attr_name, attr in group['attrs'].items()
            },
        }
    return compact


def _compact_attr(attr):
    kind = attr['kind']
    compact = _FlowMap({'id': attr['local-id']})
    if kind in ('enum', 'flags'):
        compact[kind] = attr['definition']
    elif kind == 'blob':
        blob = attr['blob']
        compact['blob'] = blob['direction']
        compact['backings'] = _FlowSeq(blob['backings'])
        compact['max-length'] = blob['max-length']
        if blob['alignment'] != 1:
            compact['alignment'] = blob['alignment']
    else:
        compact['type'] = kind

    if kind in ('uint', 'sint'):
        low, high = ((0, spec.U64_MAX) if kind == 'uint'
                     else (spec.S64_MIN, spec.S64_MAX))
        minimum = attr.get('min', low)
        maximum = attr.get('max', high)
        if minimum != low:
            compact['min'] = minimum
        if maximum != high:
            compact['max'] = maximum
        if 'unit' in attr:
            compact['unit'] = attr['unit']
            if attr['scale']:
                compact['scale'] = attr['scale']
    elif kind in ('string', 'binary'):
        minimum = attr.get('min-length', 0)
        maximum = attr['max-length']
        if kind == 'binary' and minimum == maximum:
            compact['length'] = maximum
        else:
            if minimum:
                compact['min-length'] = minimum
            compact['max-length'] = maximum

    maximum = attr['max-occurrences']
    if maximum != 1:
        compact['max-occurrences'] = (
            'unlimited' if maximum == spec.UNLIMITED else maximum)
    return compact


def _compact_op(op):
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
    compact = {'abi': abi}
    if op.get('request'):
        compact['request'] = _compact_tree(op['request'])
    if op.get('reply') is not None:
        compact['reply'] = _compact_tree(op['reply'])
    return compact


def _compact_tree(nodes):
    required = []
    optional = []
    for node in nodes:
        destination = required if node['required'] else optional
        destination.append(_compact_node(node))
    compact = {}
    if required:
        compact['attrs'] = _compact_nodes(required)
    if optional:
        compact['optional-attrs'] = _compact_nodes(optional)
    if compact and _simple_tree_length(compact) <= 54:
        return _FlowMap(compact)
    return compact


def _simple_tree_length(tree):
    length = 2
    for index, (name, selectors) in enumerate(tree.items()):
        if not isinstance(selectors, _FlowSeq):
            return 55
        if index:
            length += 2
        length += len(name) + 4
        length += sum(len(selector) + 2 for selector in selectors)
    return length


def _compact_nodes(nodes):
    if all(isinstance(node, str) for node in nodes):
        return _FlowSeq(nodes)
    return nodes


def _compact_node(node):
    name = node.get('attr', node.get('stub'))
    children = _compact_tree(node.get('children', []))
    if 'reenters' in node:
        children['reenters'] = node['reenters']
    if not children:
        return name
    return {name: children}


def _expand(compact):
    family = compact['family']
    family_id = family['id']
    definitions = {}
    for kind, section in (('enum', 'enums'), ('flags', 'flags')):
        for name, values in compact.get(section, {}).items():
            definitions[name] = {
                'kind': kind,
                'values': values,
            }
    return {
        'family': family['name'],
        'family-id': family_id,
        'parent': family.get('inherits'),
        'pre-init': family.get('pre-init', False),
        'post-init': family.get('post-init', False),
        'pre-fini': family.get('pre-fini', False),
        'definitions': definitions,
        'groups': _expand_groups(compact.get('attr-groups', {})),
        'sets': _expand_sets(compact.get('attr-sets', {})),
        'fills': {
            name: _expand_tree(tree)
            for name, tree in compact.get('fills', {}).items()
        },
        'ops': _expand_ops(compact.get('ops', {})),
    }


def _expand_groups(groups):
    expanded = {}
    for name, group in groups.items():
        expanded[name] = {
            'group-id': group['id'],
            'attrs': {
                attr_name: _expand_attr(attr)
                for attr_name, attr in group['attrs'].items()
            },
        }
    return expanded


def _expand_attr(compact):
    primary = [name for name in ('type', 'enum', 'flags', 'blob')
               if name in compact]
    if len(primary) != 1:
        raise CtlvError('ABI snapshot attr',
                        'expected one of type, enum, flags or blob')
    kind = compact[primary[0]] if primary[0] == 'type' else primary[0]
    attr = {
        'local-id': compact['id'],
        'kind': kind,
        'max-occurrences': compact.get('max-occurrences', 1),
    }
    if attr['max-occurrences'] == 'unlimited':
        attr['max-occurrences'] = spec.UNLIMITED
    if kind in ('enum', 'flags'):
        attr['definition'] = compact[kind]
    elif kind in ('uint', 'sint'):
        low, high = ((0, spec.U64_MAX) if kind == 'uint'
                     else (spec.S64_MIN, spec.S64_MAX))
        attr['min'] = compact.get('min', low)
        attr['max'] = compact.get('max', high)
        if 'unit' in compact:
            attr['unit'] = compact['unit']
            attr['scale'] = compact.get('scale', 0)
    elif kind in ('string', 'binary'):
        if 'length' in compact:
            attr['min-length'] = compact['length']
            attr['max-length'] = compact['length']
        else:
            attr['min-length'] = compact.get('min-length', 0)
            attr['max-length'] = compact['max-length']
    elif kind == 'blob':
        attr['blob'] = {
            'direction': compact['blob'],
            'backings': list(compact['backings']),
            'max-length': compact['max-length'],
            'alignment': compact.get('alignment', 1),
        }
    return attr


def _expand_sets(attr_sets):
    return {
        name: {
            'set-id': entry['id'],
            **({'stub': True} if entry.get('stub') else {}),
        }
        for name, entry in attr_sets.items()
    }


def _expand_ops(ops):
    expanded = {}
    for name, op in ops.items():
        abi = op['abi']
        op_type = abi['type']
        entry = {
            'op-id': abi['id'],
            'type': op_type,
            'availability': abi.get(
                'availability', spec.DEFAULT_AVAILABILITY),
            'optional': abi.get('optional', False),
            'pre-post': abi.get('pre-post', False),
            'request': (None if op_type == 'event'
                        else _expand_tree(op.get('request', {}))),
            'reply': (None if 'reply' not in op
                      else _expand_tree(op['reply'])),
        }
        if op_type != 'event':
            entry['fd-access'] = abi.get(
                'fd-access', spec.DEFAULT_FD_ACCESS)
        expanded[name] = entry
    return expanded


def _expand_tree(tree):
    nodes = []
    for required, key in ((True, 'attrs'), (False, 'optional-attrs')):
        for selector in tree.get(key, []):
            nodes.append(_expand_node(selector, required))
    return nodes


def _expand_node(selector, required):
    if isinstance(selector, str):
        name = selector
        nested = {}
    else:
        if not isinstance(selector, dict) or len(selector) != 1:
            raise CtlvError('ABI snapshot tree',
                            'expected an attr name or one nested attr')
        name, nested = next(iter(selector.items()))
    if '.' not in name:
        return {'stub': name, 'required': required}
    node = {'attr': name, 'required': required}
    children = _expand_tree(nested)
    if children:
        node['children'] = children
    if 'reenters' in nested:
        node['reenters'] = nested['reenters']
    return node


def registry_build(registry):
    """Return the registry in the same normalized form as its source."""
    families = []
    for entry in registry.entries:
        if entry.deprecated:
            families.append({'name': entry.name, 'deprecated': True})
        else:
            families.append(entry.name)
    return {'families': families}


def registry_render(registry):
    body = yaml.dump(registry_build(registry), Dumper=_Dumper,
                     sort_keys=False, default_flow_style=False, width=76)
    return REGISTRY_HEADER + body


def registry_problems(old, current, exact):
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
    if exact and not problems and old.entries != current.entries:
        problems.append('the registry ABI baseline needs updating')
    return problems


def registry_check(paths, registry, exact=True):
    baseline = spec.load_registry(paths, paths.registry_abi)
    problems = registry_problems(baseline, registry, exact)
    if problems:
        raise CtlvError(f'{paths.registry} against {paths.registry_abi}',
                        'the family registry is not compatible with its '
                        'baseline\n' +
                        '\n'.join(f'  {problem}' for problem in problems))


def _definitions(items):
    definitions = {}
    for definition in items:
        definitions[definition.name] = {
            'kind': definition.kind,
            'values': dict(definition.values),
        }
    return definitions


def _groups(items):
    groups = {}
    for group in items:
        groups[group.name] = {
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


def _sets(attr_sets):
    sets = {}
    for attr_set in attr_sets:
        entry = {
            'set-id': attr_set.set_id,
        }
        if attr_set.stub:
            entry['stub'] = True
        sets[attr_set.name] = entry
    return sets


def _fills(fills):
    return {fill.name: _tree(fill.root) for fill in fills}


def _ops(items):
    ops = {}
    for op in items:
        entry = {
            'op-id': op.local_id,
            'type': op.type,
            'availability': op.availability,
            'optional': op.optional,
            'pre-post': op.pre_post,
            'request': _tree(op.request),
            'reply': _tree(op.reply),
        }
        if op.type != 'event':
            entry['fd-access'] = op.fd_access
        ops[op.name] = entry
    return ops


def _tree(root):
    if root is None:
        return None
    children = [_node(child) for child in root.children()]
    children += [_stub(stub) for stub in root.stubs]
    return children


def _node(node):
    entry = {'attr': node.attr.full_name, 'required': node.required}
    if node.reenters is not None:
        # The members are recorded above, so record where they come from.
        entry['reenters'] = node.reenters
        return entry
    children = _tree(node)
    if children:
        entry['children'] = children
    return entry


def _stub(stub):
    return {'stub': stub.name, 'required': not stub.optional}


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
    for key in ('pre-init', 'post-init', 'pre-fini'):
        if old.get(key, False) != new[key]:
            problems.append(f'family: {key} was {old.get(key, False)}, now '
                            f'{new[key]}')
    _compare_definitions(problems, old['definitions'], new['definitions'])
    _compare_groups(problems, old['groups'], new['groups'])
    _compare_sets(problems, old['sets'], new['sets'])
    _compare_fills(problems, old['fills'], new['fills'])
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
        if was['group-id'] != now['group-id']:
            problems.append(f'{where}: group-id was {was["group-id"]}, now '
                            f'{now["group-id"]}')
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
        for key in ('set-id', 'stub'):
            was_value = was.get(key, False)
            now_value = now.get(key, False)
            if was_value != now_value:
                problems.append(f'attr set {name}: {key} was {was_value}, '
                                f'now {now_value}')


def _compare_fills(problems, old, new):
    for name, was in old.items():
        now = new.get(name)
        if now is None:
            problems.append(f'stub fill {name} was removed')
            continue
        _compare_nodes(problems, f'stub fill {name}', was, now)


def _compare_ops(problems, old, new):
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
            elif was[what] is not None:
                _compare_nodes(problems, f'{where} {what}', was[what],
                               now[what])


def _compare_nodes(problems, where, old, new):
    by_name = {_node_key(node): node for node in new}
    for was in old:
        key = _node_key(was)
        name = _node_name(was)
        now = by_name.pop(key, None)
        if now is None:
            problems.append(f'{where}: {name} was removed')
            continue
        if now['required'] and not was['required']:
            problems.append(f'{where}: {name} became required')
        if was.get('reenters') != now.get('reenters'):
            problems.append(f'{where}: {name} re-entered '
                            f'{was.get("reenters")}, now '
                            f'{now.get("reenters")}')
        _compare_nodes(problems, f'{where} {name}',
                       was.get('children', []), now.get('children', []))
    for node in by_name.values():
        if node['required']:
            problems.append(
                f'{where}: new {_node_name(node)} must be optional')


def _node_key(node):
    if 'attr' in node:
        return 'attr', node['attr']
    return 'stub', node['stub']


def _node_name(node):
    if 'attr' in node:
        return node['attr']
    return f'stub {node["stub"]}'


def _occurrences(max_occurrences):
    if max_occurrences == spec.UNLIMITED:
        return 'unlimited'
    return str(max_occurrences)
