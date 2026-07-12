#!/bin/sh
# Initramfs that pivots to the real root filesystem on disk.
#
# The agency Linux guest boots this small busybox ramfs from the ITB, then
# switch_root's into the full rootfs deployed by rootfs-linux:do_deploy on
# the SD card (p2, ext4 = /dev/mmcblk0p2). Selected with
# IB_RAMFS_SOURCE = "initrd" in conf/local.conf.

/bin/mount -t proc     proc     /proc
/bin/mount -t sysfs    sysfs    /sys
/bin/mount -t devtmpfs devtmpfs /dev

exec 0</dev/console
exec 1>/dev/console
exec 2>/dev/console

# The kernel always boots the ramfs (root=/dev/ram); mounting the real root
# is this script's job. p2 = the ext4 rootfs on the SD card.
ROOT_DEV=/dev/mmcblk0p2

# Wait for the SD-card rootfs partition to appear (~5 s max).
tries=0
while [ ! -b "$ROOT_DEV" ] && [ "$tries" -lt 50 ]; do
	/bin/sleep 0.1
	tries=$((tries + 1))
done

if [ ! -b "$ROOT_DEV" ]; then
	echo "init: rootfs device $ROOT_DEV not found — emergency shell"
	exec /bin/sh
fi

/bin/mkdir -p /mnt/root
if ! /bin/mount -t ext4 -o rw "$ROOT_DEV" /mnt/root; then
	echo "init: cannot mount $ROOT_DEV — emergency shell"
	exec /bin/sh
fi

# Carry the pseudo-filesystems into the new root (the kernel does not
# auto-mount devtmpfs once an initramfs /init has run).
/bin/mount --move /dev  /mnt/root/dev  2>/dev/null
/bin/mount --move /proc /mnt/root/proc 2>/dev/null
/bin/mount --move /sys  /mnt/root/sys  2>/dev/null

echo "init: switching root to $ROOT_DEV"
exec /sbin/switch_root /mnt/root /sbin/init
