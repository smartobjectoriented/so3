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

# Deploy the usr contents, i.e. the deploy/ dir, into the rootfs partition
# (p2) of the filesystem. rootfs-linux:do_deploy runs first (dependency above)
# so the apps land on top of the freshly extracted rootfs; bsp-linux:do_deploy
# pulls this task, so a full `deploy.sh bsp-linux` always deploys usr too.

python do_deploy() {

    import os

    # Same exception as rootfs-linux:do_deploy: verdin-imx8mp storage goes
    # through the Tezi / HTTP recovery flow, there is no p2 to mount here.

    if d.getVar('IB_PLATFORM') == "verdin-imx8mp":
        bb.plain("verdin-imx8mp: rootfs delivered via Tezi/HTTP, skipping usr partition deploy")
        return

    IB_USR_PATH = d.getVar('IB_USR_PATH')
    IB_FILESYSTEM_PATH = d.getVar('IB_FILESYSTEM_PATH')
    IB_ROOTFS_PARTITION = d.getVar('IB_ROOTFS_PARTITION')

    deploy_src = os.path.join(IB_USR_PATH, "build", "deploy")
    rootfs_dst = os.path.join(IB_FILESYSTEM_PATH, IB_ROOTFS_PARTITION)

    if not os.path.isdir(deploy_src):
        bb.fatal("The {} does not exist; please build usr first...".format(deploy_src))

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

# No `after do_build`: deploy is decoupled from the build (edit -> build.sh
# -> deploy.sh). do_deploy only copies the already-built build/deploy/ and
# fails clearly above when it is missing, instead of dragging usr-linux:
# do_build (and linux:do_build through do_unpack) into every deploy.

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
