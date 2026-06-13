###################################################################
#
#   The filesystem creation class
#   IB_STORAGE = soft create .img disk image
#   Prepares a flashable filesystem image containing
#   two partitions one for the bootloader and one for the rootfs
#   using losetup(8) to mount the image on a loop device  /dev/loopXX
#
#   IB_STORAGE = hard write to a real disk device
#   Writes to actual disk device, please double check config!
#
#   this class is inherited by platform-specific filesystem creation
#   classes init_storage_XX
#
#   Also see the IB_STORAGE_*, IB_ROOTFS_*
#   options in local.conf
#
#   Copyright (c) 2014-2023 REDS Institute, HEIG-VD
#   Copyright (c) 2023-2025 EDGEMTech Ltd
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

# Create and initialize the storage (including formatting partitions)
def __do_fs_init_storage(d):

    IB_STORAGE = d.getVar('IB_STORAGE')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')

    WORKDIR = d.getVar("WORKDIR")

    # Perform the check as this task can also be executed from a
    # script or directly using bitbake
    if utils_chk_is_root_user(d) == False:
        bb.fatal(("Please re-run the task/script as root - "
                  "It is required to access loop devices"))

    if IB_STORAGE == "remote":
        return None

    if IB_STORAGE == "hard" and IB_STORAGE_DEVICE == "":
        bb.fatal(("No device found; please edit conf/local.conf"
                  " IB_STORAGE_DEVICE is not set"))

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
    IB_STORAGE = d.getVar('IB_STORAGE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    # Check if the user is running the filesystem recipe as root
    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    if IB_STORAGE == "soft":
        image_path = os.path.join(IB_FILESYSTEM_PATH , "work", f"sdcard.img.{IB_PLATFORM}")
        if not os.path.isfile(image_path):
            bb.plain((f"The filesystem image: sdcard.img.{IB_PLATFORM} "
                      "does not exist - creating it"))
            __do_fs_init_storage(d)


# Mount the partitions to p1, p2 respectively
def __do_fs_mount(d):
    import os
    import subprocess
    import json
    import errno

    WORKDIR = d.getVar('IB_FILESYSTEM_PATH') + "/work"
    IB_STORAGE = d.getVar('IB_STORAGE')
    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    TMPDIR = d.getVar("TMPDIR")

    # Check if the user is running the filesystem recipe as root
    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    if IB_STORAGE == "soft":
        img_path = f"{WORKDIR}/sdcard.img.{IB_PLATFORM}"

        # Check if image exists before running losetup
        try:
            os.stat(img_path)
        except OSError as e:
            if e.errno == errno.ENOENT:
                bb.fatal(f"{img_path} does not exist")

    p1 = os.path.join(WORKDIR, "p1")
    p2 = os.path.join(WORKDIR, "p2")

    if os.path.ismount(p1):
        bb.warn(f"{p1} is already mounted - avoid remount")
        return

    if os.path.ismount(p2):
        bb.warn(f"{p2} is already mounted - avoid remount")
        return

    os.makedirs(p1, exist_ok=True)
    os.makedirs(p2, exist_ok=True)

    if IB_STORAGE == "soft":

        try:
            devname = subprocess.check_output(
                f"losetup --partscan --find --show {img_path}", shell=True,
                text=True).strip()
        except Exception as e:
            bb.fatal((f"Could not attach image: {img_path}"
                      f" to a loop device error: {e}"))

        # Keep device name only without /dev/
        devname = devname.replace("/dev/", "")
    else:
        devname = d.getVar('IB_STORAGE_DEVICE')

    shdata = {
        'IB_FILESYSTEM_DEVNAME': devname
    }

    # NOTE: Currently this file is only written too
    path = os.path.join(TMPDIR, "global_datastore.json")
    with open(path, "w") as f:
        json.dump(shdata, f)

    f.close()
    utils_chown_file(d, path)

    if devname[-1].isdigit():
        devname += "p"

    # TODO: handle more than 2 partitions
    try:
        subprocess.run(['mount', f'/dev/{devname}1', os.path.join(WORKDIR, 'p1')], check=True)

    except Exception as e:
        bb.fatal((f"Could not mount image: {IB_FILESYSTEM_PATH}"
                  f" on /dev/{devname}1 error: {e}"))

    try:
        subprocess.run(['mount', f'/dev/{devname}2', os.path.join(WORKDIR, 'p2')], check=True)

    except Exception as e:
        bb.fatal((f"Could not mount image: {IB_FILESYSTEM_PATH}"
                  f" on /dev/{devname}2 error: {e}"))

    bb.note(f"Mounted filesystem at: {IB_FILESYSTEM_PATH}/p1,p2")

    if os.path.ismount(os.path.join(WORKDIR, 'p1')):
        if os.path.lexists(IB_FILESYSTEM_PATH + "/p1"):
            os.remove(IB_FILESYSTEM_PATH + "/p1")
        os.symlink(os.path.join(WORKDIR, 'p1'), IB_FILESYSTEM_PATH+"/p1")

    if os.path.ismount(os.path.join(WORKDIR, 'p2')):
        if os.path.lexists(IB_FILESYSTEM_PATH + "/p2"):
            os.remove(IB_FILESYSTEM_PATH + "/p2")
        os.symlink(os.path.join(WORKDIR, 'p2'), IB_FILESYSTEM_PATH+"/p2")


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
            os.system(f"umount '{directory}'")

    else:
        bb.warn(f"{directory} wasn't mounted - will remove mount point dir")

    # Remove the mountpoint dir and the symlink in the fs staging area
    os.system(f"rm -rf '{directory}'")
    os.system(f"rm '{IB_FILESYSTEM_PATH}/p{partition_number}'")

    utils_restore_user_ownership(d)

def __do_fs_umount(d):
    import subprocess
    
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    WORKDIR = d.getVar('WORKDIR')

    # Check if the user is running the filesystem recipe as root
    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    __do_main_umount(d, 1)
    __do_main_umount(d, 2)
 
    try:
        subprocess.run(['losetup', '-D'], check=True)

    except Exception as e:
        bb.fatal(f"Failed during losetup -D operation, could not unmount all devices: {e}")
   
    # Change ownership of filesystem/sdcard.img.* and filesystem/work
    utils_chown_dir(d, f"{IB_FILESYSTEM_PATH}", follow_symlinks=False, recursive=True)

    # And of the filesystem/ dir itself
    utils_chown_dir(d, f"{IB_FILESYSTEM_PATH}", follow_symlinks=False, recursive=False)


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


