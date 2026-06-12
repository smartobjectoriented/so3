
SUMMARY = "QEMU EMulator"
DESCRIPTION = "QEMU emulation environment core receipt"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "8.2.2"

inherit qemu

SRC_URI = "https://download.qemu.org/qemu-8.2.2.tar.xz;protocol=https"
		  	
SRC_URI[sha256sum] = "847346c1b82c1a54b2c38f6edbd85549edeb17430b7d4d3da12620e2962bc4f3"

# Set of patches to be applied

FILESPATH:prepend: := "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_DIR}/qemu"

QEMU_CONFIGURE:virt32 = "--target-list=arm-softmmu --disable-attr --disable-werror --disable-docs"
QEMU_CONFIGURE:virt64 = "--target-list=aarch64-softmmu --disable-attr --disable-werror --disable-docs --enable-sdl"

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/qemu*
}
addtask do_clean
