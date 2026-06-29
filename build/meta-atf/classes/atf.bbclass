# Copyright (c) 2025-2026 EDGEMTech SA

# Class for building ATF/OP-TEE in infrabase

IB_ATF_PATH = "${IB_DIR}/atf"
IB_OPTEE_PATH = "${IB_DIR}/atf/optee"

# Platform-specific extra options for ATF build (e.g. IMX_BOOT_UART_BASE for imx8mp)
IB_ATF_EXTRA_OPTS ?= ""

# OP-TEE platform identifier (e.g. vexpress-qemu_armv8a)
IB_OPTEE_PLAT ?= ""

# Platform-specific extra options for OP-TEE build (e.g. CFG_TZDRAM_START)
IB_OPTEE_EXTRA_OPTS ?= ""

# Detect IB_ATF_EXTRA_OPTS changes between invocations and force a clean
# rebuild when they differ — make's per-file dependency tracking does NOT
# pick up changes to compile-time flags like SPD= , so otherwise BL2 keeps
# the OP-TEE-loading code (or vice versa) when switching between
# IB_BOOT_CHAIN=full and IB_BOOT_CHAIN=atf+uboot.

__ib_atf_clean_if_opts_changed () {
	_marker="${IB_TARGET}/build/.ib-atf-extra-opts"
	_prev=""
	if [ -f "${_marker}" ]; then
		_prev="$(cat "${_marker}")"
	fi
	if [ "${_prev}" != "${IB_ATF_EXTRA_OPTS}" ]; then
		echo "IB_ATF_EXTRA_OPTS changed (was '${_prev}', now '${IB_ATF_EXTRA_OPTS}') — running make realclean"
		make CROSS_COMPILE=${IB_TOOLCHAIN}- PLAT=${IB_ATF_PLAT} realclean || true
		mkdir -p "${IB_TARGET}/build"
		printf '%s' "${IB_ATF_EXTRA_OPTS}" > "${_marker}"
	fi
}

do_build_bl31 () {

	echo "Building ARM Trusted Firmware with ${CORES} cores..."
	cd ${IB_TARGET}
	__ib_atf_clean_if_opts_changed
	make CROSS_COMPILE=${IB_TOOLCHAIN}- PLAT=${IB_ATF_PLAT} ${IB_ATF_EXTRA_OPTS} bl31 -j${CORES}
}

do_build_all_fiptool () {

	echo "Building ARM Trusted Firmware (all stages + fiptool) with ${CORES} cores..."
	cd ${IB_TARGET}
	__ib_atf_clean_if_opts_changed
	make CROSS_COMPILE=${IB_TOOLCHAIN}- PLAT=${IB_ATF_PLAT} ${IB_ATF_EXTRA_OPTS} all fiptool -j${CORES}
}

do_build_optee () {

	echo "Building OP-TEE OS with ${CORES} cores..."
	cd ${IB_TARGET}
	make CROSS_COMPILE64=${IB_TOOLCHAIN}- \
		PLATFORM=${IB_OPTEE_PLAT} CFG_ARM64_core=y CFG_USER_TA_TARGETS=ta_arm64 \
		${IB_OPTEE_EXTRA_OPTS} -j${CORES}
}

EXPORT_FUNCTIONS do_build_bl31 do_build_all_fiptool do_build_optee
