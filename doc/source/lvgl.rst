.. _lvgl:

LVGL — Light and Versatile Embedded Graphics Library
####################################################

`LVGL <https://lvgl.io/>`__ is a free and open-source library providing an
efficient GUI for embedded systems.

Integration of LVGL in SO3
==========================

The initial port of LVGL to SO3 was done by Nikolaos Garanis in the context of
his `Bachelor project <https://nyg.gitlab.io/so3-support-graphique>`__; a short
demonstration is available in `this video <LVGL_qemu_>`__.

SO3 integrates **LVGL v9**. The library is a pure user-space component: it is
fetched into ``so3/usr/lib/lvgl`` at build time (the ``:lvgl`` override in
``build/conf/local.conf``) and linked into the LVGL applications. It draws into
the SO3 framebuffer and reads the keyboard and mouse entirely through ``/dev``
nodes — there is no LVGL code in the kernel. The device side (PL111 framebuffer,
PL050 keyboard, the ``so3,absmouse`` absolute pointer) is documented in
:ref:`display_input`.

.. _LVGL_qemu: https://youtu.be/skn_mp3ZBhI

The ``slv`` glue library
========================

``so3/usr/lib/slv`` ("SO3 ↔ LVGL") is a small library that wires LVGL to the SO3
devices so an application does not have to. ``slv_init()``:

* initialises LVGL (``lv_init``);
* opens ``/dev/fb`` and registers an LVGL **display** (``slv_fb_init`` — queries
  the resolution via the framebuffer ioctls and ``mmap``\ s the VRAM);
* opens ``/dev/keyboard`` and registers a **keypad** indev (``slv_keyboard_init``);
* opens ``/dev/mouse`` and registers a **pointer** indev, telling the driver the
  screen size so it can scale the absolute coordinates (``slv_mouse_init``);
* spawns a **tick thread** that calls ``lv_tick_inc(10)`` every 10 ms.

``slv_loop()`` then runs the LVGL event loop (``lv_timer_handler`` + sleep) until
the application is killed (Ctrl-C), and ``slv_terminate()`` tears everything down.
A typical application is just:

.. code-block:: c

   slv_t slv;
   slv_init(&slv);
   lv_demo_widgets();      /* build the UI */
   slv_loop(&slv);         /* render + dispatch input */
   slv_terminate(&slv);

Applications
============

The LVGL applications are defined in ``so3/usr/src/CMakeLists.txt``:

.. flat-table::
   :header-rows: 1
   :widths: 22 78

   * - Application
     - Description
   * - ``lvgl_widgets``
     - the LVGL ``lv_demo_widgets`` gallery — exercises the display, keyboard and
       mouse together.
   * - ``lvgl_demo``
     - the LVGL demo showcase.
   * - ``lvgl_benchmark``
     - the LVGL rendering benchmark.
   * - ``lvgl_perf``
     - a one-shot performance probe (runs a single ``lv_timer_handler`` pass and
       exits — this is by design).
   * - ``fb_test``
     - a minimal, LVGL-free framebuffer test (writes straight to ``/dev/fb``);
       useful to validate the display path on its own.

.. note::

   ``lv_demo_widgets`` needs a larger LVGL heap than the default; SO3 sets
   ``LV_MEM_SIZE`` to 4 MB in ``so3/usr/lib/lv_conf.h``.

Running LVGL under QEMU
=======================

Build the user space with the LVGL applications and a framebuffer-enabled kernel
configuration, then launch the emulator in **graphical** mode with ``-d`` (the
framebuffer needs a real window):

.. code-block:: bash

   st.sh -d

QEMU opens a GTK window that shows the PL111 framebuffer; launch an LVGL
application from the ``so3%`` prompt to draw into it, for example::

   so3% lvgl_widgets

Move the mouse and click on the widgets — the cursor maps 1:1 to the host pointer
(absolute pointer, no grab). Press **Ctrl-C** in the console to terminate the
application and return to the shell. The display, input and Ctrl-C behaviour are
covered in detail in :ref:`display_input`.
