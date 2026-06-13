
SUMMARY = "Root filesystem"
DESCRIPTION = "Root filesystem contents to be deployed on the target platform"
LICENSE = "GPLv2"

inherit rootfs
inherit filesystem

# Version and revision
PR = "r0"
PV = "1.0"

OVERRIDES += ":linux"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_ROOTFS_PATH}"

do_configure[depends] = "${IB_ROOTFS_METHOD}:do_configure"
do_configure[noexec] = "1"

do_build[depends] = "${IB_ROOTFS_METHOD}:do_build"
do_build[depends] += "linux:do_build"

# Check that the image is present before deploying rootfs
do_deploy[depends] += "filesystem:do_fs_check"

do_build[nostamp] = "1"
do_build () {

    echo building Linux rootfs
}
addtask do_build

do_attach_infrabase[depends] = "${IB_ROOTFS_METHOD}:do_attach_infrabase"
  
do_attach_infrabase () {
	
	# Remove previous link if any
	rm -f ${IB_TARGET}/board

	ln -fs ${FILE_DIRNAME}/files/board ${IB_TARGET}/board
}

# Deployment of the rootfs contents

# The deployment of the rootfs is done in the second partition

# This function performs a copy of the whole rootfs contents 
# stored in rootfs.cpio into the rootfs partition of the filesystem

do_deploy[nostamp] = "1"
python do_deploy () {
    import subprocess

    bb.plain("Deploy the rootfs into the filesystem")

    __do_fs_mount(d)

    d.setVar('ROOTFS_FILENAME', 'rootfs')
    __do_rootfs_mount(d)

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    IB_ROOTFS_PARTITION = d.getVar('IB_ROOTFS_PARTITION')
    IB_TARGET = d.getVar('IB_TARGET')
    WORKDIR = d.getVar('WORKDIR')

    # Check if the directory exists
    if not os.path.exists("{}/{}".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION)):
        __rootfs_do_umount_rootfs()
        bb.fatal("No {}/{} exists ...".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION))

    # Read and preserve the current working directory 
    original_cwd = os.getcwd()
    os.chdir("{}/fs".format(WORKDIR))

    # Change directory to IB_FILESYSTEM_PATH/IB_ROOTFS_PARTITION
    os.chdir("{}/{}".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION))

    # Copy files from IB_TARGET/fs to IB_FILESYSTEM_PATH/IB_ROOTFS_PARTITION
    cmd = f"ls {IB_TARGET}/fs/."
    result = subprocess.run(cmd, shell=True, check=True)

    cmd = f"cp -rv {IB_TARGET}/fs/. {IB_FILESYSTEM_PATH}/{IB_ROOTFS_PARTITION}"

    result = subprocess.run(cmd, shell=True, check=True)

    # Change back to the original working directory
    os.chdir(original_cwd)

    __do_rootfs_umount(d)
    __do_fs_umount(d)

    # Avoid creating logs,stamps and run files as root
    utils_restore_user_ownership(d)

}
addtask do_deploy

do_clean[depends] = "${IB_ROOTFS_METHOD}:do_clean"

do_clean[nostamp] = "1"
do_clean () {

    rm ${IB_TARGET}/board/${IB_PLATFORM}/rootfs.cpio*
    rm -f ${IB_TARGET}/board

	rm -f ${TMPDIR}/stamps/rootfs-linux*
}
addtask do_clean
 