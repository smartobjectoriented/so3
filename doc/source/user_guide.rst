.. _user_guide:

User Guide
##########

This guide walks through building SO3 with :ref:`Infrabase <build_system>` and
running it in QEMU. The instructions were validated on Ubuntu 22.04/24.04 but
work on other recent distributions. The reference platform is **virt64** (QEMU's
``virt`` machine, Cortex-A72, ARM64).

Prerequisites
=============

Install the host packages used to build the tree, the device trees, the FIT
images and the disk images:

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install build-essential bc unzip flex bison libncurses-dev
   sudo apt-get install device-tree-compiler u-boot-tools mtools
   sudo apt-get install fdisk dosfstools
   sudo apt-get install python3 chrpath diffstat gawk    # bitbake deps

You do **not** need to install QEMU or a cross-toolchain: Infrabase builds a
patched QEMU (``meta-qemu``) and the MUSL toolchains (``meta-toolchain``) itself.
Privileged image steps escalate with ``sudo -n``; ``deploy.sh`` opens the sudo
session for you.

Repository layout
=================

The most relevant top-level directories are::

   so3/          # the SO3 kernel (so3/so3) and user space (so3/usr), plus dts + target/*.its
   build/        # the Infrabase build: meta-* layers, conf/local.conf, bitbake work tree
   scripts/      # build.sh, deploy.sh, st.sh, stg.sh, updiff.sh, …
   filesystem/   # the virtual SD-card image used by QEMU
   u-boot/       # fetched U-Boot (patched)
   qemu/         # fetched QEMU (patched) -> qemu/build/qemu-system-*
   avz/          # fetched AVZ hypervisor (patched)
   env.sh        # sourced once per shell to set up the environment

Quick start (standalone, virt64)
================================

**1. Set up the shell.**

.. code-block:: bash

   source env.sh

**2. Select the platform and deployment.** In ``build/conf/local.conf``:

.. code-block:: text

   IB_PLATFORM ?= "virt64"
   IB_TARGET_ITS:so3:virt64 = "virt64_so3"      # standalone SO3

**3. Build everything** for the SO3 BSP (kernel, user space, U-Boot, rootfs and
the FIT image):

.. code-block:: bash

   ./scripts/build.sh bsp-so3

**4. Deploy** onto the virtual SD-card (this opens the sudo session and writes the
boot partition):

.. code-block:: bash

   ./scripts/deploy.sh bsp-so3

**5. Run:**

.. code-block:: bash

   ./scripts/st.sh

You should land at the ``so3%`` prompt.

.. tip::

   After editing only the kernel, rebuild and redeploy it without a full rebuild::

      ./scripts/build.sh -x so3
      ./scripts/deploy.sh bsp-so3

   The ``deploy.sh bsp-so3`` step is required because the in-tree kernel binary
   is not tracked by bitbake (see :ref:`build_system`).

Launch scripts
==============

.. flat-table::
   :header-rows: 1
   :widths: 16 84

   * - Script
     - Use
   * - ``scripts/st.sh``
     - **headless** run (``-display none``) — serial console only. The default for
       non-graphical work and CI.
   * - ``scripts/stg.sh``
     - **graphical** run — a GTK window for the PL111 framebuffer (LVGL, ``fb_test``).
       See :ref:`display_input`.

Both read ``IB_PLATFORM`` and the selected ITS from ``build/conf/local.conf``,
attach ``filesystem/sdcard.img.<platform>`` as a virtio block device, forward the
guest SSH port (host ``2222`` → guest ``22``) and expose a GDB stub on
``tcp::1234`` (see :ref:`debugging`). The exception level is chosen automatically:

* a standalone ITS → ``-M virt`` (EL1);
* an ``…avz…`` ITS → ``-M virt,virtualization=on`` (EL2 for the hypervisor);
* the presence of ``filesystem/flash0.img`` → ARM-TF chain (``secure=on``, EL3).

.. note::

   To quit QEMU from the console, type ``Ctrl-x`` then ``a``. Inside the guest,
   **Ctrl-C** interrupts the foreground application or cancels the shell line —
   see :ref:`Console Ctrl-C <console_sigint>`.

Running the AVZ hypervisor
==========================

To run SO3 as the **agency guest** on top of AVZ:

**1.** Select the AVZ ITS (and, optionally, a secure boot chain) in
``build/conf/local.conf``:

.. code-block:: text

   IB_TARGET_ITS:so3:virt64 = "virt64_avz"
   # IB_BOOT_CHAIN ?= "atf+uboot"     # or "full" (ATF + OP-TEE); default is bare U-Boot

**2.** Build the hypervisor and (re)assemble the BSP, then deploy:

.. code-block:: bash

   ./scripts/build.sh -x avz
   ./scripts/build.sh bsp-so3
   ./scripts/deploy.sh bsp-so3

**3.** Run — ``st.sh`` enables EL2 automatically because the ITS is an AVZ image:

.. code-block:: bash

   ./scripts/st.sh

.. note::

   AVZ boot produces **two ITBs** — the AVZ ITB (``virt64_avz.itb``) and the
   SO3 guest ITB (``virt64_so3_guest.itb``, built automatically) — and the
   deploy stages both on the boot partition with ``uEnv_virt64_avz.txt``; U-Boot
   loads them and jumps via its ``e1c-boot`` command. No extra step is needed,
   but see :ref:`two_itb_boot` for details.

A successful run shows the **AVZ Hypervisor** banner, the *Loading Guest Domain*
trace, and finally the agency reaching the ``so3%`` prompt. See :ref:`avz` for
what happens under the hood.

Adding a user application
=========================

User applications live in ``so3/usr/src/``; adding a C file means adding it to the
relevant ``CMakeLists.txt``. Rebuild and redeploy the user space:

.. code-block:: bash

   ./scripts/build.sh -x usr-so3
   ./scripts/deploy.sh bsp-so3      # repack the rootfs into the FIT image + write the boot media

See :ref:`user_space` for the user-space build details and the bundled
applications.

Running with Docker
===================

SO3 can also be built and run inside a container. The Dockerfiles live under
``docker/`` and two helpers under ``docker/scripts/`` start the lv_perf image:

.. code-block:: bash

   docker/scripts/lvperf-run.sh      # run the LVGL benchmark (container exits with the output)
   docker/scripts/lvperf-shell.sh    # open an interactive shell in the container

Both default to ``so3-lvperf64b``; pass ``so3-lvperf32b`` as an argument for the
32-bit image. Inside the container the same ``source env.sh`` / ``build.sh`` /
``deploy.sh`` / ``st.sh`` workflow applies.
