#!/usr/bin/python3
from subprocess import PIPE, Popen
import json
import random
import re
import time
import collections

#
# Test port split configuration in terms of speed and influence on adjacent
# interfaces.
# Check the machine ability to work with maximum capacity.
#
# First, check that all the ports which their maximum supported lanes is 1,
# fails to split.
# Second, for the ports that support more then 1 lane, check that split passes
# and the sub ports are 1 lane (i.e. split to maximum) while considering
# whether split causes blocking of ports or not.
# Then, check also that split into $k which is smaller then the max passes too.
# In that case, shouldn't check split to maximum.
#


class SPC:
    max_capacity = 64


class SPC2:
    max_capacity = 128


Port = collections.namedtuple('Port', 'bus_info name')


def run_command(cmd, should_fail=False):
    """
    Run a command in subprocess.
    Return: Tuple of (stdout, stderr).
    """

    p = Popen(cmd, stdout=PIPE, stderr=PIPE, shell=True)
    stdout, stderr = p.communicate()
    if stderr.decode() != "" and not should_fail:
        print("Error sending command: %s" % cmd)
        print(stdout.decode())
        print(stderr.decode())
    return stdout.decode(), stderr.decode()


class switch_config(object):
    """
    Class that hold information on the switch, required to the tests;
    asic_type: an object describing features of an ASIC.
               E.g. an instance of the class SPC.
    max_lanes_dict: includes the max number of lanes supported per
                interface.
    """

    def get_if_names(self):
        """
        Get a list of interfaces in the switch, using devlink.
        Return: Array of tuples (bus_info/port, if_name).
        """

        arr = []

        cmd = "devlink -j port show"
        stdout, stderr = run_command(cmd)
        assert stderr == ""
        ports = json.loads(stdout)['port']

        for port in ports:
            if ports[port]['flavour'] == 'physical':
                arr.append(Port(bus_info=port, name=ports[port]['netdev']))

        return arr

    def get_asic_type(self):
        """
        Get ASIC type from the switch using lspci command output.
        Return: ASIC type: SPC, SPC2.
        TBD: Adjust to upstream.
        """

        chip_types = {"cb84": SPC, "cf6c": SPC2}
        VENDOR_ID = "15b3"

        cmd = "lspci -n | grep %s:" % VENDOR_ID
        stdout, stderr = run_command(cmd)
        assert stderr == ""

        dev_id = stdout.split(":")[-1]
        dev_id = dev_id.strip()

        if dev_id in chip_types:
            return chip_types[dev_id]()

        raise NotImplementedError("Chip type not supported")

    def create_max_lanes_dict(self):
        """
        Create a dict which includes the max number of lanes supported per
        interface.
        Return: Dict: keys = supported lane
                      values = Interfaces with this number of lanes as the
                      maximum supported for it.
                      An interface can be presented only in one key.
        For example: if swp1 supports 1,2,4 lanes it will be shown only in 4.
        """

        max_lanes_dict = {1: [], 2: [], 4: []}

        for dev in self.get_if_names():
            lane = get_max_lanes(dev.name)
            max_lanes_dict[lane].append(dev)

        return max_lanes_dict

    def __init__(self):
        """
        Initialize the object switch_config, using the get functions above.
        Return: switch_config object.
        """

        self.asic_type = self.get_asic_type()
        self.max_lanes_dict = self.create_max_lanes_dict()


def should_block(switch, k, max_lanes):
    """
    Check if the $switch can split without blocking the consecutive interface.
    Input: switch - on which the test run.
           k - the split is according to that number.
           max_lanes - the maximum lanes a port in the $switch can have.
    Return: True is so, False otherwise.
    """
    switch_max_ports = len(switch.max_lanes_dict[1]) + \
        k * len(switch.max_lanes_dict[max_lanes])

    max_capacity = switch.asic_type.max_capacity

    return switch_max_ports > max_capacity


def ethtool_section(dev, sec_name):
    stdout, stderr = run_command("ethtool %s" % dev)
    assert stderr == ""

    lines = []
    match = False
    for line in stdout.split("\n"):
        if line.startswith("\t" + sec_name + ":"):
            match = True
            line = line[len(sec_name) + 2:]
        elif match and not line.startswith("\t "):
            break
        if match:
            lines.append(line)

    return "\n".join(lines)


def get_max_lanes(dev):
    """
    Get the max number of lanes supported per port in order to build the
    dictionary, using ethtool.
    Return: 1, 2 or 4.
    """

    supported_link_modes = ethtool_section(dev, "Supported link modes")

    if "CR4" in supported_link_modes:
        return 4
    elif "CR2" in supported_link_modes:
        return 2
    else:
        return 1


def split(k, dev, should_fail=False):
    """
    Split $dev into $k ports.
    If should_fail == True, the split should fail. Otherwise, should pass.
    Return: Array of sub ports after splitting.
            If the $dev wasn't split, the array will be empty.
    """

    cmd = "devlink port split %s count %s" % (dev.bus_info, k)
    stdout, stderr = run_command(cmd, should_fail=should_fail)

    if should_fail:
        if not test(stderr != "", "%s is unsplittable" % dev.name):
            print("split an unsplittable port %s" % dev.name)
            return create_split_group(dev, k)
    else:
        if stderr == "":
            return create_split_group(dev, k)
        print("didn't split a splittable port %s" % dev.name)

    return []


def unsplit(split_arr):
    """
    Unsplit all the ports in the $split_arr, which are the port was split
    before.
    """

    for dev in split_arr:
        cmd = "devlink port unsplit %s" % dev
        stdout, stderr = run_command(cmd)
        test(stderr == "", "Unsplit port %s" % dev)


def exists(m, dev, should_exist=True):
    """
    Check if $dev exists in the switch $m.
    Return: True is so, False otherwise.
    """

    for port in m.get_if_names():
        if port.name == dev:
            return True

    return False


def exists_and_width(m, devs, lanes):
    """
    Check if every dev in the list $devs exists in the switch $m and has $lanes
    number of lanes after splitting.
    Return: True is both are True, False otherwise.
    """
    for dev in devs:
        if not exists(m, dev):
            print("dev %s doesn't exist in the switch" % dev)
            return False
        if get_max_lanes(dev) != lanes:
            print("dev %s has %d lanes, but %s were expected" % (dev, lanes))
            return False
    return True


def get_consecutive(dev):
    """
    Get consecutive interface for $dev.
    Return: If $dev is odd number, consecutive is the following interface.
            Else, consecutive is the previous interface.
    """

    # take the port number using ip link command
    cmd = "ip -d -j link show dev %s" % dev
    stdout, stderr = run_command(cmd)
    port_name = json.loads(stdout)[0]['phys_port_name']

    port_num = int(re.findall("\d+", port_name)[-1])
    consecutive = port_num

    # if even, the previous port should disappear
    if (port_num % 2) == 0:
        consecutive -= 1

    # if odd, the next port should disappear
    else:
        consecutive += 1

    return dev[:dev.rfind(str(port_num))] + str(consecutive)


def test(cond, msg):
    """
    Check $cond and print v_msg if True, or x_msg if False.
    Return: True is pass, False otherwise.
    """

    if cond:
        print("TEST: %-60s [ OK ]" % msg)
    else:
        print("TEST: %-60s [FAIL]" % msg)

    return cond


def create_split_group(dev, k):
    """
    Create the split group for $dev.
    Return: Array with $k elements, which are the split port group.
    """

    return list(dev.name + "s" + str(i) for i in range(k))


def split_unsplittable_ports(m, k):
    """
    Test splitting of unsplittable ports fails.
    """
    split_devs = []

    for dev in m.max_lanes_dict[k]:
        # split to max
        new_split_group = split(max(m.max_lanes_dict.keys()), dev,
                                should_fail=True)
        # Once the split command ends, it takes some time to the sub ifaces'
        # to get their names. 1 sec wait seem to be enough.
        time.sleep(1)

        if new_split_group != []:
            split_devs.append(dev.bus_info)

    unsplit(split_devs)


def split_splittable_ports(m, k, max_lanes):
    """
    Test splitting of splittable ports passes correctly.
    """
    split_devs = []

    for dev in m.max_lanes_dict[max_lanes]:
        new_split_group = split(k, dev)
        time.sleep(1)

        if new_split_group != []:
            split_devs.append(dev.bus_info)
            test(exists_and_width(m, new_split_group, max_lanes/k),
                 "split port %s into %s" % (dev.name, k))

    unsplit(split_devs)


def split_block_consecutive(m):
    """
    Test splitting of blocking ports.
    Check that the consecutive port doesn't exist after splitting.
    """
    split_devs = []

    # copy to a new list all the interfaces that can be split to max
    d = {lanes: list(ifs) for lanes, ifs in m.max_lanes_dict.items()
         if lanes != 1 and ifs != []}

    for v in d.values():
        random.shuffle(v)

    while d != {}:
        key = random.choice(list(d))
        dev = d[key].pop()

        consecutive = get_consecutive(dev.name)

        new_split_group = split(key, dev)
        time.sleep(1)

        if new_split_group != []:
            split_devs.append(dev.bus_info)

        # pop the consecutive from the list
        for dev in d[key]:
            if dev.name == consecutive:
                d[key].remove(dev)

        if len(d[key]) == 0:
            del d[key]

        # verify consecutive does not exist
        # check if every port in the new split group is 1 lane after split
        if not test(not exists(m, consecutive) and
                    exists_and_width(m, new_split_group, 1),
                    "split %s into %s" % (dev.name, key)):
            if exists(m, consecutive, should_exist=False):
                print("consecutive %s interface wasn't blocked after splitting"
                      % consecutive)

    unsplit(split_devs)


def main():
    m = switch_config()

    for lane in m.max_lanes_dict:
        # if 1 lane, shouldn't be able to split
        if lane == 1:
            split_unsplittable_ports(m, lane)

        # else, splitting should pass and all the split ports should exist
        # and with minimum number of lanes.
        else:
            max_lanes = lane
            while lane > 1:
                if not should_block(m, int(lane), max_lanes):
                    split_splittable_ports(m, int(lane), max_lanes)
                else:
                    split_block_consecutive(m)

                lane //= 2


if __name__ == "__main__":
    main()
