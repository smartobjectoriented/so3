#!/bin/bash
echo "Usage: ./umount_initrd <board>"
echo "Here: board is $1"

# Currently not using cpio but fat
#../tools/umount_cpio $PWD/board/$1/initrd.cpio
sudo umount fs
sudo losetup -D


