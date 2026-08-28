.. SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
.. Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from:
..	Documentation/ctlv/specs/ctlv-test-child.yaml
.. CTLV-GEN doc rst

===============
ctlv-test-child
===============

:Family ID: 2
:Chain: ctlv -> ctlv-test -> ctlv-test-child
:Specification: Documentation/ctlv/specs/ctlv-test-child.yaml

This family inherits ``ctlv-test`` and its page describes what
``ctlv-test-child`` declares itself, together with the extension points it
fills. An inherited op or attr keeps the name and the description its own
family gives it, so a program speaking to a device of this family uses both
pages.

Attrs
=====

One complete attr ID per line, with the payload it carries and what the
framework checks it against before a callback sees it. The bounds are ABI:
widening one is a new attr rather than a new bound.

group child
-----------

::

    child.extra
        id      0x0002000100000001
        payload unsigned integer
        range   0 to 18446744073709551615

Extension points this family fills
==================================

Each of these completes an extension point an ancestor declared. What it adds
appears in every message of every op that reaches that point, so an inherited
op admits a longer message on a device of this family than on a device of the
family that declared it.

extension-point of ctlv-test
----------------------------

::

    child.extra (once, optional)

Ops
===

One section per op this family declares. An action is executed and
acknowledged, a query is executed and answered in as many fragments as its
answer needs, and an event is produced by the device and read from a subscribed
file description.

extra-noop
----------

:Op ID: ``0x0002000000000001``
:Type: action
:FD access: read-write, so only a full-control fd may execute it
:Availability: fixed, so a device that publishes it always answers it

Request: an empty root.

Reply: none.

Lengths on a device of this family:

::

    a schema-known request of at most 16 bytes
