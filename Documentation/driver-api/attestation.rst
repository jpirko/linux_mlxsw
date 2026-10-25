.. SPDX-License-Identifier: GPL-2.0-only

===========
Attestation
===========

The attestation subsystem gives each provider device a node of the
``attestation`` class, ``/dev/attestation/attestationN``, which answers as a
descendant of the ``ctlv-attestation`` family, one family per provider.  A
program tells providers apart by the last entry of ``family-get`` on the
open device, and generic tools use the ``ctlv-attestation`` operations on any
of them.  The subsystem owns common evidence retrieval, measurement-register
topology and an append-only extension journal.  Providers retain ownership of
native hardware access and immutable capability descriptions.

Registering a provider
======================

A provider embeds ``struct attestation_dev`` first in its own structure and
allocates both with ``attestation_alloc()``, passing its
``struct attestation_provider`` and its family descriptor, which inherits
``ctlv-attestation``:

.. code-block:: c

    struct foo {
            struct attestation_dev att;
            struct foo_hw *hw;
    };

    foo = attestation_alloc(parent, &foo_provider,
                            &ctlv_attestation_foo_family, struct foo, att);
    if (IS_ERR(foo))
            return PTR_ERR(foo);
    foo->hw = hw;
    ret = attestation_register(&foo->att);
    if (ret) {
            attestation_put(&foo->att);
            return ret;
    }

A provider callback gets the ``struct attestation_dev`` and reaches the
provider's structure with ``container_of()``.  A CTLV callback of the
provider's own family gets the CTLV instance, which
``attestation_from_ctlv()`` turns into the device.  No provider callback runs
before ``attestation_register()``, so the provider finishes its own setup in
between.  The descriptor, evidence capabilities, fixed MR properties, strings
and arrays remain immutable until the last ``attestation_put()``.

The subsystem embeds the CTLV instance, and its hooks run every operation,
those of the device's family included, under one instance mutex.  That
serializes an operation, its native callback, journal commit and event
emission.

Every MR can be read, so a provider with MRs implements ``mr_read``.
Fixed MRs are listed in the provider descriptor.  Their property storage
remains valid until the last ``attestation_put()``.

The evidence capabilities list the objects ``evidence-get`` produces, each
object's ID being its index.  An object gives its class and the formats it
may have, ``ctlv-attestation``'s ``evidence-format`` or its family's
``evidence-vendor-format``, and a signed object its challenge lengths.
``attestation_alloc()`` refuses a format the family does not give.

``evidence_get`` writes each object with ``attestation_evidence_write()``,
which fails the operation with ``-EMSGSIZE`` where the caller's blob is too
short, and tells the caller how long it has to be.  It fills in each object's
result: the format written and its length.  A format the object does not list
fails the call with ``-EPROTO``.

Other writers
=============

This interface is not the only way to extend a measurement register, and it
does not arbitrate between the ways there are.  Every program holding a
read-write descriptor of the device can extend a register through
``mr-extend``, and a register with a native interface of its own can be
extended through that too, as a TPM's PCRs are by programs using
``/dev/tpm0`` or ``/dev/tpmrm0`` and by IMA.  Nothing reserves a register for
one writer.  A register two writers extend holds the extensions of both, in
the order they arrived, and ``mr-read`` and the evidence report it as it is.
A program that replays a register from its own records has to be its only
writer, by agreement with whatever else runs on the system.

MR journal
==========

MR extension is transactional.  The core allocates the complete journal entry
and event before invoking ``mr_extend``.  A successful callback is followed
only by infallible publication, so userspace observes the same ordered
journal.

``MR_READ`` uses ``mr_read_generation`` only to validate query continuation;
the returned ``mr-generation`` is the journal boundary at which the values
were read.

Teardown
========

``attestation_unregister()`` first removes the node, so nothing opens the
device again.  It then withdraws the device from the kernel registry, refusing
the operations of files still open.  Only then does it unpublish the CTLV
instance, which drains the operations and events in flight and queues
``DEVICE_REMOVED``.  No provider callback runs once it returns, so the
provider may release its native state before ``attestation_put()``.  Open
files keep the structure until they close, and its release frees the journal.

Kernel API
==========

.. kernel-doc:: include/linux/attestation.h
.. kernel-doc:: drivers/attestation/core/core.c
   :export:
