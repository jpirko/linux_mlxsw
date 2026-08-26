# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Resolved names and values consumed by specification-aware tools."""

from __future__ import annotations

import json
from typing import Any

from . import semantics


def _definition(item: semantics.Definition) -> dict[str, Any]:
    values = []
    for name, number in item.values:
        value = 1 << number if item.kind == 'flags' else number
        values.append({'name': name, 'value': value})
    return {
        'name': item.name,
        'kind': item.kind,
        'values': values,
    }


def _field(item: semantics.Field) -> dict[str, str]:
    out = {
        'name': item.full_name,
        'short': item.name,
        'type': f'0x{item.type:016x}',
        'kind': item.decl.kind,
    }
    if item.decl.kind in ('enum', 'flags'):
        assert item.decl.ref is not None
        out['definition'] = item.decl.ref
    return out


def _op(item: semantics.Op) -> dict[str, Any]:
    return {
        'name': item.name,
        'id': f'0x{item.op_id:016x}',
        'type': item.type,
        'availability': item.availability,
        'request-max-len': item.sizes.request_max_len,
        'action-reply-max-len': item.sizes.action_reply_max_len,
        'query-reply-min-len': item.sizes.query_reply_min_len,
        'query-reply-suggested-len':
            item.sizes.query_reply_suggested_len,
        'error-info-max-len': item.sizes.error_info_max_len,
        'fd-access': item.fd_access,
    }


def render(family: semantics.Family) -> str:
    """Return a versioned manifest for one effective family."""
    chain = [{'name': name, 'id': family_id}
             for family_id, name in family.owners.items()]
    data = {
        'version': 1,
        'family': family.name,
        'chain': chain,
        'definitions': [_definition(item)
                        for item in family.definitions.values()],
        'fields': [_field(item) for item in family.fields.values()],
        'ops': [_op(item) for item in family.ops],
    }
    return json.dumps(data, separators=(',', ':')) + '\n'
