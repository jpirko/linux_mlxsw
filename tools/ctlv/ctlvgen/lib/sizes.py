# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Length arithmetic over an expanded op tree.

These numbers decide whether an op can run at all, so they are kept apart from
the rest of resolution and tested against independently calculated lengths.
The limits mirror include/linux/ctlv_family.h, and a test checks that.
"""

from __future__ import annotations

import dataclasses
from typing import TYPE_CHECKING

from . import spec
from .errors import CtlvError

# semantics imports this module.
if TYPE_CHECKING:
    from . import semantics

ALIGNTO: int = 8
HDR_LEN: int = 16
EVENT_HDR_LEN: int = 24

MAX_INLINE_MESSAGE_LEN: int = 0x10000
MAX_ERROR_LEN: int = 0x4000
MAX_RECORD_DEPTH: int = 8
MAX_EVENT_QUEUE_LEN: int = 0x1000000

# The suggested reply length of a query whose spec suggests none.
DEFAULT_QUERY_REPLY_LEN: int = 0x10000

# One level short of what the transport carries: op-schema-get describes a
# message, and a description goes one level deeper than what it describes.
MAX_DECLARED_RECORD_DEPTH: int = MAX_RECORD_DEPTH - 1

# Every integer, enum and flags payload is 64-bit.
SCALAR_PAYLOAD_LEN: int = 8

# A bool payload is one byte; only the TLV holding it is padded to eight.
BOOL_PAYLOAD_LEN: int = 1

# How many scalar descriptor fields each blob backing carries. A test checks
# these against the root family's blob record.
BLOB_DESCRIPTOR_FIELDS: dict[str, int] = {
    'va': 3,
}

ERROR_INFO: str = 'error-info'

# Records of the root family that are message roots in their own right, each
# with the core's limit and the name the kernel gives it.
CORE_MESSAGES: dict[str, tuple[str, int]] = {
    ERROR_INFO: ('CTLV_MAX_ERROR_LEN', MAX_ERROR_LEN),
}


@dataclasses.dataclass(frozen=True)
class OpSizes:
    """Generated lengths of one op. Zero means "does not apply"."""

    request_max_len: int
    action_reply_max_len: int
    query_reply_min_len: int
    query_reply_suggested_len: int
    event_max_len: int
    error_info_max_len: int


def align(length: int) -> int:
    return (length + ALIGNTO - 1) & ~(ALIGNTO - 1)


def tlv_max_len(node: semantics.Node) -> int:
    """Maximum length of one occurrence of node, excluding its own padding."""
    return HDR_LEN + _payload_max_len(node)


def root_max_len(root: semantics.Node) -> int:
    """Maximum length of a complete message root."""
    return HDR_LEN + _payload_max_len(root)


def min_reply_len(root: semantics.Node) -> int:
    """Buffer length needed for the largest indivisible leaf and its path.

    A leaf below a re-entered level sits as deep as the transport allows.
    """
    if _reenters(root):
        return (MAX_RECORD_DEPTH + 1) * HDR_LEN + _largest_leaf(root)
    return _min_walk(root, 0)


def compute(where: str, op: spec.OpSpec, request: semantics.Node | None,
            reply: semantics.Node | None, error_info_max_len: int) -> OpSizes:
    """Return the generated lengths of one op."""
    request_max_len = 0
    if request is not None:
        request_max_len = root_max_len(request)
        _check_inline(where, 'request', request_max_len)

    action_reply_max_len = 0
    query_reply_min_len = 0
    query_reply_suggested_len = 0
    event_max_len = 0

    if op.type == 'action' and reply is not None:
        action_reply_max_len = root_max_len(reply)
        _check_inline(where, 'action reply', action_reply_max_len)
    elif op.type == 'query':
        assert reply is not None
        query_reply_min_len = min_reply_len(reply)
        _check_inline(where, 'largest query reply leaf and its path',
                      query_reply_min_len)
        query_reply_suggested_len = _suggested_len(
            where, op.suggested_reply_length, query_reply_min_len)
    elif op.type == 'event':
        assert reply is not None
        payload_max_len = root_max_len(reply)
        _check_inline(where, 'event payload', payload_max_len)
        event_max_len = EVENT_HDR_LEN + align(payload_max_len)

    return OpSizes(request_max_len=request_max_len,
                   action_reply_max_len=action_reply_max_len,
                   query_reply_min_len=query_reply_min_len,
                   query_reply_suggested_len=query_reply_suggested_len,
                   event_max_len=event_max_len,
                   error_info_max_len=(0 if op.type == 'event'
                                       else error_info_max_len))


def composed(where: str, op: semantics.Op, request: semantics.Node | None,
             reply: semantics.Node | None) -> OpSizes:
    """Return the lengths one op needs for one family.

    compute()'s numbers over the composed tree, fills spliced in. A suggestion
    is raised here rather than refused.
    """
    sizes = OpSizes(request_max_len=root_max_len(request) if request else 0,
                    action_reply_max_len=0, query_reply_min_len=0,
                    query_reply_suggested_len=0, event_max_len=0,
                    error_info_max_len=op.sizes.error_info_max_len)
    _check_inline(where, 'request', sizes.request_max_len)

    if op.type == 'action' and reply is not None:
        sizes = dataclasses.replace(sizes,
                                    action_reply_max_len=root_max_len(reply))
        _check_inline(where, 'action reply', sizes.action_reply_max_len)
    elif op.type == 'query':
        assert reply is not None
        minimum = min_reply_len(reply)
        _check_inline(where, 'largest query reply leaf and its path', minimum)
        sizes = dataclasses.replace(
            sizes, query_reply_min_len=minimum,
            query_reply_suggested_len=max(op.sizes.query_reply_suggested_len,
                                          minimum))
    elif op.type == 'event':
        assert reply is not None
        payload_max_len = root_max_len(reply)
        _check_inline(where, 'event payload', payload_max_len)
        length = EVENT_HDR_LEN + align(payload_max_len)
        if length > MAX_EVENT_QUEUE_LEN:
            raise CtlvError(where, f'event needs {length} bytes, more than '
                                   f'the {MAX_EVENT_QUEUE_LEN} byte event '
                                   'queue limit')
        sizes = dataclasses.replace(sizes, event_max_len=length)
    return sizes


def message_max_len(where: str, name: str, root: semantics.Node) -> int:
    """Return the maximum length of one core message, against its limit."""
    limit_name, limit = CORE_MESSAGES[name]
    length = root_max_len(root)
    if length > limit:
        raise CtlvError(where, f'needs {length} bytes, more than the {limit} '
                               f'of {limit_name}')
    return length


def _payload_max_len(node: semantics.Node) -> int:
    if node.is_root:
        return _children_max_len(node)

    decl = node.child_field.decl
    if decl.kind in ('uint', 'sint', 'enum', 'flags'):
        return SCALAR_PAYLOAD_LEN
    if decl.kind == 'bool':
        return BOOL_PAYLOAD_LEN
    if decl.kind == 'string':
        assert decl.max_length is not None
        # One terminating NUL, which max-length excludes.
        return decl.max_length + 1
    if decl.kind == 'binary':
        assert decl.max_length is not None
        return decl.max_length
    if decl.kind == 'blob':
        assert decl.blob is not None
        widest = max(BLOB_DESCRIPTOR_FIELDS[backing]
                     for backing in decl.blob.backings)
        return widest * align(HDR_LEN + SCALAR_PAYLOAD_LEN)
    return _children_max_len(node)


def _children_max_len(node: semantics.Node) -> int:
    total = 0
    for child in node.children():
        if child.max_occurrences == spec.UNLIMITED:
            raise AssertionError(
                f'{child.child_field.full_name} has unlimited occurrences, '
                'so it has no maximum length')
        if child.reenters is not None:
            raise AssertionError(
                f'{child.child_field.full_name} re-enters {child.reenters}, '
                'so it has no maximum length')
        total += child.max_occurrences * align(tlv_max_len(child))
    return total


def _min_walk(node: semantics.Node, ancestors_len: int) -> int:
    children = node.children()
    if children:
        return max(_min_walk(child, ancestors_len + HDR_LEN)
                   for child in children)
    if node.is_root:
        return ancestors_len + HDR_LEN
    return ancestors_len + align(tlv_max_len(node))


def _reenters(node: semantics.Node) -> bool:
    return any(child.reenters is not None or _reenters(child)
               for child in node.children())


def _largest_leaf(node: semantics.Node) -> int:
    """The longest node of the tree that a fragment cannot split."""
    lengths = [align(tlv_max_len(child))
               for child in node.children()
               if child.child_field.decl.kind != 'record']
    lengths += [_largest_leaf(child) for child in node.children()]
    return max(lengths, default=0)


def _check_inline(where: str, what: str, length: int) -> None:
    if length > MAX_INLINE_MESSAGE_LEN:
        raise CtlvError(where, f'{what} needs {length} bytes, more than the '
                               f'{MAX_INLINE_MESSAGE_LEN} byte inline '
                               'message limit')


def _suggested_len(where: str, declared: int | None, minimum: int) -> int:
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
