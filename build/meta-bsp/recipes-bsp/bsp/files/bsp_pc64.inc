# Copyright (c) 2026 EDGEMTech SA

# Deployment on the QEMU x86_64 PC (pc64).
#
# GRUB plays the part of U-Boot: its core image sits right after the MBR,
# ahead of p1, and it loads the SO3 ELF with the device tree and the root
# filesystem as multiboot2 modules, all three read from p1. There is no ITB.

IB_PC64_GRUB_MODULES = "biosdisk part_msdos fat multiboot2 normal configfile serial terminal echo test"

do_itb:pc64 () {
	bbnote "pc64: no ITB, GRUB loads the kernel, the device tree and the rootfs from p1"
}

def __pc64_grub_cfg():
    return ("set timeout=0\n"
            "serial --unit=0 --speed=115200\n"
            "terminal_input serial console\n"
            "terminal_output serial console\n"
            "\n"
            "menuentry \"SO3 (pc64)\" {\n"
            "\tmultiboot2 /so3.elf\n"
            "\tmodule2 /pc64.dtb dtb\n"
            "\tmodule2 /rootfs.fat rootfs\n"
            "}\n")

def __do_platform_boot_chain(d):
    """Install GRUB in the raw area of the disk image: boot.img in the MBR
    code area (the partition table is left alone) and the core image from
    sector 1, where boot.img and the core image's own blocklist expect it by
    default. The core image embeds every module it needs, so p1 only holds
    grub.cfg."""
    import os
    import subprocess

    fs_dir = os.path.join(d.getVar('IB_DIR'), "filesystem")
    store = os.path.join(fs_dir, "work", "sdcard.img.%s" % d.getVar('IB_PLATFORM'))
    grub_dir = "/usr/lib/grub/i386-pc"
    core = os.path.join(fs_dir, "grub-core.img")

    if not os.path.isfile(store):
        bb.fatal("no storage image at %s — run init_storage.sh first" % store)

    if not os.path.isfile(os.path.join(grub_dir, "boot.img")):
        bb.fatal("%s/boot.img is missing: install grub-pc-bin" % grub_dir)

    subprocess.run(["grub-mkimage", "-O", "i386-pc", "-d", grub_dir, "-o", core,
                    "-p", "(hd0,msdos1)/boot/grub"] + d.getVar('IB_PC64_GRUB_MODULES').split(),
                   check=True)

    # p1 starts at 1 MiB (fs_arm_common): the core image must end before.

    if 512 + os.path.getsize(core) > 1024 * 1024:
        bb.fatal("GRUB core image too big for the gap before p1")

    subprocess.run(["dd", "if=%s/boot.img" % grub_dir, "of=%s" % store, "bs=440", "count=1",
                    "conv=notrunc,fsync"], check=True)
    subprocess.run(["dd", "if=%s" % core, "of=%s" % store, "bs=512", "seek=1",
                    "conv=notrunc,fsync"], check=True)

def __do_platform_deploy(d):
    import os
    import shutil

    p1 = d.getVar('IB_FILESYSTEM_PATH') + "/p1"
    so3 = os.path.join(d.getVar('IB_SO3_PATH'), "so3")

    files = {
        os.path.join(so3, "so3"): "so3.elf",
        os.path.join(so3, "dts", "pc64.dtb"): "pc64.dtb",
        os.path.join(d.getVar('IB_ROOTFS_PATH'), "rootfs.fat"): "rootfs.fat",
    }

    for src, dst in files.items():
        if not os.path.isfile(src):
            bb.fatal(src + " is missing ...")

        shutil.copyfile(src, os.path.join(p1, dst))

    os.makedirs(os.path.join(p1, "boot", "grub"), exist_ok=True)

    with open(os.path.join(p1, "boot", "grub", "grub.cfg"), "w") as f:
        f.write(__pc64_grub_cfg())
