#!/bin/bash

sudo umount fs

if [ "$PLATFORM" == "vexpress" ]; then
    sudo losetup -D
fi
