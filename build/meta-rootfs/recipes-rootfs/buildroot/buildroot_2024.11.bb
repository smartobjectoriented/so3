# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "Buildroot rottfs"
DESCRIPTION = "Buildroot is used as rootfs generator"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "2024.11"

OVERRIDES += ":linux"

inherit rootfs

SRC_URI = "https://buildroot.org/downloads/buildroot-2024.11.tar.gz;protocol=https"
SRC_URI[sha256sum] = "4a601600b846058c2710cfda7d152d5d820b433ff4a4bce65c7eeb49f87e5540"

# defconfig patches and auto-getty patches
FILESPATH:prepend: := "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc
 
# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_ROOTFS_PATH}/buildroot"

# BR2_DL_DIR: buildroot download cache. Defaults to ${HOME}/.buildroot-dl
# — same persistence pattern as BR2_CCACHE_DIR in the defconfigs. Survives
# workspace cleanups in local builds AND in CI (gitlab-runner $HOME is
# stable), saves ~hundreds of MB of tarball re-downloads. Override via
# env or local.conf if a different location is needed (e.g. multi-user
# host where each user wants their own cache).
IB_BUILDROOT_DL_DIR ?= "${HOME}/.buildroot-dl"

# Not stamped: the stamp ignores IB_PLATFORM, so a platform switch kept the
# previous platform's .config. A hash of the platform and its defconfig decides
# instead, and a platform switch also drops the previous output, since buildroot
# cannot change architecture in place.

do_configure[nostamp] = "1"

IB_BUILDROOT_DEFCONFIG_GUARD = "${WORKDIR}/buildroot_defconfig.sha256"
IB_BUILDROOT_PLATFORM = "${IB_ROOTFS_PATH}/.ib_platform"

do_configure () {
	cd ${IB_TARGET}

	defconfig="${IB_TARGET}/configs/${IB_PLATFORM}_defconfig"

	if [ ! -f "$defconfig" ]; then
		bbfatal "${IB_PLATFORM}_defconfig is missing in buildroot/configs ..."
	fi

	defconfig_hash=$( { echo "${IB_PLATFORM}"; cat "$defconfig"; } | sha256sum | cut -d' ' -f1)

	if [ -f "${IB_BUILDROOT_DEFCONFIG_GUARD}" ] && \
	   [ "$(cat ${IB_BUILDROOT_DEFCONFIG_GUARD})" = "$defconfig_hash" ] && \
	   [ -f "${IB_ROOTFS_PATH}/.config" ]; then
		bbplain "buildroot: ${IB_PLATFORM}_defconfig unchanged — keeping the existing .config"
		return 0
	fi

	if [ "$(cat ${IB_BUILDROOT_PLATFORM} 2>/dev/null)" != "${IB_PLATFORM}" ]; then
		bbplain "buildroot: platform is now ${IB_PLATFORM}, rebuilding the rootfs from scratch"
		rm -rf ${IB_ROOTFS_PATH}/build ${IB_ROOTFS_PATH}/target ${IB_ROOTFS_PATH}/host \
		       ${IB_ROOTFS_PATH}/scripts ${IB_ROOTFS_PATH}/images ${IB_ROOTFS_PATH}/staging
	fi

	mkdir -p "${IB_BUILDROOT_DL_DIR}"
	make O=${IB_ROOTFS_PATH} BR2_DL_DIR="${IB_BUILDROOT_DL_DIR}" ${IB_PLATFORM}_defconfig

	# Recorded only after a successful reconfiguration, same as do_build.
	echo "${IB_PLATFORM}" > "${IB_BUILDROOT_PLATFORM}"
	echo "$defconfig_hash" > "${IB_BUILDROOT_DEFCONFIG_GUARD}"
}

do_build[nostamp] = "1"

# post_image.sh writes rootfs.cpio into ${IB_ROOTFS_PATH}/board/${IB_PLATFORM},
# which is the board working copy + symlink set up by rootfs-linux:do_attach_
# infrabase under tmp/work. Make sure that copy exists before we build,
# otherwise post_image.sh has nowhere to deposit rootfs.cpio on a fresh tree.
# (buildroot here is the linux rootfs generator only — OVERRIDES += ":linux".)

do_build[depends] += "rootfs-linux:do_attach_infrabase"

do_build () {
	bbnote "Building buildroot based rootfs..."

	cd ${IB_TARGET}
	make  O=${IB_ROOTFS_PATH} BR2_DL_DIR="${IB_BUILDROOT_DL_DIR}" --no-print-directory
}

do_clean[nostamp] = "1"

do_clean () {
	bbnote "Cleaning the entire rootfs..."
	
	rm -rf ${IB_ROOTFS_PATH}/build
	rm -rf ${IB_ROOTFS_PATH}/target
	rm -rf ${IB_ROOTFS_PATH}/host
	rm -rf ${IB_ROOTFS_PATH}/scripts
	rm -rf ${IB_ROOTFS_PATH}/images
	rm -rf ${IB_ROOTFS_PATH}/staging

	rm -f ${TMPDIR}/stamps/buildroot*
}
addtask do_clean


