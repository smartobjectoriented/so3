#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Please provide the board name (vexpress, rpi4)"
	exit 0
fi 

# Partition layout on the sdcard:
# - Partition #1: 64 MB (u-boot, kernel, etc.)
# - Partition #2: 180 MB (agency main rootfs)

if [ "$1" == "vexpress" ]; then
    #create image first
    echo Creating sdcard.img.$1 ... 
    dd_size=450M
    dd if=/dev/zero of=sdcard.img.$1 bs="$dd_size" count=1
    devname=$(sudo losetup --partscan --find --show sdcard.img.$1)

    # Keep device name only without /dev/
    devname=${devname#"/dev/"}
fi

if [ "$1" == "rpi4" ]; then
    echo "Specify the MMC device you want to deploy on (ex: sdb or mmcblk0 or other...)" 
    read devname
fi


if [ "$1" == "vexpress" -o "$1" == "rpi4" ]; then
# Create the partition layout this way
    (echo o; echo n; echo p; echo; echo; echo +64M; echo t; echo c; echo n; echo p; echo; echo; echo +180M; echo w)   | sudo fdisk /dev/"$devname";
fi

# Give a chance to the real SD-card to be sync'd
sleep 1s

if [[ "$devname" = *[0-9] ]]; then
    export devname="${devname}p"
fi

sudo mkfs.vfat /dev/"$devname"1
sudo mkfs.ext4 /dev/"$devname"2

if [ "$1" == "vexpress" -o "$1" == "rpi4" ]; then
    sudo losetup -D
fi
