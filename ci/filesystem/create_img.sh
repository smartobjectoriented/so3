#!/bin/bash

# Partition layout on the sdcard:
# - Partition #1: 64 MB (u-boot, kernel, etc.)
# - Partition #2: 180 MB (agency main rootfs)

if [ "$1" == "vexpress" -o "$1" == "virt64" ]; then
    #create image first
    echo Creating sdcard.img.$1 ... 
    dd_size=68M
    dd if=/dev/zero of=sdcard.img.$1 bs="$dd_size" count=1

    # Keep device name only without /dev/

    LOOPDEV=$(losetup --find --show --partscan sdcard.img.$1)
fi

if [ "$1" == "rpi4" -o "$1" == "rpi4_64" ]; then
    echo "Specify the MMC device you want to deploy on (ex: sdb or mmcblk0 or other...)" 
    read devname
fi

echo ${LOOPDEV}

if [ "$1" == "vexpress" -o "$1" == "rpi4" -o "$1" == "rpi4_64" -o "$1" == "virt64" ]; then
#create the partition layout this way
    (echo o; echo n; echo p; echo; echo; echo +64M; echo t; echo c; echo w)   | fdisk ${LOOPDEV};
fi

echo Waiting...
# Give a chance to the real SD-card to be sync'd
sleep 2s

PARTITIONS=$(lsblk --raw --output "MAJ:MIN" --noheadings ${LOOPDEV} | tail -n +2)
COUNTER=1
for i in $PARTITIONS; do
        MAJ=$(echo $i | cut -d: -f1)
        MIN=$(echo $i | cut -d: -f2)
        if [ ! -e "${LOOPDEV}p${COUNTER}" ]; then mknod ${LOOPDEV}p${COUNTER} b $MAJ $MIN; fi
        COUNTER=$((COUNTER + 1))
done

mkfs.fat -F32 -v ${LOOPDEV}p1

if [ "$1" == "vexpress" -o "$1" == "virt64" ]; then
        losetup -D
fi
