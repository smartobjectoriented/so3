
SUMMARY = "Linux Operating System running as guest on AVZ"
DESCRIPTION = "Linux OS used as main domain (agency) running on the embedded platform"
LICENSE = "GPLv2"

# These patches contain lv_port_linux patch
FILESEXTRAPATHS:soo:prepend = "${THISDIR}/../soo/files/0002-${PF}:"
 
require files/0002-${PF}-patches.inc

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
 
