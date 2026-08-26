# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Length arithmetic over an expanded op tree.

These numbers decide whether an op can run at all, so they are kept apart from
the rest of the resolution and tested against lengths worked out by hand. The
constants mirror include/uapi/linux/ctlv.h, and a test checks that.
"""

from __future__ import annotations

import dataclasses

from . import spec
from .errors import CtlvError

ALIGNTO = 8
ATTR_HDR_LEN = 16
EVENT_HDR_LEN = 24

MAX_INLINE_MESSAGE_LEN = 0x100000
MAX_ERROR_LEN = 0x4000
MAX_NEST_DEPTH = 32
DEFAULT_QUERY_REPLY_LEN = 0x10000
MAX_EVENT_QUEUE_LEN = 0x1000000

# One level short of what the transport carries: op-schema-get describes a
# message, and a description nests one level deeper than what it describes.
MAX_DECLARED_NEST_DEPTH = MAX_NEST_DEPTH - 1

# Every integer, enum and flags payload is 64-bit.
SCALAR_PAYLOAD_LEN = 8

# A bool payload is one byte, padded to eight only as a placement.
BOOL_PAYLOAD_LEN = 1

# How many core scalar descriptor attrs each blob backing carries. A test
# checks these against the blob group of the core family.
BLOB_DESCRIPTOR_ATTRS = {
    'va': 3,
    'memfd': 4,
    'dma-buf': 4,
}

# Attr sets of the root family that are message roots in their own right, each
# with the transport header's limit and the name the header gives it.
CORE_MESSAGES = {
    'error-info': ('CTLV_MAX_ERROR_LEN', MAX_ERROR_LEN),
}


@dataclasses.dataclass(frozen=True)
class OpSizes:
    """Generated lengths of one op. Zero means "does not apply"."""

    request_max_len: int
    action_reply_max_len: int
    query_reply_min_len: int
    query_reply_suggested_len: int
    event_record_max_len: int


def align(length):
    return (length + ALIGNTO - 1) & ~(ALIGNTO - 1)


def attr_max_len(node):
    """Maximum length of one occurrence of node, excluding its own padding."""
    return ATTR_HDR_LEN + _payload_max_len(node)


def root_max_len(root):
    """Maximum length of a complete message root."""
    return ATTR_HDR_LEN + _payload_max_len(root)


def min_reply_len(root):
    """Buffer length needed for the largest indivisible leaf and its path.

    A leaf below a re-entered level sits as deep as the transport allows.
    """
    if _reenters(root):
        return (MAX_NEST_DEPTH + 1) * ATTR_HDR_LEN + _largest_leaf(root)
    return _min_walk(root, 0)


def compute(where, op, request, reply):
    """Return the generated lengths of one op."""
    request_max_len = 0
    if request is not None:
        request_max_len = root_max_len(request)
        _check_inline(where, 'request', request_max_len)

    action_reply_max_len = 0
    query_reply_min_len = 0
    query_reply_suggested_len = 0
    event_record_max_len = 0

    if op.type == 'action' and reply is not None:
        action_reply_max_len = root_max_len(reply)
        _check_inline(where, 'action reply', action_reply_max_len)
    elif op.type == 'query':
        query_reply_min_len = min_reply_len(reply)
        _check_inline(where, 'largest query reply leaf and its path',
                      query_reply_min_len)
        query_reply_suggested_len = _suggested_len(
            where, op.suggested_reply_length, query_reply_min_len)
    elif op.type == 'event':
        payload_max_len = root_max_len(reply)
        _check_inline(where, 'event payload', payload_max_len)
        event_record_max_len = EVENT_HDR_LEN + align(payload_max_len)

    return OpSizes(request_max_len=request_max_len,
                   action_reply_max_len=action_reply_max_len,
                   query_reply_min_len=query_reply_min_len,
                   query_reply_suggested_len=query_reply_suggested_len,
                   event_record_max_len=event_record_max_len)


def composed(where, op, request, reply):
    """Return the lengths one op needs on one leaf.

    compute()'s numbers over the composed tree, fills spliced in. A suggestion
    is raised here rather than refused.
    """
    sizes = OpSizes(request_max_len=root_max_len(request) if request else 0,
                    action_reply_max_len=0, query_reply_min_len=0,
                    query_reply_suggested_len=0, event_record_max_len=0)
    _check_inline(where, 'request', sizes.request_max_len)

    if op.type == 'action' and reply is not None:
        sizes = dataclasses.replace(sizes,
                                    action_reply_max_len=root_max_len(reply))
        _check_inline(where, 'action reply', sizes.action_reply_max_len)
    elif op.type == 'query':
        minimum = min_reply_len(reply)
        _check_inline(where, 'largest query reply leaf and its path', minimum)
        sizes = dataclasses.replace(
            sizes, query_reply_min_len=minimum,
            query_reply_suggested_len=max(op.sizes.query_reply_suggested_len,
                                          minimum))
    elif op.type == 'event':
        payload_max_len = root_max_len(reply)
        _check_inline(where, 'event payload', payload_max_len)
        record = EVENT_HDR_LEN + align(payload_max_len)
        if record > MAX_EVENT_QUEUE_LEN:
            raise CtlvError(where, f'event record needs {record} bytes, more '
                                   f'than the {MAX_EVENT_QUEUE_LEN} byte '
                                   'event queue limit')
        sizes = dataclasses.replace(sizes, event_record_max_len=record)
    return sizes


def message_max_len(where, name, root):
    """Return the maximum length of one core message, against its limit."""
    limit_name, limit = CORE_MESSAGES[name]
    length = root_max_len(root)
    if length > limit:
        raise CtlvError(where, f'needs {length} bytes, more than the {limit} '
                               f'of {limit_name}')
    return length


def _payload_max_len(node):
    if node.is_root:
        return _children_max_len(node)

    decl = node.attr.decl
    if decl.kind in ('uint', 'sint', 'enum', 'flags'):
        return SCALAR_PAYLOAD_LEN
    if decl.kind == 'bool':
        return BOOL_PAYLOAD_LEN
    if decl.kind == 'string':
        # One terminating NUL, which max-length excludes.
        return decl.max_length + 1
    if decl.kind == 'binary':
        return decl.max_length
    if decl.kind == 'blob':
        widest = max(BLOB_DESCRIPTOR_ATTRS[backing]
                     for backing in decl.blob.backings)
        return widest * align(ATTR_HDR_LEN + SCALAR_PAYLOAD_LEN)
    return _children_max_len(node)


def _children_max_len(node):
    total = 0
    for child in node.children():
        if child.max_occurrences == spec.UNLIMITED:
            raise AssertionError(
                f'{child.attr.full_name} has unlimited occurrences, so it has '
                'no maximum length')
        if child.reenters is not None:
            raise AssertionError(
                f'{child.attr.full_name} re-enters {child.reenters}, so it '
                'has no maximum length')
        total += child.max_occurrences * align(attr_max_len(child))
    return total


def _min_walk(node, ancestors_len):
    children = node.children()
    if children:
        return max(_min_walk(child, ancestors_len + ATTR_HDR_LEN)
                   for child in children)
    if node.is_root:
        return ancestors_len + ATTR_HDR_LEN
    return ancestors_len + align(attr_max_len(node))


def _reenters(node):
    return any(child.reenters is not None or _reenters(child)
               for child in node.children())


def _largest_leaf(node):
    """The longest placement of the tree that a fragment cannot split."""
    lengths = [align(attr_max_len(child))
               for child in node.children()
               if child.attr.decl.kind != 'nested']
    lengths += [_largest_leaf(child) for child in node.children()]
    return max(lengths, default=0)


def _check_inline(where, what, length):
    if length > MAX_INLINE_MESSAGE_LEN:
        raise CtlvError(where, f'{what} needs {length} bytes, more than the '
                               f'{MAX_INLINE_MESSAGE_LEN} byte inline '
                               'message limit')


def _suggested_len(where, declared, minimum):
    if declared is None:
        return max(DEFAULT_QUERY_REPLY_LEN, minimum)
    if declared < minimum:
        raise CtlvError(where, f'suggested-reply-length {declared} is below '
                               f'the computed minimum of {minimum}')
    if declared > MAX_INLINE_MESSAGE_LEN:
        raise CtlvError(where, f'suggested-reply-length {declared} exceeds '
                               f'the {MAX_INLINE_MESSAGE_LEN} byte inline '
                               'message limit')
    return declared
