#!/usr/bin/env bash
#
# Standalone cmake+make build for a SO3 / Linux-agency user space, meant to be
# run from inside a usr/ directory (so3/usr or linux/usr). It reproduces what
# the corresponding bitbake recipe does in do_build, so the apps (and, for
# linux/usr, the out-of-tree kernel modules) can be rebuilt quickly without
# invoking bitbake.
#
# It auto-detects the context from the directory it is invoked in:
#   - <root>/so3/usr    -> musl toolchain (in-tree <cpu>-linux-musl.cmake)
#   - <root>/linux/usr  -> buildroot toolchain + kernel modules
#
# Copyright (c) 2026 Daniel Rossier, REDS Institute - HEIG-VD
#

set -euo pipefail

# The script operates on the *current directory* (so it can live in ~/scripts),
# which must be a usr/ dir. The repo root is two levels up (…/usr -> … -> root).

USR_DIR="$PWD"
REPO_ROOT="$(cd "${USR_DIR}/../.." && pwd)"
BUILD_DIR="${USR_DIR}/build"

# Detect so3 vs linux context from the parent directory name.

PARENT="$(basename "$(dirname "${USR_DIR}")")"
case "${PARENT}" in
	so3)   MODE="so3" ;;
	linux) MODE="linux" ;;
	*)
		if [ -f "${USR_DIR}/aarch64-linux-musl.cmake" ]; then
			MODE="so3"
		else
			echo "Run this from so3/usr or linux/usr (current: ${USR_DIR})" >&2
			exit 1
		fi
		;;
esac

if [ ! -f "${USR_DIR}/CMakeLists.txt" ]; then
	echo "No CMakeLists.txt in ${USR_DIR} — not a usr/ directory." >&2
	exit 1
fi

# Defaults.

ARCH=""
BUILD_TYPE="${IB_USR_BUILD_TYPE:-Debug}"
PLATFORM=""
MICROPYTHON="OFF"
DO_CLEAN=0
RECONFIGURE=0
SKIP_MODULES=0
CORES="$(nproc)"

usage() {
	cat <<EOF
Usage: makeusr.sh [options]   (run from so3/usr or linux/usr)

Options:
  -a, --arch <aarch64|arm>   Target CPU (so3/usr only; default: last, else aarch64)
  -t, --type <Debug|Release> CMake build type (default: ${BUILD_TYPE})
  -p, --platform <name>      IB_PLATFORM for kernel modules (linux/usr; default: virt64)
  -m, --micropython          Build the micro-python apps (so3/usr, aarch64 only)
  -c, --reconfigure          Force a fresh cmake configure step
  -M, --no-modules           Skip the kernel-module build (linux/usr)
  -C, --clean                Remove build/ and exit
  -j, --jobs <N>             Parallel make jobs (default: ${CORES})
  -h, --help                 This help

Context detected: ${MODE}  (${USR_DIR})
EOF
}

while [ $# -gt 0 ]; do
	case "$1" in
		-a|--arch)        ARCH="$2"; shift 2 ;;
		-t|--type)        BUILD_TYPE="$2"; shift 2 ;;
		-p|--platform)    PLATFORM="$2"; shift 2 ;;
		-m|--micropython) MICROPYTHON="ON"; shift ;;
		-c|--reconfigure) RECONFIGURE=1; shift ;;
		-M|--no-modules)  SKIP_MODULES=1; shift ;;
		-C|--clean)       DO_CLEAN=1; shift ;;
		-j|--jobs)        CORES="$2"; shift 2 ;;
		-h|--help)        usage; exit 0 ;;
		*) echo "Unknown option: $1" >&2; usage; exit 1 ;;
	esac
done

if [ "${DO_CLEAN}" = "1" ]; then
	echo "Removing ${BUILD_DIR}"

	rm -rf "${BUILD_DIR}"
	exit 0
fi

# ============================================================================
# so3/usr
# ============================================================================

build_so3() {
	local arch_marker="${USR_DIR}/.ib_last_arch"
	local toolchain_file musl_target cc_name

	# Resolve target arch: explicit -a, else last built, else aarch64.

	if [ -z "${ARCH}" ]; then
		if [ -f "${arch_marker}" ]; then ARCH="$(cat "${arch_marker}")"; else ARCH="aarch64"; fi
	fi

	case "${ARCH}" in
		aarch64)
			toolchain_file="${USR_DIR}/aarch64-linux-musl.cmake"
			musl_target="aarch64-linux-musl"; cc_name="aarch64-linux-musl-gcc" ;;
		arm)
			toolchain_file="${USR_DIR}/arm-linux-musl.cmake"
			musl_target="arm-linux-musleabihf"; cc_name="arm-linux-musleabihf-gcc" ;;
		*) echo "Unsupported arch '${ARCH}' (aarch64|arm)" >&2; exit 1 ;;
	esac

	# The cmake toolchain files use bare compiler names, so the musl bin/ must
	# be on PATH. It usually is (/usr/local/bin); else fall back to the
	# bitbake-built toolchain under build/tmp/toolchains.

	if ! command -v "${cc_name}" >/dev/null 2>&1; then
		local fallback="${REPO_ROOT}/build/tmp/toolchains/${musl_target}/bin"

		if [ -x "${fallback}/${cc_name}" ]; then
			export PATH="${fallback}:${PATH}"
		else
			echo "Cross compiler '${cc_name}' not found on PATH nor in ${fallback}" >&2
			echo "Build the toolchain first (e.g. build.sh -x musl-toolchain)." >&2
			exit 1
		fi
	fi

	# The CMakeCache pins the compiler; switching arch without wiping produces
	# wrong-arch binaries. Mirror the recipe's do_build:prepend.

	if [ -f "${arch_marker}" ] && [ "$(cat "${arch_marker}")" != "${ARCH}" ]; then
		echo "SO3 usr arch changed ($(cat "${arch_marker}") -> ${ARCH}); wiping build/"

		rm -rf "${BUILD_DIR}"
	fi
	echo "${ARCH}" > "${arch_marker}"

	# Configure (only when needed) then build.

	mkdir -p "${BUILD_DIR}"; cd "${BUILD_DIR}"

	if [ "${RECONFIGURE}" = "1" ] || [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
		cmake -Wno-dev --no-warn-unused-cli \
			-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
			-DMICROPYTHON="${MICROPYTHON}" \
			-DCMAKE_TOOLCHAIN_FILE="${toolchain_file}" \
			"${USR_DIR}"
	fi
	make -j"${CORES}"

	# Local deploy == do_install_apps: *.elf + out/* into build/deploy.

	local deploy="${BUILD_DIR}/deploy"
	mkdir -p "${deploy}"
	cp "${BUILD_DIR}"/src/*.elf "${deploy}/" 2>/dev/null || true
	cp -r "${USR_DIR}"/out/* "${deploy}/" 2>/dev/null || true

	echo
	echo "Done. Built so3/usr ${ARCH} (${BUILD_TYPE}) -> ${deploy}"
}

# ============================================================================
# linux/usr
# ============================================================================

build_linux() {
	local toolchain_file="${REPO_ROOT}/linux/rootfs/host/share/buildroot/toolchainfile.cmake"
	local kernel_path="${REPO_ROOT}/linux/linux"

	if [ ! -f "${toolchain_file}" ]; then
		echo "Buildroot toolchain missing: ${toolchain_file}" >&2
		echo "Build the rootfs/toolchain first via bitbake." >&2
		exit 1
	fi

	# User apps: cmake + make. The buildroot toolchain file pins absolute
	# compiler paths, so no PATH tweak is needed here.

	mkdir -p "${BUILD_DIR}"; cd "${BUILD_DIR}"

	if [ "${RECONFIGURE}" = "1" ] || [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
		cmake -Wno-dev --no-warn-unused-cli \
			-DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
			-DCMAKE_KERNEL_PATH="${kernel_path}" \
			-DCMAKE_TOOLCHAIN_FILE="${toolchain_file}" \
			"${USR_DIR}"
	fi
	make -j"${CORES}"

	# Out-of-tree kernel modules (best effort), mirroring usr-linux
	# do_build:prepend's `make -C linux M=src/modules modules IB_PLATFORM=...`.
	# The kernel Makefile is patched with ARCH and CROSS_COMPILE defaults
	# (e.g. aarch64-none-linux-gnu-), so — exactly like the recipe — we pass
	# neither and let the kernel toolchain resolve from PATH. Do NOT force a
	# CROSS_COMPILE from the buildroot host tree: that is the *userspace*
	# toolchain and may differ from the one the kernel was built with.

	local mod_dir="${USR_DIR}/src/modules"

	if [ "${SKIP_MODULES}" = "0" ] && [ -d "${mod_dir}" ]; then

		if [ -f "${kernel_path}/Module.symvers" ]; then

			# IB_PLATFORM selects the module set (see src/modules/Makefile).

			local plat="${PLATFORM}"

			if [ -z "${plat}" ]; then
				plat="$(grep -E '^[[:space:]]*IB_PLATFORM[[:space:]]*[?:]?=' \
					"${REPO_ROOT}/build/conf/local.conf" 2>/dev/null \
					| grep -v '^[[:space:]]*#' | tail -1 \
					| sed -E 's/.*=[[:space:]]*"?([^"[:space:]]+)"?.*/\1/')"
				plat="${plat:-virt64}"
			fi

			# Sanity-check the kernel's cross compiler is reachable on PATH.

			local kcross
			kcross="$(sed -nE 's/^CROSS_COMPILE[[:space:]]*[?:]?=[[:space:]]*([^[:space:]]+).*/\1/p' \
				"${kernel_path}/Makefile" | head -1)"

			if [ -n "${kcross}" ] && ! command -v "${kcross}gcc" >/dev/null 2>&1; then
				echo "WARN: kernel cross compiler '${kcross}gcc' not on PATH; skipping modules." >&2
			else
				echo "Building kernel modules (IB_PLATFORM=${plat})"

				make -C "${kernel_path}" M="${mod_dir}" modules IB_PLATFORM="${plat}"
			fi
		else
			echo "WARN: ${kernel_path}/Module.symvers missing; kernel not built."
			echo "      Skipping kernel modules (build the kernel via bitbake first)."
		fi
	fi

	# Local deploy == do_install_apps: hello + *.ko into build/deploy/root.
	# Plus the SOO s3c-* apps when built (usr-linux :soo bbappend override).

	local deploy="${BUILD_DIR}/deploy/root"
	mkdir -p "${deploy}"
	cp "${BUILD_DIR}"/src/examples/hello "${deploy}/" 2>/dev/null || true
	cp "${mod_dir}"/*.ko "${deploy}/" 2>/dev/null || true
	cp "${BUILD_DIR}"/src/soo/s3c-* "${deploy}/" 2>/dev/null || true

	echo
	echo "Done. Built linux/usr (${BUILD_TYPE}) -> ${BUILD_DIR}/deploy"
}

# Dispatch.

case "${MODE}" in
	so3)   build_so3 ;;
	linux) build_linux ;;
esac
