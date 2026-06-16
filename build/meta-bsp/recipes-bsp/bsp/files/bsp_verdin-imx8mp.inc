# Copyright (c) 2026 EDGEMTech SA
#
# Toradex Verdin iMX8MP platform glue for SO3-as-an-AVZ-guest, kept independent
# of Torizon and the e1c capsules (edgem1 keeps the verdin BSP in meta-e1c; this
# is the capsule-free equivalent).
#
# The firmware (imx-boot / flash.bin = SPL + ATF(BL31) + OP-TEE + U-Boot) is
# produced by the ATF recipe (imx_mkimage). do_itb (bsp-so3) mkimage's the
# verdin_imx8mp_avz_so3 ITS (AVZ + SO3 guest + rootfs.fat) into a single ITB.
#
# Milestone 1 deploy: stage imx-boot + the SO3 ITB into filesystem/work/verdin
# for manual flashing (or feeding a TEZI image set). Automating a full TEZI/SD
# install is a later step.

require bsp_arm_common.inc


def __do_platform_boot_chain(d):
    """Stage the iMX8MP boot firmware (imx-boot/flash.bin) next to the ITB.

    flash.bin bundles SPL + ATF(BL31) + OP-TEE + U-Boot and is built by the ATF
    recipe via imx_mkimage; it is flashed to the boot medium ahead of the ITB."""
    import os
    import subprocess

    atf_path = d.getVar('IB_ATF_PATH')
    fs_dir = os.path.join(d.getVar('IB_DIR'), "filesystem", "work", "verdin")
    os.makedirs(fs_dir, exist_ok=True)

    # imx_mkimage drops flash.bin under the ATF build tree; accept the common
    # locations rather than hard-coding one imx_mkimage layout.
    candidates = [
        os.path.join(atf_path, "flash.bin"),
        os.path.join(atf_path, "build", "imx8mp", "release", "flash.bin"),
    ]
    src = next((p for p in candidates if os.path.isfile(p)), None)
    if src:
        subprocess.run(['cp', src, os.path.join(fs_dir, "imx-boot")], check=True)
    else:
        bb.warn("verdin: imx-boot (flash.bin) not found under %s — build ATF "
                "(build.sh -x atf) before deploying." % atf_path)


def __do_platform_deploy(d):
    """Stage the SO3 ITB (+ imx-boot) for flashing onto the Verdin."""
    import os
    import subprocess

    __do_platform_boot_chain(d)

    itb = "%s/%s.itb" % (d.getVar('IB_ITB_PATH'), d.getVar('IB_TARGET_ITS'))
    fs_dir = os.path.join(d.getVar('IB_DIR'), "filesystem", "work", "verdin")
    if os.path.isfile(itb):
        subprocess.run(['cp', itb, fs_dir + "/"], check=True)
        bb.plain("verdin: staged imx-boot + %s in %s — flash these to the board."
                 % (os.path.basename(itb), fs_dir))
    else:
        bb.warn("verdin: ITB not found at %s (run do_itb first)." % itb)
