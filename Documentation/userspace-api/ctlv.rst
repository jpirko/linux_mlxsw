.. SPDX-License-Identifier: GPL-2.0

==================
CTLV userspace ABI
==================

:Author: Jiri Pirko

Char-Device Tag-Length-Value (CTLV) is a flexible way to describe system calls
delivered over ioctl for a kernel subsystem. It provides a 64-bit operation
number with variable-length input and output arguments described in a YAML
schema. The arguments are serialized using a TLV scheme. Strong support for
forward and backward compatibility is baked into the schema design.

CTLV excels for uAPIs that are too unconstrained to fit in simple structs:
things that have arrays of arrays, variable-size structs, variable-sized data,
strings and similar, which are challenging to represent otherwise.

As a transport, CTLV uses a global number space. Inspection tools like strace,
or a seccomp user-notification supervisor, can always decode a CTLV ioctl
without having to know what subsystem is using it. The global number space is
organized into families, each roughly corresponding to a subsystem.

A family describes what its devices accept in YAML, a generator turns that into
kernel metadata, UAPI headers and userspace bindings, and CTLV validates every
message before a family callback sees it. Userspace executes every operation
through one ioctl and reads events with ``read()``.

This document is the ABI. What each family accepts is specified under
``Documentation/ctlv/specs``; writing a family is :doc:`/driver-api/ctlv`.

Goals
=====

A uAPI too unconstrained for fixed structures needs four things: every request
checked before a driver sees it, against one description that its headers and
documentation come from too; room to grow in both directions without versions;
one encoding that tools decode without knowing the subsystem; and answers
larger than the caller's buffer, read in parts that stay consistent.

CTLV is not worth it for a value or two, where a sysfs attribute or a plain
ioctl is simpler.

Each part of the design serves one of the needs:

- The schema. CTLV checks a message's bounds, lengths, occurrence counts and
  record depth (a record is a TLV of TLVs) against it before a family callback
  runs, and the generator makes the kernel metadata, the UAPI header, the
  userspace bindings and the documentation from it.
- The tree of TLVs, for records and variable data. Fields are optional or
  required by declaration, so a message can grow without breaking a caller
  that does not know the new field.
- One ioctl over a global number space. A new operation is a new
  specification entry rather than a new ioctl number, and strace or a seccomp
  supervisor decodes any family.
- Queries apart from actions. Only a query can be answered in parts: the
  kernel returns what fits with a cursor to continue from, and a generation,
  so that a caller whose answer changed between the parts gets ``ESTALE``
  rather than a mix of two states.
- Introspection. Every device answers ``family-get``, ``op-get`` and
  ``op-schema-get``, so a program built without a family, such as ``ctlvctl``,
  still learns what the device accepts.

`Compatibility`_ collects the rules for growing a family.

An example
==========

Take ``ctlv-example``, a small invented family whose devices hold numbered
items, each with an optional label; :doc:`/driver-api/ctlv` writes it. Its
system calls are::

    item_set_action(u64 index, string label?, blob data?) -> status;
    item_get_query() -> { u64 index; string label?; }[];
    item_changed_event -> { u64 index; string label?; };

``?`` marks an optional argument. The family's specification declares
``item-set`` with the bounds the kernel checks:

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
          data?: blob(in, ..1M, align 8, va memfd dma-buf)

To set the label of item 3 to "ab", a program builds a request of two TLVs,
the index and the label, under the root every message starts with::

    offset  TLV               len  flags   contents
    ------  ----------------  ---  ------  ----------------
         0  root               64  RECORD  type = 0
        16  .. index           24  -       3
        40  .. label           19  -       "ab" and its NUL
        64  end of message

and passes it to the device:

.. code-block:: c

    struct ctlv_op_exec exec = {
            .size = sizeof(exec),
            .op_id = CTLV_EXAMPLE_OP_ITEM_SET,
            .request = (__u64)(uintptr_t)msg,
            .request_len = 64,
    };

    if (ioctl(fd, CTLV_IOC_OP_EXEC, &exec))
            err(1, "item-set");

The op ID and the types come from the family's generated UAPI header.
Before the family sees the request, the kernel checks it against the
specification: ``index`` must be at most 4095 and ``label`` at most 63 bytes.

``libctlv`` and the generated bindings build and send the same message:

.. code-block:: c

    struct ctlv_error error;
    struct ctlv_msg *req;
    struct ctlv_dev *dev;

    /* Error checks left out. */
    ctlv_open("ctlv-example", 0, O_RDWR, &dev);
    ctlv_example_item_set_request_new(&req);
    ctlv_example_item_set_index_put(req, 3);
    ctlv_example_item_set_label_put(req, "ab");
    ctlv_example_item_set(dev, req, &error);

and so does ``ctlvctl``, given the family's specification::

    ctlvctl --spec-dir Documentation/ctlv/specs \
            exec /dev/ctlv-example item-set index=3 label=ab

The sections below define each piece: the device, the message format, the
ioctl, queries, events, blobs and introspection, and then the rules of
compatibility.

Terms
=====

family
    The specification of what a device accepts, its operations and records,
    under one number of the global number space. A family may inherit another,
    its parent, taking its operations and filling the extension points it
    leaves. A device answers as one family and is compatible with each of its
    ancestors.

operation, op
    What a family declares a device does: an action, executed once and
    acknowledged; a query; or an event, which the device produces and a
    subscribed file description reads.

TLV
    Type, length, value: a ``struct ctlv_hdr`` and its payload.

record
    A TLV whose payload is TLVs, its fields, flagged ``CTLV_HDR_F_RECORD``:
    a named record of the family, an operation's request or reply, an
    event's payload, or a record declared inside one of those.

root
    The record a message starts with, of type zero: the operation's request
    or reply, or the event's payload.

message
    A request, a reply or an event: one root record.

field
    One member of a record, a value or a record, required or optional, with its
    bounds and how often it may repeat.

type
    The 64-bit number in a TLV's header naming its family, its record and its
    field.

extension point
    A field a family leaves for a descendant family to fill with a record of
    its own, as ``ctlv-accelerator`` leaves ``device-info`` for
    ``ctlv-accelerator-foo``.

enum extension point
    An enum a family leaves for a descendant family to give values, so that a
    device takes the values its own family chain gives.

query
    An operation answered in as many fragments as its answer needs, which a
    generation keeps consistent.

fragment
    One reply of a query: as much of the answer as fits, with a cursor to
    continue from.

file description
    What ``open()`` creates and an fd refers to, as POSIX uses the term. It
    holds the access the device was opened with, the event subscriptions and
    the event queue, which fds made by ``dup()`` or passed by ``SCM_RIGHTS``
    share.

Devices
========

A device appears in one of two places, and the same ioctls run its family's
operations on both:

- a character device of a subsystem, in its class and ``/dev`` directory, as
  an accelerator subsystem would have it::

      /dev/accelerator/accelerator0
      /sys/class/accelerator/accelerator0

- a file whose other operations are a subsystem's own.

Sysfs shows class membership, the ``dev`` number and the device-model link to
the parent device. There is no family sysfs attribute; ``family-get`` on an
open descriptor is the authoritative answer to what a device is.

The number in a name says nothing about the device and, like ``sdX`` or
``eventN``, may differ between boots. Stable names come from udev rules
matching the uevent properties described under Discovery. A container is
given such a name, ``/dev/ctlv-example`` below, rather than whichever number
the device got.

Each open file description has its own event subscriptions and event queue.
``O_RDONLY`` opens an observation fd, ``O_RDWR`` opens a full-control fd, and
``O_WRONLY`` is rejected with ``-EACCES``. Every action and query declares the
authority it needs: ``read`` permits either accepted fd, while ``read-write``
requires full control. Events are readable and subscribable through both.
``dup()`` and ``SCM_RIGHTS`` preserve that authority; attenuation requires a
separate observation open.

Discovery
---------

Both uevents, addition and removal, carry the family chain as numbered
properties::

    CTLV_FAMILY=ctlv-accelerator-foo
    CTLV_COMPATIBLE_0=ctlv
    CTLV_COMPATIBLE_1=ctlv-accelerator
    CTLV_COMPATIBLE_2=ctlv-accelerator-foo
    CTLV_COMPATIBLE_N=3

``CTLV_FAMILY`` is the family the device answers as, the numbered properties
are its ancestry from the root to that family, and ``CTLV_COMPATIBLE_N`` is
how many there are. An index never moves: changing a family's parent is an
ABI break, adding a descendant only appends.

A device's uevents carry them whichever subsystem it is in, so a program finds
every device by ``CTLV_FAMILY`` rather than by class. A program that wants
every device, including ones appearing while it starts, must listen before it
looks, which closes the window between enumerating and subscribing::

    1. create a udev monitor and enable it
    2. enumerate the existing character devices whose uevent has CTLV_FAMILY
    3. drain the events already queued on the monitor, keeping those with it
    4. deduplicate by sysfs path
    5. open the device node
    6. ask family-get on the open descriptor for the authoritative chain

Steps 1 to 4 are pre-open discovery, all a udev rule can match on; step 6 does
not need a udev database. A rule matches a family through the compatibility
properties::

    # /etc/udev/rules.d/70-ctlv.rules
    #
    # Anything of one family, whatever family implementation registered it.
    ENV{CTLV_FAMILY}=="ctlv-accelerator-foo", GROUP="accel", MODE="0660"

    # Anything descended from a family.
    ENV{CTLV_COMPATIBLE_1}=="ctlv-accelerator", TAG+="uaccess"

    # A stable name for a family with one device per system.
    ENV{CTLV_FAMILY}=="ctlv-example", SYMLINK+="ctlv-example"

    # A stable name per parent device, for a family with several devices.
    ENV{CTLV_FAMILY}=="ctlv-accelerator-foo", IMPORT{builtin}="path_id"
    ENV{CTLV_FAMILY}=="ctlv-accelerator-foo", ENV{ID_PATH}=="?*", \
        SYMLINK+="accel/by-path/$env{ID_PATH}"

A device whose uevent leaves the family out has a file a program opens by the
device's own name, and the open descriptor answers ``family-get`` all the
same.

CTLV does not standardise such aliases and does not promise that a generic
identifier survives a reboot or a reprobe; a family with a meaningful identity
exposes it through an operation of its own.

Message format
==============

A message is a tree of TLVs. Every TLV begins with the same header, at an
eight-byte boundary::

    struct ctlv_hdr {
            __u32 len;              /* header + payload, without padding */
            __u32 flags;            /* CTLV_HDR_F_* */
            __aligned_u64 type;     /* family, record, field; 0 for a root */
    };

One unsigned integer TLV with an eight-byte payload, in a message::

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +---------------------------------------------------------------+
    |                        len = 24                               |
    +---------------------------------------------------------------+
    |                        flags = 0                              |
    +---------------------------------------------------------------+
    |                                                               |
    +                       type                                    +
    |                                                               |
    +---------------------------------------------------------------+
    |                                                               |
    +                       payload                                 +
    |                                                               |
    +---------------------------------------------------------------+

The rules that make a tree out of that:

- ``CTLV_ALIGNTO`` is 8 and ``CTLV_ALIGN()`` rounds up to it. Every TLV
  starts at a multiple of eight.
- ``len`` counts this header plus this TLV's payload, not its own trailing
  padding.
- A record's payload is the concatenation of ``CTLV_ALIGN(child.len)`` bytes
  per child, so a child's padding is counted inside its parent's ``len`` and
  a leaf's own padding by nobody.
- ``CTLV_HDR_F_RECORD`` is set exactly on a record, the root included.
- A message begins with one root, whose ``type`` is ``CTLV_TYPE_ROOT``
  (zero) and whose payload is the message. A root never appears inside a
  record, and a record never carries the root type.
- Scalar payloads are native-endian and their lengths exact: eight bytes for
  an integer, one byte holding zero or one for a boolean, the string including
  its NUL terminator, the declared length for binary data.
- A message goes no deeper than its operation's schema, the root being level
  zero.

A whole message with every offset, for a request of one integer and one record
carrying a string and a further record::

    offset  TLV                             len  flags   contents
    ------  ------------------------------  ---  ------  ----------------
         0  root                            120  RECORD  type = 0
        16  .. chunks                        24  -       8-byte integer
        40  .. chunk                         80  RECORD
        56     .. label                      19  -       "ab" and its NUL
        80     .. item                       40  RECORD
        96        .. value                   24  -       8-byte integer
       120  end of message

    +----------------------------------------------------------------------+ 0
    | root: len = 120, flags = RECORD, type = 0                            |
    |   +----------------------------------------------------------------+ | 16
    |   | chunks: len = 24 | payload: 8 bytes                            | |
    |   +----------------------------------------------------------------+ | 40
    |   | chunk: len = 80, flags = RECORD                                | |
    |   |   +----------------------------------------------------------+ | | 56
    |   |   | label: len = 19 | payload: 'a' 'b' 0 | 5 bytes padding   | | |
    |   |   +----------------------------------------------------------+ | | 80
    |   |   | item: len = 40, flags = RECORD                           | | |
    |   |   |   +----------------------------------------------------+ | | | 96
    |   |   |   | value: len = 24 | payload: 8 bytes                 | | | |
    |   |   |   +----------------------------------------------------+ | | | 120
    |   |   +----------------------------------------------------------+ | |
    |   +----------------------------------------------------------------+ |
    +----------------------------------------------------------------------+

``label.len`` is 19: its header and three payload bytes, not the five bytes of
padding after it. ``chunk`` counts those five, its ``len`` of 80 being its
header plus ``CTLV_ALIGN(19)`` plus ``CTLV_ALIGN(40)``. A walk over a record's
children advances by ``CTLV_ALIGN(child.len)``, a walk of one payload by
``child.len``.

A type names the family that declared it, a record within that family and a
field within that record::

     63      48 47 46 45     32 31 28 27 24 23 20 19 16 15        0
    +----------+-----+---------+-----+-----+-----+-----+-----------+
    |  family  |kind |  index  | L1  | L2  | L3  | L4  |   field   |
    +----------+-----+---------+-----+-----+-----+-----+-----------+
               |<--------------- record -------------->|

    CTLV_TYPE(family, record, field)
    CTLV_TYPE_FAMILY(type) / CTLV_TYPE_KIND(type) / CTLV_TYPE_RECORD(type) /
    CTLV_TYPE_FIELD(type)

A record's type kind, ``enum ctlv_type_kind``, says what it starts from: a
named record of the family (``CTLV_TYPE_KIND_NAMED``), an op's request
(``_REQUEST``), or an op's reply or an event's payload (``_REPLY``). The index
is the named record's place among the family's records, or the op's number. A
record declared inside one of those adds a path, one hex digit a level for up
to four levels, saying which it is among the records declared beside it,
from 1; zero ends the path. The field is its place in its record, from 1. A
named record has the same types in every message that carries it.

If ``ctlv-example`` were family 7, ``label`` in an item of the reply of
``item-get``, its op 2, would be ``0x0007c00210000002``. Read in blocks of
four hex digits, ``0007`` is the family, ``c002`` op 2's reply, ``1000`` the
path to ``items``, the reply's first record, and ``0002`` the field, the
second of an item.

An operation ID is the family and the op's number, with the record bits
zero::

    CTLV_OP(family, op)
    CTLV_OP_FAMILY(op) / CTLV_OP_NUMBER(op)

Use the generated types and op IDs. A family's header defines every type it
declares and includes its parent's.

TLV order
---------

TLVs may come in any order, with one rule: within the root and within each
record of a request, all TLVs of one type, such as the elements of an array,
must be next to each other. ``A A B`` and ``B A A`` are valid; ``A B A`` is
not. An unknown TLV the kernel skips (see the next section) separates two of
them too, so ``A X A`` is invalid even when ``X`` is skipped.

TLVs an older kernel may skip
-----------------------------

A program built against a newer version of a family may want to send a field
that an older kernel does not know. Normally an unknown type fails the
request. Setting ``CTLV_HDR_F_IGNORE_IF_UNKNOWN`` in a TLV's header asks the
kernel to skip it instead, together with everything in it.

The flag only matters for a type that none of the device's families declares.
A type the device knows is validated as usual, flag or not: it fails the
request where the schema does not place it, and is checked against its bounds
where it does. A skipped TLV still has to be well formed: its length must fit
inside its record, and its header may carry only defined flags. The root cannot
carry the flag, and replies and events never do.

So the kernel acts on what it knows and ignores the rest. A caller that needs
to know which of its fields will be acted on asks ``op-schema-get`` first.
``libctlv`` sets the flag with ``ctlv_msg_ignore_if_unknown()`` on the TLV
written last.

Executing an operation
======================

One ioctl executes every operation::

    struct ctlv_op_exec {
            __u32 size;                 /* sizeof(struct ctlv_op_exec) */
            __u32 flags;                /* must be zero */
            __aligned_u64 op_id;
            __aligned_u64 request;      /* struct ctlv_hdr __user * */
            __aligned_u64 reply;        /* struct ctlv_hdr __user * */
            __u32 request_len;
            __u32 reply_len;
            __aligned_u64 generation;   /* queries only */
            __aligned_u64 cursor;       /* queries only */
    };

    #define CTLV_IOC_OP_EXEC _IO(0xBB, 0x00)

``size`` is the size of the structure the caller was built with, at least the
56 bytes of this first version. The structure only grows at its end, and the
kernel reads ``size`` bytes: a field past the end of an older caller's
structure reads as zero, and bytes a newer caller sends past the fields this
kernel knows must be zero. The kernel writes back only within ``size``.

``request_len`` is how many bytes the kernel may read at ``request`` and
``reply_len`` how many it may write at ``reply``. They are copy boundaries, not
message lengths: a root's ``len`` may be shorter than ``request_len``, the
bytes after it being ignored, and may not be longer. ``reply_len`` is never
overwritten; the length written is the reply root's ``len``. It may be any
length: a query's fragment stops at a limit of the kernel's own, leaving the
rest of a longer buffer unused. The pointers are fixed-width so a 32-bit and a
64-bit process pass the same structure.

Every action and query carries a request root, even where the request is
empty, so ``request_len`` is at least one header. The
``request-max-len`` reported by ``op-get`` is the maximum request composed
from the known schema; a newer caller may exceed it with well-formed unknown
TLVs marked ``IGNORE_IF_UNKNOWN``. The kernel refuses a request longer than a
limit of its own, which no request composed from a schema exceeds.

The three op types differ in what they answer:

action
    Executed once and acknowledged. ``reply_len`` is at least the
    ``action-reply-max-len`` in ``op-get``, zero for an action that answers
    nothing.

query
    Executed once and answered in as many fragments as it needs, using
    ``generation`` and ``cursor``. ``reply_len`` is at least the
    ``query-reply-min-len`` in ``op-get``, which also suggests a length as
    ``query-reply-suggested-len``.

event
    Not executed. A device produces events and a subscribed file description
    reads them.

Errors
------

A failed operation returns a negative errno. Where the failure is a rule the
message broke and the caller offered a reply buffer, CTLV also writes
error information there in TLV form: a ``validation-reason`` value, a
human-readable message, the offset of the offending TLV, the offset of the
record it was in, the type of a missing field, the length an output blob
needs, and for a stale query the expected and the current generation. A
buffer of the operation's ``error-info-max-len`` in ``op-get`` has room for
it, and one too small leaves it unwritten; the errno is authoritative either
way.

What the ioctls answer with, as against a family's own callback:

``ENOTTY``
    Neither ``CTLV_IOC_OP_EXEC`` nor ``CTLV_IOC_EVENT_FD``, on a file only CTLV
    answers. Where the file is also another subsystem's, that subsystem
    answers any other ioctl.

``EINVAL``
    ``size`` was below 56, ``flags`` was not zero, ``op_id`` was zero,
    ``generation`` or ``cursor`` was set on something that is not a query, or
    the message broke a rule of the format or of the schema.

``E2BIG``
    ``size`` was larger than the structure this kernel knows, and the bytes
    past it were not zero.

``EFAULT``
    The structure, the request or the reply was not accessible.

``EOVERFLOW``
    A blob descriptor's address or offset plus its length does not fit in 64
    bits.

``EOPNOTSUPP``
    The device does not publish the operation, publishes it but has it
    disabled, or the operation is an event, which is read rather than executed.

``EACCES``
    The fd lacks the operation's declared authority.

``EMSGSIZE``
    ``reply_len`` is shorter than the operation's ``action-reply-max-len`` or
    ``query-reply-min-len``, and nothing was executed; or an ``output`` blob
    is too short, which the error information reports with
    ``required-length``; see `Blobs`_.

``ESTALE``
    A continuation whose ``generation`` no longer matches. Start the query
    again.

``ENODEV``
    The device was unregistered. Descriptors stay open and every operation on
    them fails this way.

``EBADF``, ``EIO``
    A blob descriptor named something that is not a file, or the file refused
    the transfer.

``ENOMEM``
    The kernel could not allocate what the operation needed.

``open()`` answers ``EACCES`` for ``O_WRONLY``, which no operation accepts,
``ENODEV`` once the device has been unregistered, and ``ENOMEM`` when the
kernel could not allocate the open file.

``read()`` answers ``EAGAIN`` when no event is queued on a non-blocking
description, ``EINTR`` when a blocking read was interrupted, ``EMSGSIZE`` when
the buffer cannot hold the event at the head of the queue, and ``ENODEV``
once a removed device's queue has been drained.

Queries and fragments
=====================

A query's answer may be larger than the buffer the caller wants to commit, so
the kernel fills the buffer with as much of it as fits where the tree can be
cut. The first call passes ``generation`` and ``cursor`` zero. On return:

- the kernel puts the query's generation in ``generation``;
- ``cursor`` is nonzero when there is more to come, zero when the answer is
  complete;
- the reply root carries ``CTLV_HDR_F_FRAG_MORE`` when the answer continues,
  and so does every record still logically open at the cut.

To continue, the caller repeats the call with the same ``op_id`` and request,
the returned ``cursor`` and the returned ``generation``, and reassembles by
concatenating the children of each still-open record, which ``libctlv`` does.

One answer of three items, cut by a buffer that holds two::

    call 1: cursor in = 0             call 2: cursor in = c
            cursor out = c                    cursor out = 0

    +----------------------------+    +----------------------------+
    | root: FRAG_MORE            |    | root: (no FRAG_MORE)       |
    |  +-----------------------+ |    |  +-----------------------+ |
    |  | chunk: FRAG_MORE      | |    |  | chunk:                | |
    |  |  +------------------+ | |    |  |  +------------------+ | |
    |  |  | item: value = 1  | | |    |  |  | item: value = 3  | | |
    |  |  +------------------+ | |    |  |  +------------------+ | |
    |  |  | item: value = 2  | | |    |  +-----------------------+ |
    |  |  +------------------+ | |    +----------------------------+
    |  +-----------------------+ |
    +----------------------------+

    reassembled: root -> chunk -> item(1), item(2), item(3)

``FRAG_MORE`` on the root says the answer continues; on ``chunk`` it says the
next fragment resumes inside that same chunk. A complete record does not carry
it, so a reader knows whether to append or to open a new record beside it.

Every continuing fragment adds at least one complete logical TLV beyond its
repeated path, and a cursor value cannot recur within one logical reply.
``libctlv`` rejects either violation. It also limits one query to 65,536
fragments and a 64 MiB reassembled reply.

A continuation whose generation no longer matches fails with ``-ESTALE``, and
the error information carries the expected and the current generation.

Events
======

An event reaches a file description that asked for it, and only that one:

1. subscribe with the ``subscriptions-add`` action, naming operation IDs;
2. read events with ``read()``, one complete event per call;
3. unsubscribe with ``subscriptions-remove``.

``poll()`` reports readability when an event is queued. A file whose ``read()``
and ``poll()`` are another subsystem's has its queue read from a descriptor of
its own::

    #define CTLV_IOC_EVENT_FD _IO(0xBB, 0x01)

The argument is zero, and the ioctl returns a new ``O_RDONLY | O_CLOEXEC``
descriptor whose ``read()`` and ``poll()`` serve the queue of the file
description it was asked on: an event read through either is gone from both.
It keeps that description open, so its events keep arriving after the
descriptor the ioctl was asked on is closed. It executes nothing and takes
``O_NONBLOCK`` through ``fcntl()``. Every device answers the ioctl, whichever
subsystem it is in, and still does once unregistered, so that the rest of its
queue and the event of its removal can be read.

Each event is a header followed by exactly one root::

    struct ctlv_event_hdr {
            __u32 length;             /* header + aligned root */
            __u32 flags;              /* zero */
            __aligned_u64 op_id;      /* the event op that produced it */
            __aligned_u64 sequence;   /* per file description */
    };

One ``read()`` returns one event, the header and the aligned root::

    +---------------------------------------------------------------+ 0
    | length = 24 + CTLV_ALIGN(root.len)     | flags = 0            |
    +---------------------------------------------------------------+ 8
    | op_id: the event operation that produced this event           |
    +---------------------------------------------------------------+ 16
    | sequence: counted per file description                        |
    +---------------------------------------------------------------+ 24
    | root: len, flags = RECORD, type = 0                           |
    |   the event's TLVs, exactly as a reply carries them           |
    +---------------------------------------------------------------+ length

A buffer shorter than ``length`` reads nothing and fails with ``-EMSGSIZE``. An
event is one message, no longer than the ``event-max-len`` of its operation in
``op-get``. A sequence number is assigned to every eligible event, dropped
ones included, so a gap in the sequence is exactly what was lost.

The queue is per file description: ``event-queue-get`` reports its length, how
much is in use, and the shortest and longest length the device takes, which
``event-queue-set`` changes it within. An event that does not fit is dropped,
and the next read returns an ``event-overflow`` event carrying the first and
last sequence numbers lost and how many events that was.

Two events come from the root family:

op-changed
    An operation with dynamic availability was enabled or disabled. The event
    carries the operation ID, the new state, and the state generation that
    ``op-get`` also reports, so a client can order dump against event.

device-removed
    The device is gone. Operations fail from then on and the descriptor stays
    valid until it is closed.

Blobs
=====

A ``binary`` field carries its bytes in the message itself, within the
length its family declares. An operation that moves more data than a message
should carry declares a blob field instead, and the caller passes a
descriptor rather than the data: the root family's ``blob`` record, declared
in ``Documentation/ctlv/specs/ctlv.yaml`` and numbered in
``include/uapi/ctlv/ctlv_gen.h``. ``blob.type`` names the backing and
``blob.length`` the length in bytes; the backing says which of
``blob.address``, ``blob.fd`` and ``blob.offset`` name the memory. Three
backings are defined:

``va``
    Anonymous userspace memory of the calling process, named by an address in
    ``blob.address``.

``memfd``
    A memfd, named by a file descriptor in ``blob.fd``, at ``blob.offset``.

``dma-buf``
    An exported dma-buf, named by a file descriptor in ``blob.fd``, at
    ``blob.offset``.

The backings a field accepts, the direction data may flow, the maximum
length and the required alignment are declared by the family and reported by
``op-schema-get``. Memory is acquired for the operation and released before it
returns; a descriptor the operation may not use is refused before the callback.
While the operation is in flight, userspace must not modify an ``input`` blob
or access an ``output`` or ``inout`` blob. Whether two descriptors overlap is
also the caller's responsibility. Keep every named VA mapping and file
descriptor valid until the operation returns; the generated library does this
for descriptors it builds.

A descriptor is checked with the rest of the request, before any memory is
acquired. A malformed field fails like any other. A backing the blob field
does not accept, fields that do not fit the backing, a zero length
or one above the maximum, or a misaligned address or offset fails with
``EINVAL``, and a range that does not fit in 64 bits with ``EOVERFLOW``; both
report ``bad-blob-descriptor`` at the descriptor. The memory is acquired when
the operation first uses it, and a failure there, such as ``EBADF`` or
``ENOMEM``, reports the same reason at the same place. An operation that
finds an ``output`` blob too short for what it has to write there fails with
``EMSGSIZE``, and reports the same with ``required-length``, how long the blob
has to be, so the caller can retry with one that long.

Breaking the in-flight rules harms only the caller. The kernel copies blob
data or has a device DMA it, so what such an operation reads or writes is
unspecified and the operation may fail, but no kernel memory is at risk.

A memfd always needs ``F_SEAL_SHRINK``. For ``output`` or ``inout`` it also
needs ``F_SEAL_SEAL`` and neither write seal, so it remains writable for the
whole operation.

Introspection
=============

Three queries describe a device to a program that was not compiled against it.
The CTLV core answers all three on every device.

``family-get``
    The device's family chain, root first, as a numeric ID and a name per
    entry. The authoritative answer to what the device is.

``op-get``
    One entry per published operation: its ID, op type, required fd access,
    whether its availability is fixed or dynamic, whether it is currently
    disabled, and those of its lengths on this device that apply to its op
    type: ``request-max-len``, ``action-reply-max-len``,
    ``query-reply-min-len``, ``query-reply-suggested-len``, ``event-max-len``
    and ``error-info-max-len``. An unpublished operation has no entry and is
    indistinguishable from one no family ever declared. Event entries have no
    fd-access field.

``op-schema-get``
    The whole tree of one operation: every field of the request and the
    reply, with its type, its schema kind, its bounds, its occurrence limit,
    and its unit and scale where it has them.

A schema that describes a schema does not end, so it is reported through
re-entry: a node whose fields are those of a level already described carries
``schema-node.reenters``, saying how many levels up that level is. A consumer
that follows ``reenters`` handles any depth.

The lengths in ``op-get`` include whatever a descendant family added to an
inherited operation, so ask rather than compute: a family's generated maximum
is right for its own devices and a lower bound on a descendant's. Likewise, an
enum extension point's bounds are the values the device's family chain gives
it, from 1 to 0 when it gives none, and their names are in the uapi header of
the family that gives them.

A program compiled against a family needs only ``family-get``, once, when
it opens the device. The chain runs from ``ctlv`` to the device's family, and
a HW driver usually has a family of its own, such as
``ctlv-accelerator-foo``, so the last entry also says whose fields the replies
carry. The answer comes from the open descriptor, so it works where udev never
ran, such as inside a container; the uevent variables under Discovery are for
udev rules. A field a descendant family adds to an inherited operation also
carries that family's ID, in bits 63..48 of its type.

Compatibility
=============

CTLV has no versions. A program built against one kernel keeps working on a
later one, and a later program finds out what an earlier kernel acts on, by
these rules:

- ``struct ctlv_op_exec`` carries its size and grows only at its end; see
  `Executing an operation`_.
- A family grows by new operations and new optional fields. A field's type
  includes its place in its record, so a new field goes last, and the others
  keep their types.
- A reply or an event may carry TLVs an older program does not know, and it
  skips them. A request TLV the kernel does not know fails the request unless
  the program marks it ``CTLV_HDR_F_IGNORE_IF_UNKNOWN``, and ``op-schema-get``
  tells a newer program which of its fields the kernel acts on; see
  `TLVs an older kernel may skip`_.
- Anything incompatible is a new operation.

Each family has a checked-in ABI snapshot under ``Documentation/ctlv/abi``, so
a change to its uAPI shows as a change to that file. ``make -C tools/ctlv
regen`` rewrites the snapshot from the family's specification, and refuses
removing or renumbering anything, changing a field's kind or an operation's
type, access or availability, narrowing a bound or an occurrence limit,
making a field that the kernel writes optional, and, in any message,
making a field required or adding a required one.

Library and tools
=================

``tools/ctlv/libctlv`` is a small C library for the mechanical parts: opening
a device, checking the chain, building requests, parsing replies, reassembling
fragmented queries, reading events and building blob descriptors. Per-family
bindings are generated beside it, turning operations into typed functions.
``ctlv_open()`` finds a device by the family its uevent announces, whichever
subsystem it is in. ``ctlv_open_fd()`` takes a file the caller opened, through
another subsystem's interface if need be, and ``ctlv_close()`` leaves it open.
Events are read through an event fd either way.

``tools/ctlv/ctlvctl`` defaults to runtime introspection alone. Compiled
against no family, it can list devices, print the operations one publishes and
their schemas, execute its actions and queries alike and stream its events. A
device is named as ``list`` prints it, relative to ``/dev``, or by a path::

    ctlvctl list
    ctlvctl ops /dev/ctlv-example
    ctlvctl schema /dev/ctlv-example 0x0001000000000008
    ctlvctl exec /dev/ctlv-example 0x0001000000000001
    ctlvctl exec /dev/ctlv-example 0x0001000000000002 0x0001800200000001=7
    ctlvctl monitor /dev/ctlv-example 0x000100000000000a

Passing the directory of the kernel's family specifications adds the names
that are deliberately absent from introspection. This is the ``specs``
directory of a CTLV documentation bundle; its registry and meta-schemas are in
the parent directory::

    ctlvctl --spec-dir Documentation/ctlv/specs ops /dev/ctlv-example
    ctlvctl --spec-dir Documentation/ctlv/specs \
            schema /dev/ctlv-example series-get
    ctlvctl --spec-dir Documentation/ctlv/specs \
            exec /dev/ctlv-example probe-get token=7
    ctlvctl --spec-dir Documentation/ctlv/specs \
            exec /dev/ctlv-example echo-reply \
            --json '{"u":17,"mode":"busy","count":[3,5]}'
    ctlvctl --spec-dir Documentation/ctlv/specs \
            exec /dev/ctlv-example transfer --json \
            '{"source":{"file":"in"},"target":{"file":"out","length":64}}'

``CTLV_SPEC_DIR`` avoids repeating the directory. Known items are printed by
name without IDs; ``-n`` adds them, while an item unknown to the selected
specifications stays numeric. ``schema --json-template`` prints a minimal JSON
request. ``--output-json`` emits one object for an action or query and one JSON
object per line while monitoring. A JSON request may be inline, read from
``@file``, or read from standard input with ``--json -``; it cannot be combined
with assignments. Specification and JSON support are built when json-c is
available.

In JSON, a blob is ``{"file": "<path>"}``, passed as ``va`` memory. An input
blob holds the file. An output blob is written to the file whole, so give it
the ``"length"`` the operation writes; without one it starts at a byte and
grows to each ``required-length`` the operation fails with, the operation
being retried. An ``inout`` blob is read from the file and written back.

In the directory, ``ctlvctl`` reads a family's names from ``<family>.json``
when it is there, and otherwise has ``ctlvgen``, with its Python dependencies,
resolve ``<family>.yaml``. Building ``ctlvctl`` writes the ``.json`` of each of
the tree's families beside it, so a machine with neither the source tree nor
Python needs only that directory and json-c. ``ctlvgen --spec <family>.yaml
--runtime-json`` prints the file for a family outside the tree.

``make -C tools/ctlv programs`` builds both in the source tree. After ``make
O=<dir> headers``, ``make O=<dir> tools/ctlv`` builds ``ctlvctl`` into
``<dir>/tools/ctlv/ctlvctl`` against that tree's headers, for another
architecture with ``CROSS_COMPILE`` or ``LLVM`` as the kernel takes them;
``PKG_CONFIG`` names the ``pkg-config`` that finds the target's json-c.
