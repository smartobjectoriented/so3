#!/bin/bash

sudo umount fs

if [ "$_PLATFORM" == "vexpress" ]; then
    sudo losetup -D
fi
