.. SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
.. Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from:
..	Documentation/ctlv/specs/ctlv-test.yaml
.. CTLV-GEN doc rst

=========
ctlv-test
=========

:Family ID: 1
:Chain: ctlv -> ctlv-test
:Specification: Documentation/ctlv/specs/ctlv-test.yaml

This family inherits ``ctlv`` and its page describes what ``ctlv-test``
declares itself, together with the extension points it fills. An inherited op
or attr keeps the name and the description its own family gives it, so a
program speaking to a device of this family uses both pages.

Enumerations and flags
======================

echo-mode
---------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = idle
    2 = busy
    3 = auto

echo-quirk
----------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = slow
    bit 1 = noisy

Attrs
=====

One complete attr ID per line, with the payload it carries and what the
framework checks it against before a callback sees it. The bounds are ABI:
widening one is a new attr rather than a new bound.

group probe
-----------

::

    probe.token
        id      0x0001000100000001
        payload unsigned integer
        range   0 to 18446744073709551615
    probe.extension
        id      0x0001000100000002
        payload nest
        members extension-point

group echo
----------

::

    echo.u
        id      0x0001000200000001
        payload unsigned integer
        range   1 to 4096
    echo.s
        id      0x0001000200000002
        payload signed integer
        range   -100 to 100
    echo.flag
        id      0x0001000200000003
        payload boolean
    echo.mode
        id      0x0001000200000004
        payload enumeration
        values  echo-mode
    echo.quirks
        id      0x0001000200000005
        payload flags
        values  echo-quirk
    echo.text
        id      0x0001000200000006
        payload string
        length  1 to 15
    echo.data
        id      0x0001000200000007
        payload binary
        length  4 to 32
    echo.count
        id      0x0001000200000008
        payload unsigned integer
        range   0 to 18446744073709551615
        repeats up to 3 times
    echo.item
        id      0x0001000200000009
        payload nest
        members echo-item
        repeats up to 2 times
    echo.region
        id      0x000100020000000a
        payload blob
        directioninput
        backingsva, memfd, dma-buf
        length  up to 4096
        align   8
    echo.page
        id      0x000100020000000b
        payload blob
        directionoutput
        backingsmemfd
        length  up to 65536
        align   4096

group item
----------

::

    item.index
        id      0x0001000300000001
        payload unsigned integer
        range   0 to 7
    item.label
        id      0x0001000300000002
        payload string
        length  0 to 7
    item.leaf
        id      0x0001000300000003
        payload nest
        members echo-leaf

group series
------------

::

    series.chunks
        id      0x0001000400000001
        payload unsigned integer
        range   0 to 8
    series.items
        id      0x0001000400000002
        payload unsigned integer
        range   0 to 8
    series.values
        id      0x0001000400000003
        payload unsigned integer
        range   0 to 8
    series.totals
        id      0x0001000400000004
        payload unsigned integer
        range   0 to 8
    series.chunk
        id      0x0001000400000005
        payload nest
        members series-chunk
        repeats any number
    series.item
        id      0x0001000400000006
        payload nest
        members series-item
        repeats any number
    series.value
        id      0x0001000400000007
        payload unsigned integer
        range   0 to 18446744073709551615
        repeats any number
    series.total
        id      0x0001000400000008
        payload unsigned integer
        range   0 to 18446744073709551615
        repeats any number
    series.index
        id      0x0001000400000009
        payload unsigned integer
        range   0 to 8

group ping
----------

::

    ping.count
        id      0x0001000500000001
        payload unsigned integer
        range   0 to 64
    ping.serial
        id      0x0001000500000002
        payload unsigned integer
        range   0 to 18446744073709551615
    ping.payload
        id      0x0001000500000003
        payload unsigned integer
        range   0 to 1024
    ping.abort
        id      0x0001000500000004
        payload boolean
    ping.empty
        id      0x0001000500000005
        payload boolean
    ping.filler
        id      0x0001000500000006
        payload binary
        length  0 to 1024

group transfer
--------------

::

    transfer.source
        id      0x0001000600000001
        payload blob
        directioninput
        backingsva, memfd, dma-buf
        length  up to 268435456
        align   8
    transfer.target
        id      0x0001000600000002
        payload blob
        directionoutput
        backingsva, memfd, dma-buf
        length  up to 268435456
        align   8
    transfer.scratch
        id      0x0001000600000003
        payload blob
        directioninout
        backingsva, memfd, dma-buf
        length  up to 268435456
        align   8
    transfer.piece
        id      0x0001000600000004
        payload blob
        directioninput
        backingsva, memfd, dma-buf
        length  up to 1048576
        align   1
        repeats up to 4 times
    transfer.fill
        id      0x0001000600000005
        payload unsigned integer
        range   0 to 255
    transfer.skip
        id      0x0001000600000006
        payload boolean
    transfer.twice
        id      0x0001000600000007
        payload boolean
    transfer.hold
        id      0x0001000600000008
        payload boolean
    transfer.sum
        id      0x0001000600000009
        payload unsigned integer
        range   0 to 18446744073709551615
    transfer.written
        id      0x000100060000000a
        payload unsigned integer
        range   0 to 18446744073709551615
    transfer.distinct
        id      0x000100060000000b
        payload boolean

Attr sets
=========

A set is a named group of members that an op selects for a message or a nest
selects for a level, so one shape can be used in more than one place.

token-info
----------

::

    optional probe.token

probe-report
------------

::

    optional probe.token, probe.extension

echo-leaf
---------

::

    required item.index

echo-item
---------

::

    required item.index
    optional item.label, item.leaf

echo-message
------------

::

    required echo.u
    optional echo.s, echo.flag, echo.mode, echo.quirks, echo.text, echo.data,
             echo.count, echo.item, echo.region, echo.page, probe.extension

echo-reply
----------

::

    optional echo.u, echo.s, echo.flag, echo.mode, echo.quirks, echo.text,
             echo.data, echo.count, echo.item, probe.extension

series-item
-----------

::

    required series.index
    optional series.value

series-chunk
------------

::

    required series.index
    optional series.item

series-request
--------------

::

    optional series.chunks, series.items, series.values, series.totals

series-reply
------------

::

    optional series.chunk, series.total

ping-request
------------

::

    optional ping.count, ping.serial, ping.payload, ping.abort, ping.empty

ping-report
-----------

::

    required ping.serial
    optional ping.filler

transfer-request
----------------

::

    optional transfer.source, transfer.target, transfer.scratch,
             transfer.piece, transfer.fill, transfer.skip, transfer.twice,
             transfer.hold

transfer-report
---------------

::

    optional transfer.sum, transfer.written, transfer.distinct

Extension points
----------------

A stub is a place a descendant may add members to. It is empty here and stays
empty on this family's own devices; a child that fills it lengthens every
message that reaches it, which is why the lengths below belong to this family
rather than to the op.

::

    (no members)

Ops
===

One section per op this family declares. An action is executed and
acknowledged, a query is executed and answered in as many fragments as its
answer needs, and an event is produced by the device and read from a subscribed
file description.

noop
----

:Op ID: ``0x0001000000000001``
:Type: action
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes

probe-get
---------

:Op ID: ``0x0001000000000002``
:Type: query
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    probe.token (once, optional)

Reply:

::

    probe.token (once, optional)
    probe.extension (once, optional)
      <extension-point> (extension point, required)

Lengths on a device of this family:

::

    a schema-known request of at most 40 bytes
    a reply buffer of at least 40 bytes
    65536 bytes of reply buffer suggested

dynamic-noop
------------

:Op ID: ``0x0001000000000003``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: dynamic, so a device may disable and enable it at any time

Request: an empty root.

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes

dynamic-toggle
--------------

:Op ID: ``0x0001000000000004``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    probe.token (once, optional)

Reply:

::

    probe.token (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 40 bytes
    a reply of at most 40 bytes

pinged
------

:Op ID: ``0x0001000000000005``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply: an empty root.

Lengths on a device of this family:

::

    a record of at most 40 bytes

echo
----

:Op ID: ``0x0001000000000006``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    echo.u (once, required)
    echo.s (once, optional)
    echo.flag (once, optional)
    echo.mode (once, optional)
    echo.quirks (once, optional)
    echo.text (once, optional)
    echo.data (once, optional)
    echo.count (up to 3 times, optional)
    echo.item (up to 2 times, optional)
      item.index (once, required)
      item.label (once, optional)
      item.leaf (once, optional)
        item.index (once, required)
    echo.region (once, optional)
    echo.page (once, optional)
    probe.extension (once, optional)
      <extension-point> (extension point, required)

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 736 bytes

echo-reply
----------

:Op ID: ``0x0001000000000007``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    echo.u (once, required)
    echo.s (once, optional)
    echo.flag (once, optional)
    echo.mode (once, optional)
    echo.quirks (once, optional)
    echo.text (once, optional)
    echo.data (once, optional)
    echo.count (up to 3 times, optional)
    echo.item (up to 2 times, optional)
      item.index (once, required)
      item.label (once, optional)
      item.leaf (once, optional)
        item.index (once, required)
    echo.region (once, optional)
    echo.page (once, optional)
    probe.extension (once, optional)
      <extension-point> (extension point, required)

Reply:

::

    echo.u (once, optional)
    echo.s (once, optional)
    echo.flag (once, optional)
    echo.mode (once, optional)
    echo.quirks (once, optional)
    echo.text (once, optional)
    echo.data (once, optional)
    echo.count (up to 3 times, optional)
    echo.item (up to 2 times, optional)
      item.index (once, required)
      item.label (once, optional)
      item.leaf (once, optional)
        item.index (once, required)
    probe.extension (once, optional)
      <extension-point> (extension point, required)

Lengths on a device of this family:

::

    a schema-known request of at most 736 bytes
    a reply of at most 512 bytes

series-dump
-----------

:Op ID: ``0x0001000000000008``
:Type: query
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    series.chunks (once, optional)
    series.items (once, optional)
    series.values (once, optional)
    series.totals (once, optional)

Reply:

::

    series.chunk (any number, optional)
      series.index (once, required)
      series.item (any number, optional)
        series.index (once, required)
        series.value (any number, optional)
    series.total (any number, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 112 bytes
    a reply buffer of at least 72 bytes
    4096 bytes of reply buffer suggested

ping
----

:Op ID: ``0x0001000000000009``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    ping.count (once, optional)
    ping.serial (once, optional)
    ping.payload (once, optional)
    ping.abort (once, optional)
    ping.empty (once, optional)

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 136 bytes

beeped
------

:Op ID: ``0x000100000000000a``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    ping.serial (once, required)
    ping.filler (once, optional)

Lengths on a device of this family:

::

    a record of at most 1104 bytes

transfer
--------

:Op ID: ``0x000100000000000b``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    transfer.source (once, optional)
    transfer.target (once, optional)
    transfer.scratch (once, optional)
    transfer.piece (up to 4 times, optional)
    transfer.fill (once, optional)
    transfer.skip (once, optional)
    transfer.twice (once, optional)
    transfer.hold (once, optional)

Reply:

::

    transfer.sum (once, optional)
    transfer.written (once, optional)
    transfer.distinct (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 896 bytes
    a reply of at most 88 bytes
