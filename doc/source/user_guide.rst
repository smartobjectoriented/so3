.. _user_guide:

##########
User Guide
##########

The following instructions have been validated with Ubuntu 22.04, but for sure
it will work with lower release.

*************
Pre-requisite
*************

We need to run some i386 executables, and we need to install some i386
libraries too.

.. code:: bash

   sudo dpkg --add-architecture i386
   sudo apt-get update
   sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
   sudo apt-get install lib32z1-dev
   sudo apt-get install zlib1g:i386

Various other packages are required:

.. code:: bash

   sudo apt-get install pkg-config libgtk2.0-dev bridge-utils
   sudo apt-get install unzip bc
   sudo apt-get install elfutils u-boot-tools
   sudo apt-get install device-tree-compiler
   sudo apt-get install fdisk
   sudo apt-get install libncurses-dev

The following packets are not mandatory, but they can be installed to
prevent annoying warnings:

.. code:: bash

   sudo apt-get install bison flex

********************************
Files and directroy organisation
********************************

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

************************
Build of the environment
************************

About the toolchain
===================
 
We use the ``arm-none-eabi`` toolchain which has no dependencies on a libc.

The following package can be installed:

.. code-block:: bash
   
   apt install gcc-arm-none-eabi

Quick setup & early test
========================

The following commands is helpful to have quick up-and-running
environment with SO3, i.e. a shell running on top of the kernel in the
emulated *vExpress* environment.

Setting up QEMU
===============

The QEMU emulator can be installed via apt for 32-bit and 64-bit versions,
as follows:

.. code-block:: bash 

   apt-get install qemu-system-arm
   apt-get install qemu-system-aarch64

Compiling U-boot
================

U-boot is used as initial bootloader. The kernel image, the device tree and
its root filesystem will be embedded in a single ITB image file. 

In u-boot/ directory:

.. code-block:: bash

   cd u-boot
   make vexpress_defconfig
   make -j8

Creating the virtual disk image
===============================

In *filesystem/* directory, create a virtual disk image with the
following script:

.. code-block:: bash

   cd filesystem
   ./create_img.sh vexpress

This script will create two FAT32 partitions, but only the first one will
be used currently (there is no support to mount the filesystem on the
second partition). 

Compiling the user space
========================

The user space build system is based on cmake (CMakeList.txt files).

To build the user space applications, go to ``usr/`` directory and simply
do make:

.. code-block:: bash

   cd usr
   ./build.sh
   
In order to support the configuration with an embedded ``ramfs``, you also need to create
a FAT-32 image which will contain the user apps. This is achieved with
the following script:

.. code-block:: bash

   cd rootfs
   ./create_ramfs.sh vexpress

The deployment of user applications into this *ramfs* will be done below during
the deployment into the SD-card (option ``-u`` of the ``deploy.sh`` script at 
the root directory).

Compiling the kernel space
==========================

The kernel has to be compiled in ``*so3*/`` after choosing a configuration:

.. code-block:: bash

   cd so3
   make vexpress_ramfs_defconfig
   make

In this example, we are working with an embedded *ramfs* which will be packed
in the ITB image file.

Deployment into the SD-card
===========================

At this point, all necessary components have been built. Now comes the
phase of deployment in the virtual disk. This is done by means of the
``deploy.sh`` script located at the root tree. 
Currently, you should only use option ``-b`` and ``-u`` to deploy the **kernel**, 
the **device tree** and the **ramfs** into the ITB file. This image file is 
then copied in the first partition of the SD-card.

.. code-block:: bash

   ./deploy.sh -bu

Starting SO3
============

Simply invoking the script st as following:

.. code-block:: bash

   ./st

and you should run into the shell…

.. note::

   To quit QEMU, type ``Ctrl+x`` followed by ``a`` (not Ctrl+a).


**********************
Default configurations
**********************

This section describes the default configurations of the SO3 kernel
which are present in “*so3/configs/*”.

vExpress platform
=================

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
=======================

Currently, there is only one default configuration file called
*rpi4_defconfig* which has a basic environment, without networking and
framebuffer support. The drivers required for networking and graphics
are not available yet.

Deployment of a *Hello World* application
=========================================

Using a *ramfs* configuration
-----------------------------

All user applications reside in ``usr/src`` directory. Adding a C file requires to update
the ``CMakeLists.txt`` file.

All binaries are produced in the ``usr/build`` directory by *cmake*. And all files which
will be deployed by the deployment script are stored in ``usr/build/deploy``.

.. note:: 

   Currently, the ``cd`` command is not implemented in the shell of SO3.
   For this reason, all executables (and other files) are stored in the root directory,
   except the entries of ``dev/`` used to access the drivers.

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
---------------------------

If you intend to use the *vexpress_mmc_defconfig* configuration for example, you
will need to deploy the user apps manually (the ``deploy.sh`` script will be
extended very soon). The deployment can be achieved as follows (from the root dir):

.. code-block:: bash

   cd filesystem
   ./mount.sh 1 vexpress
   sudo cp -r ../usr/build/deploy/* .
   ./umount.sh

The ``1`` refers to the partition #1.

.. warning::

   Do not forget that ``deploy.sh -b`` will erase the whole partition
   of the SD-card. You then need to re-deploy the user apps.
   


************************************
Installation and run with SO3 docker
************************************

It is also possible to start SO3 within a docker container.
The ``Dockerfile`` is located at the root directory and two scripts
``drun`` and ``drunit`` (for interactive mode) are available to start
the execution.

For example, building of a container named ``so3/vexpress`` can achieved like this:

.. code-block::bash

	docker build -t so3/vexpress .

The, starting the execution of the container:

.. code-block::bash

	./drun
	


	



 