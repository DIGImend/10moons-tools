10moons-tools
=============

10moons-tools is a collection of tools for enabling and inspecting 10moons
tablets. So far it has only one tool: 10moons-probe.

Build with `autoreconf -i -f && configure && make`.

10moons-probe is a quick and dirty tool for enabling full functionality of
10moons tablets (however much there is).

Execute like this:

    sudo ./10moons-probe <BUS> <DEV>

Where `<BUS>` is a USB bus number, and `<DEV>` is a device address.

So, if your tablet was represented by this line in the `lsusb` output:

    Bus 002 Device 038: ID 08f2:6811 Gotop Information Inc.

You can run 10moons-probe like this:

    sudo ./10moons-probe 2 38
