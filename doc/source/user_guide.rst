.. _user_guide:

User Guide
##########

This guide walks through setting up SO3 from scratch and running it in QEMU.
The instructions were validated on Ubuntu 22.04 but work on other recent
distributions. The reference platform is **virt64** (QEMU's ``virt`` machine,
Cortex-A72, ARM64).

Prerequisites
=============

Install the host packages used to build the tree, the device trees, the FIT
images and the disk images:

.. code-block:: bash

   sudo apt-get update
   sudo apt-get install build-essential bc unzip flex bison
   sudo apt-get install device-tree-compiler u-boot-tools
   sudo apt-get install fdisk dosfstools
   sudo apt-get install qemu-system-arm libncurses-dev
   sudo apt-get install bridge-utils   # for QEMU tap networking

Toolchains
==========

SO3 uses **two** cross toolchains:

Kernel
   A *bare-metal* toolchain with no libc dependency:
   ``aarch64-none-elf`` for ARM64 and ``arm-none-eabi`` for ARM32. The kernel's
   ``CONFIG_CROSS_COMPILE`` points at it (``aarch64-none-elf-`` by default).

User space
   The user applications link against **MUSL**. The MUSL toolchains
   (``aarch64-linux-musl`` and ``arm-linux-musleabihf``) are produced by:

   .. code-block:: bash

      cd toolchains
      ./build-toolchain.sh

   By default this generates the toolchain directories under ``toolchains/``.

Make sure both toolchains are on your ``PATH``.

Repository layout
=================

The most relevant top-level directories are::

   so3/          # the kernel source (and the avz/ hypervisor, soo/ framework)
   usr/          # user-space applications (CMake + MUSL)
   rootfs/       # root filesystem image (ramfs) and its creation script
   target/       # FIT image descriptions (*.its) and mkuboot.sh
   filesystem/   # the virtual SD-card image used by QEMU
   u-boot/       # the U-Boot bootloader
   avz/          # out-of-tree build of the AVZ hypervisor
   build.conf    # selects the active PLATFORM
   deploy.sh     # deploys kernel/dtb/ramfs/apps into the SD-card image

Quick start (standalone, virt64)
================================

The fastest path to a shell on top of the standalone kernel.

**1. Select the platform.** Edit ``build.conf`` so that ``PLATFORM := virt64``
(the default).

**2. Build U-Boot** (once):

.. code-block:: bash

   cd u-boot
   make virt64_defconfig
   make -j$(nproc)

**3. Create the virtual SD-card** (once), in ``filesystem/``:

.. code-block:: bash

   cd filesystem
   ./create_img.sh virt64

**4. Build the user space** and pack the ramfs:

.. code-block:: bash

   cd usr
   ./build.sh
   cd ../rootfs
   ./create_ramfs.sh        # once, creates rootfs.fat

**5. Build the kernel:**

.. code-block:: bash

   cd so3
   make virt64_defconfig
   make -j$(nproc)

**6. Deploy** kernel, device tree, ramfs and the user apps into the SD-card
image (from the repository root):

.. code-block:: bash

   ./deploy.sh -bu

The ``-b`` option deploys the boot components (it builds the FIT image and copies
it into the first partition); ``-u`` deploys the user applications into the
ramfs.

**7. Run:**

.. code-block:: bash

   ./st

You should land at the ``so3%`` prompt.

.. note::

   To quit QEMU, type ``Ctrl-x`` followed by ``a`` (not ``Ctrl-a``).

Launch scripts
==============

Three launch scripts wrap QEMU with the right options:

.. flat-table::
   :header-rows: 1
   :widths: 12 88

   * - Script
     - Use
   * - ``./st``
     - **standalone** SO3 (``-M virt``, no virtualization). The kernel runs at
       EL1.
   * - ``./stv``
     - **AVZ** (``-M virt,gic-version=2,virtualization=on``). EL2 is available,
       so the hypervisor can run.
   * - ``./stg``
     - **graphical** run (a display window instead of ``-nographic``), used for
       the framebuffer / LVGL configurations.

All three read ``PLATFORM`` from ``build.conf`` and attach the SD-card image,
a virtio block device and a tap network device.

Running the AVZ hypervisor
==========================

To run the hypervisor with an agency guest on virt64:

.. code-block:: bash

   # 1. Build the hypervisor (out-of-tree into avz/)
   cd avz
   ./build.sh virt64_avz_defconfig
   ./build.sh

   # 2. Build the agency guest (a standalone SO3) and pack the FIT image
   cd ../so3
   make virt64_defconfig
   make -j$(nproc)
   cd ../target
   ./mkuboot.sh virt64_avz_so3        # produces virt64_avz_so3.itb

   # 3. Deploy that .itb as the image U-Boot loads (virt64.itb) and run
   #    (see deploy.sh / the filesystem mount scripts), then:
   cd ..
   ./stv

A successful run shows the **AVZ Hypervisor** banner, the *Loading Guest Domain*
trace and finally the agency reaching the ``so3%`` prompt. See :ref:`avz` for
what happens under the hood.

Running with Docker
===================

SO3 can also be built and run inside a Docker container. The ``Dockerfile`` is
at the repository root, and two helpers start the container:

.. code-block:: bash

   docker build -t so3/virt64 .
   ./drun            # run
   ./drunit          # run, interactive

Deploying a *Hello World*
=========================

User applications live in ``usr/src/``; adding a C file means adding it to the
relevant ``CMakeLists.txt``. Binaries are produced under ``usr/build/`` and the
files to be deployed are gathered in ``usr/build/deploy/``. After building the
user space, deploy the apps into the ramfs with:

.. code-block:: bash

   ./deploy.sh -u

This works with a ramfs configuration: the user applications are transferred
into the ``.itb`` image that is written to the single partition of the SD-card.
See :ref:`user_space` for the user-space build details.
