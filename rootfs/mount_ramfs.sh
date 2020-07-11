#!/bin/bash
echo "Usage: ./mount_ramfs <board>"
echo "Here: board is $1"
echo "-------------------mount ramfs ---------------"

# mount the rootfs
mkdir -p fs

DEVLOOP=$(sudo losetup --partscan --find --show ./board/$1/rootfs.fat)

sudo mount ${DEVLOOP}p1 fs
