#!/bin/bash

echo Deploying the rootfs in the primary partition...

if [ "$PLATFORM" == "" ]; then
    if [ "$1" == "" ]; then
        echo "PLATFORM must be defined (virt32, virt64, rpi4, rpi4_64)"
        echo "You can invoke deploy.sh <platform>"
        exit 0
    fi
    
    PLATFORM=$1
fi

./mount.sh
cd ../filesystem
./mount.sh 1
sudo rm -rf fs/*
sudo cp -rf ../rootfs/fs/* fs/

# Sometimes, syncing between RAM and FS takes some time
sleep 1

./umount.sh 
cd ../rootfs
./umount.sh

