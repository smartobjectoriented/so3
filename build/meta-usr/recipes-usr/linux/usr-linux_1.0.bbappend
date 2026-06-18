# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "EDGE-M1 Factory Capsule - User space applications for Linux"
DESCRIPTION = "All (Linux) user space custom applications which take place in the rootfs of Linux"
LICENSE = "GPLv2"

inherit usr
inherit linux
inherit filesystem
inherit rootfs
 
python do_deploy() {

    import os
    import subprocess

    d.setVar('ROOTFS_FILENAME', 'rootfs')
    __do_rootfs_mount(d)

    IB_USR_PATH = d.getVar('IB_USR_PATH')

    if not os.path.isdir(os.path.join(IB_USR_PATH, "build", "deploy")):

        __do_rootfs_umount(d)
        bb.fatal("The {} does not exist; please build usr first...".format(IB_USR_PATH))

    IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
    IB_PLATFORM = d.getVar('IB_PLATFORM')

    if not os.path.isfile(os.path.join(IB_ROOTFS_PATH, "board", IB_PLATFORM, "rootfs.cpio")):
        __do_rootfs_umount(d)
        bb.fatal("rootfs.cpio is missing; please deploy rootfs first...")

    # The destination tree is root-owned (cpio -id), so rsync needs root
    # to preserve mode bits / ownership while writing into it.
    utils_sudo(["rsync", "-a", "--keep-dirlinks",
                f"{IB_USR_PATH}/build/deploy/",
                f"{IB_ROOTFS_PATH}/fs/"], check=True)

    __do_rootfs_umount(d)
}