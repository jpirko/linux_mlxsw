# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
"""Tests for the generated lengths.

Every expected value is written as the arithmetic that produces it, so that a
reviewer can check it against the wire format without running anything. These
lengths gate execution in the kernel, so a silent change to one is a UAPI
change.
"""

import re
import unittest

from tests import support

from ctlvgen.lib import semantics
from ctlvgen.lib import sizes

# sizeof(struct ctlv_attr) and sizeof(struct ctlv_event_hdr).
HDR = 16
EVENT_HDR = 24

# A 64-bit payload: integers, enums and flags. Already 8-aligned.
SCALAR = HDR + 8


def string_attr(max_length):
    """A string carries max-length payload bytes plus one terminating NUL."""
    return sizes.align(HDR + max_length + 1)


def binary_attr(max_length):
    return sizes.align(HDR + max_length)


def blob_attr(descriptor_attrs):
    return HDR + descriptor_attrs * SCALAR


def leaf(attr):
    return semantics.Node(attr, True, 1, (), ())


class TestAttrLengths(unittest.TestCase):
    """One occurrence of one attr, for each primary form."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture-child')

    def assert_len(self, full_name, expected):
        node = leaf(self.family.attrs[full_name])
        self.assertEqual(expected, sizes.attr_max_len(node))

    def test_integer(self):
        self.assert_len('scalar.u', HDR + 8)
        self.assert_len('scalar.s', HDR + 8)
        self.assert_len('scalar.freq', HDR + 8)

    def test_bool_carries_one_byte_and_is_padded_like_the_rest(self):
        self.assert_len('scalar.flag', HDR + 1)
        node = leaf(self.family.attrs['scalar.flag'])
        self.assertEqual(SCALAR, sizes.align(sizes.attr_max_len(node)))

    def test_enum_and_flags(self):
        self.assert_len('scalar.mode', HDR + 8)
        self.assert_len('scalar.quirks', HDR + 8)

    def test_string(self):
        self.assert_len('scalar.text', HDR + 15 + 1)
        self.assert_len('child.tag', HDR + 31 + 1)

    def test_binary(self):
        self.assert_len('scalar.fixed', HDR + 8)
        self.assert_len('scalar.bounded', HDR + 64)
        self.assert_len('child.dump', HDR + 4096)

    def test_blob_takes_its_widest_permitted_backing(self):
        # va is type, address and length; memfd and dma-buf add an offset.
        self.assert_len('region.input-va', blob_attr(3))
        self.assert_len('region.output-fd', blob_attr(4))
        self.assert_len('region.inout-any', blob_attr(4))

    def test_blob_descriptor_counts_match_the_core_family(self):
        core = support.load('ctlv')
        members = [attr.name for attr in core.groups['blob'].attrs]
        self.assertEqual(['type', 'address', 'fd', 'offset', 'length'],
                         members)
        self.assertEqual(3, sizes.BLOB_DESCRIPTOR_ATTRS['va'])
        self.assertEqual(4, sizes.BLOB_DESCRIPTOR_ATTRS['memfd'])
        self.assertEqual(4, sizes.BLOB_DESCRIPTOR_ATTRS['dma-buf'])


class TestFixtureSizes(unittest.TestCase):
    """Independently calculated lengths for every op shape."""

    @classmethod
    def setUpClass(cls):
        cls.family = support.load_family('ctlv-fixture')

    def sizes_of(self, name):
        return support.op(self.family, name).sizes

    def test_empty_request_root(self):
        got = self.sizes_of('noop')
        self.assertEqual(HDR, got.request_max_len)
        self.assertEqual(0, got.action_reply_max_len)

    def test_every_primitive_in_one_request(self):
        got = self.sizes_of('scalars-set')
        # root + u + s + delta + freq + size + mode + quirks, the one byte of
        # flag padded to the same, then the three that are not bare 64-bit
        self.assertEqual(HDR + 7 * SCALAR + sizes.align(HDR + 1) +
                         string_attr(15) + binary_attr(8) + binary_attr(64),
                         got.request_max_len)
        self.assertEqual(344, got.request_max_len)

    def test_action_with_a_bounded_reply(self):
        # root + scalar.u
        self.assertEqual(HDR + SCALAR,
                         self.sizes_of('scalars-set').action_reply_max_len)
        self.assertEqual(40, self.sizes_of('scalars-set')
                         .action_reply_max_len)

    def test_finite_repetition(self):
        # Three scalars, another optional scalar and two scalar-pair nests.
        self.assertEqual(HDR + 4 * SCALAR + 2 * (HDR + 2 * SCALAR),
                         self.sizes_of('counts-get').request_max_len)
        self.assertEqual(240, self.sizes_of('counts-get').request_max_len)

    def test_query_reply_minimum_is_the_largest_leaf_and_its_path(self):
        # A flat reply of one repeated scalar needs room for one of them.
        got = self.sizes_of('counts-get')
        self.assertEqual(HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(40, got.query_reply_min_len)

    def test_nested_unlimited_attrs_at_several_depths(self):
        got = self.sizes_of('nests-dump')
        # root + level1 + level2 + level3 + scalar.u
        self.assertEqual(4 * HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(88, got.query_reply_min_len)

    def test_declared_suggested_reply_length_is_kept(self):
        self.assertEqual(4096,
                         self.sizes_of('nests-dump')
                         .query_reply_suggested_len)

    def test_request_with_every_blob_backing(self):
        got = self.sizes_of('blobs-submit')
        # Three descriptors of one blob each, and three of the repeated one.
        self.assertEqual(HDR + 4 * blob_attr(3) + 2 * blob_attr(4),
                         got.request_max_len)
        self.assertEqual(592, got.request_max_len)
        self.assertEqual(0, got.action_reply_max_len)

    def test_a_collapsed_selection_is_counted_once(self):
        # scalar.u is selected as required and again as optional
        self.assertEqual(HDR + 2 * SCALAR,
                         self.sizes_of('collapse-check').request_max_len)
        self.assertEqual(64, self.sizes_of('collapse-check').request_max_len)

    def test_event_record_includes_its_header(self):
        changed = self.sizes_of('thing-changed')
        # root + scalar.u + scalar.s
        self.assertEqual(EVENT_HDR + HDR + 2 * SCALAR,
                         changed.event_record_max_len)
        self.assertEqual(88, changed.event_record_max_len)

    def test_payload_free_event(self):
        self.assertEqual(EVENT_HDR + HDR,
                         self.sizes_of('pinged').event_record_max_len)
        self.assertEqual(40, self.sizes_of('pinged').event_record_max_len)

    def test_an_event_has_no_request_length(self):
        self.assertEqual(0, self.sizes_of('pinged').request_max_len)


class TestInheritedSizes(unittest.TestCase):
    """Filling a stub changes the ops the child inherited."""

    @classmethod
    def setUpClass(cls):
        cls.parent = support.load_family('ctlv-fixture')
        cls.child = support.load_family('ctlv-fixture-child')

    def test_stub_expansion_adds_a_shallower_path(self):
        # The tag the child puts in nest.extension has a shorter path than
        # the level3 chain that sets the minimum, so the minimum stays.
        extension_path = HDR + HDR + string_attr(31)
        self.assertEqual(80, extension_path)
        parent_min = support.op(self.parent, 'nests-dump') \
            .sizes.query_reply_min_len
        self.assertLess(extension_path, parent_min)
        self.assertEqual(parent_min,
                         support.op(self.child, 'nests-dump')
                         .sizes.query_reply_min_len)

    def test_a_reply_of_the_filled_stub_alone(self):
        got = support.op(self.child, 'child-info-get').sizes
        # root + child.tag, the largest leaf of the reply
        self.assertEqual(HDR + string_attr(31), got.query_reply_min_len)
        self.assertEqual(64, got.query_reply_min_len)

    def test_child_query_minimum(self):
        got = support.op(self.child, 'child-dump').sizes
        # root + child.dump, the largest leaf of the reply
        self.assertEqual(HDR + binary_attr(4096), got.query_reply_min_len)
        self.assertEqual(4128, got.query_reply_min_len)

    def test_suggested_reply_length_defaults(self):
        self.assertEqual(sizes.DEFAULT_QUERY_REPLY_LEN,
                         support.op(self.child, 'child-dump')
                         .sizes.query_reply_suggested_len)


class TestCoreSizes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.family = support.load('ctlv')

    def sizes_of(self, name):
        return support.op(self.family, name).sizes

    def test_ops_dump(self):
        got = self.sizes_of('ops-dump')
        # root + op.entry + one scalar member
        self.assertEqual(HDR + HDR + SCALAR, got.query_reply_min_len)
        self.assertEqual(56, got.query_reply_min_len)
        self.assertEqual(HDR, got.request_max_len)

    def test_subscriptions_carry_up_to_256_op_ids(self):
        self.assertEqual(HDR + 256 * SCALAR,
                         self.sizes_of('subscriptions-add').request_max_len)
        self.assertEqual(6160,
                         self.sizes_of('subscriptions-add').request_max_len)

    def test_op_changed_carries_one_op(self):
        # root + op.id + op.disabled, whose one byte is padded like the rest,
        # + op.state-generation
        self.assertEqual(EVENT_HDR + HDR + 3 * SCALAR,
                         self.sizes_of('op-changed').event_record_max_len)

    def test_control_events(self):
        self.assertEqual(EVENT_HDR + HDR + 3 * SCALAR,
                         self.sizes_of('event-overflow')
                         .event_record_max_len)
        self.assertEqual(EVENT_HDR + HDR,
                         self.sizes_of('device-removed')
                         .event_record_max_len)

    def test_op_schema_get_reaches_the_nesting_limit(self):
        """A re-entered level can repeat until the transport stops it."""
        got = self.sizes_of('op-schema-get')
        self.assertEqual((sizes.MAX_NEST_DEPTH + 1) * HDR + SCALAR,
                         got.query_reply_min_len)
        self.assertEqual(552, got.query_reply_min_len)
        self.assertEqual(HDR + SCALAR, got.request_max_len)

    def test_the_deepest_description_fits_what_it_promised(self):
        """The root, the nest naming the message, one per declared level."""
        got = self.sizes_of('op-schema-get')
        path = (1 + 1 + sizes.MAX_DECLARED_NEST_DEPTH) * HDR
        self.assertEqual(path + SCALAR, got.query_reply_min_len)

    def test_error_information_fits_the_error_limit(self):
        attrs = self.family.groups['error'].attrs
        total = HDR + sum(sizes.align(sizes.attr_max_len(leaf(attr)))
                          for attr in attrs)
        self.assertEqual(HDR + 6 * SCALAR + string_attr(255), total)
        self.assertEqual(432, total)
        self.assertLessEqual(total, sizes.MAX_ERROR_LEN)


class TestLimits(unittest.TestCase):
    """The Python constants and the UAPI header must not drift apart."""

    def test_constants_match_the_uapi_header(self):
        text = support.UAPI_HEADER.read_text(encoding='utf-8')
        expected = {
            'CTLV_ALIGNTO': sizes.ALIGNTO,
            'CTLV_MAX_INLINE_MESSAGE_LEN': sizes.MAX_INLINE_MESSAGE_LEN,
            'CTLV_MAX_ERROR_LEN': sizes.MAX_ERROR_LEN,
            'CTLV_MAX_NEST_DEPTH': sizes.MAX_NEST_DEPTH,
            'CTLV_DEFAULT_QUERY_REPLY_LEN': sizes.DEFAULT_QUERY_REPLY_LEN,
            'CTLV_MAX_EVENT_QUEUE_LEN': sizes.MAX_EVENT_QUEUE_LEN,
        }
        for name, value in expected.items():
            found = re.search(rf'^#define {name}\s+(\S+)', text, re.M)
            self.assertIsNotNone(found, f'{name} is not defined')
            self.assertEqual(value, int(found.group(1), 0), name)

    def test_header_lengths_match_the_kernel_assertions(self):
        text = support.CORE_SOURCE.read_text(encoding='utf-8')
        self.assertIn(f'sizeof(struct ctlv_attr) == {HDR}', text)
        self.assertIn(f'sizeof(struct ctlv_event_hdr) == {EVENT_HDR}', text)
        self.assertEqual(HDR, sizes.ATTR_HDR_LEN)
        self.assertEqual(EVENT_HDR, sizes.EVENT_HDR_LEN)

    def test_alignment(self):
        self.assertEqual(0, sizes.align(0))
        self.assertEqual(8, sizes.align(1))
        self.assertEqual(8, sizes.align(8))
        self.assertEqual(16, sizes.align(9))


if __name__ == '__main__':
    unittest.main()
