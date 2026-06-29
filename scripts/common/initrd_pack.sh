# Copyright (c) 2025-2026 EDGEMTech SA
#
# Helpers for the `-i`/`initrd` option of mount.sh / umount.sh.
#
# A cpio archive is not a block image, so there is nothing to losetup +
# mount. "Mounting" the initrd means extracting board/<plat>/initrd.cpio
# into a working tree the user can edit; "unmounting" repacks that tree
# back into initrd.cpio. Both run under fakeroot so the archive keeps its
# root ownership / modes without needing real root — the fake-ownership DB
# is saved on mount and replayed on umount.
#
# Expects env.sh already sourced (IB_ROOT_DIR, BUILDDIR exported).

# Resolve the target platform from the active (uncommented) IB_PLATFORM in
# local.conf — same source of truth as the build. An explicit arg still
# overrides it; virt64 is the last-resort fallback.
_initrd_platform() {
	if [ -n "$1" ]; then echo "$1"; return; fi
	_p=$(sed -n 's/^[[:space:]]*IB_PLATFORM[[:space:]]*?\{0,1\}=[[:space:]]*"\([^"]*\)".*/\1/p' \
		"$BUILDDIR/conf/local.conf" 2>/dev/null | tail -1)
	[ -n "$_p" ] && echo "$_p" || echo virt64
}

# Set _IRD_SRC / _IRD_WORK / _IRD_LINK / _IRD_STATE for the resolved
# platform. Mirrors the partition-mount convention (fs_arm_common.bbclass):
# content extracted under filesystem/work/p1, with a top-level convenience
# symlink filesystem/p1 -> work/p1 that lands straight on the content.
_initrd_paths() {
	_plat=$(_initrd_platform "$1")
	_IRD_SRC="$IB_ROOT_DIR/build/meta-rootfs/recipes-rootfs/linux/files/board/$_plat/initrd.cpio"
	_IRD_WORK="$IB_ROOT_DIR/filesystem/work/p1"
	_IRD_LINK="$IB_ROOT_DIR/filesystem/p1"
	_IRD_STATE="$IB_ROOT_DIR/filesystem/work/p1.fakeroot"
}

initrd_mount() {
	command -v fakeroot >/dev/null 2>&1 || { echo "fakeroot not installed" >&2; return 1; }
	_initrd_paths "$1"
	[ -f "$_IRD_SRC" ] || { echo "no initrd.cpio for '$_plat': $_IRD_SRC" >&2; return 1; }
	if mountpoint -q "$_IRD_WORK" 2>/dev/null; then
		echo "a partition is mounted at $_IRD_WORK — umount.sh first" >&2
		return 1
	fi
	if [ -d "$_IRD_WORK" ]; then
		echo "already mounted at $_IRD_WORK — umount.sh -i first" >&2
		return 1
	fi
	mkdir -p "$_IRD_WORK"
	rm -f "$_IRD_STATE"
	( cd "$_IRD_WORK" && fakeroot -s "$_IRD_STATE" sh -c "cpio -idm --quiet < '$_IRD_SRC'" ) || return 1
	ln -sfn "$_IRD_WORK" "$_IRD_LINK"
	echo "initrd.cpio ($_plat) extracted; reach it directly via:"
	echo "    $_IRD_LINK"
	echo "edit it, then repack with:  umount.sh -i"
}

initrd_umount() {
	command -v fakeroot >/dev/null 2>&1 || { echo "fakeroot not installed" >&2; return 1; }
	_initrd_paths "$1"
	[ -d "$_IRD_WORK" ] || { echo "not mounted: $_IRD_WORK" >&2; return 1; }
	if [ -f "$_IRD_STATE" ]; then
		( cd "$_IRD_WORK" && fakeroot -i "$_IRD_STATE" -s "$_IRD_STATE" \
			sh -c "find . | sort | cpio -o -H newc --quiet > '$_IRD_SRC.tmp'" ) || return 1
	else
		echo "warning: no fakeroot state, ownership may be wrong" >&2
		( cd "$_IRD_WORK" && fakeroot \
			sh -c "find . | sort | cpio -o -H newc --quiet > '$_IRD_SRC.tmp'" ) || return 1
	fi
	mv "$_IRD_SRC.tmp" "$_IRD_SRC"
	rm -rf "$_IRD_WORK" "$_IRD_STATE"
	[ -L "$_IRD_LINK" ] && rm -f "$_IRD_LINK"
	echo "repacked $_plat initrd.cpio ($(wc -c < "$_IRD_SRC") bytes); unmounted"
}
