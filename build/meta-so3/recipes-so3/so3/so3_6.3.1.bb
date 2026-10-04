# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "SO3 kernel"
DESCRIPTION = "Smart Object Oriented Operating System"
LICENSE = "GPLv2"

inherit so3

# Version and revision
PR = "r0"
PV = "6.3.1"

# :append (not +=) so no space is inserted before ":so3" — otherwise the
# preceding CPU token parses as "arm "/"aarch64 " and :<cpu> overrides
# stop matching. See usr-so3_1.0.bb for the full rationale.
OVERRIDES:append = ":so3"

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

	# The kernel is built in place, so a stale .config/objects from the
	# other architecture (virt64<->virt32, i.e. aarch64<->arm) would
	# otherwise survive and produce a wrong-arch kernel. Track the last
	# built arch in a marker file and distclean only when it changes —
	# so same-arch rebuilds stay incremental.
	_arch_marker=".ib_last_arch"
	if [ -f "$_arch_marker" ] && [ "$(cat $_arch_marker)" != "${IB_PLAT_CPU}" ]; then
		echo "SO3 arch changed ($(cat $_arch_marker) -> ${IB_PLAT_CPU}); running make distclean"
		make distclean
	fi

	make ${IB_CONFIG}

	echo "${IB_PLAT_CPU}" > "$_arch_marker"
}

do_build () {
	echo "Building SO3..."

	cd ${IB_SO3_PATH}/so3
	make
}

# `deploy.sh so3`: the kernel travels in the ITB, which bsp-so3 repacks and
# writes onto p1 (bsp-so3:do_deploy_boot). No dependency on do_build —
# deploy writes what the last `build.sh so3` produced, it does not compile.
do_deploy[nostamp] = "1"
do_deploy[depends] = "bsp-so3:do_deploy_boot"
python do_deploy () {
    bb.plain("SO3 kernel deployed (ITB on p1)")
}
addtask do_deploy

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/so3*
}
addtask do_clean
