# Copyright (c) 2025-2026 EDGEMTech SA

# Specific task description for formatting
# the storage of imx8mp-verdin platform.
#
# bitbake runs unprivileged; root ops go through utils_sudo (`sudo -n`).
# The caller (deploy.sh / build.sh / mount.sh / umount.sh) is expected
# to have opened a sudo session beforehand.

inherit logging
inherit utils

IB_FILESYSTEM_PATH = "${IB_DIR}/filesystem"


def __platform_init_storage(d):
    import os
    import subprocess

    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE')
    IB_ROOTFS_SIZE = d.getVar('IB_ROOTFS_SIZE')
    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')
    IB_DIR = d.getVar('IB_DIR')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    WORKDIR = d.getVar('WORKDIR')

    store_filename = f"sdcard.img.{IB_PLATFORM}"
    store_path = os.path.join(WORKDIR, store_filename)
    devname = IB_STORAGE_DEVICE

    if not os.path.exists(f"/dev/{devname}"):
        print(f"Unfortunately, /dev/{devname} does not exist...")
        exit(1)

    print(f"Partitioning and formatting: {devname}")

    utils_sudo(["parted", f"/dev/{devname}", "--script", "mklabel", "msdos"])
    utils_sudo(["parted", f"/dev/{devname}", "--script", "mkpart", "primary", "ext4", "2048s", "100%"])

    print("Waiting ...")

    # TODO: use ionotify(7)
    # Give a chance to the USB drive to be sync'd
    time.sleep(2)

    utils_sudo(["mkfs.ext4", "-L", "TEZI", f"/dev/{devname}1"])


    print("Done! The storage is now initialized")


# Create and initialize the storage (including formatting partitions)
def __do_fs_init_storage(d):

    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')

    WORKDIR = d.getVar("WORKDIR")

    # Perform the tasks specific to the platform
    __platform_init_storage(d)

    # Finally create a symlink to the workdir to be able
    # to mount/umount more conveniently
    target_link = os.path.join(d.getVar('IB_DIR'), "filesystem/work")

    # Check if the symbolic link already exists
    if os.path.islink(target_link):
        # Remove the existing symbolic link
        os.unlink(target_link)

    os.symlink(WORKDIR, target_link)


# Check the presence of the virtual disk image
# if the deployment is done on the virtual ("soft") storage
# and call filesystem:fs_init_storage() if it does not exist

def __do_fs_check(d):
    import subprocess


def __do_fs_mount(d):
    import os
    import subprocess

    WORKDIR = d.getVar('IB_FILESYSTEM_PATH') + "/work"
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    usb_mountpoint = os.path.join(WORKDIR, "usb")

    if os.path.ismount(usb_mountpoint):
        bb.warn(f"{usb_mountpoint} is already mounted - avoid remount")
        return

    os.makedirs(usb_mountpoint, exist_ok=True)

    try:
        utils_sudo(['mount', f'/dev/{IB_STORAGE_DEVICE}1',
                    os.path.join(WORKDIR, 'usb')], check=True)
    except Exception as e:
        bb.fatal(f"Could not mount USB: {e}")

    bb.plain(f"Mounted USB at: {WORKDIR}/usb")


def __do_fs_umount(d):
    import os

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    usb_mountpoint = f"{IB_FILESYSTEM_PATH}/work/usb"

    if os.path.ismount(usb_mountpoint):
        while True:
            if not os.path.ismount(usb_mountpoint):
                break

            os.sync()
            time.sleep(1)
            utils_sudo(["umount", usb_mountpoint])
    else:
        bb.warn(f"{usb_mountpoint} wasn't mounted")

    utils_sudo(["rm", "-rf", usb_mountpoint])
