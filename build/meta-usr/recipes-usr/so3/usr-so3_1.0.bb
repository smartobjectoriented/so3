
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

do_build[depends] = "rootfs-so3:do_build"

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
        
        cmd = f"cp -r {src_dir}/. {dst_dir}/"
        
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

