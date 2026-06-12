
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

OVERRIDES += ":so3"

include ../bsp/files/bsp_${IB_PLATFORM}.inc

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

# Building all components

do_build[depends] = "usr-so3:do_build uboot:do_build" 

do_build () {
	bbplain "Everything built OK ..."
}
addtask do_build

####################### Recipe to deploy everything

do_itb[nostamp] = "1"
do_itb[depends] = "usr-so3:do_deploy"
do_itb () {

	if [ ! -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ]; then
		bbfatal "No corresponding ITS found (${IB_TARGET_ITS})"
	else
		mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_TARGET_ITS}.itb
	fi
	
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

