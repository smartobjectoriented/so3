#!/bin/bash

calldir=`dirname "$0"`
cp -L $1 $1.backup
cd fs
fakeroot -- /bin/bash ../${calldir}/umount_cpio_fakeroot.sh $1
cd ..
rm -rf fs
