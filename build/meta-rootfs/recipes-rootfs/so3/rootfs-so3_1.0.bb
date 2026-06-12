
SUMMARY = "SO3 root filesystem"
DESCRIPTION = "SO3 root filesystem contents to be deployed on the target platform"
LICENSE = "GPLv2"

inherit rootfs

# Release and version
PR = "r0"
PV = "1.0"

OVERRIDES += ":so3"

IB_TARGET = "${IB_DIR}/so3/rootfs"

do_build[depends] = "so3:do_build"

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

do_mount_rootfs[nostamp] = "1"
python do_rootfs_mount() {
    d.setVar('ROOTFS_FILENAME', '')
    __do_rootfs_mount(d)
}
 
do_umount_rootfs[stamp] = "1"
python do_rootfs_umount() {
    d.setVar('ROOTFS_FILENAME', '')
    __do_rootfs_umount(d)
}

do_build[nostamp] = "1"
do_build () {

    echo building SO3 rootfs
	
    cd ${IB_TARGET}
    ./create_ramfs.sh
	
}
addtask do_build

do_deploy[nostamp] = "1"
do_deploy () {

	echo No specific deployment for SO3 since it is fully contained in the ITB file
}
addtask do_deploy

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/rootfs-so3*
}
addtask do_clean
