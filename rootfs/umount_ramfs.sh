#!/bin/bash
echo "Usage: ./umount_ramfs <board>"
echo "Here: board is $1"

sudo umount fs
sudo losetup -D


