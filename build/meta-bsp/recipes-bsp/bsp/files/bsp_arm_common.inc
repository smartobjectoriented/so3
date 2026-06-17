# Copyright (c) 2025-2026 EDGEMTech SA

# Deployment of ITB for all ARM platforms (FAT boot partition p1).
#
# Two deployment shapes, selected by the chosen ITS:
#   * AVZ two-ITB boot (IB_TARGET_ITS ends in "_avz"): stage the AVZ ITB
#     (<plat>_avz.itb) AND the SO3 guest ITB (<plat>_guest.itb) plus the
#     uEnv_<plat>_avz.txt that loads both and jumps via U-Boot's e1c-boot
#     command (AVZ FIT in x0, guest ITB in x1). The guest ITB is derived
#     from IB_TARGET_ITS: <its-without-_avz>_so3_guest.itb.
#   * Single-ITB bootm (any other ITS): the standalone SO3 / Linux image.

def __deploy_arm_common(d):
    import os
    import subprocess

    p1 = d.getVar('IB_FILESYSTEM_PATH') + "/p1"
    itb_dir = d.getVar('IB_ITB_PATH')
    plat = d.getVar('IB_PLATFORM')
    its = d.getVar('IB_TARGET_ITS') or ""
    IB_UENV = d.getVar("IB_UENV")

    if its.endswith("_avz"):
        # Two-ITB AVZ boot.
        avz_itb = f"{itb_dir}/{its}.itb"
        guest_itb = f"{itb_dir}/{its[:-len('_avz')]}_so3_guest.itb"
        avz_uenv = os.path.join(os.path.dirname(IB_UENV), f"uEnv_{plat}_avz.txt")

        for f in (avz_itb, guest_itb, avz_uenv):
            if not os.path.isfile(f):
                bb.fatal(f + " is missing ...")

        subprocess.run(['cp', avz_uenv, p1 + "/uEnv.txt"], check=True)
        subprocess.run(['cp', avz_itb, f"{p1}/{plat}_avz.itb"], check=True)
        subprocess.run(['cp', guest_itb, f"{p1}/{plat}_guest.itb"], check=True)
        return

    # Single-ITB bootm.
    itb_path = f"{itb_dir}/{its}.itb"

    if not os.path.isfile(IB_UENV):
        bb.fatal(IB_UENV + " is missing ...")

    subprocess.run(['cp', IB_UENV, p1 + "/uEnv.txt"])

    if not os.path.isfile(itb_path):
        bb.fatal(itb_path + " is missing ...")

    # The target ITB loaded by U-boot has the name of the target platform
    subprocess.run(['cp', itb_path, p1 + "/" + plat + ".itb"])


do_build_firmware () {
    bbplain "By default, no firmware..."
}
addtask do_build_firmware
