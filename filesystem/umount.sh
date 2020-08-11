#!/bin/bash

sudo umount fs

if [ "$PLATFORM" == "vexpress" -o "$PLATFORM" == "merida" ]; then
    sudo losetup -D
fi
