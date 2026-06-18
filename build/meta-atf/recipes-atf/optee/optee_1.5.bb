# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "OPTEE-OS firmware"
DESCRIPTION = "Open Trusted Execution Environment OS"

LICENSE = "GPLv2"

inherit atf

# Version and revision
PR = "r0"
PV = "1.5"

SRC_URI = "git://github.com/OP-TEE/optee_os.git;nobranch=1;protocol=https"

SRCREV = "cf2504f5f89ef97c7ed7b04b3c0f639f0b014fbb"

# Make sure atf is attached before attaching optee, since optee is
# a sub-directory of atf

do_attach_infrabase[depends] = "atf:do_attach_infrabase"

# To force the task to be re-executed
do_build[nostamp] = "1"
do_configure[noexec] = "1"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_OPTEE_PATH}"

do_build () {

	if [ "${IB_PLATFORM}" = "verdin-imx8mp" ]; then
		TEE_ELF="${IB_OPTEE_PATH}/out/arm-plat-imx/core/tee.elf"
		HASH_FILE="${TEE_ELF}.sha256"
		OLD_HASH="$(cat $HASH_FILE 2>/dev/null || true)"
		NEW_HASH="$(sha256sum $TEE_ELF 2>/dev/null | cut -d' ' -f1 || true)"
		if [ -f "$TEE_ELF" ] && [ -n "$NEW_HASH" ] && [ "$OLD_HASH" = "$NEW_HASH" ]; then
			echo "OP-TEE tee.elf is up to date, skipping rebuild"
		else
			do_build_optee
			sha256sum "$TEE_ELF" | cut -d' ' -f1 > "$HASH_FILE"
		fi
	elif [ "${IB_PLATFORM}" = "virt64" ]; then
		do_build_optee
		mkdir -p ${IB_DIR}/filesystem
		cp ${IB_OPTEE_PATH}/out/arm-plat-vexpress/core/tee-pager_v2.bin ${IB_DIR}/filesystem/
	fi
}

addtask do_build

do_clean[nostamp] = "1"
do_clean () {
	rm -rf ${IB_OPTEE_PATH}/out
}
addtask do_clean
