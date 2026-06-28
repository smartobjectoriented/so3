# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "SO3 Board Support Package"
DESCRIPTION = "SO3 Board Support Package (BSP) which builds the whole set of software components \
		to be deployed on the target hardware."

LICENSE = "GPLv2"

# Version and revision
PV = "1.0.0"
PR = "r0"

inherit filesystem
inherit uboot
inherit logging
inherit bsp

# ITS templates live in the layer (rendered into IB_ITB_PATH by do_itb)
IB_ITS_SRC = "${THISDIR}/files/its"

# :append (not +=) so no space is inserted before ":so3" — otherwise the
# preceding CPU token parses as "arm "/"aarch64 " and :<cpu> overrides
# stop matching. See usr-so3_1.0.bb for the full rationale.
OVERRIDES:append = ":so3"

include ../bsp/files/bsp_${IB_PLATFORM}.inc

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

# Building all components.
#
# Every platform needs U-Boot: on QEMU it is the bare U-Boot payload, on
# verdin-imx8mp it is U-Boot wrapped into imx-boot/flash.bin (ATF + OP-TEE +
# U-Boot) by the uboot recipe. verdin additionally needs the AVZ hypervisor
# (bundled by the AVZ+SO3 ITS), so its dependency set is overridden to add it.
IB_BSP_BUILD_DEPENDS = "usr-so3:do_build uboot:do_build"
IB_BSP_BUILD_DEPENDS:verdin-imx8mp = "usr-so3:do_build uboot:do_build avz:do_build"
do_build[depends] = "${IB_BSP_BUILD_DEPENDS}"

do_build () {
	bbplain "Everything built OK ..."
}
addtask do_build

####################### Recipe to deploy everything

do_itb[nostamp] = "1"
do_itb[depends] = "usr-so3:do_deploy"
do_itb () {

	if [ ! -f ${IB_ITS_SRC}/${IB_TARGET_ITS}.its ]; then
		bbfatal "No corresponding ITS found (${IB_TARGET_ITS})"
	else
		bsp_render_its ${IB_TARGET_ITS}
		mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_TARGET_ITS}.itb
	fi

	# AVZ boot uses a SEPARATE guest ITB (loaded alongside the AVZ ITB by
	# the e1c-boot U-Boot command). The guest ITS is derived from the
	# selected AVZ ITS: <plat>_avz -> <plat>_so3_guest (deriving from
	# IB_TARGET_ITS, not IB_PLATFORM, keeps the underscore naming on
	# platforms whose IB_PLATFORM carries a hyphen, e.g. verdin-imx8mp).
	case "${IB_TARGET_ITS}" in
	*_avz)
		guest_its="$(echo "${IB_TARGET_ITS}" | sed 's/_avz$//')${IB_GUEST_SUFFIX}"
		if [ -f ${IB_ITS_SRC}/${guest_its}.its ]; then
			bsp_render_its ${guest_its}
			mkimage -f ${IB_ITB_PATH}/${guest_its}.its ${IB_ITB_PATH}/${guest_its}.itb
		fi
		;;
	esac

}

do_deploy[depends] = "usr-so3:do_deploy"
 
do_deploy[nostamp] = "1"
python do_deploy() {
    
    bb.plain("Deploy SO3 image and U-boot")

    __do_deploy_boot(d);
}

addtask do_itb before do_deploy
addtask do_deploy

do_deploy_boot[nostamp] = "1"
python do_deploy_boot() {

    bb.plain("Deploy SO3 boot (u-boot, itb)")

    __do_deploy_boot(d)
}
addtask do_itb before do_deploy_boot
addtask do_deploy_boot

do_clean[depends] = "usr-so3:do_clean so3:do_clean uboot:do_clean"
do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/bsp-so3*
}
addtask do_clean

