
SUMMARY = "Linux Operating System"
DESCRIPTION = "Linux OS used as main domain running on the embedded platform"
LICENSE = "GPLv2"

# Revision and version
PR = "r0"
PV = "6.12"

OVERRIDES += ":linux"

inherit linux

SRC_URI = "https://mirrors.edge.kernel.org/pub/linux/kernel/v6.x/linux-6.12.tar.gz;protocol=https"

SRC_URI[sha256sum] = "1376ce98485a0c8de4635d0bfb88760924e4a818c0439d830738bb1c690b7ca4"

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
	
	if [ "${IB_PLATFORM}" = "x86-qemu" -o "${IB_PLATFORM}" = "x86-pc" ]; then
		make -j${CORES} bzImage
	else
	
	  if [ "${IB_PLATFORM}" = "bbb" ]; then
	  	make -j${CORES} zImage
	  else
	    make -j${CORES} Image
	  fi
	  
	  # Compile the device tree files
	  make dtbs
	fi

}

do_clean[nostamp] = "1"
python do_clean () {
    __do_clean(d)
}
addtask do_clean
