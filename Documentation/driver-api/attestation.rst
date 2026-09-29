.. SPDX-License-Identifier: GPL-2.0-only

=====================
Attestation framework
=====================

The attestation framework publishes provider devices as descendants of the
``ctlv-attestation`` family.  It owns common evidence generations, userspace
verdicts, measurement-register topology and an append-only extension journal.
Providers retain ownership of native hardware access and immutable capability
descriptions.

Registering a provider
======================

A provider defines ``struct attestation_provider`` and passes it to the base
family through its generated ``pre_init`` callback.  The descriptor, evidence
capabilities, fixed MR properties, strings and arrays remain immutable until
``ctlv_destroy()`` or failed-creation unwind completes.

The provider family creates the CTLV device with its own generated family
descriptor.  Its family-wide operation hooks delegate once to
``ctlv_attestation_op_pre()`` and ``ctlv_attestation_op_post()`` using the
attestation parent priv.  The hooks serialize an operation, its native
callback, journal commit and event emission with one instance mutex.

Fixed MRs are listed in the provider descriptor.  A provider advertising
dynamic topology retains handles returned by ``attestation_mr_create()`` and
passes each handle exactly once to ``attestation_mr_destroy()``.  Property
storage remains valid for the handle's lifetime.

State changes
=============

``attestation_state_changed()`` is called after releasing conflicting native
locks.  It advances the device generation, retires evidence created for the
previous state, clears a recorded verdict and emits state events.  The caller
keeps the provider instance alive and serializes notifications with native
state changes.

MR extension is transactional.  The core allocates the complete journal entry
and event before invoking ``mr_extend``.  A successful callback is followed
only by infallible publication, so userspace and kernel consumers observe the
same ordered journal.

Kernel consumers
================

The interfaces in ``<linux/attestation/ima.h>`` select one live extendable MR
by family lineage and/or parent device.  A routing target owns the
``IMA_ACTIVE`` state; log-only clones retain only copied identity fields.
Each operation enters the shared registry SRCU domain, resolves the original
instance and MR lifetime, and then takes the instance mutex.

Journal entries are immutable.  A reader captures an end generation, enters
SRCU for one bounded batch, seeks by generation and follows the append-only
``next`` chain.  No borrowed entry or payload pointer may cross the SRCU
section.

Teardown
========

CTLV first drains userspace operations.  The base ``pre_fini`` callback then
withdraws the instance from the kernel registry and waits for registry SRCU
readers while provider state and CTLV event emission remain available.  CTLV
stops events only after that callback returns; provider and base ``fini``
callbacks may then release native and journal storage.

Orderly kexec
=============

The core handover serializer captures typed journal entries, live MR
properties and authoritative terminal values.  The image-transport integration
supplies that byte stream to the next kernel.  Restoration matches the exact
provider family and backing-device tuple, validates topology and native values,
then reconstructs the journal without re-extending any MR or duplicating INFO
entries.
