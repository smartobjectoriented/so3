#!/bin/bash

if [ "$PLATFORM" == "" ]; then
    if [ "$1" == "" ]; then  
        echo "PLATFORM must be defined (virt32, virt64, rpi4, rpi4_64, so3virt)"
        echo "You can invoke deploy.sh <platform>"
        exit 0
    fi
    
    PLATFORM=$1
fi
echo "Here: board is  ${PLATFORM}"
echo "------------------- deploy usr apps in so3  ---------------"


echo Deploying user apps into the ramfs partition

cd ../rootfs
./mount.sh  ${PLATFORM}
sudo cp -r ../usr/out/* fs
sudo cp -r ../usr/build/deploy/* fs
./umount.sh  ${PLATFORM}

