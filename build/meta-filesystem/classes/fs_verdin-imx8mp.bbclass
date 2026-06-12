
# Specific task description for formatting
# the storage of imx8mp-verdin platform
inherit logging
inherit utils

IB_FILESYSTEM_PATH = "${IB_DIR}/filesystem"


def __platform_init_storage(d):
    import os
    import subprocess
    from bb.process import run

    IB_STORAGE = d.getVar('IB_STORAGE')
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

    subprocess.run(["parted", f"/dev/{devname}", "--script", "mklabel", "msdos"])
    subprocess.run(["parted", f"/dev/{devname}", "--script", "mkpart", "primary", "ext4", "2048s", "100%"])


    print("Waiting ...")

    # TODO: use ionotify(7)
    # Give a chance to the USB drive to be sync'd
    time.sleep(2)

    subprocess.run(["mkfs.ext4", "-L", "TEZI", f"/dev/{devname}1"])


    print("Done! The storage is now initialized")


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

    # Perform the tasks specific to the platform
    __platform_init_storage(d)

    # Finally create a symlink to the workdir to be able
    # to mount/umount more conveniently
    target_link = os.path.join(d.getVar('IB_DIR'), "filesystem/work")

    # Check if the symbolic link already exists
    if os.path.islink(target_link):
        # Remove the existing symbolic link
        os.unlink(target_link)

    # Restore the ownership of the filesystem workdir to
    # the user that ran the task - note that this is done before the filesystem
    # is mounted to avoid touching the mounted rootfs
    utils_chown_dir(d, WORKDIR)

    os.symlink(WORKDIR, target_link)

    utils_restore_user_ownership(d)


# Check the presence of the virtual disk image
# if the deployment is done on the virtual ("soft") storage
# and call filesystem:fs_init_storage() if it does not exist
def __do_fs_check(d):
    import subprocess

    utils_restore_user_ownership(d)


def __do_fs_mount(d):
    import os
    import subprocess

    WORKDIR = d.getVar('IB_FILESYSTEM_PATH') + "/work"
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    usb_mountpoint = os.path.join(WORKDIR, "usb")

    if os.path.ismount(usb_mountpoint):
        bb.warn(f"{usb_mountpoint} is already mounted - avoid remount")
        utils_restore_user_ownership(d)
        return

    os.makedirs(usb_mountpoint, exist_ok=True)

    try:
        subprocess.run(['mount', f'/dev/{IB_STORAGE_DEVICE}1',
                       os.path.join(WORKDIR, 'usb')], check=True)
    except Exception as e:
        bb.fatal(f"Could not mount USB: {e}")

    bb.plain(f"Mounted USB at: {WORKDIR}/usb")
    utils_restore_user_ownership(d)


def __do_fs_umount(d):
    import os

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    usb_mountpoint = f"{IB_FILESYSTEM_PATH}/work/usb"

    if os.path.ismount(usb_mountpoint):
        while True:
            if not os.path.ismount(usb_mountpoint):
                break

            os.sync()
            time.sleep(1)
            os.system(f"umount '{usb_mountpoint}'")
    else:
        bb.warn(f"{usb_mountpoint} wasn't mounted")

    os.system(f"rm -rf '{usb_mountpoint}'")

    utils_restore_user_ownership(d)