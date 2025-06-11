.. SPDX-License-Identifier: GPL-2.0

============================
Devlink Shared Instances
============================

Overview
========

Shared devlink instances allow multiple physical functions (PFs) on the same
chip to share an additional devlink instance for chip-wide operations. This
is implemented alongside the individual PF devlink instances, not replacing them.

The shared devlink instance is backed by a faux device and provides a common
interface for operations that affect the entire chip rather than individual PFs.

Implementation
==============

Architecture
------------

The implementation uses:

* **Faux device**: Virtual device backing the shared devlink instance
* **Chip identification**: PFs are grouped by chip using a driver-specific identifier
* **Shared instance management**: Global list of shared instances with reference counting

Key structures include a shared devlink structure containing a global list node,
chip identifier, list of PFs using the instance, and backing faux device.

Initialization Flow
-------------------

1. **PF calls shared devlink init** during driver probe
2. **Chip identification** using driver-specific method to determine device identity
3. **Lookup existing shared instance** for this chip identifier
4. **Create new shared instance** if none exists:

   * Create faux device with chip identifier as name
   * Allocate and register devlink instance
   * Add to global shared instances list

5. **Add PF to shared instance** PF list
6. **Cleanup** when PF is removed; destroy shared instance when last PF is removed

Chip Identification
-------------------

PFs belonging to the same chip are identified using a driver-specific method.
The driver is free to choose any identifier that is suitable for determining
whether two PFs are part of the same device. Examples include VPD serial numbers,
device tree properties, or other hardware-specific identifiers.

Locking
-------

A global mutex protects the shared instances list and individual shared
instance PF lists during registration/deregistration.
