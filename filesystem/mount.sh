#!/bin/bash

if [ "$PLATFORM" == "" ]; then
    if [ "$2" == "" ]; then
        echo "PLATFORM must be defined (virt32, virt64, rpi4, rpi4_64)"
        echo "You can invoke mount.sh <partition_nr> <platform>"
        exit 0
    fi
    
    PLATFORM=$2
fi

sudo rm -rf fs/*
mkdir -p fs
 
if [ "$PLATFORM" == "virt32" -o "$PLATFORM" == "virt64" ]; then
    devname=$(sudo losetup --partscan --find --show sdcard.img.${PLATFORM})
    FS_IMG=sdcard.img.${PLATFORM}

    #sudo losetup -P --find --show flash
   

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

if [ "$PLATFORM" == "rpi4" -o "$PLATFORM" == "rpi4_64" ]; then
    sudo mount /dev/"${devname}"$1 fs
fi
