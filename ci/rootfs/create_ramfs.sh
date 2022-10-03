#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname "$script"))

# Make sure to be at the script location
cd "$SCRIPTPATH"

# Make sure the dir exists

# Check if board exists
[ $# == 0 ] && { echo "usage : $script <board>" ; exit 1 ; }
[ ! -d board/$1 ] && { echo "board $1 not found in board directory, creating it..." ; mkdir -p board/$1 ; }

start_sector=2048
partition_size=16M # This image will be copied into the .itb and written to SD card image so it must be small enough
partition_type=c
tmp_dir=$(mktemp -d -t so3-rootfs-XXXXXXXX)
partition="${tmp_dir}/partition.img"

# Create image first
image_name="board/$1/rootfs.fat"
dd if=/dev/zero of="${image_name}" count=${start_sector} status=none

# Append the formatted partition
dd if=/dev/zero of="${partition}" bs=${partition_size} count=1 status=none
mkfs.vfat "${partition}" > /dev/null
dd if="${partition}" status=none >> "${image_name}"

# Set the partition table
sfdisk "${image_name}" <<EOF
${start_sector}, ${partition_size}, ${partition_type}
EOF

# Delete temporary directory
rm -r "${tmp_dir}"
