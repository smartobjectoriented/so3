#!/bin/bash

if [ $# -ne 1 ]; then
        echo "Usage: ./mount_ramfs <board>"
	echo "Please provide the board name (irt32, rpi4, virt64, rpi_64)"
	exit 0
fi

echo "Here: board is $1"
echo "-------------------mount ramfs ---------------"

# mount the rootfs
mkdir -p fs

DEVLOOP=$(sudo losetup --partscan --find --show ./board/$1/rootfs.fat)

sudo mount ${DEVLOOP}p1 fs
