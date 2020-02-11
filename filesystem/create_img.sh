#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Please provide the board name (vexpress, rpi4, bpi)"
	exit 0
fi 

# Partition layout on the sdcard:
# - Partition #1: 16 MB (u-boot, kernel, etc.)
# - Partition #2: 512 MB (linux rootfs)

if [ "$1" == "vexpress" ]; then
    #create image first
    echo Creating sdcard.img ... 
    
    if [ "$1" == "vexpress" ]; then
        dd_size=1G
    else
        dd_size=128M
    fi
    dd if=/dev/zero of=sdcard.img bs="$dd_size" count=1
    devname=$(sudo losetup --partscan --find --show sdcard.img)
    
    # Keep device name only without /dev/
    devname=${devname#"/dev/"}
fi

if [ "$1" == "rpi4" -o "$1" == "bpi" ]; then
    echo "Specify the MMC device you want to deploy on (ex: sdb or mmcblk0 or other...)" 
    read devname
fi

if [ "$1" == "vexpress" -o "$1" == "rpi4" -o "$1" == "bpi" ]; then
    #create the partition layout this way
    (echo o; echo n; echo p; echo; echo; echo +64M; echo t; echo c; echo n; echo p; echo; echo; echo +512M; echo w)   | sudo fdisk /dev/"$devname";
fi

if [[ "$devname" = *[0-9] ]]; then
    export devname="${devname}p"
fi

sudo mkfs.vfat /dev/"$devname"1
 sudo mkfs.ext4 /dev/"$devname"2
