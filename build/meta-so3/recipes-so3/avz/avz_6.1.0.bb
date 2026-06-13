
SUMMARY = "AVZ Hypervisor"
DESCRIPTION = "AVZ (Agency Virtualizer) hypervisor based on polymorphic SO3 Operating System"
LICENSE = "GPLv2"

inherit avz

# Version and revision

PR = "r0"
PV = "6.1.0"

OVERRIDES += ":avz"

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_AVZ_PATH}"

# AVZ (the hypervisor, EL2) is built from a pinned upstream SO3 kernel
# revision — independent of the in-tree so3/ which is the guest/capsule
# (EL1) under development. The SO3 kernel is fetched and attached into
# avz/, then configured (virt64_avz_defconfig) and built in place.
SRC_URI = "git://github.com/smartobjectoriented/so3.git;nobranch=1;protocol=https"
SRCREV = "5f831ecc5071380b9f2b98e946c605598f040c98"

python do_handle_fetch_git() {

    import os
    import subprocess

    # Copy only the SO3 kernel

    gitdir = os.path.join(d.getVar('WORKDIR'), 'git')
    dst_dir = d.getVar('S')

    cmd = f"find . -not -path '*/.git/*' -and \( -type f -or -type d -empty \) -exec cp -r --parents -t {dst_dir} {{}} +"
    result = subprocess.run(cmd, shell=True, check=True, cwd=os.path.join(gitdir, "so3"))
}

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}
	make ${IB_CONFIG}
}

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
