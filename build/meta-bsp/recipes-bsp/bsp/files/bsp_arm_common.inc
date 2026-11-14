# Copyright (c) 2025-2026 EDGEMTech SA

# Deployment of ITB for all ARM platforms (FAT boot partition p1).
#
# Two deployment shapes, selected by the chosen ITS:
#   * AVZ two-ITB boot (IB_TARGET_ITS ends in "_avz"): stage the AVZ ITB
#     (<plat>_avz.itb) AND the SO3 guest ITB (<plat>_guest.itb) plus the
#     uEnv_<plat>_avz.txt that loads both and jumps via U-Boot's guest-boot
#     command (AVZ FIT in x0, guest ITB in x1). The guest ITB is derived
#     from IB_TARGET_ITS: <its-without-_avz>_so3_guest.itb.
#   * Single-ITB bootm (any other ITS): the standalone SO3 / Linux image.

# ITBs the previous deploy may have left on p1. The two boot shapes use
# different names, so switching between them (IB_HYPERVISOR none <-> avz)
# otherwise accumulates both sets: on a Linux BSP each ITB carries a kernel
# plus an initramfs and runs to tens of megabytes, so the leftovers alone
# fill the FAT boot partition and the copy below fails with a bare
# "cp: returned exit status 1" — or, worse, succeeds against a partially
# written file. Cleaning first also means p1 always describes exactly one
# boot shape, which is what someone inspecting the card expects to see.

def __clean_stale_itbs(p1, plat, keep):
    import os

    for name in (f"{plat}.itb", f"{plat}_avz.itb", f"{plat}_guest.itb"):
        if name in keep:
            continue
        path = os.path.join(p1, name)
        if os.path.isfile(path):
            bb.note(f"removing stale {name} from the boot partition")
            os.remove(path)


def __deploy_arm_common(d):
    import os
    import subprocess

    p1 = d.getVar('IB_FILESYSTEM_PATH') + "/p1"
    itb_dir = d.getVar('IB_ITB_PATH')
    plat = d.getVar('IB_PLATFORM')
    its = d.getVar('IB_TARGET_ITS') or ""
    IB_UENV = d.getVar("IB_UENV")

    if its.endswith("_avz"):
        # Two-ITB AVZ boot. The guest ITB basename is OS-specific
        # (_so3_guest for SO3, _linux_guest for the Linux agency).
        guest_suffix = d.getVar('IB_GUEST_SUFFIX') or "_so3_guest"
        avz_itb = f"{itb_dir}/{its}.itb"
        guest_itb = f"{itb_dir}/{its[:-len('_avz')]}{guest_suffix}.itb"
        avz_uenv = os.path.join(os.path.dirname(IB_UENV), f"uEnv_{plat}_avz.txt")

        for f in (avz_itb, guest_itb, avz_uenv):
            if not os.path.isfile(f):
                bb.fatal(f + " is missing ...")

        __clean_stale_itbs(p1, plat, keep=(f"{plat}_avz.itb", f"{plat}_guest.itb"))

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

    __clean_stale_itbs(p1, plat, keep=(f"{plat}.itb",))

    # The target ITB loaded by U-boot has the name of the target platform
    subprocess.run(['cp', itb_path, p1 + "/" + plat + ".itb"])


do_build_firmware () {
    bbplain "By default, no firmware..."
}
addtask do_build_firmware
