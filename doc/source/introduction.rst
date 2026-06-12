.. _introduction:

Introduction to SO3
###################

SO3 (*Smart Object Oriented*) is a compact, extensible and full-featured
operating system that is particularly suited to embedded systems such as IoT
devices, real-time environments and academic development.

SO3 is the result of several years of research and development at the
`REDS <REDS_>`__ (Reconfigurable Embedded Digital Systems) Institute of
`HEIG-VD <HEIG-VD_>`__, in the field of embedded operating systems and
execution environments for ARM 32/64-bit multicore systems. SO3 was publicly
released in early 2020 (see also the `HEIG-VD newsletter <heig-vd_news_>`__).

`Prof. Daniel Rossier <DRE_>`__ initiated the development of an operating system
in 2013, in the context of a Bachelor lecture focusing on the port of operating
systems to different embedded platforms. The OS has evolved constantly and is
used in several lectures (operating systems, embedded systems); since 2018 it
became SO3 and grew into a full execution environment for connected devices —
the foundation of the `SOO framework <https://gitlab.com/smartobject/soo>`__.

A polymorphic operating system
==============================

The most distinctive feature of SO3 is that it is **polymorphic**: a single
source tree can be configured and built into three different kinds of system.

Standalone OS
   SO3 runs directly on the hardware as a conventional monolithic OS. On ARM64
   the kernel runs at **EL1** and user applications at **EL0**. This is the
   configuration used for teaching and for plain embedded products
   (``virt64_defconfig``, ``rpi4_64_defconfig``, …).

AVZ hypervisor
   The same tree, built with ``CONFIG_AVZ``, becomes **AVZ** (*Agency
   VirtualiZer*) — a lightweight type-1 hypervisor running at **EL2**. AVZ hosts
   one or more guest *domains*; the primary guest is the **agency**, a normal
   SO3 (or Linux) running at EL1. See :ref:`avz`.

SO3 capsule
   On top of AVZ, the **SOO** framework adds *SO3 capsules* (**S3C**):
   lightweight, self-contained guests that run at EL1 beside a Linux *agency* and
   cooperate with it through split (frontend/backend) drivers. The capsule
   (guest) side lives in this repository; the Linux agency and the rest of the
   SOO framework live in a separate repository. See :ref:`capsules`.

.. note::

   *SO3 Capsule*, abbreviated **S3C**, is the current name and acronym for what
   older code and papers called a *Mobile Entity* (ME). The code base now uses
   the ``S3C`` acronym in identifiers and *capsule* in prose; the ``ME`` spelling
   no longer appears.

Approach and philosophy
=======================

There are countless operating systems for IoT and embedded systems. They all
have their own characteristics, but it remains hard to find an OS that supports
the *traditional* features — user/kernel separation, MMU, a real root
filesystem, real-time behaviour — while staying small, simple and quick to
configure.

The philosophy of SO3 is to keep the OS **as compact as possible**: small
enough to be a reasonable-complexity teaching platform, yet complete enough for
industrial prototyping and customization.

SO3 does not reinvent the wheel: it draws on long experience with other
operating systems and virtualization frameworks. It is mainly inspired by Linux
— its build system is based on **Kbuild**, and a few well-proven mechanisms
(the ``struct list_head`` linked lists and related macros, *bitops*, common type
definitions) come from Linux. This is also why SO3 is released under the
**GPLv2** licence.

Well suited to certification
============================

The same compactness that makes SO3 a good teaching platform also makes it a
convenient base for **safety- or security-certified** products (for example in
the spirit of IEC 61508, ISO 26262 or Common Criteria). The cost of a
certification campaign — code review, requirements traceability, structural
coverage, static analysis — grows with the amount of code that ends up in the
final binary, so a small, well-understood kernel is a real advantage:

* **Small, auditable code base.** The kernel is compact enough to be read and
  reviewed in its entirety, which keeps requirement-to-code traceability and
  coverage analysis tractable.

* **No unused code in the image.** Thanks to the Kconfig/Kbuild configuration,
  every subsystem and driver is opt-in: an SO3 build contains *only* the
  features that were selected. Dead or unreachable code — a recurring pain point
  for certification — is kept out of the binary instead of being disabled at
  runtime.

* **Minimal, well-bounded footprint.** A clear user/kernel separation, a single
  code base and a deterministic, reproducible build (defconfigs, FIT images)
  reduce the verification surface and make the certified artifact easy to
  characterise.

* **Self-contained dependencies.** The kernel pulls in only a handful of small,
  in-tree libraries and a MUSL-based user space, avoiding large third-party
  stacks that would otherwise have to be qualified.

In short, SO3 lets a project include just what it needs and nothing more, which
keeps the certification evidence — and the effort to produce it — proportional
to the actual functionality.

Emulation is a key concept
==========================

From the very beginning, the **QEMU** emulator has been a crucial tool to grasp
operating-system concepts and to develop and debug SO3 in a deep yet affordable
way. REDS has used QEMU since 2006.

The reference emulated platform is QEMU's **virt** machine: ``virt64`` (ARM64,
Cortex-A72) for the 64-bit configuration and ``virt32`` (ARM, Cortex-A15) for
the 32-bit one. As real hardware, the **Raspberry Pi 4** (quad-core Cortex-A72)
is the main target; the **Toradex Verdin i.MX8M Plus** is also supported.

SO3 as an experimental environment
==================================

A major advantage of SO3 is its **compilation time**. The kernel builds from
scratch in a few seconds, and the user space in a few tens of seconds depending
on what is selected. SO3 is therefore an excellent environment to experiment:
trying out a kernel function, a processor feature or a compilation trick is
quick and cheap.

.. _REDS: http://www.reds.ch
.. _HEIG-VD: http://www.heig-vd.ch
.. _heig-vd_news: https://heig-vd.ch/
.. _DRE: https://reds.heig-vd.ch/en/team/details/daniel.rossier
