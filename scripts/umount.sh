#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA
#
# Trigger filesystem:fs_umount. bitbake itself runs unprivileged — the
# recipe internally invokes umount/losetup via `sudo -n` against the
# timestamp opened here.

# Release banner, once per invocation (see scripts/common/banner.sh).
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/banner.sh"

progname=$(basename "$0")

pr_usage()
{
	printf "Close what 'mount.sh' opened: the storage, or a cpio archive\n\n"
	printf "Usage: %s [-h] [-i [platform]] [ramfs|rootfs [recipe]] [bitbake option]\n\n" "$progname"
	printf "    (no option)        Unmount filesystem/p1 and filesystem/p2 — the partitions\n"
	printf "                       mounted by 'mount.sh' from the storage selected by\n"
	printf "                       IB_PLATFORM / IB_STORAGE_MODE (loopback image in soft\n"
	printf "                       mode, real device in hard mode).\n"
	printf "    -i [platform]      Repack the tree extracted by 'mount.sh -i' back into\n"
	printf "                       board/<plat>/initrd.cpio, preserving root ownership via\n"
	printf "                       fakeroot. Defaults to the active IB_PLATFORM.\n"
	printf "    ramfs|rootfs [r]   Repack the tree unpacked by the matching 'mount.sh' back\n"
	printf "                       into board/<plat>/{initrd,rootfs}.cpio, after copying the\n"
	printf "                       previous archive to <name>.cpio.backup. The repack is\n"
	printf "                       content-deterministic, so an unmodified round trip leaves\n"
	printf "                       the archive byte-identical. Same recipe default\n"
	printf "                       (rootfs-linux) and same need for root as the mount side.\n"
	printf "    -h                 Print this help\n\n"
	printf "Any other argument is passed on to bitbake (e.g. -v).\n\n"
	printf "Always unmount before redeploying, and before unplugging a card: deploy.sh\n"
	printf "reuses an existing mount, and an unmounted card may still hold unwritten data.\n\n"
	printf "In the build container a mount lives only as long as the container that made\n"
	printf "it, so a mount made in one 'dbuild.sh' invocation is already gone by the next\n"
	printf "one — there is then nothing for this script to unmount.\n"
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

# `umount.sh -i [platform]` repacks the tree extracted by `mount.sh -i`
# back into board/<plat>/initrd.cpio (no sudo / bitbake needed).
if [ "$1" = "-i" ] || [ "$1" = "initrd" ]; then
	shift
	. ./scripts/common/initrd_pack.sh
	initrd_umount "$1"
	exit $?
fi

# `umount.sh ramfs|rootfs [recipe]` repacks the tree unpacked by the
# matching `mount.sh` back into board/<plat>/{initrd,rootfs}.cpio, after
# copying the previous archive to <name>.cpio.backup. The repack is
# content-deterministic (sorted, --reproducible, mtimes zeroed), so an
# unmodified round trip leaves the archive byte-identical. The extracted
# tree and the <IB_ROOTFS_PATH>/fs symlink are left in place — the next
# mount replaces them. Same recipe default and same privilege
# requirement as the mount side.

case "$1" in
	ramfs|rootfs)
		_ib_mode=$1
		_ib_recipe=${2:-rootfs-linux}

		. ./scripts/common/sudo_session.sh

		sudo_session_start || exit 1

		cd "$BUILDDIR"
		exec ./bitbake/bin/bitbake "$_ib_recipe" -c "${_ib_mode}_umount"
		;;
esac

. ./scripts/common/sudo_session.sh

sudo_session_start || exit 1

cd "$BUILDDIR"
./bitbake/bin/bitbake filesystem -c fs_umount $1
