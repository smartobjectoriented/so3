# Copyright (c) 2025-2026 EDGEMTech SA

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

    # bsp/rpi4/config.txt is shared with the 32-bit BSP and deliberately holds
    # no arm_64bit/kernel setting; append the 64-bit half of the chain.
    # FILE_DIRNAME is the *recipe's* directory (recipes-bsp/so3), not this
    # include's — hence the ../bsp hop, as in the copy above.
    run(f"cat {filedirname}/../bsp/files/config_rpi4_arm64.txt >> {dst}/p1/config.txt")

    run(f"cp {uboot_path}/u-boot.bin {dst}/p1/kernel8.img")

    __deploy_arm_common(d)
