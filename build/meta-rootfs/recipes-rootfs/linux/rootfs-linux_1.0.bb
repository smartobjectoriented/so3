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

# do_deploy extracts rootfs.cpio, copies it onto p2 and repacks it. The usr
# apps are either already in rootfs.cpio (IB_RAMFS_SOURCE = "rootfs") or
# copied on top afterwards by usr-linux:do_deploy.
#
# It DOES depend on ${IB_ROOTFS_METHOD}:do_build, and that dependency is
# load-bearing rather than a leftover: nothing else orders this task after
# the rootfs generator, and usr-linux:do_deploy pulls it into the *build*
# graph too (usr-linux:do_deploy[depends] = rootfs-linux:do_deploy). Without
# it, bitbake is free to run this task while buildroot is still writing
# board/<plat>/rootfs.cpio, and `cpio -id` then reads a truncated archive and
# aborts with exit status 2 — an intermittent build failure whose message
# says nothing about a race. It also covers the fresh-checkout case, where
# there is no cached cpio at all.
#
# The cost is one `make` in an already-built buildroot on a standalone
# deploy (do_build is nostamp), which is a no-op in practice — cheap next to
# a racy deploy. Do not "simplify" this dependency away.
do_deploy[depends] += "filesystem:do_fs_check ${IB_ROOTFS_METHOD}:do_build"

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
	# The copy refreshes the source config files on top of the working copy
	# but does NOT remove the dest-only productions (no --delete), so the
	# do_prepare_initrd content-hash guard (initrd.cpio.gz.srchash +
	# initrd.cpio.gz) keeps working across builds.
	#
	# Productions left under files/board by an older tree must not be
	# copied forward: a stale rootfs.cpio would replace the fresh one, and
	# buildroot's guard, which hashes these files as outputs, would then skip
	# the make that regenerates it. The exclusions mirror buildroot's hash,
	# except plain initrd.cpio, a source on the "initrd" ramfs boards.
	mkdir -p ${WORKDIR}/board
	rsync -a \
		--exclude 'rootfs.cpio' \
		--exclude '*.cpio.gz' \
		--exclude '*.srchash' \
		--exclude '*.backup' \
		--exclude '*.sha256' \
		${FILE_DIRNAME}/files/board/ ${WORKDIR}/board/

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

    # Unwind both mounts taken above before failing, or the next build trips
    # over the leftover loop device and mounts.
    if not os.path.exists("{}/{}".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION)):
        __do_rootfs_umount(d)
        __do_fs_umount(d)
        bb.fatal("No {}/{} exists ...".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION))

    # p2 holds this rootfs and what usr-linux adds on top, both deployed
    # after it: it is emptied first, or a file the build no longer has would
    # stay on the card. Only a mounted p2 is emptied, never the directory
    # under it; lost+found belongs to ext4.
    p2 = os.path.realpath("{}/{}".format(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION))
    if not os.path.ismount(p2):
        __do_rootfs_umount(d)
        __do_fs_umount(d)
        bb.fatal("{} is not mounted: refusing to deploy the rootfs".format(p2))
    subprocess.run(["sudo", "find", p2, "-mindepth", "1", "-maxdepth", "1", "!", "-name", "lost+found",
                    "-exec", "rm", "-rf", "{}", "+"], check=True)

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
 
