#!/bin/bash
script=${BASH_SOURCE[0]}
# Get the path of this script
SCRIPTPATH=$(realpath $(dirname $script))

cd $SCRIPTPATH

usage()
{
    echo "Usage: $0 [OPTIONS] <ME_NAME>"
    echo ""
    echo "Where OPTIONS are:"
    echo "  -a    Deploy all"
    echo "  -b    Deploy boot (kernel, U-boot, etc.)"
    echo "  -r    Deploy rootfs (secondary)"
    echo "  -u    Deploy usr apps"
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

while read var; do
    if [ "$var" != "" ]; then
        export $(echo $var | sed -e 's/ //g' -e /^$/d -e 's/://g' -e /^#/d)
    fi
done < build.conf

echo "Platform is : ${_PLATFORM}"

#if [ "$_PLATFORM" != "vexpress" ]; then
#    echo "Specify the device name of MMC (ex: sdb or mmcblk0 or other...)" 
#    read devname
#    export devname="$devname"
#fi

if [ "${deploy_boot}" == "y" ]; then
    # Deploy files into the boot partition (first partition)
    echo "Deploying boot files into the first partition..."
     
    cd target
    ./mkuboot.sh ${_PLATFORM} > /dev/null
    cd ../filesystem
    # This will clear the partitions
    ./create_partitions.sh

    mcopy -i partition1.img ../target/${_PLATFORM}.itb ::
    mcopy -i partition1.img ../u-boot/uEnv.d/uEnv_"$_PLATFORM".txt ::uEnv.txt
    # This is a simple way to put the time when deploy.sh was called into the file system
    mcopy -i partition1.img /proc/driver/rtc ::host_time.txt

    if [ "$_PLATFORM" == "vexpress" ]; then
        # Nothing else ...
        true
    fi

    if [ "$_PLATFORM" == "rpi3" ]; then
        mcopy -i partition1.img ../bsp/rpi3/* ::
        mcopy -i partition1.img ../u-boot/u-boot.bin ::kernel.img
        #TODO
    #    sudo cp -r ~/sootech/rpi-bsp/boot/* fs/
    fi
    
    if [ "$_PLATFORM" == "rpi4" ]; then
        mcopy -i partition1.img ../bsp/rpi4/* ::
        mcopy -i partition1.img ../u-boot/u-boot.bin ::kernel7.img
    fi

    cd ..
fi

# TODO : Find out differences between these two

if [ "${deploy_rootfs}" == "y" ]; then
    # Deploy of the rootfs (first partition)
    echo "Deploy rootfs ..."
    mcopy -i filesystem/partition1.img usr/out/* ::
fi
    
if [ "${deploy_usr}" == "y" ]; then
    # Deploy all usr applications into the rootfs
    echo "Deploy user apps ..."
    mcopy -i filesystem/partition1.img usr/out/* ::
fi

if [ "${deploy_tests}" == "y" ]; then
    echo "Deploy test apps ..."
    mcopy -i filesystem/partition1.img usr/tests/out/* ::
fi

cd filesystem
./populate_sd_image.sh
