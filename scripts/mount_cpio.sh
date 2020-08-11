#!/bin/bash

calldir=`dirname "$0"`
rm -rf fs
mkdir fs
cd fs
fakeroot -- /bin/bash ../${calldir}/mount_cpio_fakeroot.sh $1
cd ..
