#!/bin/bash

if [ $# -ne 1 ]; then
        echo "Usage: ./umount_ramfs <board>"
	echo "Please provide the board name (virt32, rpi4, virt64, rpi4_64)"
	exit 0
fi

echo "Here: board is $1"
 
sudo umount fs
sudo losetup -D
sudo rm -rf fs


