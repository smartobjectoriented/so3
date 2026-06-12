#!/bin/sh
source $IB_ROOT_DIR/env.sh

cd $IB_ROOT_DIR/build
sudo -E ./bitbake/bin/bitbake filesystem -c fs_init_storage $1
cd -
