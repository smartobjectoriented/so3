#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA
#
# Trigger filesystem:fs_mount. bitbake itself runs unprivileged — the
# recipe internally invokes mount/losetup via `sudo -n` against the
# timestamp opened here.

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

. ./scripts/common/sudo_session.sh

sudo_session_start || exit 1

cd "$BUILDDIR"
./bitbake/bin/bitbake filesystem -c fs_mount $1
