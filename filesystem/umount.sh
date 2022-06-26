#!/bin/bash

sudo umount fs

if [ "$PLATFORM" == "vexpress" -o "$PLATFORM" == "virt64" ]; then
    sudo losetup -D
fi
