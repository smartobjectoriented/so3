#!/bin/bash

if [ "$_PLATFORM" == "" ]; then
    if [ "$2" == "" ]; then
        echo "_PLATFORM must be defined (vexpress, rpi4, bpi)"
        echo "You can invoke mount.sh <partition_nr> <platform>"
        exit 0
    fi
    
    _PLATFORM=$2
fi

if [ "$_PLATFORM" == "vexpress" ]; then
    devname=$(sudo losetup --partscan --find --show sdcard.img)
    FS_IMG=sdcard.img

    #sudo losetup -P --find --show flash
    mkdir -p fs

    # device is loopback (loop<n>)
    sudo mount ${devname}p$1 fs 
    exit 0
fi

if [ "$devname" == "" ]; then
    echo "Specify the device name of MMC (ex: sdb or mmcblk0 or other...)" 
    read devname
fi

if [[ "$devname" = *[0-9] ]]; then
    export devname="${devname}p"
fi

if [ "$_PLATFORM" == "rpi4" -o "$_PLATFORM" == "bpi" ]; then
    mkdir -p fs
    sudo mount /dev/"$devname"$1 fs
fi
