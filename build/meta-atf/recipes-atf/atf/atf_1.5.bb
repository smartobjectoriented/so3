
SUMMARY = "ATF firmware"
DESCRIPTION = "ARM Trusted Firmware (ATF)"

LICENSE = "GPLv2"

inherit atf

# Version and revision
PR = "r0"
PV = "1.5"

SRCREV = "78b1610e31d9a5dbd16553b8a2ac99000a7379f7"

SRC_URI = "git://git.trustedfirmware.org/TF-A/trusted-firmware-a.git;branch=master;protocol=https"

# To force the task to be re-executed
do_build[nostamp] = "1"
do_configure[noexec] = "1"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_ATF_PATH}"

do_build () {

	# Currently, only imx8mp-verdin is supported.

	if [ "${IB_PLATFORM}" = "verdin-imx8mp" ]; then
		do_build_bl31
	fi
}

addtask do_build

