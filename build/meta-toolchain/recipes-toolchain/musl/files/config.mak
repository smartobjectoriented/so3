#
# musl-cross-make configuration for the SO3 user-space toolchain.
#
# This is the common (arch-independent) configuration. The recipe
# (musl-toolchain_*.bb) appends the TARGET and OUTPUT lines, selecting
# the target from the configured platform CPU (local.conf):
#   aarch64 -> aarch64-linux-musl
#   arm     -> arm-linux-musleabihf
#

# musl-cross-make downloads the gcc/binutils/gmp/mpfr/mpc/musl tarballs during
# the build (do_build). The stock GNU_SITE is ftpmirror.gnu.org, which 302-
# redirects to a RANDOM mirror; some mirrors are incomplete (404) or stall, and
# wget --tries just re-hits the same redirect, so the toolchain build failed
# intermittently in CI (passed on re-run). Fetch GNU components from the
# canonical, complete host instead of a random mirror, and still add retries/
# timeouts as a safety net. (-O must stay last: the Makefile appends
# <outfile> <url> after DL_CMD.)
GNU_SITE = https://ftp.gnu.org/gnu
DL_CMD = wget -c --tries=5 --waitretry=10 --timeout=30 -O

# Same story for the musl tarball itself: musl.libc.org is a single host with
# no mirror rotation, and when it goes down (timeouts, then "Unable to
# establish SSL connection" — observed 2026-08-10, both CI platforms and from
# outside CI) the toolchain build fails at ~2 min and no amount of retrying
# helps. Fetch it from the MacPorts distfiles mirror, which carries the exact
# upstream tarball. musl-cross-make checks every download against
# hashes/<file>.sha1, so a mirror serving anything else fails the build loudly
# rather than silently building something different (verified: the mirrored
# musl-1.2.5.tar.gz matches hashes/musl-1.2.5.tar.gz.sha1).
MUSL_SITE = https://distfiles.macports.org/musl

BINUTILS_VER = 2.44
GCC_VER = 12.4.0
# MUSL_VER = git-master
# GMP_VER =
# MPC_VER =
# MPFR_VER =
# ISL_VER =
# LINUX_VER =

# Recommended options for a smaller toolchain / deployable binaries:
COMMON_CONFIG += CFLAGS="-g -Os" CXXFLAGS="-g0 -Os" LDFLAGS="-s"

# Disable autotools "maintainer mode" when configuring gcc and its in-tree
# deps (gmp/mpfr/mpc/isl). Without this, if the extracted/copied source files
# have inconsistent timestamps (configure.ac newer than configure — which
# happens when the tree is copied without preserving mtimes, e.g. the CI
# container's fresh build), make tries to regenerate configure/Makefile.in and
# fails with "automake-1.17: command not found" / "autoconf: command not found".
# The so3-env CI image has no autotools (and Ubuntu ships automake 1.16, not the
# 1.17 mpfr wants), so the regen can never succeed there. Disabling maintainer
# mode turns those regen rules into no-ops and makes the build environment- and
# timestamp-independent.
GCC_CONFIG += --disable-maintainer-mode
