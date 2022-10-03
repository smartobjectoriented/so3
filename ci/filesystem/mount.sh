#!/bin/bash

if [ "$PLATFORM" == "" ]; then
    if [ "$2" == "" ]; then
        echo "PLATFORM must be defined (vexpress, virt64, rpi4, rpi4_64)"
        echo "You can invoke mount.sh <partition_nr> <platform>"
        exit 0
    fi
    
    PLATFORM=$2
fi

rm -rf fs/*
mkdir -p fs
 
if [ "$PLATFORM" == "vexpress" -o "$PLATFORM" == "virt64" ]; then
     LOOPDEV=$(losetup --find --show --partscan sdcard.img.${PLATFORM})
    FS_IMG=sdcard.img.${PLATFORM}
PARTITIONS=$(lsblk --raw --output "MAJ:MIN" --noheadings ${LOOPDEV} | tail -n +2)
COUNTER=1
for i in $PARTITIONS; do
        MAJ=$(echo $i | cut -d: -f1)
        MIN=$(echo $i | cut -d: -f2)
        if [ ! -e "${LOOPDEV}p${COUNTER}" ]; then mknod ${LOOPDEV}p${COUNTER} b $MAJ $MIN; fi
        COUNTER=$((COUNTER + 1))
done
    # losetup -P --find --show flash
   

    # device is loopback (loop<n>)
    mount ${LOOPDEV}p$1 fs 
    exit 0
fi

if [ ${LOOPDEV} == "" ]; then
    echo "Specify the device name of MMC (ex: sdb or mmcblk0 or other...)" 
    read devname
fi

if [ "$PLATFORM" == "rpi4" -o "$PLATFORM" == "rpi4_64" ]; then
    mount ${LOOPDEV}p$1 fs
fi
