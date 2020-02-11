#!/bin/bash

echo Deploying secondary rootfs into the first partition...
./mount_cpio.sh
cd ../filesystem
./mount.sh 1
sudo rm -rf fs/*
sudo cp -rf ../rootfs/initrd/* fs/
./umount.sh
cd ../rootfs
./umount_cpio.sh
