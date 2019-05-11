.. SPDX-License-Identifier: GPL-2.0

======================
Devlink Trap netdevsim
======================

Driver-specific Traps
=====================

.. list-table:: List of Driver-specific Traps Registered by ``netdevsim``
   :widths: 5 5 90

   * - Name
     - Type
     - Description
   * - ``fid_miss_exception``
     - ``exception``
     - When a packet enters the device it is classified to a filtering
       indentifier (FID) based on the ingress port and VLAN. This trap is used
       to trap packets for which a FID could not be found

Trap Groups
===========

.. list-table:: List of Trap Groups Registered by ``netdevsim``
   :widths: 10 90

   * - Name
     - Description
   * - ``l2_drops``
     - Contains packet traps for packets that were dropped by the device during
       layer 2 forwarding (i.e., bridge)
   * - ``l3_drops``
     - Contains packet traps for packets that were dropped by the device or hit
       an exception (e.g., TTL error) during layer 3 forwarding
   * - ``buffer_drops``
     - Contains packet traps for packets that were dropped by the device due to
       an enqueue decision
