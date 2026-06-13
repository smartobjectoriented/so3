
SUMMARY = "SO3 kernel"
DESCRIPTION = "Smart Object Oriented Operating System"
LICENSE = "GPLv2"

inherit so3

# Version and revision
PR = "r0"
PV = "6.1.0"

OVERRIDES += ":so3"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_SO3_PATH}"

SRC_URI = "git://github.com/smartobjectoriented/so3.git;nobranch=1;protocol=https"
SRCREV = "5f831ecc5071380b9f2b98e946c605598f040c98"


do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_SO3_PATH}/so3
	make ${IB_CONFIG}
}

do_build () {
	echo "Building SO3..."
	
	cd ${IB_SO3_PATH}/so3
	make
}

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/so3*
}
addtask do_clean
