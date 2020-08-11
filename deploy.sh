#!/bin/bash

usage()
{
  echo "Usage: $0 [OPTIONS] <ME_NAME>"
  echo ""
  echo "Where OPTIONS are:"
  echo "  -a    Deploy all"
  echo "  -b    Deploy boot (kernel, U-boot, etc.)"
  echo "  -r    Deploy usr apps into the rootfs (here as ramfs)"
  echo "  -u    Deploy usr apps in the first partition of the sdcard"
  echo ""
  exit 1
}

while getopts "abru" o; do
  case "$o" in
    a)
      deploy_boot=y
      deploy_usr=y
      deploy_rootfs=y
      ;;
    b)
      deploy_boot=y
      ;;
    r)
      deploy_rootfs=y
      ;;
    u)
      deploy_usr=y
      ;;
    *)
      usage
      ;;
  esac
done

if [ $OPTIND -eq 1 ]; then usage; fi

while read var; do
if [ "$var" != "" ]; then
  export $(echo $var | sed -e 's/ //g' -e /^$/d -e 's/://g' -e /^#/d)
fi
done < build.conf

if [ "$PLATFORM" != "vexpress" ]; then
    echo "Specify the device name of MMC (ex: sdb or mmcblk0 or other...)" 
    read devname
    export devname="$devname"
fi

# Deploy usr apps before boot components in case if the rootfs
# is embedded in a ramfs file

if [ "$deploy_usr" == "y" ]; then
 
    # Deploy all usr applications into the rootfs (first partition)
    cd usr
    ./deploy.sh
    cd ..
fi

if [ "$deploy_rootfs" == "y" ]; then
 
    # Deploy all usr applications into the ramfs
    cd rootfs
    ./deploy.sh
    cd ..
fi


if [ "$deploy_boot" == "y" ]; then
    # Deploy files into the boot partition (first partition)
    echo Deploying boot files into the first partition...
     
    cd target
    ./mkuboot.sh ${PLATFORM}
    cd ../filesystem
    ./mount.sh 1
    sudo rm -rf fs/*
    sudo cp ../target/${PLATFORM}.itb fs/
    sudo cp ../u-boot/uEnv.d/uEnv_${PLATFORM}.txt fs/uEnv.txt
       
    if [ "$PLATFORM" == "vexpress" ]; then
	# Nothing else ...
        ./umount.sh
        cd ..
    fi
 
    if [ "$PLATFORM" == "rpi3" ]; then
        sudo cp -r../bsp/rpi3/* fs/
        sudo cp -r ~/sootech/rpi-bsp/boot/* fs/
        sudo cp ../u-boot/u-boot.bin fs/kernel.img
        ./umount.sh
        cd ..
    fi
    
    if [ "$PLATFORM" == "rpi4" ]; then
        sudo cp -r ../bsp/rpi4/* fs/
        sudo cp ../u-boot/u-boot.bin fs/kernel7.img
        ./umount.sh
        cd ..
    fi
fi

