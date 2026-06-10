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
``connect()``, ``send()`` and ``recv()`` work from applications. The underlying
network interface is a **virtio-net** device under QEMU (or an ``smc911x`` MAC on
hardware), wired into lwIP through the network driver in ``devices/net/``. See
the :ref:`networking section <kernel>` of the kernel chapter.

Trying it out
=============

The ``ping`` application exercises the stack end to end. Under QEMU the launch
scripts attach a tap network device (``scripts/qemu-ifup.sh`` /
``qemu-ifdown.sh``), so the guest can reach the host network once the tap bridge
is configured.
