# Launch script for SO3 

N_QEMU_INSTANCES=`ps -A | grep qemu | wc -l`
GDB_PORT_BASE=1234


QEMU_MAC_ADDR=`printf 'DE:AD:BE:EF:%02X:%02X\n' $(($RANDOM % 255)) $((N_QEMU_INSTANCES))`
QEMU_GDB_PORT=$(($GDB_PORT_BASE + $N_QEMU_INSTANCES))


echo -e 'MAC:\t\t' $QEMU_MAC_ADDR
echo -e 'GDB Port:\t' $QEMU_GDB_PORT '\n\n'

while read var; do
if [ "$var" != "" ]; then
  export $(echo $var | sed -e 's/ //g' -e /^$/d -e 's/://g' -e /^#/d)
fi
done < build.conf

if [ "$PLATFORM" == "virt64" ]; then

sudo qemu/build/qemu-system-aarch64 $@ \
  	-smp 4 -s \
	-serial mon:stdio  \
	-M virt,gic-version=2 -cpu cortex-a72  \
	-device virtio-blk-device,drive=hd0 \
	-drive if=none,file=filesystem/sdcard.img.virt64,id=hd0,format=raw,file.locking=off \
	-m 1024 \
	-kernel u-boot/u-boot \
	-nographic \
	-netdev tap,id=n1,script=scripts/qemu-ifup.sh,downscript=scripts/qemu-ifdown.sh \
	-device virtio-net-device,netdev=n1,mac=00:01:29:53:97:BA

else

sudo qemu/build/arm-softmmu/qemu-system-arm $@ \
        -gdb tcp::$QEMU_GDB_PORT \
  	-smp 4 \
	-serial mon:stdio  \
	-M vexpress-a15,secure=on  -cpu cortex-a15\
	-m 1024 \
	-kernel u-boot/u-boot \
	-semihosting-config enable,target=native \
	-net user,tftp=. \
	-nic tap,script=scripts/qemu-ifup.sh,downscript=scripts/qemu-ifdown.sh,mac=${QEMU_MAC_ADDR} \
	-sd filesystem/sdcard.img.vexpress \
	-nographic 
fi
