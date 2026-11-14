# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "Linux Operating System"
DESCRIPTION = "Linux OS used as main domain running on the embedded platform"
LICENSE = "GPLv2"

# Revision and version
PR = "r0"
PV = "6.12-rpi4"

OVERRIDES += ":linux"

inherit linux

SRC_URI = "git://github.com/raspberrypi/linux.git;branch=rpi-6.12.y;tag=stable_20250702;protocol=https"

SRCREV = "8f77e03530f65209a377d25023e912b288e039cd"

# Set of patches to be applied

# These patches contain rpi4 64-bits enhancement
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"
 
require files/0001-${PF}-patches.inc
require linux-deploy.inc

# The fork's Makefile patch pins ARCH ?= arm64 (weak): right for rpi4_64,
# overridden from the command line for the 32-bit rpi4 — same scheme as
# linux_6.12.bb.
IB_KMAKE_ARGS = "${@'ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf-' if (d.getVar('IB_PLAT_CPU') or '') == 'arm' else ''}"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_LINUX_PATH}"

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_KMAKE_ARGS} ${IB_CONFIG} 
}

do_build () {
	echo "Building Linux with ${CORES} cores..."
	
	cd ${IB_TARGET}
	
	make ${IB_KMAKE_ARGS} -j${CORES} Image  

	# On 32-bit also produce the self-relocating zImage: a raw arm32 Image is
	# position-dependent and any load-address mistake boots to dead silence,
	# while the zImage decompressor places itself (AUTO_ZRELADDR).
	if [ "${IB_PLAT_CPU}" = "arm" ]; then
		make ${IB_KMAKE_ARGS} -j${CORES} zImage
	fi
	    
	# Compile the device tree files
	make ${IB_KMAKE_ARGS} dtbs
	 
}

do_clean[nostamp] = "1"
python do_clean () {
    __do_clean(d)
}
addtask do_clean
