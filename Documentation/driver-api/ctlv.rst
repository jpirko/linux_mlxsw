.. SPDX-License-Identifier: GPL-2.0

=====================
Writing a CTLV family
=====================

:Author: Jiri Pirko

CTLV is a transport for device control interfaces: a driver describes what its
device accepts, and the framework validates every message against that
description before a callback sees it. The ABI is :doc:`/userspace-api/ctlv`;
the generated reference pages are under :doc:`/ctlv/families/index`.

The code here is a small invented family, ``ctlv-example``.

The shape of the work
=====================

A family is a YAML specification plus the callbacks it declares. Everything in
between is generated:

.. code-block::

    Documentation/ctlv/specs/<family>.yaml     what the device accepts
              |
              |  tools/ctlv/ctlv-regen.sh
              v
    include/uapi/linux/<family>_gen.h          IDs for userspace
    drivers/.../<family>_schema_gen.c          metadata the core validates against
    drivers/.../<family>_family_gen.[ch]       the API your driver implements
    tools/.../<family>_user_gen.[ch]           bindings for userspace
    Documentation/ctlv/families/<family>.rst   the reference page

Nothing generated is edited by hand: ``tools/ctlv/ctlv-regen.sh`` rebuilds it
all, and ``ctlv-regen.sh -c`` fails a build whose checked-in outputs no longer
match the specifications.

The specification
=================

A specification declares a family, what it inherits, its attributes, the
shapes those attributes appear in, and its operations. A minimal one entire:

.. code-block:: yaml

    # SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
    %YAML 1.2
    ---
    family:
      name: ctlv-example
      inherits: ctlv

    attr-groups:
      - name: probe
        attrs:
          - name: token
            type: uint

    attr-sets:
      - name: probe-request
        optional-attrs: probe.token

    ops:
      - name: probe-get
        type: query
        request:
          attrs: probe-request
        reply:
          attrs: probe-request

Every generated file carries the licence of the specification it came from, so
the first line has to state one.

Families are registered in ``Documentation/ctlv/families.yaml``, an append-only
list indexed by family ID, and that ID composes every attribute and operation
ID of the family. An entry's position is its wire identity: never reordered,
removed, renamed or reused, and added by the change that adds the family.

Attributes and groups
---------------------

An attribute is declared once, in a group, and referred to as
``group.attribute`` wherever it is placed. The group number is part of the
complete ID, so a new attribute in an existing group moves nothing else.

The declaration carries the payload form and the rules the framework checks:

.. code-block:: yaml

    attrs:
      - name: u
        type: uint
        min: 1
        max: 4096
      - name: temperature
        type: sint
        unit: celsius
        scale: -3
      - name: label
        type: string
        max-length: 63
      - name: flag
        type: bool
      - name: mode
        enum: echo-mode
      - name: quirks
        flags: echo-quirk
      - name: payload
        type: binary
        max-length: 4096
      - name: count
        type: uint
        max-occurrences: 3
      - name: item
        attrs: example-item
      - name: data
        blob:
          direction: inout
          allowed-backings: [ va, memfd, dma-buf ]
          max-length: 0x10000000
          alignment: 8

Every attribute has exactly one primary form: ``type`` for a scalar, ``enum``
or ``flags`` naming a definition, ``attrs`` naming the set that describes a
nest's members, or ``blob`` for memory the message only describes.
``max-occurrences`` allows an attribute to repeat at every placement it has.

The bounds are ABI. Widening ``max`` lets a device send a value a program was
told could not happen, so a wider range is a new attribute. ``unit`` and
``scale`` change nothing about what is valid, and a caller reads them from
``op-schema-get``.

Attribute sets and nesting
--------------------------

A set names the members of one level. An operation selects a set for its
request and one for its reply; a nested attribute selects one for its own
level:

.. code-block:: yaml

    attr-sets:
      - name: series-request
        attrs: series.chunks
        optional-attrs:
          - series.items
          - series.values
      - name: series-chunk
        attrs: series.index

Members under ``attrs`` are required, members under ``optional-attrs`` are not.
A missing required member fails validation before the callback runs, so a
callback never checks.

A set may re-enter itself, describing unbounded depth finitely. The generated
metadata marks the re-entering node, the framework admits nesting up to
``CTLV_MAX_NEST_DEPTH``, and ``op-schema-get`` reports the re-entry.

Inheritance and extension points
--------------------------------

A family may inherit another, and then its devices answer both families'
operations, each described once by the family that declared it.

A parent that wants a descendant to add to one of its messages declares an
extension point: a set with no members that its own trees select.

.. code-block:: yaml

    # in the parent, which also selects the set somewhere in its own trees
    attr-sets:
      - name: extension-point
        stub: true

    # in the child, which fills it by declaring its members
    attr-sets:
      - name: extension-point
        optional-attrs: child.extra

An unfilled extension point is nothing at all. A filled one lengthens every
message that reaches it, so an inherited operation admits a longer message on a
child's device than on the parent's.

Operations
----------

An operation is an action, a query or an event:

.. code-block:: yaml

    ops:
      - name: noop
        type: action
      - name: echo
        type: action
        request:
          attrs: echo-message
        reply:
          attrs: echo-reply
      - name: series-dump
        type: query
        pre-post: true
        request:
          attrs: series-request
        reply:
          attrs: series-reply
        suggested-reply-length: 4096
      - name: dynamic-noop
        type: action
        availability: dynamic
      - name: pinged
        type: event
        reply:
          attrs: ping-report

An action is executed and acknowledged, and may answer with a bounded reply. A
query is answered in as many fragments as it needs. An event is produced by the
device rather than executed.

``availability: dynamic`` says a device may turn the operation off and on.
``pre-post: true`` asks for a hook pair around this operation alone, on top of
the family-wide pair.

Registering a device
====================

A driver allocates a device, narrows it if it needs to, and registers it:

.. code-block:: c

    struct ctlv_device *ctlvdev;

    ctlvdev = ctlv_device_alloc(parent_dev, &ctlv_example_ctlv_family, priv);
    if (IS_ERR(ctlvdev))
            return PTR_ERR(ctlvdev);

    /* Optional, and only between alloc and register. */
    ctlv_example_dynamic_thing_disable(ctlvdev);
    ctlv_example_legacy_thing_remove(ctlvdev);

    ret = ctlv_device_register(ctlvdev);
    if (ret) {
            ctlv_device_put(ctlvdev);
            return ret;
    }

What a device publishes and the initial state of a dynamic operation are
decided between the two calls. After ``ctlv_device_register()`` the published
set is fixed and only the enabled state still moves.

``ctlv_device_alloc()`` takes the generated descriptor of the family whose
device this is, so a parent driver registering on behalf of a child family
passes the child's. ``ctlv_family_is_a()`` answers whether a descriptor is of,
or descends from, a family::

    if (!ctlv_family_is_a(family, &ctlv_example_ctlv_schema))
            return -EINVAL;

The two are not interchangeable. ``<family>_ctlv_family`` carries the callbacks
and composed metadata of one registerable family, and is what a device is
allocated with. ``<family>_ctlv_schema`` is the family without an
implementation's callbacks, and is what an ancestor is named by.

Unregistering is ``ctlv_device_unregister()`` then ``ctlv_device_put()``, and is
synchronous with respect to callbacks: it waits for the operations already
running and none run afterwards, so a driver may free its private data once
``ctlv_device_put()`` returns. Descriptors userspace holds stay valid and fail
with ``-ENODEV``, and a subscribed reader gets a ``device-removed`` event.

Implementing an operation
=========================

The generated header of the family declares one function per action and query
it must implement, and the generated source binds them to the schema. A driver
implements the prototypes; it never fills in a table.

An action
---------

.. code-block:: c

    int ctlv_example_echo(struct ctlv_op_ctx *ctx,
                          const struct ctlv_cursor *request,
                          struct ctlv_cursor *reply)
    {
            struct example_device *dev = ctlv_op_priv(ctx);
            u64 value = 0;

            if (ctlv_example_echo_u_present(request))
                    value = ctlv_example_echo_u_get(request);

            ctlv_example_echo_u_put(reply, value);
            return 0;
    }

Getters are generated per attribute and per placement, so
``ctlv_example_echo_u_get()`` exists because ``echo.u`` is a member of that
request. ``_present()`` says whether the message carried it; anything the
framework accepted is within its declared bounds, so a callback validates
nothing. An action with no reply returns zero and writes nothing.

A query in fragments
--------------------

A query writes until the serializers stop accepting, then says where to carry
on:

.. code-block:: c

    int ctlv_example_series_dump(struct ctlv_op_ctx *ctx,
                                 const struct ctlv_cursor *request,
                                 struct ctlv_cursor *reply)
    {
            u64 cursor = ctlv_op_query_cursor(ctx);
            u64 generation = ctlv_op_query_generation(ctx);
            struct ctlv_cursor chunk;
            u64 i;

            for (i = cursor; i < dev->n_chunks; i++) {
                    if (!ctlv_example_series_chunk_start(reply, &chunk))
                            return ctlv_op_query_more(ctx, generation, i);
                    ctlv_example_series_index_put(&chunk, i);
                    ctlv_nest_end(&chunk);
            }
            return ctlv_op_query_done(ctx, generation);
    }

The cursor and the generation are both the family's own: any nonzero cursor
that says where to resume, and a generation a continuation must still match or
be refused with ``-ESTALE`` before anything is delivered. The framework keeps
nothing between fragments, so a callback resumes from what it is told and two
programs may run the same query at once.

``ctlv_op_query_more()`` marks the fragment boundary through the nests still
open, so their cursors have to be alive when it is called. A family that opens
a nest in one function and ends the fragment in another keeps the cursors in
the frame that does the ending.

Hooks
-----

Every family implements one pair of hooks around every operation it answers,
and an operation that asked for ``pre-post`` has a second pair inside it. The
order is: the family's pre, the operation's pre, the availability check, the
callback, the operation's post, the family's post.

.. code-block:: c

    int ctlv_example_op_pre(struct ctlv_op_ctx *ctx)
    {
            struct example_device *dev = ctlv_op_priv(ctx);

            return pm_runtime_resume_and_get(dev->dev);
    }

    void ctlv_example_op_post(struct ctlv_op_ctx *ctx)
    {
            struct example_device *dev = ctlv_op_priv(ctx);

            pm_runtime_put(dev->dev);
    }

Each pre that succeeded has exactly one post, so a pre that fails must undo its
own work; its errno becomes the operation's result. A post cannot change what
was answered. Only the registered family's pair runs, so inheriting a schema
does not inherit a hook and a driver's hooks are not entered for the framework's
own operations. Both hooks may sleep and neither may keep ``ctx``.

Availability and removal
------------------------

Two things can narrow what a device publishes, and they are different:

.. code-block:: c

    /* Before register: this device does not have the op at all. */
    ctlv_example_legacy_thing_remove(ctlvdev);

    /* Any time: the device has the op and refuses it for now. */
    ctlv_example_dynamic_thing_disable(ctlvdev);
    ctlv_example_dynamic_thing_enable(ctlvdev);

A removed operation is absent from ``ops-dump``, has no schema, and is
indistinguishable from one no family ever declared. A disabled operation is
listed, reported as disabled, and refused when executed; enabling or disabling
it emits an ``op-changed`` event carrying the new state and a generation.

Only an operation declared ``availability: dynamic`` may be disabled, and only
its own family's helper does it. A callback stays mandatory either way.

Blobs
-----

A blob attribute carries no payload. It describes memory of the caller's, which
the framework acquires only when the callback asks:

.. code-block:: c

    struct ctlv_blob *blob;
    int ret;

    if (!ctlv_example_transfer_data_present(request))
            return 0;

    blob = ctlv_example_transfer_data_resolve(ctx, request);
    if (IS_ERR(blob))
            return PTR_ERR(blob);

    ret = ctlv_blob_read(blob, 0, buf, ctlv_blob_len(blob));
    ctlv_blob_put(blob);
    return ret;

Resolving acquires a pin, a file reference and a mapping that make the memory
the kernel's for the duration of the call, whatever the caller does meanwhile.
Every resolve is released with ``ctlv_blob_put()`` before the callback returns.

Direction is the schema's, not the caller's: ``ctlv_blob_write()`` on a blob
declared ``input`` is a bug that warns. A resolve failing is ordinary, and its
errno is the caller's answer.

Events
------

A family builds an event and emits it:

.. code-block:: c

    struct ctlv_op_ctx *ctx;
    struct ctlv_cursor *event;

    ctx = ctlv_example_thing_happened_create(ctlvdev, &event, GFP_KERNEL);
    if (IS_ERR(ctx))
            return PTR_ERR(ctx);
    ctlv_example_thing_put(event, thing);
    return ctlv_op_emit(ctx);

Exactly one of ``ctlv_op_emit()`` and ``ctlv_op_abort()`` ends the context. The
``gfp`` argument covers the whole event including the copies made for
subscribers, so an emitter in atomic context passes ``GFP_NOWAIT`` or
``GFP_ATOMIC`` and accepts the drops that follow. Neither call may be made from
interrupt context.

Every subscribed file description gets its own copy with its own sequence
number, and one with no room left is told it lost a record rather than made to
wait. Emitting succeeds whether or not anyone was listening, and fails only on
a message the schema does not allow or a device already gone. No hook pair runs
around an event.

Reporting an error
------------------

A callback returns a negative errno, and that is what the ioctl returns. Where
the framework itself refuses a message it also writes error information into
the caller's reply buffer, naming the reason and the offset of the offending
attribute. A driver does not build that; it adds its own errno and its reply.

Configuration
=============

``CONFIG_CTLV``
    The framework. A family driver selects it.
