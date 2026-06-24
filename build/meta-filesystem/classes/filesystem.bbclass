###################################################################
#
#   The filesystem creation class
#   IB_STORAGE_MODE = soft create .img disk image
#   Prepares a flashable filesystem image containing
#   two partitions one for the bootloader and one for the rootfs
#   using losetup(8) to mount the image on a loop device  /dev/loopXX
#
#   IB_STORAGE_MODE = hard write to a real disk device
#   Writes to actual disk device, please double check config!
#
#   this class is inherited by platform-specific filesystem creation
#   classes init_storage_XX
#
#   Also see the IB_STORAGE_MODE, IB_STORAGE_DEVICE, IB_ROOTFS_*
#   options in local.conf
#
#   Copyright (c) 2014-2026 REDS Institute, HEIG-VD
#   Copyright (c) 2023-2026 EDGEMTech Ltd
#
#   Authors:
#       EDGEMTech Ltd, Daniel Rossier (daniel.rossier@edgemtech.ch)
#       EDGEMTech Ltd, Erik Tagirov (erik.tagirov@edgemtech.ch)
#
###################################################################

inherit logging
inherit utils
inherit fs_${IB_PLATFORM}

IB_FILESYSTEM_PATH = "${IB_DIR}/filesystem"

# __do_fs_mount and __do_fs_umount must be implemented
# in the specific board class.

# Create and initialize the storage (including formatting partitions).
# bitbake runs as the unprivileged user; privileged ops inside
# __platform_init_storage escalate via `utils_sudo` and rely on the
# caller having opened a sudo session (scripts/common/sudo_session.sh).
def __do_fs_init_storage(d):

    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')

    WORKDIR = d.getVar("WORKDIR")

    if IB_STORAGE_MODE == "remote":
        return None

    # `not` catches both unset (commented out in local.conf -> None) and empty.
    # IB_STORAGE_DEVICE has no default ON PURPOSE: a wrong/default device (e.g.
    # /dev/sda) on a mechanical deploy could overwrite a host disk.
    if IB_STORAGE_MODE == "hard" and not IB_STORAGE_DEVICE:
        bb.fatal("IB_STORAGE_MODE is 'hard' for platform '%s' but IB_STORAGE_DEVICE "
                 "is not set. Refusing to initialise storage: writing to a wrong or "
                 "default device (e.g. /dev/sda) could overwrite a host disk. "
                 "Uncomment and set IB_STORAGE_DEVICE for this platform in "
                 "conf/local.conf before deploying." % (d.getVar('IB_PLATFORM') or '?'))

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

    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    if IB_STORAGE_MODE == "soft":
        image_path = os.path.join(IB_FILESYSTEM_PATH , "work", f"sdcard.img.{IB_PLATFORM}")
        if not os.path.isfile(image_path):
            bb.plain((f"The filesystem image: sdcard.img.{IB_PLATFORM} "
                      "does not exist - creating it"))
            __do_fs_init_storage(d)


def __do_main_umount(d, partition_number):
    import os

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    directory = f"{IB_FILESYSTEM_PATH}/work/p{partition_number}"

    if os.path.ismount(directory):
        # TODO: use ionotify(7)
        while True:

            # Check if the source directory is still mounted
            if not os.path.ismount(directory):
                break

            os.sync()
            time.sleep(1)

            # Unmount the source directory
            utils_sudo(["umount", directory])

    else:
        bb.warn(f"{directory} wasn't mounted - will remove mount point dir")

    # Remove the mountpoint dir (root-owned because mount populated it)
    # and the symlink in the fs staging area (user-owned).
    utils_sudo(["rm", "-rf", directory])
    if os.path.lexists(f"{IB_FILESYSTEM_PATH}/p{partition_number}"):
        os.remove(f"{IB_FILESYSTEM_PATH}/p{partition_number}")


python do_fs_mount () {
    __do_fs_mount(d)
}

python do_fs_init_storage () {
    __do_fs_init_storage(d)
}

python do_fs_umount() {
    __do_fs_umount(d)
}

python do_fs_check () {
    __do_fs_check(d)
}

addtask do_fs_init_storage
addtask do_fs_check
addtask do_fs_mount
addtask do_fs_umount

# nostamp is necessary to let the user re-run this tasks many times
# on demand from scripts

do_fs_check[nostamp] = "1"
do_fs_init_storage[nostamp] = "1"
do_fs_mount[nostamp] = "1"
do_fs_umount[nostamp] = "1"
