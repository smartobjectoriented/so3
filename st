#!/bin/sh
 sudo qemu/arm-softmmu/qemu-system-arm $@ \
    -s \
  	-smp 4 \
	-serial mon:stdio  \
	-M vexpress-a15,secure=on  -cpu cortex-a15\
	-m 1024 \
	-kernel u-boot/u-boot \
	-semihosting-config enable,target=native \
	-net user,tftp=. \
	-net tap,script=scripts/qemu-ifup.sh,downscript=scripts/qemu-ifdown.sh -net nic \
	-sd filesystem/sdcard.img \
	-nographic 
    
 
