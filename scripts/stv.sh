#!/bin/bash

QEMU_AUDIO_DRV="none"
GDB_PORT_BASE=1234
USR_OPTION=$1

N_QEMU_INSTANCES=`ps -A | grep qemu-system-arm | wc -l`

launch_qemu() {
    QEMU_MAC_ADDR="$(printf 'DE:AD:BE:EF:%02X:%02X\n' $((N_QEMU_INSTANCES)) $((N_QEMU_INSTANCES)))"

    GDB_PORT=$((${GDB_PORT_BASE} + ${N_QEMU_INSTANCES}))

    echo -e "\033[01;36mMAC addr: " ${QEMU_MAC_ADDR} "\033[0;37m"
    echo -e "\033[01;36mGDB port: " ${GDB_PORT} "\033[0;37m"

    while IFS= read -r line; do
      # Check if the line starts with "IB_PLATFORM"
      if [[ $line == IB_PLATFORM* ]]; then
    	  # Extract the value between the quotes
    	  value=$(echo "$line" | awk -F'"' '{print $2}')
    
    	  # Set the IB_PLATFORM variable to the extracted value
    	  IB_PLATFORM="$value"
    	  break
      fi     
    done < build/conf/local.conf

    if [ "$IB_PLATFORM" == "virt64" ]; then
    echo Starting on virt64
    sudo qemu-system-aarch64 $@ ${USR_OPTION} \
		-smp 4  \
		-serial mon:stdio  \
		-M virt,virtualization=on,gic-version=2 -cpu cortex-a72  \
		-device virtio-blk-device,drive=hd0 \
		-drive if=none,file=filesystem/sdcard.img.virt64,id=hd0,format=raw,file.locking=off \
		-m 1024 \
		-kernel u-boot/u-boot \
		-nographic \
		-netdev tap,id=n1,script=scripts/qemu-ifup.sh,downscript=scripts/qemu-ifdown.sh \
		-device virtio-net-device,netdev=n1,mac=${QEMU_MAC_ADDR} \
        	-gdb tcp::${GDB_PORT}
	fi


    QEMU_RESULT=$?
}

launch_qemu
