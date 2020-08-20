#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname "$script"))
# Make sure to be at the script location
cd "$SCRIPTPATH"

# Image parameters
image_size=128M
sd_card_image_name=sdcard.img

# Partition parameters
partition1=partition1.img
partition1_size=64M
partition1_type=c
partition2=partition2.img
partition2_size=8M
partition2_type=''

# Create partition 1
dd if=/dev/zero of=${partition1} bs=${partition1_size} count=1 status=none
mkfs.vfat ${partition1} > /dev/null

# Create partition 2
dd if=/dev/zero of=${partition2} bs=${partition2_size} count=1 status=none
mkfs.ext4 ${partition2} > /dev/null

# Partitions can be mounted like so :
#
# mkdir -p partition1
# sudo mount partition1.img partition1
# ls partition1
# sudo umount parition1
#
# These commands do require super user priviledges
# Therefore it is recommended to use other commands
# such as mcopy or mke2fs to populate them

# Create an empty image
dd if=/dev/zero of=${sd_card_image_name} bs=${image_size} count=1 status=none

# Set partition table
# man sfdisk for more information, sfdisk is a script-oriented tool for partitioning any block device.
# <start> <size> <type> <bootable>
# Empty string means default option
sfdisk ${sd_card_image_name} << EOF
${start_sector}, ${partition1_size}, ${partition1_type}
               , ${partition2_size}, ${partition2_type}
EOF
