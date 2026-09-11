.. _capsules:

SO3 Capsules (SOO framework)
############################

An **SO3 Capsule** (acronym **S3C**) is a lightweight, self-contained guest that
runs at EL1 on top of the :ref:`AVZ hypervisor <avz>`. A capsule does not own any
hardware; it cooperates with an **agency** domain that owns the devices, through
split (frontend/backend) drivers.

.. note::

   *SO3 Capsule* / *S3C* is the current name and acronym for the concept that
   older code and papers called a *Mobile Entity* (ME). The source tree uses
   the **S3C** acronym in identifiers (``S3C_desc_t``, ``S3C_state_t``,
   ``S3C_domID``, ``MAX_S3C_DOMAINS`` …) and *capsule* in prose. The legacy
   ``ME`` spelling does not appear in the code.

The agency: Linux + the SOO framework
=====================================

The capsule model needs a **Linux** agency: Linux owns the devices and provides
the backend drivers and the higher-level services that capsules talk to — the
backend half of the frontend/backend split, the vbstore server and the
capsule-management user space (``s3c-inject``, ``s3c-list``, ``s3c-save`` /
``s3c-restore``).

The so3 **build system can fetch and build that agency itself**, the same way it
fetches AVZ, U-Boot and QEMU — it need not be built out of tree. The ``linux``
recipe pulls mainline Linux from kernel.org; an opt-in ``soo`` override
(``meta-linux/recipes-linux/soo`` and ``meta-usr/recipes-usr/soo``) patches it
into the agency and adds the SOO user space, and the ``bsp-capsules`` recipe
deploys *Linux as the guest on top of AVZ*. It is selected with
``EXTRA_OVERRIDES .= ":soo"`` and a SOO defconfig (``virt64_soo_defconfig``,
``rpi4_64_avz_soo_defconfig`` for AVZ on the Raspberry Pi 4); see
:ref:`build_system`.

The SOO additions — both the Linux kernel side and the agency user space — are
applied by the ``soo`` override as **vendored** ``file://`` patch sets
(``meta-linux/recipes-linux/soo`` and ``meta-usr/recipes-usr/soo``; the
``SOO_URI`` list is local patches, not a remote fetch). The kernel-side
patches live in a single **generic** set shared by every agency kernel
(``files/soo-generic/``); the per-kernel directory (``files/0001-<PF>/``)
only carries specifics such as the guest device tree, and a same-named
patch placed there shadows its generic counterpart when a kernel version
needs a divergent variant. Only the base Linux
kernel itself is fetched remotely (mainline from kernel.org). Those patches
derive from the **SOO framework**, developed in the separate
`soo project <https://gitlab.com/smartobject/soo>`__.

What *is* in this so3 repository is the **capsule (guest) side** and the
hypervisor support for it:

* the **frontend** drivers (``soo/drivers/``);
* the **vbus / vbstore** clients and the event-channel / grant-table glue
  (``soo/kernel/``);
* the hypervisor-side capsule **build / inject / snapshot** code
  (``avz/kernel/`` — ``capsule_build.c``, ``injector.c``).

A capsule-capable guest is produced by ``virt64_capsule_defconfig`` or
``rpi4_64_capsule_defconfig`` (enabling ``CONFIG_SOO``). The agency runs
**SMP** on the remaining cores (CPU 0–2 on both platforms) while the last
core (``S3C_CPU``, CPU 3) is reserved to AVZ for running the capsules. The AVZ demonstration shipped in this repository
(the ``virt64_avz.its`` AVZ ITB plus the separate ``virt64_so3_guest.its``
guest ITB — see :ref:`two_itb_boot`) boots a plain **SO3** guest
(``CONFIG_SOO=n``), which is enough to exercise the hypervisor; running actual
capsules additionally requires the **Linux** agency (built with the ``soo``
override described above).

Split (frontend/backend) drivers
=================================

Every virtual device a capsule uses is a **split driver**: a **frontend** in the
capsule talks to a **backend** in the Linux agency through a shared memory ring
and an event channel (set up with grant tables and :ref:`event channels <event_channels>`).

.. figure:: img/so3_capsule.png
   :width: 100%

   Split drivers and the vbstore configuration tree.

The frontends shipped in ``soo/drivers/`` are:

.. flat-table::
   :header-rows: 1
   :widths: 30 70

   * - Frontend
     - Purpose
   * - ``vfbdevfront``
     - virtual framebuffer (display output for the capsule)
   * - ``vinputfront``
     - virtual input (keyboard / mouse events)
   * - ``vuartfront``
     - virtual serial console
   * - ``vuihandlerfront``
     - UI handler / application channel
   * - ``vlogsfront``
     - logging service
   * - ``vsenseledfront`` / ``vsensejfront``
     - Raspberry Pi Sense HAT LED matrix and joystick
   * - ``vdummyfront``
     - reference/test driver

All frontends share the common machinery in ``soo/drivers/vdevfront.c`` and
register through ``vbus`` as ``vbus_driver``\ s.

vbus and vbstore
================

The frontend and backend find and configure each other through two Xen-inspired
mechanisms:

vbstore
   A small, hierarchical **configuration store** (``soo/kernel/vbstore/``),
   analogous to Xenstore. Devices advertise their state and parameters at paths
   such as ``device/<domID>/<dev>/state``, ``…/ring-ref`` and
   ``…/event-channel``.

vbus
   The **virtual bus** (``soo/kernel/vbus/``) that enumerates devices and drives
   their state machine. When a frontend and its backend have both published their
   ring reference and event channel in vbstore and reached the *Connected* state,
   I/O can flow. The same state machine drives *probe*, *suspend* and *resume*.

Capsule state and the snapshot mechanism
========================================

A capsule's state is tracked by ``S3C_state_t``
(``soo/include/soo/uapi/soo.h``): ``S3C_state_stopped``, ``S3C_state_living``,
``S3C_state_suspended``, ``S3C_state_resuming``, ``S3C_state_awakened``,
``S3C_state_killed``, ``S3C_state_terminated``, ``S3C_state_dead`` (and the
``S3C_state_hibernate`` / ``S3C_state_booting`` intermediates).

AVZ provides a low-level **snapshot** primitive — ``AVZ_S3C_READ_SNAPSHOT`` and
``AVZ_S3C_WRITE_SNAPSHOT`` — that saves and restores a capsule's memory image and
its vbstore state. This is the building block the SOO framework uses to move a
capsule's execution state; the higher-level orchestration that drives it lives in
the soo repository, not here.

How a snapshot is moved
-----------------------

Both hypercalls are **staged**, and the agency drives the stages:

``AVZ_STAGE_INIT``
   Prepare. Reading, this pauses the capsule and hands the header back — the
   payload size and the domain context. Writing, this allocates the slot,
   restores the domain context and sets up the page tables.

``AVZ_STAGE_CHUNK``
   Move ``AVZ_STAGE_CHUNK_SIZE`` bytes of capsule memory at most, and advance the
   cursor by what was actually copied. Repeated until the payload is through.

``AVZ_STAGE_FINALIZE``
   Complete: rebuild the stack, rebind the event channels, resume the capsule.

The chunking is what keeps the calling CPU from spending seconds at EL2 with the
interrupts off, and it is also what keeps the agency from having to find a
contiguous region the size of a capsule slot: ``snapshot_paddr`` points at a
**bounce buffer** of one chunk, reserved once when the soo module initialises,
while the CMA zone is still free of any movable page which would have to be
migrated out of the way. The agency copies each chunk to or from user space as
the stages progress, so the snapshot is never held twice in RAM.

The buffer carries the header at the INIT **and** FINALIZE stages — AVZ reads the
domain context again to restore the EL2 frame — and one chunk of capsule memory,
at its very beginning, in between.

.. note::

   The size stored at the beginning of a snapshot does not count itself, while
   the value AVZ hands back to the agency does. Deriving the header size from the
   stored value therefore misses ``sizeof(uint32_t)``, and every chunk is then
   read from the wrong offset — which restores a capsule onto shifted contents.

Snapshotting without resuming
-----------------------------

A snapshot leaves the capsule living: it is suspended for the time its memory is
read, then resumed. That is the point of snapshotting a running capsule, and
``AGENCY_IOCTL_READ_SNAPSHOT`` does exactly that.

A caller which shuts the capsule down right after — pausing it, in an engine
which stores the snapshot and frees the slot — gets the opposite of what it
wants: the capsule is woken up only to be killed, and runs for a moment,
diverging from the snapshot just taken. ``AGENCY_IOCTL_READ_SNAPSHOT_HOLD``
reaches ``AVZ_STAGE_FINALIZE_HOLD`` instead, which completes the snapshot and
leaves the capsule suspended.

Releasing what a capsule owns
-----------------------------

A capsule owns state in three places, and each owner releases its own share.
Nothing else does it for them, so a capsule which skips one of these steps leaves
its slot unusable for the next one:

.. list-table::
   :header-rows: 1
   :widths: 18 82

   * - Owner
     - What it releases
   * - the capsule
     - Its frontends, its grants and its vbstore entries, while it is still
       running — driven by the ``DC_SHUTDOWN`` handshake.
   * - the agency
     - The backends bound to that domain, and its vbstore subtrees
       (``backend/<type>/<domID>``, ``device/<domID>``, ``soo/s3c/<domID>``),
       when the capsule can no longer answer that handshake.
   * - AVZ
     - The grant table of the domain, when the domain is destroyed.

The agency's share is done in ``shutdown_capsule()``, in the branch where the
handshake is skipped: a capsule which is suspended, stopped, or already killed
after a fault cannot answer it, and ``do_sync_dom()`` waits for that answer
without any deadline.
