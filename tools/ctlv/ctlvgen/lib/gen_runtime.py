# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Resolved names and values consumed by specification-aware tools."""

import json


def _definition(item):
    values = []
    for name, number in item.values:
        value = 1 << number if item.kind == 'flags' else number
        values.append({'name': name, 'value': value})
    return {
        'name': item.name,
        'kind': item.kind,
        'values': values,
    }


def _attr(item):
    out = {
        'name': item.full_name,
        'short': item.name,
        'id': f'0x{item.attr_id:016x}',
        'kind': item.decl.kind,
    }
    if item.decl.kind in ('enum', 'flags'):
        out['definition'] = item.decl.ref
    return out


def _op(item):
    out = {
        'name': item.name,
        'id': f'0x{item.op_id:016x}',
        'type': item.type,
        'availability': item.availability,
        'request-max-len': item.sizes.request_max_len,
        'action-reply-max-len': item.sizes.action_reply_max_len,
        'query-reply-min-len': item.sizes.query_reply_min_len,
        'query-reply-suggested-len':
            item.sizes.query_reply_suggested_len,
        'event-record-max-len': item.sizes.event_record_max_len,
    }
    if item.type != 'event':
        out['fd-access'] = item.fd_access
    return out


def render(family):
    """Return a versioned manifest for one effective family."""
    chain = [{'name': name, 'id': family_id}
             for family_id, name in family.owners.items()]
    data = {
        'version': 1,
        'family': family.name,
        'chain': chain,
        'definitions': [_definition(item)
                        for item in family.definitions.values()],
        'attrs': [_attr(item) for item in family.attrs.values()],
        'ops': [_op(item) for item in family.ops],
    }
    return json.dumps(data, separators=(',', ':')) + '\n'
