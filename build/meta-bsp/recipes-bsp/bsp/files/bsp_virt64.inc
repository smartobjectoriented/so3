# Copyright (c) 2025-2026 EDGEMTech SA
#
# virt64 (QEMU armv8) platform glue. The ITB and uEnv.txt are copied onto
# the boot partition (p1) by __deploy_arm_common (bsp_arm_common.inc).
# __do_platform_boot_chain assembles the optional ATF/OP-TEE firmware
# (flash0.img) — generic and product-agnostic. Any capsule/OSTree
# specifics live in a downstream layer, not here.

require bsp_arm_common.inc

# Entry point of an ELF64, which for a non-relocatable image is also where it
# has to be loaded. Read here rather than shelled out to readelf: the header
# is fixed-width and this is two fields of it.
def __elf_entry(path):
    import struct

    with open(path, "rb") as f:
        header = f.read(64)

    if header[:4] != b"\x7fELF" or header[4] != 2:
        bb.fatal("%s is not an ELF64 — cannot tell where it wants to run"
                 % path)

    little = header[5] == 1
    return struct.unpack_from("<Q" if little else ">Q", header, 24)[0]


# How this machine starts, written by the build for the launcher to obey.
#
# scripts/st.sh used to work the boot mode out for itself, from a mix of
# which artefacts were lying in filesystem/ and a grep of local.conf for
# IB_TARGET_ITS. Both were guesses at something the build already knows —
# the grep could not even see a capsule's ITS, which is declared in a layer,
# and local.conf carried a warning saying so. The build states it instead.
def __write_boot_conf(d, fs_dir, chain, machine, boot_opt):
    import os

    with open(os.path.join(fs_dir, "boot.conf"), "w") as f:
        f.write("# Written by __do_platform_boot_chain. How to start this\n"
                "# machine, for scripts/st.sh — do not edit, deploy again.\n")
        f.write("IB_QEMU_CHAIN=\"%s\"\n" % chain)
        f.write("IB_QEMU_MACHINE=\"%s\"\n" % machine)
        f.write("IB_QEMU_BOOT=\"%s\"\n" % boot_opt)


def __do_platform_deploy(d):
    # Both shapes (two-ITB AVZ boot when the ITS ends in "_avz", single-ITB
    # bootm otherwise) are handled generically by __deploy_arm_common.
    __deploy_arm_common(d)


def __do_platform_boot_chain(d):
    """Assemble the virt64 QEMU boot firmware in ${IB_DIR}/filesystem/.

    Selected by IB_BOOT_CHAIN (see local.conf), normalised beforehand by
    ib_normalize_boot_axes (base.bbclass):
      "mcuboot"          no U-Boot and no ATF: the BSP's own bootloader is
                         the first thing the machine runs, and it is read off
                         the card by the machine's boot ROM — see
                         virt_bootrom_setup() in the tree's QEMU. Written to
                         the raw area ahead of p1, where imx-boot lives on the
                         verdin's eMMC. filesystem/bootrom.conf records the
                         offset and load address for the launcher.
      "uboot"            bare U-Boot — no flash0.img. The QEMU launcher
                         boots U-Boot directly (st.sh: -kernel
                         u-boot/u-boot, the ELF). Any stale flash0.img is
                         removed so the launcher picks the bare path.
      "atf+uboot"        flash0.img = BL1 @ 0 + FIP @ 256K, FIP = BL2 +
                         BL31 + U-Boot (no OP-TEE). ATF gives EL3 -> EL2.
      "atf+optee+uboot"  flash0.img = BL1 + FIP[BL2 + BL31 + OP-TEE
                         (3-part) + U-Boot] — secure world (S-EL1 OP-TEE).

    Orthogonal to IB_HYPERVISOR: whether Linux runs directly or as an AVZ
    guest changes the ITB shape, not the firmware assembled here — AVZ
    boots on any of the three chains.

    Invoked from bsp.bbclass:do_deploy_boot_chain (which has already pulled
    in atf/optee builds when the chain requires them)."""
    import os
    import subprocess

    chain      = d.getVar('IB_BOOT_CHAIN') or ""
    hyp        = d.getVar('IB_HYPERVISOR') or "none"
    has_atf    = bool(d.getVar('IB_CHAIN_HAS_ATF'))
    has_uboot  = bool(d.getVar('IB_CHAIN_HAS_UBOOT'))
    uboot_path = d.getVar('IB_UBOOT_PATH')
    fs_dir     = os.path.join(d.getVar('IB_DIR'), "filesystem")
    os.makedirs(fs_dir, exist_ok=True)

    flash_path = os.path.join(fs_dir, "flash0.img")
    # work/, not the filesystem/ symlink beside it — see
    # __deploy_zephyr_boot_app in bsp-zephyr_1.0.bb.
    store      = os.path.join(fs_dir, "work",
                              "sdcard.img.%s" % d.getVar('IB_PLATFORM'))

    # U-Boot is needed in every mode but "mcuboot" — as the first stage, and
    # as the FIP's BL33 in the ATF chains. Kept in filesystem/ for the
    # recovery and flashing scripts that expect it there.
    if has_uboot:
        subprocess.run(['cp', f"{uboot_path}/u-boot.bin",
                        os.path.join(fs_dir, "u-boot.bin")], check=True)

    # Whether the first stage can come off the card at all is a property of
    # the image, not of the chain: a ROM copies bytes to an address and jumps,
    # so the image has to be one that runs from RAM.
    #
    # MCUboot always is. U-Boot depends on how it was configured, and this
    # tree builds it two ways (see uboot_2022.04.bb): IB_BOOT_CHAIN="uboot"
    # with no hypervisor takes upstream qemu_arm64_defconfig, which is
    # position-independent with a text base of 0 — an image meant to execute
    # in place from flash, which is why QEMU is handed it as -kernel today and
    # why -bios would be the other right answer. Every other combination takes
    # ${IB_PLATFORM}_defconfig, linked at a RAM address, and that one can be
    # read off the card like any other first stage.
    #
    # The ELF entry point is what says which: zero means "wherever flash is".
    uboot_entry = 0
    if has_uboot:
        uboot_entry = __elf_entry(os.path.join(uboot_path, "u-boot"))

    # Reading a stage off the card needs a machine that does it, which for
    # QEMU means the bootrom-* properties this tree's virt.c patch adds. A
    # tree whose QEMU patch has not been regenerated with them sets this to
    # "0": U-Boot is then handed to QEMU as an ELF, exactly as before, and
    # only a chain with no U-Boot at all — where there is no other way in —
    # still goes to the card.
    #
    # A variable rather than two versions of this file: the rule is the
    # same everywhere, it is the emulator underneath that differs.
    from_card = (d.getVar('IB_FIRST_STAGE_FROM_CARD') or "1") != "0"

    if not has_uboot or (from_card and not has_atf and uboot_entry):
        # The first stage goes on the card, in the raw area ahead of p1, and
        # the machine's boot ROM reads it from there — which is where
        # imx-boot lives on the verdin's eMMC and how its ROM finds it.
        # Nothing about this stage comes from a host file at run time.
        #
        # The ATF chains never come here: BL1 is linked to execute in place
        # from address 0, and pflash is exactly where that belongs.
        if not has_uboot:
            binary = d.getVar('IB_FIRST_STAGE_BIN')
            load = d.getVar('IB_FIRST_STAGE_LOAD')
            if not binary or not os.path.isfile(binary):
                bb.fatal('IB_BOOT_CHAIN="mcuboot" needs IB_FIRST_STAGE_BIN to '
                         'point at the bootloader this BSP builds (got "%s").'
                         % (binary or ""))
            if not load:
                bb.fatal('IB_BOOT_CHAIN="mcuboot" needs IB_FIRST_STAGE_LOAD: '
                         'the ROM has to be told where to put the first stage.')
        else:
            # The .bin and not the ELF: a ROM copies bytes to a fixed
            # address, it does not parse program headers. So the address has
            # to come from somewhere, and the ELF is where it is written —
            # this U-Boot is built without CONFIG_POSITION_INDEPENDENT, so
            # its one LOAD segment has to land exactly where it was linked.
            binary = os.path.join(uboot_path, "u-boot.bin")
            load = "0x%x" % uboot_entry

        if not os.path.isfile(store):
            bb.fatal("no storage image at %s — run init_storage.sh first"
                     % store)

        # The offset has to stay below the first partition. A DOS label puts
        # that at 1 MiB, so there is just under a megabyte of raw space, and
        # U-Boot (565 KiB) and MCUboot (66 KiB) both fit.
        offset = int(d.getVar('IB_FIRST_STAGE_OFFSET') or "8192")
        size = os.path.getsize(binary)
        if offset + size > 1024 * 1024:
            bb.fatal("first stage is %d bytes at offset %d — that runs past "
                     "the 1 MiB where the first partition starts"
                     % (size, offset))

        subprocess.run(['dd', 'if=%s' % binary, 'of=%s' % store,
                        'bs=1', 'seek=%d' % offset,
                        'conv=notrunc,fsync'], check=True)

        if os.path.exists(flash_path):
            os.remove(flash_path)

        machine = "virt,gic-version=2"
        if hyp == "avz":
            # AVZ is an EL2 hypervisor, so the machine has to expose EL2.
            # Without it AVZ faults on its first EL2 system-register access,
            # which surfaces as a synchronous abort and a reset.
            machine += ",virtualization=on"
        machine += (",bootrom-drive=hd0,bootrom-offset=%d,bootrom-load=%s"
                    % (offset, load))

        __write_boot_conf(d, fs_dir, chain, machine, "")
        bb.plain("First stage on the card at offset %d (%d bytes), entered "
                 "at %s" % (offset, size, load))
        return

    if not has_atf:
        # The flash-shaped U-Boot. It cannot be read off the card — see above
        # — so QEMU is handed the ELF and places it itself. Drop any stale
        # flash0.img from a prior ATF deploy so nothing else claims address 0.
        if os.path.exists(flash_path):
            os.remove(flash_path)

        __write_boot_conf(d, fs_dir, chain, "virt,gic-version=2",
                          "-kernel u-boot/u-boot")
        return

    # ATF chains: build flash0.img. "atf+uboot" and "full" differ only by
    # whether the FIP carries the OP-TEE (BL32) entries.
    atf_path    = d.getVar('IB_ATF_PATH')
    atf_release = f"{atf_path}/build/qemu/release"
    subprocess.run(['cp', f"{atf_release}/bl31.bin",
                    os.path.join(fs_dir, "bl31.bin")], check=True)

    fip_args = [
        '--tb-fw',  f'{atf_release}/bl2.bin',
        '--soc-fw', f'{atf_release}/bl31.bin',
        '--nt-fw',  f'{uboot_path}/u-boot.bin',
    ]

    if d.getVar('IB_CHAIN_HAS_OPTEE'):
        optee_core = f"{d.getVar('IB_OPTEE_PATH')}/out/arm-plat-vexpress/core"
        for src, name in [
            (f"{optee_core}/tee-header_v2.bin",   "tee-header_v2.bin"),
            (f"{optee_core}/tee-pager_v2.bin",    "tee-pager_v2.bin"),
            (f"{optee_core}/tee-pageable_v2.bin", "tee-pageable_v2.bin"),
        ]:
            subprocess.run(['cp', src, os.path.join(fs_dir, name)], check=True)
        fip_args += [
            '--tos-fw',        f'{optee_core}/tee-header_v2.bin',
            '--tos-fw-extra1', f'{optee_core}/tee-pager_v2.bin',
            '--tos-fw-extra2', f'{optee_core}/tee-pageable_v2.bin',
        ]

    fiptool  = f"{atf_path}/tools/fiptool/fiptool"
    fip_path = os.path.join(fs_dir, "fip.bin")
    subprocess.run([fiptool, 'create'] + fip_args + [fip_path], check=True)

    subprocess.run(['dd', 'if=/dev/zero', f'of={flash_path}', 'bs=1M', 'count=64'], check=True)
    subprocess.run(['dd', f'if={atf_release}/bl1.bin', f'of={flash_path}', 'conv=notrunc'], check=True)
    subprocess.run(['dd', f'if={fip_path}', f'of={flash_path}', 'bs=1k', 'seek=256', 'conv=notrunc'], check=True)

    # EL3 (secure=on) for ATF itself and EL2 for what runs above it. Unlike
    # the two chains above, the first stage here is not on the card: BL1 is
    # linked to execute in place from address 0, and pflash is where that is.
    __write_boot_conf(d, fs_dir, chain,
                      "virt,virtualization=on,gic-version=2,secure=on",
                      "-drive if=pflash,format=raw,file=filesystem/flash0.img")
