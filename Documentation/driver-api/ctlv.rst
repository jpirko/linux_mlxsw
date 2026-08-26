.. SPDX-License-Identifier: GPL-2.0

=====================
Writing a CTLV family
=====================

:Author: Jiri Pirko

A CTLV family is a set of system calls for one sort of device: actions and
queries a program executes. The family is written as a
YAML specification, a generator turns that into headers, and the family
implementation supplies the callbacks the generated header names. CTLV checks
every request against the specification before a callback sees it. The
userspace ABI is :doc:`/userspace-api/ctlv`; the in-tree specifications are
under ``Documentation/ctlv/specs``.

The example
===========

This guide writes a small invented family, ``ctlv-example``, with two system
calls::

    item_set_action(u64 index, string label?, blob data?) -> status;
    item_get_query() -> { u64 index; string label?; }[];

``index`` is 0..4095, ``label`` is at most 63 bytes excluding NUL, and
``data`` is at most 1 MiB with 8-byte alignment. ``?`` marks an optional
argument. An action runs once and returns a status. A query returns an answer
that may be larger than the caller's buffer, which CTLV delivers in as many
fragments as the buffer needs; a list, as here, is the usual case.

Write the specification
=======================

The specification starts with the family. ``ctlv-example`` inherits ``ctlv``,
the root family every device answers:

.. code-block:: yaml

    # SPDX-License-Identifier: ((GPL-2.0 WITH Linux-syscall-note) OR BSD-3-Clause)
    %YAML 1.2
    ---
    # Numbered items, each with an optional label.
    family: ctlv-example
    inherits: ctlv

The comment on the lines above an entry is its documentation, which the
generated headers carry. The generator refuses a family, record, field, op,
enum or flags without one, and an enum value or a flag without its own, so
values go one to a line.

Each system call is an op:

.. code-block:: yaml

    ops:
      # Sets an item's label and data.
      item-set:
        type: action
        request:
          # Which item it is.
          index: uint(..4095)
          # What the item is called.
          label?: string(..63)
          # The item's contents, which the device reads.
          data?: blob(in, ..1M, align 8, va)

      # Lists the items.
      item-get:
        type: query
        reply:
          # Every item the device keeps.
          items[]:
            # Which item it is.
            index: uint(..4095)
            # What the item is called.
            label?: string(..63)

``request`` is what an op takes and ``reply`` what it returns. An op
without a ``request`` takes no arguments, and an action without a ``reply``
returns only its status. An action or a query may also set these keys:

=========================== ========== ========================================
Key                         Default    What it does
=========================== ========== ========================================
``fd-access``               read-write ``read`` lets an ``O_RDONLY`` fd run it
``suggested-reply-length``  64K        the buffer a caller starts a query with
``optional``                false      see `Optional operations`_
``availability``            fixed      see `Dynamic availability`_
``pre-post``                false      see `Hooks`_
=========================== ========== ========================================

An op that reads is named ``<object>-get``, the object in the singular, and
each kind of object has one. A get over a collection, as ``item-get`` is, is a
query even while it returns one object, as an op's type never changes; its
request fields are optional filters, none meaning every object. A filter
matching nothing gives an empty answer, and an ID naming nothing fails with
``ENOENT``. An action get returns exactly one thing, such as the device.

What an op takes or returns is a record: the fields one level of a message
may hold, one per line. ``name?`` is an optional field, ``name[N]`` one that
occurs up to N times, ``name[enum]`` one that occurs up to once per value of
the enum, and ``name[]`` one a query's reply repeats any number of times. A
field with fields indented below it is a record, the one declared there, as
``items[]`` is. Each field has generated helpers named after its op and the
records it is in: ``ctlv_example_item_set_index_get()`` reads ``index`` in an
``item-set`` request, and ``ctlv_example_item_get_items_index_put()`` writes
it in an item of the ``item-get`` reply.

Fields that several messages repeat can be declared once as a named record,
and then the specification reads:

.. code-block:: yaml

    records:
      # One item, as item-set takes it and item-get lists it.
      item:
        # Which item it is.
        index: uint(..4095)
        # What the item is called.
        label?: string(..63)

    ops:
      # Sets an item's label and data.
      item-set:
        type: action
        request:
          $include: item
          # The item's contents, which the device reads.
          data?: blob(in, ..1M, align 8, va)

      # Lists the items.
      item-get:
        type: query
        reply:
          # Every item the device keeps.
          items[]: record item

``$include: item`` puts the record's fields in place, and ``record item`` makes
it a whole message or a record's field. The field is then the same wherever it
appears: ``item.index`` has one type, one bound and one generated helper in
the ``item-set`` request and in the items ``item-get`` returns, and
``ctlv_example_item_index_put()`` writes it in the latter. This guide keeps
the fields spelled out.

A field's type, the ID it has in a message, is made of the family's number,
its record and its place in that record. A named record is numbered by its
place in ``records``, an op's request and reply by the op's place in ``ops``,
and a record declared in place by its place among the records declared beside
it. Every one of those places is ABI, so records, fields and ops are only ever
appended. Records declared in place go at most four deep and fifteen side by
side below a named record or an op's message; a deeper or wider tree takes
another named record.

A field has one schema kind. Bounds follow it in parentheses, inclusive, with
limits such as ``u32-max`` written by name and sizes such as ``4K``, which an
integer takes only with the ``bytes`` unit, as in ``uint(1..4K, bytes)``:

============ ============================== ==================================
Kind         Holds                          Declared as, for example
============ ============================== ==================================
``uint``     an unsigned 64-bit integer     ``uint(..4095)``
``sint``     a signed 64-bit integer        ``sint(-8..8)``
``bool``     true or false                  ``bool``
``string``   text, bounded without its NUL  ``string(..63)``
``binary``   bytes                          ``binary(16..64)``
enum         one value of an enum           the enum's name
flags        a set of flags                 the flags' name
record       fields                         ``record item``, or fields below
``blob``     caller memory, see `Blobs`_    ``blob(in, ..1M, align 8, va)``
============ ============================== ==================================

Bounds, record membership and whether a field is required are ABI.

The top-level keys may come in any order; the in-tree specifications put
``enums`` and ``flags`` first, then ``records`` and ``ops``.

An enum a uapi header already defines names the header in ``$header``, a path
under ``include/uapi``, and the enum in ``$enum``. The enum's constants are its
values, under their own names and numbers, whatever they start from and
whatever gaps they leave, and need no comments. ``$end`` names a sentinel,
leaving it and every constant after it out:

.. code-block:: yaml

    enums:
      # The kernel's hash algorithms.
      hash-algo:
        $header: linux/hash_info.h
        $enum: hash_algo
        $end: HASH_ALGO__LAST

The generated code includes the header rather than repeating the enum. CTLV
takes any number from the lowest value up to one below ``$end``, or to the
highest value without it, so a number in a gap reaches the callback, which
refuses it. ``$end`` may move later as the header grows, never earlier. An
enum written as ``extension`` has no values of its own; see
`Inheritance and HW driver fields`_.

No message outgrows the core's inline limit, ``CTLV_MAX_INLINE_MESSAGE_LEN``
(64 KiB): the generator refuses an operation whose request or action reply
could be longer, or a query whose longest field could not fit in one
fragment. Larger data goes in a blob.

Generate the family
===================

A new family is one change holding its specification, its registry entry and
its output manifest entry::

    Documentation/ctlv/specs/ctlv-example.yaml
    Documentation/ctlv/families.yaml
    Documentation/ctlv/generated-files.yaml

The family's position in ``families.yaml`` is its family ID, part of every
type and op ID it generates. Append only: never reorder, remove,
rename or reuse an entry. Agree the position with the CTLV maintainers (CTLV
SUBSYSTEM in MAINTAINERS) before adding a family, since families added in
parallel trees would otherwise end up with the same ID. The output manifest
says where the generated UAPI, kernel and userspace files go.

Generate the files and record the new family's ABI::

    make -C tools/ctlv regen

``regen`` names each file it writes. A new family should add its generated
files, its snapshot, ``Documentation/ctlv/abi/ctlv-example.yaml``, its entry
in ``Documentation/ctlv/abi/families.registry.yaml`` and one line in
``include/uapi/ctlv/ctlv_families_gen.h``, and change nothing generated for
another family. Anything else means the specification or the generator
changed more than intended. ``regen`` refuses a specification change that
breaks the ABI as last committed unless ``ABI_BREAK=1`` says it is meant, so a
snapshot's diff shows every ABI change for review. ``make -C tools/ctlv``
checks the same without writing anything.

Never edit a generated file; change the YAML or the generator and regenerate.

``make -C tools/ctlv docs`` writes optional RST reference pages to
``Documentation/output/ctlv/families``.

Create and destroy a device
===========================

CTLV comes in two parts. The engine in ``lib/ctlv/`` is what a subsystem embeds
in its devices and calls from their file operations. The nodes, declared in
``<linux/ctlv_device.h>``, are character devices of the subsystem's own class,
with the engine's file operations.

A device's CTLV state is a ``struct ctlv``, embedded in the structure of
whoever gives the device. A callback gets the instance and reaches that
structure with ``container_of()``. The node is a ``struct ctlv_device``, which
the structure embeds beside the instance:

.. code-block:: c

    struct example_device {
            struct ctlv_device ctlvdev;
            struct ctlv ctlv;
            u64 items_count;
            u64 generation;
    };

    static struct example_device *to_example(struct ctlv *ctlv)
    {
            return container_of(ctlv, struct example_device, ctlv);
    }

The subsystem's class is a ``struct ctlv_device_class``, registered once with
``ctlv_device_class_register()``, which takes its device numbers. Its
``dev_release`` frees the structure, so an open node holds the module that
registered the class:

.. code-block:: c

    static void example_release(struct device *dev)
    {
            struct example_device *example =
                    container_of(dev, struct example_device, ctlvdev.dev);

            ctlv_fini(&example->ctlv);
            ctlv_device_fini(&example->ctlvdev);
            kfree(example);
    }

    static struct ctlv_device_class example_class = {
            .class = {
                    .name = "example",
                    .dev_release = example_release,
            },
            .max_devices = 16,
    };

A device of the class is then made:

.. code-block:: c

    example = kzalloc(sizeof(*example), GFP_KERNEL);
    if (!example)
            return -ENOMEM;
    ret = ctlv_device_init(&example->ctlvdev, &example_class, parent,
                           &example->ctlv);
    if (!ret)
            ret = ctlv_init(&example->ctlv, &example->ctlvdev.dev,
                            &ctlv_example_family, NULL);
    if (ret)
            goto err_put;
    ctlv_publish(&example->ctlv);
    ret = ctlv_device_add(&example->ctlvdev);
    if (ret)
            goto err_unpublish;

``ctlv_device_init()`` names the node after the class, ``/dev/example0``.
``ctlv_init()`` resolves the family chain. Until the instance is published, the
driver selects its optional operations and their initial dynamic state, see
Hooks and availability. ``ctlv_publish()`` makes operations live, and only then
may the node be added, as a program may open it as soon as it appears.
``ctlv_device_add()`` adds the node, whose uevent names the family; that is how
libctlv and udev find the device. Destruction is the reverse:

.. code-block:: c

    ctlv_device_del(&example->ctlvdev);
    ctlv_unpublish(&example->ctlv);
    put_device(&example->ctlvdev.dev);

``ctlv_unpublish()`` waits for the operations in flight; no callback runs
afterwards. Each open file description fails its operations with ``-ENODEV``.
The structure is freed with the last reference, the device's or an open file's,
so what the callbacks used may stay in it. From ``ctlv_device_init()`` on,
``put_device()`` alone unwinds a failed initialization, as it does a failed add
once the instance is unpublished.

A file of another subsystem
---------------------------

A subsystem may serve the instance on a file of its own instead, a misc device
whose other operations are the subsystem's, say. It publishes the instance
around the life of that file, attaches CTLV to each open file and passes on its
ioctls:

.. code-block:: c

    static int foo_open(struct inode *inode, struct file *filp)
    {
            struct foo *foo = container_of(filp->private_data, struct foo, misc);
            struct ctlv_file *cfile = ctlv_file_open(&foo->ctlv, filp);

            if (IS_ERR(cfile))
                    return PTR_ERR(cfile);
            filp->private_data = cfile;
            return 0;
    }

    static long foo_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
    {
            long ret = ctlv_file_ioctl(filp->private_data, cmd, arg);

            return ret == -ENOIOCTLCMD ? foo_own_ioctl(filp, cmd, arg) : ret;
    }

Its release calls ``ctlv_file_close()``. The file's uevent is the other
subsystem's and names no family, so a program opens the file by its name.

The ``struct ctlv_hooks`` passed to ``ctlv_init()`` holds the device's pre and
post hooks, see `Hooks`_. The device structure's release calls ``ctlv_fini()``
when the last reference goes.

Implement operations
====================

Action
------

Generated getters read only what validation accepted, and generated
serializers write only what the reply declares:

.. code-block:: c

    int ctlv_example_item_set_action(struct ctlv *ctlv,
                                     struct ctlv_op_ctx *ctx,
                                     const struct ctlv_cursor *request,
                                     struct ctlv_cursor *reply)
    {
            struct example_device *example = to_example(ctlv);
            u64 index = ctlv_example_item_set_index_get(request);

            if (item_busy(example, index))
                    return ctlv_op_error(ctx, -EBUSY, "item is busy");
            return item_set(example, index);
    }

Use ``_present()`` for optional fields and ``_for_each()`` for repeated
ones. ``ctlv_op_error()`` preserves the errno and adds a short diagnostic when
the caller supplied enough reply space.

Query
-----

.. code-block:: c

    int ctlv_example_item_get_query(struct ctlv *ctlv,
                                      struct ctlv_op_ctx *ctx,
                                      const struct ctlv_cursor *request,
                                      struct ctlv_cursor *reply)
    {
            struct example_device *example = to_example(ctlv);
            u64 generation = READ_ONCE(example->generation);
            u64 cursor = ctlv_op_query_cursor(ctx);
            struct ctlv_cursor item;
            u64 i = cursor ? cursor - 1 : 0;

            if (cursor && ctlv_op_query_generation(ctx) != generation)
                    return -ESTALE;
            for (; i < example->items_count; i++) {
                    if (!ctlv_example_item_get_items_start(reply, &item))
                            return ctlv_op_query_more(ctx, generation, i + 1);
                    if (!ctlv_example_item_get_items_index_put(&item, i)) {
                            ctlv_record_cancel(&item);
                            return ctlv_op_query_more(ctx, generation, i + 1);
                    }
                    ctlv_record_end(&item);
            }
            return ctlv_op_query_done(ctx, generation);
    }

A continuation cursor is nonzero and advances. Every continuing fragment
completes a logical leaf. An item that runs out of room partway is dropped with
``ctlv_record_cancel()``, for the next fragment to write again. Call
``ctlv_op_query_more()`` while cursors for open continuation records are alive.
A generation mismatch returns ``-ESTALE``.

Hooks and availability
======================

Hooks
-----

The pre and post hooks of the ``struct ctlv_hooks`` passed to ``ctlv_init()``
run around every action and query the core does not answer. An op declared
with ``pre-post: true`` also runs hooks of its own family, inside the
device's:

.. code-block:: yaml

    item-get:
      type: query
      pre-post: true

The family then implements ``ctlv_example_item_get_pre()`` and
``ctlv_example_item_get_post()``, and the order is::

    device pre -> operation pre -> availability check -> callback
               -> operation post -> device post

Each successful pre hook has one post hook. A failing pre hook undoes its own
work. Hooks may sleep and must not retain ``ctx``. Each hook and callback gets
the instance.

Optional operations
-------------------

An op declared ``optional: true`` is published only on a device whose driver
provides it before publishing the instance:

.. code-block:: yaml

    item-set:
      type: action
      optional: true

.. code-block:: c

    ctlv_example_item_set_provide(&example->ctlv);

On a device that does not provide it, a program finds no entry for the
operation in ``op-get`` or ``op-schema-get``, and executing it fails with
``-EOPNOTSUPP``.

Dynamic availability
--------------------

An op declared ``availability: dynamic`` can be refused for a while on a
device that publishes it:

.. code-block:: yaml

    item-get:
      type: query
      availability: dynamic

.. code-block:: c

    ctlv_example_item_get_disable(&example->ctlv);
    ctlv_example_item_get_enable(&example->ctlv);

A disabled operation stays in ``op-get``, which reports it disabled, and
returns ``-EOPNOTSUPP``.

Blobs
=====

A blob field describes caller-owned memory. Resolve it only in the callback
that received its request and release every successful resolve before return:

.. code-block:: c

    blob = ctlv_example_item_set_data_resolve(ctx, request);
    if (IS_ERR(blob))
            return PTR_ERR(blob);

    ret = ctlv_blob_read(blob, 0, buf, ctlv_blob_len(blob));
    put_ret = ctlv_blob_put(blob);
    return ret ?: put_ret;

The schema fixes direction, backing, length, and alignment. Resolution holds
the backing for the ioctl. An output blob too short for
what the operation produces is the caller's to enlarge: return
``ctlv_blob_too_short()`` with the length needed, rather than writing part of
it.

Only a request carries a blob; the generator refuses one in a reply. A request
that shares fields with a reply includes them and declares its blob beside them,
as ``item-set`` does when it includes ``item``.

Inheritance and HW driver fields
================================

Several HW drivers can implement ``ctlv-example``, and each knows things
about its devices that a program may want, such as which bank of the foo
chip's SRAM holds the items. Those do not belong in ``ctlv-example``, which
all of them share. Instead, ``ctlv-example`` leaves a place for them, an
extension point, and each HW driver has a family of its own that inherits
``ctlv-example`` and fills it.

Say ``ctlv-example`` gains a system call reporting the device::

    device_get_action() -> { details? };

On a device of the foo driver, ``details`` holds foo's fields::

    device_get_action() -> { details? { u64 sram_bank; }; };

``ctlv-example`` declares the op and leaves the contents of ``details`` to the
HW driver, with an extension point:

.. code-block:: yaml

    ops:
      # Reports the device.
      device-get:
        type: action
        fd-access: read
        reply:
          # Empty here; a HW driver's family fills it with fields of its own.
          details?: extension device-details

The foo driver's family fills it by declaring a record of the same name:

.. code-block:: yaml

    # The devices of the foo driver, which fill what ctlv-example leaves open.
    family: ctlv-example-foo
    inherits: ctlv-example

    records:
      # The extension point ctlv-example left empty, filled for foo devices.
      device-details:
        # The bank of the foo chip's SRAM that holds the items.
        sram-bank: uint(..3)

The foo driver initializes its instances with ``ctlv_example_foo_family``, and
they answer every ``ctlv-example`` op. A family implements only the ops it
declares, so the inherited ones keep the callbacks and hooks of
``ctlv-example``, and the foo module depends on the module that implements
``ctlv-example``.

The ``device-get`` callback in ``ctlv-example`` opens the record and has the
device's family fill it:

.. code-block:: c

    if (ctlv_example_device_get_details_start(reply, &details)) {
            ret = ctlv_ext_fill(ctx, NULL, &details);
            ctlv_record_end(&details);
            if (ret)
                    return ret;
    }

The foo family implements the generated fill callback:

.. code-block:: c

    int ctlv_example_foo_device_details_fill(struct ctlv *ctlv,
                                             struct ctlv_op_ctx *ctx,
                                             const struct ctlv_cursor *request,
                                             struct ctlv_cursor *record)
    {
            struct foo_device *foo = container_of(ctlv, struct foo_device,
                                                  example.ctlv);

            ctlv_example_foo_device_details_sram_bank_put(record,
                                                          foo->sram_bank);
            return 0;
    }

``request`` is the matching record of the request, for an op whose request has
an extension point too; ``device-get`` has none and passes ``NULL``. An
extension point no family fills adds no record and no length. ``ctlv-example``
never names ``ctlv-example-foo``, and the foo family names nothing of
``ctlv-example`` beyond the extension point it fills.

A program knows whose fields it is reading without looking at them. A
device's family chain runs from the root to the family of its instance,
here ``ctlv``, ``ctlv-example`` and ``ctlv-example-foo``, and that last one,
the leaf, is the HW driver's. ``family-get`` reports the chain on the open
device, so a program picks its handler for the HW driver's fields once, when
it opens the device; the uevent carries the same chain for udev rules. A tool
that knows only ``ctlv-example`` skips the TLVs it does not know, as on any
reply. Each TLV in a filled record also carries the family that declared its
field, in bits 63..48 of its type: ``CTLV_TYPE_FAMILY()`` extracts it and
``family-get`` maps it to a name.

An enum can be left open the same way, when each HW driver has values of its
own, such as the data formats its devices produce. ``ctlv-example`` declares
the enum as an enum extension point, and its fields name it as any enum:

.. code-block:: yaml

    enums:
      # The format of a device's data; a HW driver's family gives the values.
      data-format: extension

The foo driver's family gives the values, by declaring an enum of that name:

.. code-block:: yaml

    enums:
      # Fills ctlv-example's data-format.
      data-format:
        # The foo device's own layout.
        - foo-raw

One family of a chain gives the values, numbered from 1 and appended to as any
enum's. The core takes only those values on a device of that family, in a
request and in what its driver puts in a reply, and none on a device whose
chain gives none. The names are in the uapi header of the family that gives
them, such as ``CTLV_EXAMPLE_FOO_DATA_FORMAT_FOO_RAW``. A family that checks
values a HW driver hands it, before any reaches a reply, asks
``ctlv_enum_ext_max()`` for the highest the driver's family gives.

The structures embed one another as the families inherit. The module
implementing ``ctlv-example`` keeps what every device of that family has in a
structure around the instance, and exports what initializes it for a family
that inherits ``ctlv-example``. A HW driver embeds that structure in its own
and describes itself there, for the ``ctlv-example`` callbacks to use:

.. code-block:: c

    struct ctlv_example_device {
            struct ctlv ctlv;
            const struct ctlv_example_provider *provider;
    };

    struct foo_device {
            struct ctlv_device ctlvdev;
            struct ctlv_example_device example;
            u32 sram_bank;
    };

    static const struct ctlv_example_provider foo_provider = {
            .name = "foo",
            .ops = &foo_provider_ops,
    };

    foo = kzalloc(sizeof(*foo), GFP_KERNEL);
    if (!foo)
            return -ENOMEM;
    ret = ctlv_device_init(&foo->ctlvdev, &foo_class, parent,
                           &foo->example.ctlv);
    if (!ret)
            ret = ctlv_example_device_init(&foo->example, &foo->ctlvdev.dev,
                                           &ctlv_example_foo_family,
                                           &foo_provider);

The node is of the foo driver's own class, ``foo_class``, registered as
``example_class`` is above. ``ctlv_example_device_init()`` refuses a family
that ``ctlv_family_inherits()`` says is not ``ctlv-example`` or a descendant,
then calls ``ctlv_init()``.

Tests and examples
==================

``Documentation/ctlv/specs/ctlv-test.yaml``
    Every schema kind, records, actions, queries, blobs, availability, hooks,
    and an extension point.

``drivers/ctlv/tests/ctlv_test.c``
    The ``ctlv-test`` family implementation.

``drivers/ctlv/tests/ctlv_test_child.c``
    A separately built inheriting family and extension-point fill.

``lib/ctlv/tests/*_kunit.c``
    Unit tests of the engine and of its nodes.

``tools/testing/selftests/ctlv/``
    Raw-message, generated-binding, allocation-failure, and mutation selftests.

Kernel API
==========

.. kernel-doc:: include/linux/ctlv.h
.. kernel-doc:: lib/ctlv/core.c
   :export:
.. kernel-doc:: lib/ctlv/builder.c
   :export:
.. kernel-doc:: lib/ctlv/schema.c
   :export:
.. kernel-doc:: lib/ctlv/blob.c
   :export:
.. kernel-doc:: include/linux/ctlv_device.h
.. kernel-doc:: lib/ctlv/device.c
   :export:
