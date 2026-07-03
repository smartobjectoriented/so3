# Copyright (c) 2025-2026 EDGEMTech SA

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

# do_deploy is a pure DEPLOY step: it extracts the already-built rootfs.cpio
# (apps baked in by usr-linux:do_deploy during the build) onto p2. It must
# NOT pull ${IB_ROOTFS_METHOD}:do_build — that would rebuild buildroot on
# every `deploy.sh`. The workflow is edit -> build.sh (produces rootfs.cpio)
# -> deploy.sh; a deploy with no prior build fails clearly in
# __do_rootfs_mount (missing rootfs.cpio) rather than silently rebuilding.
do_deploy[depends] += "filesystem:do_fs_check"

do_build[nostamp] = "1"
do_build () {

    echo building Linux rootfs
}
addtask do_build

do_attach_infrabase[depends] = "${IB_ROOTFS_METHOD}:do_attach_infrabase"

# Re-run on every build so edits to the source board files (post_image.sh,
# rootfs_overlay, ...) are propagated into the working copy below.

do_attach_infrabase[nostamp] = "1"

do_attach_infrabase () {

	# Keep a private working copy of the board directory under WORKDIR
	# (tmp/work). The rootfs generator writes its productions there during
	# the build (rootfs.cpio via post_image.sh, then rootfs.cpio.backup,
	# initrd.cpio.gz, initrd.cpio.gz.srchash). Pointing the board symlink
	# at this copy keeps those productions out of the git-tracked source
	# tree under files/board. Note: board/<plat>/initrd.cpio is now a
	# git-tracked SOURCE (the static embedded ramfs for IB_RAMFS_SOURCE =
	# "initrd"), refreshed from files/board on every attach.
	#
	# `cp -r .../board/.` refreshes the source config files on top of the
	# copy but does NOT remove the dest-only productions, so the
	# do_prepare_initrd content-hash guard (initrd.cpio.gz.srchash +
	# initrd.cpio.gz) keeps working across builds.
	mkdir -p ${WORKDIR}/board
	cp -r ${FILE_DIRNAME}/files/board/. ${WORKDIR}/board

	# Expose the working copy through the historical board symlink so every
	# consumer keeps referencing ${IB_ROOTFS_PATH}/board/... unchanged.
	rm -f ${IB_TARGET}/board
	ln -fs ${WORKDIR}/board ${IB_TARGET}/board
}

# Deployment of the rootfs contents

# The deployment of the rootfs is done in the second partition

# This function performs a copy of the whole rootfs contents 
# stored in rootfs.cpio into the rootfs partition of the filesystem

do_deploy[nostamp] = "1"
python do_deploy () {
    import subprocess

    bb.plain("Deploy the rootfs into the filesystem")

    # Verdin-imx8mp does not deploy its rootfs via a build-time partition
    # mount: storage is delivered through the platform's Tezi / HTTP recovery
    # flow (IB_STORAGE_MODE = "http"), so the verdin __do_fs_mount would try
    # to mount a physical /dev/sda1 that is absent in a build/CI context,
    # so do_deploy returns early for verdin here.
    if d.getVar('IB_PLATFORM') == "verdin-imx8mp":
        bb.plain("verdin-imx8mp: rootfs delivered via Tezi/HTTP, skipping partition deploy")
        return

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

    # Privileged + archive copy: the source IB_TARGET/fs is the rootfs image
    # loop-mounted as root (so its files are root-owned, some unreadable to the
    # unprivileged builder — a plain `cp` aborts with exit 1), and the ext4 p2
    # rootfs needs ownership/perms/symlinks preserved (setuid bins, /etc/shadow,
    # ...). `sudo cp -a` both reads the root-owned tree and reproduces it faithfully.
    cmd = f"sudo cp -av {IB_TARGET}/fs/. {IB_FILESYSTEM_PATH}/{IB_ROOTFS_PARTITION}"

    result = subprocess.run(cmd, shell=True, check=True)

    # Change back to the original working directory
    os.chdir(original_cwd)

    __do_rootfs_umount(d)
    __do_fs_umount(d)

}
addtask do_deploy

do_clean[depends] = "${IB_ROOTFS_METHOD}:do_clean"

do_clean[nostamp] = "1"
do_clean () {

	# Drop the board working copy (holds every build production) and the
	# symlink that exposed it under the rootfs path.
	
	rm -rf ${WORKDIR}/board
	rm -f ${IB_TARGET}/board

	rm -f ${TMPDIR}/stamps/rootfs-linux*
}
addtask do_clean
 
