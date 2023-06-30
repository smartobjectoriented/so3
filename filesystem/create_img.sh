#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Please provide the board name (vexpress, virt32, virt64, rpi4)"
	exit 0
fi 

# Partition layout on the sdcard:
# - Partition #1: 64 MB (u-boot, kernel, etc.)
# - Partition #2: 180 MB (agency main rootfs)

if [ "$1" == "virt32" -o "$1" == "virt64" ]; then
    #create image first
    echo Creating sdcard.img.$1 ... 
    dd_size=256M
    dd if=/dev/zero of=sdcard.img.$1 bs="$dd_size" count=1
    devname=$(sudo losetup --partscan --find --show sdcard.img.$1)

    # Keep device name only without /dev/
    devname=${devname#"/dev/"}
fi

if [ "$1" == "rpi4" -o "$1" == "rpi4_64" ]; then
    echo "Specify the MMC device you want to deploy on (ex: sdb or mmcblk0 or other...)" 
    read devname
fi

if [ "$1" == "virt32" -o "$1" == "rpi4" -o "$1" == "rpi4_64" -o "$1" == "virt64" ]; then
#create the partition layout this way
    (echo o; echo n; echo p; echo; echo; echo; echo t; echo c; echo w) | sudo fdisk /dev/"$devname";
fi

echo Waiting...
# Give a chance to the real SD-card to be sync'd
sleep 2s

if [[ "$devname" = *[0-9] ]]; then
    export devname="${devname}p"
fi

sudo mkfs.fat -F32 -v /dev/"$devname"1
sudo mkfs.ext4 /dev/"$devname"2

if [ "$1" == "virt32" -o "$1" == "virt64" ]; then
	sudo losetup -D
fi

