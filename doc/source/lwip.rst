.. _lwip:

lwIP — Lightweight IP
#####################

`lwIP <https://savannah.nongnu.org/projects/lwip>`__ is a small, independent
implementation of the TCP/IP protocol suite, designed for embedded systems with
limited resources.

Integration of lwIP in SO3
==========================

The port of lwIP to SO3 was originally done by Julien Quartier as a Diploma work.
The stack lives under ``net/lwip/`` and provides TCP, UDP, IPv4/IPv6 and a DNS
resolver.

SO3 exposes a BSD-style **socket API** to user space: the ``net/`` glue maps VFS
file descriptors onto lwIP sockets so that ``socket()``, ``bind()``,
``connect()``, ``send()`` and ``recv()`` work from applications. The network
interface driver lives in ``devices/net/`` — currently an **smc911x**
(``smsc,smc911x``) MAC. See the :ref:`networking section <kernel>` of the kernel
chapter.

.. note::

   Networking is **opt-in**: ``CONFIG_NET`` plus ``CONFIG_SMC911X``. Both are on
   in ``virt64_defconfig`` and ``virt64_fb_defconfig``, and off everywhere else —
   the other platforms have no driver for their own MAC yet.

The NIC on QEMU virt
====================

QEMU's stock ``virt`` machine offers only virtio-net, which SO3 has no driver
for. The so3 QEMU patch therefore adds an **SMSC LAN9118** to the machine model,
next to the PL111/PL050 devices it already adds:

- MMIO base ``0x08804000`` (``VIRT_ETH`` in the ``hw/arm/virt.c`` memmap,
  ``ethernet@08804000`` in ``dts/virt{32,64}.dts``);
- interrupt SPI 15 (``irqmap[VIRT_ETH]``, and ``interrupts = <0 15 4>`` in the
  same DT nodes).

Two details are worth knowing. The MAC is **not** at the ``0x1a000000`` the SO3
device trees inherited from vexpress-a15: that address is inside the PCIe MMIO
window, whose gpex alias covers the whole range and shadows anything mapped
underneath, so the guest read back nothing and never detected the chip. And it
is only instantiated when the command line fills the legacy on-board NIC slot
(``-nic``/``-net``); without it the machine is exactly as it was and
``smc911x_detect_chip()`` simply fails.

Trying it out
=============

``ping`` exercises the stack end to end::

   / % ping -c 6 10.0.2.2
   64 bytes from 10.0.2.2: icmp_seq=1 ttl=255 time=6.497070 ms
   64 bytes from 10.0.2.2: icmp_seq=2 ttl=255 time=0.601074 ms
   ...
   6 packets transmitted, 6 received, 0.000000% packet loss

``st.sh`` attaches two NICs on **user-mode (slirp)** networking — QEMU itself
plays DHCP, DNS and NAT, so nothing has to be set up on the host and no ``sudo``
is needed. The LAN9118 (``-nic user,model=lan9118``) is the one SO3 drives; the
virtio-net device is there for the Linux agency of the AVZ boot chain, and
carries the host port ``2222`` → guest port ``22`` forward. Each guest ignores
the NIC it cannot drive. The trade-off of slirp is that the guest is NAT'd and
not reachable from the LAN. (``scripts/qemu-ifup.sh`` / ``qemu-ifdown.sh`` are
leftovers from the earlier ``tap``-and-bridge setup and are no longer used.)

The interface comes up by DHCP during ``netif_add()``, so the address is
announced on the console shortly after boot::

   Network Interface Controller (NIC) found LAN9118
   smc911x: detected LAN9118 controller
   IP Network up and running with address 10.0.2.15

Pinging past the slirp gateway (``10.0.2.2``) needs the **host** to let QEMU
open ICMP sockets, otherwise slirp emulates the echo over UDP and relays back
the port-unreachable it gets::

   From 192.168.1.1 icmp_seq=1 Destination Port Unreachable

The kernel decides that with ``net.ipv4.ping_group_range``, whose default on
several distributions is the empty range ``1 0``. Widening it (``sysctl -w
net.ipv4.ping_group_range="0 2147483647"``) lets slirp forward the echo for
real. Nothing in SO3 is involved either way.

.. note::

   The **RX Status Level** the driver programs into ``FIFO_INT`` must be 0: the
   controller raises the RSFL interrupt when the RX status FIFO holds *more*
   entries than that level, so a level of 1 only interrupts once a second frame
   has arrived, and every reception stays one frame behind.
