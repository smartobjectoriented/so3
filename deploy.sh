#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname "$script"))

# TODO : mcopy may require the -s flag for recursive copy

cd "$SCRIPTPATH"

usage()
{
    echo "Usage: $0 [OPTIONS] <ME_NAME>"
    echo ""
    echo "Where OPTIONS are:"
    echo "  -a    Deploy all"
    echo "  -b    Deploy boot (kernel, U-boot, etc.)"
    echo "  -r    Deploy usr apps into the rootfs (here as ramfs)"
    echo "  -u    Deploy usr apps in the first partition of the sdcard"
    echo "  -t    Deploy test apps"
    echo ""
    exit 1
}

while getopts "abrut" o; do
    case "$o" in
        a)
            deploy_rootfs=y
            deploy_boot=y
            deploy_usr=y
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
        t)
            deploy_tests=y
            ;;
        *)
            usage
            ;;
    esac
done

if [ $OPTIND -eq 1 ]; then usage; fi

# Extract variables from build.conf file
while read var; do
    if [ "$var" != "" ]; then
        export $(echo $var | sed -e 's/ //g' -e /^$/d -e 's/://g' -e /^#/d)
    fi
done < build.conf

echo "Platform is : ${PLATFORM}"
[ "${PLATFORM}" == "" ] && { echo "Platform variable is not set, exiting..." ; exit 1 ; }

# This will clear the partitions
filesystem/create_partitions.sh

if [ "${deploy_rootfs}" == "y" ]; then
    # Deploy of the rootfs (first partition of ramfs)
    echo "Deploying rootfs..."
    rootfs/create_ramfs.sh ${PLATFORM}
    rootfs/deploy.sh ${PLATFORM}
fi

if [ "${deploy_usr}" == "y" ]; then
    # Deploy all usr applications into the rootfs
    echo "Deploying user apps..."
    mcopy -i filesystem/partition1.img usr/out/* ::
fi

if [ "${deploy_tests}" == "y" ]; then
    echo "Deploying test apps..."
    mcopy -i filesystem/partition1.img usr/tests/out/* ::
    if [ "${deploy_rootfs}" == "y" ]; then
        rootfs/deploy.sh ${PLATFORM} usr/tests/out/
    fi
fi

if [ "${deploy_boot}" == "y" ]; then
    # Deploy files into the boot partition (first partition)
    echo "Deploying boot files into the first partition..."
     
    cd target
    ./mkuboot.sh ${PLATFORM} > /dev/null || { echo "./mkuboot.sh failed, exiting..." ; exit 1 ; }
    cd ..

    cd filesystem
    mcopy -i partition1.img ../target/${PLATFORM}.itb ::
    mcopy -i partition1.img ../u-boot/uEnv.d/uEnv_"$PLATFORM".txt ::uEnv.txt
    # This is a simple way to put the time when deploy.sh was called into the file system
    mcopy -i partition1.img /proc/driver/rtc ::host_time.txt

    if [ "$PLATFORM" == "vexpress" ]; then
        # Nothing else ...
        true
    fi

    if [ "$PLATFORM" == "rpi3" ]; then
        mcopy -i partition1.img ../bsp/rpi3/* ::
        mcopy -i partition1.img ../u-boot/u-boot.bin ::kernel.img
        # TODO itb for rpi3 may be missing from git repo
    fi
    
    if [ "$PLATFORM" == "rpi4" ]; then
        # https://www.raspberrypi.org/documentation/configuration/boot_folder.md
        mcopy -i partition1.img ../bsp/rpi4/* ::
        mcopy -i partition1.img ../u-boot/u-boot.bin ::kernel7.img
    fi

    cd ..
fi

if [ "${deploy_boot}" == "y" ] || [ "${deploy_usr}" == "y" ] || [ "${deploy_tests}" == "y" ]; then
    cd filesystem
    ./populate_sd_image.sh
fi

