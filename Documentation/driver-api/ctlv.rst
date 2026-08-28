.. SPDX-License-Identifier: GPL-2.0

=====================
Writing a CTLV family
=====================

:Author: Jiri Pirko

CTLV is a transport for device control interfaces: a driver describes what its
device accepts, and the framework validates every message against that
description before a callback sees it. The ABI is :doc:`/userspace-api/ctlv`;
the generated reference pages are under :doc:`/ctlv/families/index`.

The code here is a small invented family, ``ctlv-example``. The complete
example in the tree is ``ctlv-test``, described at the end.

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
    drivers/.../<family>_gen.[ch]              metadata and the driver API
    tools/.../<family>_user_gen.[ch]           bindings for userspace
    Documentation/ctlv/families/<family>.rst   the reference page

Nothing generated is edited by hand: ``tools/ctlv/ctlv-regen.sh`` rebuilds it
all, and ``ctlv-regen.sh -c`` fails a build whose checked-in outputs no longer
match the specifications. ``Documentation/ctlv/generated-files.yaml`` is the
complete output manifest, including each file's generation mode.

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
      pre-init: true
      post-init: true
      pre-fini: true

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
``Documentation/ctlv/abi/families.registry.yaml`` records the checked-in
ordered ABI baseline.

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

A caller may mark an attribute of its own request as one to skip if this kernel
declares no such ID, which is how a program built against a later version of
this family reaches an earlier one. A skipped attribute answers to nothing the
schema declares, so no getter can reach it and a callback sees a request
without it.

A set may re-enter itself, describing unbounded depth finitely. The generated
metadata marks the re-entering node, the framework admits nesting up to
``CTLV_MAX_NEST_DEPTH``, and ``op-schema-get`` reports the re-entry.

Inheritance and extension points
--------------------------------

A family may inherit another, and then its devices answer both families'
operations, each described once by the family that declared it and answered once
by the driver that wrote it. A driver of an inheriting family implements the
operations its own specification declares and nothing else: the rest are entered
in the module of the family that declared them, which is a module the inheriting
one depends on.

Every family has a core-allocated priv and the same generated lifecycle,
described under `Family priv`_. A device therefore carries one priv for every
family in its chain, including the root.

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
child's device than on the parent's. What goes into one and what comes out of it
are the filling family's, described under `Extension points`_.

Operations
----------

An operation is an action, a query or an event:

.. code-block:: yaml

    ops:
      - name: noop
        type: action
        fd-access: read
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
        optional: true
      - name: pinged
        type: event
        reply:
          attrs: ping-report

An action is executed and acknowledged, and may answer with a bounded reply. A
query is answered in as many fragments as it needs. An event is produced by the
device rather than executed.

An action or query defaults to ``fd-access: read-write``, restricting it to a
full-control ``O_RDWR`` fd. Declare ``fd-access: read`` only when the op is safe
for an ``O_RDONLY`` observation fd. This classification is independent of op
type: a read-access action may update per-open observation state, while a query
which exposes control-sensitive information may retain the read-write default.
Events cannot declare fd access and are available through either accepted fd.

``availability: dynamic`` says a device may turn the operation off and on.
``optional: true`` says family initialization must opt in before publication.
``pre-post: true`` asks for a hook pair around this operation alone, on top of
the device family's family-wide pair. An inherited op keeps its declaring
family's callback and op-specific hooks. Core ops use the core family pair.

Creating a device
=================

``ctlv_create()`` uses family-specific initialization information, runs every
family lifecycle callback, and publishes the completed device:

.. code-block:: c

    const struct ctlv_example_init_info init_info = {
            .parent_init_info = {
                    /* Parent-family fields. */
            },
            /* Family-specific fields. */
    };
    struct ctlv_device *ctlvdev;

    ctlvdev = ctlv_create(parent_dev, &ctlv_example_ctlv_family,
                          &init_info);
    if (IS_ERR(ctlvdev))
            return PTR_ERR(ctlvdev);

No userspace handle escapes before the device is complete. Family ``init``
callbacks select optional ops and initial dynamic availability. CTLV then
freezes the operation set and starts event admission before ``post_init``.
A ``post_init`` callback may expose the provider to kernel consumers; userspace
registration follows only after every callback succeeds. After
``ctlv_create()`` returns, only runtime dynamic availability may change.

``<family>_ctlv_family`` links the direct parent descriptor, owner-local schema,
callbacks, hooks, and priv lifecycle. ``ctlv_family_is_a()`` checks ancestry::

    if (!ctlv_family_is_a(family, &ctlv_example_ctlv_family))
            return -EINVAL;

``ctlv_destroy()`` removes userspace access and drains admitted operation
callbacks before running ``pre_fini``. It then stops event admission, drains
event producers, queues the terminal removal record, runs ``fini``, and
consumes the reference returned by ``ctlv_create()``. Open descriptions may
remain to drain core-owned records and otherwise fail with ``-ENODEV``.

Family priv
-----------

Every family implements ``priv_size``, ``init``, and ``fini``.
``pre-init: true``, ``post-init: true``, and ``pre-fini: true`` add the
corresponding callbacks:

.. code-block:: c

    size_t ctlv_example_priv_size(void)
    {
            return sizeof(struct ctlv_example_priv);
    }

    const void *ctlv_example_pre_init(const void *init_info)
    {
            const struct ctlv_example_init_info *info = init_info;

            return &info->parent_init_info;
    }

    int ctlv_example_init(struct ctlv_device *ctlvdev,
                          const void *init_info, void *priv)
    {
            struct ctlv_example_priv *example = priv;

            /* Parent priv is initialized already. */
            example->parent = ctlv_priv_parent(priv);
            ctlv_example_optional_thing_provide(ctlvdev);
            return 0;
    }

    int ctlv_example_post_init(struct ctlv_device *ctlvdev, void *priv)
    {
            return finish_initialization(priv);
    }

    void ctlv_example_pre_fini(struct ctlv_device *ctlvdev, void *priv)
    {
            prepare_for_teardown(priv);
    }

    void ctlv_example_fini(struct ctlv_device *ctlvdev, void *priv)
    {
            struct ctlv_example_priv *example = priv;

            release_resources(example);
    }

When requested, ``pre_init`` runs from the device family to the root and
returns immutable initialization information for the direct parent, or an
``ERR_PTR()``. It must not modify initialization information, acquire
resources, or return storage local to its own frame. Caller-owned stack storage
is valid because ``ctlv_create()`` uses it synchronously. Without ``pre_init``,
the family still receives its incoming ``init_info`` and its parent receives
NULL.

CTLV allocates every priv together in one zeroed block, with each object aligned
to ``CTLV_PRIV_ALIGN``. ``init`` runs from the root to the device family. After
every ``init`` succeeds, CTLV freezes the operation set and starts event
admission. Requested ``post_init`` callbacks then run from the device family to
the root, before character-device registration. All family privs and event
emission are available during ``post_init``.

For normal destruction, CTLV first withdraws userspace access and drains every
admitted operation callback. Requested ``pre_fini`` callbacks then run from the
root to the device family, without the registration lock or a CTLV spinlock
held. A family that published itself to kernel consumers in ``post_init`` must
withdraw itself and drain those consumers in ``pre_fini``. Provider state,
dynamic state changes, and event creation and emission remain available until
every ``pre_fini`` returns.

CTLV then stops new event contexts, drains admitted contexts, and queues the
terminal removal record. Only afterward does ``fini`` unwind from the device
family to the root. The same ordering applies when ``post_init`` or later
creation fails. If an ``init`` callback fails, ``pre_fini`` does not run: the
failing callback releases its own partial resources and CTLV unwinds only the
previously initialized ancestors through ``fini``. CTLV frees the priv memory
after all ``fini`` callbacks.
``ctlv_priv_parent()`` and
``ctlv_priv_child()`` return direct neighbors in this device's family chain; the
root has no parent and the device family has no child. During a family's
``init`` its child is not initialized yet; every child is still initialized
during ``pre_fini``, and during ``fini`` that child has already been finalized.
A private type needing stronger alignment
allocates that object separately and keeps its pointer in priv.

``ctlv_device_family()`` returns the immutable leaf-family descriptor supplied
to ``ctlv_create()``. It carries no reference and is valid through
``pre_fini`` and ``fini``; callers must not retain it beyond
``ctlv_destroy()``. Prefer ``ctlv_family_is_a()`` with a generated descriptor
for compile-time-known ancestry. A common family that cannot link against all
possible descendants may walk the returned descriptor's parent chain and
match the stable ``schema->name``.

``ctlv_device_parent()`` returns the exact ``parent_dev`` supplied to
``ctlv_create()``. The returned pointer carries no reference.

Extension points
----------------

The implementation that owns a message opens the nest an extension point stands
for and hands it to whichever family of the device's chain filled it:

.. code-block:: c

    struct ctlv_cursor nest;
    int ret;

    if (!ctlv_example_extension_start(reply, &nest))
            return 0;
    ret = ctlv_stub_fill(ctx, request, &nest);
    ctlv_nest_end(&nest);
    return ret;

The filling family implements the pair the generator declares for it, one per
extension point its specification fills:

.. code-block:: c

    int ctlv_example_child_extension_point_fill(void *priv,
                                                struct ctlv_op_ctx *ctx,
                                                const struct ctlv_cursor *request,
                                                struct ctlv_cursor *nest)
    {
            ctlv_example_child_extra_put(nest, 1);
            return 0;
    }

    void ctlv_example_child_extension_point_read(void *priv,
                                                 struct ctlv_op_ctx *ctx,
                                                 const struct ctlv_cursor *nest)
    {
            if (ctlv_example_child_extension_point_extra_present(nest))
                    handle(ctlv_example_child_extension_point_extra_get(nest));
    }

``request`` is the same filled nest of the request being answered, or ``NULL``
when that fill has no corresponding request scope. ``ctlv_stub_read()`` is the
other direction, for an
implementation handing the members it cannot know to the family that declared
them; a nest addressing nothing is nothing to read. Neither side names the family
on the other, and an extension point nothing filled is nothing to do.

Implementing an operation
=========================

The generated header of the family declares one function per action and query
it must implement, and the generated source binds them to the schema. A driver
implements the prototypes; it never fills in a table.

An action
---------

.. code-block:: c

    int ctlv_example_echo_action(void *priv, struct ctlv_op_ctx *ctx,
                                 const struct ctlv_cursor *request,
                                 struct ctlv_cursor *reply)
    {
            struct ctlv_example_priv *example = priv;
            u64 value = example->default_value;

            if (ctlv_example_echo_u_present(request))
                    value = ctlv_example_echo_u_get(request);

            ctlv_example_echo_u_put(reply, value);
            return 0;
    }

Getters are generated per attribute and per placement, so
``ctlv_example_echo_u_get()`` exists because ``echo.u`` is a member of that
request. A second placement with another local slot gets an operation- and
nest-qualified name. ``_present()`` says whether the message carried it; anything the
framework accepted is within its declared bounds, so a callback validates
nothing. Validation indexes each request nest by its generated schema and
dense local slots. Getters, presence checks, and repeated-attribute iterators
therefore do not rescan the TLV stream, and a cursor from another nest schema
is refused. An action with no reply returns zero and writes nothing.

A query in fragments
--------------------

A query writes until the serializers stop accepting, then says where to carry
on:

.. code-block:: c

    int ctlv_example_series_dump_query(void *priv, struct ctlv_op_ctx *ctx,
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
the frame that does the ending. Every continuing fragment must complete at
least one logical leaf, an empty completed nest included; merely re-emitting or
opening the continuation path is rejected.

Reporting an error
------------------

A callback or pre hook can return an errno with a short diagnostic:

.. code-block:: c

    if (device_is_active(example))
            return ctlv_op_error(ctx, -EBUSY,
                                 "configuration cannot change while active");

    if (queue >= example->max_queues)
            return ctlv_op_errorf(ctx, -EINVAL,
                                  "queue %u exceeds maximum %u",
                                  queue, example->max_queues);

The message is copied into the operation context and the first one recorded
wins. It may be temporary and is truncated to 255 bytes. The caller always
receives the errno; when its reply buffer has room, the framework also writes
the message as ``error.message`` in the all-or-nothing ``error-info`` reply.

Hooks
-----

The device family's pair wraps every effective non-core action or query. One that
asked for ``pre-post`` keeps its declaring family's second pair and callback
inside that outer pair. Core actions and queries use only the core family pair.
The order is: family pre, operation pre, availability check, callback,
operation post, family post.

Every callback receives the priv of the family that implements it. Thus the
outer pair receives the device-family priv while an inherited operation and its own pair
receive the declaring family's priv.

Each pre that succeeded has exactly one post, so a pre that fails must undo its
own work; its errno becomes the operation's result. A post cannot change what was
answered. Family-wide pairs do not stack along an inheritance chain. Both hooks
may sleep and neither may keep ``ctx``.

Availability
------------

Two things decide what a device publishes, and they are different:

.. code-block:: c

    /* From family init: this device has the optional op after all. */
    ctlv_example_optional_thing_provide(ctlvdev);

    /* Any time: the device has the op and refuses it for now. */
    ctlv_example_dynamic_thing_disable(ctlvdev);
    ctlv_example_dynamic_thing_enable(ctlvdev);

An operation declared ``optional: true`` does not exist on a device until that
device opts in during family initialization. Left alone it is absent from
``ops-dump``, has no schema, and is indistinguishable from one no family ever
declared. A disabled operation is listed, reported as disabled, and refused
when executed; enabling
or disabling it emits an ``op-changed`` event carrying the new state and a
generation.

Only an operation declared ``availability: dynamic`` may be disabled, and only
its own family's helper does either thing. A callback is mandatory whatever a
device decides, so the two declarations compose: an optional operation may also
be dynamic.

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
    if (!ret)
            ret = ctlv_blob_put(blob);
    else
            ctlv_blob_put(blob);
    return ret;

Resolving acquires a pin, file reference, or dma-buf reference that keeps the
backing stable. CPU mapping is lazy, so an attachment-only dma-buf exporter
works when the family requests device access. Every resolve is released with
``ctlv_blob_put()`` before the callback returns. The framework cleans up and
fails an otherwise successful operation if a callback forgets one. Access and
release calls for one blob are made serially in the callback's task.

Direction is the schema's, not the caller's: ``ctlv_blob_write()`` on a blob
declared ``input`` is a bug that warns. A resolve failing is ordinary, and its
errno is the caller's answer. A dma-buf CPU access uses the standard
``dma_buf_begin_cpu_access()``/``dma_buf_end_cpu_access()`` ownership protocol;
userspace must obey the input/output ownership rules while the ioctl runs.
Memfd resolution also checks the open mode, the file's actual read/write
operations, and seals before returning a blob. Output and inout require
``F_SEAL_SHRINK | F_SEAL_SEAL`` without either write seal, freezing their
writability for the callback. The framework uses a private file description,
so later ``F_SETFL`` changes and ``O_APPEND`` cannot move writes outside the
declared range; an append-only or immutable inode is rejected. A hugetlb memfd
is supported for input, but rejected for output or inout because hugetlbfs has
no write operation.

A family that submits device DMA maps an opaque blob without choosing the
direction:

.. code-block:: c

    struct ctlv_blob_dma *dma;
    struct dma_fence *fence;
    struct sg_table *sgt;
    u64 skip;

    dma = ctlv_blob_dma_map(blob, dev, &sgt, &skip);
    if (IS_ERR(dma)) {
            ret = PTR_ERR(dma);
            goto out_blob;
    }
    fence = prepare_job(sgt, skip, ctlv_blob_len(blob));
    if (IS_ERR(fence)) {
            ret = PTR_ERR(fence);
            goto out_dma;
    }
    ret = ctlv_blob_dma_begin(dma, fence);
    if (!ret) {
            ret = submit_prepared_job(fence);
            ctlv_blob_dma_submit_done(dma, ret);
    }
    /* On success, the completion path signals fence after hardware stops. */
out_dma:
    ctlv_blob_dma_unmap(dma, ret);
    if (!IS_ERR(fence))
            dma_fence_put(fence);
out_blob:
    if (!ret)
            ret = ctlv_blob_put(blob);
    else
            ctlv_blob_put(blob);
    return ret;

The scatterlist is a borrowed, immutable whole-buffer DMA mapping; ``skip``
locates the blob's subrange in its DMA-length stream. After all potentially
allocating preparation, ``ctlv_blob_dma_begin()`` waits implicit dependencies
and publishes the family's unsignaled, non-container completion fence with read
or write usage derived from the schema. It also opens the annotation which
``ctlv_blob_dma_submit_done()`` closes immediately after the attempt to queue
the prepared work. A zero status leaves the family responsible for signaling
the fence, with the normal dma-fence annotations, after hardware stops. A
negative submission status sets the fence error and signals it immediately
because no device completion will arrive. Unmapping is called in the same
callback task after hardware has stopped; it waits for the fence if necessary,
records the completion status, and tears down mapping and attachment. An
operation holds at most one device mapping at a time, so it cannot acquire new
implicit dependencies after publishing a fence; unmap it before mapping another
blob.
VA and memfd backings return ``-EOPNOTSUPP`` from this device API, so an attr
intended for direct device DMA must permit dma-buf.

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

If hardware changed before an event context could be allocated, the family
must commit its invalidation and explicitly report the missing notification:

.. code-block:: c

    invalidate_stale_state(priv);
    priv->generation++;

    ctx = ctlv_example_thing_happened_create(ctlvdev, &event, GFP_ATOMIC);
    if (IS_ERR(ctx)) {
            if (PTR_ERR(ctx) == -ENOMEM)
                    ctlv_event_lost(ctlvdev,
                                    CTLV_EXAMPLE_OP_THING_HAPPENED);
            return;
    }
    ctlv_example_generation_put(event, priv->generation);
    ctlv_op_emit(ctx);

``ctlv_event_lost()`` allocates nothing and does not sleep. For each subscribed
file description it advances the event sequence and aggregates the loss into
the preallocated ``EVENT_OVERFLOW`` record. The caller keeps the device alive
and serializes the report with normal emission of that event. Like event
emission, it is not called from interrupt context. It is only for an event that
actually occurred but could not be constructed: do not call it for an aborted
event or for copy-allocation and full-queue drops already accounted for by
``ctlv_op_emit()``. Reports after event shutdown or the terminal
``DEVICE_REMOVED`` record are ignored.

Reporting an error
------------------

A callback returns a negative errno, and that is what the ioctl returns. Where
the framework itself refuses a message it also writes error information into
the caller's reply buffer, naming the reason and the offset of the offending
attribute. A driver does not build that; it adds its own errno and its reply.

The worked example
==================

``ctlv-test`` is the most complete example in the tree:

``Documentation/ctlv/specs/ctlv-test.yaml``
    Every declaration form the schema language has, including an extension
    point that its child family fills.

Configuration
=============

``CONFIG_CTLV``
    The framework. A family driver selects it.
