.. SPDX-License-Identifier: GPL-2.0-only

===========
Attestation
===========

The attestation subsystem publishes provider devices as descendants of the
``ctlv-attestation`` family.  It owns common evidence retrieval,
measurement-register topology and an append-only extension journal.
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

Fixed MRs are listed in the provider descriptor.  Their property storage
remains valid until ``ctlv_destroy()`` or failed-creation unwind completes.

MR journal
==========

MR extension is transactional.  The core allocates the complete journal entry
and event before invoking ``mr_extend``.  A successful callback is followed
only by infallible publication, so userspace observes the same ordered
journal.

Teardown
========

CTLV first drains userspace operations.  The base ``pre_fini`` callback then
withdraws the instance from the kernel registry while provider state and CTLV
event emission remain available.  CTLV stops events only after that callback
returns; provider and base ``fini`` callbacks may then release native storage.
