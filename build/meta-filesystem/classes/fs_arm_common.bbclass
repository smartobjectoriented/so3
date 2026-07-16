# Copyright (c) 2025-2026 EDGEMTech SA

# Storage layout for ARM platforms (sdcard.img, IB_ROOTFS_SIZE total).
#
# Two partitions:
# - p1, FAT, 128 MiB — boot partition
#       uEnv.txt, AVZ ITB (small), legacy firmware artefacts
# - p2, ext4, ~rest of IB_ROOTFS_SIZE — rootfs partition (label "rootfs1")
#       /          — Linux rootfs proper (deployed by rootfs-linux).
#
# bitbake itself runs as the unprivileged user. Each privileged op
# (losetup/fdisk/mkfs/mount/umount) goes through utils_sudo (`sudo -n`)
# and assumes the caller pre-opened a sudo session.

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

    if IB_STORAGE_MODE == "soft":
        # Make sure the filesystem dir exists (user-owned)
        os.makedirs(IB_FILESYSTEM_PATH, exist_ok=True)

        # Create image first
        print(f"Creating {store_path}")

        dd_size = IB_ROOTFS_SIZE
        subprocess.run(["truncate", "-s", dd_size, store_path])

        devname = utils_sudo(["losetup", "--partscan", "--find", "--show", store_path],
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
    utils_sudo(["fdisk", f"/dev/{devname}"], input=fdisk_input.encode())

    print("Waiting ...")

    # TODO: use ionotify(7)
    # Give a chance to the real SD-card to be sync'd
    time.sleep(2)

    if devname[-1].isdigit():
        devname += "p"

    utils_sudo(["mkfs.fat", "-F32", "-a", "-v", "-n", "boot", f"/dev/{devname}1"])
    utils_sudo(["mkfs.ext4", "-L", "rootfs1", f"/dev/{devname}2"])

    if IB_STORAGE_MODE == "soft":
        utils_sudo(["losetup", "-D"])

    print("Done! The storage is now initialized")



# Create and initialize the storage (including formatting partitions)
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


# Mount the partitions to p1, p2 respectively
def __do_fs_mount(d):
    import os
    import subprocess
    import json
    import errno

    WORKDIR = d.getVar('IB_FILESYSTEM_PATH') + "/work"
    IB_STORAGE_MODE = d.getVar('IB_STORAGE_MODE')
    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_STORAGE_DEVICE = d.getVar('IB_STORAGE_DEVICE')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    TMPDIR = d.getVar("TMPDIR")

    if IB_STORAGE_MODE == "soft":
        img_path = f"{WORKDIR}/sdcard.img.{IB_PLATFORM}"

        # Check if image exists before running losetup
        try:
            os.stat(img_path)
        except OSError as e:
            if e.errno == errno.ENOENT:
                bb.fatal(
                    f"Storage image '{img_path}' does not exist: the filesystem "
                    f"for platform '{IB_PLATFORM}' has not been initialised yet.\n"
                    f"Deploy normally creates it automatically; if you reach this, "
                    f"initialise the storage explicitly by running:\n"
                    f"    ./scripts/init_storage.sh\n"
                    f"then run the deploy again.")

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

    if IB_STORAGE_MODE == "soft":

        try:
            devname = utils_sudo(["losetup", "--partscan", "--find", "--show", img_path],
                                 capture_output=True, text=True, check=True).stdout.strip()
        except Exception as e:
            bb.fatal((f"Could not attach image: {img_path}"
                      f" to a loop device error: {e}"))

        # Keep device name only without /dev/
        devname = devname.replace("/dev/", "")
    else:
        devname = d.getVar('IB_STORAGE_DEVICE')
        # IB_STORAGE_DEVICE has no default ON PURPOSE (a wrong device could
        # overwrite a host disk). Fail with an actionable message instead of
        # crashing on `devname[-1]` below when it is unset.
        if not devname:
            bb.fatal("IB_STORAGE_DEVICE is not set for platform '%s' (IB_STORAGE_MODE='%s'). "
                     "Set it to the target device without /dev/ — e.g. "
                     "IB_STORAGE_DEVICE:%s = \"sda\" — in build/conf/local.conf, or use "
                     "IB_STORAGE_MODE:%s = \"soft\" to build a flashable sdcard.img instead."
                     % (IB_PLATFORM, IB_STORAGE_MODE, IB_PLATFORM, IB_PLATFORM))

    shdata = {
        'IB_FILESYSTEM_DEVNAME': devname
    }

    # NOTE: Currently this file is only written too
    path = os.path.join(TMPDIR, "global_datastore.json")
    with open(path, "w") as f:
        json.dump(shdata, f);

    f.close()

    if devname[-1].isdigit():
        devname += "p"

    # bitbake runs unprivileged in the post-refactor architecture (cf.
    # utils_sudo). The subsequent __do_platform_deploy needs to write
    # uEnv.txt, ITBs and rootfs payloads to both partitions WITHOUT
    # escalating each cp/tar call. Make the mounts user-writable:
    #   - FAT (p1): pass uid=/gid= mount opts so every file appears
    #     owned by the invoking user (vfat has no on-disk uid/gid).
    #   - ext4 (p2): mount normally, then chown the root inode to the
    #     user (mkfs.ext4 sets / to root:root by default; chowning just
    #     the top inode is enough — children inherit the new owner).
    uid = os.getuid()
    gid = os.getgid()

    # TODO: handle more than 2 partitions
    try:
        utils_sudo(['mount', '-o', f'uid={uid},gid={gid}',
                    f'/dev/{devname}1', os.path.join(WORKDIR, 'p1')], check=True)

    except Exception as e:
        bb.fatal((f"Could not mount image: {IB_FILESYSTEM_PATH}"
                  f" on /dev/{devname}1 error: {e}"))

    try:
        utils_sudo(['mount', f'/dev/{devname}2', os.path.join(WORKDIR, 'p2')], check=True)
        utils_sudo(['chown', f'{uid}:{gid}', os.path.join(WORKDIR, 'p2')], check=True)

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


def __do_fs_umount(d):

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    WORKDIR = d.getVar('WORKDIR')

    __do_main_umount(d, 1)
    __do_main_umount(d, 2)

    utils_sudo(["losetup", "-D"])
