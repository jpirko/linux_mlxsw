.. SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
.. Do not edit, regenerate with tools/ctlv/ctlv-regen.sh, from:
..	Documentation/ctlv/specs/ctlv-attestation-sim.yaml
.. CTLV-GEN doc rst

====================
ctlv-attestation-sim
====================

:Family ID: 4
:Chain: ctlv -> ctlv-attestation -> ctlv-attestation-sim
:Specification: Documentation/ctlv/specs/ctlv-attestation-sim.yaml

This family inherits ``ctlv-attestation`` and its page describes what
``ctlv-attestation-sim`` declares itself, together with the extension points it
fills. An inherited op or attr keeps the name and the description its own
family gives it, so a program speaking to a device of this family uses both
pages.

Attrs
=====

One complete attr ID per line, with the payload it carries and what the
framework checks it against before a callback sees it. The bounds are ABI:
widening one is a new attr rather than a new bound.

group sim
---------

::

    sim.name
        id      0x0004000100000001
        payload string
        length  1 to 63

Extension points this family fills
==================================

Each of these completes an extension point an ancestor declared. What it adds
appears in every message of every op that reaches that point, so an inherited
op admits a longer message on a device of this family than on a device of the
family that declared it.

device-info of ctlv-attestation
-------------------------------

::

    sim.name (once, required)
