#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname "$script"))

directory=$(realpath "$SCRIPTPATH"/../usr/out)
[ $# == 2 ] && directory=$(realpath "$2") # Overwrite directory if provided
#echo "directory is ${directory}"

# Make sure to be at the script location
cd "$SCRIPTPATH"

# Check if board exists
[ $# == 0 ] && { echo "usage : ./deploy.sh <board> [directory to copy]" ; exit 1 ; }
[ ! -d board/$1 ] && { echo "board $1 not found in board directory, exiting..." ; exit 1 ; }
[ ! -d "${directory}" ] && { echo "directory ${directory} to copy not found, exiting..." ; exit 1 ; }

fs_name=rootfs.fat
# Block size can be checked with fdisk -l (sector size)
block_size=512
# Partition start can be checked with fdisk -l too
start_pos=2048
# Create temporary directory
tmp_dir=$(mktemp -d -t so3-rootfs-XXXXXXXX)
partition="${tmp_dir}/${fs_name}1"

echo "Copying ${directory}/* into the ramfs filesystem..."

# Extract the first partition
dd if="board/$1/${fs_name}" of="${partition}" bs=$block_size skip=$start_pos status=none

# Here the partition is of size 2048-1 because of start position, which is not a multiple of 32
# Therefore we need to skip the checks for mcopy !
export MTOOLS_SKIP_CHECK=1

# Copy the files from usr/out to the partition (fat) the tmp dir is kept for inspection if fail
# -o option is for overwrite
mcopy -o -i ${partition} ${directory}/* :: || { echo "mcopy failed, exiting..." ;
                                                echo "temporary directory is ${tmp_dir}"
                                                exit 1 ; }

# Write the first partition back to the binary file
dd if="${partition}" of="board/$1/${fs_name}" conv=notrunc bs=${block_size} seek=$start_pos status=none

# Delete temporary directory
rm -r "${tmp_dir}"
