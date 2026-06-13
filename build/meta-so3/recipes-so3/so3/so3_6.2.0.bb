
SUMMARY = "SO3 kernel"
DESCRIPTION = "Smart Object Oriented Operating System"
LICENSE = "GPLv2"

inherit so3

# Version and revision
PR = "r0"
PV = "6.2.0"

OVERRIDES += ":so3"

# This repository IS the SO3 source tree, so the kernel is built in
# place — there is no upstream fetch. The fetch/unpack/attach tasks
# (which would clone github and overwrite ${IB_SO3_PATH}) are disabled.
IB_TARGET = "${IB_SO3_PATH}"

do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_attach_infrabase[noexec] = "1"

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
