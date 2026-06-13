
SUMMARY = "SO3 capsule Deployment"
DESCRIPTION = "SO3 capsules are aimed to run with Linux as guest on top of the AVZ hypervisor."

LICENSE = "GPLv2"

# Version and revision
PV = "1.0.0"
PR = "r0"

inherit filesystem
inherit uboot
inherit logging
inherit bsp

OVERRIDES += ":so3"

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

# Building all components

do_build[depends] = "usr-so3:do_build" 

do_build () {
	bbplain "Everything built OK ..."
}
addtask do_build

####################### Recipe to deploy everything

def __do_platform_deploy(d):

    import os
    import subprocess   

    capsule_path = d.getVar('IB_FILESYSTEM_PATH') + "/p2/mnt/capsules/image"
    itb_path = d.getVar('IB_ITB_PATH') + "/" + d.getVar('IB_TARGET_ITS') + ".itb"

    if not os.path.isfile(itb_path):
        bb.fatal(itb_path + " is missing ...")
 
    subprocess.run(['mkdir', '-p', capsule_path])
    subprocess.run(['cp', itb_path, capsule_path])

do_itb[nostamp] = "1"
do_itb[depends] = "usr-so3:do_deploy"
do_itb () {

	if [ ! -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ]; then
		bbfatal "No corresponding ITS found for container ${IB_TARGET_ITS}"
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

 
 

