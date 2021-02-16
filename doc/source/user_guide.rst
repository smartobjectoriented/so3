.. _user_guide:

User Guide
==========

Pre-requisite
-------------

The Linaro arm-linux-gnueabihf toolchain must be installed. Version
6.4.1 has been successfully tested, but more recent versions should be
fine.

Files and directory organization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The SO3 tree is organized in two main parts: **kernel** and **user**
space files. All kernel space related files are located in “**so3/**”
while the user space files resides in “**usr/**”. The SO3 user space
libc is based on the musl library

Quick setup & early test
------------------------

The following commands is helpful to have quick up-and-running
environment with SO3, i.e. a shell running on top of the kernel in the
emulated *vExpress* environment.

Building Qemu
~~~~~~~~~~~~~

The emulator must be built in *qemu/* using the command line described
in README.so3 followed by invoking make (-j8 means parallel building on
8 cores):

.. code-block:: bash 

   cd qemu
   ./configure --target-list=arm-softmmu --disable-attr --disable-werror --disable-docs
   make -j8

Compiling U-boot
~~~~~~~~~~~~~~~~

U-boot is used as initial bootloader. It will be possible to start an
ITB image file containing the kernel, the device tree and an initrd
filesystem. In u-boot/ directory:

.. code-block:: bash

   cd u-boot
   make vexpress_defconfig
   make -j8

Creating the virtual disk image
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In *filesystem/* directory, create a virtual disk image with the
following script:

.. code-block:: bash

   cd filesystem
   ./create_img.sh vexpress

Compiling the user space
~~~~~~~~~~~~~~~~~~~~~~~~

To build the user space applications, go to *usr/* directory and simply
do make:

.. code-block:: bash

   cd usr
   make

Compiling the kernel space
~~~~~~~~~~~~~~~~~~~~~~~~~~

The kernel has to be compiled in *so3*/ after choosing a configuration:

.. code-block:: bash

   cd so3
   make vexpress_mmc_defconfig
   make

At this point, all necessary components have been built. Now comes the
phase of deployment in the virtual disk. This done by means of the
deploy.sh script located at the root tree. Currently, you should only
use option b and u to deploy the ITB image as well as the user apps.

.. code-block:: bash

   ./deploy.sh -bu

Starting SO3
~~~~~~~~~~~~

Simply invoking the script st as following:

.. code-block:: bash

   ./st

and you should run into the shell…

Default configurations
----------------------

This section describes the default configurations of the SO3 kernel
which are present in “*so3/configs/*”.

vExpress platform
~~~~~~~~~~~~~~~~~

+-----------------------------+----------------------------------------------------+
| Name                        | Environment                                        |
+=============================+====================================================+
| *vexpress_mmc_defconfig*    | Basic environment with a separate *rootfs* needed  |
|                             | to be stored in an MMC partition                   |
+-----------------------------+----------------------------------------------------+
| *vexpress_thread_defconfig* | Basic environment with no process support, hence   |
|                             | no *user space*.                                   |
+-----------------------------+----------------------------------------------------+
| *vexpress_nommu_defconfig*  | The MMU is disabled and only threads are available |
|                             | (no process/\ *user space*).                       |
+-----------------------------+----------------------------------------------------+
| *vexpress_net_defconfig*    | Environment with networking support (*lwip*)       |
|                             |                                                    |
+-----------------------------+----------------------------------------------------+
| *vexpress_fb_defconfig*     | Environment with LVGL and framebuffer support      |
|                             |                                                    |
+-----------------------------+----------------------------------------------------+
| *vexpress_full_defconfig*   | Complete environment with networking and           |
|                             | framebuffer support                                |
+-----------------------------+----------------------------------------------------+

Raspberry Pi 4 platform
~~~~~~~~~~~~~~~~~~~~~~~

Currently, there is only one default configuration file called
*rpi4_defconfig* which has a basic environment, without networking and
framebuffer support. The drivers required for networking and graphics
are not available yet.
