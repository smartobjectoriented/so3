
SUMMARY = "User space applications for SO3"
DESCRIPTION = "All (SO3) user space custom applications which take place in the rootfs of SO3"
LICENSE = "GPLv2"

inherit usr
  
# Release and version
PR = "r0"
PV = "1.0"

OVERRIDES += ":so3"

# These patches bring lv_port_linux/lvgl in the usr structure
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_DIR}/so3/usr"

IB_TOOLCHAIN_PATH = "${IB_TARGET}/${IB_PLAT_CPU}-linux-musl.cmake"

# This repo IS the SO3 source tree: the user space under usr/ is already
# in its final form (the slv/lvgl integration patches are committed, and
# lvgl is checked out as a git submodule at usr/lib/lvgl). It is built in
# place — no fetch/unpack/patch/attach, which would either re-apply the
# already-applied patches (reversed-patch failure) or clobber usr/.
do_fetch[noexec] = "1"
do_unpack[noexec] = "1"
do_patch[noexec] = "1"
do_attach_infrabase[noexec] = "1"

do_build[depends] = "rootfs-so3:do_build"

# The user space is cross-compiled with the SO3 musl toolchain built by
# the musl-toolchain recipe (meta-toolchain). Make it available and put
# its bin/ on PATH so the bare compiler names in the cmake toolchain
# file (e.g. aarch64-linux-musl-gcc) resolve.
do_build[depends] += "musl-toolchain:do_build"
do_build:prepend () {
	export PATH="${IB_MUSL_TOOLCHAIN_DIR}/${IB_MUSL_TARGET}/bin:$PATH"
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

do_clean:append () {
    rm -f ${TMPDIR}/stamps/usr-so3*
}

