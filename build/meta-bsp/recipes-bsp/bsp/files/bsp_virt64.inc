# Copyright (c) 2025-2026 EDGEMTech SA
#
# virt64 (QEMU armv8) platform deploy glue. The ITB and uEnv.txt are
# copied onto the boot partition (p1) of the storage image by
# __deploy_arm_common (bsp_arm_common.inc); U-Boot itself is loaded
# directly by QEMU, so there is no boot-chain artefact to stage here.

require bsp_arm_common.inc

def __do_platform_deploy(d):
    __deploy_arm_common(d)
