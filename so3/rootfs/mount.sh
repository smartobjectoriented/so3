#!/bin/bash

echo "-------------------mount ramfs ---------------"

# mount the rootfs
mkdir -p fs

DEVLOOP=$(sudo losetup --partscan --find --show ./rootfs.fat)

sudo mount ${DEVLOOP}p1 fs
