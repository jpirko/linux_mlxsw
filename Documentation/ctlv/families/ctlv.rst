.. SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
.. Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from:
..	Documentation/ctlv/specs/ctlv.yaml
.. CTLV-GEN doc rst

====
ctlv
====

:Family ID: 0
:Chain: ctlv
:Specification: Documentation/ctlv/specs/ctlv.yaml

This is the family every other family inherits: its ops are the ones the
framework answers itself, on every device, whatever family registered it.

Enumerations and flags
======================

op-type
-------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = action
    2 = query
    3 = event

op-availability
---------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = fixed
    2 = dynamic

blob-type
---------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = va
    2 = memfd
    3 = dma-buf

blob-direction
--------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = input
    2 = output
    3 = inout

validation-reason
-----------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = malformed-message
    2 = unknown-attr
    3 = unknown-flag
    4 = nesting-too-deep
    5 = missing-attr
    6 = too-many-occurrences
    7 = value-out-of-range
    8 = bad-length
    9 = bad-string
    10 = bad-blob-descriptor
    11 = stale-generation

Attrs
=====

One complete attr ID per line, with the payload it carries and what the
framework checks it against before a callback sees it. The bounds are ABI:
widening one is a new attr rather than a new bound.

group family
------------

::

    family.id
        id      0x0000000100000001
        payload unsigned integer
        range   0 to 18446744073709551615
    family.name
        id      0x0000000100000002
        payload string
        length  0 to 63
    family.entry
        id      0x0000000100000003
        payload nest
        members family-entry
        repeats any number

group blob
----------

::

    blob.type
        id      0x0000000200000001
        payload enumeration
        values  blob-type
    blob.address
        id      0x0000000200000002
        payload unsigned integer
        range   0 to 18446744073709551615
    blob.fd
        id      0x0000000200000003
        payload unsigned integer
        range   0 to 2147483647
    blob.offset
        id      0x0000000200000004
        payload unsigned integer
        range   0 to 18446744073709551615
    blob.length
        id      0x0000000200000005
        payload unsigned integer
        range   1 to 18446744073709551615

group error
-----------

::

    error.reason
        id      0x0000000300000001
        payload enumeration
        values  validation-reason
    error.message
        id      0x0000000300000002
        payload string
        length  0 to 255
    error.bad-attr-offset
        id      0x0000000300000003
        payload unsigned integer
        range   0 to 18446744073709551615
    error.nest-offset
        id      0x0000000300000004
        payload unsigned integer
        range   0 to 18446744073709551615
    error.missing-attr-id
        id      0x0000000300000005
        payload unsigned integer
        range   0 to 18446744073709551615
    error.expected-generation
        id      0x0000000300000006
        payload unsigned integer
        range   0 to 18446744073709551615
    error.current-generation
        id      0x0000000300000007
        payload unsigned integer
        range   0 to 18446744073709551615

group op
--------

::

    op.id
        id      0x0000000400000001
        payload unsigned integer
        range   1 to 18446744073709551615
    op.type
        id      0x0000000400000002
        payload enumeration
        values  op-type
    op.availability
        id      0x0000000400000003
        payload enumeration
        values  op-availability
    op.disabled
        id      0x0000000400000004
        payload boolean
    op.request-max-len
        id      0x0000000400000005
        payload unsigned integer
        range   0 to 18446744073709551615
    op.action-reply-max-len
        id      0x0000000400000006
        payload unsigned integer
        range   0 to 18446744073709551615
    op.query-reply-min-len
        id      0x0000000400000007
        payload unsigned integer
        range   0 to 18446744073709551615
    op.query-reply-suggested-len
        id      0x0000000400000008
        payload unsigned integer
        range   0 to 18446744073709551615
    op.event-record-max-len
        id      0x0000000400000009
        payload unsigned integer
        range   0 to 18446744073709551615
    op.state-generation
        id      0x000000040000000a
        payload unsigned integer
        range   0 to 18446744073709551615
    op.entry
        id      0x000000040000000b
        payload nest
        members op-entry
        repeats any number

group schema
------------

::

    schema.attr-id
        id      0x0000000500000001
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.kind
        id      0x0000000500000002
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.max-occurrences
        id      0x0000000500000003
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.min
        id      0x0000000500000004
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.max
        id      0x0000000500000005
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.min-length
        id      0x0000000500000006
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.max-length
        id      0x0000000500000007
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.flags-mask
        id      0x0000000500000008
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.unit
        id      0x0000000500000009
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.scale
        id      0x000000050000000a
        payload signed integer
        range   -9223372036854775808 to 9223372036854775807
    schema.blob-direction
        id      0x000000050000000b
        payload enumeration
        values  blob-direction
    schema.blob-backings
        id      0x000000050000000c
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.blob-max-length
        id      0x000000050000000d
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.blob-alignment
        id      0x000000050000000e
        payload unsigned integer
        range   0 to 18446744073709551615
    schema.reenters
        id      0x000000050000000f
        payload unsigned integer
        range   1 to 18446744073709551615
    schema.attrs
        id      0x0000000500000010
        payload nest
        members schema-node
        repeats any number
    schema.optional-attrs
        id      0x0000000500000011
        payload nest
        members schema-node
        repeats any number
    schema.request
        id      0x0000000500000012
        payload nest
        members schema-node
    schema.reply
        id      0x0000000500000013
        payload nest
        members schema-node

group event
-----------

::

    event.op-id
        id      0x0000000600000001
        payload unsigned integer
        range   1 to 18446744073709551615
        repeats up to 256 times
    event.queue-len
        id      0x0000000600000002
        payload unsigned integer
        range   1 to 16777216
        unit    bytes, scale 0
    event.queue-used
        id      0x0000000600000003
        payload unsigned integer
        range   0 to 18446744073709551615
        unit    bytes, scale 0
    event.first-sequence
        id      0x0000000600000004
        payload unsigned integer
        range   0 to 18446744073709551615
    event.last-sequence
        id      0x0000000600000005
        payload unsigned integer
        range   0 to 18446744073709551615
    event.dropped-count
        id      0x0000000600000006
        payload unsigned integer
        range   0 to 18446744073709551615

Attr sets
=========

A set is a named group of members that an op selects for a message or a nest
selects for a level, so one shape can be used in more than one place.

family-entry
------------

::

    required family.id, family.name

op-entry
--------

::

    required op.id, op.type
    optional op.availability, op.disabled, op.request-max-len,
             op.action-reply-max-len, op.query-reply-min-len,
             op.query-reply-suggested-len, op.event-record-max-len

op-schema-request
-----------------

::

    required op.id

schema-node
-----------

::

    required schema.attr-id, schema.kind, schema.max-occurrences
    optional schema.min, schema.max, schema.min-length, schema.max-length,
             schema.flags-mask, schema.unit, schema.scale,
             schema.blob-direction, schema.blob-backings,
             schema.blob-max-length, schema.blob-alignment, schema.reenters,
             schema.attrs, schema.optional-attrs

op-schema-info
--------------

::

    optional schema.request, schema.reply

op-change-info
--------------

::

    required op.id, op.disabled, op.state-generation

error-info
----------

::

    optional error.reason, error.message, error.bad-attr-offset,
             error.nest-offset, error.missing-attr-id,
             error.expected-generation, error.current-generation

event-subscription
------------------

::

    required event.op-id

event-queue-limit
-----------------

::

    required event.queue-len

event-queue-state
-----------------

::

    required event.queue-len, event.queue-used

event-overflow-info
-------------------

::

    required event.first-sequence, event.last-sequence, event.dropped-count

Ops
===

One section per op this family declares. An action is executed and
acknowledged, a query is executed and answered in as many fragments as its
answer needs, and an event is produced by the device and read from a subscribed
file description.

families-dump
-------------

:Op ID: ``0x0000000000000001``
:Type: query
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    family.entry (any number, required)
      family.id (once, required)
      family.name (once, required)

Lengths on a device of this family:

::

    a request of at most 16 bytes
    a reply buffer of at least 112 bytes
    65536 bytes of reply buffer suggested

ops-dump
--------

:Op ID: ``0x0000000000000002``
:Type: query
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    op.entry (any number, required)
      op.id (once, required)
      op.type (once, required)
      op.availability (once, optional)
      op.disabled (once, optional)
      op.request-max-len (once, optional)
      op.action-reply-max-len (once, optional)
      op.query-reply-min-len (once, optional)
      op.query-reply-suggested-len (once, optional)
      op.event-record-max-len (once, optional)

Lengths on a device of this family:

::

    a request of at most 16 bytes
    a reply buffer of at least 56 bytes
    65536 bytes of reply buffer suggested

op-schema-get
-------------

:Op ID: ``0x0000000000000003``
:Type: query
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    op.id (once, required)

Reply:

::

    schema.request (once, optional)
      schema.attr-id (once, required)
      schema.kind (once, required)
      schema.max-occurrences (once, required)
      schema.min (once, optional)
      schema.max (once, optional)
      schema.min-length (once, optional)
      schema.max-length (once, optional)
      schema.flags-mask (once, optional)
      schema.unit (once, optional)
      schema.scale (once, optional)
      schema.blob-direction (once, optional)
      schema.blob-backings (once, optional)
      schema.blob-max-length (once, optional)
      schema.blob-alignment (once, optional)
      schema.reenters (once, optional)
      schema.attrs (any number, optional, re-enters schema-node)
      schema.optional-attrs (any number, optional, re-enters schema-node)
    schema.reply (once, optional)
      schema.attr-id (once, required)
      schema.kind (once, required)
      schema.max-occurrences (once, required)
      schema.min (once, optional)
      schema.max (once, optional)
      schema.min-length (once, optional)
      schema.max-length (once, optional)
      schema.flags-mask (once, optional)
      schema.unit (once, optional)
      schema.scale (once, optional)
      schema.blob-direction (once, optional)
      schema.blob-backings (once, optional)
      schema.blob-max-length (once, optional)
      schema.blob-alignment (once, optional)
      schema.reenters (once, optional)
      schema.attrs (any number, optional, re-enters schema-node)
      schema.optional-attrs (any number, optional, re-enters schema-node)

Lengths on a device of this family:

::

    a request of at most 40 bytes
    a reply buffer of at least 552 bytes
    65536 bytes of reply buffer suggested

subscriptions-add
-----------------

:Op ID: ``0x0000000000000004``
:Type: action
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    event.op-id (up to 256 times, required)

Reply: none.

Lengths on a device of this family:

::

    a request of at most 6160 bytes

subscriptions-remove
--------------------

:Op ID: ``0x0000000000000005``
:Type: action
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    event.op-id (up to 256 times, required)

Reply: none.

Lengths on a device of this family:

::

    a request of at most 6160 bytes

event-queue-get
---------------

:Op ID: ``0x0000000000000006``
:Type: query
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    event.queue-len (once, required)
    event.queue-used (once, required)

Lengths on a device of this family:

::

    a request of at most 16 bytes
    a reply buffer of at least 40 bytes
    65536 bytes of reply buffer suggested

event-queue-set
---------------

:Op ID: ``0x0000000000000007``
:Type: action
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    event.queue-len (once, required)

Reply: none.

Lengths on a device of this family:

::

    a request of at most 40 bytes

op-changed
----------

:Op ID: ``0x0000000000000008``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    op.id (once, required)
    op.disabled (once, required)
    op.state-generation (once, required)

Lengths on a device of this family:

::

    a record of at most 112 bytes

event-overflow
--------------

:Op ID: ``0x0000000000000009``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    event.first-sequence (once, required)
    event.last-sequence (once, required)
    event.dropped-count (once, required)

Lengths on a device of this family:

::

    a record of at most 112 bytes

device-removed
--------------

:Op ID: ``0x000000000000000a``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply: an empty root.

Lengths on a device of this family:

::

    a record of at most 40 bytes

Messages no op declares
=======================

The framework writes these itself, so they belong to no op: a caller reads one
out of the buffer it offered for a reply.

error-info
----------

::

    error.reason (once, optional)
    error.message (once, optional)
    error.bad-attr-offset (once, optional)
    error.nest-offset (once, optional)
    error.missing-attr-id (once, optional)
    error.expected-generation (once, optional)
    error.current-generation (once, optional)

At most 432 bytes, bounded by CTLV_MAX_ERROR_LEN.
