# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "EDGE-M1 Factory Capsule - User space applications for Linux"
DESCRIPTION = "All (Linux) user space custom applications which take place in the rootfs of Linux"
LICENSE = "GPLv2"

inherit usr
inherit linux
inherit filesystem
inherit rootfs
 
# This is a BUILD step, not a media deploy: it bakes the agency apps INTO
# rootfs.cpio (rsync into the cpio tree, then re-pack). That single rootfs
# image is then the source for both targets:
#   - the embedded ramfs (gzip -> initrd.cpio.gz -> ITB) for
#     IB_RAMFS_SOURCE="rootfs";
#   - the p2 ext4 partition, which rootfs-linux:do_deploy populates by
#     extracting this same (apps-included) rootfs.cpio at DEPLOY time.
# So p2 gets the apps for free via the final rootfs.cpio — no separate
# media write here. Depends on buildroot (rootfs.cpio) + usr:do_build
# (the apps), NOT on rootfs-linux:do_deploy, so the build never touches
# the boot media.
do_deploy[depends] = "${IB_ROOTFS_METHOD}:do_build"

python do_deploy() {

    import os

    IB_USR_PATH = d.getVar('IB_USR_PATH')
    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    IB_PLATFORM = d.getVar('IB_PLATFORM')

    deploy_src = os.path.join(IB_USR_PATH, "build", "deploy")
    if not os.path.isdir(deploy_src):
        bb.fatal("The {} does not exist; please build usr first...".format(IB_USR_PATH))
    if not os.path.isfile(os.path.join(IB_ROOTFS_PATH, "board", IB_PLATFORM, "rootfs.cpio")):
        bb.fatal("rootfs.cpio is missing; please build rootfs first...")

    # The destination tree is root-owned (cpio -id), so rsync needs root to
    # preserve mode bits / ownership while writing into it.
    d.setVar('ROOTFS_FILENAME', 'rootfs')
    __do_rootfs_mount(d)
    utils_sudo(["rsync", "-a", "--keep-dirlinks",
                deploy_src + "/", f"{IB_ROOTFS_PATH}/fs/"], check=True)
    __do_rootfs_umount(d)
}