.. _user_guide:

User Guide
##########

The following instructions have been validated with Ubuntu 22.04, but for sure
it will work with lower release.

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
emulated *virt32* environment.

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
   make virt32_defconfig
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
   ./create_ramfs.sh

The deployment of user applications into this *ramfs* will be done below during
the deployment into the SD-card (option ``-u`` of the ``deploy.sh`` script at 
the root directory).

Compiling the kernel space
==========================

The kernel has to be compiled in ``*so3*/`` after choosing a configuration:

.. code-block:: bash

   cd so3
   make virt32_ramfs_defconfig
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
************

Simply invoking the script st as following:

.. code-block:: bash

   ./st

and you should run into the shell…

.. note::

   To quit QEMU, type ``Ctrl+x`` followed by ``a`` (not Ctrl+a).



SO3 Configuration and ITS files
*******************************

This section describes the default configurations of the SO3 kernel
which are present in ``*so3/configs/*`` and in ``target/``

SO3 works with the following plaforms: ``virt32``, ``virt64``, ``rpi4``, ``rpi4_64``

+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| ITS                        | DTS                | Config               | virt32 | virt64 | rpi4 | rpi4_64 | avz | pv | vt | soo (linux) | rootfs | Validation |
+============================+====================+======================+========+========+======+=========+=====+====+====+=============+========+============+
|                            |                    | virt32_defconfig     |        |        |      |         |     |    |    |             |        |            |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
|                            |                    | virt64_defconfig     |        | X      |      |         |     |    |    |             | X      | 27.09.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| virt32_avz.its             | virt32.dts         | virt32_pv_defconfig  | X      |        |      |         | X   | X  |    |             | X      | 13.12.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| rpi4.its                   | rpi4.dts           | rpi4_defconfig       |        |        | X    |         |     |    |    |             | X      | 30.11.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| rpi4_64_avz_so3_pv.its     | rpi4_64_avz_pv.dts | rpi4_64_pv_defconfig |        |        |      | X       | X   | X  |    |             | X      | 27.09.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| rpi4_64_so3_standalone.its | rpi4_64.dts        | rpi4_64_defconfig    |        |        |      | X       |     |    |    |             | X      | 26.09.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| virt64_avz_so3_pv.its      | virt64.dts         | virt64_pv_defconfig  |        | X      |      |         | X   | X  |    |             | X      | 26.09.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| rpi4_64_avz_so3_vt.its     |                    | rpi4_64_defconfig    |        |        |      | X       | X   |    | X  |             | X      | 27.09.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| (soo) virt64.its           | virt64_avz_pv      | virt64_pv_defconfig  |        | X      |      |         |     | X  |    | X           |        | 07.10.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+
| (soo) rpi4_64.its          | rpi4_64_avz_pv     | virt64_pv_defconfig  |        |        |      | X       | X   | X  |    | X           |        | 08.10.23   |
+----------------------------+--------------------+----------------------+--------+--------+------+---------+-----+----+----+-------------+--------+------------+


*To be completed*

Deployment of a *Hello World* application
*****************************************

Using a *ramfs* configuration
=============================

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
   

Installation and run with SO3 docker
************************************

It is also possible to start SO3 within a docker container.
The ``Dockerfile`` is located at the root directory and two scripts
``drun`` and ``drunit`` (for interactive mode) are available to start
the execution.

For example, building of a container named ``so3/virt32`` can achieved like this:

.. code-block::bash

	docker build -t so3/virt32 .

The, starting the execution of the container:

.. code-block::bash

	./drun
	


	



 