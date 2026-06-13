#!/bin/sh

cd $IB_ROOT_DIR/build
sudo -E ./bitbake/bin/bitbake filesystem -c fs_mount $1
cd -
