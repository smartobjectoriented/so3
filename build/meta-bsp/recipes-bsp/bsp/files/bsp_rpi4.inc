
# Deployment on RPi4 (32-bit) platform

require bsp_arm_common.inc

def __do_platform_deploy(d):

    bb.plain("Deploying on RPi4 32-bit platform")

    __deploy_arm_common

    src_dir = d.getVar('FILE_DIRNAME')
    dst_dir = d.getVar('IB_FILESYSTEM_PATH')
    u_boot_path = d.getVar('IB_UBOOT_PATH')

    cmd = f"cp -r ${src_dir}/files/rpi4/* ${dst_dir}/p1"
    result = subprocess.run(cmd, shell=True, check=True)
    
    cmd = f"cp -${u_boot_path}/u-boot.bin ${dst_dir}/p1/kernel7.img"
    result = subprocess.run(cmd, shell=True, check=True)
    
 