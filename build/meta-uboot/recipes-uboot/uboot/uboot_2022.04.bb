
SUMMARY = "U-boot Universal Bootloader"
DESCRIPTION = "U-boot bootloader receipt"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "2022.04"

inherit filesystem
inherit uboot
inherit bsp

SRCREV = "e4b6ebd3de982ae7185dbf689a030e73fd06e0d2"

SRC_URI = "git://github.com/u-boot/u-boot;branch=master;protocol=https"

# Set of patches to be applied to get a version adapted
# and adding various defconfig files.

FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc
 
# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_UBOOT_PATH}"

# Future enhancement with ATF
# do_configure[depends] += "atf:do_build"
# do_configure[depends] += "bsp-linux:do_build_firmware"

# The following code is here as example, but actually currently not used

do_configure[nostamp] = "1"
do_configure () {
	
	cd ${IB_TARGET}
	make ${IB_PLATFORM}_defconfig
	
	# Specific handling for bbb platform
	if [ "${IB_PLATFORM}" = "bbb" ]; then
	
		# If a disk image is used with BBB, it is intended to be
		# deployed in the eMMC, hence some different uEnv configurations
		
		if [ "${IB_STORAGE}" = "soft" ]; then
			ln -fs ${IB_UBOOT_PATH}/uEnv.d/uEnv_bbb_flash.txt ${IB_UBOOT_PATH}/uEnv.d/uEnv_bbb.txt 
		else
			ln -fs ${IB_UBOOT_PATH}/uEnv.d/uEnv_bbb_sd.txt ${IB_UBOOT_PATH}/uEnv.d/uEnv_bbb.txt 
		fi

	elif [ "${IB_PLATFORM}" = "imx8_colibri" ]; then

		ln -fs ${IB_ATF_PATH}/build/imx8qx/release/bl31.bin .
		echo "--> ${IB_BSP_PATH}"
		cp ${IB_BSP_PATH}/mx8qxc0-ahab-container.img mx8qx-ahab-container.img
		
	fi
	
}

do_build[nostamp] = "1"
do_build () {
	
	bbplain "Building U-boot with ${CORES} cores..."
	cd ${IB_TARGET}
	make -j${CORES}
}

def __do_platform_deploy(d):
    import os
    import subprocess
   
    src_dir = d.getVar('IB_BSP_PATH')
    dst_dir = d.getVar('IB_FILESYSTEM_PATH')
    u_boot_path = d.getVar('IB_UBOOT_PATH')
    IB_PLATFORM = d.getVar('IB_PLATFORM')
    
    if IB_PLATFORM == "rpi4_64":
         
        cmd = f"cp -r {src_dir}/rpi4/* {dst_dir}/p1/"
        result = subprocess.run(cmd, shell=True, check=True)

        cmd = f"cp {u_boot_path}/u-boot.bin {dst_dir}/p1/kernel8.img"
        result = subprocess.run(cmd, shell=True, check=True)

    else:
        cmd = f"cp {u_boot_path}/u-boot.bin {dst_dir}/p1/"
        result = subprocess.run(cmd, shell=True, check=True)
 
do_deploy[nostamp] = "1"
python do_deploy() {
    
    bb.plain("Deploy U-boot only ...")

    __do_deploy_boot(d);
}
 
addtask do_deploy

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/uboot*
}
addtask do_clean
