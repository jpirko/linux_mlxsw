.. SPDX-License-Identifier: GPL-2.0

==================
Devlink Trap mlxsw
==================

Trap Groups
===========

.. list-table:: List of Trap Groups Registered by ``mlxsw``
   :widths: 10 90

   * - Name
     - Description
   * - ``l2_drops``
     - Contains packet traps for packets that were dropped by the device during
       layer 2 forwarding (i.e., bridge)
