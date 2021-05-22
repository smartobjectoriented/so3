#!/bin/bash

sudo umount fs

if [ "$PLATFORM" == "vexpress" -o "$PLATFORM" = "virt-riscv64" ]; then
    sudo losetup -D
fi
