
Before starting, you can have a look at the [SO3 discussion forum](https://discourse.heig-vd.ch/c/so3).
Feel free to post any comments/suggestions/remarks about SO3. If you wish to participate to development, please simply ask us and we will manage separate branches of development. 

# Initial setup

## Pre-requisite

The [Linaro arm-linux-gnueabihf toolchain](https://releases.linaro.org/components/toolchain/binaries/latest-7/arm-linux-gnueabihf) must be installed. Version 6.4.1 has been successfully tested, but more recent versions should be fine.

## Files and directory organization
* The SO3 tree is organized in two main parts: kernel and user space files.
All kernel space related files are located in so3/ and the user space files are in 
usr/
* SO3 user space libc is based on the [musl library](https://musl.libc.org)

## Quick setup & early test
The following commands is helpful to have quick up-and-running environment with SO3, 
i.e. a shell running on top of the kernel in the emulated vExpress environment.

### Building Qemu
The emulator must be built in qemu/ using the command line described in README.so3 
followed by invoking make (-j8 means parallel building on 8 cores):

```
cd qemu
./configure --target-list=arm-softmmu --disable-attr --disable-werror --disable-docs
make -j8

```

### Compiling U-boot
U-boot is used as initial bootloader. It will be possible to start an ITB image file 
containing the kernel, the device tree and an initrd filesystem. In u-boot/ directory:
```
cd u-boot
make vexpress_defconfig
make -j8
```



### Creating the virtual disk image
In filesystem/ directory, create a virtual disk image with the following script:
```
cd filesystem
./create_img.sh vexpress
```
### Compiling the user space
To build the user space applications, go to usr/ directory and simply do make:
```
cd usr
make
```
### Compiling the kernel space
The kernel has to be compiled in so3/ after choosing a configuration:
```
cd so3
make vexpress_mmc_defconfig
make
```

At this point, all necessary components have been built. Now comes the phase of deployment in the virtual disk.
This done by means of the deploy.sh script located at the root tree.

Currently, you should only use option b and u to deploy the ITB image as well as the user apps.
```
./deploy.sh -bu
```

### Starting SO3
Simply invoking the script st as following:
```
./st
```
and you should run into the shell...





