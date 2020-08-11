#!/bin/bash

# Create image first

# Make sure the dir exists
mkdir -p board/$1

dd if=/dev/zero of=board/$1/rootfs.fat bs=1024 count=1024
DEVLOOP=$(sudo losetup --partscan --find --show board/$1/rootfs.fat)

#create the partition this way
(echo o; echo n; echo p; echo; echo; echo; echo; echo; echo t; echo; echo c; echo w) | sudo fdisk $DEVLOOP;

sudo mkfs.vfat ${DEVLOOP}p1
sudo losetup -D
