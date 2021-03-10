.. _user_guide:

User Guide
==========

Pre-requisite
-------------

For the AArch32 (32-bit) version (the only one supported currently),
a `Linaro arm-linux-gnueabihf toolchain <https://releases.linaro.org/components/toolchain/binaries/>`__ 
must be installed. Version 6.4.1 has been successfully tested, but more recent versions should be
fine.

In the next sections, we assume working on the AArch32 (32-bit) ARM architecture (since are
are working with ARM cores having *armv7* or *armv8* micro-architectures).

Files and directory organization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SO3 root directory (main subdirs)::

   so3
   ├── usr
   ├── src
   ├── filesystem
   ├── target
   ├── u-boot
   └── qemu

- The SO3 tree is organized in two main parts: **kernel** and **user**
  space files. All kernel space related files are located in ``so3/``
  while the user space files resides in ``usr/``. 

- The ``filesystem`` directory contains the virtual SD-card image which will
  be attached to the QEMU environment.

- The ``target`` directory contains the set of ``.its`` files which are used
  by the *U-boot* bootloader.

- The ``u-boot`` and ``qemu`` directories contain the *bootloader* and 
  the *emulator* respectively. 

Quick setup & early test
------------------------

The following commands is helpful to have quick up-and-running
environment with SO3, i.e. a shell running on top of the kernel in the
emulated *vExpress* environment.

Building Qemu
~~~~~~~~~~~~~

The QEMU emulator must be built in *qemu/* using the command line described
in README.so3 followed by invoking make (-j8 means parallel building on
8 cores):

.. code-block:: bash 

   cd qemu
   ./configure --target-list=arm-softmmu --disable-attr --disable-werror --disable-docs
   make -j8

Again this build produces the binary for a 32-bit ARM (AArch32) architecture.

Compiling U-boot
~~~~~~~~~~~~~~~~

U-boot is used as initial bootloader. The kernel image, the device tree and
its root filesystem will be embedded in a single ITB image file. 

In u-boot/ directory:

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

This script will create two FAT32 partitions, but only the first one will
be used currently (there is no support to mount the filesystem on the
second partition). 

Compiling the user space
~~~~~~~~~~~~~~~~~~~~~~~~

To build the user space applications, go to ``usr/`` directory and simply
do make:

.. code-block:: bash

   cd usr
   make

Compiling the kernel space
~~~~~~~~~~~~~~~~~~~~~~~~~~

The kernel has to be compiled in ``*so3*/`` after choosing a configuration:

.. code-block:: bash

   cd so3
   make vexpress_mmc_defconfig
   make

Deployment into the SD-card
~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

Deployment of a *Hello World* application
-----------------------------------------

Using a *ramfs* configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All user applications reside in ``usr/src`` directory. Adding a C file requires to update
the ``TARGETS`` variable in the *Makefile* and launch *make* in ``usr/``.

The binaries are stored in ``usr/out`` directory.

The deployment into the virtual SD-card is simply done with the ``deploy.sh`` script
at the root dir as follows:

.. code-block:: bash

   ./deploy.sh -u

.. note::

   This manner of deploying user applications requires to have a ramfs 
   configuration. All user apps are actually transfered into the *itb* file
   which is deployed in the unique partition of the SD-card.
   
   The next section shows how you should deploy with the MMC configuration.

Using a *mmc* configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you intend to use the *vexpress_mmc_defconfig* configuration for example, you
will need to deploy the user apps manually (the ``deploy.sh`` script will be
extended very soon). The deployment can be achieved as follows (from the root dir):

.. code-block:: bash

   cd filesystem
   ./mount.sh 1 vexpress
   sudo cp -r ../usr/out/* .
   ./umount.sh

The ``1`` refers to the partition #1.

.. warning::

   Do not forget that ``deploy.sh -b`` will erase the whole partition
   of the SD-card. You then need to re-deploy the user apps.
   




 