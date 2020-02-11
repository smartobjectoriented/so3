#!/bin/bash

# mount the initrd
mkdir -p fs

DEVLOOP=$(sudo losetup --partscan --find --show initrd)

sudo mount ${DEVLOOP}p1 fs 
