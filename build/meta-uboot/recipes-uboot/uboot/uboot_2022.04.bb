# Copyright (c) 2025-2026 EDGEMTech SA

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

# ATF is only needed when the boot chain actually carries it (FIP-loaded
# U-Boot in atf+uboot / full modes). In "uboot" mode QEMU's -bios loads
# u-boot.bin directly — no BL31 in the chain, no reason to build ATF.
# bb.utils.contains_any returns the second arg when IB_BOOT_CHAIN is in
# the set, else the third arg.

do_configure[depends] = "${@bb.utils.contains_any('IB_BOOT_CHAIN', 'atf+uboot full', 'atf:do_build', '', d)}"

do_configure[nostamp] = "1"
do_configure () {

	cd ${IB_TARGET}

	# Defconfig selection on virt64:
	#   - IB_BOOT_CHAIN="uboot" (bare bsp-linux, no ATF):
	#     upstream qemu_arm64_defconfig — CONFIG_POSITION_INDEPENDENT=y
	#     and CONFIG_ARCH_QEMU=y, both required for QEMU `-kernel` to
	#     load the U-Boot ELF at any address. Linux runs at EL1 (no
	#     secure / no virtualization in QEMU machine).
	#   - IB_BOOT_CHAIN="atf+uboot" (bare bsp-torizon) and "full" (capsule)
	#     both use virt64_defconfig — same FIP-aware U-Boot, the boot
	#     chain difference lives in ATF (no SPD vs SPD=opteed) and the
	#     guest DT (see bsp_torizon doc, "Known virt64 DT Workarounds").
	#     U-Boot is entered at EL2 by ATF and does NOT switch to EL1
	#     before booting the kernel — the kernel performs its own
	#     el2_setup and drops to EL1 itself, as per the arm64 boot
	#     protocol.
	#   - verdin platforms: ${IB_PLATFORM}_defconfig (verdin uses
	#     uboot_2024.07.bb anyway).

	if [ "${IB_PLATFORM}" = "virt64" ] && [ "${IB_BOOT_CHAIN}" = "uboot" ]; then
		make qemu_arm64_defconfig
	else
		make ${IB_PLATFORM}_defconfig
	fi

	# Specific handling for bbb platform
	if [ "${IB_PLATFORM}" = "bbb" ]; then

		# If a disk image is used with BBB, it is intended to be
		# deployed in the eMMC, hence some different uEnv configurations

		if [ "${IB_STORAGE_MODE}" = "soft" ]; then
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

	bbplain "Building U-boot with ${CORES} cores (CROSS_COMPILE=${IB_TOOLCHAIN}-)..."
	cd ${IB_TARGET}
	# Pin the cross compiler per platform (IB_TOOLCHAIN) so the build does
	# not depend on a CROSS_COMPILE inherited from the caller's shell —
	# e.g. virt32 (arm-linux-gnueabihf) vs virt64 (aarch64-none-linux-gnu).
	make CROSS_COMPILE=${IB_TOOLCHAIN}- -j${CORES}

	if [ "${IB_PLATFORM}" = "virt64" ]; then
		mkdir -p ${IB_DIR}/filesystem
		cp ${IB_UBOOT_PATH}/u-boot.bin ${IB_DIR}/filesystem/
	fi
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
