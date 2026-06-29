# Copyright (c) 2025-2026 EDGEMTech SA

require bsp_arm_common.inc

def __do_platform_deploy(d):

        print("Platorm " + d.getVar('IB_PLATFORM'))

        __deploy_arm_common(d)


