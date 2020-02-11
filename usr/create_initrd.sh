#!/bin/bash

#create image first
dd if=/dev/zero of=initrd bs=4096 count=4096
DEVLOOP=$(sudo losetup --partscan --find --show initrd)

#create the partition this way
(echo o; echo n; echo p; echo; echo; echo; echo; echo; echo t; echo; echo c; echo w) | sudo fdisk $DEVLOOP;

sudo mkfs.vfat ${DEVLOOP}p1
sudo losetup -d $DEVLOOP
