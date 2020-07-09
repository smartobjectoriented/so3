#!/bin/bash

#create image first
dd if=/dev/zero of=board/$1/initrd.fat bs=1024 count=1024
DEVLOOP=$(sudo losetup --partscan --find --show board/$1/initrd.fat)

#create the partition this way
(echo o; echo n; echo p; echo; echo; echo; echo; echo; echo t; echo; echo c; echo w) | sudo fdisk $DEVLOOP;

sudo mkfs.vfat ${DEVLOOP}p1
sudo losetup -D
