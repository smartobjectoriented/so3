.. _build_system:

Build System
############

SO3's build system is derived from the Linux **Kbuild** infrastructure, so it
will feel familiar to anyone who has built a Linux kernel: ``Kconfig`` files,
``defconfig`` snapshots, ``obj-y`` lists and a recursive descent through the
source tree.

Configuration (Kconfig)
=======================

Each subsystem carries a ``Kconfig`` describing its options, and every option
becomes a ``CONFIG_*`` symbol. Configuration is stored in ``so3/.config`` and
exposed to the build as ``include/generated/autoconf.h``. The usual targets
work::

   cd so3
   make virt64_defconfig      # load a defconfig snapshot
   make menuconfig            # interactive configuration (optional)
   make -j$(nproc)            # build the kernel

The selected target architecture (``CONFIG_VIRT64``, ``CONFIG_RPI4_64``,
``CONFIG_VERDIN_IMX8MP`` for ARM64; ``CONFIG_VIRT32``, ``CONFIG_RPI4`` for ARM32)
and the deployment mode (``CONFIG_AVZ``, ``CONFIG_SOO``) are all driven from
``.config``. The cross-compiler is taken from ``CONFIG_CROSS_COMPILE`` — the
kernel uses the bare-metal **aarch64-none-elf** (or **arm-none-eabi**) toolchain.

Defconfigs and platforms
========================

The ready-made configurations live in ``so3/configs/``:

.. flat-table::
   :header-rows: 1
   :widths: 38 12 12 12 26

   * - defconfig
     - Arch
     - AVZ
     - SOO
     - Purpose
   * - ``virt64_defconfig``
     - arm64
     -
     -
     - standalone, QEMU virt
   * - ``virt64_fb_defconfig``
     - arm64
     -
     -
     - standalone + framebuffer (LVGL)
   * - ``virt64_lvperf_defconfig``
     - arm64
     -
     -
     - LVGL performance build
   * - ``virt64_avz_defconfig``
     - arm64
     - ✔
     -
     - AVZ hypervisor + agency
   * - ``virt64_avz_soo_defconfig``
     - arm64
     - ✔
     - ✔
     - AVZ + agency with SOO
   * - ``virt64_capsule_defconfig``
     - arm64
     -
     - ✔
     - SO3 capsule (guest)
   * - ``rpi4_64_defconfig``
     - arm64
     -
     -
     - Raspberry Pi 4 (64-bit)
   * - ``rpi4_64_avz_defconfig``
     - arm64
     - ✔
     -
     - RPi4 AVZ hypervisor
   * - ``rpi4_64_avz_soo_defconfig``
     - arm64
     - ✔
     - ✔
     - RPi4 AVZ + SOO
   * - ``verdin-imx8mp_avz_defconfig``
     - arm64
     - ✔
     -
     - Toradex Verdin i.MX8M Plus
   * - ``virt32_defconfig`` / ``…_fb`` / ``…_lvperf``
     - arm32
     -
     -
     - 32-bit QEMU virt variants
   * - ``rpi4_defconfig``
     - arm32
     -
     -
     - Raspberry Pi 4 (32-bit)

The active platform is also recorded at the repository root in ``build.conf``
(``PLATFORM := virt64``); the deployment and launch scripts read it from there.

Linker script and asm-offsets
=============================

The kernel is linked with an architecture-specific linker script
(``arch/arm64/so3.lds``). It places the exception vectors, the boot code
(``.head.text``), the code/data/bss, the per-CPU area (``.percpu_data``), the
system page tables, the driver *initcall* sections, the kernel heap and the
per-CPU stacks. Sizes come from configuration symbols (``CONFIG_HEAP_SIZE_MB``,
``CONFIG_NR_CPUS``, the stack sizes) passed to the linker with ``--defsym``; the
kernel base address is ``CONFIG_KERNEL_VADDR``.

Assembly code needs the byte offsets of C structures (for instance
``cpu_regs``). These are produced by ``arch/arm64/asm-offsets.c``, compiled to a
header of ``#define OFFSET_*`` values that both C and assembly share — the same
mechanism Linux uses.

Device trees
============

Hardware is described by **device trees** in ``so3/dts/``. The ``.dts`` sources
are compiled to ``.dtb`` blobs by the in-tree ``dtc`` (``scripts/dtc/``). The
relevant ``.dtb`` is shipped to the kernel inside the FIT image (below); the
kernel parses it at boot to discover RAM and devices (:ref:`kernel`).

FIT image and U-Boot
====================

SO3 is started by **U-Boot**, which expects a **FIT image** (``.itb``) — a single
file bundling the kernel, the device tree and the root filesystem (and, for AVZ,
the hypervisor and the agency guest). The image is described by an ``.its`` file
in ``target/`` and assembled with ``mkimage``:

.. flat-table::
   :header-rows: 1
   :widths: 40 60

   * - ``.its`` (in ``target/``)
     - Contents
   * - ``virt64.its``
     - standalone: SO3 kernel + DTB + ramfs
   * - ``virt64_avz_so3.its``
     - AVZ hypervisor + agency SO3 + DTBs + ramfs
   * - ``virt64_capsule.its``
     - a capsule image
   * - ``virt64_lvperf.its``
     - LVGL performance build
   * - ``rpi4_64*.its`` / ``virt32*.its``
     - the corresponding RPi4 / 32-bit variants

The helper ``target/mkuboot.sh <name>`` runs ``mkimage -f <name>.its
<name>.itb``. The resulting ``.itb`` is copied into the first partition of the
SD-card image by ``deploy.sh`` (see :ref:`user_guide`). U-Boot's environment
(``u-boot/uEnv.d/uEnv_<platform>.txt``) loads ``<platform>.itb`` and boots it.

Building the AVZ hypervisor
===========================

The AVZ hypervisor is an **out-of-tree** build of the same source, configured
into the repository-root ``avz/`` directory (whose ``avz/source`` is a symlink to
``so3/``)::

   cd avz
   ./build.sh virt64_avz_defconfig    # configure
   ./build.sh                         # build -> avz/so3.bin

The resulting ``avz/so3.bin`` is the hypervisor; the agency guest is an ordinary
``so3/so3.bin``. Both, together with the device trees and the root filesystem,
are packaged by ``virt64_avz_so3.its``. See :ref:`user_guide` for the full AVZ
run procedure.

User space
==========

The user-space applications are built separately with **CMake** and the
**MUSL** cross toolchain (``usr/aarch64-linux-musl.cmake`` /
``usr/arm-linux-musl.cmake``). This is covered in :ref:`user_space`.
