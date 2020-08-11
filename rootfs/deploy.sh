#!/bin/bash

if [ "$PLATFORM" == "" ]; then
    if [ "$2" == "" ]; then
        echo "PLATFORM must be defined (vexpress, rpi3, rpi4, bpi)"
        echo "You can invoke deploy.sh <platform>"
        exit 0
    fi
    
    PLATFORM=$2
fi
echo "Deploying the usr apps into the ramfs filesystem..."
./mount_ramfs.sh "$PLATFORM"
sudo cp -rf ../usr/out/* fs/
./umount_ramfs.sh "$PLATFORM"
