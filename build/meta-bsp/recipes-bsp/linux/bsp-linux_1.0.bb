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

# ITS templates live in the layer (rendered into IB_ITB_PATH by do_itb)
IB_ITS_SRC = "${THISDIR}/files/its"

# AVZ two-ITB boot: the Linux agency guest ITB is <plat>_linux_guest.itb
# (the AVZ ITB carries only the hypervisor, like the SO3 case).
IB_GUEST_SUFFIX = "_linux_guest"

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

	# ITS are rendered into IB_ITB_PATH by the shared do_render_its (before
	# do_itb); this task only mkimage's them.
	if [ "${IB_BOOT_CHAIN}" = "full" ]; then
		# AVZ mode: two ITBs (e1c-boot) — the AVZ hypervisor ITB and a
		# SEPARATE Linux agency guest ITB, mirroring the SO3-on-AVZ model.

		# AVZ hypervisor ITB
		if [ ! -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ]; then
			bbfatal "No corresponding ITS found (${IB_TARGET_ITS})"
		fi
		mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_TARGET_ITS}.itb

		# Linux agency guest ITB (loaded by AVZ from x1 via e1c-boot)
		guest_its="$(echo "${IB_TARGET_ITS}" | sed 's/_avz$//')${IB_GUEST_SUFFIX}"
		if [ ! -f ${IB_ITB_PATH}/${guest_its}.its ]; then
			bbfatal "No Linux guest ITS found (${guest_its})"
		fi
		mkimage -f ${IB_ITB_PATH}/${guest_its}.its ${IB_ITB_PATH}/${guest_its}.itb
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

# do_prepare_initrd: gzip the selected cpio into initrd.cpio.gz so do_itb
# /incbin/'s it. Lived in the FC capsule bbappend originally; moved here so
# bare bsp-linux (no capsule layer loaded) still gets a fresh initrd in the
# bare ITB. The FC bbappend's do_inject_kernel_modules still runs before
# this, editing rootfs.cpio in place — the content-hash guard below picks
# up the new content and regenerates initrd.cpio.gz.
#
# IB_RAMFS_SOURCE selects which cpio becomes the embedded ramfs (default in
# bsp.bbclass):
#   "rootfs" - board/<plat>/rootfs.cpio, the freshly built full rootfs.
#   "initrd" - board/<plat>/initrd.cpio, a static git-tracked busybox ramfs.
# Either way the result is gzipped straight into initrd.cpio.gz; we never
# overwrite the (now meaningful) static initrd.cpio.

do_prepare_initrd[nostamp] = "1"
do_prepare_initrd[depends] = "usr-linux:do_deploy"

python do_prepare_initrd () {
    import hashlib
    import shutil
    import gzip
    import os

    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    IB_PLATFORM    = d.getVar('IB_PLATFORM')
    ramfs_source   = (d.getVar('IB_RAMFS_SOURCE') or "rootfs").strip()

    board_dir      = os.path.join(IB_ROOTFS_PATH, "board", IB_PLATFORM)
    initrd_gz      = os.path.join(board_dir, "initrd.cpio.gz")

    if ramfs_source == "rootfs":
        src = os.path.join(board_dir, "rootfs.cpio")
    elif ramfs_source == "initrd":
        src = os.path.join(board_dir, "initrd.cpio")
    else:
        bb.fatal("IB_RAMFS_SOURCE must be 'rootfs' or 'initrd', got '{}'".format(ramfs_source))

    if not os.path.exists(src):
        bb.fatal("ramfs source not found: {} (IB_RAMFS_SOURCE={})".format(src, ramfs_source))

    # Guard against rebuilds. Content hash instead of mtime: __do_rootfs_umount
    # always rewrites rootfs.cpio with a fresh mtime even when content is
    # unchanged, so mtime would force false rebuilds. The mode is folded into
    # the stored value so toggling IB_RAMFS_SOURCE always invalidates the gz,
    # even if the new source happens to hash to a previously seen value.

    h = hashlib.sha256()
    with open(src, 'rb') as f:
        for chunk in iter(lambda: f.read(65536), b''):
            h.update(chunk)
    current_hash = "{}:{}".format(ramfs_source, h.hexdigest())

    # Single mode-aware guard file, decoupled from the source filename.
    src_hash_file = initrd_gz + ".srchash"

    if os.path.exists(initrd_gz) and os.path.exists(src_hash_file):
        with open(src_hash_file, 'r') as f:
            stored_hash = f.read().strip()
        if stored_hash == current_hash:
            bb.plain("initrd.cpio.gz is up to date, skipping")
            return

    bb.plain("Prepare initrd.cpio.gz from {} (IB_RAMFS_SOURCE={})".format(
        os.path.basename(src), ramfs_source))
    with open(src, 'rb') as f_in, gzip.open(initrd_gz, 'wb') as f_out:
        shutil.copyfileobj(f_in, f_out)
    with open(src_hash_file, 'w') as f:
        f.write(current_hash)
}

addtask do_prepare_initrd before do_itb

# Deploy everything
#
# Deploy is decoupled from the build: it writes the already-built artefacts
# onto the boot media WITHOUT recompiling. It pulls rootfs-linux:do_deploy
# (which extracts the final rootfs.cpio — apps baked in at build time — onto
# p2) and writes the .itb produced by do_itb during `build.sh -a` onto p1
# (__do_platform_deploy). It does NOT pull do_build / do_itb / usr-linux's
# rootfs.cpio populate — those belong to `build.sh -a`. Workflow:
# edit -> build.sh -> deploy.sh. A deploy with no prior build fails clearly
# (missing rootfs.cpio / .itb) rather than silently rebuilding.

do_deploy[depends] = "filesystem:do_fs_check rootfs-linux:do_deploy"

do_deploy[nostamp] = "1"
python do_deploy() {

    bb.plain("Deploy Linux boot (u-boot, itb)")

    __do_deploy_boot(d);
}

# do_itb runs in the BUILD chain only (it produces the .itb that deploy
# consumes); deploy must not re-trigger it.
addtask do_itb before do_build
addtask do_deploy

do_deploy_boot[nostamp] = "1"
do_deploy_boot[depends] = "filesystem:do_fs_check"

python do_deploy_boot() {

    bb.plain("Deploy Linux boot (u-boot, itb)")

    __do_deploy_boot(d)
}
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
