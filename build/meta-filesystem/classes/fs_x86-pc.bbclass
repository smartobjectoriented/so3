
# TODO: move to a Python function

# Specific task description for formatting
# the storage of x86 platform with one partition

do_filesystem_platform_init_storage () {

        # Partition layout is as follows:
	# - Partition #1: 8 GB with rootfs including kernel image and GRUB

	if [ "${IB_STORAGE}" = "soft" ]; then
   		# Create image first
   		bbnote "Creating sdcard.img.${IB_PLATFORM} ..."

   		dd_size=512M

 		truncate -s $dd_size ${WORKDIR}/sdcard.img.${IB_PLATFORM}

   		devname=$(losetup --partscan --find --show ${WORKDIR}/sdcard.img.${IB_PLATFORM})

   		# Keep device name only without /dev/
   		devname=${devname#"/dev/"}

		bbnote "Linking the storage image ${IB_DIR}"

		mkdir -p ${WORKDIR}/filesystem

		ln -fs ${WORKDIR}/sdcard* ${IB_DIR}/filesystem/

 	else
 		devname=${IB_DEVICE:${IB_PLATFORM}}
 	fi

 	bbnote "devname is defined as $devname"

	# Create the partition layout this way

	parted -s /dev/"$devname" mklabel msdos mkpart primary ext4 1MiB 100%

	bbnote Waiting...

	# Give a chance to the real SD-card to be sync'd
	sleep 2s

	if [[ "$devname" = *[0-9] ]]; then
    	        export devname="${devname}p"
	fi

	set +e

	mke2fs -F -t ext4 /dev/"$devname"1
  	e2label /dev/"$devname"1 rootfs

	set -e

	if [ "${IB_STORAGE}" = "soft" ]; then
    	        losetup -D
	fi

	bbnote "Done! The storage is now initialized"
}
