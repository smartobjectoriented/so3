#!/bin/sh

# Copyright (c) 2025-2026 EDGEMTech SA
#
# Trigger filesystem:fs_umount. bitbake itself runs unprivileged — the
# recipe internally invokes umount/losetup via `sudo -n` against the
# timestamp opened here.

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

. ./scripts/common/sudo_session.sh

sudo_session_start || exit 1

cd "$BUILDDIR"
./bitbake/bin/bitbake filesystem -c fs_umount $1
