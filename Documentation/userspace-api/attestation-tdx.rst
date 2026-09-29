.. SPDX-License-Identifier: GPL-2.0

===============
TDX attestation
===============

With ``CONFIG_ATTESTATION``, the TDX guest driver registers an attestation
device of the ``ctlv-attestation-tdx`` family, whose specification is
``Documentation/ctlv/specs/ctlv-attestation-tdx.yaml``. Its ``device-info``,
the ``info`` of ``device-get``, carries ``mrconfigid``, ``mrowner`` and
``mrownerconfig``, 48 bytes each.

Measurement registers
=====================

==  =====  ==============  =======  ======
ID  Name   Register index  Bank     Extend
==  =====  ==============  =======  ======
0   mrtd   none            SHA-384  no
1   rtmr0  0               SHA-384  yes
2   rtmr1  1               SHA-384  yes
3   rtmr2  2               SHA-384  yes
4   rtmr3  3               SHA-384  yes
==  =====  ==============  =======  ======

Each ``mr-read`` takes a TDREPORT.

The TSM-MR files,
``/sys/devices/virtual/misc/tdx_guest/measurements/rtmr[0123]:sha384``, extend
the same RTMRs as ``mr-extend``. Firmware and the boot loader usually extend
RTMR0 to RTMR2 before the kernel starts, which leaves RTMR3 to the running
system.

Evidence
========

==  ========  =========  =================  =========  ========
ID  Object    Class      Formats            Challenge  Binds
==  ========  =========  =================  =========  ========
0   Quote     signed     quote, quote-dice  64 bytes   every MR
1   CCEL log  event-log  ccel               none       no MR
==  ========  =========  =================  =========  ========

The challenge goes into the REPORTDATA the Quote carries. The host's quoting
service returns a DCAP Quote or a DICE-based one, and the device names it
``quote`` by its header's version and TEE type, or ``quote-dice`` by the CBOR
magic it starts with. Both are ``ctlv-attestation-tdx``'s vendor formats. A
Quote of neither fails ``evidence-get`` with ``EPROTO``.

The log is there where the firmware describes its CC event log with an ACPI
CCEL table, and is the whole log area. It does not change after boot, and one
call returns it with the Quote.

The TDREPORT is MAC'd for the platform, not signed, so it is no object;
``TDX_CMD_GET_REPORT0`` of ``/dev/tdx_guest`` returns it.

A guest has one TDX device, which a udev rule can give a stable name::

    ENV{CTLV_FAMILY}=="ctlv-attestation-tdx", SYMLINK+="attestation/tdx"
