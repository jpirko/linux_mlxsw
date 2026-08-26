# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the generated lengths.

Every expected value is written as the arithmetic that produces it, so that a
reviewer can check it against the message format without running anything.
These lengths gate execution in the kernel, so a silent change to one is a
UAPI change.
"""

from __future__ import annotations

import re
import unittest

from tests import support

from ctlvgen.lib import semantics
from ctlvgen.lib import sizes

# sizeof(struct ctlv_hdr) and sizeof(struct ctlv_event_hdr).
HDR: int = 16
EVENT_HDR: int = 24

# A 64-bit payload: integers, enums and flags. Already 8-aligned.
SCALAR: int = HDR + 8


def string_tlv(max_length: int) -> int:
    """A string carries max-length payload bytes plus one terminating NUL."""
    return sizes.align(HDR + max_length + 1)


def binary_tlv(max_length: int) -> int:
    return sizes.align(HDR + max_length)


def blob_tlv(descriptor_fields: int) -> int:
    return HDR + descriptor_fields * SCALAR


def leaf(field: semantics.Field) -> semantics.Node:
    return semantics.Node(field, True, 1, (), ())


class TestFieldLengths(unittest.TestCase):
    """One occurrence of one field, for each schema kind."""

    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture-child')

    def assert_len(self, full_name: str, expected: int) -> None:
        node = leaf(self.family.fields[full_name])
        self.assertEqual(expected, sizes.tlv_max_len(node))

    def test_integer(self) -> None:
        self.assert_len('u', HDR + 8)
        self.assert_len('s', HDR + 8)
        self.assert_len('scalar.freq', HDR + 8)

    def test_bool_carries_one_byte_and_is_padded_like_the_rest(self) -> None:
        self.assert_len('scalar.flag', HDR + 1)
        node = leaf(self.family.fields['scalar.flag'])
        self.assertEqual(SCALAR, sizes.align(sizes.tlv_max_len(node)))

    def test_enum_and_flags(self) -> None:
        self.assert_len('scalar.mode', HDR + 8)
        self.assert_len('scalar.quirks', HDR + 8)

    def test_string(self) -> None:
        self.assert_len('scalar.text', HDR + 15 + 1)
        self.assert_len('extension-point.tag', HDR + 31 + 1)

    def test_binary(self) -> None:
        self.assert_len('scalar.fixed', HDR + 8)
        self.assert_len('scalar.bounded', HDR + 64)
        self.assert_len('child-dump dump', HDR + 4096)

    def test_blob_takes_its_widest_permitted_backing(self) -> None:
        # va is type, address and length; memfd and dma-buf add an offset.
        self.assert_len('blobs-submit input-va', blob_tlv(3))
        self.assert_len('blobs-submit output-fd', blob_tlv(4))
        self.assert_len('blobs-submit inout-any', blob_tlv(4))

    def test_blob_descriptor_counts_match_the_core_family(self) -> None:
        core = support.load('ctlv')
        members = [field.name for field in core.records['blob'].fields]
        self.assertEqual(['type', 'address', 'fd', 'offset', 'length'],
                         members)
        self.assertEqual(3, sizes.BLOB_DESCRIPTOR_FIELDS['va'])
        self.assertEqual(4, sizes.BLOB_DESCRIPTOR_FIELDS['memfd'])
        self.assertEqual(4, sizes.BLOB_DESCRIPTOR_FIELDS['dma-buf'])


class TestFixtureSizes(unittest.TestCase):
    """Independently calculated lengths for every op shape."""

    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load_family('ctlv-fixture')

    def sizes_of(self, name: str) -> sizes.OpSizes:
        return support.op(self.family, name).sizes

    def test_empty_request_root(self) -> None:
        got = self.sizes_of('noop')
        self.assertEqual(HDR, got.request_max_len)
        self.assertEqual(0, got.action_reply_max_len)
        self.assertEqual(456, got.error_info_max_len)

    def test_every_schema_kind_in_one_request(self) -> None:
        got = self.sizes_of('scalars-set')
        # root + u + s + delta + freq + size + mode + quirks, the one byte of
        # flag padded to the same, then the three that are not bare 64-bit
        self.assertEqual(HDR + 7 * SCALAR + sizes.align(HDR + 1) +
                         string_tlv(15) + binary_tlv(8) + binary_tlv(64),
                         got.request_max_len)
        self.assertEqual(344, got.request_max_len)

    def test_action_with_a_bounded_reply(self) -> None:
        # root + u
        self.assertEqual(HDR + SCALAR,
                         self.sizes_of('scalars-set').action_reply_max_len)
        self.assertEqual(40, self.sizes_of('scalars-set')
                         .action_reply_max_len)

    def test_finite_repetition(self) -> None:
        # Three scalars, two other optional scalars, two scalar-pair records,
        # then a scalar per value of mode and one per value of hash-algo.
        algos = len(self.family.definitions['hash-algo'].values)
        self.assertEqual(HDR + (8 + algos) * SCALAR + 2 * (HDR + 2 * SCALAR),
                         self.sizes_of('counts-get').request_max_len)
        self.assertEqual(336 + algos * SCALAR,
                         self.sizes_of('counts-get').request_max_len)

    def test_query_reply_minimum_is_the_largest_leaf_and_its_path(
            self) -> None:
        # A flat reply of one repeated scalar needs room for one of them.
        got = self.sizes_of('counts-get')
        self.assertEqual(HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(40, got.query_reply_min_len)

    def test_unlimited_fields_at_several_record_depths(self) -> None:
        got = self.sizes_of('records-dump')
        # root + level1 + level2 + level3 + u
        self.assertEqual(4 * HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(88, got.query_reply_min_len)

    def test_declared_suggested_reply_length_is_kept(self) -> None:
        self.assertEqual(4096,
                         self.sizes_of('records-dump')
                         .query_reply_suggested_len)

    def test_request_with_every_blob_backing(self) -> None:
        got = self.sizes_of('blobs-submit')
        # Three descriptors of one blob each, and three of the repeated one.
        self.assertEqual(HDR + 4 * blob_tlv(3) + 2 * blob_tlv(4),
                         got.request_max_len)
        self.assertEqual(592, got.request_max_len)
        self.assertEqual(0, got.action_reply_max_len)

    def test_an_optional_field_is_counted_as_present(self) -> None:
        # root + u + mode + variant
        self.assertEqual(HDR + 3 * SCALAR,
                         self.sizes_of('optional-check').request_max_len)
        self.assertEqual(88, self.sizes_of('optional-check').request_max_len)

    def test_an_event_includes_its_header(self) -> None:
        changed = self.sizes_of('thing-changed')
        # root + u + s
        self.assertEqual(EVENT_HDR + HDR + 2 * SCALAR, changed.event_max_len)
        self.assertEqual(88, changed.event_max_len)

    def test_payload_free_event(self) -> None:
        self.assertEqual(EVENT_HDR + HDR,
                         self.sizes_of('pinged').event_max_len)
        self.assertEqual(40, self.sizes_of('pinged').event_max_len)

    def test_an_event_has_no_request_length(self) -> None:
        self.assertEqual(0, self.sizes_of('pinged').request_max_len)


class TestInheritedSizes(unittest.TestCase):
    """Filling an extension point changes the ops the child inherited."""

    parent: semantics.Family
    child: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.parent = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def test_extension_expansion_adds_a_shallower_path(self) -> None:
        # The tag the child puts in records-dump extension has a shorter path
        # than the level3 chain that sets the minimum, so the minimum stays.
        extension_path = HDR + HDR + string_tlv(31)
        self.assertEqual(80, extension_path)
        parent_min = support.op(self.parent, 'records-dump') \
            .sizes.query_reply_min_len
        self.assertLess(extension_path, parent_min)
        self.assertEqual(parent_min,
                         support.op(self.child, 'records-dump')
                         .sizes.query_reply_min_len)

    def test_a_reply_of_the_filled_extension_alone(self) -> None:
        got = support.op(self.child, 'child-info-get').sizes
        # root + extension-point.tag, the largest leaf of the reply
        self.assertEqual(HDR + string_tlv(31), got.query_reply_min_len)
        self.assertEqual(64, got.query_reply_min_len)

    def test_child_query_minimum(self) -> None:
        got = support.op(self.child, 'child-dump').sizes
        # root + child-dump dump, the largest leaf of the reply
        self.assertEqual(HDR + binary_tlv(4096), got.query_reply_min_len)
        self.assertEqual(4128, got.query_reply_min_len)

    def test_suggested_reply_length_defaults(self) -> None:
        self.assertEqual(sizes.DEFAULT_QUERY_REPLY_LEN,
                         support.op(self.child, 'child-dump')
                         .sizes.query_reply_suggested_len)


class TestCoreSizes(unittest.TestCase):
    family: semantics.Family

    @classmethod
    def setUpClass(cls) -> None:
        cls.family = support.load('ctlv')

    def sizes_of(self, name: str) -> sizes.OpSizes:
        return support.op(self.family, name).sizes

    def test_op_get(self) -> None:
        got = self.sizes_of('op-get')
        # root + one ops record + one scalar member
        self.assertEqual(HDR + HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(56, got.query_reply_min_len)
        self.assertEqual(HDR, got.request_max_len)

    def test_subscriptions_carry_up_to_256_op_ids(self) -> None:
        self.assertEqual(HDR + 256 * SCALAR,
                         self.sizes_of('subscriptions-add').request_max_len)
        self.assertEqual(6160,
                         self.sizes_of('subscriptions-add').request_max_len)

    def test_op_changed_carries_one_op(self) -> None:
        # root + id + disabled, whose one byte is padded like the rest,
        # + state-generation
        self.assertEqual(EVENT_HDR + HDR + 3 * SCALAR,
                         self.sizes_of('op-changed').event_max_len)

    def test_control_events(self) -> None:
        self.assertEqual(EVENT_HDR + HDR + 3 * SCALAR,
                         self.sizes_of('event-overflow').event_max_len)
        self.assertEqual(EVENT_HDR + HDR,
                         self.sizes_of('device-removed').event_max_len)

    def test_op_schema_get_reaches_the_depth_limit(self) -> None:
        """A re-entered level can repeat until the transport stops it."""
        got = self.sizes_of('op-schema-get')
        self.assertEqual((sizes.MAX_RECORD_DEPTH + 1) * HDR + SCALAR,
                         got.query_reply_min_len)
        self.assertEqual(168, got.query_reply_min_len)
        self.assertEqual(HDR + SCALAR, got.request_max_len)

    def test_the_deepest_description_fits_what_it_promised(self) -> None:
        """The root, the record naming the message, one per declared level."""
        got = self.sizes_of('op-schema-get')
        path = (1 + 1 + sizes.MAX_DECLARED_RECORD_DEPTH) * HDR
        self.assertEqual(path + SCALAR, got.query_reply_min_len)

    def test_error_information_fits_the_error_limit(self) -> None:
        fields = self.family.records['error-info'].fields
        total = HDR + sum(sizes.align(sizes.tlv_max_len(leaf(field)))
                          for field in fields)
        self.assertEqual(HDR + 7 * SCALAR + string_tlv(255), total)
        self.assertEqual(456, total)
        self.assertLessEqual(total, sizes.MAX_ERROR_LEN)

    def test_actions_and_queries_take_the_error_information(self) -> None:
        self.assertEqual(456, self.sizes_of('op-get').error_info_max_len)
        self.assertEqual(456, self.sizes_of('subscriptions-add')
                         .error_info_max_len)
        self.assertEqual(0, self.sizes_of('op-changed').error_info_max_len)


class TestLimits(unittest.TestCase):
    """The Python constants and the kernel's headers must not drift apart."""

    LIMITS = {
        'CTLV_MAX_INLINE_MESSAGE_LEN': sizes.MAX_INLINE_MESSAGE_LEN,
        'CTLV_MAX_ERROR_LEN': sizes.MAX_ERROR_LEN,
        'CTLV_MAX_RECORD_DEPTH': sizes.MAX_RECORD_DEPTH,
        'CTLV_MAX_EVENT_QUEUE_LEN': sizes.MAX_EVENT_QUEUE_LEN,
    }

    def test_constants_match_the_kernel_headers(self) -> None:
        expected = {
            support.UAPI_HEADER: {'CTLV_ALIGNTO': sizes.ALIGNTO},
            support.FAMILY_HEADER: self.LIMITS,
        }
        for header, constants in expected.items():
            text = header.read_text(encoding='utf-8')
            for name, value in constants.items():
                found = re.search(rf'^#define {name}\s+(\S+)', text, re.M)
                assert found is not None, f'{name} is not defined'
                self.assertEqual(value, int(found.group(1), 0), name)

    def test_no_limit_is_userspace_api(self) -> None:
        """Userspace has each op's lengths from op-get instead."""
        text = support.UAPI_HEADER.read_text(encoding='utf-8')
        for name in (*self.LIMITS, 'CTLV_DEFAULT_QUERY_REPLY_LEN'):
            self.assertNotIn(name, text)

    def test_header_lengths_match_the_kernel_assertions(self) -> None:
        text = support.CORE_SOURCE.read_text(encoding='utf-8')
        self.assertIn(f'sizeof(struct ctlv_hdr) == {HDR}', text)
        self.assertIn(f'sizeof(struct ctlv_event_hdr) == {EVENT_HDR}', text)
        self.assertEqual(HDR, sizes.HDR_LEN)
        self.assertEqual(EVENT_HDR, sizes.EVENT_HDR_LEN)

    def test_alignment(self) -> None:
        self.assertEqual(0, sizes.align(0))
        self.assertEqual(8, sizes.align(1))
        self.assertEqual(8, sizes.align(8))
        self.assertEqual(16, sizes.align(9))


if __name__ == '__main__':
    unittest.main()
