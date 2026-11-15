# Copyright (c) 2026 EDGEMTech SA

# Storage of the x86 PC: same layout as the ARM platforms, p1 (FAT, boot)
# starting at 1 MiB leaves room for the GRUB core image after the MBR.

inherit fs_arm_common
