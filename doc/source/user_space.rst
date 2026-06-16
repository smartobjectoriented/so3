.. _user_space:

User Space
##########

The SO3 user space is a small, self-contained set of applications built against
the **MUSL** C library. Everything lives under ``so3/usr/``.

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

The user space is built with **CMake** and the MUSL cross toolchain by the
``usr-so3`` recipe (``meta-usr``). The MUSL toolchain (``aarch64-linux-musl`` for
64-bit platforms, ``arm-linux-musleabihf`` for 32-bit ones) is produced by
``meta-toolchain``, so there is no manual toolchain step. Build and (re)deploy the
user space with:

.. code-block:: bash

   ./scripts/build.sh -x usr-so3      # configure + cross-compile (CMake + MUSL)
   ./scripts/deploy.sh -k bsp-so3     # repack the rootfs into the FIT image

The recipe configures and builds under ``so3/usr/build/`` and gathers the
deployable, statically-linked ``*.elf`` binaries for the root filesystem. Adding
an application means dropping a C file in ``so3/usr/src/`` and referencing it from
the relevant ``CMakeLists.txt``.

Applications
============

The standard applications in ``so3/usr/src/`` include:

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
   * - ``lvgl_widgets`` / ``lvgl_demo`` / ``lvgl_perf`` / ``lvgl_benchmark``
     - LVGL graphical demos (framebuffer builds — see :ref:`lvgl`)
   * - ``fb_test``
     - minimal framebuffer test, straight to ``/dev/fb`` (see :ref:`display_input`)
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
``execve()``\ -ing the corresponding ``.elf`` from the root filesystem. It
supports:

.. flat-table::
   :header-rows: 1
   :widths: 32 68

   * - Syntax
     - Meaning
   * - ``cmd arg1 arg2``
     - run ``cmd.elf`` with arguments
   * - ``cmd1 | cmd2 | cmd3``
     - pipeline (any number of stages)
   * - ``cmd > file`` / ``cmd >> file``
     - redirect stdout (truncate / append)
   * - ``cmd < file``
     - redirect stdin from a file
   * - ``cmd &``
     - run in the background (reaped on the next prompt)
   * - ``'literal'`` / ``"…$VAR…"``
     - single quotes (literal) / double quotes (with expansion)
   * - ``$VAR`` / ``${VAR}``
     - environment variable expansion
   * - ``exit`` / ``env`` / ``setenv`` / ``kill``
     - builtins

Operators (``| < > >> &``) must be surrounded by whitespace. Built-in
``setenv NAME VALUE`` sets a variable (``setenv NAME`` unsets it) and ``env``
lists the environment.

.. note::

   The shell does not implement ``cd``: there is no per-process working
   directory in the kernel yet, so all executables and data files live in the
   root directory of the filesystem (except the device nodes under ``dev/``).

Root filesystem
===============

The applications are delivered through a root filesystem. In the default
(ramfs) configuration this is a FAT image, ``so3/rootfs/rootfs.fat``, built by the
``rootfs-so3`` recipe (``meta-rootfs``) from the freshly built user binaries.
``./scripts/deploy.sh -k bsp-so3`` packs it into the FIT image and writes it to
the SD-card (:ref:`user_guide`); ``./scripts/build.sh -a bsp-so3`` does the whole
build + image in one step. The FAT image can be inspected on the host (e.g. with
``mtools``) — useful when debugging what actually ended up on the target.

.. _MUSL: https://musl.libc.org
