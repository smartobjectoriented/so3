
# Class for building U-boot in infrabase

IB_ATF_PATH = "${IB_DIR}/atf"

do_build_bl31 () {

	echo "Building ARM Trusted Firmware with ${CORES} cores..."
	cd ${IB_TARGET}
	make CROSS_COMPILE=${IB_TOOLCHAIN}- PLAT=${IB_ATF_PLAT} IMX_BOOT_UART_BASE=0x30880000 bl31 -j${CORES}
}

EXPORT_FUNCTIONS do_build_bl31
