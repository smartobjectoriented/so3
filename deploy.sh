#!/bin/bash

usage()
{
  echo "Usage: $0 [OPTIONS]"
  echo ""
  echo "Where OPTIONS are:"
  echo "  -b    Deploy boot (kernel, U-boot, etc.)"
  echo "  -r    Deploy rootfs (secondary) in the second partition"
  echo "  -u    Deploy SO3 usr apps"
  echo ""
  
  exit 1
}

while getopts "bru" o; do
  case "$o" in
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

# We now have ${PLATFORM} which names the platform base
# and ${PLATFORM_TYPE} to be used when the type is required.
# Note that ${PLATFORM_TYPE} can be equal to ${PLATFORM} if no type is specified.

if [ "$PLATFORM" != "virt32" -a "$PLATFORM" != "vexpress" -a "$PLATFORM" != "virt64" ]; then
    echo "Specify the device name of MMC (ex: sdb or mmcblk0 or other...)" 
    read devname
    export devname="$devname"
fi

if [ "$deploy_usr" == "y" ]; then
 
    # Deploy the usr apps into the ramfs
    cd usr
    ./deploy.sh
    sleep 1
    cd ..
    deploy_boot=y
fi

if [ "$deploy_rootfs" == "y" ]; then
    # Deploy of the rootfs (currently first partition)
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
    
# Check if the rootfs has been redeployed (in partition #1 currently). In this case, the contents must be preserved.
    if [ "$deploy_rootfs" != "y" ]; then
    sudo rm -rf fs/*
    fi
    
    [ -f ../target/${PLATFORM}.itb ] && sudo cp ../target/${PLATFORM}.itb fs/ && echo ITB deployed.
    sudo cp ../u-boot/uEnv.d/uEnv_${PLATFORM}.txt fs/uEnv.txt
       
    if [ "$PLATFORM" == "virt32" -o "$PLATFORM" == "vexpress" -o "$PLATFORM" == "virt64" ]; then
	# Nothing else ...
        ./umount.sh
        cd ..
    fi
 
    if [ "$PLATFORM" == "rpi4" ]; then
        sudo cp -r ../bsp/rpi4/* fs/
        sudo cp ../u-boot/u-boot.bin fs/kernel7.img
        ./umount.sh
        cd ..
    fi
    
    if [ "$PLATFORM" == "rpi4_64" ]; then
        sudo cp -r ../bsp/rpi4/* fs/
        sudo cp ../u-boot/u-boot.bin fs/kernel8.img
        ./umount.sh
        cd ..
    fi
fi

    
