#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0

"""
Check that FDB notifications carry the netlink port ID of the requester in
nlmsg_pid. A listener socket subscribed to the neighbour group receives the
notifications, while separate requester sockets make the changes. The port ID
the listener sees is expected to match that of the requester socket.
"""

import socket
import struct
from collections import namedtuple

from lib.py import ksft_run, ksft_exit, ksft_eq, ksft_ne, ksft_true
from lib.py import ksft_busy_wait
from lib.py import ip, tool
from lib.py import NetNS, NetNSEnter
from lib.py import Netlink, RtnlNeighFamily

RTM_NEWNEIGH = 28
RTM_DELNEIGH = 29

NTF_SELF = 0x02
NTF_MASTER = 0x04

NUD_NOARP = 0x40
NUD_PERMANENT = 0x80

ETH_P_EXP1 = 0x88b5  # Local Experimental EtherType 1

Ntf = namedtuple("Ntf", "type portid addr")


class Env:
    """The listener socket, the two requester sockets A and B, and the
    netlink port IDs of the latter. All sockets live in the given netns."""

    @staticmethod
    def _socket(ns):
        with NetNSEnter(ns):
            rtnl = RtnlNeighFamily()
        return rtnl

    def __init__(self, ns):
        self.ns = str(ns)

        self.listener = self._socket(ns)
        self.listener.include_raw = True
        self.listener.ntf_subscribe("rtnlgrp-neigh")

        self.a = self._socket(ns)
        self.a.sock.bind((0, 0))

        self.b = self._socket(ns)
        self.b.sock.bind((0, 0))

        self.portids = {self.a: self.a.sock.getsockname()[0],
                        self.b: self.b.sock.getsockname()[0]}

        ksft_ne(self.portids[self.a], self.portids[self.b])
        ksft_ne(self.portids[self.a], 0)

    def ifindex(self, name):
        """Look up the ifindex of a device in the test netns."""
        with NetNSEnter(self.ns):
            return socket.if_nametoindex(name)

    def ntfs(self):
        """Return the bridge-family notifications received by the listener."""
        self.listener.check_ntf()
        ntfs = []
        while self.listener.async_msg_queue.qsize() > 0:
            msg = self.listener.async_msg_queue.get()
            if msg['msg']['ndm-family'] == socket.AF_BRIDGE:
                ntfs.append(Ntf(msg['raw'].nl_type, msg['raw'].nl_portid,
                                msg['msg']['lladdr']))
        return ntfs

    def check_ntfs(self, nl_type, addr, requester, count=1):
        """Expect count notifications for addr, all from the requester."""
        portids = [n.portid for n in self.ntfs()
                   if n.type == nl_type and n.addr == addr]
        ksft_eq(portids, [self.portids[requester]] * count,
                f"nlmsg_pid of {addr} notifications")


def fdb_req(ifindex, ndm_flags, attrs):
    """Build an AF_BRIDGE neighbour request with the given attributes."""
    return {'ndm-family': socket.AF_BRIDGE, 'ndm-ifindex': ifindex,
            'ndm-flags': ndm_flags, **attrs}


def test_msg(env, sock, message, message_type,
             ifindex, addr, ndm_flags, flags, count, attrs):
    """Send one request through SOCK, and expect COUNT notifications of
    MESSAGE_TYPE about ADDR carrying the port ID of SOCK."""
    # pylint: disable=too-many-arguments,too-many-positional-arguments
    req = fdb_req(ifindex, ndm_flags, attrs | {"lladdr": addr})
    sock.do(message, req, flags)
    env.check_ntfs(message_type, addr, sock, count)


def add_replace_del(env, ifname, addr, ndm_flags, *, add, rep, dlt, count=1):
    """Add an entry through socket A, replace it through B, delete through A.
    ADD, REP and DLT are the attributes of the respective requests. If REP is
    None, the replace step is skipped."""
    # pylint: disable=too-many-arguments
    ifindex = env.ifindex(ifname)

    test_msg(env, env.a, 'newneigh', RTM_NEWNEIGH, ifindex, addr, ndm_flags,
             flags=[Netlink.NLM_F_CREATE | Netlink.NLM_F_EXCL],
             count=count, attrs=add)

    if rep is not None:
        test_msg(env, env.b, 'newneigh', RTM_NEWNEIGH, ifindex, addr, ndm_flags,
                 flags=[Netlink.NLM_F_CREATE | Netlink.NLM_F_REPLACE],
                 count=count, attrs=rep)

    test_msg(env, env.a, 'delneigh', RTM_DELNEIGH, ifindex, addr, ndm_flags,
             flags=[],
             count=count, attrs=dlt)


def flush(env, ifname, addrs, add, dlt):
    """Flush all permanent entries through socket A, and check that all the
    notifications carry the port ID."""

    ifindex = env.ifindex(ifname)

    for addr in addrs:
        attrs = add | {"ndm-state": NUD_PERMANENT,
                       "lladdr": addr}
        env.a.do('newneigh', fdb_req(ifindex, NTF_SELF, attrs),
                 flags=[Netlink.NLM_F_CREATE | Netlink.NLM_F_EXCL])

    env.ntfs()

    attrs = dlt | {'ndm-state': NUD_PERMANENT,
                   'ndm-state-mask': NUD_PERMANENT}
    env.a.do('delneigh', fdb_req(ifindex, NTF_SELF, attrs),
             flags=[Netlink.NLM_F_BULK])

    ntfs = [n for n in env.ntfs() if n.type == RTM_DELNEIGH]

    ksft_true({n.addr for n in ntfs} >= set(addrs),
              "Flush did not notify about all the entries")
    ksft_eq({n.portid for n in ntfs}, {env.portids[env.a]},
            "nlmsg_pid of flush notifications")


def test_bridge() -> None:
    """Bridge port and bridge device entries: add, replace, delete."""
    with NetNS() as ns:
        env = Env(ns)

        ip("link add br0 up type bridge vlan_default_pvid 0", ns=ns)
        ip("link add d0 up type dummy", ns=ns)
        ip("link set dev d0 master br0", ns=ns)

        add_replace_del(env, "d0", "00:11:22:33:44:01", NTF_MASTER,
                        add={"ndm-state": NUD_PERMANENT},
                        rep={"ndm-state": NUD_NOARP},
                        dlt={})

        add_replace_del(env, "br0", "00:11:22:33:44:02", NTF_SELF,
                        add={"ndm-state": NUD_PERMANENT},
                        rep=None,
                        dlt={})


def test_bridge_vlan() -> None:
    """Without a VID given, an entry is added and deleted in every VLAN."""
    with NetNS() as ns:
        env = Env(ns)

        ip("link add br0 up type bridge vlan_filtering 1 vlan_default_pvid 0",
           ns=ns)
        ip("link add d0 up type dummy", ns=ns)
        ip("link set dev d0 master br0", ns=ns)

        tool("bridge", "vlan add dev d0 vid 10", ns=ns)
        tool("bridge", "vlan add dev d0 vid 20", ns=ns)

        # Three notifications: for VID 10 and 20, and for the entry
        # without VLAN.
        add_replace_del(env, "d0", "00:11:22:33:44:03", NTF_MASTER, count=3,
                        add={"ndm-state": NUD_PERMANENT},
                        rep=None,
                        dlt={})


def test_bridge_flush() -> None:
    """Bulk delete of bridge entries."""
    with NetNS() as ns:
        env = Env(ns)
        ip("link add br0 up type bridge vlan_default_pvid 0", ns=ns)
        ip("link add d0 up type dummy", ns=ns)
        ip("link set dev d0 master br0", ns=ns)

        flush(env, "br0", ["00:11:22:33:44:10",
                           "00:11:22:33:44:11",
                           "00:11:22:33:44:12"],
              add={},
              dlt={})


def test_bridge_learned() -> None:
    """Entries that are not a result of a request carry port ID of zero."""
    with NetNS() as ns:
        env = Env(ns)
        ip("link add br0 up type bridge", ns=ns)
        ip("link add v0 up type veth peer name v1", ns=ns)
        ip("link set dev v0 master br0", ns=ns)
        ip("link set dev v1 up", ns=ns)

        addr = "00:11:22:33:44:31"
        frame = (b"\xff" * 6 + bytes.fromhex(addr.replace(":", "")) +
                 struct.pack("!H", ETH_P_EXP1) + b"\0" * 46)
        with NetNSEnter(env.ns):
            tx = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
            tx.bind(("v1", 0))

        portids = []

        def learned():
            nonlocal portids
            tx.send(frame)
            portids = [n.portid for n in env.ntfs()
                       if n.type == RTM_NEWNEIGH and n.addr == addr]
            return portids != []

        # Keep sending until the entry is learned: a frame that
        # arrives while the bridge port is not yet up is dropped
        # without triggering learning. Once the entry exists, repeated
        # frames only refresh it and do not generate notifications.
        ksft_busy_wait(learned, deadline=2)

        ksft_eq(portids, [0], "nlmsg_pid of learned entry")


def test_vxlan() -> None:
    """VXLAN entries: add, replace the remote, delete."""
    with NetNS() as ns:
        env = Env(ns)
        ip("link add vx0 up type vxlan id 2000 dstport 4789", ns=ns)

        add_replace_del(env, "vx0", "00:11:22:33:44:41", NTF_SELF,
                        add={"dst": "192.0.2.1", "ndm-state": NUD_PERMANENT},
                        rep={"dst": "192.0.2.2", "ndm-state": NUD_PERMANENT},
                        dlt={"dst": "192.0.2.2"})


def test_vxlan_flush() -> None:
    """Bulk delete of VXLAN entries."""
    with NetNS() as ns:
        env = Env(ns)
        ip("link add vx0 up type vxlan id 2000 dstport 4789", ns=ns)

        flush(env, "vx0", ["00:11:22:33:44:50",
                           "00:11:22:33:44:51",
                           "00:11:22:33:44:52"],
              add={"dst": "192.0.2.1"},
              dlt={"dst": "192.0.2.1"})


def test_macvlan() -> None:
    """The notifications for macvlan, which does not notify by itself, are
    sent by the generic code in rtnetlink."""
    with NetNS() as ns:
        env = Env(ns)
        ip("link add dd up type dummy", ns=ns)
        ip("link add mv0 up link dd type macvlan mode passthru", ns=ns)

        # macvlan does not support replace.
        add_replace_del(env, "mv0", "00:11:22:33:44:61", NTF_SELF,
                        add={"ndm-state": NUD_PERMANENT},
                        rep=None,
                        dlt={})


def main() -> None:
    """Run all the tests."""
    ksft_run([
        test_bridge,
        test_bridge_vlan,
        test_bridge_flush,
        test_bridge_learned,
        test_vxlan,
        test_vxlan_flush,
        test_macvlan,
    ])
    ksft_exit()


if __name__ == "__main__":
    main()
