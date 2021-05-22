#!/bin/bash

if [ "$PLATFORM" == "" ]; then
    if [ "$2" == "" ]; then
        echo "PLATFORM must be defined (vexpress, rpi4, virt-riscv64)"
        echo "You can invoke mount.sh <partition_nr> <platform>"
        exit 0
    fi
    
    PLATFORM=$2
fi

if [ "$PLATFORM" == "vexpress" -o "$PLATFORM" = "merida" -o "$PLATFORM" = "virt-riscv64" ]; then
    devname=$(sudo losetup --partscan --find --show sdcard.img.${PLATFORM})
    FS_IMG=sdcard.img.${PLATFORM} # TODO unused???

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

if [ "$PLATFORM" == "rpi3" -o "$PLATFORM" == "bpi" -o "$PLATFORM" == "rpi4" ]; then
    mkdir -p fs
    sudo mount /dev/"$devname"$1 fs
fi
