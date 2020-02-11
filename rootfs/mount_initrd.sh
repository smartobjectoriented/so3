#!/bin/bash
echo "Usage: ./mount_initrd <board>"
echo "Here: board is $1"
echo "-------------------mount initrd ---------------"
../tools/mount_cpio $PWD/board/$1/initrd.cpio
