.. SPDX-License-Identifier: GPL-2.0

==================
Devlink Trap mlxsw
==================

Driver-specific Traps
=====================

.. list-table:: List of Driver-specific Traps Registered by ``mlxsw``
   :widths: 5 5 90

   * - Name
     - Type
     - Description
   * - ``irif_disabled``
     - ``drop``
       - Traps packets that the device decided to drop because they need to be
         routed from disabled router interface (RIF)
         This can happen during RIF dismantle, when the RIF is first disabled
         before being removed completely
   * - ``erif_disabled``
     - ``drop``
       - Traps packets that the device decided to drop because they need to be
         routed through a disabled router interface (RIF)
         This can happen during RIF dismantle, when the RIF is first disabled
         before being removed completely
