
# Deployment of ITB for all ARM platforms

def __deploy_arm_common(d):
    import os
    import subprocess

    IB_UENV = d.getVar("IB_UENV")
    filesys_path = d.getVar('IB_FILESYSTEM_PATH') + "/p1"
    itb_path = d.getVar('IB_ITB_PATH') + "/" + d.getVar('IB_TARGET_ITS') + ".itb"

    if not os.path.isfile(IB_UENV):
        bb.fatal(uenv_path + " is missing ...")

    subprocess.run(['cp', IB_UENV, filesys_path + "/uEnv.txt"])

    if not os.path.isfile(itb_path):
        bb.fatal(itb_path + " is missing ...")

    # The target ITB loaded by U-boot has the name of the target platform
    subprocess.run(['cp', itb_path, filesys_path + "/" + d.getVar('IB_PLATFORM') + ".itb"])


do_build_firmware () {
    bbplain "By default, no firmware..."
}
addtask do_build_firmware
