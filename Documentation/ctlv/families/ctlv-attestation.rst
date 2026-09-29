.. SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
.. Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from:
..	Documentation/ctlv/specs/ctlv-attestation.yaml
.. CTLV-GEN doc rst

================
ctlv-attestation
================

:Family ID: 3
:Chain: ctlv -> ctlv-attestation
:Specification: Documentation/ctlv/specs/ctlv-attestation.yaml

This family inherits ``ctlv`` and its page describes what ``ctlv-attestation``
declares itself, together with the extension points it fills. An inherited op
or attr keeps the name and the description its own family gives it, so a
program speaking to a device of this family uses both pages.

Enumerations and flags
======================

evidence-representation
-----------------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = raw
    2 = tcb-digest

evidence-blob-class
-------------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = primary
    2 = auxiliary
    3 = manifest
    4 = cert-chain
    5 = spdm-vca
    6 = spdm-measurements
    7 = tdisp-report
    8 = signature
    9 = event-log

mr-log-event
------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = info
    2 = extend

tdisp-state
-----------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = unlocked
    2 = locked
    3 = run
    4 = error

device-trust-state
------------------

A payload restricted to these values. An unknown value is refused on a request
and tolerated on a reply, so a caller built against an older specification
keeps working.

::

    1 = unset
    2 = none
    3 = adversary
    4 = auto
    5 = tcb

evidence-capabilities
---------------------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = evidence-accept
    bit 1 = mr-selection
    bit 2 = privlevel
    bit 3 = service

mr-device-capabilities
----------------------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = read
    bit 1 = extend
    bit 2 = dynamic-topology

devsec-capabilities
-------------------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = run

mr-capabilities
---------------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = read
    bit 1 = extend
    bit 2 = removable

mr-state-flags
--------------

A payload restricted to these bits. An unknown bit is refused on a request and
tolerated on a reply.

::

    bit 0 = ima-active

Attrs
=====

One complete attr ID per line, with the payload it carries and what the
framework checks it against before a callback sees it. The bounds are ABI:
widening one is a new attr rather than a new bound.

group device
------------

::

    device.generation
        id      0x0003000100000001
        payload unsigned integer
        range   0 to 4294967295
    device.capabilities
        id      0x0003000100000002
        payload nest
        members device-capabilities
    device.info
        id      0x0003000100000003
        payload nest
        members device-info

group capability
----------------

::

    capability.evidence
        id      0x0003000200000001
        payload nest
        members evidence-capability
    capability.mr
        id      0x0003000200000002
        payload nest
        members mr-device-capability
    capability.devsec
        id      0x0003000200000003
        payload nest
        members devsec-capability
    capability.min-challenge-length
        id      0x0003000200000004
        payload unsigned integer
        range   0 to 18446744073709551615
    capability.max-challenge-length
        id      0x0003000200000005
        payload unsigned integer
        range   0 to 18446744073709551615
    capability.max-total-output-length
        id      0x0003000200000006
        payload unsigned integer
        range   0 to 18446744073709551615
    capability.evidence-flags
        id      0x0003000200000007
        payload flags
        values  evidence-capabilities
    capability.mr-flags
        id      0x0003000200000008
        payload flags
        values  mr-device-capabilities
    capability.devsec-flags
        id      0x0003000200000009
        payload flags
        values  devsec-capabilities
    capability.object
        id      0x000300020000000a
        payload nest
        members evidence-object-capability
        repeats up to 16 times
    capability.blob-class
        id      0x000300020000000b
        payload enumeration
        values  evidence-blob-class
    capability.blob-instance
        id      0x000300020000000c
        payload unsigned integer
        range   0 to 4294967295
    capability.format
        id      0x000300020000000d
        payload string
        length  1 to 128
    capability.representation
        id      0x000300020000000e
        payload enumeration
        values  evidence-representation
        repeats up to 2 times
    capability.hash-algo
        id      0x000300020000000f
        payload unsigned integer
        range   0 to 20
        repeats up to 21 times
    capability.max-length
        id      0x0003000200000010
        payload unsigned integer
        range   0 to 18446744073709551615

group evidence
--------------

::

    evidence.generation
        id      0x0003000300000001
        payload unsigned integer
        range   0 to 4294967295
    evidence.accepted-generation
        id      0x0003000300000002
        payload unsigned integer
        range   0 to 4294967295
    evidence.challenge
        id      0x0003000300000003
        payload binary
        length  0 to 4096
    evidence.object
        id      0x0003000300000004
        payload nest
        members evidence-object-request
        repeats up to 16 times
    evidence.blob-class
        id      0x0003000300000005
        payload enumeration
        values  evidence-blob-class
    evidence.blob-instance
        id      0x0003000300000006
        payload unsigned integer
        range   0 to 4294967295
    evidence.format
        id      0x0003000300000007
        payload string
        length  1 to 128
    evidence.representation
        id      0x0003000300000008
        payload enumeration
        values  evidence-representation
    evidence.hash-algo
        id      0x0003000300000009
        payload unsigned integer
        range   0 to 20
    evidence.data
        id      0x000300030000000a
        payload blob
        directionoutput
        backingsva, memfd, dma-buf
        length  up to 4294967295
        align   1
    evidence.selection
        id      0x000300030000000b
        payload nest
        members evidence-mr-selection
        repeats up to 256 times
    evidence.privlevel
        id      0x000300030000000c
        payload unsigned integer
        range   0 to 4294967295
    evidence.service-provider
        id      0x000300030000000d
        payload string
        length  1 to 128
    evidence.service-guid
        id      0x000300030000000e
        payload binary
        length  16 to 16
    evidence.service-manifest-version
        id      0x000300030000000f
        payload unsigned integer
        range   0 to 4294967295
    evidence.options
        id      0x0003000300000010
        payload nest
        members evidence-options
    evidence.object-length
        id      0x0003000300000011
        payload unsigned integer
        range   0 to 18446744073709551615
        repeats up to 16 times

group mr
--------

::

    mr.id
        id      0x0003000400000001
        payload unsigned integer
        range   0 to 4294967295
    mr.name
        id      0x0003000400000002
        payload string
        length  1 to 128
    mr.hash-algo
        id      0x0003000400000003
        payload unsigned integer
        range   0 to 20
        repeats up to 21 times
    mr.register-index
        id      0x0003000400000004
        payload unsigned integer
        range   0 to 4294967295
    mr.capabilities
        id      0x0003000400000005
        payload flags
        values  mr-capabilities
    mr.state-flags
        id      0x0003000400000006
        payload flags
        values  mr-state-flags
    mr.item
        id      0x0003000400000007
        payload nest
        members mr-dump-entry
        repeats any number
    mr.options
        id      0x0003000400000008
        payload nest
        members mr-options
    mr.value
        id      0x0003000400000009
        payload nest
        members mr-value
        repeats up to 21 times
    mr.data
        id      0x000300040000000a
        payload binary
        length  16 to 64
    mr.generation
        id      0x000300040000000b
        payload unsigned integer
        range   0 to 4294967295
    mr.selection
        id      0x000300040000000c
        payload nest
        members mr-log-selection
        repeats up to 256 times
    mr.start-generation
        id      0x000300040000000d
        payload unsigned integer
        range   0 to 4294967295
    mr.end-generation
        id      0x000300040000000e
        payload unsigned integer
        range   0 to 4294967295

group record
------------

::

    record.item
        id      0x0003000500000001
        payload nest
        members record-data
    record.format
        id      0x0003000500000002
        payload string
        length  1 to 128
    record.data
        id      0x0003000500000003
        payload binary
        length  0 to 4096

group log
---------

::

    log.entry
        id      0x0003000600000001
        payload nest
        members mr-log-entry
        repeats any number
    log.event
        id      0x0003000600000002
        payload enumeration
        values  mr-log-event
    log.mr
        id      0x0003000600000003
        payload nest
        members mr-log-info
    log.ima
        id      0x0003000600000004
        payload nest
        members mr-log-ima
    log.template-name
        id      0x0003000600000005
        payload string
        length  1 to 128
    log.template-data
        id      0x0003000600000006
        payload binary
        length  0 to 65536
    log.violation
        id      0x0003000600000007
        payload boolean

group devsec
------------

::

    devsec.tdisp-state
        id      0x0003000700000001
        payload enumeration
        values  tdisp-state
    devsec.device-trust
        id      0x0003000700000002
        payload enumeration
        values  device-trust-state

group notification
------------------

::

    notification.entry
        id      0x0003000800000001
        payload nest
        members mr-log-entry

Attr sets
=========

A set is a named group of members that an op selects for a message or a nest
selects for a level, so one shape can be used in more than one place.

evidence-object-capability
--------------------------

::

    required capability.blob-class, capability.format,
             capability.representation, capability.max-length
    optional capability.blob-instance, capability.hash-algo

evidence-capability
-------------------

::

    required capability.min-challenge-length, capability.max-challenge-length,
             capability.max-total-output-length, capability.evidence-flags,
             capability.object

mr-device-capability
--------------------

::

    required capability.mr-flags

devsec-capability
-----------------

::

    required capability.devsec-flags

device-capabilities
-------------------

::

    optional capability.evidence, capability.mr, capability.devsec

device-reply
------------

::

    required device.generation, device.capabilities, device.info

evidence-object-request
-----------------------

::

    required evidence.blob-class, evidence.format, evidence.representation,
             evidence.data
    optional evidence.blob-instance, evidence.hash-algo

evidence-mr-selection
---------------------

::

    required mr.id, evidence.hash-algo

evidence-request
----------------

::

    required evidence.challenge, evidence.object
    optional device.generation, evidence.selection, evidence.privlevel,
             evidence.service-provider, evidence.service-guid,
             evidence.service-manifest-version, evidence.options

evidence-reply
--------------

::

    required device.generation, evidence.object-length
    optional evidence.generation, mr.generation

evidence-generation-request
---------------------------

::

    required device.generation, evidence.generation

mr-props
--------

::

    required mr.name, mr.hash-algo, mr.capabilities, mr.state-flags
    optional mr.register-index

mr-dump-entry
-------------

::

    required mr.id, mr.name, mr.hash-algo, mr.capabilities, mr.state-flags
    optional mr.register-index

mr-id-request
-------------

::

    required mr.id

mr-read-request
---------------

::

    required mr.id
    optional device.generation, mr.options

mr-value
--------

::

    required mr.hash-algo, mr.data

mr-read-reply
-------------

::

    required device.generation, mr.generation, mr.value

record-data
-----------

::

    required record.format, record.data

mr-extend-request
-----------------

::

    required mr.id, mr.value
    optional device.generation, mr.options, record.item

mr-change-reply
---------------

::

    required device.generation, mr.generation

mr-log-selection
----------------

::

    required mr.id

mr-log-request
--------------

::

    optional mr.selection, mr.start-generation, mr.end-generation

mr-log-info
-----------

::

    required mr.name, mr.hash-algo, mr.capabilities
    optional mr.register-index

mr-log-ima
----------

::

    required log.template-name, log.template-data, log.violation

mr-log-entry
------------

::

    required mr.generation, device.generation, mr.id, log.event
    optional mr.value, record.item, log.mr, log.ima

mr-log-reply
------------

::

    optional mr.start-generation, mr.end-generation, log.entry

devsec-state-reply
------------------

::

    required device.generation
    optional evidence.generation, evidence.accepted-generation,
             devsec.tdisp-state, devsec.device-trust

devsec-run-reply
----------------

::

    required device.generation

device-state-event
------------------

::

    required device.generation

evidence-state-event
--------------------

::

    required device.generation
    optional evidence.generation, evidence.accepted-generation

mr-log-event
------------

::

    required notification.entry

Extension points
----------------

A stub is a place a descendant may add members to. It is empty here and stays
empty on this family's own devices; a child that fills it lengthens every
message that reaches it, which is why the lengths below belong to this family
rather than to the op.

::

    (no members)

::

    (no members)

::

    (no members)

Ops
===

One section per op this family declares. An action is executed and
acknowledged, a query is executed and answered in as many fragments as its
answer needs, and an event is produced by the device and read from a subscribed
file description.

device-get
----------

:Op ID: ``0x0003000000000001``
:Type: action
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    device.generation (once, required)
    device.capabilities (once, required)
      capability.evidence (once, optional)
        capability.min-challenge-length (once, required)
        capability.max-challenge-length (once, required)
        capability.max-total-output-length (once, required)
        capability.evidence-flags (once, required)
        capability.object (up to 16 times, required)
          capability.blob-class (once, required)
          capability.format (once, required)
          capability.representation (up to 2 times, required)
          capability.max-length (once, required)
          capability.blob-instance (once, optional)
          capability.hash-algo (up to 21 times, optional)
      capability.mr (once, optional)
        capability.mr-flags (once, required)
      capability.devsec (once, optional)
        capability.devsec-flags (once, required)
    device.info (once, required)
      <device-info> (extension point, required)

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes
    a reply of at most 12936 bytes

evidence-get
------------

:Op ID: ``0x0003000000000002``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: dynamic, so a device may disable and enable it at any time

Request:

::

    evidence.challenge (once, required)
    evidence.object (up to 16 times, required)
      evidence.blob-class (once, required)
      evidence.format (once, required)
      evidence.representation (once, required)
      evidence.data (once, required)
      evidence.blob-instance (once, optional)
      evidence.hash-algo (once, optional)
    device.generation (once, optional)
    evidence.selection (up to 256 times, optional)
      mr.id (once, required)
      evidence.hash-algo (once, required)
    evidence.privlevel (once, optional)
    evidence.service-provider (once, optional)
    evidence.service-guid (once, optional)
    evidence.service-manifest-version (once, optional)
    evidence.options (once, optional)
      <evidence-options> (extension point, required)

Reply:

::

    device.generation (once, required)
    evidence.object-length (up to 16 times, required)
    evidence.generation (once, optional)
    mr.generation (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 26800 bytes
    a reply of at most 472 bytes

evidence-accept
---------------

:Op ID: ``0x0003000000000003``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    device.generation (once, required)
    evidence.generation (once, required)

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 64 bytes

evidence-accept-clear
---------------------

:Op ID: ``0x0003000000000004``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes

mr-get
------

:Op ID: ``0x0003000000000005``
:Type: action
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    mr.id (once, required)

Reply:

::

    mr.name (once, required)
    mr.hash-algo (up to 21 times, required)
    mr.capabilities (once, required)
    mr.state-flags (once, required)
    mr.register-index (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 40 bytes
    a reply of at most 744 bytes

mr-dump
-------

:Op ID: ``0x0003000000000006``
:Type: query
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    mr.item (any number, required)
      mr.id (once, required)
      mr.name (once, required)
      mr.hash-algo (up to 21 times, required)
      mr.capabilities (once, required)
      mr.state-flags (once, required)
      mr.register-index (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes
    a reply buffer of at least 184 bytes
    4096 bytes of reply buffer suggested

mr-read
-------

:Op ID: ``0x0003000000000007``
:Type: query
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    mr.id (once, required)
    device.generation (once, optional)
    mr.options (once, optional)
      <mr-options> (extension point, required)

Reply:

::

    device.generation (once, required)
    mr.generation (once, required)
    mr.value (up to 21 times, required)
      mr.hash-algo (up to 21 times, required)
      mr.data (once, required)

Lengths on a device of this family:

::

    a schema-known request of at most 80 bytes
    a reply buffer of at least 112 bytes
    4096 bytes of reply buffer suggested

mr-extend
---------

:Op ID: ``0x0003000000000008``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    mr.id (once, required)
    mr.value (up to 21 times, required)
      mr.hash-algo (up to 21 times, required)
      mr.data (once, required)
    device.generation (once, optional)
    mr.options (once, optional)
      <mr-options> (extension point, required)
    record.item (once, optional)
      record.format (once, required)
      record.data (once, required)

Reply:

::

    device.generation (once, required)
    mr.generation (once, required)

Lengths on a device of this family:

::

    a schema-known request of at most 16960 bytes
    a reply of at most 64 bytes

mr-log-get
----------

:Op ID: ``0x0003000000000009``
:Type: query
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request:

::

    mr.selection (up to 256 times, optional)
      mr.id (once, required)
    mr.start-generation (once, optional)
    mr.end-generation (once, optional)

Reply:

::

    mr.start-generation (once, optional)
    mr.end-generation (once, optional)
    log.entry (any number, optional)
      mr.generation (once, required)
      device.generation (once, required)
      mr.id (once, required)
      log.event (once, required)
      mr.value (up to 21 times, optional)
        mr.hash-algo (up to 21 times, required)
        mr.data (once, required)
      record.item (once, optional)
        record.format (once, required)
        record.data (once, required)
      log.mr (once, optional)
        mr.name (once, required)
        mr.hash-algo (up to 21 times, required)
        mr.capabilities (once, required)
        mr.register-index (once, optional)
      log.ima (once, optional)
        log.template-name (once, required)
        log.template-data (once, required)
        log.violation (once, required)

Lengths on a device of this family:

::

    a schema-known request of at most 10304 bytes
    a reply buffer of at least 65600 bytes
    131072 bytes of reply buffer suggested

devsec-state-get
----------------

:Op ID: ``0x000300000000000a``
:Type: query
:FD access: read, so an observation or full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply:

::

    device.generation (once, required)
    evidence.generation (once, optional)
    evidence.accepted-generation (once, optional)
    devsec.tdisp-state (once, optional)
    devsec.device-trust (once, optional)

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes
    a reply buffer of at least 40 bytes
    1024 bytes of reply buffer suggested

devsec-run
----------

:Op ID: ``0x000300000000000b``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: dynamic, so a device may disable and enable it at any time

Request:

::

    device.generation (once, required)
    evidence.generation (once, required)

Reply:

::

    device.generation (once, required)

Lengths on a device of this family:

::

    a schema-known request of at most 64 bytes
    a reply of at most 40 bytes

device-state-changed
--------------------

:Op ID: ``0x000300000000000c``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    device.generation (once, required)

Lengths on a device of this family:

::

    a record of at most 64 bytes

evidence-state-changed
----------------------

:Op ID: ``0x000300000000000d``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    device.generation (once, required)
    evidence.generation (once, optional)
    evidence.accepted-generation (once, optional)

Lengths on a device of this family:

::

    a record of at most 112 bytes

mr-log-entry-appended
---------------------

:Op ID: ``0x000300000000000e``
:Type: event

A record of this op reaches a file description that subscribed to it, and is
dropped for one that did not.

Request: none.

Reply:

::

    notification.entry (once, required)
      mr.generation (once, required)
      device.generation (once, required)
      mr.id (once, required)
      log.event (once, required)
      mr.value (up to 21 times, optional)
        mr.hash-algo (up to 21 times, required)
        mr.data (once, required)
      record.item (once, optional)
        record.format (once, required)
        record.data (once, required)
      log.mr (once, optional)
        mr.name (once, required)
        mr.hash-algo (up to 21 times, required)
        mr.capabilities (once, required)
        mr.register-index (once, optional)
      log.ima (once, optional)
        log.template-name (once, required)
        log.template-data (once, required)
        log.violation (once, required)

Lengths on a device of this family:

::

    a record of at most 83496 bytes
