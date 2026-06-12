
# Specific task description for formatting
# the storage of ARM platform with 3 partitions

# Partition layout is as follows:
# - Partition #1: 128 MB (u-boot, kernel, etc.)
# - Partition #2: 400 MB (Main rootfs)
# - Partition #3: 100 MB (A data partition used for SO3 capsules for example)

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

    if IB_STORAGE == "soft":
        # Make sure the filesystem dir exists
        run(f"sudo mkdir -p {IB_FILESYSTEM_PATH}")

        # Create image first
        print(f"Creating {store_path}")

        dd_size = IB_ROOTFS_SIZE
        subprocess.run(["truncate", "-s", dd_size, store_path])

        devname = subprocess.run(["losetup", "--partscan", "--find", "--show", store_path],
                capture_output=True, text=True).stdout.strip()
        print(devname)

        if devname == "":
            bb.fatal(f"{store_path}")

        # Keep device name only without /dev/
        devname = devname.replace("/dev/", "")

        print("Linking the storage image", IB_DIR)

        os.makedirs(os.path.join(WORKDIR, "filesystem"), exist_ok=True)

        target_link = os.path.join(IB_DIR, f"filesystem/{store_filename}")
        source_link = store_path

        # Check if the symbolic link already exists
        if os.path.islink(target_link):
            # Remove the existing symbolic link
            os.unlink(target_link)

        os.symlink(source_link, target_link)

    if not os.path.exists(f"/dev/{devname}"):
        print(f"Unfortunately, /dev/{devname} does not exist...")
        exit(1)

    print(f"Partitioning and formatting: {devname}")

    # Create the partition layout this way
    # TODO: use sfdisk(8) which is more suitable for scripting
    fdisk_input = "o\nn\np\n\n\n+128M\nt\nc\na\nn\np\n\n\n+1600M\nw\n"
    subprocess.run(["fdisk", f"/dev/{devname}"], input=fdisk_input.encode())

    print("Waiting ...")

    # TODO: use ionotify(7)
    # Give a chance to the real SD-card to be sync'd
    time.sleep(2)

    if devname[-1].isdigit():
        devname += "p"

    subprocess.run(["mkfs.fat", "-F32", "-a", "-v", "-n", "boot", f"/dev/{devname}1"])
    subprocess.run(["mkfs.ext4", "-L", "rootfs1", f"/dev/{devname}2"])

    if IB_STORAGE == "soft":
        subprocess.run(["losetup", "-D"])

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

    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_STORAGE = d.getVar('IB_STORAGE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')

    uid = utils_get_user_uid(d)

    # Check if the user is running the filesystem recipe as root
    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    if IB_STORAGE == "soft":
        image_path = os.path.join(IB_FILESYSTEM_PATH , "work", f"sdcard.img.{IB_PLATFORM}")
        if not os.path.isfile(image_path):
            bb.plain((f"The filesystem image: sdcard.img.{IB_PLATFORM} "
                      "does not exist - creating it"))
            __do_fs_init_storage(d)

    utils_restore_user_ownership(d)

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
        utils_restore_user_ownership(d)
        return

    if os.path.ismount(p2):
        bb.warn(f"{p2} is already mounted - avoid remount")
        utils_restore_user_ownership(d)
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
        json.dump(shdata, f);

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

    utils_restore_user_ownership(d)


def __do_fs_umount(d):

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    WORKDIR = d.getVar('WORKDIR')

    # Check if the user is running the filesystem recipe as root
    if utils_chk_is_root_user(d) == False:
        bb.fatal("Please re-run the task/script as root")

    __do_main_umount(d, 1)
    __do_main_umount(d, 2)

    os.system("losetup -D")

    utils_restore_user_ownership(d)
