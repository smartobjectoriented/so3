# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "AVZ Hypervisor"
DESCRIPTION = "AVZ (Agency Virtualizer) hypervisor based on polymorphic SO3 Operating System"
LICENSE = "GPLv2"

inherit avz

# Version and revision
 
PR = "r0"
PV = "6.2.0"

OVERRIDES += ":avz"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_AVZ_PATH}"

# Build AVZ from this local so3 tree's HEAD (so the hypervisor carries the
# in-tree changes). Bump SRCREV after committing AVZ changes here.
SRC_URI = "git:///home/rossierd/soo/so3;protocol=file;nobranch=1"
SRCREV = "54ad443f569be2b740e33c04e3a2d30ab45fe49a"

python do_handle_fetch_git() {

    import os
    import subprocess

    # Copy only the SO3 kernel. The repo nests the kernel under
    # <root>/so3/so3, so copy from gitdir/so3/so3 (the kernel root).

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    dst_dir = d.getVar('S')

    cmd = f"find . -not -path '*/.git/*' -and \( -type f -or -type d -empty \) -exec cp -r --parents -t {dst_dir} {{}} +"
    result = subprocess.run(cmd, shell=True, check=True, cwd=os.path.join(gitdir, "so3", "so3"))
}

# Apply the EDGE-M1 platform + GICv3 vGIC patch to S so that
# do_attach_infrabase faithfully reproduces the local working tree.
# Idempotent: re-applying is detected by the standard quilt machinery.

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_CONFIG}
}

do_build[nostamp] = "1"
do_build () {
	echo "Building AVZ..."

	cd ${IB_TARGET}
	make
}

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/avz*
}
addtask do_clean
