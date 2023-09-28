#!/bin/bash

echo Fetching QEMU 8.0.0
wget https://download.qemu.org/qemu-8.0.0.tar.xz

echo Extracting QEMU tar
tar xf qemu-8.0.0.tar.xz

echo Moving to its final destination
mv qemu-8.0.0/* .
rm -rf qemu-8.0.0

echo Patching
patch -p1 < qemu.patch

echo Removing the tar file
rm qemu-8.0.0.tar.xz

