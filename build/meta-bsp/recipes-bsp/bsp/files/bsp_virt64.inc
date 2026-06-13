require bsp_arm_common.inc

do_itb () {
	if [ ! -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ]; then
		bbfatal "${IB_DIR} No corresponding ITS found for platform ${IB_PLATFORM} ${IB_ITB_PATH}/${IB_TARGET_ITS}.its"
	else
		mkimage -f ${IB_ITB_PATH}/${IB_TARGET_ITS}.its ${IB_ITB_PATH}/${IB_TARGET_ITS}.itb
	fi
}


def __do_platform_deploy(d):
    import subprocess
    
    print("Platorm " + d.getVar('IB_PLATFORM'))

    ovrs = (d.getVar('OVERRIDES') or '').replace(' ', '').split(':')

    # Are we in Torizon recipes?
    if 'torizon' not in ovrs:

        __deploy_arm_common(d)

    else:

        IB_UBOOT_PATH = d.getVar("IB_UBOOT_PATH")
        IB_PLATFORM = d.getVar("IB_PLATFORM")
        IB_ITB_PATH = d.getVar("IB_ITB_PATH")
        IB_FILESYSTEM_PATH = d.getVar("IB_FILESYSTEM_PATH")
        IB_UENV = d.getVar("IB_UENV")

        torizon_rootfs_ar_path = __torizon_rootfs_archive_path(d)
        filesys_path = f"{IB_FILESYSTEM_PATH}/p1"
        itb_path = f"{IB_ITB_PATH}/{IB_PLATFORM}.itb"

        cmd = (f"tar -tf {torizon_rootfs_ar_path} | grep ^\./ostree/boot.1.1 | "
            f"grep -o /[a-f0-9]*/$ | sed 's/\///g\'")

        res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
        deployment_id = res.stdout.strip()

        if not os.path.isfile(IB_UENV):
            bb.fatal(IB_UENV + " is missing ...")

        # Read the uEnv file perform the substitution
        with open(IB_UENV) as uenv_file:
            uenv_content = uenv_file.read()
            uenv_file.close()

        uenv_content = uenv_content.replace("${OSTREE_DEPLOYMENT_ID}", deployment_id)

        if not os.path.isfile(itb_path):
            bb.fatal(itb_path + " is missing ...")

        try:
            with open(f"{filesys_path}/uEnv.txt", "w") as dest_uenv_file:
                dest_uenv_file.write(uenv_content)

        except Exception as e:
            __do_fs_umount(d)
            bb.fatal(f"An error has occurred: {e}")

        subprocess.run(["cp", itb_path, filesys_path])
