
SUMMARY = "AVZ Hypervisor"
DESCRIPTION = "AVZ (Agency Virtualizer) hypervisor based on polymorphic SO3 Operating System"
LICENSE = "GPLv2"

inherit avz

# Version and revision
 
PR = "r0"
PV = "6.1.0"

OVERRIDES += ":avz"

# AVZ is built in place from the in-tree sources — avz/ is the
# out-of-tree (O=) build dir whose stub Makefile points at the kernel
# source (so3/so3). No upstream fetch.
IB_TARGET = "${IB_AVZ_PATH}"

do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_attach_infrabase[noexec] = "1"

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_CONFIG}
}

do_build () {
	echo "Building AVZ..."

	cd ${IB_TARGET}
	make
}

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/avz*
}
addtask do_clean
