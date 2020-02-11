#!/bin/bash
echo "Usage: ./umount_initrd <board>"
echo "Here: board is $1"
../tools/umount_cpio $PWD/board/$1/initrd.cpio

