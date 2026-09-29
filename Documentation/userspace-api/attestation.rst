.. SPDX-License-Identifier: GPL-2.0

=========================
Attestation userspace ABI
=========================

A confidential guest proves what it runs to a remote verifier with evidence: a
statement of its measurements, signed by its root of trust and carrying a
challenge the verifier chose, and the logs behind those measurements. An
attestation provider gives its device a node, ``/dev/attestation/attestationN``.
Through it a program asks for evidence, and reads and extends the measurement
registers (MRs) the evidence reports.

The node speaks CTLV, described in :doc:`ctlv`, as a descendant of the
``ctlv-attestation`` family, one family per provider. Every operation below
belongs to ``ctlv-attestation``, so a tool written against it works with any
provider. The specification is
``Documentation/ctlv/specs/ctlv-attestation.yaml`` and the constants are in
``include/uapi/ctlv/ctlv_attestation_gen.h``. Writing a provider is
:doc:`/driver-api/attestation`.

======================  ======  ==========  ================================
Operation               Type    Access      Purpose
======================  ======  ==========  ================================
device-get              action  read        what the device supports
evidence-get            action  read-write  evidence objects
mr-get                  query   read        describe the MRs
mr-read                 query   read        one MR's values
mr-extend               action  read-write  extend one MR
======================  ======  ==========  ================================

``device-get`` is always provided. Every other operation is optional, and a
device provides those its provider implements, which ``op-get`` lists. The
errors below are the family's own. Those of the transport, such as ``EACCES``
for a descriptor without the access an operation needs, are in :doc:`ctlv`,
and once a device is gone or has stopped, every operation fails with
``ENODEV``. A provider's own failure returns its errno, such as ``EIO``.

Generations
===========

A counter tells a program whether what it holds is still current. It is 32
bits wide. An operation that would advance it past its maximum fails with
``EOVERFLOW`` and stops the device.

``mr-generation``
    The state of the MRs. It advances with every extension, and a reply that
    depends on MR state carries the generation it reflects.

Evidence
========

``device-get``
--------------

Describes the device: what it supports and the provider's own information. It
takes no request. The reply carries:

``caps``
    A record for each part the device has:

    ``caps.evidence``
        With ``evidence-get``: one of ``objects`` for each object the device
        produces, up to 16, as `Objects`_ describes.

    ``caps.mr``
        With MRs: ``flags``, which has ``extend`` where ``mr-extend`` is
        provided. ``mr-read`` is always provided with MRs.

``info``
    The device's family's fields, its ``device-info``.

``evidence-get``
----------------

Writes each object the request names into a blob the caller supplies. A
signed object is made by the call, over the challenge, and never reused. The
others, such as an event log, are read as they stand at the call. One call so
returns signed evidence with the log needed to check it. The request carries:

``challenge``
    The challenge, described below. Present exactly when a signed object is
    named.

``objects``
    1 to 16 objects, each naming one of the ``caps.evidence.objects`` of
    ``device-get`` by its ``id`` and giving ``data``, the output blob it is
    written to, sized as described below.

The reply carries:

``results``
    One per object, in request order, each giving the object's format in
    exactly one of ``format`` and ``vendor-format``, and its ``length``, the
    bytes written to its blob.

``mr-generation``
    Where an object binds MRs, the MR generation the evidence reflects.

``EINVAL``
    A signed object is named without a challenge, a challenge is given
    without one, or its length is one a signed object named does not take.
    Two objects are the same.

``EMSGSIZE``
    An object's blob is too short for it. The error information's
    ``required-length`` says how long it has to be.

``ENOENT``
    An object's ``id`` is not one ``device-get`` lists.

``EPROTO``
    The root of trust returned an object in a format the device does not
    list for it.

Access is read-write. Availability is dynamic: a device may switch the
operation off for a time, and meanwhile it fails with ``EOPNOTSUPP``.

The challenge
~~~~~~~~~~~~~

A signed object binds the challenge. Each signed object lists its
``challenge-lengths``, up to 8, each one ``length`` or a range from
``min-length`` to ``max-length``, and the subsystem keeps them within 1 to 4K.
The challenge has to be of a length every signed object named lists.

Pass a nonce from the verifier, or a hash that binds such a nonce to the
caller's own context, such as a key the evidence is to vouch for. Never pass
data a peer chose. Any process that can run ``evidence-get`` can have the
device sign what it passes, and one that passes a peer's bytes makes the
device a signing oracle for that peer.

Objects
~~~~~~~

Each of ``caps.evidence.objects`` describes one object:

``id``
    0 to 15, which a request names it by.

``class``
    What the object is, as ``evidence-class`` below says.

``formats`` and ``vendor-formats``
    The formats it may have.

``max-length``
    The longest it gets, however many objects a call asks for.

``challenge-lengths``
    For a signed object, the challenge lengths it takes.

``evidence-class`` says what an object is:

``signed``
    Evidence the root of trust signs over the challenge. It carries its own
    signature.

``event-log``
    The platform's log of what firmware and the boot chain measured into the
    MRs, for a verifier to replay. Later extends, through ``mr-extend`` or
    otherwise, are not in it.

A blob of the object's ``max-length`` always holds it. A shorter one saves
memory where objects run shorter than their bound, as certificate chains do:
where the object does not fit, ``evidence-get`` fails with ``EMSGSIZE`` and
returns no evidence, and ``required-length`` in the error information is the
length to retry with. The retry collects the evidence anew, and may pass the
same challenge.

Two enums name the formats:

``evidence-format``
    ``ctlv-attestation``'s, for a layout a standard defines, such as the
    CCEL, so every family that produces it uses the same value.

``evidence-vendor-format``
    For a layout one vendor defines, whose values the device's family gives
    through a CTLV enum extension point.

The ABI check of ``regen`` covers both enums. An object lists the formats it
may have, from either enum, in ``formats`` and ``vendor-formats``, and the
reply names the one written in exactly one of ``format`` and
``vendor-format``. A new layout is a new value.

Measurement registers
=====================

Extending an MR folds a digest into its value, so the value commits to every
extension in order. An MR has a value for each hash algorithm it supports, a
bank. An MR's ``id`` is the device's handle for it, and ``mr-desc`` describes
it:

``name``
    The platform's name for the MR, such as ``rtmr0``.

``supported-hash-algorithms``
    One for each bank, in the provider's order.

``register-index``
    The hardware's number for the MR, where it has one.

``caps``
    ``extend`` where the MR may be extended. A provider still refuses an
    extend the platform forbids at the time, as with ``EPERM`` for a TPM PCR
    the current locality may not extend.

Every MR can be read, though a read can still fail, as when the caller lacks
authorization or the platform's state forbids it.

The name, banks, caps and register index describe an MR, and do not change.
The values change with every extension, so the two are read apart.
``mr-get`` describes every MR, which is how a program learns the IDs, or the
one an ID names. It is answered from what the device keeps, while
``mr-read``, which returns one MR's values, asks the hardware. ``mr-read`` and
``mr-extend`` take only the ID.

A hash algorithm is a constant of ``enum hash_algo`` in
``include/uapi/linux/hash_info.h``, such as ``HASH_ALGO_SHA384``.

Nothing reserves an MR, and the kernel does not arbitrate between its
writers. Any program with a read-write descriptor can extend it, and so can
the register's own interface, as for a TPM's PCRs. A program that replays an
MR from its own records has to make sure, through the system's configuration,
that it is the only writer; ``mr-generation`` counts only the extensions made
through the device.

``mr-get``
----------

Describes every MR, in fragments ordered by ID, or only the MR whose ``id``
the request carries. Each is one of ``items``, with its ``id`` and its
``mr-desc``, as `Measurement registers`_ lists it. The query's generation is
the MR generation, so a continuation fails with ``ESTALE`` once any MR has
changed.

``ENOENT``
    No MR has that ID.

Access is read.

``mr-read``
-----------

Returns the current values of the MR whose ``id`` the request carries. The
reply carries:

``mr-generation``
    The MR generation of the values.

``mr-values``
    One for each bank, in ``mr-get``'s order, with its ``hash-algo`` and
    ``data``.

``ENOENT``
    No MR has that ID.

Access is read.

``mr-extend``
-------------

Extends the MR whose ``id`` the request carries. The request carries one of
``mr-values`` for each bank of the MR to extend, in any order, each a
``hash-algo`` and a ``data`` of exactly that algorithm's digest size. The
banks left out keep their values. The device hashes nothing for the caller.

The reply carries ``mr-generation``, the MR generation after the extension.

``ENOENT``
    No MR has that ID.

``EOPNOTSUPP``
    The MR cannot be extended, or the provider cannot extend only some of its
    banks.

``EPERM``
    The platform forbids extending the MR at the time.

``EINVAL``
    The request carries no ``mr-values``, a value is for a bank the MR does not
    have or repeats one, or a digest has the wrong length.

Access is read-write.

Providers
=========

What each provider's family adds, and what its device produces:

.. toctree::
   :maxdepth: 1

   attestation-tdx
