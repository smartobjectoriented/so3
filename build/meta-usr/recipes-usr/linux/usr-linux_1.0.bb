# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "User space applications for Linux"
DESCRIPTION = "All (Linux) user space custom applications which take place in the rootfs of Linux"
LICENSE = "GPLv2"

inherit usr
inherit linux
inherit filesystem
inherit rootfs

# Release and version
PR = "r0"
PV = "1.0"

OVERRIDES += ":linux"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_DIR}/linux/usr"

IB_TOOLCHAIN_PATH = "${IB_ROOTFS_PATH}/host/share/buildroot/toolchainfile.cmake"

# The agency user space is NOT committed in this repository: the whole tree is
# regenerated from a patch set (the infrabase base below + the :soo override
# apps). do_unpack therefore starts from an EMPTY ${S} (retrieve_usr_dir, which
# would seed ${S} from a committed IB_TARGET, is dropped), the base + soo
# creation patches populate ${S}, and do_attach_infrabase regenerates IB_TARGET
# from the patched ${S} before do_build. so3/linux/usr is gitignored.
FILESPATH:prepend = "${THISDIR}/files/0001-${PF}:"
require files/0001-${PF}-patches.inc

# Drop retrieve_usr_dir (it would seed ${S} from a committed IB_TARGET that does
# not exist here) so do_unpack leaves ${S} empty for the creation patches.
python () {
    pf = (d.getVarFlag('do_unpack', 'postfuncs') or '').split()
    pf = [x for x in pf if x != 'retrieve_usr_dir']
    d.setVarFlag('do_unpack', 'postfuncs', ' '.join(pf))
}
addtask do_attach_infrabase after do_patch before do_build

do_build[depends] = "rootfs-linux:do_build"
do_unpack[depends] += "linux:do_build"

do_deploy[depends] = "rootfs-linux:do_deploy"
do_deploy[nostamp] = "1"

# Deploy the usr contents, i.e. the deploy/ dir, where the running system will
# find them. That depends on what the kernel boots as its root, and the apps go
# to ONE place only:
#
#   IB_RAMFS_SOURCE = "rootfs" - the embedded ramfs IS rootfs.cpio, so the apps
#     are baked INTO rootfs.cpio (rsync into the extracted tree, then re-pack).
#     bsp-linux:do_prepare_initrd pulls this task into the BUILD, before it
#     gzips rootfs.cpio into the ITB, so here it must not touch the boot media:
#     it depends on rootfs-linux:do_build instead of rootfs-linux:do_deploy and
#     runs after do_build (anonymous function below).
#
#   any other value (the static "initrd" ramfs, which pivots to p2) - the apps
#     are copied onto the rootfs partition p2, on top of what
#     rootfs-linux:do_deploy extracted. bsp-linux:do_deploy pulls this task,
#     so a full deploy always carries the user space.
#
# Unset means "rootfs", the bsp.bbclass default.

def usr_linux_ramfs_is_rootfs(d):
    return (d.getVar('IB_RAMFS_SOURCE') or "rootfs").strip() == "rootfs"

python () {
    if usr_linux_ramfs_is_rootfs(d):
        d.setVarFlag('do_deploy', 'depends', 'rootfs-linux:do_build')
        bb.build.addtask('do_deploy', None, 'do_build', d)
}

python do_deploy() {

    import os

    IB_USR_PATH = d.getVar('IB_USR_PATH')
    deploy_src = os.path.join(IB_USR_PATH, "build", "deploy")

    if not os.path.isdir(deploy_src):
        bb.fatal("The {} does not exist; please build usr first...".format(deploy_src))

    if usr_linux_ramfs_is_rootfs(d):
        IB_ROOTFS_PATH = d.getVar('IB_ROOTFS_PATH')
        IB_PLATFORM = d.getVar('IB_PLATFORM')

        if not os.path.isfile(os.path.join(IB_ROOTFS_PATH, "board", IB_PLATFORM, "rootfs.cpio")):
            bb.fatal("rootfs.cpio is missing; please build rootfs first...")

        # The extracted tree is root-owned (cpio -id), so rsync needs root to
        # write into it while preserving mode bits and ownership.

        d.setVar('ROOTFS_FILENAME', 'rootfs')
        __do_rootfs_mount(d)
        utils_sudo(["rsync", "-a", "--keep-dirlinks",
                    deploy_src + "/", f"{IB_ROOTFS_PATH}/fs/"], check=True)
        __do_rootfs_umount(d)

        bb.plain("usr deployed into rootfs.cpio (IB_RAMFS_SOURCE = rootfs)")
        return

    # Same exception as rootfs-linux:do_deploy: verdin-imx8mp storage goes
    # through the Tezi / HTTP recovery flow, there is no p2 to mount here.

    if d.getVar('IB_PLATFORM') == "verdin-imx8mp":
        bb.plain("verdin-imx8mp: rootfs delivered via Tezi/HTTP, skipping usr partition deploy")
        return

    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    IB_ROOTFS_PARTITION = d.getVar('IB_ROOTFS_PARTITION')
    rootfs_dst = os.path.join(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION)

    __do_fs_mount(d)

    if not os.path.isdir(os.path.join(rootfs_dst, "root")):
        __do_fs_umount(d)
        bb.fatal("The root directory is not present in the second partition; please deploy rootfs...")

    # p2 is written by rootfs-linux:do_deploy with `sudo cp -a`, so its tree
    # is root-owned: the copy needs root too. --keep-dirlinks keeps the
    # rootfs directory symlinks (e.g. /lib -> usr/lib) instead of replacing
    # them with real directories. check=True: a failed copy must fail the
    # deploy, not leave a rootfs silently without the apps.

    try:
        utils_sudo(["rsync", "-a", "--keep-dirlinks",
                    deploy_src + "/", rootfs_dst + "/"], check=True)
    finally:
        __do_fs_umount(d)
}

# No static `after do_build`: in the p2 case do_deploy is a pure deploy step
# (edit -> build.sh -> deploy.sh) that copies the already-built build/deploy/
# and fails clearly when it is missing, instead of dragging usr-linux:do_build
# (and linux:do_build through do_unpack) into every deploy. The rootfs.cpio
# case, which runs inside the build, gets its ordering above.

addtask do_deploy

# Build extra components which is not in src/ directory like modules
do_build:prepend () {

	# Modules

	if [ ! -f ${IB_LINUX_PATH}/Module.symvers ]; then
        	echo "Generating Module.symvers..." ;
        	make -j${CORES} -C ${IB_LINUX_PATH} modules ;
            make INSTALL_MOD_PATH=${IB_ROOTFS_PATH}/target -C ${IB_LINUX_PATH} modules_install ; \
    fi

	make -C ${IB_LINUX_PATH} M=${IB_TARGET}/src/modules modules IB_PLATFORM=${IB_PLATFORM}
}

# Installing usr apps mean to move the binary and all files which need to
# be copied to the rootfs. Be aware that it is a deploy directory and not
# the rootfs itself; this is achieved with the do_deploy task (by the bsp recipe)

do_install_apps () {

    usr_do_install_file_root "${IB_TARGET}/build/src/examples/hello"

    # Installation of modules if any

    usr_do_install_file_root "${IB_TARGET}/src/modules/*.ko"
}

do_clean:append () {

    rm -f ${TMPDIR}/stamps/usr-linux*
    rm -f ${WORKDIR}/*.patch

    # The whole IB_TARGET is regenerated from the patch set on every build,
    # so a clean removes it entirely (tree, re-attach backup and manifest).
    rm -rf ${IB_TARGET} ${IB_TARGET}.back ${IB_TARGET}.attach.sha256
}
