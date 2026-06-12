
# Deployment on RPi4_64 (64-bit) platform

require bsp_arm_common.inc

def __do_platform_deploy(d):

    import os
    from bb.process import run

    print("Platorm " + d.getVar('IB_PLATFORM'))
   
    filedirname = d.getVar('FILE_DIRNAME')
    dst = d.getVar('IB_FILESYSTEM_PATH')
    uboot_path = d.getVar('IB_UBOOT_PATH')

    run(f"cp -r {filedirname}/../bsp/rpi4/* {dst}/p1")
    run(f"cp {uboot_path}/u-boot.bin {dst}/p1/kernel8.img")

    __deploy_arm_common(d)
