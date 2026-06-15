
SUMMARY = "User space applications for SO3"
DESCRIPTION = "All (SO3) user space custom applications which take place in the rootfs of SO3"
LICENSE = "GPLv2"

inherit usr
  
# Release and version
PR = "r0"
PV = "1.0"

# Use :append (not +=) so no space is inserted before ":so3". With +=,
# OVERRIDES becomes "...:arm :so3" and the CPU token parses as "arm "
# (trailing space), so :arm overrides like IB_MUSL_TARGET:arm never
# collapse — breaking the musl toolchain PATH on virt32.
OVERRIDES:append = ":so3"

# These patches bring lv_port_linux/lvgl in the usr structure
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_DIR}/so3/usr"

IB_TOOLCHAIN_PATH = "${IB_TARGET}/${IB_PLAT_CPU}-linux-musl.cmake"

# lvgl is FETCHED at build time (see the meta-usr/recipes-usr/lvgl
# usr-so3 bbappend, gated on the :lvgl override) into usr/lib/lvgl — it is
# NOT a committed git submodule. do_fetch/do_unpack/do_attach therefore run
# so the bbappend's do_handle_fetch_git can pull lvgl into the workdir and
# attach it back into usr/.
#
# do_patch stays noexec: the slv/lvgl integration patches are already baked
# into the in-tree usr/ source, so re-applying them fails (reversed patch).
do_patch[noexec] = "1"

do_build[depends] = "rootfs-so3:do_build"

# The user space is cross-compiled with the SO3 musl toolchain built by
# the musl-toolchain recipe (meta-toolchain). Make it available and put
# its bin/ on PATH so the bare compiler names in the cmake toolchain
# file (e.g. aarch64-linux-musl-gcc) resolve.
do_build[depends] += "musl-toolchain:do_build"
do_build:prepend () {
	export PATH="${IB_MUSL_TOOLCHAIN_DIR}/${IB_MUSL_TARGET}/bin:$PATH"

	# The cmake build dir caches the toolchain (CMakeCache.txt pins the
	# compiler), so switching IB_PLATFORM between virt64 and virt32
	# (aarch64<->arm) would otherwise keep producing wrong-arch user
	# binaries (e.g. an aarch64 init.elf on a 32-bit kernel -> prefetch
	# abort at boot). Wipe build/ when the arch changes; same-arch
	# rebuilds stay incremental. The marker lives at the usr/ root so it
	# survives the build/ wipe.
	_arch_marker="${IB_TARGET}/.ib_last_arch"
	if [ -f "$_arch_marker" ] && [ "$(cat $_arch_marker)" != "${IB_PLAT_CPU}" ]; then
		echo "SO3 usr arch changed ($(cat $_arch_marker) -> ${IB_PLAT_CPU}); wiping build/"
		rm -rf ${IB_TARGET}/build
	fi
	echo "${IB_PLAT_CPU}" > "$_arch_marker"
}

# Make sure so3 has been installed correctly to fetch other components if required
do_unpack[depends] += "so3:do_attach_infrabase"

do_deploy[nostamp] = "1"

# Deploy the usr contents, i.e. the deploy/ dir, in the SO3 rootfs
python do_deploy() {
    import subprocess
    import os
    
    d.setVar('ROOTFS_FILENAME', '')

    if os.path.isdir(d.getVar('IB_ROOTFS_PATH')):
        __do_rootfs_mount(d)
        
        src_dir = os.path.join(d.getVar('IB_TARGET'), 'build', 'deploy')
        dst_dir = os.path.join(d.getVar('IB_ROOTFS_PATH'), 'fs')

        # The SO3 rootfs.fat is loop-mounted at rootfs/fs as root
        # (rootfs/mount.sh), so the copy must be privileged. The split
        # debug-info files (*.debug) are host-side gdb symbols, not
        # runtime artifacts — keep them out of the (small) rootfs.
        # -r only (no -a): vfat rejects chown/owner/perms preservation.
        cmd = f"sudo rsync -rL --exclude='*.debug' {src_dir}/ {dst_dir}/"

        result = subprocess.run(cmd, shell=True, check=True)
        
        __do_rootfs_umount(d)
    else:
        utils_restore_user_ownership(d)
        bb.fatal("Hum, it seeems the so3 usr has not been built correctly - rootfs missing...")
    
}
 
addtask do_deploy
do_deploy[nostamp] = "1"

# Installation of the user space components

do_install_apps () {

        # All ELF applications available in usr

        usr_do_install_file_dir "${IB_TARGET}/build/src/*.elf" .
        usr_do_install_file_dir "${IB_TARGET}/out/*" .
}

# Python (not shell) to match the Python do_clean in usr.bbclass and to
# avoid the shell-task temp/fifo failure. Removes this recipe's stamps.
python do_clean:append() {
    import glob, os
    for f in glob.glob(d.getVar('TMPDIR') + '/stamps/usr-so3*'):
        try:
            os.remove(f)
        except OSError:
            pass
}

