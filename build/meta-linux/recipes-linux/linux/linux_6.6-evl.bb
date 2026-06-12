
SUMMARY = "Linux Operating System"
DESCRIPTION = "Linux OS used as main domain running on the embedded platform"
LICENSE = "GPLv2"

# Revision and version
PR = "r0"
PV = "6.6-evl"

OVERRIDES += ":linux"

inherit linux
 
SRC_URI = "git://gitlab.com/Xenomai/xenomai4/linux-evl.git;branch=v6.6.y-evl-rebase;protocol=https"

SRCREV = "a183f2498555d05cddd096212d5b81f10894a762"

# Set of patches to be applied

# These patches enables QEMU/virt64 with framebuffer
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
