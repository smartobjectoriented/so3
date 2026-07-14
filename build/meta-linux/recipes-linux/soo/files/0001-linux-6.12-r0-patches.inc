# Copyright (c) 2025-2026 EDGEMTech SA
#
# virt64 6.12 agency specifics on top of soo-generic-patches.inc:
# the guest device tree and the SOO defconfig.

SRC_URI:append:soo = "\
    file://0001-virt64_guest.dts.patch \
    file://0002-virt64_soo_defconfig.patch \
"
