
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

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_LINUX_PATH}"

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_CONFIG} 
}

do_build () {
	echo "Building Linux with ${CORES} cores..."
	
	cd ${IB_TARGET}
	
	make -j${CORES} Image  
	    
	# Compile the device tree files
	make dtbs
	 
}

do_clean[nostamp] = "1"
python do_clean () {
    __do_clean(d)
}
addtask do_clean
