#!/bin/bash
echo "Usage: ./mount_initrd <board>"
echo "Here: board is $1"
echo "-------------------mount initrd ---------------"

# Currently not using cpio but fat
#../tools/mount_cpio $PWD/board/$1/initrd.cpio
# mount the initrd
mkdir -p fs

DEVLOOP=$(sudo losetup --partscan --find --show ./board/$1/initrd.fat)

sudo mount ${DEVLOOP}p1 fs
