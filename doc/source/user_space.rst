.. _user_space:

User Space
##########

The SO3 user space is a small, self-contained set of applications built against
the **MUSL** C library. Everything lives under ``usr/``.

The MUSL C library
==================

SO3 uses `MUSL <MUSL_>`__ as its libc for user applications — a small, clean and
static-friendly implementation well suited to embedded systems. Not every libc
function is enabled; functions are pulled in as the need arises. More complex
facilities (for example full ``pthreads``) are intentionally kept minimal.

Applications are linked **statically** against MUSL, so each executable is
self-contained.

Build system (CMake)
====================

The user space is built with **CMake** and the MUSL cross toolchain. The build
is driven by ``usr/build.sh``, which:

* reads ``PLATFORM`` from ``../build.conf`` and selects the matching toolchain
  file — ``usr/aarch64-linux-musl.cmake`` for 64-bit platforms (virt64,
  rpi4_64), ``usr/arm-linux-musl.cmake`` for 32-bit ones;
* configures and builds the tree under ``usr/build/``;
* separates debug information (a stripped ``*.elf`` plus a ``*.elf.debug``) and
  collects the deployable binaries in ``usr/build/deploy/``.

.. code-block:: bash

   cd usr
   ./build.sh

Adding an application means dropping a C file in ``usr/src/`` and referencing it
from the relevant ``CMakeLists.txt``.

Applications
============

The standard applications in ``usr/src/`` include:

.. flat-table::
   :header-rows: 1
   :widths: 22 78

   * - Program
     - Role
   * - ``init.elf``
     - the **init** process: reads ``commands.ini`` and launches the shell
   * - ``sh.elf``
     - the interactive **shell** (the ``so3%`` prompt)
   * - ``ls`` / ``cat`` / ``more`` / ``echo``
     - basic file utilities
   * - ``ping``
     - ICMP ping (exercises the lwIP stack)
   * - ``time``
     - simple timing utility
   * - ``hello-world``
     - minimal example
   * - ``thread_example`` / ``logs_example`` / ``mydev_test``
     - API and subsystem demonstrations
   * - ``lvgl_demo`` / ``lvgl_perf`` / ``lvgl_benchmark``
     - LVGL graphical demos (framebuffer builds — see :ref:`lvgl`)
   * - MicroPython
     - the MicroPython interpreter (ARM64 — see :ref:`micropython`)

User-space libraries used by the applications live in ``usr/lib/`` (the LVGL
graphics library, logging helpers, and so on).

Init and the shell
==================

When the kernel hands over to user space, the root process ``execve()``\ s
``init.elf``. Init reads a small ``commands.ini`` script and runs it line by
line; a typical script prints a banner and starts the shell:

.. code-block:: text

   echo SO3 Init Program :)
   shell

The shell then presents the ``so3%`` prompt and runs commands by forking and
``execve()``\ -ing the corresponding ``.elf`` from the root filesystem.

.. note::

   The shell does not implement ``cd``: all executables and data files live in
   the root directory of the filesystem (except the device nodes under
   ``dev/``).

Root filesystem
===============

The applications are delivered through a root filesystem. In the default
(ramfs) configuration this is a FAT image, ``rootfs/rootfs.fat``, created by:

.. code-block:: bash

   cd rootfs
   ./create_ramfs.sh

``deploy.sh -u`` copies the freshly built user binaries from
``usr/build/deploy/`` into this image, which is then packed into the FIT image
and written to the SD-card (:ref:`user_guide`). The same FAT image can be
inspected on the host by mounting its partition — useful when debugging what
actually ended up on the target.

.. _MUSL: https://musl.libc.org
