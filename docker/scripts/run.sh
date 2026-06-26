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
# align=off (the default): the virtual icount clock already provides determinism;
# align=on would only pace the guest to host wall-clock and spam "the guest is
# now late by N seconds" warnings whenever it can't keep up.

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

# This launcher boots a STANDALONE SO3 image at EL1 (plain -M virt). It cannot
# boot an AVZ (EL2 hypervisor) or an ATF/flash0 (EL3) deployment — those need
# QEMU started with virtualization=on / secure=on, which scripts/st.sh and
# scripts/stg.sh select from the deployed ITS. The lv_perf images deploy a
# standalone ITS, so this guard only ever trips when run by hand on a host tree
# configured for AVZ/ATF.
SO3_ITS=$(grep -E "^IB_TARGET_ITS:so3:${PLATFORM}\b" build/conf/local.conf 2>/dev/null \
              | awk -F'"' '{print $2}' | tail -1)
if [ -f filesystem/flash0.img ]; then
    why="ATF/flash0.img chain (EL3)"
elif [ "${SO3_ITS#*avz}" != "$SO3_ITS" ]; then
    why="AVZ ITS '${SO3_ITS}' (EL2)"
fi
if [ -n "$why" ]; then
    echo "Error: the deployment is not standalone — $why." >&2
    echo "       run.sh boots SO3 standalone at EL1 only. For an AVZ (EL2) or ATF" >&2
    echo "       (EL3) build, use the host launchers instead, which select the EL" >&2
    echo "       from the ITS:  scripts/st.sh  (headless)  /  scripts/stg.sh  (GTK)." >&2
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
    -icount shift=0,sleep=on,align=off \
    -serial mon:stdio \
    -M virt -cpu "$CPU" \
    -device virtio-blk-device,drive=hd0 \
    -drive if=none,file="$FILESYSTEM_PATH",id=hd0,format=raw,file.locking=off \
    -m 1024 \
    -kernel u-boot/u-boot \
    -nographic
