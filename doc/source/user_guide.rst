.. _user_guide:

User Guide
==========

Pre-requisite
-------------

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

The following packets are not mandatory, but they can be installed to
prevent annoying warnings:

.. code:: bash

   sudo apt-get install bison flex

Toolchain
~~~~~~~~~

The toolchain has been created by Linaro, with the version 2018-05. It
includes an arm-linux-gnueabihf GCC 6.4.1 compiler. For now, nothing has
been tested with a version greater than this one.

You can download the toolchain at the following address:
http://releases.linaro.org/components/toolchain/binaries/6.4-2018.05/arm-linux-gnueabihf/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf.tar.xz.asc

Uncompress the archive and add the *bin* subdirectory to your path.

You can check the version by running the following command:

.. code:: bash

   arm-linux-gnueabihf-gcc --version

The output should look like:

::

   Using built-in specs.
   COLLECT_GCC=/opt/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc
   COLLECT_LTO_WRAPPER=/opt/gcc-linaro-6.4.1-2018.05-x86_64_arm-linux-gnueabihf/bin/../libexec/gcc/arm-linux-gnueabihf/6.4.1/lto-wrapper
   Target: arm-linux-gnueabihf
   Configured with: '/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/snapshots/gcc.git~linaro-6.4-2018.05/configure' SHELL=/bin/bash --with-mpc=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/builds/destdir/x86_64-unknown-linux-gnu --with-mpfr=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/builds/destdir/x86_64-unknown-linux-gnu --with-gmp=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/builds/destdir/x86_64-unknown-linux-gnu --with-gnu-as --with-gnu-ld --disable-libmudflap --enable-lto --enable-shared --without-included-gettext --enable-nls --with-system-zlib --disable-sjlj-exceptions --enable-gnu-unique-object --enable-linker-build-id --disable-libstdcxx-pch --enable-c99 --enable-clocale=gnu --enable-libstdcxx-debug --enable-long-long --with-cloog=no --with-ppl=no --with-isl=no --disable-multilib --with-float=hard --with-fpu=vfpv3-d16 --with-mode=thumb --with-tune=cortex-a9 --with-arch=armv7-a --enable-threads=posix --enable-multiarch --enable-libstdcxx-time=yes --enable-gnu-indirect-function --with-build-sysroot=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/sysroots/arm-linux-gnueabihf --with-sysroot=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/builds/destdir/x86_64-unknown-linux-gnu/arm-linux-gnueabihf/libc --enable-checking=release --disable-bootstrap --enable-languages=c,c++,fortran,lto --build=x86_64-unknown-linux-gnu --host=x86_64-unknown-linux-gnu --target=arm-linux-gnueabihf --prefix=/home/tcwg-buildslave/workspace/tcwg-make-release/builder_arch/amd64/label/tcwg-x86_64-build/target/arm-linux-gnueabihf/_build/builds/destdir/x86_64-unknown-linux-gnu
   Thread model: posix
   gcc version 6.4.1 20180425 [linaro-6.4-2018.05 revision 7b15d0869c096fe39603ad63dc19ab7cf035eb70] (Linaro GCC 6.4-2018.05)

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
~~~~~~~~~~~~~~~~~~~~~~~~~~

The kernel has to be compiled in ``*so3*/`` after choosing a configuration:

.. code-block:: bash

   cd so3
   make vexpress_ramfs_defconfig
   make

In this example, we are working with an embedded *ramfs* which will be packed
in the ITB image file.

Deployment into the SD-card
~~~~~~~~~~~~~~~~~~~~~~~~~~~

At this point, all necessary components have been built. Now comes the
phase of deployment in the virtual disk. This is done by means of the
``deploy.sh`` script located at the root tree. 
Currently, you should only use option ``-b`` and ``-u`` to deploy the **kernel**, 
the **device tree** and the **ramfs** into the ITB file. This image file is 
then copied in the first partition of the SD-card.

.. code-block:: bash

   ./deploy.sh -bu

Starting SO3
~~~~~~~~~~~~

Simply invoking the script st as following:

.. code-block:: bash

   ./st

and you should run into the shell…

.. note::

   To quit QEMU, type ``Ctrl+x`` followed by ``a`` (not Ctrl+a).

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
~~~~~~~~~~~~~~~~~~~~~~~~~~~

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
   




 