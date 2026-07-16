# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "U-boot Universal Bootloader"
DESCRIPTION = "U-boot bootloader receipt"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "2024.07"

inherit filesystem
inherit uboot
inherit bsp
inherit atf

SRCREV = "3f772959501c99fbe5aa0b22a36efe3478d1ae1c"
SRC_URI = "git://gitlab.com/u-boot/u-boot.git;nobranch=1;protocol=https"

# Set of patches to be applied to get a version adapted 
# and adding various defconfig files.

FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc
 
# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_UBOOT_PATH}"

# Future enhancement with ATF
do_configure[depends] += "atf:do_build"

# Verdin's do_configure shells out to OP-TEE's gen_tee_bin.py and
# consumes tee.elf to produce ./tee.bin embedded in flash.bin. Optee
# must be built (and its source attached at ${IB_OPTEE_PATH}) first.
# Gated on platform via anonymous python because the optee path is
# wrapped in `if [ "${IB_PLATFORM}" = "verdin-imx8mp" ]` below — virt64
# and other targets don't touch optee here.
python () {
    if d.getVar('IB_PLATFORM') == 'verdin-imx8mp':
        d.appendVarFlag('do_configure', 'depends', ' optee:do_build')
}

do_configure[nostamp] = "1"
do_configure () {

	cd ${IB_TARGET}
	make ${IB_PLATFORM}_defconfig

	# Hand the OS off at EL1 instead of EL2 (SO3 standalone is EL1-native;
	# AVZ needs EL2, so this is opt-in via IB_UBOOT_SWITCH_EL1 in local.conf).
	if [ "${IB_UBOOT_SWITCH_EL1}" = "1" ]; then
		echo "CONFIG_ARMV8_SWITCH_TO_EL1=y" >> .config
		make olddefconfig
	fi

    # NXP DDR training firmware (lpddr4 PMU), vendored capsule-free under
    # meta-bsp; IB_IMX_FIRMWARE_PATH (local.conf) is one flat directory.
    IMX8MP_FW_PATH="${IB_IMX_FIRMWARE_PATH}"

	if [ "${IB_PLATFORM}" = "verdin-imx8mp" ]; then

		cp ${IB_ATF_PATH}/build/imx8mp/release/bl31.bin .
		# Use raw binary (no OPTE v1 header): ATF BL31-only mode jumps directly
		# to BL32_BASE so the binary must start with _start, not a header.
		python3 ${IB_OPTEE_PATH}/scripts/gen_tee_bin.py \
			--input ${IB_OPTEE_PATH}/out/arm-plat-imx/core/tee.elf \
			--out_tee_raw_bin ./tee.bin

        cp ${IMX8MP_FW_PATH}/lpddr4*_202006.bin .
	fi
}

addtask do_configure

# do_build must run after do_configure so atf:do_build (a do_configure dep) is triggered first
addtask do_build after do_configure

do_build[nostamp] = "1"
do_build[depends] += "atf:do_build"
do_build () {

	bbplain "Building U-boot with ${CORES} cores..."
	cd ${IB_TARGET}
	if [ "${IB_PLATFORM}" = "verdin-imx8mp" ]; then
		BL31="${IB_ATF_PATH}/build/imx8mp/release/bl31.bin"
		TEE_ELF="${IB_OPTEE_PATH}/out/arm-plat-imx/core/tee.elf"
		HASH_FILE="flash.bin.inputs.sha256"
		CURRENT_HASH="$(sha256sum "$BL31" "$TEE_ELF" 2>/dev/null | sha256sum | cut -d' ' -f1)"
		if [ -f flash.bin ] && [ -f "$HASH_FILE" ] && [ "$(cat $HASH_FILE)" = "$CURRENT_HASH" ]; then
			echo "U-Boot flash.bin is up to date, skipping rebuild"
		else
			make CROSS_COMPILE=${IB_TOOLCHAIN}- TEE=./tee.bin -j${CORES}
			echo "$CURRENT_HASH" > "$HASH_FILE"
		fi
	else
		make CROSS_COMPILE=${IB_TOOLCHAIN}- -j${CORES}
	fi
}

def __do_platform_deploy(d):
    import os
    import subprocess

    src_dir = d.getVar('FILE_DIRNAME')
    dst_dir = d.getVar('IB_FILESYSTEM_PATH')
    u_boot_path = d.getVar('IB_UBOOT_PATH')
    IB_PLATFORM = d.getVar('IB_PLATFORM')

    if IB_PLATFORM == "verdin-imx8mp":

        cmd = f"cp -r {u_boot_path}/flash.bin {dst_dir}/p1/imx-boot"
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
