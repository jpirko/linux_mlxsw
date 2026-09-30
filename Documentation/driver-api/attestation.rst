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

``MR_READ`` uses ``mr_read_generation`` only to validate query continuation;
the returned ``mr.generation`` is the journal boundary at which the values
were read.

With ``CONFIG_IMA_ATTESTATION``, IMA hashes each completed template record
with every algorithm advertised by the selected MR.  One core transaction
extends those values and records the template fields and violation state.
Successful measurements are not also added to IMA's legacy list or extended
through its TPM path.

Teardown
========

CTLV first drains userspace operations.  The base ``pre_fini`` callback then
withdraws the instance from the kernel registry and waits for registry SRCU
readers while provider state and CTLV event emission remain available.  CTLV
stops events only after that callback returns; provider and base ``fini``
callbacks may then release native and journal storage.

TDX provider
============

``CONFIG_ATTESTATION_TDX`` registers a ``ctlv-attestation-tdx`` instance for
the native TDX guest service.  MRTD has stable MR ID 0 and is read-only.
RTMR0 through RTMR3 have IDs 1 through 4 and are readable and extendable with
SHA-384.

``EVIDENCE_GET`` supports complete raw TDREPORT and Quote objects.  Both
formats cryptographically bind all architectural TDX measurement registers,
so successful replies carry the matching MR-journal boundary.  The provider
exposes no verdict or device-security operation.

The provider consumes only ``<linux/tdx-guest.h>``.  It does not call the TDX
misc ioctl, configfs TSM-report, or TSM-MR sysfs frontends.
