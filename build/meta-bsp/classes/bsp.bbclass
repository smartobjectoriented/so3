

# Copyright (c) 2023-2026 EDGEMTech Ltd

# Class for BSP - Main recipe

# The link to the firmwares of all boards
IB_BSP_PATH = "${IB_DIR}/build/meta-bsp/recipes-bsp/bsp"

# Standard tree locations for the boot-chain components. atf.bbclass sets
# the same values, but BSP recipes that don't `inherit atf` still need
# these to assemble flash0.img / flash.bin in do_deploy_boot_chain.

IB_ATF_PATH = "${IB_DIR}/atf"
IB_OPTEE_PATH = "${IB_DIR}/atf/optee"

# ITB build/output directory (gitignored). The ITS *sources* live in each
# BSP recipe's files/its/ (IB_ITS_SRC) and reference the component trees via
# ${IB_*_PATH} variables; do_itb renders them here (bsp_render_its) before
# mkimage and writes the resulting .itb here too.
IB_ITB_PATH:so3 = "${IB_DIR}/so3/images"
IB_ITB_PATH:linux = "${IB_DIR}/linux/images"

# Component tree locations referenced from the ITS templates. Provided here
# (?=) so every BSP recipe can render any ITS regardless of which classes it
# inherits; the real definitions in so3/avz/linux .bbclass take precedence.
IB_SO3_PATH ?= "${IB_DIR}/so3"
IB_AVZ_PATH ?= "${IB_DIR}/avz"
IB_LINUX_PATH ?= "${IB_DIR}/linux/linux"

# AVZ two-ITB boot: the guest ITB basename is <IB_TARGET_ITS without _avz> +
# this suffix. SO3 guests use _so3_guest (default); the Linux agency overrides
# it to _linux_guest in bsp-linux.
IB_GUEST_SUFFIX ?= "_so3_guest"

# Which cpio feeds the embedded ramfs (the initrd.cpio.gz that the ITS
# /incbin/'s into the guest ITB). Both modes run-from-RAM; only the source
# differs:
#   "rootfs" (default) - the freshly built board/<plat>/rootfs.cpio (~30 MB,
#                        full buildroot rootfs). p2 holds the same content.
#   "initrd"           - the static, git-tracked board/<plat>/initrd.cpio
#                        (small busybox ramfs). p2 still receives the full
#                        rootfs.cpio; the kernel just boots the small initrd.
# Override per build in conf/local.conf, e.g. IB_RAMFS_SOURCE = "initrd".
IB_RAMFS_SOURCE ?= "rootfs"

# Render an ITS template from IB_ITS_SRC into IB_ITB_PATH, expanding the
# ${IB_*_PATH} / ${IB_PLATFORM} placeholders to absolute build paths. The sed
# patterns use char classes ([$][{]...[}]) so bitbake leaves them untouched
# and only the replacement side is expanded.
bsp_render_its() {
	mkdir -p "${IB_ITB_PATH}"
	sed -e "s|[$][{]IB_SO3_PATH[}]|${IB_SO3_PATH}|g" \
	    -e "s|[$][{]IB_AVZ_PATH[}]|${IB_AVZ_PATH}|g" \
	    -e "s|[$][{]IB_LINUX_PATH[}]|${IB_LINUX_PATH}|g" \
	    -e "s|[$][{]IB_ROOTFS_PATH[}]|${IB_ROOTFS_PATH}|g" \
	    -e "s|[$][{]IB_PLATFORM[}]|${IB_PLATFORM}|g" \
	    "${IB_ITS_SRC}/$1.its" > "${IB_ITB_PATH}/$1.its"
}

# This is the uEnv.txt file related to U-boot depending on the BSP
IB_UENV = "${FILE_DIRNAME}/files/uEnv_${IB_PLATFORM}.txt"

inherit logging
inherit filesystem

# Platform boot chain — produces the bootloader-side artefacts (flash0.img
# + FIP for virt64) AND the AVZ ITB. Lives at the BSP class level so it is
# shared across every BSP recipe (bsp-linux, bsp-so3) and stays
# capsule-agnostic. AVZ is part of the boot chain by design — the
# hypervisor runs underneath every capsule, and its ITB is identical
# regardless of which capsule sits on top.
#
# Per-platform bootloader assembly lives in bsp_<platform>.inc as
# __do_platform_boot_chain(d). The AVZ ITB build is generic: mkimage on
# ${IB_DIR}/linux/target/${IB_PLATFORM}_avz.its.

# Boot-chain dependencies gate on IB_BOOT_CHAIN:
#   "uboot"      → only u-boot (no ATF, OP-TEE, AVZ pulled in)
#   "atf+uboot"  → ATF + u-boot, no OP-TEE, no AVZ
#   "full"       → ATF + OP-TEE + u-boot + AVZ (capsule builds)
#
# Deps are wired into BOTH do_build (so `build.sh -a` actually compiles
# every source artefact) AND do_deploy_boot_chain (so a standalone
# `deploy.sh -a` without a prior build still pulls them through sstate).

do_deploy_boot_chain[nostamp] = "1"
do_deploy_boot_chain[depends] = "uboot:do_build"

python () {
    chain = d.getVar('IB_BOOT_CHAIN') or ""
    extra = []
    if chain in ("atf+uboot", "full"):
        extra.append("atf:do_build")
    if chain == "full":
        extra += ["optee:do_build", "avz:do_build"]
    if extra:
        deps = ' ' + ' '.join(extra)
        d.appendVarFlag('do_deploy_boot_chain', 'depends', deps)
        d.appendVarFlag('do_build', 'depends', deps)
}

python do_deploy_boot_chain () {
    plat = d.getVar('IB_PLATFORM') or '?'
    chain = d.getVar('IB_BOOT_CHAIN') or '?'
    bb.plain(f"Deploy boot chain for platform {plat} (chain={chain})")

    try:
        __do_platform_boot_chain(d)
    except NameError:
        bb.note(f"No __do_platform_boot_chain defined for {plat} — skipping")

    # The AVZ ITB is produced by per-platform do_itb
    # (bsp_<platform>.inc) which is wired `before do_build`, so they
    # exist by the time do_deploy_boot_chain runs.
}
addtask do_deploy_boot_chain before do_deploy

def __do_deploy_boot(d):

    if d.getVar('IB_STORAGE_MODE') not in ("remote", "http"):
        bb.warn("Now mounting")
        __do_fs_mount(d)

    __do_platform_deploy(d)

    if d.getVar('IB_STORAGE_MODE') not in ("remote", "http"):
        __do_fs_umount(d)
