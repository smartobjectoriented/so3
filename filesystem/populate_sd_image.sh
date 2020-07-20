#!/bin/bash

partition1=partition1.img
partition2=partition2.img
sd_card_image_name=sdcard.img

# Partition parameters
start_sector=2048
# sfdisk -l sdcard.img will show sector size
sector_size=512

# Copy the partitions
cat ${partition1} ${partition2} >> partitions.img
dd if=partitions.img of=${sd_card_image_name} bs=${sector_size} seek=${start_sector} conv=notrunc
rm partitions.img
