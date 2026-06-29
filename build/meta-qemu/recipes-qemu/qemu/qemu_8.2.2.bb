# Copyright (c) 2025-2026 EDGEMTech SA

SUMMARY = "QEMU EMulator"
DESCRIPTION = "QEMU emulation environment core receipt"
LICENSE = "GPLv2"

# Release and version
PR = "r0"
PV = "8.2.2"

inherit qemu

SRC_URI = "https://download.qemu.org/qemu-8.2.2.tar.xz;protocol=https"
		  	
SRC_URI[sha256sum] = "847346c1b82c1a54b2c38f6edbd85549edeb17430b7d4d3da12620e2962bc4f3"

# Set of patches to be applied

FILESPATH:prepend: := "${THISDIR}/files/0001-${PF}:"

require files/0001-${PF}-patches.inc

# Where the working directory will be placed in infrabase root dir
IB_TARGET = "${IB_DIR}/qemu"

# Keep generated/fetched trees out of the updiff patchset so only hand-edited
# sources (hw/arm/virt.c, include/hw/arm/virt.h, ...) are diffed against the
# pristine snapshot:
#   build/        — QEMU's out-of-tree meson/ninja dir (1000+ generated files)
#   subprojects/  — meson subprojects (dtc, libslirp, ...) cloned during
#                   do_configure, i.e. AFTER the pristine snapshot is taken, so
#                   they'd otherwise show up as hundreds of "added" files.
#   GNUmakefile   — meson-generated top-level ninja wrapper (qemu ships a
#                   Makefile + meson.build; this file is created at configure).
# All three are produced AFTER do_unpack snapshots the pristine tree, so they
# read as "added" in the diff; none are ever hand-edited.

IB_UPDIFF_EXCLUDE = "build subprojects GNUmakefile"

# Softmmu target per platform (resolved via IB_PLATFORM/OVERRIDES). The
# bbclass builds this target and keeps any other arch already built, so
# building one platform's qemu doesn't wipe the other's.

QEMU_TARGET:virt32 = "arm-softmmu"
QEMU_TARGET:virt64 = "aarch64-softmmu"
QEMU_OPTS = "--enable-slirp --disable-attr --disable-werror --disable-docs --enable-sdl"

do_clean[nostamp] = "1"
do_clean () {
	rm -f ${TMPDIR}/stamps/qemu*
}
addtask do_clean
