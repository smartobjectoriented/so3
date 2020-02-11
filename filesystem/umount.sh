#!/bin/bash

sudo umount fs

if [ "$_PLATFORM" == "vexpress" -o "$_PLATFORM" == "merida" ]; then
    sudo losetup -D
fi
