.. SPDX-License-Identifier: GPL-2.0

=====================
Writing a CTLV family
=====================

:Author: Jiri Pirko

A CTLV family is a YAML specification plus the callbacks named by its generated
header. CTLV validates requests before a callback sees them. The wire ABI is
:doc:`/userspace-api/ctlv`; family specifications are under
``Documentation/ctlv/specs``.

Generate the family
===================

Add the specification, registry entry, and output manifest entry in one change::

    Documentation/ctlv/specs/ctlv-example.yaml
    Documentation/ctlv/families.yaml
    Documentation/ctlv/generated-files.yaml

``families.yaml`` is append-only. Its array index is the family ID and is part
of every generated attribute and operation ID. Never reorder, remove, rename,
or reuse an entry. ``Documentation/ctlv/abi/families.registry.yaml`` is the
checked-in registry ABI. The output manifest selects the generated UAPI,
kernel and userspace paths and generation modes.

Generate and verify them with::

    make -C tools/ctlv regen
    make -C tools/ctlv all

Never edit a generated output. Change the YAML or generator and regenerate it.
Optional RST reference pages are written to
``Documentation/output/ctlv/families`` by::

    make -C tools/ctlv docs

Write the specification
=======================

.. code-block:: yaml

    # SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
    %YAML 1.2
    ---
    family:
      name: ctlv-example
      inherits: ctlv
      post-init: true
      pre-fini: true

    attr-groups:
      - name: item
        attrs:
          - name: index
            type: uint
            max: 4095
          - name: label
            type: string
            max-length: 63
          - name: data
            blob:
              direction: input
              allowed-backings: [ va, memfd, dma-buf ]
              max-length: 1048576
              alignment: 8
      - name: list
        attrs:
          - name: item
            attrs: item-reply
            max-occurrences: unlimited

    attr-sets:
      - name: item-request
        attrs: item.index
        optional-attrs:
          - item.label
          - item.data
      - name: item-reply
        attrs: item.index
        optional-attrs: item.label
      - name: items-reply
        optional-attrs: list.item

    ops:
      - name: item-set
        type: action
        optional: true
        request:
          attrs: item-request
      - name: items-dump
        type: query
        availability: dynamic
        pre-post: true
        reply:
          attrs: items-reply
        suggested-reply-length: 4096
      - name: item-changed
        type: event
        reply:
          attrs: item-reply

An attribute has one payload form: ``type``, ``enum``, ``flags``, ``attrs``, or
``blob``. An attr set names one message level. ``attrs`` are required and
``optional-attrs`` are optional. Its bounds and membership are ABI.

Inheritance and extension points
================================

A descendant implements only the operations it declares. Inherited operations
keep the callbacks and operation hooks of their declaring family. Every module
therefore depends on the module containing its parent implementation.

A parent reserves an extension point with an empty stub set:

.. code-block:: yaml

    attr-groups:
      - name: device
        attrs:
          - name: details
            attrs: details-extension

    attr-sets:
      - name: details-extension
        stub: true
      - name: device-reply
        optional-attrs: device.details

A child fills that same set:

.. code-block:: yaml

    family:
      name: ctlv-example-vendor
      inherits: ctlv-example
      pre-init: true

    attr-groups:
      - name: vendor
        attrs:
          - name: serial
            type: uint

    attr-sets:
      - name: details-extension
        optional-attrs: vendor.serial

The parent implementation opens the physical nest and delegates its members:

.. code-block:: c

    if (ctlv_example_device_details_start(reply, &details)) {
            ret = ctlv_stub_fill(ctx, request_details, &details);
            ctlv_nest_end(&details);
            if (ret)
                    return ret;
    }

The generated child callback fills it:

.. code-block:: c

    int ctlv_example_vendor_details_extension_fill(
                            void *priv, struct ctlv_op_ctx *ctx,
                            const struct ctlv_cursor *request,
                            struct ctlv_cursor *nest)
    {
            ctlv_example_vendor_vendor_serial_put(nest,
                                                   example_serial(priv));
            return 0;
    }

``ctlv_stub_read()`` delegates members received in the other direction. An
unfilled stub contributes no nest or message length. Neither side names the
family on the other side.

Create and destroy a device
===========================

``ctlv_create()`` takes the leaf family descriptor and leaf initialization
information:

.. code-block:: c

    struct ctlv_example_vendor_init_info info = {
            .dev = dev,
    };
    struct ctlv_device *ctlvdev;

    ctlvdev = ctlv_create(dev, &ctlv_example_vendor_ctlv_family, &info);
    if (IS_ERR(ctlvdev))
            return PTR_ERR(ctlvdev);

A family with ``pre-init: true`` supplies initialization information to its
direct parent. A typical provider descriptor is static and immutable:

.. code-block:: c

    static const struct ctlv_example_provider example_provider = {
            .name = "vendor-example",
            .ops = &example_provider_ops,
    };

    const void *ctlv_example_vendor_pre_init(const void *init_info)
    {
            return &example_provider;
    }

The leaf's per-device ``init_info`` still reaches its own ``init`` callback; it
is not forwarded as the parent's descriptor. ``pre_init`` runs leaf to root and
may return an ``ERR_PTR()``. It must not acquire resources or return automatic
storage.

Each family supplies ``priv_size``, ``init``, and ``fini``. The YAML flags add
``pre_init``, ``post_init``, and ``pre_fini`` callbacks. Creation order is::

    leaf -> root     pre_init
    root -> leaf     init
                     freeze published operations; start event admission
    leaf -> root     post_init
                     register the character device

Select optional operations and their initial dynamic state in ``init``. A
``post_init`` callback may publish the device to kernel consumers because all
family privs and event emission are then available.

Normal destruction is ordered as follows::

    withdraw userspace access; drain admitted operation callbacks
    root -> leaf     pre_fini
    stop and drain event contexts; queue DEVICE_REMOVED
    leaf -> root     fini

A family published in ``post_init`` withdraws and drains its consumers in
``pre_fini``. Provider state and events remain available until all
``pre_fini`` callbacks finish. ``ctlv_destroy()`` consumes the reference from
``ctlv_create()``; open file descriptions remain only to drain queued records
and otherwise fail with ``-ENODEV``.

If ``init`` fails, that callback releases its partial resources and CTLV calls
``fini`` only for initialized ancestors. A failure after initialization uses
the normal ``pre_fini`` and ``fini`` ordering.

CTLV allocates one zeroed, cache-line-aligned priv per family. Direct neighbors
are available without naming their types globally:

.. code-block:: c

    parent = ctlv_priv_parent(priv);
    child = ctlv_priv_child(priv);

During ``init``, the child is not initialized yet. During ``pre_fini`` it still
is; during ``fini`` it has already been finalized. Allocate a separately
aligned object when cache-line alignment is insufficient.

Implement operations
====================

Action
------

Generated getters read only validated placements and generated serializers
write only declared replies:

.. code-block:: c

    int ctlv_example_item_set_action(void *priv, struct ctlv_op_ctx *ctx,
                                     const struct ctlv_cursor *request,
                                     struct ctlv_cursor *reply)
    {
            u64 index = ctlv_example_item_index_get(request);

            if (item_busy(priv, index))
                    return ctlv_op_error(ctx, -EBUSY, "item is busy");
            return item_set(priv, index);
    }

Use ``_present()`` for optional attributes and ``_for_each()`` for repeated
ones. ``ctlv_op_error()`` preserves the errno and adds a short diagnostic when
the caller supplied enough reply space.

Query
-----

.. code-block:: c

    int ctlv_example_items_dump_query(void *priv, struct ctlv_op_ctx *ctx,
                                      const struct ctlv_cursor *request,
                                      struct ctlv_cursor *reply)
    {
            struct ctlv_example *example = priv;
            u64 generation = READ_ONCE(example->generation);
            u64 cursor = ctlv_op_query_cursor(ctx);
            struct ctlv_cursor item;
            u64 i = cursor ? cursor - 1 : 0;

            if (cursor && ctlv_op_query_generation(ctx) != generation)
                    return -ESTALE;
            for (; i < example->items_count; i++) {
                    if (!ctlv_example_list_item_start(reply, &item))
                            return ctlv_op_query_more(ctx, generation, i + 1);
                    ctlv_example_item_index_put(&item, i);
                    ctlv_nest_end(&item);
            }
            return ctlv_op_query_done(ctx, generation);
    }

A continuation cursor is nonzero and advances. Every continuing fragment
completes a logical leaf. Call ``ctlv_op_query_more()`` while cursors for open
continuation nests are alive. A generation mismatch returns ``-ESTALE``.

Hooks and availability
======================

The leaf family's hooks wrap each effective non-core action or query. An
operation with ``pre-post: true`` has its declaring family's pair inside them::

    family pre -> operation pre -> availability check -> callback
               -> operation post -> family post

Each successful pre hook has one post hook. A failing pre hook undoes its own
work. Hooks may sleep and must not retain ``ctx``. Each hook and callback gets
the priv of the family implementing it.

Optional and dynamic operations are separate choices:

.. code-block:: c

    /* During init: publish this optional operation on this device. */
    ctlv_example_item_set_provide(ctlvdev);

    /* At runtime: temporarily refuse a published dynamic operation. */
    ctlv_example_items_dump_disable(ctlvdev);
    ctlv_example_items_dump_enable(ctlvdev);

Declare the latter with ``availability: dynamic``. An unpublished optional
operation has no schema entry. A disabled operation remains discoverable and
returns ``-EOPNOTSUPP``; a state change emits ``op-changed``.

Blobs
=====

A blob attribute describes caller-owned memory. Resolve it only in the callback
that received its request and release every successful resolve before return:

.. code-block:: c

    blob = ctlv_example_item_data_resolve(ctx, request);
    if (IS_ERR(blob))
            return PTR_ERR(blob);

    ret = ctlv_blob_read(blob, 0, buf, ctlv_blob_len(blob));
    put_ret = ctlv_blob_put(blob);
    return ret ?: put_ret;

The schema fixes direction, backing, length, and alignment. Resolution holds
the backing for the ioctl; CPU mapping is lazy.

For device DMA, ``ctlv_blob_dma_map()`` accepts dma-buf backing, returns a
borrowed whole-buffer scatterlist and the blob offset in its DMA-length stream.
Call ``ctlv_blob_dma_begin()``, submit the fenced work,
``ctlv_blob_dma_submit_done()``, then ``ctlv_blob_dma_unmap()`` after hardware
stops. An operation holds at most one device mapping.

Events
======

.. code-block:: c

    ctx = ctlv_example_item_changed_create(ctlvdev, &event, GFP_KERNEL);
    if (IS_ERR(ctx))
            return PTR_ERR(ctx);
    ctlv_example_item_index_put(event, index);
    return ctlv_event_emit(ctx);

Exactly one of ``ctlv_event_emit()`` and ``ctlv_event_abort()`` ends the context. The
GFP argument covers subscriber copies; atomic producers use ``GFP_NOWAIT`` or
``GFP_ATOMIC``. Event APIs are not called from interrupt context, and hooks do
not run around events.

If an event occurred but allocation failed, commit the state change and call
``ctlv_event_lost()``. It neither allocates nor sleeps. Do not use it for an
aborted event or for per-subscriber drops already tracked by
``ctlv_event_emit()``.

Tests and examples
==================

``Documentation/ctlv/specs/ctlv-test.yaml``
    Attribute forms, nesting, actions, queries, events, blobs, availability,
    hooks, and an extension point.

Configuration
=============

``CONFIG_CTLV``
    CTLV. A family implementation selects it.
