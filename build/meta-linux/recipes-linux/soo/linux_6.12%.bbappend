# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "Linux Operating System running as guest on AVZ"
DESCRIPTION = "Linux OS used as main domain (agency) running on the embedded platform"
LICENSE = "GPLv2"

# SOO agency patches: the generic set (soo-generic) is shared by every
# agency kernel; the per-${PF} directory carries kernel-version/platform
# specifics. FILESPATH order puts the per-PF dir FIRST, so a same-named
# patch there shadows its generic counterpart.

FILESEXTRAPATHS:soo:prepend = "${THISDIR}/../soo/files/0001-${PF}:${THISDIR}/../soo/files/soo-generic:"

require files/soo-generic-patches.inc
require files/0001-${PF}-patches.inc

do_configure[nostamp] = "1"
do_configure:soo () {
	cd ${IB_TARGET}
	make ${IB_CONFIG}
}

do_build:soo () {
	echo "Building Linux with ${CORES} cores..."
	
	cd ${IB_TARGET}

	make -j${CORES} Image
	  
	# Compile the device tree files
	make dtbs
}
 
