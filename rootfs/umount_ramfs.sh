#!/bin/bash

if [ $# -ne 1 ]; then
        echo "Usage: ./umount_ramfs <board>"
	echo "Please provide the board name (vexpress, rpi4)"
	exit 0
fi

echo "Here: board is $1"
 
sudo umount fs
sudo losetup -D
sudo rm -rf fs


