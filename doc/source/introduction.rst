.. _introduction:

Introduction to SO3
===================

SO3 (*Smart Object Oriented*) is a compact, extensible and full featured operating system which is particularly suited to embedded systems
such as IoT devices, real-time environment, academic development, etc.

SO3 Operating System results from several Years of research and development at 
`REDS <REDS_>`__ (Reconfigurable Embedded Digital Systems) Institute from `HEIG-VD <HEIG-VD_>`__,
in the field of embedded operating systems and execution environment with ARM 32/64-bit multicore systems. SO3 has
been publicly released in early 2020 (see also `HEIG-VD newsletter <heig-vd_news_>`__).

`Prof. Daniel Rossier <DRE_>`__ initiated the development of an operating system in 2013 in the context of a Bachelor 
lecture focusing on the port of operating systems on different embedded platforms. The OS has been constantly evolved
and used in other lectures (like operating systems of course) and since 2018 became SO3 to be used as
a full *migrating* execution environment between connected devices; this novel approach leads to the 
`SOO framework <https://gitlab.com/smartobject/soo>`__ which is still in development.

More recently, SO3 is become a *polymorphic* operating system, meaning that it can be configured to
run as an hypervisor, a standalone OS or a guest on top of the hypervisor. The hypervisor is called **AVZ**
(Agency VirtualiZer) and is aimed at running with or without ARM VT support.

Approach and philosophy
-----------------------

Today, there are plenty of operating systems especially in the area of IoT devices and embedded systems in general.
All these OS oviously have their own characteristics, pros and cons, but it still remains pretty hard to find an 
OS which supports most of *traditional* features such as support for a user and kernel space, MMU, rootfs, 
realtime feature, etc. while keeping a simple OS with low memory footprint and minimal configuration. 

The philosophy of SO3 is to keep an OS as compact as possible, which can be used not only in an academic environment
to provide students an OS envronment with a reasonible complexity, but in industry oriented projects where some
customization and rapid prototyping is required.  

Of course, it does not consist in reiventing the wheel, but in opposite, to benefit from various experiences with existing
operating systems, emulation and virtualization frameworks. SO3 is mainly based on Linux principles; for example, its build system
fully relies on the KBuild and some portions of code (but not so many) come from Linux (examples are the very nice mechanisms
of linked list (``struct list_head``) and related macros, or *bitops* and other *types* declarations).
It is also one of the main reason why SO3 is released under the GPLv2 licence.


Emulation is a **key** concept
------------------------------

From the beginning, the ``QEMU`` emulator constituted a crucial tool to understand and to grasp concepts in an operating system
like Linux, and it also essential to elaborate and to debug an OS in a deep and affordable way. At the REDS Institute,
we used QEMU since 2006 and have a strong experience in emulated environment.
Now, the emulated *vExpress* machine in QEMU is used as a reference platform for the 32-bit configuration (virt64 will
be used as soon as the AArch64 support will be integrated).

Besides emulation, the Raspberry Pi 4 is currently our platform of choice as real hardware. Its quadcore Cortex-A72 
can be used in a wide range of applications and brings along many interesting features.


SO3 as an experimental environment
----------------------------------

One of the major advantage with SO3 is its time of compilation. The kernel is compiled (from scratch) in less than 10 secs
and the user space less than 20 secs depending on what is compiled.
Therefore, SO3 can be used a very nice environment to experiment different things; testing a kernel function, 
processor features, or compilation tricks are easily achieved.

Ongoing work with SO3
---------------------

SO3 is evolving constantly. Here is a non-exhaustive list of future work:

- Improvement of the networking based on lwIP 
- Support of 64-bit (AArch64)
- Support of multi-cores
- Porting SO3 on a RISC-V architecture (Diploma work)

And finally, we are looking for passoniated contributors :-) Please do not hesitate to `contact us <DRE_mail_>`__.

.. _REDS: http://www.reds.ch
.. _HEIG-VD: http://www.heig-vd.ch
.. _heig-vd_news: https://heig-vd.ch/actualites?utm_medium=email&utm_campaign=Newsletter%20externe%2039&utm_content=Newsletter%20externe%2039+CID_db69309487920998ee2eaa75dc3cab5a&utm_source=heig%20vd&utm_term=PLUS%20DINFORMATIONS#/2020/02/11/so3systemeexploitation
.. _DRE: https://reds.heig-vd.ch/en/team/details/daniel.rossier
.. _DRE_mail: info@soo.tech


