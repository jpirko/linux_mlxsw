.. SPDX-License-Identifier: GPL-2.0-only

===========
Attestation
===========

The attestation subsystem publishes provider devices as descendants of the
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

The existing IMA binary and ASCII runtime-measurement files project that MR
lifetime from the attestation journal.  IMA entries retain their original
template records; userspace extensions appear as ``ima-mr-extend`` records
carrying the exact submitted value and optional canonical record envelope.
Each open file owns a log-only target clone and traverses fixed journal ends in
bounded SRCU batches.  Targets without SHA-1 do not create the unsuffixed
SHA-1 aliases.

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

Attestation-backed IMA adds a small handover record containing the exact parsed
boot selector, target lifetime, captured journal end and violation count.  It
is published after IMA measurement suspension and the core journal capture.
The next kernel requires the same selector and restored lifetime, rebuilds
deduplication from committed IMA journal entries, and never imports the legacy
IMA measurement-list buffer for this mode.

Simulation provider
===================

``CONFIG_ATTESTATION_SIM`` provides synthetic evidence and RAM-backed MRs for
testing.  It is not a hardware security boundary.  Root may create and destroy
named fixtures through ``/sys/kernel/debug/attestation-sim``.  Each fixture has
one parent device and one ``ctlv-attestation-sim`` instance.

The ``state``, ``mr-add`` and ``mr-remove`` files model native changes through
the normal provider API.  ``fail-next`` injects one provider errno.
``pause-next`` and ``resume`` provide deterministic operation races, while
``status`` reports callback counts and the paused callback.  Removing a fixture
cancels pauses before calling ``ctlv_destroy()``.

When the provider is built in, ``attestation_sim.early_fixture=<name>`` creates
one fixture at device-init time.  This opt-in path exists so later IMA tests can
select a provider before IMA's late initialization; post-boot debugfs creation
is too late for that boot target.

The IMA selftest boots with
``attestation_sim.early_fixture=ima-test
ima_attestation=ctlv-attestation-sim:1``.  It checks target ownership,
kernel-originated journal metadata, both algorithm-specific securityfs views,
single boot-aggregate projection, policy target assertions and interleaved
userspace extensions.  A different fixture name may be supplied through
``IMA_ATTESTATION_TEST_FIXTURE``.

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
