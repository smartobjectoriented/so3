# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "Linux Board Support Package"
DESCRIPTION = "Linux Board Support Package (BSP) which builds the whole set of software components \
		to be deployed on the target hardware."

LICENSE = "GPLv2"

# Version and revision
PV = "1.0"
PR = "r0"

inherit filesystem
inherit linux
inherit logging
inherit bsp
inherit uboot
inherit atf

OVERRIDES += ":linux"

COMPATIBLE_PLATFORM = "virt32|virt64|rpi4_64"

do_attach_infrabase[noexec] = "1"

# Platform-specific deploy logic (__do_platform_deploy).
include recipes-bsp/bsp/files/bsp_${IB_PLATFORM}.inc

do_configure[noexec] = "1"

# Building all components

do_build[depends] = "usr-linux:do_build uboot:do_build"

do_build () {
	bbplain "Everything built OK ..."
}
addtask do_build

####################### Recipe to deploy everything

do_itb[nostamp] = "1"

do_itb () {

	if [ "${IB_BOOT_CHAIN}" = "full" ]; then
		# AVZ mode: wrap Linux (agency) into the AVZ ITB.

		if [ ! -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ]; then
			bbfatal "No corresponding ITS found (${IB_TARGET_ITS})"
		fi
		mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_PLATFORM}_avz.itb
	else
		# Bare bsp-linux (IB_BOOT_CHAIN ∈ {uboot, atf+uboot}): single
		# plain ITB from ${IB_PLATFORM}.its with the buildroot initrd
		# bundled in. Direct bootm by U-Boot, no AVZ wrapping.

		if [ ! -f ${IB_ITB_PATH}/${IB_PLATFORM}.its ]; then
			bbfatal "No bare ITS found at ${IB_ITB_PATH}/${IB_PLATFORM}.its"
		fi
		mkimage -f ${IB_ITB_PATH}/${IB_PLATFORM}.its ${IB_ITB_PATH}/${IB_PLATFORM}.itb
	fi
}

# do_prepare_initrd: gzip rootfs.cpio (produced by usr-linux:do_deploy)
# into initrd.cpio.gz so do_itb /incbin/'s it. Lived in the FC capsule
# bbappend originally; moved here so bare bsp-linux (no capsule layer
# loaded) still gets a fresh initrd in the bare ITB. The FC bbappend's
# do_inject_kernel_modules still runs before this, editing rootfs.cpio
# in place — the content-hash guard below picks up the new content and
# regenerates initrd.cpio.gz.

do_prepare_initrd[nostamp] = "1"
do_prepare_initrd[depends] = "usr-linux:do_deploy"

python do_prepare_initrd () {
    import hashlib
    import shutil
    import gzip
    import os

    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    IB_PLATFORM    = d.getVar('IB_PLATFORM')

    board_dir      = os.path.join(IB_ROOTFS_PATH, "board", IB_PLATFORM)
    src            = os.path.join(board_dir, "rootfs.cpio")
    initrd         = os.path.join(board_dir, "initrd.cpio")
    initrd_gz      = initrd + ".gz"
    src_hash_file  = src + ".sha256"

    if not os.path.exists(src):
        bb.fatal("rootfs.cpio not found: {}".format(src))

    # Content hash instead of mtime: __do_rootfs_umount always rewrites
    # rootfs.cpio with a fresh mtime even when content is unchanged, so
    # mtime would force false rebuilds. sha256 fixes both directions.

    h = hashlib.sha256()
    with open(src, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    current_hash = h.hexdigest()

    if os.path.exists(initrd_gz) and os.path.exists(src_hash_file):
        with open(src_hash_file, 'r') as f:
            stored_hash = f.read().strip()
        if stored_hash == current_hash:
            bb.plain("initrd.cpio.gz is up to date, skipping")
            return

    bb.plain("Prepare initrd.cpio.gz from rootfs.cpio")
    shutil.copy2(src, initrd)
    with open(initrd, 'rb') as f_in, gzip.open(initrd_gz, 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)
    with open(src_hash_file, 'w') as f:
        f.write(current_hash)
}

addtask do_prepare_initrd before do_itb

# Deploy everything

do_deploy[depends] = "filesystem:do_fs_check usr-linux:do_deploy"

do_deploy[nostamp] = "1"
python do_deploy() {

    bb.plain("Deploy Linux boot (u-boot, itb)")

    __do_deploy_boot(d);
}

# Wire do_itb before both do_build (so `build.sh -a` produces the
# .itb artefacts) and do_deploy (so a standalone `deploy.sh -a` still
# triggers the assembly through sstate misses).
addtask do_itb before do_build before do_deploy
addtask do_deploy

do_deploy_boot[nostamp] = "1"
do_deploy_boot[depends] = "filesystem:do_fs_check"

python do_deploy_boot() {

    bb.plain("Deploy Linux boot (u-boot, itb)")

    __do_deploy_boot(d)
}
addtask do_itb before do_deploy_boot
addtask do_deploy_boot

do_clean[depends] = "usr-linux:do_clean rootfs-linux:do_clean linux:do_clean uboot:do_clean"
python () {
    if d.getVar('IB_PLATFORM') == 'virt64':
        d.appendVarFlag('do_clean', 'depends', ' atf:do_clean optee:do_clean avz:do_clean')
}
do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/bsp-linux*
}
addtask do_clean
