#! /bin/bash

# Here, you can do something after the rootfs images has been
# generated.
# See post_build.sh script if you want to do something just before
# the images are generated.
# Note: you should have access to most BR variables here.

# you can change the passed arguments in config
IMAGES_DIR=$1
DST_DIR=$2
AUTO_NFS=$3


