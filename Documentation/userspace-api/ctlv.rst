.. SPDX-License-Identifier: GPL-2.0

==================
CTLV userspace ABI
==================

:Author: Jiri Pirko

CTLV is a transport for device control interfaces. A family describes what its
devices accept in YAML, a generator turns that into kernel metadata and UAPI
headers, and CTLV validates every message before a family callback sees it.
Userspace sees one character device per instance, one ioctl, and ``read()`` for
events.

This document is the ABI. What each family accepts is specified under
``Documentation/ctlv/specs``; writing a family is :doc:`/driver-api/ctlv`.

Why a TLV transport
===================

CTLV puts the rules of every operation in one description and validates them
once:

- One ioctl carries every operation of every family, so a new operation is a
  new specification entry, not a new ioctl number.
- Attributes are optional or required by declaration, so a message can grow
  without breaking a caller that does not know the new attribute.
- An unknown attribute is refused on a request and tolerated on a reply, so a
  new kernel can say more to an old program. A request may mark an attribute an
  old kernel is welcome to skip, so a new program can say more too.
- Bounds, lengths, occurrence counts and nesting depth are checked before a
  family callback runs.

Devices
=======

Each registered instance appears as a character device in the ``ctlv`` class
with a dynamic minor::

    /dev/ctlv0
    /sys/class/ctlv/ctlv0

Sysfs shows class membership, the ``dev`` number and the device-model link to
the parent device. There is no family sysfs attribute; ``families-dump`` on an
open descriptor is the authoritative answer to what a device is.

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
are its ancestry from the root to that family, and
``CTLV_COMPATIBLE_N`` is how many there are. An index never moves: changing a
family's parent is an ABI
break, adding a descendant only appends.

A program that wants every device, including ones appearing while it starts,
must listen before it looks, which closes the window between enumerating and
subscribing::

    1. create a udev monitor filtered to the ctlv subsystem, and enable it
    2. enumerate the existing ctlv class devices
    3. drain the events already queued on the monitor
    4. deduplicate by sysfs path
    5. open the device node
    6. ask families-dump on the open descriptor for the authoritative chain

Steps 1 to 4 are pre-open discovery, all a udev rule can match on; step 6 does
not need a udev database. A rule matches a family through the compatibility
properties::

    # /etc/udev/rules.d/70-ctlv.rules
    #
    # Anything of one family, whatever family implementation registered it.
    SUBSYSTEM=="ctlv", ENV{CTLV_FAMILY}=="ctlv-accelerator-foo", \
        GROUP="accel", MODE="0660"

    # Anything descended from a family.
    SUBSYSTEM=="ctlv", ENV{CTLV_COMPATIBLE_1}=="ctlv-accelerator", \
        TAG+="uaccess"

    # A stable name from the parent device, which is deployment policy.
    SUBSYSTEM=="ctlv", ENV{CTLV_FAMILY}=="ctlv-accelerator-foo", \
        SYMLINK+="accel/by-path/$env{ID_PATH}"

CTLV does not standardise such aliases and does not promise that a generic
identifier survives a reboot or a reprobe; a family with a meaningful identity
exposes it through an operation of its own.

Wire format
===========

A message is a tree of attributes. Every attribute begins with the same
header, at an eight-byte boundary::

    struct ctlv_attr {
            __u32 len;              /* header + payload, without padding */
            __u32 flags;            /* CTLV_ATTR_F_* */
            __aligned_u64 attr_id;  /* complete ID, or 0 for a root */
    };

One unsigned integer attribute with an eight-byte payload, on the wire::

     0                   1                   2                   3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    +---------------------------------------------------------------+
    |                        len = 24                               |
    +---------------------------------------------------------------+
    |                        flags = 0                              |
    +---------------------------------------------------------------+
    |                                                               |
    +                       attr_id                                 +
    |                                                               |
    +---------------------------------------------------------------+
    |                                                               |
    +                       payload                                 +
    |                                                               |
    +---------------------------------------------------------------+

The rules that make a tree out of that:

- ``CTLV_ALIGNTO`` is 8 and ``CTLV_ALIGN()`` rounds up to it. Every attribute
  starts at a multiple of eight.
- ``len`` counts this header plus this attribute's payload, not its own
  trailing padding.
- A nested payload is the concatenation of ``CTLV_ALIGN(child.len)`` bytes per
  child, so a child's padding is counted inside its parent's ``len`` and a
  leaf's own padding by nobody.
- ``CTLV_ATTR_F_NESTED`` is set exactly on a root and on a nest.
- A message begins with one root attribute, whose ``attr_id`` is
  ``CTLV_ATTR_ROOT`` (zero) and whose payload is the message. A root never
  appears inside a nest, and a nest never carries the root ID.
- Scalar payloads are native-endian and their lengths exact: eight bytes for
  an integer, one byte holding zero or one for a boolean, the string including
  its NUL terminator, the declared length for binary data.
- Nesting is limited to ``CTLV_MAX_NEST_DEPTH`` levels, the root being level
  zero.

A whole message with every offset, for a request of one integer and one nest
carrying a string and a further nest::

    offset  attribute                       len  flags   contents
    ------  ------------------------------  ---  ------  ----------------
         0  root                            120  NESTED  attr_id = 0
        16  .. chunks                        24  -       8-byte integer
        40  .. chunk                         80  NESTED
        56     .. label                      19  -       "ab" and its NUL
        80     .. item                       40  NESTED
        96        .. value                   24  -       8-byte integer
       120  end of message

    +--------------------------------------------------------------+ 0
    | root: len = 120, flags = NESTED, attr_id = 0                 |
    +--------------------------------------------------------------+ 16
    | chunks: len = 24 | payload: 8 bytes                          |
    +--------------------------------------------------------------+ 40
    | chunk: len = 80, flags = NESTED                              |
    |   +--------------------------------------------------------+ | 56
    |   | label: len = 19 | payload: 'a' 'b' 0 | 5 bytes padding | |
    |   +--------------------------------------------------------+ | 80
    |   | item: len = 40, flags = NESTED                         | |
    |   |   +--------------------------------------------------+ | | 96
    |   |   | value: len = 24 | payload: 8 bytes               | | |
    |   |   +--------------------------------------------------+ | | 120
    +--------------------------------------------------------------+

``label.len`` is 19: its header and three payload bytes, not the five bytes of
padding after it. ``chunk`` counts those five, its ``len`` of 80 being its
header plus ``CTLV_ALIGN(19)`` plus ``CTLV_ALIGN(40)``. A walk over a nest's
children advances by ``CTLV_ALIGN(child.len)``, a walk of one payload by
``child.len``.

An attribute ID names the family that declared it, a group within that family
and an attribute within that group::

    63           48 47           32 31                             0
    +--------------+--------------+--------------------------------+
    |    family    |    group     |             attr               |
    +--------------+--------------+--------------------------------+

    CTLV_ATTR(family, group, attr)
    CTLV_ATTR_FAMILY(attr) / CTLV_ATTR_GROUP(attr) / CTLV_ATTR_NUMBER(attr)

An operation ID is the same shape with the group bits zero::

    CTLV_OP(family, op)
    CTLV_OP_FAMILY(op) / CTLV_OP_NUMBER(op)

Use generated complete IDs. A family's header defines every ID it declares
and includes its parent's.

Request occurrence order
------------------------

Occurrences of one attribute ID must be contiguous within each request root or
nest. ``A A B`` is valid, while ``A B A`` is not. An ignored unknown attribute
is not an occurrence of any known attribute, but it ends the preceding run, so
``A ignored-unknown A`` is also invalid. Different IDs otherwise have no
required numeric or schema order.

Attributes a kernel may skip
----------------------------

A request attribute, the root aside, may carry
``CTLV_ATTR_F_IGNORE_IF_UNKNOWN``. If no family the device answers declares
that complete ID, the kernel steps over the whole attribute without looking
inside it; without the bit, an attribute the schema does not place where it
appears fails the request as it always has.

The bit forgives an ID this kernel has never heard of and nothing else. An ID
the device's schema declares somewhere is held to that schema, so an attribute
sent where it does not belong fails whether the bit is set or not, and one sent
where it does belong is checked as usual. Every other rule applies too: the
length has to fit its nest and the header may carry no bit the format does not
define. Replies and events never carry it.

This is how a program built against a newer version of a family talks to an
older kernel. What that kernel knows is validated, what it does not is
ignored, and a caller that has to know which is which asks ``op-schema-get``
first.

Executing an operation
======================

One ioctl executes every operation::

    struct ctlv_op_exec {
            __aligned_u64 op_id;
            __aligned_u64 request;      /* struct ctlv_attr __user * */
            __aligned_u64 reply;        /* struct ctlv_attr __user * */
            __u32 request_len;
            __u32 reply_len;
            __aligned_u64 generation;   /* queries only */
            __aligned_u64 cursor;       /* queries only */
            __aligned_u64 reserved[2];  /* must be zero */
    };

    #define CTLV_IOC_OP_EXEC _IOWR(0xBB, 0x00, struct ctlv_op_exec)

``request_len`` is how many bytes the kernel may read at ``request`` and
``reply_len`` how many it may write at ``reply``. They are copy boundaries, not
message lengths: a root's ``len`` may be shorter than ``request_len``, the
bytes after it being ignored, and may not be longer. ``reply_len`` is never
overwritten; the length written is the reply root's ``len``. The pointers are
fixed-width so a 32-bit and a 64-bit process pass the same structure, and in
place of a size field the ioctl number encodes the size and ``reserved``, which
must be zero, covers compatible growth.

Every action and query carries a physical request root, including an empty
request. ``request_len`` is therefore at least one attr header and at most the
1 MiB transport limit. The ``request-max-len`` reported by ``ops-dump`` is the
maximum request composed from the known schema; a newer caller may exceed it
with well-formed unknown attrs marked ``IGNORE_IF_UNKNOWN``, up to the
transport limit.

The three kinds of operation differ in what they answer:

action
    Executed once and acknowledged. An action with no reply takes
    ``reply_len`` zero; one with a bounded reply has its maximum in
    ``ops-dump``.

query
    Executed once and answered in as many fragments as it needs, using
    ``generation`` and ``cursor``.

event
    Not executed. A device produces records and a subscribed file description
    reads them.

Errors
------

A failed operation returns a negative errno. Where the failure is a rule the
message broke and the caller offered a reply buffer, CTLV also writes
error information there in TLV form: a ``validation-reason`` value, a
human-readable message, the offset of the offending attribute, the offset of
the nest it was in, the ID of a missing attribute, and for a stale query the
expected and the current generation. A buffer too small leaves it unwritten;
the errno is authoritative either way.

What the transport answers with, as against a family's own callback:

``ENOTTY``
    Not ``CTLV_IOC_OP_EXEC``; this transport has one ioctl.

``EINVAL``
    A reserved field was not zero, ``op_id`` was zero, ``generation`` or
    ``cursor`` was set on something that is not a query, or the message broke
    a rule of the format or of the schema.

``EFAULT``
    The structure, the request or the reply was not accessible.

``EOPNOTSUPP``
    The device does not publish the operation, publishes it but has it
    disabled, or the operation is an event, which is read rather than executed.

``EACCES``
    The fd lacks the operation's declared authority, or an ``O_WRONLY`` open
    was attempted.

``EMSGSIZE``
    ``reply_len`` is smaller than the smallest fragment the operation can
    answer with. Nothing was executed.

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

``read()`` answers ``EAGAIN`` when no record is queued on a non-blocking
description, ``EINTR`` when a blocking read was interrupted, ``EMSGSIZE`` when
the buffer cannot hold the record at the head of the queue, and ``ENODEV``
once a removed device's queue has been drained.

Queries and fragments
=====================

A query's answer may be larger than the buffer the caller wants to commit, so
the kernel fills the buffer with as much of it as fits where the tree can be
cut. The first call passes ``generation`` and ``cursor`` zero. On return:

- the kernel puts the query's generation in ``generation``;
- ``cursor`` is nonzero when there is more to come, zero when the answer is
  complete;
- the reply root carries ``CTLV_ATTR_F_FRAG_MORE`` when the answer continues,
  and so does every nest still logically open at the cut.

To continue, the caller repeats the call with the same ``op_id`` and request,
the returned ``cursor`` and the returned ``generation``, and reassembles by
concatenating the children of each still-open nest.

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
next fragment resumes inside that same chunk. A complete nest does not carry
it, so a reader knows whether to append or to open a new nest beside it.

Every continuing fragment adds at least one complete logical attr beyond its
repeated path, and a cursor value cannot recur within one logical reply.
A reader must reject either violation.

A continuation whose generation no longer matches fails with ``-ESTALE``, and
the error information carries the expected and the current generation.

Events
======

An event reaches a file description that asked for it, and only that one:

1. subscribe with the ``subscriptions-add`` action, naming operation IDs;
2. read records with ``read()``, one complete record per call;
3. unsubscribe with ``subscriptions-remove``.

``poll()`` reports readability when a record is queued. Each record is a
header followed by exactly one root attribute::

    struct ctlv_event_hdr {
            __u32 length;             /* header + aligned root */
            __u32 flags;              /* zero */
            __aligned_u64 op_id;      /* the event op that produced it */
            __aligned_u64 sequence;   /* per file description */
    };

One ``read()`` returns one record, the header and the aligned root::

    +---------------------------------------------------------------+ 0
    | length = 24 + CTLV_ALIGN(root.len)     | flags = 0            |
    +---------------------------------------------------------------+ 8
    | op_id: the event operation that produced this record          |
    +---------------------------------------------------------------+ 16
    | sequence: counted per file description                        |
    +---------------------------------------------------------------+ 24
    | root: len, flags = NESTED, attr_id = 0                        |
    |   the event's attributes, exactly as a reply carries them      |
    +---------------------------------------------------------------+ length

A buffer shorter than ``length`` reads nothing and fails with ``-EMSGSIZE``. A
record is one message, bounded by ``CTLV_MAX_INLINE_MESSAGE_LEN``. A sequence
number is assigned to every eligible record, dropped ones included, so a gap in
the sequence is exactly what was lost.

The queue is per file description: ``event-queue-get`` reports its length and
how much is in use, ``event-queue-set`` changes it up to
``CTLV_MAX_EVENT_QUEUE_LEN``. A record that does not fit is dropped, and the
next read returns an ``event-overflow`` record carrying the first and last
sequence numbers lost and how many records that was.

Two events come from the root family:

op-changed
    An operation with dynamic availability was enabled or disabled. The record
    carries the operation ID, the new state, and the state generation that
    ``ops-dump`` also reports, so a client can order dump against event.

device-removed
    The device is gone. Operations fail from then on and the descriptor stays
    valid until it is closed.

Blobs
=====

An operation that moves more data than a message should carry declares a blob
attribute, and the caller passes a descriptor instead of the data: a nest
carrying the backing type, the length and the identifier of the memory. One
backing is defined:

``va``
    Anonymous userspace memory of the calling process, named by an address in
    ``blob.address``.

The backings an attribute accepts, the direction data may flow, the maximum
length and the required alignment are declared by the family and reported by
``op-schema-get``. Memory is acquired for the operation and released before it
returns; a descriptor the operation may not use is refused before the callback.
While the operation is in flight, userspace must not modify an ``input`` blob
or access an ``output`` or ``inout`` blob. Whether two descriptors overlap is
also the caller's responsibility. Keep every named VA mapping valid until the
operation returns.

Introspection
=============

Three queries describe a device to a program that was not compiled against it.
The CTLV core answers all three on every device.

``families-dump``
    The device's family chain, root first, as a numeric ID and a name per
    entry. The authoritative answer to what the device is.

``ops-dump``
    One entry per published operation: its ID, kind, required fd access,
    whether its availability is fixed or dynamic, whether it is currently
    disabled, and the message lengths that apply on this device. An
    unpublished operation has no entry and is indistinguishable from one no
    family ever declared. Event entries have no fd-access field.

``op-schema-get``
    The whole tree of one operation: every attribute of the request and the
    reply, with its payload form, its bounds, its occurrence limit, and its
    unit and scale where it has them.

A schema that describes a schema does not end, so it is reported through
re-entry: a node whose members are those of a level already described carries
``schema.reenters``, saying how many levels up that level is. A consumer that
follows ``reenters`` handles any depth.

The lengths in ``ops-dump`` include whatever a descendant family added to an
inherited operation, so ask rather than compute: a family's generated maximum
is right for its own devices and a lower bound on a descendant's.

Limits
======

Implementation limits, not encoded maxima of the wire fields, so raising one
later is a compatible change::

    CTLV_MAX_INLINE_MESSAGE_LEN   1 MiB    request roots, action replies,
                                           query fragments, event roots
    CTLV_MAX_ERROR_LEN            16 KiB   error information
    CTLV_MAX_NEST_DEPTH           32       levels, root at zero
    CTLV_DEFAULT_QUERY_REPLY_LEN  64 KiB   suggested query buffer
    CTLV_MAX_EVENT_QUEUE_LEN      16 MiB   per file description
