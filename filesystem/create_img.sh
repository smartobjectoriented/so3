#!/bin/bash

if [ $# -ne 1 ]; then
	echo "Please provide the board name (vexpress, rpi3, merida, bpi)"
	exit 0
fi 

# Partition layout on the sdcard:
# - Partition #1: 16 MB (u-boot, kernel, initrd, etc.)
# - Partition #2: 8 MB (main rootfs)

if [ "$1" == "vexpress" ]; then
    #create image first
    echo Creating sdcard.img ... 
    dd_size=32M
    
    dd if=/dev/zero of=sdcard.img bs="$dd_size" count=1
    devname=$(sudo losetup --partscan --find --show sdcard.img)

    # Keep device name only without /dev/
    devname=${devname#"/dev/"}
fi

if [ "$1" == "rpi3" -o "$1" == "bpi" ]; then
    echo "Specify the MMC device you want to deploy on (ex: sdb or mmcblk0 or other...)" 
    read devname
fi


if [ "$1" == "vexpress" -o "$1" == "rpi3" -o "$1" == "bpi" ]; then
#create the partition layout this way
    (echo o; echo n; echo p; echo; echo; echo +16M; echo t; echo c; echo n; echo p; echo; echo; echo +8M; echo w)   | sudo fdisk /dev/"$devname";
fi

if [[ "$devname" = *[0-9] ]]; then
    export devname="${devname}p"
fi

sudo mkfs.vfat /dev/"$devname"1
sudo mkfs.ext4 /dev/"$devname"2
