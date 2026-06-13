#
# musl-cross-make configuration for the SO3 user-space toolchain.
#
# This is the common (arch-independent) configuration. The recipe
# (musl-toolchain_*.bb) appends the TARGET and OUTPUT lines, selecting
# the target from the configured platform CPU (local.conf):
#   aarch64 -> aarch64-linux-musl
#   arm     -> arm-linux-musleabihf
#

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
