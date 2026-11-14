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

# Content-hash guard, same pattern as bsp-linux:do_prepare_initrd.
#
# The task stays nostamp, since bitbake cannot see what buildroot consumes.
# But make is not free when nothing changed: buildroot re-runs target-finalize
# and the image steps every time, and regenerates rootfs.cpio, discarding
# anything injected into it after the build. So hash what buildroot consumes
# and skip the make while it is unchanged:
#
#   IB_PLATFORM              a platform switch always rebuilds
#   .config                  the resolved configuration
#   buildroot.attach.sha256  the manifest standing for the patched source
#   board/                   the BR2_EXTERNAL tree, both its git-tracked source
#                            and the working copy rootfs-linux attaches, minus
#                            the artefacts the build writes there
#
# Content rather than mtimes: the attach and the mount/umount cycle rewrite
# files without changing them. `build.sh -c buildroot` still forces a make.

IB_BUILDROOT_GUARD = "${WORKDIR}/buildroot_inputs.sha256"
IB_BUILDROOT_SRC_BOARD := "${THISDIR}/../linux/files/board"

# find -L: ${IB_ROOTFS_PATH}/board is a symlink, which find would not enter.
# Each element ends in "|| echo <marker>": under set -e one failing command
# would abort the group and turn the hash into a constant that never changes.

buildroot_hash_tree() {
	find -L "$1" -type f \
		! -name '*.cpio' ! -name '*.cpio.gz' ! -name '*.srchash' \
		! -name '*.backup' ! -name '*.sha256' \
		-exec sha256sum {} + 2>/dev/null | sort || echo "no-tree:$1"
}

buildroot_inputs_hash() {
	{ echo "${IB_PLATFORM}"
	  cat "${IB_ROOTFS_PATH}/.config" 2>/dev/null || echo "no-config"
	  cat "${IB_ROOTFS_PATH}/buildroot.attach.sha256" 2>/dev/null || echo "no-manifest"
	  buildroot_hash_tree "${IB_BUILDROOT_SRC_BOARD}"
	  buildroot_hash_tree "${IB_ROOTFS_PATH}/board"
	} | sha256sum | cut -d' ' -f1
}

# For convenience, images/ also shows the platform's static initrd.cpio next
# to rootfs.cpio. The link targets the git-tracked source, the file that
# mount.sh -i / umount.sh -i edit and the next build embeds, so it is never
# stale. It goes away with images/.

buildroot_link_initrd() {
	if [ -f ${IB_BUILDROOT_SRC_BOARD}/${IB_PLATFORM}/initrd.cpio ]; then
		ln -sfnr ${IB_BUILDROOT_SRC_BOARD}/${IB_PLATFORM}/initrd.cpio \
			${IB_ROOTFS_PATH}/images/initrd.cpio
	fi
}

do_build () {
	bbnote "Building buildroot based rootfs..."

	if [ -f "${IB_BUILDROOT_GUARD}" ] && \
	   [ "$(cat ${IB_BUILDROOT_GUARD})" = "$(buildroot_inputs_hash)" ] && \
	   [ -f "${IB_ROOTFS_PATH}/board/${IB_PLATFORM}/rootfs.cpio" ]; then
		bbplain "buildroot: inputs unchanged, rootfs.cpio present — skipping (build.sh -c buildroot forces it)"
		return 0
	fi

	cd ${IB_TARGET}
	make  O=${IB_ROOTFS_PATH} BR2_DL_DIR="${IB_BUILDROOT_DL_DIR}" --no-print-directory

	buildroot_link_initrd

	# Recorded only after a successful make, and re-hashed since the make may
	# touch its own inputs (.config normalisation, for one).

	buildroot_inputs_hash > "${IB_BUILDROOT_GUARD}"
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

	# The guard goes with the output it vouches for: board/<plat>/rootfs.cpio
	# survives this task, so a kept guard would skip the make next time.

	rm -f ${IB_BUILDROOT_GUARD}
}
addtask do_clean


