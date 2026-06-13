
SUMMARY = "Buildroot rottfs"
DESCRIPTION = "Buildroot is used as rootfs generator"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "2024.11"

OVERRIDES += ":linux"

inherit logging
inherit rootfs

SRC_URI = "https://buildroot.org/downloads/buildroot-2024.11.tar.gz;protocol=https"
SRC_URI[sha256sum] = "4a601600b846058c2710cfda7d152d5d820b433ff4a4bce65c7eeb49f87e5540"

# defconfig patches
FILESPATH:prepend: := "${THISDIR}/files/0001-${PF}:"

# auto-getty package
FILESPATH:prepend: := "${THISDIR}/files/0002-${PF}:"

require files/0001-${PF}-patches.inc
require files/0002-${PF}-patches.inc

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_ROOTFS_PATH}/buildroot"

do_configure () {
	cd ${IB_TARGET}
	
	if [ ! -f ${IB_TARGET}/configs/${IB_PLATFORM}_defconfig ]; then
		bbfatal "${IB_PLATFORM}_defconfig is missing in buildroot/configs ..."
	fi
	
	make O=${IB_ROOTFS_PATH} ${IB_PLATFORM}_defconfig
}

do_build () {
	bbnote "Building buildroot based rootfs..."
	
	cd ${IB_TARGET}
	make  O=${IB_ROOTFS_PATH} BR2_EXTERNAL_DIRS=.. --no-print-directory 
}

do_clean[nostamp] = "1"

do_clean () {
	bbnote "Cleaning the entire rootfs..."
	
	rm -rf ${IB_ROOTFS_PATH}/build
	rm -rf ${IB_ROOTFS_PATH}/target
	rm -rf ${IB_ROOTFS_PATH}/host
	rm -rf ${IB_ROOTFS_PATH}/scripts
	rm -rf ${IB_ROOTFS_PATH}/images
	rm -rf ${IB_ROOTFS_PATH}/staging

	rm -f ${TMPDIR}/stamps/buildroot*
}
addtask do_clean


