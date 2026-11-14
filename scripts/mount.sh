#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA
#
# Trigger filesystem:fs_mount. bitbake itself runs unprivileged — the
# recipe internally invokes mount/losetup via `sudo -n` against the
# timestamp opened here.

# Release banner, once per invocation (see scripts/common/banner.sh).
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/banner.sh"

progname=$(basename "$0")

pr_usage()
{
	printf "Open the target storage, or a cpio archive, for editing\n\n"
	printf "Usage: %s [-h] [-i [platform]] [ramfs|rootfs [recipe]] [bitbake option]\n\n" "$progname"
	printf "    (no option)        Mount the two partitions of the storage selected by\n"
	printf "                       IB_PLATFORM / IB_STORAGE_MODE in build/conf/local.conf:\n"
	printf "                         soft   the loopback image filesystem/sdcard.img.<plat>\n"
	printf "                         hard   the real device named by IB_STORAGE_DEVICE\n"
	printf "                       They show up as filesystem/p1 (boot, FAT) and\n"
	printf "                       filesystem/p2 (rootfs, ext4).\n"
	printf "    -i [platform]      Leave the storage alone: extract board/<plat>/initrd.cpio\n"
	printf "                       into filesystem/p1 so it can be edited. Under fakeroot —\n"
	printf "                       no sudo, no bitbake. Defaults to the active IB_PLATFORM.\n"
	printf "                       Repack it with 'umount.sh -i'.\n"
	printf "    ramfs|rootfs [r]   Unpack a rootfs recipe's board/<plat>/{initrd,rootfs}.cpio\n"
	printf "                       into that recipe's WORKDIR, with <IB_ROOTFS_PATH>/fs\n"
	printf "                       pointing at it. Goes through bitbake and needs root\n"
	printf "                       ('cpio -id' restores device nodes). The recipe defaults\n"
	printf "                       to rootfs-linux; pass rootfs-so3 or buildroot as [r].\n"
	printf "                       Repack with the matching 'umount.sh'.\n"
	printf "    -h                 Print this help\n\n"
	printf "For a plain initrd edit prefer -i: same archive, without sudo or bitbake.\n"
	printf "'rootfs' has no such alternative.\n\n"
	printf "Any other argument is passed on to bitbake (e.g. -v).\n\n"
	printf "Unmount before redeploying: deploy.sh reuses an existing mount instead of\n"
	printf "making a fresh one.\n\n"
	printf "In the build container a mount lives only as long as the container that made\n"
	printf "it: 'dbuild.sh %s' mounts inside its own mount namespace, and the mount is\n" "$progname"
	printf "gone once the command returns. Run the mount and whatever uses it in the SAME\n"
	printf "container — typically from an interactive 'dbuild.sh' shell.\n"
}

# Handled before setup_env.sh: printing the help should not trigger the
# tree-switch prompt that sourcing the environment can raise.
case "$1" in
	-h|--help)
		pr_usage
		exit 0
		;;
esac

# Resolve project root from this script's own location, cd there, and
# source env.sh — prompting the user first if the parent shell points
# at a different tree. See scripts/common/setup_env.sh for details.
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"

# `mount.sh -i [platform]` extracts board/<plat>/initrd.cpio for editing
# instead of loop-mounting the sdcard partitions (no sudo / bitbake needed).
if [ "$1" = "-i" ] || [ "$1" = "initrd" ]; then
	shift
	. ./scripts/common/initrd_pack.sh
	initrd_mount "$1"
	exit $?
fi

# `mount.sh ramfs|rootfs [recipe]` unpacks a rootfs recipe's cpio
# (board/<plat>/{initrd,rootfs}.cpio) into that recipe's WORKDIR and
# symlinks <IB_ROOTFS_PATH>/fs at it, so the tree can be edited in
# place. Repack with the matching `umount.sh ramfs|rootfs`.
#
# The recipe defaults to rootfs-linux; pass another one (rootfs-so3,
# buildroot) as the second argument.
#
# This is the bitbake path (do_{ramfs,rootfs}_mount) and needs root —
# `cpio -id` restores device nodes — hence the sudo session below.
# NB `-i` above reaches the same initrd.cpio through fakeroot, without
# sudo or bitbake; prefer it for a plain initrd edit. `rootfs` has no
# such alternative.
case "$1" in
	ramfs|rootfs)
		_ib_mode=$1
		_ib_recipe=${2:-rootfs-linux}

		. ./scripts/common/sudo_session.sh

		sudo_session_start || exit 1

		cd "$BUILDDIR"
		exec ./bitbake/bin/bitbake "$_ib_recipe" -c "${_ib_mode}_mount"
		;;
esac

. ./scripts/common/sudo_session.sh

sudo_session_start || exit 1

cd "$BUILDDIR"
./bitbake/bin/bitbake filesystem -c fs_mount $1
