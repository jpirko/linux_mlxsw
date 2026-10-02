.. SPDX-License-Identifier: GPL-2.0

=========================
Attestation userspace ABI
=========================

A confidential guest proves what it runs to a remote verifier with evidence: a
statement of its measurements, signed by its root of trust and carrying a
challenge the verifier chose, and the logs behind those measurements. An
attestation provider gives its device a node, ``/dev/attestation/attestationN``.
Through it a program asks for evidence, reads and extends the measurement
registers (MRs) the evidence reports, follows a journal of every extension
and, where the provider gates a device on it, runs the device once a verifier
accepts its evidence.

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
evidence-accept         action  read-write  accept evidence, run the device
devsec-state-get        query   read        the device's security state
mr-get                  query   read        describe the MRs
mr-read                 query   read        one MR's values
mr-extend               action  read-write  extend one MR
mr-log-get              query   read        entries of the MR journal
device-state-changed    event               the device generation advanced
evidence-state-changed  event               evidence produced or retired
mr-log-entry-appended   event               a journal entry was added
======================  ======  ==========  ================================

``device-get`` and the events are always provided. Every other operation is
optional, and a device provides those its provider implements, which
``op-get`` lists. The errors below are the family's own. Those of the
transport, such as ``EACCES`` for a descriptor without the access an operation
needs, are in :doc:`ctlv`, and once a device is gone or has stopped, every
operation fails with ``ENODEV``. A provider's own failure returns its errno,
such as ``EIO``.

Generations
===========

Three counters tell a program whether what it holds is still current. Each is
32 bits wide. An operation that would advance one past its maximum fails with
``EOVERFLOW`` and stops the device.

``device-generation``
    The state of the device. It advances when the provider reports a change to
    its native state and when ``evidence-accept`` runs the device. Evidence of
    the previous state is retired with it. A request that carries it fails
    with ``ESTALE`` unless it is current.

``evidence-generation``
    The evidence ``evidence-get`` returned, on a device with
    ``evidence-accept``. Every call returns a new one, which
    ``evidence-accept`` then names. Evidence stays acceptable until the device
    generation advances and, if it binds MRs, until an MR changes.

``mr-generation``
    A position in the MR journal. Every journal entry has its own, and a reply
    that depends on MR state carries the position it reflects.

Evidence
========

``device-get``
--------------

Describes the device: its generation, what it supports and the provider's own
information. It takes no request. The reply carries:

``device-generation``
    The current device generation.

``caps``
    A record for each part the device has:

    ``caps.evidence``
        With ``evidence-get``: ``flags``, and one of ``objects`` for each
        object the device produces, up to 16, as `Objects`_ describes.
        ``flags`` has ``evidence-accept`` where that operation is provided,
        and ``mr-selection``, ``privlevel`` and ``service`` where
        ``evidence-get`` takes the fields of those names.

    ``caps.mr``
        With MRs: ``flags``, which has ``extend`` where ``mr-extend`` is
        provided, and ``dynamic-topology`` where MRs may come and go while
        the device lives. ``mr-read`` is always provided with MRs.

``info``
    The device's family's fields, its ``device-info``.

``evidence-get``
----------------

Writes each object the request names into a blob the caller supplies. A
signed object is made by the call, over the challenge, and never reused. The
others, such as an event log, are read as they stand at the call. One call so
returns signed evidence with the log and certificates needed to check it. The
request carries:

``challenge``
    The challenge, described below. Present exactly when a signed object is
    named.

``objects``
    1 to 16 objects, each naming one of the ``caps.evidence.objects`` of
    ``device-get`` by its ``id`` and giving ``data``, the output blob it is
    written to, sized as described below.

``device-generation``
    Optional, to refuse a device that has changed since.

``selection``
    Optional, with ``mr-selection``: up to 256 pairs of an MR's ``id`` and a
    ``hash-algo``, choosing the MR banks the evidence reports.

``privlevel``
    Optional, with ``privlevel``: the privilege level to report for, as
    configfs-tsm's ``privlevel``.

``service-provider``
    Optional, with ``service``, and only together with ``service-guid`` and
    ``service-manifest-version``: the service to attest, as configfs-tsm's
    ``service_provider``, ``service_guid`` and ``service_manifest_version``.

``options``
    Optional, the device's family's fields, its ``evidence-options``.

The reply carries:

``device-generation``
    The device generation the evidence was produced in.

``results``
    One per object, in request order, each giving the object's format in
    exactly one of ``format`` and ``vendor-format``, its ``length``, the bytes
    written to its blob, and, where the object lists a ``tcb-digest-algo``,
    ``tcb-digest``: the digest the TCB vouches for it with, a ``hash-algo``
    and its ``data``. The blob always holds the object's own bytes.

``mr-generation``
    Where an object binds MRs or the request selected some, the journal
    position the evidence reflects.

``evidence-generation``
    On a device with ``evidence-accept``, the number of this evidence, which
    ``evidence-state-changed`` reports.

``EINVAL``
    A signed object is named without a challenge, a challenge is given
    without one, or its length is one a signed object named does not take.
    Two objects or two selections are the same. Only some of the service
    fields are given.

``EOPNOTSUPP``
    The device does not take the request's selection, privilege level or
    service.

``EMSGSIZE``
    An object's blob is too short for it. The error information's
    ``required-length`` says how long it has to be.

``ENOENT``
    An object's ``id`` is not one ``device-get`` lists. A selection names an
    MR the device lacks, or a bank that MR does not have.

``EPROTO``
    The root of trust returned an object in a format the device does not
    list for it.

``ESTALE``
    ``device-generation`` is not current.

Access is read-write. Availability is dynamic: a device may switch the
operation off for a time, ``op-changed`` reports it, and meanwhile it fails
with ``EOPNOTSUPP``.

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

``instance``
    Where a device has several objects of one kind, which one this is, as it
    numbers SPDM certificate slots.

``tcb-digest-algo``
    Where the TCB vouches for the object with a digest, the digest's
    algorithm.

``evidence-class`` says what an object is:

``signed``
    Evidence the root of trust signs over the challenge. It carries its own
    signature.

``event-log``
    The platform's log of what firmware and the boot chain measured into the
    MRs, for a verifier to replay. Later extends, through ``mr-extend`` or
    otherwise, are not in it.

``cert-chain``
    The certificates that chain a signing key to its vendor's root.

``auxiliary``
    Anything else a verifier reads beside the signed evidence, such as a
    manifest, an SPDM message transcript or a TDISP interface report.

A blob of the object's ``max-length`` always holds it. A shorter one saves
memory where objects run shorter than their bound, as certificate chains do:
where the object does not fit, ``evidence-get`` fails with ``EMSGSIZE`` and
returns no evidence, and ``required-length`` in the error information is the
length to retry with. The retry collects the evidence anew, and may pass the
same challenge.

Two enums name the formats:

``evidence-format``
    ``ctlv-attestation``'s, for a layout a standard defines, such as the
    CCEL or SPDM's, so every family that produces it uses the same value.

``evidence-vendor-format``
    For a layout one vendor defines, whose values the device's family gives
    through a CTLV enum extension point.

The ABI check of ``regen`` covers both enums. An object lists the formats it
may have, from either enum, in ``formats`` and ``vendor-formats``, and the
reply names the one written in exactly one of ``format`` and
``vendor-format``. A new layout is a new value:

======  ============  ==========  =========  ===========================
Family  Format        Class       Challenge  Bytes
======  ============  ==========  =========  ===========================
tdx     quote         signed      64         Intel TDX DCAP Quote
tdx     quote-dice    signed      64         DICE-based TDX Quote
tdx     ccel          event-log   none       ACPI CCEL log area, a TCG
                                             crypto-agile event log
spdm    cert-chain    cert-chain  none       SPDM certificate chain,
                                             instance = slot 0-7
spdm    transcript    auxiliary   none       SPDM message transcript,
                                             starting with VCA
spdm    measurements  signed      32         SPDM GET_MEASUREMENTS
                                             response
spdm    tdisp-report  auxiliary   none       TDISP interface report
tpm     quote         signed      varies     TPM2_Quote's attestation
                                             structure and signature
tpm     event-log     event-log   none       TCG PC Client crypto-agile
                                             event log
tpm     ek-cert       cert-chain  none       EK certificate from NV
armcca  token         signed      64         CCA attestation token,
                                             platform and realm parts
armcca  ccel          event-log   none       ACPI CCEL log area, a TCG
                                             crypto-agile event log
snp     report        signed      64         SNP attestation report
snp     cert-table    cert-chain  none       VCEK or VLEK, ASK and ARK
======  ============  ==========  =========  ===========================

``evidence-format`` prefixes a standard's layouts with its name where the
bare one could be another's: ``ccel``, ``spdm-cert-chain``,
``spdm-transcript``, ``spdm-measurements``, ``tdisp-report``, ``tpm-quote``,
``tpm-event-log`` and ``tpm-ek-cert``. Of the families above only TDX is in
the tree; the rest show where their formats go.

Accepting evidence and device security
======================================

A device with ``evidence-accept`` in ``caps.evidence.flags`` runs once a
verifier accepts its evidence, as a TDISP device interface enters RUN.

``evidence-accept``
-------------------

Tells the provider that a verifier accepted the evidence named by the
request's ``device-generation`` and ``evidence-generation``, which need not be
the latest, and has it run the device. Running is a new device state, so the
device generation advances, retiring all evidence of the previous one. The
reply carries the new ``device-generation``, which ``device-state-changed``
and ``evidence-state-changed`` report too. Nothing is recorded: a provider's
failure leaves the evidence acceptable unless the device changed, which then
advances the device generation as any change does.

``ESTALE``
    The generations do not name evidence that is still acceptable.

Access is read-write. Availability is dynamic, as for ``evidence-get``: the
operation is on only while the device can run, as in TDISP's CONFIG_LOCKED.

``devsec-state-get``
--------------------

Reports the device's security state. It takes no request. The reply carries:

``device-generation``
    The current device generation.

``tdisp-state``
    Where the provider reports it, the TDISP state of the device interface:
    ``unlocked``, ``locked``, ``run`` or ``error``, for TDISP's
    CONFIG_UNLOCKED, CONFIG_LOCKED, RUN and ERROR.

The query's generation changes whenever either does.

Access is read.

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
    ``extend`` where the MR may be extended, and ``removable`` where the
    provider may remove it. A provider still refuses an extend the platform
    forbids at the time, as with ``EPERM`` for a TPM PCR the current locality
    may not extend.

``state-flags``
    ``ima-active`` on the MR IMA extends.

Every MR can be read, though a read can still fail, as when the caller lacks
authorization or the platform's state forbids it.

The name, banks, caps, state flags and register index describe an MR. They
change only when MRs come or go, on a device with ``dynamic-topology``, or when
IMA starts or stops extending one, and either advances the MR generation. The
values change with every extension, so the two are read apart.
``mr-get`` describes every MR, which is how a program learns the IDs, or the
one an ID names. It is answered from what the device keeps, while
``mr-read``, which returns one MR's values, asks the hardware. ``mr-read`` and
``mr-extend`` take only the ID.

A hash algorithm is a constant of ``enum hash_algo`` in
``include/uapi/linux/hash_info.h``, such as ``HASH_ALGO_SHA384``.

Nothing reserves an MR, and the kernel does not arbitrate between its
writers. Any program with a read-write descriptor can extend it, and so can
the register's own interface, as for a TPM's PCRs. A program that replays an
MR from the journal has to make sure, through the system's configuration, that
it is the only writer; the journal records only the extensions made through
the device.

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
request may also carry ``device-generation`` and ``options``, the provider
family's own fields. The reply carries:

``device-generation``
    The current device generation.

``mr-generation``
    The journal position of the values.

``mr-values``
    One for each bank, in ``mr-get``'s order, with its ``hash-algo`` and
    ``data``.

``ENOENT``
    No MR has that ID.

``ESTALE``
    ``device-generation`` is not current.

Access is read.

``mr-extend``
-------------

Extends the MR whose ``id`` the request carries, and appends the extension
to the journal. The request carries one of ``mr-values`` for each bank of the
MR to extend, in any order, each a ``hash-algo`` and a ``data`` of exactly
that algorithm's digest size. The banks left out keep their values. The device
hashes nothing for the caller. Optionally, the request may also carry:

``device-generation``
    To refuse a device that has changed since.

``record``
    What was measured, kept in the journal entry: a ``format`` of printable
    ASCII without spaces and up to 4K of ``data``.

``options``
    The device's family's fields.

The reply carries ``device-generation`` and the ``mr-generation`` of the new
entry, which ``mr-log-entry-appended`` reports.

``ENOENT``
    No MR has that ID.

``EOPNOTSUPP``
    The MR cannot be extended, or the provider cannot extend only some of its
    banks.

``EPERM``
    The platform forbids extending the MR at the time.

``EINVAL``
    The request carries no ``mr-values``, a value is for a bank the MR does not
    have or repeats one, a digest has the wrong length, or the record format
    has a character outside printable ASCII.

``ESTALE``
    ``device-generation`` is not current.

``EBUSY``
    The journal is frozen for a kexec handover.

Access is read-write.

The journal
-----------

The device keeps an append-only journal of its MRs, numbered by
``mr-generation`` from 1. It starts with an ``info`` entry for each MR,
describing it as ``mr-get`` does, and an MR added later gets one when it
appears. Every extension then appends an ``extend`` entry with the digests of
the banks it extended, and with the ``record`` that ``mr-extend`` passed, if
any, or IMA's template for IMA's measurements. Every entry carries the device
generation it was made in. Removing an MR, or IMA starting or ceasing to extend
one, advances the MR generation without adding an entry, so ``mr-get`` shows
what changed. After an orderly kexec, the device continues the previous
kernel's journal.

A bank is replayed from the entries with a digest for it; an entry without one
did not extend that bank. So evidence reporting a bank vouches only for the
entries that extended that bank.

``mr-log-get``
~~~~~~~~~~~~~~

Returns journal entries in order, in fragments. Optionally, the request may
carry:

``selection``
    Up to 256 MR ``id`` values, returning only those MRs' entries.

``mr-start-generation``, ``mr-end-generation``
    The range to return. The start defaults to 1 and the end to the end of the
    journal when the query starts. Later entries are not returned by its
    continuations.

The first fragment carries the ``mr-start-generation`` and
``mr-end-generation`` the query covers. Every fragment carries ``entries``,
each an ``mr-log-entry`` with:

``mr-generation``
    The entry's position in the journal.

``device-generation``
    The device generation the entry was made in.

``mr-id``
    The MR the entry is about.

``event``
    ``info`` or ``extend``.

``mr``
    In an ``info`` entry, the MR's description, with the fields of
    ``mr-desc``.

``mr-values``
    In an ``extend`` entry, the digests the extension folded in.

``record``
    In an ``extend`` entry that has one, the ``record`` ``mr-extend`` passed.

``ima``
    In an ``extend`` entry that has one, IMA's ``template-name``,
    ``template-data`` and ``violation``.

``EINVAL``
    The selection names an MR twice, or the start is past the end.

Access is read.

Events
======

Events are read as :doc:`ctlv` describes. An event that cannot be built shows
as a gap in the sequence, as a dropped one does.

``device-state-changed``
    The device generation advanced, which the event carries in
    ``device-generation``.

``evidence-state-changed``
    Evidence was produced, or a device state change retired it. The event
    carries ``device-generation`` and, once evidence of that device state
    exists, ``evidence-generation``, the latest.

``mr-log-entry-appended``
    An entry was appended to the journal, carried in ``entry`` as
    ``mr-log-get`` returns it.

Providers
=========

What each provider's family adds, and what its device produces:

.. toctree::
   :maxdepth: 1

   attestation-tdx
   attestation-sim
