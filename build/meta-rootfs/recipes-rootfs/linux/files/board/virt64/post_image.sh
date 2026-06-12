#! /bin/bash

# Here, you can do something after the rootfs images has been
# generated.
# See post_build.sh script if you want to do something just before
# the images are generated.
# Note: you should have access to most BR variables here.
cp ${BASE_DIR}/images/rootfs.cpio ${BASE_DIR}/board/virt64

