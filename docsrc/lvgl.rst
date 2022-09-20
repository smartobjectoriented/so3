
LVGL - Light and Versatile Embedded Graphics Library
====================================================

LVGL is a free and open-source library providing an efficient GUI for embedded systems.
More information is available on the `official site <https://lvgl.io/>`__.

Integration of LVGL in SO3
--------------------------

The major work of porting LVGL in SO3 has been done by Nikolaos Garanis in the context of 
his `Bachelor project <https://nyg.gitlab.io/so3-support-graphique>`_.

Some details about the porting can be found in our `discussion forum <https://discourse.heig-vd.ch/t/graphics-support-for-so3/41/18>`__.

There is `a small video <LVGL_qemu_>`__ to show LVGL running in the QEMU/vExpress framebuffer emulated environment.

In addition, from LVGL v8, the ``lv_demo_widgets`` is now fully supported. And yes, SO3 integrates LVGL v8.

This part will be completed very soon...


.. _LVGL_qemu: https://youtu.be/skn_mp3ZBhI

Using LVGL in the emulated environment
--------------------------------------

.. note::

   First, make sure you compiled the kernel with a configuration
   which has the framebuffer enabled (for example, *vexpress_fb_defconfig*)
   
In order to have a graphical framebuffer in the emulated QEMU/vExpress 
environment, it is necessary to start the emulator with the ``stg`` script:

.. code-block:: bash

   ./stg
   
QEMU will start a new GUI window used as framebuffer.

