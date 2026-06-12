
# Specific task description for formatting
# the storage of QEMU/x86 platform with one partition

def do_filesystem_platform_init_storage(d):

    import os
    import subprocess

    IB_STORAGE = d.getVar('IB_STORAGE')
    IB_PLATFORM = d.getVar('IB_PLATFORM')
    IB_DIR = d.getVar('IB_DIR')
    WORKDIR = d.getVar('WORKDIR')

    store_filename = "sdcard.img." + IB_PLATFORM

    # Partition layout is as follows:
    # - Partition #1: 8 GB with rootfs including kernel image and GRUB

    if IB_STORAGE != "soft":
        raise ValueError("Cannot be a hard storage deployment ...")

    # Create image first
    print("Creating " + store_filename)

    dd_size = "8G"

    subprocess.run(["truncate", "-s", dd_size, os.path.join(WORKDIR, store_filename)])

    devname = subprocess.check_output(["losetup", "--partscan", "--find", "--show", os.path.join(WORKDIR, "sdcard.img.{}".format(IB_PLATFORM))], text=True).strip()

    # Keep device name only without /dev/
    devname = devname.replace("/dev/", "")

    print("devname is defined as", devname)

    print("Linking the storage image", IB_DIR)

    os.makedirs(os.path.join(WORKDIR, "filesystem"), exist_ok=True)

    target_link = os.path.join(IB_DIR, "filesystem/"+store_filename)
    source_link = os.path.join(WORKDIR, store_filename)

    # Check if the symbolic link already exists
    if os.path.islink(target_link):
        # Remove the existing symbolic link
        os.unlink(target_link)

    os.symlink(source_link, target_link)

    # Create the partition layout this way
    fdisk_input = "o\nn\np\n\n\n\n\n\nw\n"
    subprocess.run(["fdisk", "/dev/{}".format(devname)], input=fdisk_input.encode())

    print("Waiting ...")

    # Give a chance to the real SD-card to be sync'd
    time.sleep(1)

    if devname[-1].isdigit():
        devname += "p"

    subprocess.run(["mke2fs", "-F", "-t", "ext4", "/dev/{}1".format(devname)])
    subprocess.run(["e2label", "/dev/{}1".format(devname), "rootfs"])

    subprocess.run(["losetup", "-D"])

    print("Done! The storage is now initialized")



