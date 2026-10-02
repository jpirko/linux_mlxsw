.. SPDX-License-Identifier: GPL-2.0

=====================
Simulated attestation
=====================

``CONFIG_ATTESTATION_SIM`` provides attestation devices of the
``ctlv-attestation-sim`` family, whose specification is
``Documentation/ctlv/specs/ctlv-attestation-sim.yaml``, for tests. Their
evidence is synthetic and their MRs are kept in RAM, so they are no security
boundary. Each device belongs to a fixture, and its ``device-info``, the
``info`` of ``device-get``, carries the fixture's ``name``.

Measurement registers
=====================

==  ==============  ==============  ================  ======
ID  Name            Register index  Banks             Extend
==  ==============  ==============  ================  ======
0   sim-read-only   0               SHA-384           no
1   sim-extendable  1               SHA-256, SHA-384  yes
==  ==============  ==============  ================  ======

The device has ``dynamic-topology``: the fixture's ``mr-add`` adds an
extendable, removable ``sim-dynamic-N`` of ID and register index N, and
``mr-remove`` removes it. Every MR starts at zero.

Evidence
========

==  ========  =========  ========  =================  =======
ID  Object    Class      Formats   Challenge          Binds
==  ========  =========  ========  =================  =======
0   report    signed     report    1-64 or 128 bytes  the MRs
1   token     signed     token     32 bytes           no MR
2   log       event-log  ccel      none               no MR
3   manifest  auxiliary  manifest  none               no MR
==  ========  =========  ========  =================  =======

``report``, ``token`` and ``manifest`` are ``ctlv-attestation-sim``'s vendor
formats. The report holds the challenge, the request's privilege level and
service, and the values of the banks the request selects, or of every bank of
every MR. Its ``tcb-digest`` is its SHA-384. The token holds the challenge
alone, and a request for both passes 32 bytes, the one length both take. The
log is a ``ccel`` log of the Spec ID event alone, and the manifest, instance 1,
names the fixture.

``devsec-state-get`` reports the TDISP state the fixture's ``state`` file
sets. ``evidence-accept`` is on while that state is ``locked``, and moves it to
``run``.

Fixtures
========

Root creates a fixture by writing its name, up to 63 bytes without a slash, to
``/sys/kernel/debug/attestation-sim/create``, and destroys it by writing the
name to ``destroy``. Each fixture's device has a root device of its own,
``attestation-sim-<name>``, as its parent, and the fixture has a directory of
its name with these files:

``state``
    The TDISP state: ``UNLOCKED``, ``LOCKED``, ``RUN`` or ``ERROR``. Writing
    one changes the device's state, which advances its generation.

``mr-add``
    An ID from 2 and the hash algorithms of its banks, as
    ``2 sha256 sha384``, add ``sim-dynamic-2``. A fixture has up to 16 such
    MRs.

``mr-remove``
    The ID of an MR ``mr-add`` added removes it.

``fail-next``
    A callback and an errno, as ``mr_read EIO``, fail the callback's next
    call. The callbacks are ``evidence_get``, ``evidence_accept``,
    ``mr_read``, ``mr_extend`` and ``tdisp_state_get``, and the errnos
    ``EIO``, ``EAGAIN``, ``ENOMEM`` and ``EBUSY``.

``pause-next``
    A callback holds its next call until ``1`` is written to ``resume``, for
    a test to race other operations against it.

``status``
    The paused callback, or ``none``, and how many times each callback ran.

Destroying a fixture releases a paused call, which fails with ``ENODEV``.

With the simulator built in, ``attestation_sim.early_fixture=<name>`` creates
a fixture at boot, early enough for IMA to select its device, which one
created through debugfs is not.
