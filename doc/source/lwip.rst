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

   Networking is **opt-in**: it is enabled with ``CONFIG_NET`` and is *off* in
   the default ``virt64_defconfig``. The smc911x MAC is present on boards such as
   the ARM Versatile Express; QEMU's ``virt`` machine does not provide one, so a
   different NIC driver is needed to exercise the stack there.

Trying it out
=============

With ``CONFIG_NET`` enabled and a supported NIC, the ``ping`` application
exercises the stack end to end. Under QEMU, ``st.sh`` attaches a **user-mode
(slirp)** network device — QEMU itself plays DHCP, DNS and NAT, and forwards host
port ``2222`` to the guest's port ``22`` — so nothing has to be set up on the host
and no ``sudo`` is needed. The trade-off is that the guest is NAT'd and not
reachable from the LAN. (``scripts/qemu-ifup.sh`` / ``qemu-ifdown.sh`` are
leftovers from the earlier ``tap``-and-bridge setup and are no longer used.)
