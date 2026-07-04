# Copyright (c) 2025-2026 EDGEMTech SA
#
# virt64 (QEMU armv8) platform glue. The ITB and uEnv.txt are copied onto
# the boot partition (p1) by __deploy_arm_common (bsp_arm_common.inc).
# __do_platform_boot_chain assembles the optional ATF/OP-TEE firmware
# (flash0.img) — generic and product-agnostic. Any capsule/OSTree
# specifics live in a downstream layer, not here.

require bsp_arm_common.inc

def __do_platform_deploy(d):
    # Both shapes (two-ITB AVZ boot when the ITS ends in "_avz", single-ITB
    # bootm otherwise) are handled generically by __deploy_arm_common.
    __deploy_arm_common(d)


def __do_platform_boot_chain(d):
    """Assemble the virt64 QEMU boot firmware in ${IB_DIR}/filesystem/.

    Selected by IB_BOOT_CHAIN (see local.conf):
      "" / "uboot"  bare U-Boot — no flash0.img. The QEMU launcher boots
                    U-Boot directly (st.sh: -kernel u-boot, EL1 standalone
                    or EL2/virtualization=on for the avz ITS). Any stale
                    flash0.img is removed so the launcher picks the bare
                    path.
      "atf+uboot"   flash0.img = BL1 @ 0 + FIP @ 256K, FIP = BL2 + BL31 +
                    U-Boot (no OP-TEE). ATF gives EL3 -> EL2.
      "full"        flash0.img = BL1 + FIP[BL2 + BL31 + OP-TEE(3-part) +
                    U-Boot] — secure world (S-EL1 OP-TEE).
    Invoked from bsp.bbclass:do_deploy_boot_chain (which has already pulled
    in atf/optee builds when the chain requires them)."""
    import os
    import subprocess

    chain      = d.getVar('IB_BOOT_CHAIN') or ""
    uboot_path = d.getVar('IB_UBOOT_PATH')
    fs_dir     = os.path.join(d.getVar('IB_DIR'), "filesystem")
    os.makedirs(fs_dir, exist_ok=True)

    flash_path = os.path.join(fs_dir, "flash0.img")

    # U-Boot is needed in every mode (as -kernel/-bios payload, and as the
    # FIP's BL33 in the ATF chains).
    subprocess.run(['cp', f"{uboot_path}/u-boot.bin",
                    os.path.join(fs_dir, "u-boot.bin")], check=True)

    if chain in ("", "uboot"):
        # Bare U-Boot: drop any stale flash0.img from a prior ATF deploy.
        if os.path.exists(flash_path):
            os.remove(flash_path)
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

    if chain == "full":
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
