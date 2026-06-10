.. _lvgl:

LVGL — Light and Versatile Embedded Graphics Library
####################################################

`LVGL <https://lvgl.io/>`__ is a free and open-source library providing an
efficient GUI for embedded systems.

Integration of LVGL in SO3
==========================

The initial port of LVGL to SO3 was done by Nikolaos Garanis in the context of
his `Bachelor project <https://nyg.gitlab.io/so3-support-graphique>`__; some
details are available on our `discussion forum
<https://discourse.heig-vd.ch/t/graphics-support-for-so3/41/18>`__ and in
`a short video <LVGL_qemu_>`__.

SO3 integrates **LVGL v8**, and the ``lv_demo_widgets`` demo is fully supported.
On the kernel side LVGL draws to the SO3 framebuffer device (``devices/fb/``,
exposed through ``/dev`` — see :ref:`kernel`); the LVGL library itself is built
into the user space (``usr/lib/lvgl``) and used by the ``lvgl_demo``,
``lvgl_perf`` and ``lvgl_benchmark`` applications.

.. _LVGL_qemu: https://youtu.be/skn_mp3ZBhI

Running LVGL under QEMU
=======================

.. note::

   Build the kernel with a framebuffer-enabled configuration, for example
   ``virt64_fb_defconfig``, and the user space with the LVGL apps.

The framebuffer needs a real display window, so start the emulator with the
graphical launch script instead of ``./st``:

.. code-block:: bash

   ./stg

QEMU opens a window that acts as the framebuffer; launch an LVGL application from
the ``so3%`` prompt to draw into it.
