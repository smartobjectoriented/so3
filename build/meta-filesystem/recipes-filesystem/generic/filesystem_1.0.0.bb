
SUMMARY = "Filesystem management"
DESCRIPTION = "This is the core filesystem recipe to create a virtual or physical \
               multi-partitioned disk."
LICENSE = "GPLv2"

# Version
PV = "1.0.0"
PR = "r0"

inherit filesystem
inherit logging

do_configure[noexec] = "1"
do_attach_infrabase[noexec] = "1"

IB_TARGET = "${IB_FILESYSTEM_PATH}"

do_build[nostamp] = "1"
do_build[depends] = "filesystem:do_fs_check"
do_build[noexec] = "1"
