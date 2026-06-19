SUMMARY = "MUSL cross toolchain for SO3 user space"
DESCRIPTION = "Builds the cross toolchain used to compile the SO3 user-space \
applications, via richfelker/musl-cross-make. Replaces the former \
toolchains/build-toolchain.sh script. Only the toolchain matching the \
configured platform CPU (local.conf) is built."

LICENSE = "GPLv2 & MIT"

# Release and version
PR = "r0"
PV = "1.0"

# musl-cross-make pinned commit (was GIT_COMMIT=3635262 in the old
# toolchains/build-toolchain.sh).
SRC_URI = "git://github.com/richfelker/musl-cross-make;protocol=https;nobranch=1"
SRCREV = "3635262e4524c991552789af6f36211a335a77b3"

# The common config.mak shipped alongside this recipe.
FILESPATH:prepend = "${THISDIR}/files:"
SRC_URI += "file://config.mak"

# IB_MUSL_TARGET follows the platform CPU (defined in local.conf;
# OVERRIDES carries ${IB_PLAT_CPU}). Building for aarch64 does not build
# the arm variant and vice-versa.

# Fetched musl-cross-make source/build tree and the final install
# prefix, both under the bitbake-managed build/tmp area
# (IB_MUSL_TOOLCHAIN_DIR is defined in local.conf).
IB_TARGET = "${IB_MUSL_TOOLCHAIN_DIR}/musl-cross-make"
IB_MUSL_OUTPUT = "${IB_MUSL_TOOLCHAIN_DIR}/${IB_MUSL_TARGET}"

do_configure[nostamp] = "1"
do_configure () {
	cd ${IB_TARGET}

	# Common settings + the arch-specific TARGET/OUTPUT selected here.
	cp ${WORKDIR}/config.mak config.mak
	echo "TARGET = ${IB_MUSL_TARGET}" >> config.mak
	echo "OUTPUT = ${IB_MUSL_OUTPUT}" >> config.mak
}

# do_configure is nostamp (it re-pins TARGET/OUTPUT in config.mak per arch),
# which forces this do_build to re-run on every build that depends on
# musl-toolchain:do_build (e.g. `build.sh -x usr-so3`). Guard against that:
# if the compiler for the current IB_MUSL_TARGET is already installed, skip
# the (very long) binutils/gcc/musl build. A different arch yields a different
# IB_MUSL_OUTPUT path, so it still builds; `do_clean` wipes the output to force
# a rebuild.
do_build () {
	if [ -x "${IB_MUSL_OUTPUT}/bin/${IB_MUSL_TARGET}-gcc" ]; then
		echo "musl cross toolchain '${IB_MUSL_TARGET}' already built — skipping."
	else
		echo "Building musl cross toolchain '${IB_MUSL_TARGET}' — this downloads and compiles binutils/gcc/musl, expect a long first build..."

		cd ${IB_TARGET}
		make -j${CORES}
		make install
	fi
}

do_clean[nostamp] = "1"
do_clean () {
	rm -rf ${IB_MUSL_OUTPUT}
	rm -f ${TMPDIR}/stamps/musl-toolchain*
}
addtask do_clean
