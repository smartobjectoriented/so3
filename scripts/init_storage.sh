#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA
#
# Trigger filesystem:fs_init_storage. bitbake itself runs unprivileged
# — the recipe internally invokes losetup/fdisk/parted/mkfs via
# `sudo -n` against the timestamp opened here.

# Release banner, once per invocation (see scripts/common/banner.sh).
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/banner.sh"

progname=$(basename "$0")

pr_usage()
{
	printf "Infrabase storage initialisation\n\n"
	printf "Usage: %s [-h] [-l <layout>] [-f]\n" "$progname"
}

pr_help()
{
	printf "\nPartition and format the storage for this platform.\n"
	printf "\nAvailable options:\n"
	printf "    -h              Print this help\n"
	printf "    -l <layout>     Partition layout, overriding the one this\n"
	printf "                    configuration would derive:\n"
	printf "\n"
	printf "                      rootfs   p1 boot (FAT) + p2 rootfs (ext4).\n"
	printf "                               What every Linux capsule and every\n"
	printf "                               bare BSP expects.\n"
	printf "                      ab       p1 boot (FAT) + p2 and p3, two raw\n"
	printf "                               slots. What a bootloader picking an\n"
	printf "                               image out of a slot expects.\n"
	printf "\n"
	printf "                    Without -l the layout is derived from the boot\n"
	printf "                    chain, which means it follows whatever the last\n"
	printf "                    build.sh left in build/conf/bblayers.conf — the\n"
	printf "                    reason this flag exists.\n"
	printf "    -f              Re-partition even if an image already exists.\n"
	printf "                    An image is never converted in place, so this\n"
	printf "                    deletes it (and the symlink beside it) first.\n"
	printf "\nExamples:\n\n"
	printf "%s                  Layout derived from the current configuration\n" "$progname"
	printf "%s -l ab            Two raw slots, whatever the configuration says\n" "$progname"
	printf "%s -f -l rootfs     Throw away the current image and redo it\n" "$progname"
}

layout=''
force=0

while getopts "hl:f" o; do
	case "$o" in
		h)  pr_usage; pr_help; exit ;;
		l)  layout="$OPTARG" ;;
		f)  force=1 ;;
		*)  pr_usage; exit 1 ;;
	esac
done
shift $((OPTIND - 1))

# Checked here as well as in the recipe: a typo should cost a second, not
# a sudo prompt followed by a bitbake parse.
case "$layout" in
	''|rootfs|ab) ;;
	*)
		printf 'Error: unknown layout "%s" (expected "rootfs" or "ab")\n\n' \
			"$layout" >&2
		pr_usage
		exit 1
		;;
esac

# Resolve project root from this script's own location, cd there, and
# source env.sh — prompting the user first if the parent shell points
# at a different tree. See scripts/common/setup_env.sh for details.
. "$(cd "$(dirname "$(command -v -- "$0")")" && pwd)/common/setup_env.sh"
. ./scripts/common/sudo_session.sh

_plat=$(grep -E "^IB_PLATFORM[[:space:]]*[?:]?=" \
	"$BUILDDIR/conf/local.conf" 2>/dev/null \
	| grep -v "^IB_PLATFORM:" | tail -1 | sed -n 's/.*"\([^"]*\)".*/\1/p')
_img="$IB_ROOT_DIR/filesystem/work/sdcard.img.$_plat"
_link="$IB_ROOT_DIR/filesystem/sdcard.img.$_plat"

# An image is never re-partitioned: fs_init_storage only runs when there
# is none, so without -f a second invocation silently does nothing and
# the caller is left believing the layout changed.
if test -f "$_img"; then
	if test "$force" = "1"; then
		printf '[infrabase] removing the existing %s image\n' "$_plat"
		rm -f "$_img" "$_link"
	else
		printf 'Error: %s already exists.\n' "$_img" >&2
		printf '       An image is never re-partitioned in place. Use -f to\n' >&2
		printf '       delete it and start again — everything on it is lost.\n' >&2
		exit 1
	fi
fi

sudo_session_start || exit 1

# The layout reaches the recipe through the environment, which env.sh
# names in BB_ENV_PASSTHROUGH_ADDITIONS. It wins over the ?= default in
# fs_arm_common.bbclass, which is what makes this an override rather than
# a suggestion.
if test -n "$layout"; then
	printf '[infrabase] partition layout: %s (-l)\n' "$layout"
	IB_PARTITION_LAYOUT="$layout"
	export IB_PARTITION_LAYOUT
fi

cd "$BUILDDIR"
./bitbake/bin/bitbake filesystem -c fs_init_storage
