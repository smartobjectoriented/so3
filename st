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

if [ "$PLATFORM" == "virt-riscv64" ]; then
    sudo qemu/build/qemu-system-riscv64 $@ \
        -gdb tcp::$QEMU_GDB_PORT \
        -serial mon:stdio  \
        -smp 1 \
        -m 4096 \
        -M virt \
        -bios none \
        -kernel u-boot/u-boot \
        -nographic \
        -device virtio-blk-device,drive=hd0 \
        -drive file=filesystem/sdcard.img.virt-riscv64,format=raw,id=hd0

else
    sudo qemu/arm-softmmu/qemu-system-arm $@ \
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
