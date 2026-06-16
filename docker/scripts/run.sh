#!/bin/sh
#
# Launch the LVGL performance run under the patched QEMU.
#
# Copyright (c) 2025-2026 REDS Institute, HEIG-VD
#
# Assumes the Infrabase build + deploy already ran (see the image CMD):
#   - the patched QEMU is at qemu/build/qemu-system-${QEMU_ARCH}
#   - the bootable SD-card image is at filesystem/sdcard.img.${PLATFORM}
#   - U-Boot is at u-boot/u-boot
#
# The virt${XX}_lvperf kernel runs the LVGL benchmark as its init program; when
# it finishes, the kernel performs a semihosting exit and QEMU halts, so this
# script returns and the container exits with the perf output on stdout.
#
# Determinism for reproducible FPS numbers comes from -icount + -semihosting.

set -e

case "$QEMU_ARCH" in
    aarch64) CPU="cortex-a72"; PLATFORM="${PLATFORM:-virt64}" ;;
    arm)     CPU="cortex-a15"; PLATFORM="${PLATFORM:-virt32}" ;;
    *)
        echo "Error: unsupported QEMU_ARCH='$QEMU_ARCH' (expected arm or aarch64)" >&2
        exit 1
        ;;
esac

FILESYSTEM_PATH="filesystem/sdcard.img.${PLATFORM}"
QEMU_BIN="qemu/build/qemu-system-${QEMU_ARCH}"

if [ ! -f "$FILESYSTEM_PATH" ]; then
    echo "Error: SD-card image '$FILESYSTEM_PATH' not found." >&2
    echo "       Build & deploy it first: build.sh -a bsp-so3 && deploy.sh -a bsp-so3" >&2
    exit 1
fi

# Optional GDB stub when SO3_USR_DEBUG is set.
QEMU_DEBUG_ARGS=""
if [ -n "$SO3_USR_DEBUG" ]; then
    QEMU_DEBUG_ARGS="-S -gdb tcp::1234"
fi

exec "$QEMU_BIN" $QEMU_DEBUG_ARGS \
    -semihosting \
    -smp 2 \
    -icount shift=0,sleep=on,align=on \
    -serial mon:stdio \
    -M virt -cpu "$CPU" \
    -device virtio-blk-device,drive=hd0 \
    -drive if=none,file="$FILESYSTEM_PATH",id=hd0,format=raw,file.locking=off \
    -m 1024 \
    -kernel u-boot/u-boot \
    -nographic
